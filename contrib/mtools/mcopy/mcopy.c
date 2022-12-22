/*	BSDI	$Id: mcopy.c,v 1.3 1994/01/29 16:36:33 polk Exp $	*/
/*
 * A front-end to the mread/mwrite commands.
 *
 * Emmet P. Gray			US Army, HQ III Corps & Fort Hood
 * ...!uunet!uiucuxc!fthood!egray	Attn: AFZF-DE-ENV
 * fthood!egray@uxc.cso.uiuc.edu	Directorate of Engineering & Housing
 * 					Environmental Management Office
 * 					Fort Hood, TX 76544-5057
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "patchlevel.h"

#define NONE	0
#define MREAD	1
#define MWRITE	2
#define MKDIR

#ifndef WEXITSTATUS
#define WEXITSTATUS(x) (((x)>>8)&0xff)
#endif /* WEXITSTATUS */

main(argc, argv)
int argc;
char *argv[];
{
	extern int optind;
	extern char *optarg;
	int i, oops, msdos_args, unix_args, destination;
	char **nargv, *malloc();
	char *mrw;
	void exit();
					/* get command line options */
	msdos_args = 0;
	unix_args = 0;
	oops = 0;
	while ((i = getopt(argc, argv, "tnvm")) != EOF) {
		switch (i) {
			case 't':
			case 'n':
			case 'v':
			case 'm':
				break;
			default:
				oops = 1;
				break;
		}
	}

	if (oops || (argc - optind) < 2) {
		fprintf(stderr, "Mtools version %s, dated %s\n", VERSION, DATE);
		fprintf(stderr, "Usage: %s [-tnvm] sourcefile targetfile\n", argv[0]);
		fprintf(stderr, "       %s [-tnvm] sourcefile [sourcefiles...] targetdirectory\n", argv[0]);
		exit(1);
	}
					/* last file determines the direction */
	if (argv[argc - 1][1] == ':')
		destination = MWRITE;
	else
		destination = MREAD;

					/* count the arguments */
	for (i = optind; i < argc; i++) {
		if (argv[i][1] == ':')
			msdos_args++;
		else
			unix_args++;
	}

	if (destination == MREAD && unix_args > 1) {
		fprintf(stderr, "%s: Duplicate destination files\n", argv[0]); 
		exit(1);
	}
					/* chaining of mread and mwrite */
	if (destination == MWRITE && msdos_args > 1)
		chain(argc, argv);

	/*
	 * Copy the *argv[] array in case your Unix doesn't end the array
	 * with a null when it passes it to main()
	 */
	nargv = (char **) malloc((unsigned int) (argc + 1) * sizeof(*argv));
	nargv[0] = "mcopy";
	for (i = 1; i < argc; i++)
		nargv[i] = argv[i];
	nargv[argc] = NULL;

	mrw = destination == MWRITE ? "mwrite" : "mread";
	execvp(mrw, nargv);
	perror(mrw);
	exit(1);
}

chain(argc, argv)
int argc;
char *argv[];
{
	extern int optind;
	int i, j, pid, status_read, status_write;
	char *tmpdir, *mktemp(), **nargv, *malloc(), buf[256], *strcpy();
	char *unixname(), *realloc();
	void exit();

	nargv = (char **) malloc((unsigned int) (argc + 4) * sizeof(*argv));
	nargv[0] = "mread";
	nargv[1] = "-n";
					/* copy only the msdos arguments */
	j = 2;
	for (i = optind; i < argc -1; i++) {
		if (argv[i][1] == ':')
			nargv[j++] = argv[i];
	}
					/* create a temp directory */
	tmpdir = mktemp("/tmp/mtoolsXXXXXX");
	if (mkdir(tmpdir, 0777) < 0) {
		perror("mkdir");
		exit(1);
	}

	nargv[j++] = tmpdir;
	nargv[j] = NULL;

	printf("reading...\n");
	if (!(pid = fork())) {
		execvp("mread", nargv);
		perror("mread");
		exit (1);
	}

	while (wait(&status_read) != pid)
		;
					/* we blew it... */
	if (WEXITSTATUS(status_read) == 1)
		exit(1);
					/* reconstruct the argv[] */
	nargv[0] = "sh";
	nargv[1] = "-c";
	nargv[2] = (char *) malloc(7);
	strcpy(nargv[2], "mwrite");

	j = 3;
	for (i = 1; i < argc -1; i++) {
		/*
		 * Substitute the msdos arguments for their unix
		 * counterparts that have already been copied to tmpdir.
		 */
		if (argv[i][1] == ':')
			sprintf(buf, "%s/%s", tmpdir, unixname(argv[i]));
		else
			strcpy(buf, argv[i]);

		nargv[2] = (char *) realloc(nargv[2], sizeof(nargv[2]) + sizeof(buf));
		strcat(nargv[2], " ");
		strcat(nargv[2], buf);
	}
					/* protect last arg from expansion */
	sprintf(buf, "'%s'", argv[i]);
	nargv[2] = (char *) realloc(nargv[2], sizeof(nargv[2]) + sizeof(buf));
	strcat(nargv[2], " ");
	strcat(nargv[2], buf);

	nargv[3] = NULL;

	printf("writing...\n");
	if (!(pid = fork())) {
		execvp("sh", nargv);
		perror("sh");
		exit(1);
	}

	while (wait(&status_write) != pid)
		;
					/* clobber the directory */
	sprintf(buf, "rm -fr %s", tmpdir);
	system(buf);
	exit(WEXITSTATUS(status_write));
}

char *
unixname(filename)
char *filename;
{
	char *s, *temp, *strcpy(), *strrchr(), buf[256];
	static char ans[13];

	strcpy(buf, filename);
	temp = buf;
					/* skip drive letter */
	if (buf[0] && buf[1] == ':')
		temp = &buf[2];
					/* find the last separator */
	if (s = strrchr(temp, '/'))
		temp = s + 1;
	if (s = strrchr(temp, '\\'))
		temp = s + 1;
					/* xlate to lower case */
	for (s = temp; *s; ++s) {
		if (isupper(*s))
			*s = tolower(*s);
	}

	strcpy(ans, temp);
	return(ans);
}

#ifdef MKDIR
/* ARGSUSED */
mkdir(path, mode)
char *path;
int mode;
{
	char buf[256];
	sprintf(buf, "mkdir %s", path);
	return(system(buf));
}
#endif /* MKDIR */
