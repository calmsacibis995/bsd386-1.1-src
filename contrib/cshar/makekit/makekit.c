/*
**  MAKEKIT
**  Split up source files into reasonably-sized shar lists.
*/
#include "shar.h"
#ifdef	RCSID
static char RCS[] =
	"$Header: /bsdi/MASTER/BSDI_OS/contrib/cshar/makekit/makekit.c,v 1.1.1.1 1993/02/23 18:00:03 polk Exp $";
#endif	/* RCSID */


/*
**  Our block of information about the files we're doing.
*/
typedef struct _block {
    char	*Name;			/* Filename			*/
    char	*Text;			/* What it is			*/
    int		 Where;			/* Where it is			*/
    int		 Type;			/* Directory or file?		*/
    long	 Bsize;			/* Size in bytes		*/
} BLOCK;


/*
**  Our block of information about the archives we're making.
*/
typedef struct _archive {
    int		 Count;			/* Number of files		*/
    long	 Asize;			/* Bytes used by archive	*/
} ARCHIVE;


/*
**  Format strings; these are strict K&R so you shouldn't have to change them.
*/
#define FORMAT1		" %-25s %2d\t%s\n"
#define FORMAT2		"%s%2.2d"
#ifdef	FMT02d
#undef FORMAT2
#define FORMAT2		"%s%02.2d"	/* I spoke too soon...		*/
#endif	/* FMT02d */


/*
**  Global variables.
*/
char	*InName;			/* File with list to pack	*/
char	*OutName;			/* Where our output goes	*/
char	*SharName = "Part";		/* Prefix for name of each shar	*/
char	*Trailer;			/* Text for shar to pack in	*/
char	*TEMP;				/* Temporary manifest file	*/
#ifdef	MSDOS
char	*FLST;				/* File with list for shar	*/
#endif	/* MSDOS */
int	 ArkCount = 20;			/* Max number of archives	*/
int	 ExcludeIt;			/* Leave out the output file?	*/
int	 Header;			/* Lines of prolog in input	*/
int	 Preserve;			/* Preserve order for Manifest?	*/
int	 Working = TRUE;		/* Call shar when done?		*/
long	 Size = 55000;			/* Largest legal archive size	*/


/*
**  Sorting predicate to put README or MANIFEST first, then directories,
**  then larger files, then smaller files, which is how we want to pack
**  stuff in archives.
*/
static int
SizeP(t1, t2)
    BLOCK	*t1;
    BLOCK	*t2;
{
    long	 i;

    if (t1->Type == F_DIR)
	return(t2->Type == F_DIR ? 0 : -1);
    if (t2->Type == F_DIR)
	return(1);
    if (EQn(t1->Name, "README", 6) || (OutName && EQ(t1->Name, OutName)))
	return(-1);
    if (EQn(t2->Name, "README", 6) || (OutName && EQ(t2->Name, OutName)))
	return(1);
    return((i = t1->Bsize - t2->Bsize) == 0L ? 0 : (i < 0L ? -1 : 1));
}


/*
**  Sorting predicate to get things in alphabetical order, which is how
**  we write the Manifest file.
*/
static int
NameP(t1, t2)
    BLOCK	*t1;
    BLOCK	*t2;
{
    int		 i;

    return((i = *t1->Name - *t2->Name) ? i : strcmp(t1->Name, t2->Name));
}


/*
**  Skip whitespace.
*/
static char *
Skip(p)
    REGISTER char	*p;
{
    while (*p && WHITE(*p))
	p++;
    return(p);
}


/*
**  Signal handler.  Clean up and die.
*/
static sigret_t
Catch(s)
    int		 s;
{
    int		 e;

    e = errno;
    if (TEMP)
	(void)unlink(TEMP);
#ifdef	MSDOS
    if (FLST)
	(void)unlink(FLST);
#endif	/* MSDOS */
    Fprintf(stderr, "Got signal %d, %s.\n", s, Ermsg(e));
    exit(1);
}


main(ac, av)
    REGISTER int	 ac;
    char		*av[];
{
    REGISTER FILE	*F;
    REGISTER FILE	*In;
    REGISTER BLOCK	*t;
    REGISTER ARCHIVE	*k;
    REGISTER char	*p;
    REGISTER int	 i;
    REGISTER int	 lines;
    REGISTER int	 Value;
    BLOCK		*Table;
    BLOCK		*TabEnd;
    ARCHIVE		*Ark;
    ARCHIVE		*ArkEnd;
    char		 buff[BUFSIZ];
    long		 lsize;
    int			 LastOne;
    int			 Start;
    int			 Notkits;
    char		 EndArkNum[20];
    char		 CurArkNum[20];

    /* Collect input. */
    Value = FALSE;
    Notkits = FALSE;
    while ((i = getopt(ac, av, "1eh:i:k:n:mo:ps:t:x")) != EOF)
	switch (i) {
	default:
	    Fprintf(stderr, "usage: makekit %s\n        %s\n",
		    "[-1] [-e] [-x] [-k #] [-s #[k]] [-n Name] [-t Text]",
		    "[-p] [-m | -i MANIFEST -o MANIFEST -h 2] [file ...]");
	    exit(1);
	case '1':
	    Notkits = TRUE;
	    break;
	case 'e':
	    ExcludeIt = TRUE;
	    break;
	case 'h':
	    Header = atoi(optarg);
	    break;
	case 'i':
	    InName = optarg;
	    break;
	case 'k':
	    ArkCount = atoi(optarg);
	    break;
	case 'm':
	    InName = OutName = "MANIFEST";
	    Header = 2;
	    break;
	case 'n':
	    SharName = optarg;
	    break;
	case 'o':
	    OutName = optarg;
	    break;
	case 'p':
	    Preserve = TRUE;
	    break;
	case 's':
	    Size = atoi(optarg);
	    if (IDX(optarg, 'k') || IDX(optarg, 'K'))
		Size *= 1024;
	    break;
	case 't':
	    Trailer = optarg;
	    break;
	case 'x':
	    Working = FALSE;
	    break;
	}
    ac -= optind;
    av += optind;

    /* Write the file list to a temp file. */
    TEMP = mktemp("/tmp/maniXXXXXX");
    F = fopen(TEMP, "w");
    SetSigs(TRUE, Catch);
    if (av[0])
	/* Got the arguments on the command line. */
	while (*av)
	    Fprintf(F, "%s\n", *av++);
    else {
	/* Got the name of the file from the command line. */
	if (InName == NULL)
	    In = stdin;
	else if ((In = fopen(InName, "r")) == NULL) {
	    Fprintf(stderr, "Can't read %s as manifest, %s.\n",
		    InName, Ermsg(errno));
	    exit(1);
	}
	/* Skip any possible prolog, then output rest of file. */
	while (--Header >= 0 && fgets(buff, sizeof buff, In))
	    ;
	if (feof(In)) {
	    Fprintf(stderr, "Nothing but header lines in list!?\n");
	    exit(1);
	}
	while (fgets(buff, sizeof buff, In))
	    fputs(buff, F);
	if (In != stdin)
	    (void)fclose(In);
    }
    (void)fclose(F);

    /* Count number of files, allow for NULL and our output file. */
    F = fopen(TEMP, "r");
    for (lines = 2; fgets(buff, sizeof buff, F); lines++)
	;
    rewind(F);

    /* Read lines and parse lines, see if we found our OutFile. */
    Table = NEW(BLOCK, lines);
    for (t = Table, Value = FALSE, lines = 0; fgets(buff, sizeof buff, F); ) {
	/* Read line, skip first word, check for blank line. */
	if (p = IDX(buff, '\n'))
	    *p = '\0';
	else
	    Fprintf(stderr, "Warning, line truncated:\n%s\n", buff);
	p = Skip(buff);
	if (*p == '\0')
	    continue;

	/* Copy the line, snip off the first word. */
	for (p = t->Name = COPY(p); *p && !WHITE(*p); p++)
	    ;
	if (*p)
	    *p++ = '\0';

	/* Skip <spaces><digits><spaces>; remainder is the file description. */
	for (p = Skip(p); *p && isascii(*p) && isdigit(*p); )
	    p++;
	t->Text = Skip(p);

	/* Get file type. */
	if (!GetStat(t->Name)) {
	    Fprintf(stderr, "Can't stat %s (%s), skipping.\n",
		    t->Name, Ermsg(errno));
	    continue;
	}
	t->Type = Ftype(t->Name);

	/* Guesstimate its size when archived:  prolog, plus one char/line. */
	t->Bsize = strlen(t->Name) * 3 + 200;
	if (t->Type == F_FILE) {
	    lsize = Fsize(t->Name);
	    t->Bsize += lsize + lsize / 60;
	}
	if (t->Bsize > Size) {
	    Fprintf(stderr, "At %ld bytes, %s is too big for any archive!\n",
		    t->Bsize, t->Name);
	    exit(1);
	}

	/* Is our ouput file there? */
	if (!Value && OutName && EQ(OutName, t->Name))
	    Value = TRUE;

	/* All done -- advance to next entry. */
	t++;
    }
    (void)fclose(F);
    (void)unlink(TEMP);
    SetSigs(S_RESET, (sigret_t (*)())NULL);

    /* Add our output file? */
    if (!ExcludeIt && !Value && OutName) {
	t->Name = OutName;
	t->Text = "This shipping list";
	t->Type = F_FILE;
	t->Bsize = lines * 60;
	t++;
    }

    /* Sort by size, get archive space. */
    lines = t - Table;
    TabEnd = &Table[lines];
    if (!Preserve)
	qsort((char *)Table, lines, sizeof Table[0], SizeP);
    Ark = NEW(ARCHIVE, ArkCount);
    ArkEnd = &Ark[ArkCount];

    /* Loop through the pieces, and put everyone into an archive. */
    for (t = Table; t < TabEnd; t++) {
	for (k = Ark; k < ArkEnd; k++)
	    if (t->Bsize + k->Asize < Size) {
		k->Asize += t->Bsize;
		t->Where = k - Ark;
		k->Count++;
		break;
	    }
	if (k == ArkEnd) {
	    Fprintf(stderr, "'%s' doesn't fit -- need more then %d archives.\n",
		    t->Name, ArkCount);
	    exit(1);
	}
	/* Since our share doesn't build sub-directories... */
	if (t->Type == F_DIR && k != Ark)
	    Fprintf(stderr, "Warning:  directory '%s' is in archive %d\n",
		    t->Name, k - Ark + 1);
    }

    /* Open the output file. */
    if (OutName == NULL)
	F = stdout;
    else {
	if (GetStat(OutName)) {
#ifdef	BACKUP_PREFIX
	    (void)sprintf(buff, "%s%s", BACKUP_PREFIX, OutName);
#else
	    /* Handle /foo/bar/VeryLongFileName.BAK for non-BSD sites. */
	    (void)sprintf(buff, "%s.BAK", OutName);
	    p = (p = RDX(buff, '/')) ? p + 1 : buff;
	    if (strlen(p) > 14)
		/* ... well, sort of handle it. */
		(void)strcpy(&p[10], ".BAK");
#endif	/* BACKUP_PREFIX */
	    Fprintf(stderr, "Renaming %s to %s\n", OutName, buff);
	    (void)rename(OutName, buff);
	}
	if ((F = fopen(OutName, "w")) == NULL) {
	    Fprintf(stderr, "Can't open '%s' for output, %s.\n",
		    OutName, Ermsg(errno));
	    exit(1);
	}
    }

    /* Sort the shipping list, then write it. */
    if (!Preserve)
	qsort((char *)Table, lines, sizeof Table[0], NameP);
    Fprintf(F, "   File Name\t\tArchive #\tDescription\n");
    Fprintf(F, "-----------------------------------------------------------\n");
    for (t = Table; t < TabEnd; t++)
	Fprintf(F, FORMAT1, t->Name, t->Where + 1, t->Text);

    /* Close output.  Are we done? */
    if (F != stdout)
	(void)fclose(F);
    if (!Working)
	exit(0);

    /* Find last archive number. */
    for (i = 0, t = Table; t < TabEnd; t++)
	if (i < t->Where)
	    i = t->Where;
    LastOne = i + 1;

    /* Find archive with most files in it. */
    for (i = 0, k = Ark; k < ArkEnd; k++)
	if (i < k->Count)
	    i = k->Count;

    /* Build the fixed part of the argument vector. */
    av = NEW(char*, i + 10);
    av[0] = "cshar";
    i = 1;
    if (Trailer) {
	av[i++] = "-t";
	av[i++] = Trailer;
    }
    if (Notkits == FALSE) {
	(void)sprintf(EndArkNum, "%d", LastOne);
	av[i++] = "-e";
	av[i++] = EndArkNum;
	av[i++] = "-n";
	av[i++] = CurArkNum;
    }
#ifdef	MSDOS
    av[i++] = "-i";
    av[i++] = FLST = mktemp("/tmp/manlXXXXXX");
#endif	/* MSDOS */

    av[i++] = "-o";
    av[i++] = buff;

    /* Call shar to package up each archive. */
    for (Start = i, i = 0; i < LastOne; i++) {
	(void)sprintf(CurArkNum, "%d", i + 1);
	(void)sprintf(buff, FORMAT2, SharName, i + 1);
#ifndef	MSDOS
	for (lines = Start, t = Table; t < TabEnd; t++)
	    if (t->Where == i)
		av[lines++] = t->Name;
	av[lines] = NULL;
#else
	if ((F = fopen(FLST, "w")) == NULL) {
	    Fprintf(stderr, "Can't open list file '%s' for output, %s.\n",
		    FLST, Ermsg(errno));
	    exit(1);
	}
	for (t = Table; t < TabEnd; t++)
	    if (t->Where == i)
		Fprintf(F, "%s\n", t->Name);
	(void)fclose(F);
#endif /* MSDOS */
	Fprintf(stderr, "Packing kit %d...\n", i + 1);
	if (lines = Execute(av))
	    Fprintf(stderr, "Warning:  shar returned status %d.\n", lines);
    }

#ifdef	MSDOS
    (void)unlink(FLST);
#endif	/* MSDOS */
    /* That's all she wrote. */
    exit(0);
}
