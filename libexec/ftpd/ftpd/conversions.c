#include "config.h"

#include <stdio.h>
#include <errno.h>
#include <syslog.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "conversions.h"
#include "extensions.h"
#include "pathnames.h"

/*************************************************************************/
/* FUNCTION  : readconv                                                  */
/* PURPOSE   : Read the conversions into memory                          */
/* ARGUMENTS : The pathname of the conversion file                       */
/* RETURNS   : 0 if error, 1 if no error                                 */
/*************************************************************************/

char *convbuf = NULL;
struct convert *cvtptr;

struct str2int {
    char *string;
    int value;
};

struct str2int c_list[] =
{"T_REG", T_REG,
 "T_ASCII", T_ASCII,
 "T_DIR", T_DIR,
 "O_COMPRESS", O_COMPRESS,
 "O_UNCOMPRESS", O_UNCOMPRESS,
 "O_TAR", O_TAR,
 NULL, 0,};

int
conv(char *str)
{
    int rc = 0;
    int counter;

    /* check for presence of ALL items in string... */

    for (counter = 0; c_list[counter].string; ++counter)
        if (strstr(str, c_list[counter].string))
            rc = rc | c_list[counter].value;
    return (rc);
}

int
readconv(char *convpath)
{
    FILE *convfile;
    struct stat finfo;

    if (stat(convpath, &finfo) != 0) {
        syslog(LOG_ERR, "cannot stat conversion file %s: %s", convpath,
               strerror(errno));
        return (0);
    }
    if ((convfile = fopen(convpath, "r")) == NULL) {
        if (errno != ENOENT)
            syslog(LOG_ERR, "cannot open conversion file %s: %s",
                   convpath, strerror(errno));
        return (0);
    }
    if (finfo.st_size == 0) {
        convbuf = (char *) calloc(1, 1);
    } else {
        if (!(convbuf = (char *) malloc((unsigned) finfo.st_size + 1))) {
            syslog(LOG_ERR, "could not malloc convbuf (%d bytes)", finfo.st_size + 1);
            return (0);
        }
        if (!fread(convbuf, (size_t) finfo.st_size, 1, convfile)) {
            syslog(LOG_ERR, "error reading conv file %s: %s", convpath,
                   strerror(errno));
            convbuf = NULL;
            return (0);
        }
        *(convbuf + finfo.st_size) = '\0';
    }
    return (1);
}

void
parseconv(void)
{
    char *ptr;
    char *convptr = convbuf,
     *line;
    char **argv[51],
    **ap = (char **) argv,
     *p,
     *val;
    struct convert *cptr;

    if (!convbuf || !(*convbuf))
        return;

    /* build list, initialize to zero. */
    cvtptr = (struct convert *) calloc(1, sizeof(struct convert));

    /* read through convbuf, stripping comments. */
    while (*convptr != '\0') {
        line = convptr;
        while (*convptr && *convptr != '\n')
            convptr++;
        *convptr++ = '\0';

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

        /* if we didn't read 7 things, skip that line... */

        /* add element to beginning of list */
        cptr = (struct convert *) calloc(1, sizeof(struct convert));

        cptr->next = cvtptr;
        cvtptr = cptr;

        cptr->stripprefix = (char *) argv[0];
        cptr->stripfix = (char *) argv[1];
        cptr->prefix = (char *) argv[2];
        cptr->postfix = (char *) argv[3];
        cptr->external_cmd = (char *) argv[4];
        cptr->types = conv((char *) argv[5]);
        cptr->options = conv((char *) argv[6]);
        cptr->name = (char *) argv[7];
    }
}

void
conv_init(void)
{
#ifdef VERBOSE
    struct convert *cptr;

#endif

    if ((readconv(_PATH_CVT)) < 0)
        return;
    parseconv();
}
