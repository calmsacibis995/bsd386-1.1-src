/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

/*
 * If you change the layout of the at file, be sure and update
 * AT_VERSION to be different so at can tell.
 *
 * AT_VERSION and AT_LOCKID must be the same length.
 */
#define AT_VERSION "# at job\n"
#define AT_LOCKID  "# locked\n"
typedef	struct	JobQ_s {
	long	id;		/* job id number */
	time_t	when;		/* when to run */
	int	notify;		/* user wants mail for sure */

	uid_t	owner;
	gid_t	gid;
	int	ngroups;
	int	groups[NGROUPS];

	int	valid;		/* was readqueue successful */
	char	*buf;		/* used by readqueue */
	char	*shell;		/* used when creating job */
	char	*jobname;

	char	*fn;		/* filename */
	int	fd;		/* file descripter */

	struct	JobQ_s *next;
} *JobQ;

/* at.c */
extern	JobQ	job;
extern	char	*progname;
int	at_allowed __P(());
char	*queuejob __P((int *argc, char **argv[], char *envp[]));
void	listjobs __P((int *argc, char **argv[]));
void	removeitem __P((JobQ));
void	removejobs __P((int *argc, char **argv[]));

/* readqueue.c */
JobQ	readqueue();
JobQ	freeitem __P((JobQ));		/* free()'s item return item->next */
void	freequeue __P((JobQ));		/* free()'s all memory */
int	bytime __P((const void *, const void *));
void	sortqueue __P((JobQ));		/* sorts queue */
void	printhdr();
void	printitem __P((JobQ));
void	printqueue __P((JobQ));

/* sched.c */
void	scheduler();

/* utils.c */
char	*username __P((uid_t uid));
char	*qfpath __P((JobQ));
char	*xfpath __P((JobQ));
char	*outputpath __P((JobQ));
char	*escstr __P((char *str, char match, char *repl));
char	*shellesc __P((char *str));	/* escstr(s, '\'', "'\\''"); */
void	swrite __P((int fd, char *fmt, ...));
long	getjobid();
