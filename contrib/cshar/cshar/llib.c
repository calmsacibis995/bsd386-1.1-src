/*
**  Some systems will need these routines because they're missing from
**  their C library.  I put Ermsg() here so that everyone will need
**  something from this file...
*/
/* LINTLIBRARY */
#include "shar.h"
#ifdef	RCSID
static char RCS[] =
	"$Header: /bsdi/MASTER/BSDI_OS/contrib/cshar/cshar/llib.c,v 1.1.1.1 1993/02/23 18:00:01 polk Exp $";
#endif	/* RCSID */


/*
**  Return the text string that corresponds to errno.
*/
char *
Ermsg(e)
    int			 e;
{
#ifdef	USE_SYSERRLIST
    extern int		 sys_nerr;
    extern char		*sys_errlist[];
#endif	/* USE_SYSERRLIST */
    static char		 buff[30];

#ifdef	USE_SYSERRLIST
    if (e > 0 && e < sys_nerr)
	return(sys_errlist[e]);
#endif	/* USE_SYSERRLIST */
    (void)sprintf(buff, "Error code %d", e);
    return(buff);
}


#ifdef	NEED_RENAME
/*
**  Give the file "from" the new name "to", removing an old "to"
**  if it exists.
*/
int
rename(from, to)
    char	*from;
    char	*to;
{
    (void)unlink(from);
    (void)link(to, from);
    return(unlink(to));
}
#endif	/* NEED_RENAME */


#ifdef	NEED_MKDIR
/*
**  Quick and dirty mkdir routine for them that's need it.
*/
int
mkdir(name, mode)
    char	*name;
    int		 mode;
{
    char	*av[3];
    int		 i;
    int		 U;

    av[0] = "mkdir";
    av[1] = name;
    av[2] = NULL;
    U = umask(~mode);
    i = Execute(av);
    (void)umask(U);
    return(i ? -1 : 0);
}
#endif	/* NEED_MKDIR */


#ifdef	NEED_QSORT
/*
**  Bubble sort an array of arbitrarily-sized elements.  This routine
**  can be used as an (inefficient) replacement for the Unix qsort
**  routine, hence the name.  If I were to put this into my C library,
**  I'd do two things:
**	-Make it be a quicksort;
**	-Have a front routine which called specialized routines for
**	 cases where Width equals sizeof(int), sizeof(char *), etc.
*/
qsort(Table, Number, Width, Compare)
    REGISTER char	 *Table;
    REGISTER int	  Number;
    REGISTER int	  Width;
    REGISTER int	(*Compare)();
{
    REGISTER char	 *i;
    REGISTER char	 *j;

    for (i = &Table[Number * Width]; (i -= Width) >= &Table[Width]; )
	for (j = i; (j -= Width) >= &Table[0]; )
	    if ((*Compare)(i, j) < 0) {
		REGISTER char	*p;
		REGISTER char	*q;
		REGISTER int	 t;
		REGISTER int	 w;

		/* Swap elements pointed to by i and j. */
		for (w = Width, p = i, q = j; --w >= 0; *p++ = *q, *q++ = t)
		    t = *p;
	    }
}
#endif	/* NEED_QSORT */


#ifdef	NEED_GETOPT

#define TYPE	int

#define ERR(s, c)	if(opterr){\
	char errbuf[2];\
	errbuf[0] = c; errbuf[1] = '\n';\
	(void) write(2, argv[0], (TYPE)strlen(argv[0]));\
	(void) write(2, s, (TYPE)strlen(s));\
	(void) write(2, errbuf, 2);}

extern int strcmp();

int	opterr = 1;
int	optind = 1;
int	optopt;
char	*optarg;

/*
**  Return options and their values from the command line.
**  This comes from the AT&T public-domain getopt published in mod.sources.
*/
int
getopt(argc, argv, opts)
int	argc;
char	**argv, *opts;
{
	static int sp = 1;
	REGISTER int c;
	REGISTER char *cp;

	if(sp == 1)
		if(optind >= argc ||
		   argv[optind][0] != '-' || argv[optind][1] == '\0')
			return(EOF);
		else if(strcmp(argv[optind], "--") == NULL) {
			optind++;

		}
	optopt = c = argv[optind][sp];
	if(c == ':' || (cp=IDX(opts, c)) == NULL) {
		ERR(": illegal option -- ", c);
		if(argv[optind][++sp] == '\0') {
			optind++;
			sp = 1;
		}
		return('?');
	}
	if(*++cp == ':') {
		if(argv[optind][sp+1] != '\0')
			optarg = &argv[optind++][sp+1];
		else if(++optind >= argc) {
			ERR(": option requires an argument -- ", c);
			sp = 1;
			return('?');
		} else
			optarg = argv[optind++];
		sp = 1;
	} else {
		if(argv[optind][++sp] == '\0') {
			sp = 1;
			optind++;
		}
		optarg = NULL;
	}
	return(c);
}

#endif	/* NEED_GETOPT */
