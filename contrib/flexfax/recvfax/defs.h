/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/recvfax/defs.h,v 1.1.1.1 1994/01/14 23:10:40 torek Exp $
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
#ifndef _DEFS_
#define	_DEFS_

#include "config.h"

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <syslog.h>
#include <ctype.h>
#include <stdarg.h>

#include "port.h"

typedef struct {
    char faxNumber[40];
} config;

#define JOB_LOCKED	0x0001
#define JOB_SENT	0x0002
#define JOB_INVALID	0x0004

typedef struct _job {
    struct _job* next;
    unsigned int flags;
    char*	qfile;		/* associated q file name */
    time_t	tts;		/* time to send */
    char*	killtime;	/* time to kill job */
    char*	sender;		/* sender's name */
    char*	mailaddr;	/* return mail address */
    char*	number;		/* dialstring for fax machine */
    char*	external;	/* displayable phone number of fax machine */
    char*	status;		/* reason for current state */
    char*	modem;		/* requested outgoing modem */
} Job;

#define	TYPE_TIFF	1
#define	TYPE_POSTSCRIPT	2

#define	isTag(a)	(strcasecmp(tag,a) == 0)
#define	isCmd(a)	(strcasecmp(line,a) == 0)
#define	MIN(a,b)	((a)<(b)?(a):(b))

extern	char* SPOOLDIR;

extern	char** dataFiles;
extern	char** pollIDs;
extern	int* fileTypes;
extern	int nDataFiles;
extern	int nPollIDs;
extern	char line[1024];		/* current input line */
extern	char* tag;			/* keyword: tag */
extern	int debug;
extern	Job* jobList;
extern	int seqnum;
extern	int version;			/* protocol version */
extern	char userID[1024];		/* user identity */
extern	struct tm* now;			/* ``current'' time of day */

extern	int getCommandLine();
extern	void newDataFile(char* filename, int type);
extern	Job* readJobs();
extern	void checkPermission();
extern	int notifyServer(const char* modem, const char* fmt, ...);
extern	int cvtTime(const char* spec, struct tm* ref, u_long* result,
	    const char* what);
extern	void sendClient(char* tag, char* fmt, ...);
extern	void sendError(char* fmt, ...);
extern	void sendAndLogError(char* fmt, ...);
extern	void protocolBotch(char* fmt, ...);
extern	void vsendClient(char* tag, char* fmt, va_list ap);
extern	void done(int status, char* how);

extern	int modemMatch(const char*, const char*);
typedef	void jobFunc(Job*, const char*, const char* arg);
extern	void applyToJob(const char* modem, char* tag, const char* op, jobFunc*);

extern	void newPollID(const char* modem, char* pid);
extern	void getTIFFData(const char* modem, char* tag);
extern	void getPostScriptData(const char* modem, char* tag);
extern	void getDataOldWay(const char* modem, char* tag);
extern	void submitJob(const char* modem, char* tag);
extern	void sendRecvStatus(const char* modem, char* tag);
extern	void sendAllStatus(const char* modem, char* tag);
extern	void sendJobStatus(const char* modem, char* onwhat);
extern	void sendUserStatus(const char* modem, char* onwhat);
extern	void sendServerStatus(const char* modem, char* tag);
extern	void removeJob(const char* modem, char* tag);
extern	void killJob(const char* modem, char* tag);
extern	void alterJobTTS(const char* modem, char* tag);
extern	void alterJobKillTime(const char* modem, char* tag);
extern	void alterJobMaxDials(const char* modem, char* tag);
extern	void alterJobNotification(const char* modem, char* tag);
#endif /* _DEFS_ */
