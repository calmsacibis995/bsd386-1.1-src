# include "structs.h"

SaveFile (name, verbose)
    char   *name;
{
    reg FILE *f;

    f = fopen (name, "w");
    if (f == NULL)
    {
	Error ("Can't open file `%s'", name);
	return;
    }

    if (verbose)
	printf ("\t\"%s\"", name);
    fflush (stdout);
    ListLines (f, ZERO, INFINITY);
    fclose (f);
    ClearChanges ();
    if (verbose)
	printf (", %d lines\n", LineCount ());
}

SaveLine (lno, indent, node)
    long    lno;
    int     indent;
    struct
    tree_node *node;
{
    reg struct line_descr *lp, *tmplp;
    extern struct line_descr *FastTab;
    extern unsigned FastCount, FastSize;

    if (FastCount >= FastSize)
    {
	FastSize *= 2;
	FastTab = (struct line_descr *) realloc ((char *) FastTab,
	    (unsigned) (FastSize * sizeof (struct
		    line_descr)));
	FirstLine = FastTab;
	LastLine = FastTab + FastCount - 1;
#ifdef DEBUG
	if (Opts['F'])
	{
	    printf ("allocating %d lines\n", FastSize / 2);
	}
#endif
    }

    lp = FindLine (lno);

    if (lp->l_no == lno)
    {
	FreeTree (lp->l_tree);
	lp->l_indent = indent;
	lp->l_tree = node;
	return;
    }
#ifdef DEBUG
    if (Opts['F'])
	fprintf (stderr, "insert %d before %d\n", lno, lp->l_no);
#endif
    for (tmplp = LastLine; tmplp >= lp; --tmplp)
    {
	*(tmplp + 1) = *tmplp;
#ifdef DEBUG
	if (Opts['F'] > 2)
	    fprintf (stderr, "move %d to %d+1\n", tmplp - FirstLine,
		tmplp - FirstLine + 1);
#endif
    }
    lp->l_no = lno;
    lp->l_indent = indent;
    lp->l_tree = node;
    FastCount++;
    LastLine = FastTab + FastCount - 1;
    MadeChanges ();
#ifdef DEBUG
    if (Opts['F'] > 1)
    {
	DumpLineNos (FastTab, FastCount);
    }
#endif
}


struct line_descr *
FindLine (lno)
    reg long lno;
{
    reg struct line_descr *low, *mid, *high;
    extern struct line_descr *FastTab;
    extern unsigned FastCount, FastSize;

    low = FirstLine;
    high = LastLine;
    while (low < high)
    {
	mid = low + ((high - low) >> 1);
	if (lno > mid->l_no)
	    low = mid + 1;
	else
	    high = mid;
    }
    if (lno <= low->l_no)
    {
#ifdef DEBUG
	if (Opts['F'])
	    printf ("found lin %d\n", lno);
#endif
	return low;
    }
    if (low < LastLine)
    {
#ifdef DEBUG
	if (Opts['F'])
	    fprintf (stderr, "return higher: %d\n", (low + 1)->l_no);
#endif
	return (low + 1);
    }
#ifdef DEBUG
    if (Opts['F'])
	fprintf (stderr, "returning INFINITY %d\n", LastLine->l_no);
#endif
    return LastLine;
}

ListLines (file, start, finish)
    reg FILE *file;
    reg long start, finish;
{
    reg struct line_descr *l;
    int     printed, i;

    if (FirstLine == NULL)
    {
	Error ("Workspace empty.");
	return;
    }

    if (finish > LastLine->l_no)
	finish = LastLine->l_no;
    if (start > LastLine->l_no)
	start = LastLine->l_no;

    printed = 0;
    l = FindLine (start);
    for (; IsValidLine (l) && l->l_no <= finish; l = LineAfter (l))
    {
	fprintf (file, "%5d\t", l->l_no);
	for (i = 1; i <= TabCount (l->l_indent) - 1; ++i)
	    putc ('\t', file);
	for (i = 0; i < SpaceCount (l->l_indent); ++i)
	    putc (' ', file);
	PrintTree (file, l->l_tree);
	putc ('\n', file);
	++printed;
    }
    if (printed == 0)
	Error ("No lines in range.");
}

DeleteLine (lno)
    reg long lno;
{
    reg struct line_descr *lp;
    extern struct line_descr *FastTab;
    extern unsigned FastCount, FastSize;

    if (FastTab == NULL)
	return;

    lp = FindLine (lno);
    if (lp->l_no != lno)
	return;
    for (; lp < LastLine; ++lp)
	*lp = *(lp + 1);
    FastCount--;
    LastLine = FastTab + FastCount - 1;
    MadeChanges ();
#ifdef DEBUG
    if (Opts['F'] > 1)
    {
	DumpLineNos (FastTab, FastCount);
    }
#endif
}



ClearBuffer ()
{
    while (FirstLine < LastLine)
	DeleteLine (FirstLine->l_no);
    ClearChanges ();
}

CanQuit ()
{
    if (ChangesMade)
    {
	printf ("\tYou've changed your program. %s\n",
	    "You should save it first.");
	ClearChanges ();
	return 0;
    }
    return 1;
}

char    tempfile[20];

RunEditor (cmd)
    char   *cmd;
{
    char    oldfile[100], comline[40];
    char    *mktemp ();

    if (tempfile[0] == '\0')
    {
	(void) strcpy (tempfile, "/tmp/BasXXXXXX");
	mktemp (tempfile);
    }
    SaveFile (tempfile, 0);
    (void) sprintf (comline, "%s %s", cmd, tempfile);
    system (comline);
    (void) strcpy (oldfile, CurFile);
    LoadFile (tempfile, 0);
    (void) sprintf (comline, "rm %s", tempfile);
    system (comline);
    MadeChanges ();
    (void) strcpy (CurFile, oldfile);
}

DumpLineNos (tab, count)
    struct line_descr *tab;
    unsigned count;
{
    struct line_descr *l;
    int     i;

    i = -1;
    for (l = tab; l < tab + count; ++l)
    {
	if (++i % 10 == 0)
	    putc ('\n', stderr);
	printf ("%7d", l->l_no);
    }
    putc ('\n', stderr);
}

struct line_descr *FastTab;
unsigned FastCount = 0, FastSize = 128;

AllocFastTab ()
{
    if (FastTab != NULL)
    {
	Error ("Argh!  table being reallocated.\n");
	abort ();
	return;
    }
    FastTab = (struct line_descr *) malloc ((unsigned) (FastSize * sizeof (struct line_descr)));
    FastCount = 1;
    FirstLine = LastLine = FastTab;
    FirstLine->l_no = INFINITY;
    FirstLine->l_tree = NULL;
}
