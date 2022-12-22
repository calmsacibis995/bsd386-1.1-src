/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/recvfax/main.c,v 1.1.1.1 1994/01/14 23:10:40 torek Exp $
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
#include "defs.h"

#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>

char*	SPOOLDIR = FAX_SPOOLDIR;

char**	dataFiles;
int*	fileTypes;
int	nDataFiles = 0;
int	nPollIDs = 0;
char**	pollIDs;
char	line[1024];		/* current input line */
char*	tag;			/* keyword: tag */
int	debug = 0;
Job*	jobList = 0;
int	seqnum = 0;
int	version = 0;
char	userID[1024];
struct	tm* now;		/* current time of day */

void
done(int status, char* how)
{
    fflush(stdout);
    if (debug)
	syslog(LOG_DEBUG, how);
    exit(status);
}

static void
setUserID(const char* modemname, char* tag)
{
    strncpy(userID, tag, sizeof (userID)-1);
}

static void
setProtoVersion(const char* modemname, char* tag)
{
    version = atoi(tag);
    if (version > FAX_PROTOVERS) {
	protocolBotch(
	    "protocol version %u requested: only understand up to %u.",
	    version, FAX_PROTOVERS);
	done(1, "EXIT");
    }
}

static void
ackPermission(const char* modemname, char* tag)
{
    sendClient("permission", "%s", "granted");
    fflush(stdout);
}

#define	TRUE	1
#define	FALSE	0

struct {
    const char* cmd;		/* command to match */
    int		check;		/* if true, checkPermission first */
    void	(*cmdFunc)(const char*, char*);
} cmds[] = {
    { "begin",		TRUE,	submitJob },
    { "checkPerm",	TRUE,	ackPermission },
    { "tiff",		TRUE,	getTIFFData },
    { "postscript",	TRUE,	getPostScriptData },
    { "data",		TRUE,	getDataOldWay },
    { "poll",		TRUE,	newPollID },
    { "userID",		FALSE,	setUserID },
    { "version",	FALSE,	setProtoVersion },
    { "serverStatus",	FALSE,	sendServerStatus },
    { "allStatus",	FALSE,	sendAllStatus },
    { "userStatus",	FALSE,	sendUserStatus },
    { "jobStatus",	FALSE,	sendJobStatus },
    { "recvStatus",	FALSE,	sendRecvStatus },
    { "remove",		TRUE,	removeJob },
    { "kill",		TRUE,	killJob },
    { "alterTTS",	TRUE,	alterJobTTS },
    { "alterKillTime",	TRUE,	alterJobKillTime },
    { "alterMaxDials",	TRUE,	alterJobMaxDials },
    { "alterNotify",	TRUE,	alterJobNotification },
};
#define	NCMDS	(sizeof (cmds) / sizeof (cmds[0]))

main(int argc, char** argv)
{
    extern char* optarg;
    char modemname[80];
    time_t t = time(0);
    int c;

    now = localtime(&t);
    openlog(argv[0], LOG_PID, LOG_FAX);
    umask(077);
    while ((c = getopt(argc, argv, "q:d")) != -1)
	switch (c) {
	case 'q':
	    SPOOLDIR = optarg;
	    break;
	case 'd':
	    debug = 1;
	    syslog(LOG_DEBUG, "BEGIN");
	    break;
	case '?':
	    syslog(LOG_ERR,
		"Bad option `%c'; usage: faxd.recv [-q queue-dir] [-d]", c);
	    done(-1, "EXIT");
	}
    if (chdir(SPOOLDIR) < 0) {
	syslog(LOG_ERR, "%s: chdir: %m", SPOOLDIR);
	sendError("Can not change to spooling directory.");
	done(-1, "EXIT");
    }
    if (debug) {
	char buf[82];
	syslog(LOG_DEBUG, "chdir to %s", getcwd(buf, 80));
    }
#if defined(SO_LINGER) && !defined(__linux__)
    { struct linger opt;
      opt.l_onoff = 1;
      opt.l_linger = 60;
      if (setsockopt(0, SOL_SOCKET, SO_LINGER, (char*)&opt, sizeof (opt)) < 0)
	syslog(LOG_WARNING, "setsockopt (SO_LINGER): %m");
    }
#endif
    strcpy(modemname, MODEM_ANY);
    strcpy(userID, "");
    while (getCommandLine() && !isCmd(".")) {
	if (isCmd("modem")) {			/* select outgoing device */
	    int l = strlen(DEV_PREFIX);
	    char* cp;
	    /*
	     * Convert modem name to identifier form by stripping
	     * any leading device pathname prefix and by replacing
	     * '/'s with '_'s for SVR4 where terminal devices are
	     * in subdirectories.
	     */
	    if (strncmp(tag, DEV_PREFIX, l) == 0)
		tag += l;
	    for (cp = tag; cp = strchr(cp, '/'); *cp = '_')
		;
	    strncpy(modemname, tag, sizeof (modemname)-1);
	} else {
	    int i;
	    for (i = 0; i < NCMDS && !isCmd(cmds[i].cmd); i++)
		;
	    if (i == NCMDS) {
		protocolBotch("unrecognized cmd \"%s\".", line);
		done(1, "EXIT");
	    }
	    if (cmds[i].check)
		checkPermission();
	    (*cmds[i].cmdFunc)(modemname, tag);
	}
    }
    /* remove original files -- only links remain */
    { int i;
      for (i = 0; i <nDataFiles; i++)
	unlink(dataFiles[i]);
    }
    done(0, "END");
}

int
getCommandLine()
{
    char* cp;

    if (!fgets(line, sizeof (line) - 1, stdin)) {
	protocolBotch("unexpected EOF.");
	return (0);
    }
    cp = strchr(line, '\0');
    if (cp > line && cp[-1] == '\n')
	*--cp = '\0';
    if (cp > line && cp[-1] == '\r')		/* for telnet users */
	*--cp = '\0';
    if (debug)
	syslog(LOG_DEBUG, "line \"%s\"", line);
    if (strcmp(line, ".") && strcmp(line, "..")) {
	tag = strchr(line, ':');
	if (!tag) {
	    protocolBotch("malformed line \"%s\".", line);
	    return (0);
	}
	*tag++ = '\0';
	while (isspace(*tag))
	    tag++;
    }
    return (1);
}

/*
 * Notify server of job parameter alteration.
 */
int
notifyServer(const char* modem, const char* va_alist, ...)
#define	fmt va_alist
{
    char fifoname[1024];
    int fifo;

    if (strcmp(modem, MODEM_ANY) == 0)
	strcpy(fifoname, FAX_FIFO);
    else
	sprintf(fifoname, "%s.%.*s", FAX_FIFO,
	    sizeof (fifoname) - (sizeof (FAX_FIFO)+2), modem);
    if (debug)
	syslog(LOG_DEBUG, "notify server for \"%s\"", modem);
    fifo = open(fifoname, O_WRONLY|O_NDELAY);
    if (fifo != -1) {
	char buf[2048];
	int len, ok;
	va_list ap;

	va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
	va_end(ap);
	len = strlen(buf);
	if (debug)
	    syslog(LOG_DEBUG, "write \"%.*s\" to fifo", len, buf);
	/*
	 * Turn off O_NDELAY so that write will block if FIFO is full.
	 */
	if (fcntl(fifo, F_SETFL, fcntl(fifo, F_GETFL, 0) &~ O_NDELAY) <0)
	    syslog(LOG_ERR, "fcntl: %m");
	ok = (write(fifo, buf, len+1) == len+1);
	if (!ok)
	    syslog(LOG_ERR, "FIFO write failed: %m");
	close(fifo);
	return (ok);
    } else if (debug)
	syslog(LOG_INFO, "%s: Can not open for notification: %m", fifoname);
    return (0);
}
#undef fmt

extern int parseAtSyntax(const char*, const struct tm*, struct tm*, char* emsg);

int
cvtTime(const char* spec, struct tm* ref, u_long* result, const char* what)
{
    char emsg[1024];
    struct tm when;
    if (!parseAtSyntax(spec, ref, &when, emsg)) {
	sendAndLogError("Error parsing %s \"%s\": %s.", what, spec, emsg);
	return (0);
    } else {
	*result = (u_long) mktime(&when);
	return (1);
    }
}

void
vsendClient(char* tag, char* fmt, va_list ap)
{
    fprintf(stdout, "%s:", tag);
    vfprintf(stdout, fmt, ap);
    fputc('\n', stdout);
    if (debug) {
	char buf[2048];
	sprintf(buf, "%s:", tag);
	vsprintf(buf+strlen(buf), fmt, ap);
	syslog(LOG_DEBUG, "%s", buf);
    }
}

void
sendClient(char* tag, char* va_alist, ...)
#define	fmt va_alist
{
    va_list ap;
    va_start(ap, fmt);
    vsendClient(tag, fmt, ap);
    va_end(ap);
}
#undef fmt

void
sendError(char* va_alist, ...)
#define	fmt va_alist
{
    va_list ap;
    va_start(ap, fmt);
    vsendClient("error", fmt, ap);
    va_end(ap);
}
#undef fmt

void
sendAndLogError(char* va_alist, ...)
#define	fmt va_alist
{
    va_list ap;
    va_start(ap, fmt);
    vsendClient("error", fmt, ap);
    vsyslog(LOG_ERR, fmt, ap);
    va_end(ap);
}
#undef fmt

void
protocolBotch(char* va_alist, ...)
#define	fmt va_alist
{
    char buf[1024];
    va_list ap;
    va_start(ap, fmt);
    sprintf(buf, "Protocol botch, %s", fmt);
    vsendClient("error", buf, ap);
    vsyslog(LOG_ERR, buf, ap);
    va_end(ap);
}
#undef fmt
