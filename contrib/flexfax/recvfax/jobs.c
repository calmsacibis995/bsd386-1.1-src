/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/recvfax/jobs.c,v 1.1.1.1 1994/01/14 23:10:40 torek Exp $
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

#include <sys/file.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <pwd.h>
#include <errno.h>

#include "flock.h"

static int
readJob(Job* job)
{
    char line[1024];
    char* cp;
    char* tag;
    FILE* fp;

    fp = fopen((char*) job->qfile, "r");
    if (fp == NULL)
	return (0);
    if (flock(fileno(fp), LOCK_SH|LOCK_NB) < 0 && errno == EWOULDBLOCK)
	job->flags |= JOB_LOCKED;
    while (fgets(line, sizeof (line) - 1, fp)) {
	if (line[0] == '#')
	    continue;
	if (cp = strchr(line, '\n'))
	    *cp = '\0';
	tag = strchr(line, ':');
	if (!tag || !*tag)
	    continue;
	*tag++ = '\0';
	while (isspace(*tag))
	    tag++;
	if (isCmd("tts")) {
	    job->tts = atoi(tag);
	} else if (isCmd("killtime")) {
	    job->killtime = strdup(tag);
	} else if (isCmd("number")) {
	    job->number = strdup(tag);
	} else if (isCmd("external")) {
	    job->external = strdup(tag);
	} else if (isCmd("sender")) {
	    job->sender = strdup(tag);
	} else if (isCmd("mailaddr")) {
	    job->mailaddr = strdup(tag);
	} else if (isCmd("status")) {
	    job->status = strdup(tag);
	} else if (isCmd("modem")) {
	    job->modem = strdup(tag);
	}
    }
    if (job->status == NULL)
	job->status = strdup("");
    if (job->modem == NULL)
	job->modem = strdup(MODEM_ANY);
    if (job->external == NULL && job->number != NULL)
	job->external = strdup(job->number);
    if ((job->flags & JOB_LOCKED) == 0)
	(void) flock(fileno(fp), LOCK_UN);
    fclose(fp);
    /* NB: number and sender are uniformly assumed to exist */
    return (job->number != NULL && job->sender != NULL);
}

static Job*
newJob(const char* qfile)
{
    Job* job = (Job*) malloc(sizeof(Job));
    memset((char*) job, 0, sizeof (*job));
    job->qfile = malloc(strlen(FAX_SENDDIR) + strlen(qfile) + 2);
    sprintf(job->qfile, "%s/%s", FAX_SENDDIR, qfile);
    return (job);
}

static void
freeJob(Job* job)
{
    if (job->qfile)
	free(job->qfile);
    if (job->killtime)
	free(job->killtime);
    if (job->sender)
	free(job->sender);
    if (job->mailaddr)
	free(job->mailaddr);
    if (job->number)
	free(job->number);
    if (job->external)
	free(job->external);
    if (job->status)
	free(job->status);
    if (job->modem)
	free(job->modem);
    free(job);
}

Job*
readJobs()
{
    DIR* dirp;
    struct dirent* dentp;
    Job* jobs = 0;
    Job* jobp = 0;

    if (!(dirp = opendir(FAX_SENDDIR))) {
	syslog(LOG_ERR, "%s: opendir: %m", FAX_SENDDIR);
	sendError("Problem accessing send directory.");
	done(-1, "EXIT");
    }
    (void) flock(dirp->dd_fd, LOCK_SH);
    for (dentp = readdir(dirp); dentp; dentp = readdir(dirp)) {
	if (dentp->d_name[0] != 'q')
	    continue;
	jobp = newJob(dentp->d_name);
	if (jobp) {
	    if (readJob(jobp)) {
		jobp->next = jobs;
		jobs = jobp;
	    } else
		freeJob(jobp);
	}
    }
    (void) flock(dirp->dd_fd, LOCK_UN);
    closedir(dirp);
    return jobs;
}

int
modemMatch(const char* a, const char* b)
{
    return strcmp(a, MODEM_ANY) == 0 || strcmp(b, MODEM_ANY) == 0 ||
	strcmp(a, b) == 0;
}

static int
checkUser(const char* requestor, struct passwd* pwd)
{
    char buf[1024];
    char* cp;

    buf[0] = '\0';
    if (pwd->pw_gecos) {
	if (pwd->pw_gecos[0] == '&') {
	    strcpy(buf, pwd->pw_name);
	    strcat(buf, pwd->pw_gecos+1);
	    if (islower(buf[0]))
		buf[0] = toupper(buf[0]);
	} else
	    strcpy(buf, pwd->pw_gecos);
	if ((cp = strchr(buf,',')) != 0)
	    *cp = '\0';
    } else
	strcpy(buf, pwd->pw_name);
    if (debug) {
	if (*buf)
	     syslog(LOG_DEBUG, "%s user: \"%s\"", pwd->pw_name, buf);
	else
	     syslog(LOG_DEBUG, "name of %s user unknown", pwd->pw_name);
    }
    return (strcmp(requestor, buf) == 0);
}

void
applyToJob(const char* modem, char* tag, const char* op, jobFunc* f)
{
    Job** job;
    char* requestor;
    char* arg;

    if (!jobList)
	jobList = readJobs();

    if ((requestor = strchr(tag, ':')) == 0) {
	protocolBotch("no requestor name for \"%s\" command.", op);
	done(1, "EXIT");
    }
    *requestor++ = '\0';
    arg = strchr(requestor, ':');
    if (arg)
	*arg++ = '\0';

    for (job = &jobList; *job; job = &((*job)->next)) {
	char *jobname = (*job)->qfile+strlen(FAX_SENDDIR)+2;
	if (modemMatch(modem, (*job)->modem) && strcmp(jobname, tag) == 0)
	    break;
    }
    if (!*job) {
	sendClient("notQueued", "%s", tag);
	return;
    }
    /*
     * Validate requestor is permitted to do op to the
     * requested job.  We permit the person that submitted
     * the job, the fax user, and root.  Using the GECOS
     * field in doing the comparison is a crock, but not
     * something to change now--leave it for a protocol
     * redesign.
     */
    if (strcmp(requestor, (*job)->sender) != 0) {	/* not the sender */
	struct passwd* pwd = getpwuid(getuid());

	if (!pwd) {
	    syslog(LOG_ERR, "getpwuid failed for uid %d: %m", getuid());
	    pwd = getpwuid(geteuid());
	}
	if (!pwd) {
	    syslog(LOG_ERR, "getpwuid failed for effective uid %d: %m",
		geteuid());
	    sendClient("sOwner", "%s", tag);
	    return;
	}
	if (!checkUser(requestor, pwd)) {		/* not fax user */
	    pwd = getpwnam("root");
	    if (!pwd) {
		syslog(LOG_ERR, "getpwnam failed for \"root\": %m");
		sendClient("sOwner", "%s", tag);
		return;
	    }
	    if (!checkUser(requestor, pwd)) {		/* not root user */
		sendClient("jobOwner", "%s", tag);
		return;
	    }
	}
    }
    if (debug)
	syslog(LOG_DEBUG, "%s request by %s for %s", op, requestor, tag);
    (*f)(*job, tag, arg);
    if ((*job)->flags & JOB_INVALID)
	*job = (*job)->next;
}
