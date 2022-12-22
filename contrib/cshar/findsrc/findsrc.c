/*
**  FINDSRC
**  Walk directories, trying to find source files.
*/
#include "shar.h"
#ifdef	RCSID
static char RCS[] =
	"$Header: /bsdi/MASTER/BSDI_OS/contrib/cshar/findsrc/findsrc.c,v 1.1.1.1 1993/02/23 18:00:03 polk Exp $";
#endif	/* RCSID */


/*
**  Global variables.
*/
int	 DoDOTFILES;			/* Do .newsrc and friends?	*/
int	 DoRCS;				/* Do RCS and SCCS files?	*/
int	 Default;			/* Default answer from user	*/
int	 Verbose;			/* List rejected files, too?	*/
char	*Dname;				/* Filename of directory list	*/
char	*Fname;				/* Filename of file list	*/
FILE	*Dfile;				/* List of directories found	*/
FILE	*Ffile;				/* List of files found		*/
FILE	*DEVTTY;			/* The tty, if in filter mode	*/


/*
**  Signal handler.  Clean up and die.
*/
static sigret_t
Catch(s)
    int		 s;
{
    int		 e;

    e = errno;
    if (Dname)
	(void)unlink(Dname);
    if (Fname)
    (void)unlink(Fname);
    Fprintf(stderr, "Got signal %d, %s.\n", s, Ermsg(e));
    exit(1);
}


/*
**  Given a filename, apply heuristics to see if we want it.
*/
static int
Wanted(Name)
    REGISTER char	*Name;
{
    REGISTER FILE	*F;
    REGISTER char	*s;
    REGISTER char	*p;
    REGISTER char	*d;
    char		 buff[BUFSIZ];

    /* Get down to brass tacks. */
    s = (p = RDX(Name, '/')) ? p + 1 : Name;

    /* Only do directories other than . and .. and regular files. */
    if ((Ftype(Name) != F_DIR && Ftype(Name) != F_FILE)
     || EQ(s, ".") || EQ(s, ".."))
	return(FALSE);

    /* Common cruft we never want. */
    if (EQ(s, "foo") || EQ(s, "core") || EQ(s, "tags") || EQ(s, "lint"))
	return(FALSE);

    /* Disregard stuff with bogus suffixes. */
    d = RDX(s, '.');
    if ((p = d)
     && (EQ(++p, "out") || EQ(p, "orig") || EQ(p, "rej") || EQ(p, "BAK")
      || EQ(p, "CKP") || EQ(p, "old") || EQ(p, "o") || EQ(p, "EXE")
      || EQ(p, "OBJ")))
	return(FALSE);

    /* Want .cshrc, .newsrc, etc.? */
    if (*s == '.' && isascii(s[1]) && isalpha(s[1]))
	return(DoDOTFILES);

    /* RCS or SCCS file or directory? */
    if (EQ(s, "RCS")
     || ((p = RDX(s, ',')) && *++p == 'v' && *++p == '\0')
     || EQ(s, "SCCS") || (s[0] == 's' && s[1] == '.'))
	return(DoRCS);

    /* Mlisp, m4 (yes to .m? but no to .mo)? */
    /* really, no to .mo but yes to names matching the regexp ".m?" */
    if ((p = d) && *++p == 'm' && p[2] == '\0')
	return(*++p != 'o');

    /* Gnu Emacs elisp (yes to .el*, but no to .elc)? */
    if ((p = d) && *++p == 'e' && *++p == 'l')
	return(*++p != 'c' || *++p);

    /* C source (*.[ch])? */
    if ((p = d) && (*++p == 'c' || *p == 'h') && *++p == '\0')
	return(TRUE);

    /* Manpage (*.[1234567890] or *.man)? */
    if ((p = d) && isascii(s[1]) && isdigit(*p))
	return(TRUE);
    if ((p = d) && *++p == 'm' && *++p == 'a' && *++p == 'n')
	return(TRUE);

    /* Make control file? */
    if ((*s == 'M' || *s == 'm') && EQ(s + 1, "akefile"))
	return(TRUE);

    /* Convert to lowercase, and see if it's a README or MANIFEST. */
    for (p = strcpy(buff, s); *p; p++)
	if (isascii(*p) && isupper(*p))
	    *p = tolower(*p);
    if (EQ(buff, "readme") || EQ(buff, "read_me") || EQ(buff, "read-me")
     || EQ(buff, "manifest"))
	return(TRUE);

    /* Larry Wall-style template file (*.SH)? */
    if ((p = d) && *++p == 'S' && *++p == 'H')
	return(TRUE);

    /* If we have a default, give it back. */
    if (Default)
	return(Default == 'y');

#ifdef	CAN_POPEN
    /* See what file(1) has to say; if it says executable, punt. */
    (void)sprintf(buff, "exec file '%s'", Name);
    if (F = popen(buff, "r")) {
	(void)fgets(buff, sizeof buff, F);
	(void)pclose(F);
	for (p = buff; p = IDX(p, 'e'); p++)
	    if (PREFIX(p, "executable"))
		return(FALSE);
	(void)fputs(buff, stderr);
    }
#endif	/* CAN_POPEN */

    /* Add it? */
    while (TRUE) {
	if (DEVTTY == NULL)
	    DEVTTY = fopen(THE_TTY, "r");
	Fprintf(stderr, "Add this one (y or n)[y]?  ");
	(void)fflush(stderr);
	if (fgets(buff, sizeof buff, DEVTTY) == NULL
	 || buff[0] == '\n' || buff[0] == 'y' || buff[0] == 'Y')
	    break;
	if (buff[0] == 'n' || buff[0] == 'N')
	    return(FALSE);
	if (buff[0] == '!' )
	    (void)system(&buff[1]);
	Fprintf(stderr, "--------------------\n%s:  ", Name);
	clearerr(DEVTTY);
    }
    return(TRUE);
}


/*
**  Quick and dirty recursive routine to walk down directory tree.
**  Could be made more general, but why bother?
*/
static void
Process(p, level)
    REGISTER char	 *p;
    REGISTER int	  level;
{
    REGISTER char	 *q;
    DIR			 *Dp;
    DIRENTRY		 *E;
    char		  buff[BUFSIZ];

#ifdef	MSDOS
    strlwr(p);
#endif	/* MSDOS */

    if (!GetStat(p))
	Fprintf(stderr, "Can't walk down %s, %s.\n", p, Ermsg(errno));
    else {
	/* Skip leading ./ which find(1), e.g., likes to put out. */
	if (p[0] == '.' && p[1] == '/')
	    p += 2;

	if (Wanted(p))
	    Fprintf(Ftype(p) == F_FILE ? Ffile : Dfile, "%s\n", p);
	else if (Verbose)
	    Fprintf(Ftype(p) == F_FILE ? Ffile : Dfile, "PUNTED %s\n", p);

	if (Ftype(p) == F_DIR)
	    if (++level == MAX_LEVELS)
		Fprintf(stderr, "Won't walk down %s -- more than %d levels.\n",
			p, level);
	    else if (Dp = opendir(p)) {
		q = buff + strlen(strcpy(buff, p));
		for (*q++ = '/'; E = readdir(Dp); )
		    if (!EQ(E->d_name, ".") && !EQ(E->d_name, "..")) {
			(void)strcpy(q, E->d_name);
			Process(buff, level);
		    }
		(void)closedir(Dp);
	    }
	    else
		Fprintf(stderr, "Can't open directory %s, %s.\n",
			p, Ermsg(errno));
    }
}


main(ac, av)
    REGISTER int	 ac;
    REGISTER char	*av[];
{
    REGISTER char	*p;
    REGISTER int	 i;
    REGISTER int	 Oops;
    char		 buff[BUFSIZ];

    /* Parse JCL. */
    for (Oops = 0; (i = getopt(ac, av, ".d:o:RSv")) != EOF; )
	switch (i) {
	default:
	    Oops++;
	    break;
	case '.':
	    DoDOTFILES++;
	    break;
	case 'd':
	    switch (optarg[0]) {
	    default:
		Oops++;
		break;
	    case 'y':
	    case 'Y':
		Default = 'y';
		break;
	    case 'n':
	    case 'N':
		Default = 'n';
		break;
	    }
	    break;
	case 'o':
	    if (freopen(optarg, "w", stdout) == NULL) {
		Fprintf(stderr, "Can't open %s for output, %s.\n",
			optarg, Ermsg(errno));
		exit(1);
	    }
	case 'R':
	case 'S':
	    DoRCS++;
	    break;
	case 'v':
	    Verbose++;
	    break;
	}
    if (Oops) {
	Fprintf(stderr, "Usage: findsrc [-d{yn}] [-.] [-{RS}] [-v] files...\n");
	exit(1);
    }
    av += optind;

    /* Set signal catcher, open temp files. */
    SetSigs(TRUE, Catch);
    Dname = mktemp("/tmp/findDXXXXXX");
    Fname = mktemp("/tmp/findFXXXXXX");
    Dfile = fopen(Dname, "w");
    Ffile = fopen(Fname, "w");

    /* Read list of files, determine their status. */
    if (*av)
	for (DEVTTY = stdin; *av; av++)
	    Process(*av, 0);
    else
	while (fgets(buff, sizeof buff, stdin)) {
	    if (p = IDX(buff, '\n'))
		*p = '\0';
	    else
		Fprintf(stderr, "Warning, line too long:\n\t%s\n", buff);
	    Process(buff, 0);
	}

    /* First print directories. */
    if (freopen(Dname, "r", Dfile)) {
	while (fgets(buff, sizeof buff, Dfile))
	    (void)fputs(buff, stdout);
	(void)fclose(Dfile);
    }

    /* Now print regular files. */
    if (freopen(Fname, "r", Ffile)) {
	while (fgets(buff, sizeof buff, Ffile))
	    (void)fputs(buff, stdout);
	(void)fclose(Ffile);
    }

    /* That's all she wrote. */
    (void)unlink(Dname);
    (void)unlink(Fname);
    exit(0);
}
