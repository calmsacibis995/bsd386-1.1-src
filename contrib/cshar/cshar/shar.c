/*
**  SHAR
**  Make a shell archive of a list of files.
*/
#include "shar.h"
#ifdef	RCSID
static char RCS[] =
	"$Header: /bsdi/MASTER/BSDI_OS/contrib/cshar/cshar/shar.c,v 1.1.1.1 1993/02/23 18:00:00 polk Exp $";
#endif	/* RCSID */

/*
**  Minimum allocation of file name pointers used in "-i" option processing.
*/
#define	MIN_FILES	50


/*
**  This prolog is output before the archive.
*/
static char	 *Prolog[] = {
  "! /bin/sh",
  " This is a shell archive.  Remove anything before this line, then unpack",
  " it by saving it into a file and typing \"sh file\".  To overwrite existing",
  " files, type \"sh file -c\".  You can also feed this as standard input via",
  " unshar, or by typing \"sh <file\", e.g..  If this archive is complete, you",
  " will see the following message at the end:",
  NULL
};


/*
**  Package up one file or directory.
*/
static void
shar(file, Basename)
    char		*file;
    int			 Basename;
{
    REGISTER char	*s;
    REGISTER char	*Name;
    REGISTER int	 Bads;
    REGISTER off_t	 Size;
    char		 buf[BUFSIZ];

    /* Just in case. */
    if (EQ(file, ".") || EQ(file, ".."))
	return;

    Size = Fsize(file);
    Name =  Basename && (Name = RDX(file, '/')) ? Name + 1 : file;

    /* Making a directory? */
    if (Ftype(file) == F_DIR) {
	Printf("if test ! -d '%s' ; then\n", Name);
	Printf("    echo shar: Creating directory \\\"'%s'\\\"\n", Name);
	Printf("    mkdir '%s'\n", Name);
	Printf("fi\n");
    }
    else {
	if (freopen(file, "r", stdin) == NULL) {
	    Fprintf(stderr, "Can't open %s, %s\n", file, Ermsg(errno));
	    exit(1);
	}

	/* Emit the per-file prolog. */
	Printf("if test -f '%s' -a \"${1}\" != \"-c\" ; then \n", Name);
	Printf("  echo shar: Will not clobber existing file \\\"'%s'\\\"\n",
	       Name);
	Printf("else\n");
	Printf("echo shar: Extracting \\\"'%s'\\\" \\(%ld character%s\\)\n",
	       Name, (long)Size, Size == 1 ? "" : "s");
	Printf("sed \"s/^X//\" >'%s' <<'END_OF_FILE'\n", Name, Name);

	/* Output the file contents. */
	for (Bads = 0; fgets(buf, BUFSIZ, stdin); )
	    if (buf[0]) {
#ifdef	TOO_FANCY
		if (buf[0] == 'X' || buf[0] == 'E' || buf[0] == 'F'
		 || !isascii(buf[0]) || !isalpha(buf[0]))
		    /* Protect non-alpha's, the shar pattern character, the
		     * END_OF_FILE message, and mail "From" lines. */
		    (void)putchar('X');
#else
		/* Always put out a leading X. */
		(void)putchar('X');
#endif	/* TOO_FANCY */
		for (s = buf; *s; s++) {
		    if (BADCHAR(*s))
			Bads++;
		    (void)putchar(*s);
		}
	    }

	/* Tell about and control characters. */
	Printf("END_OF_FILE\n", Name);
	if (Bads) {
	    Printf(
    "echo shar: %d control character%s may be missing from \\\"'%s'\\\"\n",
		   Bads, Bads == 1 ? "" : "s", Name);
	    Fprintf(stderr, "Found %d control char%s in \"'%s'\"\n",
		    Bads, Bads == 1 ? "" : "s", Name);
	}

	/* Output size check. */
	Printf("if test %ld -ne `wc -c <'%s'`; then\n", (long)Size, Name);
	Printf("    echo shar: \\\"'%s'\\\" unpacked with wrong size!\n", Name);
	Printf("fi\n");

	/* Executable? */
	if (Fexecute(file))
	    Printf("chmod +x '%s'\n", Name);

	Printf("# end of '%s'\nfi\n", Name);
    }
}


/*
**  Read list of files from file.
*/
static char **
GetFiles(Name)
    char		 *Name;
{
    REGISTER FILE	 *F;
    REGISTER int	  i;
    REGISTER int	  count;
    REGISTER char	**files;
    REGISTER char	**temp;
    REGISTER int	  j;
    char		  buff[BUFSIZ];
    char		 *p;

    /* Open the file. */
    if (EQ(Name, "-"))
	F = stdin;
    else if ((F = fopen(Name, "r")) == NULL) {
	Fprintf(stderr, "Can't open '%s' for input.\n", Name);
	return(NULL);
    }

    /* Get space. */
    count = MIN_FILES;
    files = NEW(char*, count);

    /* Read lines. */
    for (i = 0; fgets(buff, sizeof buff, F); ) {
	if (p = IDX(buff, '\n'))
	    *p = '\0';
	files[i] = COPY(buff);
	if (++i == count - 2) {
	    /* Get more space; some systems don't have realloc()... */
	    temp = NEW(char*, count);
	    for (count += MIN_FILES, j = 0; j < i; j++)
		temp[j] = files[j];
	    files = temp;
	}
    }

    /* Clean up, close up, return. */
    files[i] = NULL;
    (void)fclose(F);
    return(files);
}


main(ac, av)
    int			 ac;
    REGISTER char	*av[];
{
    REGISTER char	*Trailer;
    REGISTER char	*p;
    REGISTER char	*q;
    REGISTER int	 i;
    REGISTER int	 length;
    REGISTER int	 Oops;
    REGISTER int	 Knum;
    REGISTER int	 Kmax;
    REGISTER int	 Basename;
    REGISTER int	 j;
    time_t		 clock;
    char		**Flist;

    /* Parse JCL. */
    Basename = 0;
    Knum = 0;
    Kmax = 0;
    Trailer = NULL;
    Flist = NULL;
    for (Oops = 0; (i = getopt(ac, av, "be:i:n:o:t:")) != EOF; )
	switch (i) {
	default:
	    Oops++;
	    break;
	case 'b':
	    Basename++;
	    break;
	case 'e':
	    Kmax = atoi(optarg);
	    break;
	case 'i':
	    Flist = GetFiles(optarg);
	    break;
	case 'n':
	    Knum = atoi(optarg);
	    break;
	case 'o':
	    if (freopen(optarg, "w", stdout) == NULL) {
		Fprintf(stderr, "Can't open %s for output, %s.\n",
			optarg, Ermsg(errno));
		Oops++;
	    }
	    break;
	case 't':
	    Trailer = optarg;
	    break;
	}

    /* Rest of arguments are files. */
    if  (Flist == NULL) {
	av += optind;
	if (*av == NULL) {
	    Fprintf(stderr, "No input files\n");
	    Oops++;
	}
	Flist = av;
    }

    if (Oops) {
	Fprintf(stderr,
		"USAGE: shar [-b] [-o:] [-i:] [-n:e:t:] file... >SHAR\n");
	exit(1);
    }

    /* Everything readable and reasonably-named? */
    for (Oops = 0, i = 0; p = Flist[i]; i++)
	if (freopen(p, "r", stdin) == NULL) {
	    Fprintf(stderr, "Can't read %s, %s.\n", p, Ermsg(errno));
	    Oops++;
	}
	else
	    for (; *p; p++)
		if (!isascii(*p)) {
		    Fprintf(stderr, "Bad character '%c' in '%s'.\n",
			    *p, Flist[i]);
		    Oops++;
		}
    if (Oops)
	exit(2);

    /* Prolog. */
    for (i = 0; p = Prolog[i]; i++)
	Printf("#%s\n", p);
    if (Knum && Kmax)
	Printf("#\t\t\"End of archive %d (of %d).\"\n", Knum, Kmax);
    else
        Printf("#\t\t\"End of shell archive.\"\n");
    Printf("# Contents: ");
    for (length = 12, i = 0; p = Flist[i++]; length += j) {
	if (Basename && (q = RDX(p, '/')))
	    p = q + 1;
	j = strlen(p) + 1;
	if (length + j < WIDTH)
	    Printf(" %s", p);
	else {
	    Printf("\n#   %s", p);
	    length = 4;
	}
    }
    Printf("\n");
    clock = time((time_t *)NULL);
    Printf("# Wrapped by %s@%s on %s", User(), Host(), ctime(&clock));
    Printf("PATH=/bin:/usr/bin:/usr/ucb ; export PATH\n");

    /* Do it. */
    while (*Flist)
	shar(*Flist++, Basename);

    /* Epilog. */
    if (Knum && Kmax) {
	Printf("echo shar: End of archive %d \\(of %d\\).\n", Knum, Kmax);
	Printf("cp /dev/null ark%disdone\n", Knum);
	Printf("MISSING=\"\"\n");
	Printf("for I in");
	for (i = 0; i < Kmax; i++)
	    Printf(" %d", i + 1);
	Printf(" ; do\n");
	Printf("    if test ! -f ark${I}isdone ; then\n");
	Printf("\tMISSING=\"${MISSING} ${I}\"\n");
	Printf("    fi\n");
	Printf("done\n");
	Printf("if test \"${MISSING}\" = \"\" ; then\n");
	if (Kmax == 1)
	    Printf("    echo You have the archive.\n");
	else if (Kmax == 2)
	    Printf("    echo You have unpacked both archives.\n");
	else
	    Printf("    echo You have unpacked all %d archives.\n", Kmax);
	if (Trailer && *Trailer)
	    Printf("    echo \"%s\"\n", Trailer);
	Printf("    rm -f ark[1-9]isdone%s\n",
	       Kmax >= 9 ? " ark[1-9][0-9]isdone" : "");
	Printf("else\n");
	Printf("    echo You still need to unpack the following archives:\n");
	Printf("    echo \"        \" ${MISSING}\n");
	Printf("fi\n");
	Printf("##  End of shell archive.\n");
    }
    else {
        Printf("echo shar: End of shell archive.\n");
	if (Trailer && *Trailer)
	    Printf("echo \"%s\"\n", Trailer);
    }

    Printf("exit 0\n");

    exit(0);
}
