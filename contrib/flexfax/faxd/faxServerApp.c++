/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/faxServerApp.c++,v 1.1.1.1 1994/01/14 23:09:49 torek Exp $
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
#include <sys/types.h>
#include <syslog.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <osfcn.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <pwd.h>
#include <math.h>
#include <paths.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/wait.h>

#include <Dispatch/dispatcher.h>

#include "FaxServer.h"
#include "FaxRecvInfo.h"
#include "faxServerApp.h"
#include "FaxMachineInfo.h"
#include "config.h"
#include "flock.h"

/*
 * FAX Spooling and Command Agent.
 */

const fxStr faxServerApp::fifoName	= FAX_FIFO;
const fxStr faxServerApp::sendDir	= FAX_SENDDIR;
const fxStr faxServerApp::recvDir	= FAX_RECVDIR;
const fxStr faxServerApp::ps2faxCmd	= FAX_PS2FAX;
const fxStr faxServerApp::notifyCmd	= FAX_NOTIFYCMD;
const fxStr faxServerApp::faxRcvdCmd	= FAX_FAXRCVDCMD;
const fxStr faxServerApp::pollRcvdCmd	= FAX_POLLRCVDCMD;

extern	void fxFatal(const char* va_alist ...);

faxServerApp::faxServerApp() : queueDir(FAX_SPOOLDIR)
{
    use2D = TRUE;
    running = FALSE;
    queue = 0;
    currentTimeout = 0;
    requeueInterval = FAX_REQUEUE;
    fifo = -1;
    devfifo = -1;
    server = 0;
    curreq = NULL;
    openlog("FaxServer", LOG_PID, LOG_FAX);
}

faxServerApp::~faxServerApp()
{
    if (fifo != -1) {
	Dispatcher::instance().unlink(fifo);
	::close(fifo);
    }
    if (devfifo != -1) {
	Dispatcher::instance().unlink(devfifo);
	::close(devfifo);
    }
    delete server;
    closelog();
}

void
faxServerApp::initialize(int argc, char** argv)
{
    if (getuid() != 0)
	fxFatal("The fax server must run with real uid root.\n");
    uid_t euid = geteuid();
    if (euid == 0) {
	struct passwd* pwd = getpwnam(FAX_USER);
	if (!pwd)
	    fxFatal("No fax user \"%s\" defined on your system!\n"
		"This software is not installed properly!");
	if (setegid(pwd->pw_gid) < 0)
	    fxFatal("Can not setup permissions (gid)");
	if (seteuid(pwd->pw_uid) < 0)
	    fxFatal("Can not setup permissions (uid)");
    } else {
	struct passwd* pwd = getpwuid(euid);
	if (!pwd)
	    fxFatal("Can not figure out the identity of uid %d", euid);
	if (strcmp(pwd->pw_name, FAX_USER) && strcmp(pwd->pw_name, "uucp"))
	    fxFatal("The fax server must run as the fax user \"%s\".");
    }

    appName = argv[0];
    u_int l = appName.length();
    appName = appName.tokenR(l, '/');

    extern int optind, opterr;
    extern char* optarg;
    int c;
    opterr = 0;
    fxBool debug = FALSE;
    while ((c = getopt(argc, argv, "m:g:i:q:d1")) != -1)
	switch (c) {
	case 'm':
	    device = optarg;
	    break;
	case 'i':
	    requeueInterval = atoi(optarg);
	    break;
	case 'q':
	    queueDir = optarg;
	    break;
	case 'd':
	    debug = TRUE;
	    break;
	case '1':
	    use2D = FALSE;
	    break;
	case '?':
	    fxFatal("usage: %s"
		" -m modem-device"
		" [-g gettyspeed]"
		" [-i requeue-interval]"
		" [-q queue-directory]"
		" [-d] [-1]"
		, (char*) appName
		);
	}
    if (device == "")
	fxFatal("No modem device specified");
    if (!debug)
	detachFromTTY();

    serverPID = fxStr(getpid(), "%u");
    /*
     * Construct an identifier for the device special
     * file by stripping a leading prefix (DEV_PREFIX)
     * and converting all remaining '/'s to '_'s.  This
     * is required for SVR4 systems which have their
     * devices in subdirectories!
     */
    devID = device;
    fxStr prefix(DEV_PREFIX);
    u_int pl = prefix.length();
    if (devID.length() > pl && devID.head(pl) == prefix)
	devID.remove(0, pl);
    while ((l = devID.next(0, '/')) < devID.length())
	devID[l] = '_';
    server = new FaxServer(this, device, devID);

    /*
     * Requeue interval must be at least 2 seconds for
     * calculations below.  Should probably force it to
     * something much higher to avoid bad behaviour.
     */
    if (requeueInterval < 2)
	requeueInterval = 2;

    if (::chdir((char*) queueDir) < 0)
	fxFatal("%s: Can not change directory", (char*) queueDir);
    fifo = openFIFO(fifoName, 0600, TRUE);
    Dispatcher::instance().link(fifo, Dispatcher::ReadMask, this);
    devfifo = openFIFO(fifoName | "." | devID, 0600, TRUE);
    Dispatcher::instance().link(devfifo, Dispatcher::ReadMask, this);

    server->initialize(argc, argv);
}

/*
 * Break the association with the controlling
 * tty if we can preserve it later with the
 * POSIX O_NOCTTY mechanism.
 */
void
faxServerApp::detachFromTTY()
{
#ifdef O_NOCTTY
    int fd = ::open(_PATH_DEVNULL, O_RDWR);
    dup2(fd, STDIN_FILENO);
    dup2(fd, STDOUT_FILENO);
    dup2(fd, STDERR_FILENO);
    for (fd = 0; fd < _POSIX_OPEN_MAX; fd++)
	if (fd != STDIN_FILENO && fd != STDOUT_FILENO && fd != STDERR_FILENO)
	    (void) ::close(fd);
    switch (fork()) {
    case 0:	break;		// child, continue
    case -1:	_exit(1);	// error
    default:	_exit(0);	// parent, terminate
    }
    (void) setsid();
#endif
}

void
faxServerApp::open()
{
    traceServer("OPEN \"%s\"", (char*) device);
    server->open();
    scanQueueDirectory();
    startTimer(time(0) + 5);		// poke scheduler in a bit
    running = TRUE;
}

void
faxServerApp::close()
{
    if (running) {
	traceServer("CLOSE \"%s\"", (char*) device);
	stopTimer();
	server->close();
    }
    running = FALSE;
}

/*
 * Scan the spool directory for queue files and enter them
 * in the queue of outgoing jobs.  We do a flock on the
 * queue directory while scanning to avoid conflict between
 * servers (and clients).
 *
 * Note that what is done here is not right; we should
 * read the qfiles and retrieve the time-to-send for each
 * job (and also check the modem to use).  What we do
 * instead just causes the server to reread the queue
 * contents and do a single pass over it to reach a correct
 * state.  This can result in our processing jobs in the
 * wrong order.  Since this is done only once at server
 * startup (which is assumed to be infrequent); we live
 * with it.  When the queue management is rewritten this
 * will need to be corrected.
 */
void
faxServerApp::scanQueueDirectory()
{
    DIR* dir = opendir(sendDir);
    if (dir) {
	fxStr prefix(sendDir | "/");
	(void) flock(dir->dd_fd, LOCK_SH);
	time_t now = time(0);
	Job** last = &queue;
	for (dirent* dp = readdir(dir); dp; dp = readdir(dir)) {
	    if (dp->d_name[0] != 'q')
		continue;
	    struct stat sb;
	    fxStr file(prefix | dp->d_name);
	    if (stat((char*)file, &sb) >= 0 && (sb.st_mode&S_IFMT) == S_IFREG) {
		// NB: not right--should read tts from q file
		Job* job = new Job(file, now);
		job->next = 0;
		*last = job;
		last = &job->next;
	    }
	}
	(void) flock(dir->dd_fd, LOCK_UN);
	closedir(dir);
    } else
	logError("Could not scan queue directory");
    // XXX could do some intelligent queue processing
    // (e.g. coalesce jobs to the same destination)
}

/*
 * Process the next job in the queue.  The queue file
 * is locked to avoid conflict with other servers that
 * are processing the same spool area.  The queue file
 * associated with the job is read into an internal
 * format that's sent off to the fax server thread.  When
 * the fax server is done with the job (either sending the
 * files or failing), the transmitted request is returned,
 * triggering a call to notifyJobComplete.  Thus if
 * we fail to handle the entry at the front of the queue
 * we just simulate a server response by invoking the
 * method directly.
 */
void
faxServerApp::processJob(Job* job)
{
    int fd = ::open((char*) job->file, O_RDWR);
    if (fd >= 0) {
	time_t now = time(0);
	if (flock(fd, LOCK_EX|LOCK_NB) >= 0) {
	    FaxRequest* req = readQFile(job->file, fd);
	    if (req) {
		if (req->modem == MODEM_ANY || req->modem == devID) {
		    /*
		     * Job is for us, check kill time, then tts.
		     */
		    if (now < req->killtime) {
			/*
			 * If it is time to send this job and the modem is
			 * available, proceeed.  Otherwise reset the tts
			 * and requeue the job.
			 */
			if (req->tts <= now && server->modemReady()) {
			    // NB: gets locked, open file descriptor
			    processJob1(job, req);
			    return;
			}
			if (req->tts > now) {
			    /*
			     * It is not time yet, just requeue the job.
			     */
			    traceQueue("SEND NOT READY: \"%s\" in %s",
				(char*) req->qfile, fmtTime(req->tts - now));
			    job->tts = req->tts;
			} else {
			    /*
			     * The modem is not ready.  Set the tts
			     * so that the job will be processed
			     * immediately when we are notified that the
			     * modem is available for use.
			     */
			    job->tts = req->tts;
			}
			insertJob(new Job(job));
			delete req;
		    } else
			/*
			 * Job has expired, terminate it and notify user.
			 */
			deleteRequest(Job::timedout, req, TRUE);
		} else			// job is for another server
		    delete req;
	    } else
		logError("Could not read q file for \"%s\"", (char*) job->file);
	} else {
	    if (errno == EWOULDBLOCK) {
		/*
		 * The job is either being scanned by another server or
		 * by a faxd.recv process.  Bump the tts a bit so that
		 * if this is the job at the head of the queue it'll
		 * get reprocessed soon.
		 */
		job->tts = now + random() % 30;
		insertJob(new Job(job));
	    } else
		logError("Could not lock job file \"%s\": %m",
		    (char*) job->file);
	}
	::close(fd);
    } else {
	// file might have been removed by another server
	if (errno != ENOENT)
	    logError("Could not open job file \"%s\" (errno %d)",
		(char*) job->file, errno);
    }
}

/*
 * Check the remote machine info data to see if
 * there is an outgoing job currently being processed.
 * Note that we verify the specified job still exists
 * to deal with jobs being removed from the queue w/o
 * the associated info file being updated.  This is
 * because it is very hard to figure out the correct
 * info file to update when a job is removed by a user
 */
static fxBool
destIsBusy(FaxMachineInfo& info, const fxStr& qfile)
{
    const fxStr& job = info.getJobInProgress();
    if (job != "" && job != qfile) {
	struct stat sb;
	return (stat((char*) job, &sb) >= 0);
    } else
	return (FALSE);
}

/*
 * Process a ``send job''.
 *
 * Check the remote machine registry for the destination
 * machine's current call status and use that info to
 * decide if the job should be started.
 */
void
faxServerApp::processJob1(Job* job, FaxRequest* req)
{
    fxStr canon(server->canonicalizePhoneNumber(req->external));
    if (canon != "") {
	FaxMachineInfo info(canon, FALSE);
	if (info.isBusy() || destIsBusy(info, req->qfile)) {
	    /*
	     * Another call/job is in progress to this machine,
	     * requeue this job until the current job is done.
	     */
	    fxStr notice;
	    time_t now = time(0);
	    if (info.isBusy()) {
		job->tts = now + random() % 30;	// XXX maybe too agressive
		notice = "blocked by concurrent call";
		if (notice != req->notice) {	// avoid frequent updates
		    req->notice = notice;
		    req->writeQFile();
		}
		traceQueue("SEND BLOCKED BY CONCURRENT CALL: \"%s\" in %s",
		    (char*) req->qfile, fmtTime(job->tts - now));
	    } else {
		job->tts =
		    now + (requeueInterval>>1) + (random() % requeueInterval);
		req->notice = "blocked by concurrent job";
		req->writeQFile();
		traceQueue("SEND BLOCKED BY CONCURRENT JOB: \"%s\" by \"%s\"",
		    (char*) req->qfile, (char*) info.getJobInProgress(),
		    fmtTime(job->tts - now));
	    }
	    insertJob(new Job(job));
	    delete req;
	    return;
	}
	if (info.getRejectNotice() == "") {
	    sendJob(req, info);
	    return;
	}
	/*
	 * Calls to this remote machine are being rejected
	 * for a specified reason that we return to the sender.
	 * Note that we clear the job in progress field since
	 * the rejection notice could have been placed in the
	 * file since our first call.
	 */
	info.setJobInProgress("");
	req->status = send_failed;
	req->notice = info.getRejectNotice();
    } else {
	req->status = send_failed;
	req->notice = "unable to convert phone number to canonical format";
    }
    deleteRequest(Job::done, req, TRUE);
}

/*
 * Carry out the work for a ``send job''.
 * 
 * Use the remote machine's capabilities to convert
 * and preformat documents.  In particular, we use this
 * info to decide on the page size, whether or not high
 * res is permissible, and whether or not 2d encoding
 * should be used in preparing the facsimile to be sent.
 */
void
faxServerApp::sendJob(FaxRequest* req, FaxMachineInfo& info)
{
    traceQueue("JOB \"%s\"", (char*) req->qfile);
    JobStatus status = Job::done;
    fxBool updateQFile = FALSE;
    fxStr temp;		// NB: here to avoid compiler complaint
    for (u_int i = 0; i < req->ops.length(); i++) {
	const fxStr& file = req->files[i];
	switch (req->ops[i]) {
	case send_postscript:		// convert PostScript
	    temp = file | "+";
	    status = convertPostScript(file, temp, *req, info);
	    if (status == Job::done) {
		/*
		 * Insert converted file into list and mark the
		 * PostScript document so that it's saved, but
		 * not processed again.  The converted file
		 * is sent, while the saved file is kept around
		 * in case it needs to be returned to the sender.
		 */
		req->files.insert(temp, i+1);
		req->ops.insert(FaxSendOp(send_tiff), i+1);
		req->ops[i] = send_postscript_saved;
		updateQFile = TRUE;
	    } else
		(void) unlink((char*) temp);// bail out
	    break;
	case send_tiff:			// verify format
	    status = checkFileFormat(file, info, req->notice);
	    break;
	}
	if (status != Job::done)
	    break;
    }
    if (status == Job::done) {
	/*
	 * Write the q file before we start the send
	 * to make sure there is consistent state on
	 * disk should something happen during the send.
	 */
	if (updateQFile)
	    req->writeQFile();
	/*
	 * Everything should now be converted and compatible
	 * with the local and remote fax modems; start the job.
	 */
	traceServer("SEND BEGIN: %s TO %s FROM %s",
	    (char*) req->qfile,
	    (char*) req->external,
	    (char*) req->sender);
	jobStart = fileStart = time(0);
	curreq = req;
	server->sendFax(*req, info);
    } else {
	deleteRequest(status, req, TRUE);
    }
}

/*
 * Invoke the PostScript interpreter to image
 * the document according to the capabilities
 * of the remote fax machine.
 */
JobStatus
faxServerApp::convertPostScript(const fxStr& inFile, const fxStr& outFile,
    FaxRequest& req, const FaxMachineInfo& info)
{
    float resolution = req.resolution;
    if (resolution > 150 &&
      (!info.getSupportsHighRes() || !server->modemSupportsVRes(resolution)))
	resolution = 98;	// guaranteed to be supported
    /*
     * Convert pagewidth in mm to pixel width to one
     * of the possible G3 values.  Note that we don't
     * handle the reduce image widths (e.g. middle
     * 1216 pixels out of 1728 on a 151mm line).
     */
    u_int pageWidth = (req.pagewidth == 255 ? 2048 :
		       req.pagewidth == 303 ? 2432 :
					      1728);
    pageWidth = fxmin(pageWidth, (u_int) info.getMaxPageWidth());
    float pageLength = req.pagelength;
    if (info.getMaxPageLength() != -1 && pageLength > info.getMaxPageLength())
	pageLength = info.getMaxPageLength();
    /*
     * ps2fax:
     *   -o file		output (temp) file
     *   -r <res>		output resolution (dpi)
     *   -w <pagewidth>		output page width (pixels)
     *   -l <pagelength>	output page length (mm)
     *   [-1|-2]		1d or 2d encoding
     */
    char buf[1024];
    /*
     * Generate 2D-encoded facsimile if:
     * o the server is permitted to generate 2D-encoded data,
     * o the remote side is known to be capable of it, and
     * o the modem is capable of sending 2D-encoded data.
     */
    int encoding = 1 + (
	use2D &&
	info.getCalledBefore() && info.getSupports2DEncoding() &&
	server->modemSupports2D()
	);
    sprintf(buf, " -r %g -w %d -l %g -%d ",
	resolution, pageWidth, pageLength, encoding);
    fxStr cmd(ps2faxCmd | " -o " | outFile | buf | inFile | " 2>&1");
    traceQueue("CONVERT POSTSCRIPT: \"%s\"", (char*) cmd);
    JobStatus status;
    req.notice.resize(0);
    FILE* fp = popen(cmd, "r");
    if (fp) {
	/*
	 * Collect the output from the interpreter
	 * in case there is an error -- this is sent
	 * back to the user that submitted the job.
	 */
	int n;
	while ((n = fread(buf, 1, sizeof (buf), fp)) > 0) {
	    for (int i = 0; i < n; i++)
		if (!isprint(buf[i]) && !isspace(buf[i]))
		    buf[i] = '?';		// convert garbage to "?"'s
	    req.notice.append(buf, n);
	}
	int exitstat = pclose(fp);
	if (exitstat != 0) {
	    status = Job::format_failed;
	    logError("CONVERT POSTSCRIPT: \"%s\" failed (exit %#d)",
		(char*) cmd, exitstat);
	} else
	    status = Job::done;
    } else {
	logError("CONVERT POSTSCRIPT: \"%s\" failed (popen)", (char*) cmd);
	status = Job::no_formatter;
    }
    return (status);
}

/*
 * Verify the format of a TIFF file against the capabilities
 * of the server's modem and the remote facsimile machine.
 */
JobStatus
faxServerApp::checkFileFormat(const fxStr& file, const FaxMachineInfo& info,
    fxStr& emsg)
{
    JobStatus status;
    TIFF* tif = TIFFOpen(file, "r");
    if (tif) {
	status = Job::done;
	do {
	    if (!checkPageFormat(tif, info, emsg))
		status = Job::file_rejected;
	} while (status == Job::done && TIFFReadDirectory(tif));
	TIFFClose(tif);
    } else {
	status = Job::file_rejected;
	emsg = "Can not open file";
    }
    if (status != Job::done)
	logError("SEND: REJECT %s because: \"%s\"",
	    (char*) file, (char*) emsg);
    return (status);
}

/*
 * Check the format of a page against the capabilities
 * of the modem (or the default capabilities if no modem
 * is currently setup).
 */
fxBool
faxServerApp::checkPageFormat(TIFF* tif, const FaxMachineInfo& info, fxStr& emsg)
{
    short bitspersample;
    TIFFGetFieldDefaulted(tif, TIFFTAG_BITSPERSAMPLE, &bitspersample);
    if (bitspersample != 1) {
	emsg = fxStr(bitspersample, "Not a bilevel image (bits/sample %u)");
	return (FALSE);
    }
    short samplesperpixel;
    TIFFGetFieldDefaulted(tif, TIFFTAG_SAMPLESPERPIXEL, &samplesperpixel);
    if (samplesperpixel != 1) {
	emsg = fxStr(samplesperpixel, "Multi-sample data (samples %u)");
	return (FALSE);
    }
    short compression = 0;
    (void) TIFFGetField(tif, TIFFTAG_COMPRESSION, &compression);
    if (compression != COMPRESSION_CCITTFAX3) {
	emsg = "Not in Group 3 format";
	return (FALSE);
    }
    long g3opts = 0;
    (void) TIFFGetField(tif, TIFFTAG_GROUP3OPTIONS, &g3opts);
    if (g3opts & GROUP3OPT_2DENCODING) {
	if (info.getCalledBefore() && !info.getSupports2DEncoding()) {
	    emsg = "Client is incapable of receiving 2DMR-encoded documents";
	    return (FALSE);
	}
	if (!server->modemSupports2D()) {
	    emsg = "Modem is incapable of sending 2DMR-encoded documents";
	    return (FALSE);
	}
    }
    u_long w;
    if (!TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w)) {
	emsg = "Malformed document (no image width)";
	return (FALSE);
    }
    if (w > info.getMaxPageWidth()) {
	emsg = fxStr((long) w,
	    "Client is incapable of receiving document with page width %lu");
	return (FALSE);
    }
    if (!server->modemSupportsPageWidth((u_int) w)) {
	emsg = fxStr((long) w,
	     "Modem is incapable of sending document with page width %lu");
	return (FALSE);
    }

    /*
     * Try to deduce the vertical resolution of the image
     * image.  This can be problematical for arbitrary TIFF
     * images because vendors sometimes do not give the units.
     * We, however, can depend on the info in images that
     * we generate because we are careful to include valid info.
     */
    float yres;
    if (TIFFGetField(tif, TIFFTAG_YRESOLUTION, &yres)) {
	short resunit = RESUNIT_NONE;
	(void) TIFFGetField(tif, TIFFTAG_RESOLUTIONUNIT, &resunit);
	if (resunit == RESUNIT_CENTIMETER)
	    yres *= 25.4;
    } else {
	/*
	 * No vertical resolution is specified, try
	 * to deduce one from the image length.
	 */
	u_long l;
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &l);
	yres = (l < 1450 ? 98 : 196);		// B4 at 98 lpi is ~1400 lines
    }
    if (yres >= 150 && !info.getSupportsHighRes()) {
	emsg = "Client is incapable of receiving high resolution documents";
	return (FALSE);
    }
    if (!server->modemSupportsVRes(yres)) {
	emsg = "Modem is incapable of sending high resolution documents";
	return (FALSE);
    }

    /*
     * Select page length according to the image size and
     * vertical resolution.  Note that if the resolution
     * info is bogus, we may select the wrong page size.
     * Note also that we're a bit lenient in places here
     * to take into account sloppy coding practice (e.g.
     * using 200 dpi for high-res facsimile.
     */
    u_long h = 0;
    if (!TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h)) {
	emsg = "Malformed document (no image length)";
	return (FALSE);
    }
    float len = h / (yres == 0 ? 1. : yres);		// page length in mm
    if (info.getMaxPageLength() != -1 && len > info.getMaxPageLength()) {
	emsg = fxStr((long) len,
	    "Client is incapable of receiving document with page length %lu");
	return (FALSE);
    }
    if (!server->modemSupportsPageLength((u_int) len)) {
	emsg = fxStr((long) len,
	    "Modem is incapable of sending document with page length %lu");
	return (FALSE);
    }
    return (TRUE);
}

/*
 * Insert a job in the queue according to
 * the time-to-send criteria.  The queue
 * is maintained sorted by the tts values.
 */
void
faxServerApp::insertJob(Job* job)
{
    Job* jp;
    Job** jpp = &queue;
    for (; (jp = *jpp) && jp->tts < job->tts; jpp = &jp->next)
	;
    job->next = jp;
    *jpp = job;
}

/*
 * Remove a job from the queue and update the time-to-send
 * values of any remaining jobs.  Also, if necessary, adjust
 * the current timeout.
 */
Job*
faxServerApp::removeJob(const fxStr& name)
{
    Job* jp;
    for (Job** jpp = &queue; (jp = *jpp) && jp->file != name; jpp = &jp->next)
	;
    if (jp) {
	if (*jpp = jp->next) {
	    if (jpp == &queue && currentTimeout != 0) {	// adjust timeout
		stopTimer();
		startTimer(queue->tts);
	    }
	}
    }
    return (jp);
}

/*
 * Start the timer associated with scanning the job queue.
 * Note that we maintain the expected time the timer will
 * expire so that we can calculate how long it's been running
 * in case we prematurely stop the timer.
 */
void
faxServerApp::startTimer(u_long sec)
{
    time_t now = time(0);
    traceQueue("JOB TIMER START %s", fmtTime(sec - now));
    if (sec > 0) {		// should always be later
	currentTimeout = sec - now;
	Dispatcher::instance().startTimer(currentTimeout, 0, this);
    } else
	logError("Zero-time timer requested, server may be hosed");
}

/*
 * Stop any current timer and adjust the
 * tts of the job at the head of the queue.
 */
void
faxServerApp::stopTimer()
{
    if (currentTimeout != 0) {
	Dispatcher::instance().stopTimer(this);
	currentTimeout = 0;
	if (queue)
	    traceQueue("JOB TIMER STOP (Q head %s tts %s)",
		(char*) queue->file, fmtTime(queue->tts - time(0)));
	else
	    traceQueue("JOB TIMER STOP (Q empty)");
    }
}

/*
 * Delete a queued job.
 */
void
faxServerApp::deleteJob(const fxStr& name)
{
    Job* jp = removeJob(name);
    if (jp)
	delete jp;
}

/*
 * Alter job parameters.
 */
fxBool
faxServerApp::alterJob(const char* s)
{
    const char cmd = *s++;
    const char* cp = strchr(s, ' ');
    if (!cp) {
	logError("Malformed JOB request \"%s\"", s);
	return (FALSE);
    }
    fxStr name(s, cp-s);
    Job* jp = removeJob(name);
    if (!jp) {
	logError("JOB \"%s\" not found on the queue.", (char*) name);
	return (FALSE);
    }
    while (isspace(*cp))
	cp++;
    switch (cmd) {
    case 'T':			// time-to-send
	jp->tts = atoi(cp);
	break;
    case 'P':			// change priority
	jp->pri = atoi(cp);
	break;
    default:
	logError("Invalid JOB request command \"%c\" ignored.", cmd);
	break;
    }
    insertJob(jp);		// place back on queue
    return (TRUE);
}

/*
 * Scan the list of jobs and process those that are
 * ready to send (time-to-send is <= 0).  This method
 * is invoked from the following places:
 *
 * o in response to a FIFO command that creates a new job or
 *   that alters the parameters of an existing job
 * o in response to a scheduler timeout
 * o by the fax server thread when a modem changes state from
 *   busy/unavailable to ready for use
 *
 * Note that since we are a pseduo-multi-threaded application
 * we guard against recursive invocations by checking if there
 * is a current request being processed.  This handles the case
 * where we are processing a job and, for example, receive a
 * FIFO message that causes us to be invoked.
 */
void
faxServerApp::scanJobQueue()
{
    if (queue && curreq == NULL) {
	stopTimer();
	while (queue != NULL) {
	    /*
	     * If the job at the head of the queue isn't
	     * ready to send yet, then restart the timer.
	     * Note that we don't need to scan the entire
	     * queue because we organize it s.t. the entry
	     * at the front is always the first to be ready.
	     */
	    if (queue->tts > time(0)) {
		startTimer(queue->tts);
		break;
	    }
	    /*
	     * Don't try to process anything if the server
	     * thread is busy doing something else (like
	     * waiting for a getty to exit).  We'll be notified
	     * through notifyModemReady when the server+modem
	     * are once again available.
	     */
	    if (server->serverBusy())
		break;
	    /*
	     * The job at the head of the queue should be processed.
	     * Remove it from the queue and initiate the work.  If
	     * the job must be requeued, a new ``job'' will be created
	     * (could be optimized).
	     */
	    Job* job = queue;
	    queue = job->next;
	    processJob(job);
	    delete job;
	}
    }
}

struct FaxAcctInfo {
    const char*	user;		// sender/receiver identity
    time_t	start;		// starting time
    time_t	duration;	// job duration (minutes)
    const char*	device;		// modem device
    const char*	dest;		// receiver phone number
    const char*	csi;		// remote csi
    u_int	npages;		// pages successfully sent/rcvd
    u_int	sigrate;	// negotiated signalling rate
    const char*	df;		// negotiated data format
    const char*	status;		// status info (optional)
};

/*
 * Handle notification of a send job completing.
 */
void
faxServerApp::notifySendDone(FaxRequest* req,
    u_int npages, const char* csi, u_int sigrate, const char* df)
{
    if (req != NULL) {			// XXX should never be NULL
	FaxAcctInfo ai;
	ai.user = req->mailaddr;
	ai.start = jobStart;
	ai.duration = time(0) - jobStart;
	ai.device = devID;
	ai.dest = req->external;
	ai.csi = csi;
	ai.npages = npages;
	ai.sigrate = sigrate;
	ai.df = df;
	ai.status = req->notice;
	if (req->status == send_done)
	    ai.status = "";
	account("SEND", ai);
    }
    requestComplete(req, TRUE);
}

/*
 * Handle the completion of processing of a fax request.
 * We free up any resources associated with the request
 * and then process the next entry in the queue or restart
 * the scan timer.
 */
void
faxServerApp::requestComplete(FaxRequest* req, fxBool notify)
{
    if (req != NULL) {			// XXX should never be NULL
	if (req == curreq)
	    curreq = NULL;
	time_t now = time(0);
	if (req->status == send_retry) {// send should be retried
	    if (req->tts <= now) {
		/*
		 * Send failed, bump it's ``time to send''
		 * and rewrite the queue file.  This causes
		 * the job to be rescheduled for transmission
		 * at a future time.
		 */
		req->tts =
		    now + (requeueInterval>>1) + (random() % requeueInterval);
	    }
	    traceServer("REQUEUE %s FOR %s, REASON \"%s\"",
		(char*) req->qfile,
		fmtTime(req->tts - now),
		(char*) req->notice);
	    req->writeQFile();
	    if (notify && req->notify == FaxRequest::when_requeued)
		notifySender(Job::requeued, *req); 
	    insertJob(new Job(req->qfile, req->tts));
	    delete req;				// implicit unlock of q file
	} else {
	    traceServer("SEND DONE: %s", fmtTime(now - jobStart));
	    // NB: always notify client if job failed
	    deleteRequest(Job::done, req, req->status == send_failed);
	}
    } else
	logError("notifyJobComplete called with a NULL request");
}

/*
 * Notify the sender of the facsimile that something has
 * happened -- the job has completed, it's been requeued
 * for later transmission, etc.
 */
void
faxServerApp::notifySender(JobStatus why, FaxRequest& req)
{
    traceServer("NOTIFY %s\n", (char*) req.mailaddr);
    static const char* whys[] = {
	"no_status",
	"done",
	"requeued",
	"removed",
	"timedout",
	"no_formatter",
	"format_failed",
	"poll_rejected",
	"poll_no_document",
	"poll_failed",
	"file_rejected",
	"killed",
    };
    fxStr whystr(whys[why]);
    if (why == Job::done && req.status != send_done)
	whystr = "failed";
    fxStr quote(" \""); fxStr enquote("\"");
    // NB: use a temporary here to avoid gcc bug
    fxStr canon(server->canonicalizePhoneNumber(req.external));
    fxStr cmd(notifyCmd
	|   " " | req.qfile
	| quote | whystr | enquote
	| quote | fmtTime(time(0) - jobStart) | enquote
	|   " " | serverPID
	| quote | canon | enquote
    );
    if (why == Job::requeued) {
	/*
	 * It's too hard to do localtime in an awk script,
	 * so if we may need it, we calculate it here
	 * and pass the result as an optional argument.
	 */
	char buf[30];
	time_t tts = req.tts - time(0);
	strftime(buf, sizeof (buf), " \"%H:%M\"", localtime(&tts));
	cmd.append(buf);
    }
    runCmd(cmd, TRUE);
}

/*
 * Open the specified FIFO file.
 */
int
faxServerApp::openFIFO(const char* fifoName, int mode, fxBool okToExist)
{
    if (mkfifo(fifoName, mode & 0777) < 0) {
	if (errno != EEXIST || !okToExist)
	    fxFatal("Could not create FIFO \"%s\".", fifoName);
    }
#if !defined(sco) && !defined(AIXV3) && !defined(ultrix)
    int fd = ::open(fifoName, O_RDONLY|O_NDELAY, 0);
    if (fd == -1)
	fxFatal("Could not open FIFO file \"%s\"", fifoName);
    // open should set O_NDELAY, but just to be sure...
    if (fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NDELAY) < 0)
	logError("openFIFO: fcntl: %m");
#else
    /*
     * SCO ODT 2.0, AIX 3.2, and Ultrix 4.2a have bugs that
     * cause select to loop unless FIFO special files are
     * opened read+write.
     */
    int fd = ::open(fifoName, O_RDWR|O_NDELAY, 0);
    if (fd == -1)
	fxFatal("Could not open FIFO file \"%s\"", fifoName);
#endif
    return (fd);
}

/*
 * Respond to input on a FIFO.
 */
int
faxServerApp::inputReady(int fd)
{
    char buf[2048];
    int n;
    while ((n = ::read(fd, buf, sizeof (buf)-1)) > 0) {
	buf[n] = '\0';
	/*
	 * Break up '\0'-separated records and strip
	 * any trailing '\n' so that "echo mumble>FIFO"
	 * works (i.e. echo appends a '\n' character).
	 */
	char* bp = &buf[0];
	do {
	    char* cp = strchr(bp, '\0');
	    if (cp > bp) {
		if (cp[-1] == '\n')
		    cp[-1] = '\0';
		fifoMessage(bp);
	    }
	    bp = cp+1;
	} while (bp < &buf[n]);
    }
#ifdef solaris2
    /*
     * Solaris 2.x botch.  A client close of an open FIFO causes
     * an M_HANGUP to be sent and results in the receiver's file
     * descriptor being marked ``hung up''.  This in turn causes
     * select to perpetually return true and if we're running as
     * a realtime process, brings the system to a halt.  The
     * workaround for Solaris 2.1 was to do a parallel reopen of
     * the appropriate FIFO so that the original descriptor is
     * recycled.  This apparently no longer works in Solaris 2.2
     * or later and we are forced to close and reopen both FIFO
     * descriptors (noone appears capable of answering why this
     * this is necessary and I personally don't care...)
     */
    ::close(fifo); fifo = openFIFO(fifoName, 0600, TRUE);
    Dispatcher::instance().link(fifo, Dispatcher::ReadMask, this);
    ::close(devfifo); devfifo = openFIFO(fifoName | "." | devID, 0600, TRUE);
    Dispatcher::instance().link(devfifo, Dispatcher::ReadMask, this);
#endif
    return (0);
}

/*
 * Process a message received through a FIFO.
 */
void
faxServerApp::fifoMessage(const char* cp)
{
    switch (cp[0]) {
    case 'A':				// answer the phone
	if (cp[1] != '\0') {
	    traceServer("ANSWER %s", cp+1);
	    if (streq(cp+1, "fax"))
		server->answerPhone(FaxModem::ANSTYPE_FAX, TRUE);
	    else if (streq(cp+1, "data"))
		server->answerPhone(FaxModem::ANSTYPE_DATA, TRUE);
	    else if (streq(cp+1, "voice"))
		server->answerPhone(FaxModem::ANSTYPE_VOICE, TRUE);
	} else {
	    traceServer("ANSWER");
	    server->answerPhone(FaxModem::ANSTYPE_ANY, TRUE);
	}
	break;
    case 'M':				// modem control
	traceServer("MODEM \"%s\"", cp+1);
	server->restoreStateItem(cp+1);
	break;
    case 'J':				// alter job parameter(s)
	traceServer("JOB PARAMS \"%s\"", cp+1);
	if (curreq == NULL || curreq->qfile != cp+1) {
	    if (alterJob(cp+1))
		scanJobQueue();
	} else
	    ; // XXX can't alter parameters of job being processed
	break;
    case 'Q':				// quit
	traceServer("QUIT");
	faxServerApp::close();
	break;
    case 'R':				// remove job
    case 'K':				// kill job
	traceServer("%s \"%s\"", (cp[0] == 'R' ? "REMOVE" : "KILL"), cp+1);
	if (curreq == NULL || curreq->qfile != cp+1) {
	    deleteJob(cp+1);
	    // NOTE: actual removal normally done by faxd.recv
	    int fd = ::open(cp+1, O_RDWR);
	    if (fd > 0) {
		if (flock(fd, LOCK_EX|LOCK_NB) >= 0) {
		    FaxRequest* req = new FaxRequest(cp+1);
		    if (req) {
			(void) req->readQFile(fd);
			if (cp[0] == 'K')
			    deleteRequest(Job::killed, req, TRUE);
			else
			    deleteRequest(Job::removed, req, FALSE);
		    }
		}
		::close(fd);
	    }
	    scanJobQueue();
	} else
	    // aborting the job causes it to be purged
	    server->abortSession();
	break;
    case 'S':				// submit a send job
	traceServer("SUBMIT \"%s\"", cp+1);
	/*
	 * Insert the new job in the queue and if something
	 * is not already being processed, scan the queue.
	 */
	insertJob(new Job(cp+1, time(0)));
	scanJobQueue();
	break;
    default:
	logError("bad fifo message \"%s\"", cp);
	break;
    }
}

/*
 * Delete a request and associated state.
 */
void
faxServerApp::deleteRequest(JobStatus why, FaxRequest* req, fxBool force)
{
    if (req->notify != FaxRequest::no_notice || force) {
	req->writeQFile();		// update disk copy for notification use
	notifySender(why, *req);
    }
    for (u_int i = 0, n = req->files.length(); i < n; i++) {
	if (req->ops[i] != send_poll)
	    (void) unlink((char*) req->files[i]);
    }
    (void) unlink((char*) req->qfile);
    delete req;
}

/*
 * Handle notification that the modem device has become
 * available again after a period of being unavailable.
 */
void
faxServerApp::notifyModemReady()
{
    scanJobQueue();
}

#define	isSavedOp(op) \
    ((op) == send_tiff_saved || (op) == send_postscript_saved)

/*
 * Handle notification that a document has been successfully
 * transmitted.  We remove the file from the request array so
 * that it's not resent if the job is requeued.
 */
void
faxServerApp::notifyDocumentSent(FaxRequest& req, u_int fi)
{
    time_t now = time(0);
    traceServer("SEND: FROM %s TO %s (%s sent in %s)",
	(char*) req.mailaddr, (char*) req.external, (char*) req.files[fi],
	fmtTime(now - fileStart));
    fileStart = now;			// for next file
    if (req.ops[fi] == send_tiff) {
	(void) unlink((char*) req.files[fi]);
	u_int n = 1;
	if (fi > 0 && isSavedOp(req.ops[fi-1])) {
	    /*
	     * Document sent was converted from another; delete
	     * the original as well.  (Or perhaps we should hold
	     * onto it to return to sender in case of a problem?)
	     */
	    (void) unlink((char*) req.files[fi-1]);
	    fi--, n++;
	}
	req.files.remove(fi, n);
	req.ops.remove(fi, n);
	req.writeQFile();
    } else
	logError("notifyDocumentSent called for non-TIFF file");
}

/*
 * Handle notification of a document received as a
 * result of a poll request.
 */
void
faxServerApp::notifyPollRecvd(FaxRequest& req, const FaxRecvInfo& ri)
{
    recordRecv(ri);
    // hand to delivery/notification command
    runCmd(pollRcvdCmd
	 | " " | req.mailaddr
	 | " " | ri.qfile
	 | " " | fxStr(ri.time / 60.,"%.2f")
	 | " " | fxStr((int) ri.sigrate, "%u")
	 | " \"" | ri.protocol | "\""
	 | " \"" | ri.reason | "\""
	 , TRUE);
}

/*
 * Handle notification that a poll operation has been
 * successfully completed.  Note that any received
 * documents have already been passed to notifyPollRecvd.
 */
void
faxServerApp::notifyPollDone(FaxRequest& req, u_int pi)
{
    time_t now = time(0);
    traceServer("POLL: BY %s TO %s completed in %s",
	(char*) req.mailaddr, (char*) req.external, fmtTime(now - fileStart));
    fileStart = now;
    if (req.ops[pi] == send_poll) {
	req.files.remove(pi);
	req.ops.remove(pi);
	req.writeQFile();
    } else
	logError("notifyPollDone called for non-poll request");
}

/*
 * Handle notification that a document has been received.
 */
void
faxServerApp::notifyRecvDone(const FaxRecvInfo& ri)
{
    recordRecv(ri);
    // hand to delivery/notification command
    runCmd(faxRcvdCmd
	 | " " | ri.qfile
	 | " " | fxStr(ri.time / 60.,"%.2f")
	 | " " | fxStr((int) ri.sigrate, "%u")
	 | " \"" | ri.protocol | "\""
	 | " \"" | ri.reason | "\""
	 , TRUE);
}

void
faxServerApp::recordRecv(const FaxRecvInfo& ri)
{
    char type[80];
    if (ri.pagelength == 297 || ri.pagelength == -1)
	strcpy(type, "A4");
    else if (ri.pagelength == 364)
	strcpy(type, "B4");
    else
	sprintf(type, "(%u x %.2f)", ri.pagewidth, ri.pagelength);
    traceServer("RECV: %s from %s, %d %s pages, %g dpi, %s, %s at %u baud",
	(char*) ri.qfile, (char*) ri.sender,
	ri.npages, type, ri.resolution,
	(char*) ri.protocol, fmtTime((time_t) ri.time), ri.sigrate);

    FaxAcctInfo ai;
    ai.user = "fax";
    ai.duration = (time_t) ri.time;
    ai.start = time(0) - ai.duration;
    ai.device = devID;
    ai.dest = server->getModemNumber();
    ai.csi = ri.sender;
    ai.npages = ri.npages;
    ai.sigrate = ri.sigrate;
    ai.df = ri.protocol;
    ai.status = ri.reason;
    account("RECV", ai);
}

/*
 * Process a timer expiring.
 */
void
faxServerApp::timerExpired(long, long)
{
    if (queue) {
	currentTimeout = 0;
	scanJobQueue();
    }
}

/*
 * Create a FaxRequest and read the
 * associated queue file into it.
 */
FaxRequest*
faxServerApp::readQFile(const fxStr& filename, int fd)
{
    FaxRequest* req = new FaxRequest(filename);
    if (req->readQFile(fd)) {
	if (req->external == "")
	    req->external = server->canonicalizePhoneNumber(req->number);
    } else
	delete req, req = NULL;
    return req;
}

/*
 * Force the real uid+gid to be the same as
 * the effective ones.  Must temporarily
 * make the effective uid root in order to
 * do the real id manipulations.
 */
void
faxServerApp::setRealIDs()
{
    uid_t euid = geteuid();
    if (seteuid(0) < 0)
	logError("seteuid(root): %m");
    if (setgid(getegid()) < 0)
	logError("setgid: %m");
    if (setuid(euid) < 0)
	logError("setuid: %m");
}

/*
 * Run the specified shell command.  If changeIDs is
 * true, we set the real uid+gid to the effective; this
 * is so that programs like sendmail show an informative
 * from address.
 */
void
faxServerApp::runCmd(const char* cmd, fxBool changeIDs)
{
    pid_t pid = fork();
    switch (pid) {
    case 0:
	if (changeIDs)
	    setRealIDs();
	execl("/bin/sh", "sh", "-c", cmd, (char*) NULL);
	_exit(127);
    case -1:
	logError("Can not fork for \"%s\"", cmd);
	break;
    default:
	{ int status = 0;
	  (void) waitpid(pid, &status, 0);
	  if (status != 0)
	    logError("Bad exit status %#o for \"%s\"", status, cmd);
	}
	break;
    }
}

struct popenRecord {
    popenRecord* next;
    pid_t	pid;
    FILE*	fp;
};
popenRecord* faxServerApp::popenList = NULL;

/*
 * Internal version of popen/pclose.  These are
 * used instead of the normal libc routines so
 * that we can force the real uid in the child
 * process (for programs like at).
 */
FILE*
faxServerApp::popen(const char* cmd, const char* mode)
{
    popenRecord* pr = new popenRecord;
    if (!pr) {
	logError("popen: Out of memory");
	return (NULL);
    }
    int pfd[2];
    if (pipe(pfd) < 0) {
	logError("Can not create pipe for \"%s\"", cmd);
	delete pr;
	return (NULL);
    }
    pr->pid = fork();
    switch (pr->pid) {
    case -1:				// error
	logError("Can not fork for \"%s\"", cmd);
	::close(pfd[0]);
	::close(pfd[1]);
	delete pr;
	return (NULL);
    case 0:				// child
	/*
	 * Force real uid to match effective.  This insures
	 * that programs like at get the necessary environment.
	 */
	setRealIDs();
	if (*mode == 'r') {
	    if (pfd[1] != STDOUT_FILENO) {
		dup2(pfd[1], STDOUT_FILENO);
		::close(pfd[1]);
	    }
	    ::close(pfd[0]);
	} else {
	    if (pfd[0] != STDIN_FILENO) {
		dup2(pfd[0], STDIN_FILENO);
		::close(pfd[0]);
	    }
	    ::close(pfd[1]);
	}
	execl("/bin/sh", "sh", "-c", cmd, (char*) NULL);
	_exit(127);
	/*NOTREACHED*/
    }
    // NB: we assume fdopen always succeeds
    if (*mode == 'r') {
	pr->fp = fdopen(pfd[0], mode);
	::close(pfd[1]);
    } else {
	pr->fp = fdopen(pfd[1], mode);
	::close(pfd[0]);
    }
    pr->next = popenList;
    popenList = pr;
    return pr->fp;
}

int
faxServerApp::pclose(FILE* fp)
{
    fclose(fp);
    popenRecord* pr;
    popenRecord** ppr;
    int status = -1;
    for (ppr = &popenList; pr = *ppr; ppr = &pr->next)
	if (pr->fp == fp) {
	    *ppr = pr->next;				// remove from list
	    (void) waitpid(pr->pid, &status, 0);	// reap process
	    delete pr;					// free record
	    break;
	}
    return (status);
}

const char*
faxServerApp::fmtTime(time_t t)
{
    static char tbuf[10];
    const char* digits = "0123456789";
    char* cp = tbuf;
    long v;

    if ((v = t/3600) > 0) {
	if (v >= 10)
	    *cp++ = digits[v / 10];
	*cp++ = digits[v % 10];
	*cp++ = ':';
	t -= v*3600;
    }
    v = t/60;
    if (v >= 10 || cp > tbuf)
	*cp++ = digits[v / 10];
    *cp++ = digits[v % 10];
    t -= v*60;
    *cp++ = ':';
    *cp++ = digits[t / 10];
    *cp++ = digits[t % 10];
    *cp = '\0';
    return tbuf;
}

void
faxServerApp::traceServer(const char* va_alist ...)
#define	fmt va_alist
{
    if (server->getServerTracing() & FAXTRACE_SERVER) {
	va_list ap;
	va_start(ap, va_alist);
	vlogInfo(fmt, ap);
	va_end(ap);
    }
}
#undef fmt

void
faxServerApp::traceQueue(const char* va_alist ...)
#define	fmt va_alist
{
    if (server->getServerTracing() & FAXTRACE_QUEUEMGMT) {
	va_list ap;
	va_start(ap, va_alist);
	vlogInfo(fmt, ap);
	va_end(ap);
    }
}
#undef fmt

/*
 * Record a transfer in the transfer log file.
 */
void
faxServerApp::account(const char* cmd, const FaxAcctInfo& ai)
{
    FILE* flog = fopen(FAX_XFERLOG, "a");
    if (flog != NULL) {
	flock(fileno(flog), LOCK_EX);
	char buf[80];
	strftime(buf, sizeof (buf), "%D %H:%M", localtime(&ai.start));
	fprintf(flog,
	    "%s\t%s\t%s\t%s\t\"%s\"\t\"%s\"\t%d\t%s\t%d\t%s\t\"%s\"\n",
	    buf, cmd, ai.device,
	    ai.user,
	    ai.dest, ai.csi,
	    ai.sigrate, ai.df,
	    ai.npages, fmtTime(ai.duration),
	    ai.status);
	fclose(flog);
    } else
	logError("Can not open log file \"%s\"", FAX_XFERLOG);
}

void
faxServerApp::vlogInfo(const char* fmt, va_list ap)
{
    vsyslog(LOG_INFO, fmt, ap);
}

void
faxServerApp::logError(const char* va_alist ...)
#define	fmt va_alist
{
    va_list ap;
    va_start(ap, va_alist);
    vlogError(fmt, ap);
    va_end(ap);
}
#undef fmt

void
faxServerApp::vlogError(const char* fmt, va_list ap)
{
    vsyslog(LOG_ERR, fmt, ap);
}

void
fxFatal(const char* va_alist ...)
#define	fmt va_alist
{
    va_list ap;
    va_start(ap, va_alist);
    vsyslog(LOG_ERR, fmt, ap);
    va_end(ap);
    exit(-1);
}
#undef fmt

static	faxServerApp* app;

#include <signal.h>

static void
sigCleanup(int)
{
    if (app) {
	if (app->isRunning())
	    app->close();
	delete app;
    }
    _exit(-1);
}

extern "C" int
main(int argc, char** argv)
{
    signal(SIGTERM, fxSIGHANDLER(sigCleanup));
    signal(SIGINT, fxSIGHANDLER(sigCleanup));
    app = new faxServerApp;
    app->initialize(argc, argv);
    app->open();
    while (app->isRunning())
	Dispatcher::instance().dispatch();
    app->close();
    delete app;
    return 0;
}
