/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/FaxServer.c++,v 1.1.1.1 1994/01/14 23:09:48 torek Exp $
/*
 * Copyright (c) 1990, 1991, 1992, 1993 Sam Leffler
 * Copyright (c) 1991, 1992, 1993 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */
#include <osfcn.h>
#include <ctype.h>
#include <termios.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <Dispatch/dispatcher.h>

#include "t.30.h"
#include "tiffio.h"
#include "FaxServer.h"
#include "faxServerApp.h"
#include "FaxRecvInfo.h"
#include "RegExArray.h"
#include "Getty.h"
#include "UUCPLock.h"
#include "DialRules.h"
#include "config.h"
#include "flock.h"

#ifndef MAXHOSTNAMELEN
#define	MAXHOSTNAMELEN	64
#endif
#ifndef O_NOCTTY
#define	O_NOCTTY	0		// no POSIX O_NOCTTY support
#endif

/*
 * FAX Modem Server.
 */

extern	void fxFatal(const char* va_alist ...);

int FaxServer::pageWidthCodes[8] = {
    1728,	// 1728 in 215 mm line
    2048,	// 2048 in 255 mm line
    2432,	// 2432 in 303 mm line
    1216,	// 1216 in 151 mm line
    864,	// 864 in 107 mm line
    1728,	// undefined
    1728,	// undefined
    1728,	// undefined
};
int FaxServer::pageLengthCodes[4] = {
    297,	// A4 paper
    364,	// B4 paper
    -1,		// unlimited
    -1		// undefined
};

// map FaxModem::BaudRate to numeric value
static u_int baudRates[] = {
    0,		// BR0
    300,	// BR300
    1200,	// BR1200
    2400,	// BR2400
    4800,	// BR4800
    9600,	// BR9600
    19200,	// BR19200
    38400,	// BR38400
    57600,	// BR57600
};

FaxServer::FaxServer(faxServerApp* a, const fxStr& devName, const fxStr& devID)
    : modemDevice(devName), modemDevID(devID)
{
    state = BASE;
    app = a;
    getty = NULL;
    statusFile = NULL;
    abortCall = FALSE;
    deduceComplain = TRUE;		// first failure causes complaint
    modemFd = -1;
    modemLock = OSnewUUCPLock(devName);
    modem = NULL;
    lastConfigModTime = 0;
    speakerVolume = FaxModem::QUIET;	// default speaker volume
    curRate = FaxModem::BR0;		// unspecified baud rate
    ringsBeforeAnswer = 0;		// default is not to answer phone
    noCarrierRetrys = 1;		// retry sends once if no carrier
    recvFileMode = 0600;		// default protection mode
    deviceMode = 0600;			// default mode for modem device
    logMode = 0600;			// default mode for log files
    tracingLevel = FAXTRACE_SERVER;
    logTracingLevel = FAXTRACE_SERVER;
    adaptiveAnswer = FALSE;		// don't answer data if fax answer fails
    answerBias = -1;
    setAnswerRotary("any");
    rcvNext = rcvCC = 0;
    lastPatModTime = 0;
    tsiPats = NULL;
    log = NULL;
    dialRules = NULL;
}

FaxServer::~FaxServer()
{
    close();
    delete modemLock;
    delete tsiPats;
    delete getty;
    delete log;
    delete dialRules;
}

/*
 * Startup the server for the first time.
 */
void
FaxServer::open()
{
    if (modemLock->lock()) {
	fxBool modemReady = setupModem();
	modemLock->unlock();
	if (!modemReady)
	    changeState(MODEMWAIT, TIMER_POLLMODEM);
	else
	    changeState(RUNNING, 0);
    } else {
	traceStatus(FAXTRACE_SERVER, "%s: Can not lock device.",
	    (char*) modemDevice);
	changeState(LOCKWAIT, TIMER_POLLMODEM);
    }
}

/*
 * Close down the server.
 */
void
FaxServer::close()
{
    if (modemLock->lock()) {
	if (modem)
	    modem->hangup();
	discardModem(TRUE);
	modemLock->unlock();
    }
}

/*
 * Initialize the server from command line arguments.
 */
void
FaxServer::initialize(int argc, char** argv)
{
    extern int optind, opterr;
    extern char* optarg;
    int c;
    optind = 1;				// 'cuz we're using getopt twice
    opterr = 0;
    while ((c = getopt(argc, argv, "m:g:i:q:d")) != -1)
	switch (c) {
	case 'g':
	    if (*optarg == '\0')
		fxFatal("No getty speed specified");
	    gettyArgs = optarg;
	    break;
	}
    TIFFSetErrorHandler(NULL);
    TIFFSetWarningHandler(NULL);
    updateTSIPatterns();
    // setup server's status file
    fxStr file(FAX_STATUSDIR);
    file.append("/" | modemDevID);
    statusFile = fopen(file, "w");
    if (statusFile != NULL) {
#if HAS_FCHMOD
	fchmod(fileno(statusFile), 0644);
#else
	chmod((char*) file, 0644);
#endif
	setServerStatus("Initializing server");
    }
    hostname.resize(MAXHOSTNAMELEN);
    if (gethostname((char*) hostname, MAXHOSTNAMELEN) == 0)
	hostname.resize(strlen(hostname));
    ::umask(077);			// keep all temp files private
}

const char* FaxServer::stateNames[8] = {
    "BASE",
    "RUNNING",
    "MODEMWAIT",
    "LOCKWAIT",
    "GETTYWAIT",
    "SENDING",
    "ANSWERING",
    "RECEIVING"
};
const char* FaxServer::stateStatus[8] = {
    "Initializing server and modem",		// BASE
    "Running and idle",				// RUNNING
    "Waiting for modem to come ready",		// MODEMWAIT
    "Waiting for modem to come free",		// LOCKWAIT
    "Waiting for login session to terminate",	// GETTYWAIT
    "Sending facsimile",			// SENDING
    "Answering the phone",			// ANSWERING
    "Receiving facsimile",			// RECEIVING
};

/*
 * Change the server's state and, optionally,
 * start a timer running for timeout seconds.
 */
void
FaxServer::changeState(FaxServerState s, long timeout)
{
    if (s != state) {
	if (timeout)
	    traceStatus(FAXTRACE_STATETRANS,
		"STATE CHANGE: %s -> %s (timeout %ld)",
		stateNames[state], stateNames[s], timeout);
	else
	    traceStatus(FAXTRACE_STATETRANS, "STATE CHANGE: %s -> %s",
		stateNames[state], stateNames[s]);
	state = s;
	setProcessPriority(state);
	setServerStatus(stateStatus[state]);
	if (s == RUNNING)
	    app->notifyModemReady();		// notify surrogate
    }
    if (timeout)
	Dispatcher::instance().startTimer(timeout, 0, this);
}

#ifdef sgi
#include <sys/schedctl.h>
/*
 * When low latency is required, use a nondegrading process
 * priority; otherwise just remove any nondegrading priority.
 * Note that we assign a high nondegrading priority when sending,
 * answering the telephone, or receiving.  We assume that if the
 * incoming call spawns a getty process that the priority will
 * be reset in the child before the getty is exec'd.
 */
static const int schedCtlParams[8][2] = {
    { NDPRI, 0 },		// BASE
    { NDPRI, 0 },		// RUNNING
    { NDPRI, 0 },		// MODEMWAIT
    { NDPRI, 0 },		// LOCKWAIT
    { NDPRI, 0 },		// GETTYWAIT
    { NDPRI, NDPHIMIN },	// SENDING
    { NDPRI, NDPHIMIN },	// ANSWERING
    { NDPRI, NDPHIMIN },	// RECEIVING
};
#elif svr4 
extern "C" {
#include <sys/priocntl.h>
#include <sys/rtpriocntl.h>
#include <sys/tspriocntl.h>
}
static struct SchedInfo {
    const char*	clname;		// scheduling class name
    int		params[3];	// scheduling class parameters
} schedInfo[8] = {
    { "TS", { TS_NOCHANGE, TS_NOCHANGE } },		// BASE
    { "TS", { TS_NOCHANGE, TS_NOCHANGE } },		// RUNNING
    { "TS", { TS_NOCHANGE, TS_NOCHANGE } },		// MODEMWAIT
    { "TS", { TS_NOCHANGE, TS_NOCHANGE } },		// LOCKWAIT
    { "TS", { TS_NOCHANGE, TS_NOCHANGE } },		// GETTYWAIT
    { "RT", { RT_NOCHANGE, RT_NOCHANGE, RT_NOCHANGE } },// SENDING
    { "RT", { RT_NOCHANGE, RT_NOCHANGE, RT_NOCHANGE } },// ANSWERING
    { "RT", { RT_NOCHANGE, RT_NOCHANGE, RT_NOCHANGE } },// RECEIVING
};
#endif

void
FaxServer::setProcessPriority(FaxServerState s)
{
#ifdef sgi
    uid_t euid = geteuid();
    if (seteuid(0) >= 0) {		// must be done as root
	if (schedctl(schedCtlParams[s][0], 0, schedCtlParams[s][1]) < 0)
	    traceStatus(FAXTRACE_SERVER, "schedctl: %m");
	if (seteuid(euid) < 0)		// restore previous effective uid
	    traceStatus(FAXTRACE_SERVER, "seteuid(%d): %m", euid);
    } else
	traceStatus(FAXTRACE_SERVER, "seteuid(root): %m");
#elif svr4
    uid_t euid = geteuid();
    if (seteuid(0) >= 0) {		// must be done as root
	const SchedInfo& si = schedInfo[s];
	pcinfo_t pcinfo;
	strcpy(pcinfo.pc_clname, si.clname);
	if (priocntl(0, 0, PC_GETCID, (caddr_t)&pcinfo) >= 0) {
	    pcparms_t pcparms;
	    pcparms.pc_cid = pcinfo.pc_cid;
	    if (strcmp(si.clname,"RT") == 0) {
		rtparms_t* rtp = (rtparms_t*) pcparms.pc_clparms;
		rtp->rt_pri	= si.params[0];
		rtp->rt_tqsecs	= (ulong) si.params[1];
		rtp->rt_tqnsecs	= si.params[2];
	    } else {
		tsparms_t* tsp = (tsparms_t*) pcparms.pc_clparms;
		tsp->ts_uprilim	= si.params[0];
		tsp->ts_upri	= si.params[1];
	    }
	    if (priocntl(P_PID, P_MYID, PC_SETPARMS, (caddr_t)&pcparms) < 0)
		traceStatus(FAXTRACE_SERVER,
		    "Unable to set %s scheduling parameters: %m", si.clname);
	} else
	    traceStatus(FAXTRACE_SERVER, "priocntl(%s): %m", si.clname);
	if (seteuid(euid) < 0)		// restore previous effective uid
	    traceStatus(FAXTRACE_SERVER, "setreuid(%d): %m", euid);
    } else
	traceStatus(FAXTRACE_SERVER, "setreuid(root): %m");
#endif
}

/*
 * Record the server status in the status file.
 */
void
FaxServer::setServerStatus(const char* fmt, ...)
{
    if (statusFile == NULL)
	return;
    flock(fileno(statusFile), LOCK_EX);
    fseek(statusFile, 0L, 0);
    ftruncate(fileno(statusFile), 0);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(statusFile, fmt, ap);
    fprintf(statusFile, "\n");
    fflush(statusFile);
    flock(fileno(statusFile), LOCK_UN);
}

/*
 * Setup the modem; if needed.  Note that we reread
 * the configuration file if it's been changed prior
 * to setting up the modem.  This makes it easy to
 * swap modems that need different configurations
 * just by yanking the cable and then swapping the
 * config file before hooking up the new modem.
 */
fxBool
FaxServer::setupModem()
{
    /*
     * Possibly reread the configuration file.  We
     * always do this prior to setting up the modem
     * so that someone can tweak the configuration file
     * and have the server automatically select new
     * modem configuration parameters (instead of
     * requiring that the server be restarted).
     */
    fxStr file(FAX_CONFIG);
    file.append("." | modemDevID);
    if (restoreState(file) && modem)
	discardModem(TRUE);
    if (!modem) {
	const char* dev = modemDevice;
	if (!openDevice(dev))
	    return (FALSE);
	/*
	 * Deduce modem type and setup configuration info.
	 * The deduceComplain cruft is just to reduce the
	 * noise in the log file when probing for a modem.
	 */
	modem = FaxModem::deduceModem(*this, modemConfig);
	if (!modem) {
	    discardModem(TRUE);
	    if (deduceComplain) {
		traceStatus(FAXTRACE_SERVER,
		    "%s: Can not deduce modem type.", dev);
		deduceComplain = FALSE;
	    }
	    return (FALSE);
	} else {
	    deduceComplain = TRUE;
	    traceStatus(FAXTRACE_SERVER, "MODEM %s %s/%s",
		(char*) modem->getManufacturer(),
		(char*) modem->getModel(),
		(char*) modem->getRevision());
	}
    } else
	/*
	 * Reset the modem in case some other program
	 * went in and messed with the configuration.
	 */
	modem->reset();
    /*
     * Most modem-related parameters are dealt with
     * in the modem driver.  The speaker volume is
     * kept in the fax server because it often gets
     * changed on the fly.  The phone number has no
     * business in the modem class.
     */
    modem->setSpeakerVolume(speakerVolume);
    modem->setLID(canonicalizePhoneNumber(FAXNumber));
    /*
     * If the server is configured, listen for incoming calls.
     */
    setRingsBeforeAnswer(ringsBeforeAnswer);
    return (TRUE);
}

/*
 * Open the tty device associated with the modem
 * and change the device file to be owned by us
 * and with a protected file mode.
 */
fxBool
FaxServer::openDevice(const char* dev)
{
    /*
     * Temporarily become root to open the device.
     * Routines that call setupModem *must* first
     * lock the device with the usual effective uid.
     */
    uid_t euid = geteuid();
    if (seteuid(0) < 0) {
	 traceStatus(FAXTRACE_SERVER, "%s: seteuid root failed (%m)", dev);
	 return (FALSE);
    }
#ifdef O_NDELAY
    /*
     * Open device w/ O_NDELAY to bypass modem
     * control signals, then turn off the flag bit.
     */
    modemFd = ::open(dev, O_RDWR|O_NDELAY|O_NOCTTY);
    if (modemFd < 0) {
	seteuid(euid);
	traceStatus(FAXTRACE_SERVER, "%s: Can not open modem (%m)", dev);
	return (FALSE);
    }
    int flags = fcntl(modemFd, F_GETFL, 0);
    if (fcntl(modemFd, F_SETFL, flags &~ O_NDELAY) < 0) {
	 traceStatus(FAXTRACE_SERVER, "%s: fcntl: %m", dev);
	 ::close(modemFd), modemFd = -1;
	 return (FALSE);
    }
#else
    startTimeout(3*1000);
    modemFd = ::open(dev, O_RDWR);
    stopTimeout("opening modem");
    if (modemFd < 0) {
	seteuid(euid);
	traceStatus(FAXTRACE_SERVER,
	    (timeout ?
		"%s: Can not open modem (timed out)." :
		"%s: Can not open modem (%m)."),
	    dev, errno);
	return (FALSE);
    }
#endif
    /*
     * NB: we stat and use the gid because passing -1
     *     through the gid_t parameter in the prototype
     *	   causes it to get truncated to 65535.
     */
    struct stat sb;
    (void) fstat(modemFd, &sb);
#if HAS_FCHOWN
    if (fchown(modemFd, UUCPLock::getUUCPUid(), sb.st_gid) < 0)
#else
    if (chown((char*) dev, UUCPLock::getUUCPUid(), sb.st_gid) < 0)
#endif
	traceStatus(FAXTRACE_SERVER, "%s: chown: %m", dev);
#if HAS_FCHMOD
    if (fchmod(modemFd, deviceMode) < 0)
#else
    if (chmod((char*) dev, deviceMode) < 0)
#endif
	traceStatus(FAXTRACE_SERVER, "%s: chmod: %m", dev);
    seteuid(euid);
    return (TRUE);
}

fxBool
FaxServer::reopenDevice()
{
    if (modemFd >= 0)
	::close(modemFd), modemFd = -1;
    return openDevice(modemDevice);
}

/*
 * Discard any handle on the modem.
 */
void
FaxServer::discardModem(fxBool dropDTR)
{
    if (modemFd >= 0) {
	if (Dispatcher::instance().handler(modemFd, Dispatcher::ReadMask))
	    Dispatcher::instance().unlink(modemFd);
	if (dropDTR)
	    (void) setDTR(FALSE);			// force hangup
	::close(modemFd), modemFd = -1;			// discard open file
    }
    delete modem, modem = NULL;
}

/*
 * Return true if a request has been made to abort
 * the current session.  This is true if a previous
 * abort request was made or if an external abort
 * message is dispatched during our processing.
 */
fxBool
FaxServer::abortRequested()
{
#ifndef AIXV3						/* AIX is busted */
    if (!abortCall) {
	// poll for input so abort commands get processed
	long sec = 0;
	long usec = 0;
	while (Dispatcher::instance().dispatch(sec,usec) && !abortCall)
	    ;
    }
#endif
    return (abortCall);
}

void
FaxServer::abortSession()
{
    abortCall = TRUE;
    traceStatus(FAXTRACE_SERVER, "ABORT: job abort requested");
}

/*
 * Dispatcher input ready routine.  This is usually called
 * because the phone is ringing, but it can also be called
 * when, for example, a process dials out on the modem.
 */
int
FaxServer::inputReady(int)
{
    answerPhone(FaxModem::ANSTYPE_ANY, FALSE);
    return (0);
}

/*
 * Dispatcher timer expired routine.  Perform the action
 * associated with the server's state and, possible, transition
 * to a new state.
 */
void
FaxServer::timerExpired(long, long)
{
    switch (state) {
    case MODEMWAIT:
    case LOCKWAIT:
	/*
	 * Waiting for modem to start working.  Retry setup
	 * and either change state or restart the timer.
	 * Note that we unlock the modem before we change
	 * our state to RUNNING after a modem setup so that
	 * any callback doesn't find the modem locked (and
	 * so cause jobs to be requeued).
	 */
	if (modemLock->lock()) {
	    fxBool modemReady = setupModem();
	    modemLock->unlock();
	    if (modemReady)
		changeState(RUNNING, 0);
	    else
		changeState(MODEMWAIT, TIMER_POLLMODEM);
	} else
	    changeState(LOCKWAIT, TIMER_POLLMODEM);
	break;
    }
}

void
FaxServer::childStatus(int, int status)
{
    switch (state) {
    case GETTYWAIT:
	/*
	 * Waiting for getty/login process to terminate.
	 * Unlock the modem and reset the world because
	 * we discarded modem state when we started the getty.
	 */
	traceStatus(FAXTRACE_SERVER, "GETTY: exit status %#o", status);
	delete getty, getty = NULL;
	modemLock->unlock();		// it's safe now to remove the lock
	changeState(MODEMWAIT, 2);	// wait a touch for the modem to settle
	break;
    }
}

/*
 * Answer the telephone in response to data from the modem
 * (e.g. a "RING" message) or an explicit command from the
 * user (sending an "ANSWER" command through the FIFO).
 */
void
FaxServer::answerPhone(AnswerType atype, fxBool force)
{
    if (modemLock->lock()) {
	changeState(ANSWERING);
	if (force || modem->waitForRings(ringsBeforeAnswer)) {
	    fxStr emsg;
	    fxStr canon(canonicalizePhoneNumber(FAXNumber));
	    log = new FaxMachineLog(canon, logMode);
	    /*
	     * If requested, rotate the way we answer the phone
	     * if the request type is "any".  This should probably
	     * only be used if the modem does not directly support
	     * adaptive-answer.
	     */
	    if (atype == FaxModem::ANSTYPE_ANY)
		atype = answerRotary[answerRotor];
	    CallType ctype = modem->answerCall(atype, emsg);
	    fxBool waitForProcess;
	    fxBool callSetup = setupCall(atype, ctype, waitForProcess, emsg);
	    if (!callSetup && ctype == FaxModem::CALLTYPE_FAX && adaptiveAnswer) {
		/*
		 * Status indicated a call from a fax machine, but we
		 * were unable to complete the initial handshake.
		 * If we are doing adaptive answer, immediately
		 * retry answering the call as data w/o hanging up
		 * the phone (note that this depends on characteristics
		 * of the local phone system).
		 */
		atype = FaxModem::ANSTYPE_DATA;
		ctype = modem->answerCall(atype, emsg);
		callSetup = setupCall(atype, ctype, waitForProcess, emsg);
	    }
	    /*
	     * Call resolved.  If we were able to recognize the call
	     * type and setup a session, then reset the answer rotary
	     * state if there is a bias toward a specific answer type.
	     * Also, deal with call types that are processed through
	     * a subprocess, rather than within this process.  Otherwise,
	     * if the call failed, advance the rotor to the next answer
	     * type in preparation for the next call.
	     */
	    if (callSetup) {
		if (answerBias >= 0)
		    answerRotor = answerBias;
		/*
		 * Some calls are handled by starting up a subprocess
		 * that does the work.  For such calls we have to wait
		 * for the process to exit before we can remove the
		 * lock file and do related cleanup work.
		 */
		if (waitForProcess)
		    return;
	    } else
		answerRotor = (answerRotor+1) % answerRotorSize;
	    delete log, log = NULL;
	} else
	    modemFlushInput();
	/*
	 * Because some modems are impossible to safely hangup in the
	 * event of a problem, we force a close on the device so that
	 * the modem will see DTR go down and (hopefully) clean up any
	 * bad state its in.  We then immediately try to setup the modem
	 * again so that we can be ready to answer incoming calls again.
	 */
	modem->hangup();
	discardModem(TRUE);
	fxBool modemReady = setupModem();
	modemLock->unlock();
	if (!modemReady)
	    changeState(MODEMWAIT, TIMER_POLLMODEM);
	else
	    changeState(RUNNING);
    } else {
	/*
	 * The modem is in use to call out, or by way of an incoming
	 * call.  If we're not sending or receiving, discard our handle
	 * on the modem and change to MODEMWAIT state where we wait
	 * for the modem to come available again.
	 */
	traceStatus(FAXTRACE_SERVER, "ANSWER: Can not lock modem device");
	if (state != SENDING && state != ANSWERING) {
	    discardModem(FALSE);
	    changeState(LOCKWAIT, TIMER_POLLMODEM);
	}
    }
}

/*
 * Do setup after answering an incoming call.
 */
fxBool
FaxServer::setupCall(AnswerType atype, CallType ctype, fxBool& waitForProcess,
    fxStr& emsg)
{
    fxBool callSetup = FALSE;
    waitForProcess = FALSE;

    switch (ctype) {
    case FaxModem::CALLTYPE_FAX:
	traceStatus(FAXTRACE_SERVER, "ANSWER: FAX CONNECTION");
	callSetup = recvFax();
	break;
    case FaxModem::CALLTYPE_DATA:
	traceStatus(FAXTRACE_SERVER, "ANSWER: DATA CONNECTION");
	if (gettyArgs == "") {
	    traceStatus(FAXTRACE_SERVER,
		"ANSWER: Data connections are not permitted");
	    break;
	}
	/*
	 * If call was answered using an adaptive-answering
	 * facility, then give the modem an opportunity to
	 * establish data services.
	 */
	if (atype == FaxModem::ANSTYPE_ANY && !modem->dataService()) {
	    traceStatus(FAXTRACE_SERVER,
		"ANSWER: Could not switch modem to data service");
	    break;
	}
	/*
	 * Fork and exec a getty process to handle the
	 * data connection.  Note that we return without
	 * removing our lock on the modem--this is done
	 * after we reap the child getty process to insure
	 * outgoing modem use is disallowed.
	 */
	if (runGetty(gettyArgs)) {
	    delete log, log = NULL;
	    callSetup = TRUE;
	    waitForProcess = TRUE;
	}
	break;
    case FaxModem::CALLTYPE_VOICE:
	traceStatus(FAXTRACE_SERVER, "ANSWER: VOICE CONNECTION");
	/*
	 * If call was answered using an adaptive-answering
	 * facility, then give the modem an opportunity to
	 * establish voice services.
	 */
	if (atype == FaxModem::ANSTYPE_ANY && !modem->voiceService()) {
	    traceStatus(FAXTRACE_SERVER,
		"ANSWER: Could not switch modem to voice service");
	    break;
	}
	// XXX setup voice process a la getty
	break;
    case FaxModem::CALLTYPE_ERROR:
	traceStatus(FAXTRACE_SERVER, "ANSWER: %s", (char*) emsg);
	break;
    }
    return (callSetup);
}

/*
 * Startup a getty process in response to a data connection.
 * The speed parameter is passed to getty to use in establishing
 * a login session.
 */
fxBool
FaxServer::runGetty(const char* args)
{
    fxStr prefix(DEV_PREFIX);
    fxStr dev(modemDevice);
    if (dev.head(prefix.length()) == prefix)
	dev.remove(0, prefix.length());
    getty = OSnewGetty(dev, fxStr((int) baudRates[curRate], "%u"));
    if (!getty) {
	traceStatus(FAXTRACE_SERVER, "GETTY: could not create");
	return (FALSE);
    }
    pid_t pid = fork();
    if (pid == -1) {
	traceStatus(FAXTRACE_SERVER, "GETTY: can not fork");
	return (FALSE);
    }
    if (pid == 0) {
	setProcessPriority(BASE);		// remove any high priority
	if (setegid(getgid()) < 0)
	    traceStatus(FAXTRACE_SERVER, "runGetty::setegid: %m");
	if (seteuid(getuid()) < 0)
	    traceStatus(FAXTRACE_SERVER, "runGetty::seteuid: %m");
	getty->run(modemFd, args);
	_exit(127);
	/*NOTREACHED*/
    } else {
	traceStatus(FAXTRACE_SERVER, "GETTY: start pid %u, \"%s\"",
	   pid, args);
	getty->setPID(pid);
	/*
	 * Purge existing modem state because the getty+login processed
	 * will change everything and because we must close the descriptor
	 * so that the getty will get SIGHUP on last close.
	 */
	discardModem(FALSE);
	changeState(GETTYWAIT);
	Dispatcher::instance().startChild(pid, this);
	return (TRUE);
    }
}

void
FaxServer::setDialRules(const char* name)
{
    if (dialRules)
	delete dialRules, dialRules = NULL;
    dialRules = new DialStringRules(name);
    /*
     * Setup configuration environment.
     */
    dialRules->def("AreaCode", myAreaCode);
    dialRules->def("CountryCode", myCountryCode);
    dialRules->def("LongDistancePrefix", longDistancePrefix);
    dialRules->def("InternationalPrefix", internationalPrefix);
    if (!dialRules->parse())
	traceStatus(FAXTRACE_SERVER,
	    "Parse error in dial string rules \"%s\"", name);
}

/*
 * Convert a dialing string to a canonical format.
 */
fxStr
FaxServer::canonicalizePhoneNumber(const fxStr& ds)
{
    if (dialRules)
	return dialRules->canonicalNumber(ds);
    else
	return ds;
}

/*
 * Prepare a dialing string for use.
 */
fxStr
FaxServer::prepareDialString(const fxStr& ds)
{
    if (dialRules)
	return dialRules->dialString(ds);
    else
	return ds;
}

/*
 * Modem support interfaces.  Note that the values
 * returned when we don't have a handle on the modem
 * are selected so that any imaged facsimile should
 * still be sendable.
 */
fxBool FaxServer::modemReady() const
    { return modem != NULL; }
fxBool FaxServer::modemSupports2D() const
    { return modem ? modem->supports2D() : FALSE; }
fxBool FaxServer::modemSupportsEOLPadding() const
    { return modem ? modem->supportsEOLPadding() : FALSE; }
fxBool FaxServer::modemSupportsVRes(float res) const
    { return modem ? modem->supportsVRes(res) : TRUE; }
fxBool FaxServer::modemSupportsPageWidth(u_int w) const
    { return modem ? modem->supportsPageWidth(w) : TRUE; }
fxBool FaxServer::modemSupportsPageLength(u_int l) const
    { return modem ? modem->supportsPageLength(l) : TRUE; }

fxBool FaxServer::serverBusy() const
    { return state != RUNNING; }

/*
 * FaxServer configuration and query methods.
 */

void
FaxServer::setModemNumber(const fxStr& number)
{
    FAXNumber = number;
    if (modem)
	modem->setLID(canonicalizePhoneNumber(number));
}
const fxStr& FaxServer::getModemNumber()	{ return (FAXNumber); }

void FaxServer::setServerTracing(int level)	{ tracingLevel = level; }
int FaxServer::getServerTracing()		{ return (tracingLevel); }
void FaxServer::setSessionTracing(int level)	{ logTracingLevel = level; }
int FaxServer::getSessionTracing()		{ return (logTracingLevel); }

void
FaxServer::setRingsBeforeAnswer(int rings)
{
    ringsBeforeAnswer = rings;
    if (modem) {
	IOHandler* h =
	    Dispatcher::instance().handler(modemFd, Dispatcher::ReadMask);
	if (rings > 0 && h == NULL)
	    Dispatcher::instance().link(modemFd, Dispatcher::ReadMask, this);
	else if (rings == 0 && h != NULL)
	    Dispatcher::instance().unlink(modemFd);
    }
}
int FaxServer::getRingsBeforeAnswer()		{ return (ringsBeforeAnswer); }

void
FaxServer::setModemSpeakerVolume(SpeakerVolume lev)
{
    speakerVolume = lev;
    if (modem)
	modem->setSpeakerVolume(lev);
}
SpeakerVolume FaxServer::getModemSpeakerVolume(){ return (speakerVolume); }

static SpeakerVolume
getVol(const char* cp)
{
    return (strcasecmp(cp, "off") == 0	 ? FaxModem::OFF :
	    strcasecmp(cp, "quiet") == 0 ? FaxModem::QUIET :
	    strcasecmp(cp, "low") == 0	 ? FaxModem::LOW :
	    strcasecmp(cp, "medium") == 0? FaxModem::MEDIUM :
	    strcasecmp(cp, "high") == 0	 ? FaxModem::HIGH :
					   (SpeakerVolume) atoi(cp));
}

fxBool
FaxServer::restoreState(const fxStr& filename)
{
    fxBool didSomething = FALSE;
    FILE* fd = fopen(filename, "r");
    if (fd) {
	struct stat sb;
	if (fstat(fileno(fd), &sb) >= 0 && sb.st_mtime != lastConfigModTime) {
	    char line[512];
	    while (fgets(line, sizeof (line)-1, fd))
		restoreStateItem(line);
	    lastConfigModTime = sb.st_mtime;
	    didSomething = TRUE;
	}
	fclose(fd);
    }
    return (didSomething);
}

static int
getnum(const char* s)
{
    return ((int) strtol(s, NULL, 0));
}

static fxBool
getbool(const char* cp)
{
    return (streq(cp, "on") || streq(cp, "yes"));
}

/*
 * Process an answer rotary spec string.
 */
void
FaxServer::setAnswerRotary(const fxStr& value)
{
    u_int l = 0;
    for (u_int i = 0; i < 3 && l < value.length(); i++) {
	fxStr type(value.token(l, " \t"));
	type.raisecase();
	if (type == "FAX")
	    answerRotary[i] = FaxModem::ANSTYPE_FAX;
	else if (type == "DATA")
	    answerRotary[i] = FaxModem::ANSTYPE_DATA;
	else if (type == "VOICE")
	    answerRotary[i] = FaxModem::ANSTYPE_VOICE;
	else {
	    if (type != "ANY")
		traceStatus(FAXTRACE_SERVER,
		    "Unknown answer type \"%s\"", (char*) type);
	    answerRotary[i] = FaxModem::ANSTYPE_ANY;
	}
    }
    if (i == 0)				// void string
	answerRotary[i++] = FaxModem::ANSTYPE_ANY;
    answerRotor = 0;
    answerRotorSize = i;
}

void
FaxServer::restoreStateItem(const char* b)
{
    char buf[512];
    char* cp;

    strncpy(buf, b, sizeof (buf));
    for (cp = buf; isspace(*cp); cp++)		// skip leading white space
	;
    if (*cp == '#')
	return;
    const char* tag = cp;			// start of tag
    while (*cp && *cp != ':') {			// skip to demarcating ':'
	if (isupper(*cp))
	    *cp = tolower(*cp);
	cp++;
    }
    if (*cp != ':') {
	traceStatus(FAXTRACE_SERVER, "Syntax error, missing ':' in \"%s\"", b);
	return;
    }
    for (*cp++ = '\0'; isspace(*cp); cp++)	// skip white space again
	;
    const char* value;
    if (*cp == '"') {				// "..." value
	int c;
	/*
	 * Parse quoted string and deal with \ escapes.
	 */
	char* dp = ++cp;
	for (value = dp; (c = *cp) != '"'; cp++) {
	    if (c == '\0') {			// unmatched quote mark
		traceStatus(FAXTRACE_SERVER,
		    "Syntax error, missing quote mark in \"%s\"", b);
		return;
	    }
	    if (c == '\\') {
		c = *++cp;
		if (isdigit(c)) {		// \nnn octal escape
		    int v = c - '0';
		    if (isdigit(c = cp[1])) {
			cp++, v = (v << 3) + (c - '0');
			if (isdigit(c = cp[1]))
			    cp++, v = (v << 3) + (c - '0');
		    }
		    c = v;
		} else {			// \<char> escapes
		    for (const char* tp = "n\nt\tr\rb\bf\fv\013"; *tp; tp += 2)
			if (c == tp[0]) {
			    c = tp[1];
			    break;
			}
		}
	    }
	    *dp++ = c;
	}
	*dp = '\0';
    } else {					// value up to 1st non-ws
	for (value = cp; *cp && !isspace(*cp); cp++)
	    ;
	*cp = '\0';
    }
    if (streq(tag, "recvfilemode"))	
	recvFileMode = (mode_t) strtol(value, 0, 8);
    else if (streq(tag, "devicemode"))	
	deviceMode = (mode_t) strtol(value, 0, 8);
    else if (streq(tag, "logfilemode"))	
	logMode = (mode_t) strtol(value, 0, 8);
    else if (streq(tag, "ringsbeforeanswer"))
	setRingsBeforeAnswer(getnum(value));
    else if (streq(tag, "speakervolume"))
	setModemSpeakerVolume(getVol(value));
    else if (streq(tag, "protocoltracing"))
	tracingLevel = logTracingLevel = getnum(value);
    else if (streq(tag, "servertracing"))	tracingLevel = getnum(value);
    else if (streq(tag, "sessiontracing"))	logTracingLevel = getnum(value);
    else if (streq(tag, "faxnumber"))		setModemNumber(value);
    else if (streq(tag, "areacode"))		myAreaCode = value;
    else if (streq(tag, "countrycode"))		myCountryCode = value;
    else if (streq(tag, "longdistanceprefix"))	longDistancePrefix = value;
    else if (streq(tag, "internationalprefix"))	internationalPrefix = value;
    else if (streq(tag, "dialstringrules"))	setDialRules(value);
    else if (streq(tag, "qualifytsi"))		qualifyTSI = value;
    else if (streq(tag, "gettyspeed"))		gettyArgs = value;
    else if (streq(tag, "gettyargs"))		gettyArgs = value;
    else if (streq(tag, "nocarrierretrys"))	noCarrierRetrys = getnum(value);
    else if (streq(tag, "adaptiveanswer"))	adaptiveAnswer = getbool(value);
    else if (streq(tag, "answerrotary"))	setAnswerRotary(value);
    else if (streq(tag, "answerbias"))
	answerBias = fxmin(getnum(value),2);
    else if (!modemConfig.parseItem(tag, value))
	traceStatus(FAXTRACE_SERVER,
	    "Unknown configuration parameter \"%s\" ignored", b);
}

void
FaxServer::traceStatus(int kind, const char* va_alist ...)
#define	fmt va_alist
{
    va_list ap;
    va_start(ap, fmt);
    vtraceStatus(kind, fmt, ap);
    va_end(ap);
}
#undef fmt

#include "faxServerApp.h"

void
FaxServer::vtraceStatus(int kind, const char* fmt, va_list ap)
{
    if (log) {
	if (kind == FAXTRACE_SERVER)	// always log server stuff
	    app->vlogInfo(fmt, ap);
	if (logTracingLevel & kind)
	    log->vlog(fmt, ap);
    } else if (tracingLevel & kind)
	app->vlogInfo(fmt, ap);
}

#include "StackBuffer.h"

void
FaxServer::traceModemIO(const char* dir, const u_char* data, u_int cc)
{
    if (log) {
	if ((logTracingLevel& FAXTRACE_MODEMIO) == 0)
	    return;
    } else if ((tracingLevel & FAXTRACE_MODEMIO) == 0)
	return;

    const char* hexdigits = "0123456789ABCDEF";
    fxStackBuffer buf;
    for (u_int i = 0; i < cc; i++) {
	u_char b = data[i];
	if (i > 0)
	    buf.put(' ');
	buf.put(hexdigits[b>>4]);
	buf.put(hexdigits[b&0xf]);
    }
    traceStatus(FAXTRACE_MODEMIO, "%s <%u:%.*s>",
	dir, cc, buf.getLength(), (char*) buf);
}

#ifndef B38400
#define	B38400	B19200
#endif
#ifndef B57600
#define	B57600	B38400
#endif
static u_int termioBaud[] = {
    B0,		// BR0
    B300,	// BR300
    B1200,	// BR1200
    B2400,	// BR2400
    B4800,	// BR4800
    B9600,	// BR9600
    B19200,	// BR19200
    B38400,	// BR38400
    B57600,	// BR57600
};
#define	NBAUDS	(sizeof (termioBaud) / sizeof (termioBaud[0]))
static const char* flowNames[] =
    { "NONE", "CURRENT", "XON/XOFF", "RTS/CTS", };

#ifdef __bsdi__
#undef	CRTSCTS
#define	CRTSCTS	(CCTS_OFLOW|CRTS_IFLOW)
#endif
#ifndef CRTSCTS
#define	CRTSCTS	0
#endif

static void
setFlow(termios& term, FlowControl iflow, FlowControl oflow)
{
    switch (iflow) {
    case FaxModem::FLOW_CURRENT:
	break;
    case FaxModem::FLOW_NONE:
	term.c_iflag &= ~IXON;
	term.c_cflag &= ~CRTSCTS;
	break;
    case FaxModem::FLOW_XONXOFF:
	term.c_iflag |= IXON;
	term.c_cflag &= ~CRTSCTS;
	break;
    case FaxModem::FLOW_RTSCTS:
	term.c_iflag &= ~IXON;
	term.c_cflag |= CRTSCTS;
	break;
    }
    switch (oflow) {
    case FaxModem::FLOW_CURRENT:
	break;
    case FaxModem::FLOW_NONE:
	term.c_iflag &= ~IXOFF;
	term.c_cflag &= ~CRTSCTS;
	break;
    case FaxModem::FLOW_XONXOFF:
	term.c_iflag |= IXOFF;
	term.c_cflag &= ~CRTSCTS;
	break;
    case FaxModem::FLOW_RTSCTS:
	term.c_iflag &= ~IXOFF;
	term.c_cflag |= CRTSCTS;
	break;
    }
}

/*
 * Device manipulation.
 */

/*
 * Set tty port baud rate and flow control.
 */
fxBool
FaxServer::setBaudRate(BaudRate rate, FlowControl iFlow, FlowControl oFlow)
{
    struct termios term;
    if (tcgetattr(modemFd, &term) == 0) {
	if (rate >= NBAUDS)
	    rate = NBAUDS-1;
	curRate = rate;				// NB: for use elsewhere
	term.c_oflag = 0;
	term.c_lflag = 0;
	term.c_iflag &= IXON|IXOFF;		// keep these bits
	term.c_cflag &= CRTSCTS;		// and these bits
	setFlow(term, iFlow, oFlow);
	term.c_cflag |= CLOCAL | CS8 | CREAD;
	cfsetospeed(&term, termioBaud[rate]);
	cfsetispeed(&term, termioBaud[rate]);
	term.c_cc[VMIN] = 127;			// buffer input by default
	term.c_cc[VTIME] = 1;
	traceStatus(FAXTRACE_MODEMOPS,
	    "MODEM set baud rate: %d baud, input flow %s, output flow %s",
	    baudRates[rate], flowNames[iFlow], flowNames[oFlow]);
	flushModemInput();
	return (tcsetattr(modemFd, TCSAFLUSH, &term) == 0);
    } else
	return (FALSE);
}

/*
 * Set tty port baud rate and leave flow control state unchanged.
 */
fxBool
FaxServer::setBaudRate(BaudRate rate)
{
    struct termios term;
    if (tcgetattr(modemFd, &term) == 0) {
	if (rate >= NBAUDS)
	    rate = NBAUDS-1;
	curRate = rate;				// NB: for use elsewhere
	term.c_oflag = 0;
	term.c_lflag = 0;
	term.c_iflag &= IXON|IXOFF;		// keep these bits
	term.c_cflag &= CRTSCTS;		// and these bits
	term.c_cflag |= CLOCAL | CS8 | CREAD;
	cfsetospeed(&term, termioBaud[rate]);
	cfsetispeed(&term, termioBaud[rate]);
	term.c_cc[VMIN] = 127;			// buffer input by default
	term.c_cc[VTIME] = 1;
	traceStatus(FAXTRACE_MODEMOPS,
	    "MODEM set baud rate: %d baud (flow control unchanged)",
	    baudRates[rate]);
	flushModemInput();
	return (tcsetattr(modemFd, TCSAFLUSH, &term) == 0);
    } else
	return (FALSE);
}

/*
 * Manipulate DTR on tty port.
 *
 * On systems that support explicit DTR control this is done
 * with an ioctl.  Otherwise we assume that setting the baud
 * rate to zero causes DTR to be dropped (asserting DTR is
 * assumed to be implicit in setting a non-zero baud rate).
 *
 * NB: we use the explicit DTR manipulation ioctls because
 *     setting the baud rate to zero on some systems can cause
 *     strange side effects.
 */
fxBool
FaxServer::setDTR(fxBool onoff)
{
    traceStatus(FAXTRACE_MODEMOPS, "MODEM set DTR %s", onoff ? "ON" : "OFF");
#ifdef TIOCMBIS
    int mctl = TIOCM_DTR;
#ifdef svr4
    /*
     * Happy days! SVR4 passes the arg by value, while
     * SunOS 4.x passes it by reference; is this progress?
     */
    if (ioctl(modemFd, onoff ? TIOCMBIS : TIOCMBIC, (char *)mctl) == 0)
#else
    if (ioctl(modemFd, onoff ? TIOCMBIS : TIOCMBIC, (char *)&mctl) == 0)
#endif
	return (TRUE);
    /*
     * Sigh, Sun seems to support this ioctl only on *some*
     * devices (e.g. on-board duarts, but not the ALM-2 card);
     * so if the ioctl that should work fails, we fallback
     * on the usual way of doing things...
     */
#endif
    return (onoff ? TRUE : setBaudRate(FaxModem::BR0));
}

static const char* actNames[] = { "NOW", "DRAIN", "FLUSH" };
static u_int actCode[] = { TCSANOW, TCSADRAIN, TCSAFLUSH };

/*
 * Set tty modes so that the specified handling
 * is done on data being sent and received.  When
 * transmitting binary data, oFlow is FLOW_NONE to
 * disable the transmission of XON/XOFF by the host
 * to the modem.  When receiving binary data, iFlow
 * is FLOW_NONE to cause XON/XOFF from the modem
 * to not be interpreted.  In each case the opposite
 * XON/XOFF handling should be enabled so that any
 * XON/XOFF from/to the modem will be interpreted.
 */
fxBool
FaxServer::setXONXOFF(FlowControl iFlow, FlowControl oFlow, SetAction act)
{
    struct termios term;
    if (tcgetattr(modemFd, &term) == 0) {
	setFlow(term, iFlow, oFlow);
	traceStatus(FAXTRACE_MODEMOPS,
	    "MODEM set XON/XOFF/%s: input %s, output %s",
	    actNames[act],
	    iFlow == FaxModem::FLOW_NONE ? "disabled" : "enabled",
	    oFlow == FaxModem::FLOW_NONE ? "disabled" : "enabled"
	);
	if (act == FaxModem::ACT_FLUSH)
	    flushModemInput();
	if (tcsetattr(modemFd, actCode[act], &term) == 0)
	    return (TRUE);
    }
    return (FALSE);
}

#ifdef sgi
#include <sys/stropts.h>
#include <sys/z8530.h>
#endif
#ifdef sun
#include <sys/stropts.h>
#endif

/*
 * Setup process state either for minimum latency (no buffering)
 * or reduced latency (input may be buffered).  We fiddle with
 * the termio structure and, if required, the streams timer
 * that delays the delivery of input data from the UART module
 * upstream to the tty module.
 */
fxBool
FaxServer::setInputBuffering(fxBool on)
{
    traceStatus(FAXTRACE_MODEMOPS, "MODEM input buffering %s",
	on ? "enabled" : "disabled");
#ifdef SIOC_ITIMER
    /*
     * Silicon Graphics systems have a settable timer
     * that causes the UART driver to delay passing
     * data upstream to the tty module.  This can cause
     * anywhere from 20-30ms delay between input characters.
     * We set it to zero when input latency is critical.
     */
    strioctl str;
    str.ic_cmd = SIOC_ITIMER;
    str.ic_timout = (on ? 2 : 0);	// 2 ticks = 20ms (usually)
    str.ic_len = 4;
    int arg = 0;
    str.ic_dp = (char*)&arg;
    if (ioctl(modemFd, I_STR, &str) < 0)
	traceStatus(FAXTRACE_MODEMOPS, "MODEM ioctl(SIOC_ITIMER): %m");
#endif
#ifdef sun
    /*
     * SunOS has a timer similar to the SIOC_ITIMER described
     * above for input on the on-board serial ports, but it is
     * not generally accessible because it is controlled by a
     * stream control message (M_CTL w/ either MC_SERVICEDEF or
     * MC_SERVICEIMM) and you can not do a putmsg directly to
     * the UART module and the tty driver does not provide an
     * interface.  Also, the ALM-2 driver apparently also has
     * a timer, but does not provide the M_CTL interface that's
     * provided for the on-board ports.  All in all this means
     * that the only way to defeat the timer for the on-board
     * serial ports (and thereby provide enough control for the
     * fax server to work with Class 1 modems) is to implement
     * a streams module in the kernel that provides an interface
     * to the timer--which is what has been done.  In the case of
     * the ALM-2, however, you are just plain out of luck unless
     * you have source code.
     */
    static fxBool zsunbuf_push_tried = FALSE;
    static fxBool zsunbuf_push_ok = FALSE;
    if (on) {			// pop zsunbuf if present to turn on buffering
	char topmodule[FMNAMESZ+1];
        if (zsunbuf_push_ok && ioctl(modemFd, I_LOOK, topmodule) >= 0 &&
	  streq(topmodule, "zsunbuf")) {
	    if (ioctl(modemFd, I_POP, 0) < 0)
		traceStatus(FAXTRACE_MODEMOPS, "MODEM pop zsunbuf failed %m");
	}
    } else {			// push zsunbuf to turn off buffering
        if (!zsunbuf_push_tried) {
            zsunbuf_push_ok = (ioctl(modemFd, I_PUSH, "zsunbuf") >= 0);
            traceStatus(FAXTRACE_MODEMOPS, "MODEM initial push zsunbuf %s",
                zsunbuf_push_ok ? "succeeded" : "failed");
            zsunbuf_push_tried = TRUE;
        } else if (zsunbuf_push_ok) {
            if (ioctl(modemFd, I_PUSH, "zsunbuf") < 0)
                traceStatus(FAXTRACE_MODEMOPS, "MODEM push zsunbuf failed %m");
        }
    }
#endif
    struct termios term;
    (void) tcgetattr(modemFd, &term);
    if (on) {
	term.c_cc[VMIN] = 127;
	term.c_cc[VTIME] = 1;
    } else {
	term.c_cc[VMIN] = 1;
	term.c_cc[VTIME] = 0;
    }
    return (tcsetattr(modemFd, TCSANOW, &term) == 0);
}

fxBool
FaxServer::sendBreak(fxBool pause)
{
    traceStatus(FAXTRACE_MODEMOPS, "MODEM send break");
    flushModemInput();
    if (pause) {
	/*
	 * NB: TCSBRK is supposed to wait for output to drain,
	 * but modem appears loses data if we don't do this.
	 */
	(void) tcdrain(modemFd);
    }
    return (tcsendbreak(modemFd, 0) == 0);
}

static fxBool timerExpired = FALSE;
static void sigAlarm(int)		{ timerExpired = TRUE; }

#ifndef SA_INTERRUPT
#define	SA_INTERRUPT	0
#endif

void
FaxServer::startTimeout(long ms)
{
    timeout = ::timerExpired = FALSE;
#ifdef SV_INTERRUPT			/* BSD-style */
    static struct sigvec sv;
    sv.sv_handler = fxSIGVECHANDLER(sigAlarm);
    sv.sv_flags = SV_INTERRUPT;
    sigvec(SIGALRM, &sv, (struct sigvec*) 0);
#else
#ifdef SA_NOCLDSTOP			/* POSIX */
    static struct sigaction sa;
    sa.sa_handler = fxSIGACTIONHANDLER(sigAlarm);
    sa.sa_flags = SA_INTERRUPT;
    sigaction(SIGALRM, &sa, (struct sigaction*) 0);
#else					/* System V-style */
    signal(SIGALRM, fxSIGHANDLER(sigAlarm));
#endif
#endif
#ifdef ITIMER_REAL
    itimerval itv;
    itv.it_value.tv_sec = ms / 1000;
    itv.it_value.tv_usec = (ms % 1000) * 1000;
    timerclear(&itv.it_interval);
    (void) setitimer(ITIMER_REAL, &itv, (itimerval*) 0);
    traceStatus(FAXTRACE_TIMEOUTS, "START %ld.%02ld second timeout",
	itv.it_value.tv_sec, itv.it_value.tv_usec / 10000);
#else
    long secs = howmany(ms, 1000);
    (void) alarm(secs);
    traceStatus(FAXTRACE_TIMEOUTS, "START %ld second timeout", secs);
#endif
}

void
FaxServer::stopTimeout(const char* whichdir)
{
#ifdef ITIMER_REAL
    static itimerval itv = { 0, 0, 0, 0 };
    (void) setitimer(ITIMER_REAL, &itv, (itimerval*) 0);
#else
    (void) alarm(0);
#endif
    traceStatus(FAXTRACE_TIMEOUTS,
	"STOP timeout%s", ::timerExpired ? ", timer expired" : "");
    if (timeout = ::timerExpired)
	traceStatus(FAXTRACE_MODEMOPS, "TIMEOUT: %s", whichdir);
}

int
FaxServer::getModemLine(char rbuf[], long ms)
{
    int c;
    int cc = 0;
    if (ms) startTimeout(ms);
    do {
	while ((c = getModemChar(0)) != EOF && c != '\n')
	    if (c != '\0' && c != '\r')
		rbuf[cc++] = c;
    } while (cc == 0 && c != EOF);
    rbuf[cc] = '\0';
    if (ms) stopTimeout("reading line from modem");
    if (!timeout)
	traceStatus(FAXTRACE_MODEMCOM, "--> [%d:%s]", cc, rbuf);
    return (cc);
}

int
FaxServer::getModemChar(long ms)
{
    if (rcvNext >= rcvCC) {
	int n = 0;
	if (ms) startTimeout(ms);
	do
	    rcvCC = ::read(modemFd, rcvBuf, sizeof (rcvBuf));
	while (n++ < 5 && rcvCC == 0);
	if (ms) stopTimeout("reading from modem");
	if (rcvCC <= 0) {
	    if (rcvCC < 0) {
		if (errno != EINTR)
		    traceStatus(FAXTRACE_MODEMOPS,
			"Error #%u reading from modem", errno);
	    }
	    return (EOF);
	} else
	    traceModemIO("-->", rcvBuf, rcvCC);
	rcvNext = 0;
    }
    return (rcvBuf[rcvNext++]);
}

void
FaxServer::modemFlushInput()
{
    flushModemInput();
    (void) tcflush(modemFd, TCIFLUSH);
    traceStatus(FAXTRACE_MODEMOPS, "MODEM flush i/o");
}

fxBool
FaxServer::modemStopOutput()
{
    return (tcflow(modemFd, TCOOFF) == 0);
}

void
FaxServer::flushModemInput()
{
    rcvCC = rcvNext = 0;
}

fxBool
FaxServer::putModem(void* data, int n, long ms)
{
    traceStatus(FAXTRACE_MODEMCOM, "<-- data [%d]", n);
    if (ms)
	startTimeout(ms);
    else
	timeout = FALSE;
    int cc = ::write(modemFd, (char*) data, n);
    if (ms)
	stopTimeout("writing to modem");
    if (cc > 0) {
	traceModemIO("<--", (u_char*) data, cc);
	n -= cc;
    }
    return (!timeout && n == 0);
}

void
FaxServer::putModemLine(const char* cp)
{
    int n = strlen(cp);
    traceStatus(FAXTRACE_MODEMCOM, "<-- [%d:%s]", n, cp);
    ::write(modemFd, cp, n);
    static char CR = '\r';
    ::write(modemFd, &CR, 1);
}
