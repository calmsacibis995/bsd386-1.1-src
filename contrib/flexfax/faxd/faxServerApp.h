/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/faxServerApp.h,v 1.1.1.1 1994/01/14 23:09:45 torek Exp $
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
#ifndef _faxServerApp_
#define	_faxServerApp_
/*
 * Fax Queue Manager and Command Handler.
 */
#include <stdarg.h>
#include <Dispatch/iohandler.h>
#include "Str.h"

class FaxServer;
class FaxRequest;
class FaxMachineInfo;
class FaxRecvInfo;
class TIFF;

typedef unsigned int JobStatus;

/*
 * Jobs represent send requests in the queue.
 */
struct Job {
    enum {
	no_status	= 0,
	done		= 1,
	requeued	= 2,
	removed		= 3,
	timedout	= 4,
	no_formatter	= 5,
	format_failed	= 6,
	poll_rejected	= 7,
	poll_no_document= 8,
	poll_failed	= 9,
	file_rejected	= 10,
	killed		= 11,
    };
    Job*	next;		// linked list
    fxStr	file;		// queue file name
    time_t	tts;		// time to send job
    int		pri;		// priority

    Job(const fxStr& s, time_t t) : file(s) { tts = t; pri = 0; }
    Job(const Job& j) : file(j.file) { tts = j.tts; pri = 0; }
    Job(const Job* j) : file(j->file) { tts = j->tts; pri = 0; }
    ~Job() {}
};

/*
 * This class represents a thread of control that manages the
 * send queue, deals with system-related notification (sends
 * complete, facsimile received, errors), and delivers external
 * commands passed in through the command FIFOs.  This class is
 * also responsible for preparing documents for sending by
 * doing formatting tasks such as converting PostScript to TIFF.
 *
 * In a multi-threaded environment, this class represents a
 * separate thread of control.
 */
class faxServerApp : public IOHandler {
private:
    fxStr	appName;		// program name
    fxStr	device;			// modem device
    fxStr	devID;			// mangled device name
    fxStr	queueDir;		// spooling directory
    fxStr	serverPID;		// pid of this process
    FaxServer*	server;			// active server app
    Job*	queue;			// job queue
    fxBool	use2D;			// ok to generate 2D-encoded facsimile
    fxBool	running;		// server running
    int		requeueInterval;	// time between job retries
    u_long	currentTimeout;		// time associated with timer
    time_t	jobStart;		// starting time for job
    time_t	fileStart;		// starting time for file/poll
    int		fifo;			// fifo job queue interface
    int		devfifo;		// fifo device interface
    FaxRequest*	curreq;			// current job being processed

    static const fxStr fifoName;
    static const fxStr sendDir;
    static const fxStr recvDir;
    static const fxStr notifyCmd;
    static const fxStr faxRcvdCmd;
    static const fxStr pollRcvdCmd;
    static const fxStr ps2faxCmd;

// miscellaneous stuff
    void	detachFromTTY();
    void	setRealIDs();
    void	logError(const char* fmt ...);
    void	vlogError(const char* fmt, va_list ap);
    void	traceServer(const char* fmt ...);
    void	traceQueue(const char* fmt ...);
    void	traceAtJobs(const char* fmt ...);
    const char*	fmtTime(time_t);
    void	recordRecv(const FaxRecvInfo& ri);
    void	account(const char* cmd, const struct FaxAcctInfo&);
    void	runCmd(const char* cmd, fxBool changeIDs = FALSE);

    static struct popenRecord* popenList;
    FILE*	popen(const char* cmd, const char* mode);
    int		pclose(FILE*);
// FIFO-related stuff
    int		openFIFO(const char* fifoName, int mode,
		    fxBool okToExist = FALSE);
    void	fifoMessage(const char* mesage);
// job management interfaces
    void	processJob(Job* job);
    void	processJob1(Job*, FaxRequest* req);
    void	sendJob(FaxRequest* req, FaxMachineInfo&);
    void	requestComplete(FaxRequest* req, fxBool notify);
    void	insertJob(Job* job);
    void	deleteJob(const fxStr& name);
    Job*	removeJob(const fxStr& name);
    fxBool	alterJob(const char* s);
// job preparation stuff
    void	scanQueueDirectory();
    void	scanJobQueue();
    void	startTimer(u_long sec);
    void	stopTimer();
    FaxRequest*	readQFile(const fxStr& filename, int fd);
    void	deleteRequest(JobStatus why, FaxRequest* req,
		    fxBool force = FALSE);
    void	notifySender(JobStatus why, FaxRequest& req);
    JobStatus	convertPostScript(const fxStr& inFile, const fxStr& outFile,
		    FaxRequest& req, const FaxMachineInfo& info);
    JobStatus	checkFileFormat(const fxStr& file, const FaxMachineInfo& info,
		    fxStr& emsg);
    fxBool	checkPageFormat(TIFF*, const FaxMachineInfo& info, fxStr& emsg);
public:
    faxServerApp();
    ~faxServerApp();

    void	initialize(int argc, char** argv);
    void	open();
    void	close();

    fxBool	isRunning() const;
// Dispatcher hooks
    int		inputReady(int);
    void	timerExpired(long sec, long usec);
// notification interfaces used by FaxServer
    void	notifyModemReady();
    void	notifyDocumentSent(FaxRequest&, u_int fileIndex);
    void	notifyPollRecvd(FaxRequest&, const FaxRecvInfo&);
    void	notifyPollDone(FaxRequest&, u_int pollIndex);
    void	notifySendDone(FaxRequest* req, u_int npages,
		    const char* csi, u_int sigrate, const char* df);
    void	notifyRecvDone(const FaxRecvInfo& req);
// system logging interfaces used by FaxServer
    void	vlogInfo(const char* fmt, va_list ap);
};
inline fxBool faxServerApp::isRunning() const { return running; }
#endif
