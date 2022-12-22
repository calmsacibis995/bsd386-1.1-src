/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/recvfax/submit.c,v 1.1.1.1 1994/01/14 23:10:40 torek Exp $
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

#include <fcntl.h>
#include <sys/file.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>

#include "flock.h"

static	char qfile[1024];
static	FILE* qfd = NULL;

static	int getSequenceNumber();
static	void coverProtocol(int version, int seqnum);
static	void setupData(int seqnum);
static	void cleanupJob();
static	u_long cvtTimeOrDie(const char* spec, struct tm* ref, const char* what);

void
submitJob(const char* modem, char* otag)
{
    u_long tts = 0;

    sprintf(qfile, "%s/q%d", FAX_SENDDIR, seqnum = getSequenceNumber());
    qfd = fopen(qfile, "w");
    if (qfd == NULL) {
	syslog(LOG_ERR, "%s: Can not create qfile: %m", qfile);
	sendError("Can not create qfile \"%s\".", qfile);
	cleanupJob();
    }
    flock(fileno(qfd), LOCK_EX);
    fprintf(qfd, "modem:%s\n", modem);
    for (;;) {
	if (!getCommandLine())
	    cleanupJob();
	if (isCmd("end") || isCmd(".")) {
	    setupData(seqnum);
	    break;
	}
	if (isCmd("sendAt")) {
	    tts = cvtTimeOrDie(tag, now, "time-to-send");
	} else if (isCmd("killtime")) {
	    fprintf(qfd, "%s:%lu\n", line, cvtTimeOrDie(tag, now, "kill-time"));
	} else if (isCmd("cover")) {
	    coverProtocol(atoi(tag), seqnum);
	} else
	    fprintf(qfd, "%s:%s\n", line, tag);	/* XXX check info */
    }
    fprintf(qfd, "tts:%lu\n", tts);
    fclose(qfd), qfd = NULL;
    if (!notifyServer(modem, "S%s", qfile))
	sendError("Warning, no server appears to be running.");
    sendClient("job", "%d", seqnum);
}

static u_long
cvtTimeOrDie(const char* spec, struct tm* ref, const char* what)
{
    u_long when;

    if (!cvtTime(spec, ref, &when, what)) {
	cleanupJob();
	/*NOTREACHED*/
    }
    return (when);
}

static void
coverProtocol(int version, int seqnum)
{
    char template[1024];
    FILE* fd;

    sprintf(template, "%s/doc%d.cover", FAX_DOCDIR, seqnum);
    fd = fopen(template, "w");
    if (fd == NULL) {
	syslog(LOG_ERR, "%s: Could not create cover sheet file: %m",
	    template);
	sendError("Could not create cover sheet file \"%s\".", template);
	cleanupJob();
    }
    while (getCommandLine()) {
	if (isCmd("..")) {
	    fprintf(qfd, "postscript: %s\n", template);
	    break;
	}
	fprintf(fd, "%s\n", tag);
    }
    fclose(fd);
}

static void
setupData(int seqnum)
{
    int i;

    for (i = 0; i < nDataFiles; i++) {
	char doc[1024];
	sprintf(doc, "%s/doc%d.%d", FAX_DOCDIR, seqnum, i);
	if (link(dataFiles[i], doc) < 0) {
	    syslog(LOG_ERR, "Can not link document \"%s\": %m", doc);
	    sendError("Problem setting up document files.");
	    while (--i >= 0) {
		sprintf(doc, "%s/doc%d.%d", FAX_DOCDIR, seqnum, i);
		unlink(doc);
	    }
	    cleanupJob();
	    /*NOTREACHED*/
	}
	fprintf(qfd, "%s:%s\n",
	    fileTypes[i] == TYPE_TIFF ? "tiff" : "postscript", doc);
    }
    for (i = 0; i < nPollIDs; i++)
	fprintf(qfd, "poll:%s\n", pollIDs[i]);
}

static void
getData(int type, long cc)
{
    long total;
    int dfd;
    char template[80];

    if (cc <= 0)
	return;
    sprintf(template, "%s/sndXXXXXX", FAX_TMPDIR);
    dfd = mkstemp(template);
    if (dfd < 0) {
	syslog(LOG_ERR, "%s: Could not create data temp file: %m", template);
	sendError("Could not create data temp file.");
	cleanupJob();
    }
    newDataFile(template, type);
    total = 0;
    while (cc > 0) {
	char buf[4096];
	int n = MIN(cc, sizeof (buf));
	if (fread(buf, n, 1, stdin) != 1) {
	    protocolBotch( "not enough data received: %u of %u bytes.",
		total, total+cc);
	    cleanupJob();
	}
	if (write(dfd, buf, n) != n) {
	    extern int errno;
	    sendAndLogError("Error writing data file: %s.", strerror(errno));
	    cleanupJob();
	}
	cc -= n;
	total += n;
    }
    close(dfd);
    if (debug)
	syslog(LOG_DEBUG, "%s: %d-byte %s", template, total,
	    type == TYPE_TIFF ? "TIFF image" : "PostScript document");
}

void
getTIFFData(const char* modemname, char* tag)
{
    getData(TYPE_TIFF, atol(tag));
}

void
getPostScriptData(const char* modemname, char* tag)
{
    getData(TYPE_POSTSCRIPT, atol(tag));
}

void
getDataOldWay(const char* modemname, char* tag)
{
    long cc;
    int type;

    if (isTag("tiff"))
	type = TYPE_TIFF;
    else if (isTag("postscript"))
	type = TYPE_POSTSCRIPT;
    else {
	sendAndLogError("Can not handle \"%s\"-type data", tag);
	cleanupJob();
	/*NOTREACHED*/
    }
    if (fread(&cc, sizeof (long), 1, stdin) != 1) {
	protocolBotch("no data byte count.");
	cleanupJob();
    }
    getData(type, cc);
}

void
newDataFile(char* filename, int type)
{
    int l;
    char* cp;

    if (++nDataFiles == 1) {
	dataFiles = (char**)malloc(sizeof (char*));
	fileTypes = (int*)malloc(sizeof (int));
    } else {
	dataFiles = (char**)realloc(dataFiles, nDataFiles * sizeof (char*));
	fileTypes = (int*)realloc(fileTypes, nDataFiles * sizeof (int));
    }
    l = strlen(filename)+1;
    cp = (char*)malloc(l);
    memcpy(cp, filename, l);
    dataFiles[nDataFiles-1] = cp;
    fileTypes[nDataFiles-1] = type;
}

void
newPollID(const char* modemname, char* pid)
{
    int l;
    char* cp;

    if (++nPollIDs == 1)
	pollIDs = (char**)malloc(sizeof (char*));
    else
	pollIDs = (char**)realloc(pollIDs, nPollIDs * sizeof (char*));
    l = strlen(pid)+1;
    cp = (char*)malloc(l);
    memcpy(cp, pid, l);
    pollIDs[nPollIDs-1] = cp;
}

static void
cleanupJob()
{
    int i;

    for (i = 0; i < nDataFiles; i++)
	unlink(dataFiles[i]);
    { char template[1024];
      sprintf(template, "%s/doc%d.cover", FAX_DOCDIR, seqnum);
      unlink(template);
    }
    if (qfd)
	unlink(qfile);
    done(1, "EXIT");
}

static int
getSequenceNumber()
{
    int seqnum;
    int fd;

    fd = open(FAX_SEQF, O_CREAT|O_RDWR, 0644);
    if (fd < 0) {
	syslog(LOG_ERR, "%s: open: %m", FAX_SEQF);
	sendError("Problem opening sequence number file.");
	done(-2, "EXIT");
    }
    flock(fd, LOCK_EX);
    seqnum = 1;
    if (read(fd, line, sizeof (line)) > 0)
	seqnum = atoi(line);
    sprintf(line, "%d", seqnum < 9999 ? seqnum+1 : 1);
    lseek(fd, 0, SEEK_SET);
    if (write(fd, line, strlen(line)) != strlen(line)) {
	sendAndLogError("Problem updating sequence number file.");
	done(-3, "EXIT");
    }
    close(fd);			/* implicit unlock */
    if (debug)
	syslog(LOG_DEBUG, "seqnum %d", seqnum);
    return (seqnum);
}
