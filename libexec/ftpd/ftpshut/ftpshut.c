/* ftpshut 
 * ======= 
 * creates the ftpd shutdown file.
 */

#include "config.h"

#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/param.h>

#include "pathnames.h"

#define  WIDTH  70

int denyoffset = 10;            /* default deny time   */
int discoffset = 5;             /* default disc time   */
char *message = "System shutdown at %s";    /* default message     */

extern version[];

void
massage(char *buf)
{
    char *sp = NULL;
    char *ptr;
    int i = 0;
    int j = 0;

    ptr = buf;

    while (*ptr++ != '\0') {
        ++i;

        /* if we have a space, keep track of where and at what "count" */

        if (*ptr == ' ') {
            sp = ptr;
            j = i;
        }
        /* magic cookies... */

        if (*ptr == '%') {
            ++ptr;
            switch (*ptr) {
            case 'r':
            case 's':
            case 'd':
            case 'T':
                i = i + 24;
                break;
            case '\n':
                i = 0;
                break;
            case 'C':
            case 'R':
            case 'L':
            case 'U':
                i = i + 10;
                break;
            case 'M':
            case 'N':
                i = i + 3;
                break;
            case '\0':
                return;
                break;
            default:
                i = i + 1;
                break;
            }
        }
        /* break up the long lines... */

        if ((i >= WIDTH) && (sp != NULL)) {
            *sp = '\n';
            sp = NULL;
            i = i - j;
        }
    }
}

int
main(int argc, char **argv)
{
    time_t c_time;
    struct tm *tp;

    char buf[BUFSIZ];

    int c;
    extern int optind;
    extern char *optarg;

    FILE *fp;
    FILE *accessfile;
    char *aclbuf,
     *myaclbuf,
     *crptr;
    char *sp;
    char linebuf[1024];
    struct stat finfo;

    struct passwd *pwent;

    while ((c = getopt(argc, argv, "vl:d:")) != EOF) {
        switch (c) {
        case 'l':
            denyoffset = atoi(optarg);
            break;
        case 'd':
            discoffset = atoi(optarg);
            break;
		case 'v':
			fprintf(stderr, "%s\n", version);
			exit(0);
        default:
            fprintf(stderr,
                "Usage: %s [-d min] [-l min] now [\"message\"]\n", argv[0]);
            fprintf(stderr,
                "       %s [-d min] [-l min] +dd [\"message\"]\n", argv[0]);
            fprintf(stderr,
               "       %s [-d min] [-l min] HHMM [\"message\"]\n", argv[0]);
            exit(-1);
        }
    }

    if ((accessfile = fopen(_PATH_FTPACCESS, "r")) == NULL) {
        if (errno != ENOENT)
            perror("ftpshut: could not open() access file");
        exit(1);
    }
    if (stat(_PATH_FTPACCESS, &finfo)) {
        perror("ftpshut: could not stat() access file");
        exit(1);
    }
    if (finfo.st_size == 0) {
        printf("ftpshut: no service shutdown path defined\n");
        exit(0);
    } else {
        if (!(aclbuf = (char *) malloc(finfo.st_size + 1))) {
            perror("ftpcount: could not malloc aclbuf");
            exit(1);
        }
        fread(aclbuf, finfo.st_size, 1, accessfile);
        *(aclbuf + finfo.st_size) = '\0';
    }

    myaclbuf = aclbuf;
    while (*myaclbuf != NULL) {
        if (strncasecmp(myaclbuf, "shutdown", 8) == 0) {
            for (crptr = myaclbuf; *crptr++ != '\n';) ;
            *--crptr = NULL;
            strcpy(linebuf, myaclbuf);
            *crptr = '\n';
            (void) strtok(linebuf, " \t");  /* returns "shutdown" */
            sp = strtok(NULL, " \t");   /* returns shutdown path */
        }
        while (*myaclbuf && *myaclbuf++ != '\n') ;
    }

    /* three cases 
     * -- now 
     * -- +ddd 
     * -- HHMM 
     */

    c = -1;

    if (optind < argc) {
        if (!strcasecmp(argv[optind], "now")) {
            c_time = time(0);
            tp = localtime(&c_time);
        } else if ((*(argv[optind])) == '+') {
            c_time = time(0);
            c_time += 60 * atoi(++(argv[optind]));
            tp = localtime(&c_time);
        } else if ((c = atoi(argv[optind])) >= 0) {
            c_time = time(0);
            tp = localtime(&c_time);
            tp->tm_hour = c / 100;
            tp->tm_min = c % 100;

            if ((tp->tm_hour > 23) || (tp->tm_min > 59)) {
                fprintf(stderr, "Illegal time format.\n");
                return(1);
            }
        }
    }
    if (c_time <= 0) {
        fprintf(stderr, "Usage: %s [-d min] [-l min] now [\"message\"]\n",
                argv[0]);
        fprintf(stderr, "       %s [-d min] [-l min] +dd [\"message\"]\n",
                argv[0]);
        fprintf(stderr, "       %s [-d min] [-l min] HHMM [\"message\"]\n",
                argv[0]);
        return(1);
    }
    /* do we have a shutdown message? */

    if (++optind < argc)
        strcpy(buf, argv[optind++]);
    else
        strcpy(buf, message);

    massage(buf);

    if ((fp = fopen(sp, "w")) == NULL) {
        perror("Couldn't open shutdown file");
        return(1);
    }
    fprintf(fp, "%.4d %.2d %.2d %.2d %.2d %.4d %.4d\n",
            (tp->tm_year) + 1900,
            tp->tm_mon,
            tp->tm_mday,
            tp->tm_hour,
            tp->tm_min,
            denyoffset,
            discoffset);
    fprintf(fp, "%s\n", buf);
    fclose(fp);
}
