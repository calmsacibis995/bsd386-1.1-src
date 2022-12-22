/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/recvfax/status.c,v 1.1.1.1 1994/01/14 23:10:40 torek Exp $
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
#include "tiffio.h"

#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

#include "flock.h"

static	void getConfig(char* fileName, config* configp, config* deflt);
static	void getServerStatus(char* fileName, char buf[1024]);

void
sendServerStatus(const char* modem, char* tag)
{
    DIR* dirp;
    struct dirent* dentp;
    config deflt;
    char fifomatch[80];
    int fifomatchlen;

    if (!(dirp = opendir("."))) {
	syslog(LOG_ERR, "%s: opendir: %m", SPOOLDIR);
	sendError("Problem accessing spool directory.");
	done(-1, "EXIT");
    }
    /*
     * Setup a prefix for matching potential FIFO files.
     * We do this carefully and in a way that insures we
     * use only the definitions in config.h.
     */
    if (strcmp(modem, MODEM_ANY) == 0)
	modem = "";
    sprintf(fifomatch, "%s.%.*s", FAX_FIFO,
	sizeof (fifomatch) - (sizeof (FAX_FIFO)+2), modem);
    fifomatchlen = strlen(fifomatch);
    while ((dentp = readdir(dirp)) != 0) {
	int fifo;

	if (strncmp(dentp->d_name, fifomatch, fifomatchlen) != 0)
	    continue;
	fifo = open(dentp->d_name, O_WRONLY|O_NDELAY);
	if (fifo != -1) {
	    config configuration;
	    char fileName[1024];
	    char* cp;

	    (void) close(fifo);
	    cp = strchr(dentp->d_name, '.') + 1;
	    sprintf(fileName, "%s.%s", FAX_CONFIG, cp);
	    getConfig(fileName, &configuration, &deflt);
	    if (version > 0) {
		char serverStatus[1024];
		char* tp;

		sprintf(fileName, "%s/%s", FAX_STATUSDIR, cp);
		getServerStatus(fileName, serverStatus);
		/*
		 * Convert fifo name from canonical format back
		 * to a pathname by replacing '_'s with '/'s.
		 */
		for (tp = cp; tp = strchr(tp, '_'); *tp = '/')
		    ;
		sendClient("server", "%s:%s:%s",
		    configuration.faxNumber, cp, serverStatus);
	    } else
		sendClient("server", "%s", configuration.faxNumber);
	}
    }
    (void) closedir(dirp);
}

static void
sendClientJobStatus(Job* job, const char* jobname)
{
    if (version > 0) {
	char tts[30];
	if (job->tts != 0)
	    strftime(tts, sizeof (tts), "%Y/%m/%d %H.%M.%S",
		localtime(&job->tts));
	else
	    strcpy(tts, "asap");
	sendClient("jobStatus", "%s:%s:%s:%s:%s:%s",
	    jobname, job->sender, tts, job->external, job->modem, job->status);
    } else
	sendClient("jobStatus", "%s:%s:%d:%s",
	    jobname, job->sender, job->tts, job->external);
}

static void
sendClientJobLocked(Job* job, const char* jobname)
{
    if (version > 0)
	sendClient("jobStatus", "%s:%s:locked:%s:%s:%s",
	    jobname, job->sender, job->external, job->modem, job->status);
    else
	sendClient("jobLocked", "%s:%s:0:%s",
	    jobname, job->sender, job->external);
}

void
sendAllStatus(const char* modem, char* tag)
{
    Job* job;

    if (!jobList)
	jobList = readJobs();
    for(job = jobList; job; job = job->next) {
	char *jobname = job->qfile+strlen(FAX_SENDDIR)+2;
	if (!modemMatch(modem, job->modem))
	    continue;
	if (job->flags & JOB_SENT)
	    continue;
	job->flags |= JOB_SENT;
	if (job->flags & JOB_LOCKED)
	    sendClientJobLocked(job, jobname);
	else
	    sendClientJobStatus(job, jobname);
    }
}

void
sendJobStatus(const char* modem, char* onwhat)
{
    Job* job;

    if (!jobList)
	jobList = readJobs();
    for(job = jobList; job; job = job->next) {
	char *jobname = job->qfile+strlen(FAX_SENDDIR)+2;
	if (!modemMatch(modem, job->modem))
	    continue;
	if ((job->flags & JOB_SENT) || strcmp(jobname, onwhat) != 0)
	    continue;
	job->flags |= JOB_SENT;
	if (job->flags & JOB_LOCKED)
	    sendClientJobLocked(job, jobname);
	else
	    sendClientJobStatus(job, jobname);
    }
}

void
sendUserStatus(const char* modem, char* onwhat)
{
    Job* job;

    if (!jobList)
	jobList = readJobs();
    for(job = jobList; job; job = job->next) {
	char *jobname = job->qfile+strlen(FAX_SENDDIR)+2;
	if (!modemMatch(modem, job->modem))
	    continue;
	if (job->flags & JOB_SENT)
	    continue;
	if (job->flags & JOB_LOCKED) {
	    job->flags |= JOB_SENT;
	    sendClientJobLocked(job, jobname);
	} else if (strcmp(job->sender, onwhat) == 0) {
	    job->flags |= JOB_SENT;
	    sendClientJobStatus(job, jobname);
	}
    }
}

static int
isFAXImage(TIFF* tif)
{
    u_short w;
    if (TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &w) && w != 1)
	return (0);
    if (TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &w) && w != 1)
	return (0);
    if (!TIFFGetField(tif, TIFFTAG_COMPRESSION, &w) ||
      (w != COMPRESSION_CCITTFAX3 && w != COMPRESSION_CCITTFAX4))
	return (0);
    if (!TIFFGetField(tif, TIFFTAG_PHOTOMETRIC, &w) ||
      (w != PHOTOMETRIC_MINISWHITE && w != PHOTOMETRIC_MINISBLACK))
	return (0);
    return (1);
}

static void
sanitize(char* dst, const char* src, u_int maxlen)
{
    u_int i;
    for (i = 0; i < maxlen-1 && src[i] != '\0'; i++)
	dst[i] = (isascii(src[i]) && isprint(src[i]) ? src[i] : '?');
    dst[i] = '\0';
}

static int
readQFile(int fd, char* qfile, int beingReceived, struct stat* sb)
{
    int ok = 0;
    TIFF* tif = TIFFFdOpen(fd, qfile, "r");
    if (tif) {
	ok = isFAXImage(tif);
	if (ok) {
	    u_long pageWidth, pageLength;
	    char* cp;
	    char sender[80];
	    char date[30];
	    float resolution = 98;
	    int c, i, npages;

	    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &pageWidth);
	    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &pageLength);
	    if (TIFFGetField(tif, TIFFTAG_YRESOLUTION, &resolution)) {
		u_short resunit = RESUNIT_NONE;
		TIFFGetField(tif, TIFFTAG_RESOLUTIONUNIT, &resunit);
		if (resunit == RESUNIT_CENTIMETER)
		    resolution *= 25.4;
	    } else
		resolution = 98;
	    if (TIFFGetField(tif, TIFFTAG_IMAGEDESCRIPTION, &cp))
		sanitize(sender, cp, sizeof (sender));
	    else
		strcpy(sender, "<unknown>");
	    if (TIFFGetField(tif, TIFFTAG_DATETIME, &cp))
		sanitize(date, cp, sizeof (date));
	    else
		strftime(date, sizeof (date), "%Y:%m:%d %H:%M:%S",
		    localtime(&sb->st_mtime));
	    npages = 0;
	    do {
		npages++;
	    } while (TIFFReadDirectory(tif));
	    if (version > 0)
		sendClient("recvJob", "%d:%lu:%lu:%3.1f:%u:%s:%s",
		    beingReceived, pageWidth, pageLength, resolution,
		    npages, date, sender);
	    else
		sendClient("recvJob", "%d:%lu:%lu:%3.1f:%u:%u:%s",
		    beingReceived, pageWidth, pageLength, resolution,
		    npages, sb->st_mtime, sender);
	}
	TIFFClose(tif);
    }
    return (ok);
}

/*
 * NB: received files must be opened with write access
 * under svr4 because the flock emulation needs it.
 */
#if defined(svr4) || defined(hpux)
#define	RECV_OMODE	O_RDWR
#else
#define	RECV_OMODE	O_RDONLY
#endif

void
sendRecvStatus(const char* modem, char* tag)
{
    DIR* dir = opendir(FAX_RECVDIR);
    if (dir != NULL) {
	struct dirent* dp;

	TIFFSetErrorHandler(NULL);
	TIFFSetWarningHandler(NULL);
	while (dp = readdir(dir)) {
	    char entry[1024];
	    struct stat sb;
	    int fd;

	    if (strncmp(dp->d_name, "fax", 3) != 0)
		continue;
	    sprintf(entry, "%s/%s", FAX_RECVDIR, dp->d_name);
	    if (stat(entry, &sb) < 0 || (sb.st_mode & S_IFMT) != S_IFREG)
		continue;
	    fd = open(entry, RECV_OMODE);
	    if (fd > 0) {
		int beingReceived =
		   (flock(fd, LOCK_EX|LOCK_NB) < 0 && errno == EWOULDBLOCK);
		(void) readQFile(fd, entry, beingReceived, &sb);
		close(fd);
	    }
	}
	closedir(dir);
    } else
	sendAndLogError("Can not access receive queue directory \"%s\".",
	    FAX_RECVDIR);
}

static void
getConfig(char* fileName, config* configp, config* deflt)
{
    char* cp;
    FILE* configFile;
    char configLine[1024];

    if (!configp)
	return;
    if (deflt) {
	strcpy(configp->faxNumber, deflt->faxNumber);
    } else {
	configp->faxNumber[0] = '\0';
    }
    if (!fileName || !(configFile = fopen(fileName, "r")))
	return;
    while (fgets(configLine, sizeof (configLine)-1, configFile)) {
	if (! (cp = strchr(configLine, '#')))
	    cp = strchr(configLine, '\n');
	if (cp)
	    *cp = '\0';
	if ((cp = strchr(configLine, ':')) != 0)
	    for (*cp++ = '\0'; isspace(*cp); cp++)
		;
	else
	    continue;
	if (strcasecmp(configLine, "FAXNumber") == 0) {
	    strcpy(configp->faxNumber, cp);
	    break;
	}
    }
    (void) fclose(configFile);
}

static void
getServerStatus(char* fileName, char buf[1024])
{
    int fd = open(fileName, O_RDONLY);
    if (fd > 0) {
	int n;
	flock(fd, LOCK_SH);
	n = read(fd, buf, 1024-1);
	buf[n] = '\0';
	if (n > 0 && buf[n-1] == '\n')
	    buf[n-1] = '\0';
	close(fd);
    } else
	strcpy(buf, "No status");
}
