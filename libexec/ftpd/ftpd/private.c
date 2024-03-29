#ifndef NO_PRIVATE

#include "config.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <grp.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>

#include "pathnames.h"
#include "extensions.h"

#define MAXGROUPLEN 100
char *passbuf = NULL;
char groupname[MAXGROUPLEN];
int group_given = 0;

struct acgrp {
    char  gname[MAXGROUPLEN];    /* access group name */
    char  gpass[MAXGROUPLEN];    /* access group password */
    char  gr_name[MAXGROUPLEN];  /* group to setgid() to */
	gid_t gr_gid;
	struct acgrp *next;
};

struct acgrp *privptr;

extern int lgi_failure_threshold,
  autospout_free;
extern char remotehost[],
  remoteaddr[],
 *autospout;
int group_attempts;

void
parsepriv(void)
{
    char *ptr;
    char *acptr = passbuf,
     *line;
    char **argv[51],
    **ap = (char **) argv,
     *p,
     *val;
    struct acgrp *aptr;
	struct group *gr;

    if (!passbuf || !(*passbuf))
        return;

    /* build list, initialize to zero. */
    privptr = (struct acgrp *) calloc(1, sizeof(struct acgrp));

    /* read through passbuf, stripping comments. */
    while (*acptr != '\0') {
        line = acptr;
        while (*acptr && *acptr != '\n')
            acptr++;
        *acptr++ = '\0';

        /* deal with comments */
        if ((ptr = strchr(line, '#')) != NULL)
            *ptr = '\0';

        if (*line == '\0')
            continue;

        ap = (char **) argv;

        /* parse the lines... */
        for (p = line; p != NULL;) {
            while ((val = (char *) strsep(&p, ":\n")) != NULL && *val == '\0') ;
            *ap = val;
            if (**ap == ' ')
                *ap = NULL;
            *ap++;
        }
        *ap = 0;

        /* if we didn't read 3 things, skip that line... */

        /* add element to beginning of list */
        aptr = (struct acgrp *) calloc(1, sizeof(struct acgrp));

        aptr->next = privptr;
        privptr = aptr;

        strcpy(aptr->gname, (char *) argv[0]);
        strcpy(aptr->gpass, (char *) argv[1]);
        strcpy(aptr->gr_name, (char *) argv[2]);
    	if ((gr = getgrnam(aptr->gr_name)) != NULL)
        	aptr->gr_gid = gr->gr_gid;
		else
			aptr->gr_gid = -1;
    }
}

/*************************************************************************/
/* FUNCTION  : priv_setup                                                */
/* PURPOSE   : Set things up to use the private access password file.    */
/* ARGUMENTS : path, the path to the private access password file        */
/*************************************************************************/

void
priv_setup(char *path)
{
    FILE *prvfile;
    struct stat finfo;

    passbuf = (char *) NULL;

    if (stat(path, &finfo) != 0) {
        syslog(LOG_ERR, "cannot stat private access file %s: %s", path,
               strerror(errno));
        return;
    }
    if ((prvfile = fopen(path, "r")) == NULL) {
        if (errno != ENOENT)
            syslog(LOG_ERR, "cannot open private access file %s: %s",
                   path, strerror(errno));
        return;
    }
    if (finfo.st_size == 0) {
        passbuf = (char *) calloc(1, 1);
    } else {
        if (!(passbuf = malloc((unsigned) finfo.st_size + 1))) {
            (void) syslog(LOG_ERR, "could not malloc passbuf (%d bytes)",
                          finfo.st_size + 1);
            return;
        }
        if (!fread(passbuf, (size_t) finfo.st_size, 1, prvfile)) {
            (void) syslog(LOG_ERR, "error reading private access file %s: %s",
                          path, strerror(errno));
        }
        *(passbuf + finfo.st_size) = '\0';
    }

	(void) parsepriv();
}

/*************************************************************************/
/* FUNCTION  : priv_getent                                               */
/* PURPOSE   : Retrieve an entry from the in-memory copy of the group    */
/* access file.                                              */
/* ARGUMENTS : pointer to group name                                     */
/*************************************************************************/

struct acgrp *
priv_getent(char *group)
{
	struct acgrp *ptr;

	for (ptr = privptr; ptr; ptr=ptr->next)
		if (!strcmp(group, ptr->gname))
			return(ptr);

    return (NULL);
}

/*************************************************************************/
/* FUNCTION  : priv_group                                                */
/* PURPOSE   :                                                           */
/* ARGUMENTS :                                                           */
/*************************************************************************/

void
priv_group(char *group)
{
    if (strlen(group) < MAXGROUPLEN) {
        strncpy(groupname, group, MAXGROUPLEN);
        group_given = 1;
        reply(200, "Request for access to group %s accepted.", group);
    } else {
        group_given = 0;
        reply(500, "Illegal group name");
    }

}

/*************************************************************************/
/* FUNCTION  : priv_gpass                                                */
/* PURPOSE   : validate the group access request, and if OK place user   */
/* in the proper group.                                      */
/* ARGUMENTS : group access password                                     */
/*************************************************************************/

void
priv_gpass(char *gpass)
{
    char *xgpass,
     *salt;
#ifndef NO_CRYPT_PROTO
    char *crypt(const char *, const char *);
#endif
    struct acgrp *grp;
    struct group *gr;
    uid_t uid;
    gid_t gid;

    if (group_given == 0) {
        reply(503, "Give group name with SITE GROUP first.");
        return;
    }
    /* OK, now they're getting a chance to specify a password.  Make them
     * give the group name again if they fail... */
    group_given = 0;

    if (passbuf != NULL) {
        grp = priv_getent(groupname);

        if (grp == NULL)
            salt = "xx";
        else
            salt = grp->gpass;

        xgpass = crypt(gpass, salt);
    } else
        grp = NULL;

    /* The strcmp does not catch null passwords! */
    if (grp == NULL || *grp->gpass == '\0' || strcmp(xgpass, grp->gpass)) {
        reply(530, "Group access request incorrect.");
        grp = NULL;
        if (++group_attempts >= lgi_failure_threshold) {
            syslog(LOG_NOTICE,
                   "repeated group access failures from %s [%s], group %s",
                   remotehost, remoteaddr, groupname);
            exit(0);
        }
        sleep(group_attempts);  /* slow down password crackers */
        return;
    }
    /* THIS CODE NEEDS CHECKING BY INFORMED, SECURITY-CONSCIOUS PEOPLE */
    if ( (gid = grp->gr_gid) == -1 ) {
        reply(500, "Configuration error -- group access not granted");
        syslog(LOG_ERR, "group %s does not exist in group file", grp->gr_name);
        return;
    }

    uid = geteuid();
    seteuid(0);
    setegid(gid);
    seteuid(uid);
    /* END CODE */

    reply(200, "Group access enabled.");
    group_attempts = 0;
}
#endif /* !NO_PRIVATE */
