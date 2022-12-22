 /*	SC	A Spreadsheet Calculator
 *		Main driver
 *
 *		original by James Gosling, September 1982
 *		modifications by Mark Weiser and Bruce Israel,
 *			University of Maryland
 *
 *              More mods Robert Bond, 12/86
 *		More mods by Alan Silverstein, 3-4/88, see list of changes.
 *		Currently supported by sequent!sawmill!buhrt (Jeff Buhrt)
 *		$Revision: 1.1.1.1 $
 *
 */

#include <sys/types.h>
#include <signal.h>
#include <curses.h>
#include <ctype.h>

#ifdef BSD42
#include <strings.h>
#else
#ifndef SYSIII
#include <string.h>
#endif
#endif

#include <stdio.h>
#include "sc.h"

extern	char	*getenv();
extern	void	startdisp(), stopdisp();

#ifdef SYSV3
void exit();
#endif

#ifndef SAVENAME
#define	SAVENAME "SC.SAVE" /* file name to use for emergency saves */
#endif /* SAVENAME */

#ifndef DFLT_PAGER
#define	DFLT_PAGER "more"	/* more is probably more widespread than less */
#endif /* DFLT_PAGER */

#define MAXCMD 160	/* for ! command below */

/* Globals defined in sc.h */

struct ent ***tbl;
int strow = 0, stcol = 0;
int currow = 0, curcol = 0;
int savedrow, savedcol;
int FullUpdate = 0;
int ClearScreen = 0;	/* don't try to be smart */
int maxrow, maxcol;
int maxrows, maxcols;
int *fwidth;
int *precision;
int *realfmt;
char *col_hidden;
char *row_hidden;
char line[FBUFLEN];
int changed;
struct ent *to_fix;
int modflg;
int numeric;
char *mdir;
int showsc, showsr;	/* Starting cell for highlighted range */
#ifdef RIGHT_CBUG
int	wasforw	= FALSE;
#endif

void	update();
void	repaint();

char curfile[PATHLEN];
char    revmsg[80];

int  linelim = -1;

int  showtop   = 1;	/* Causes current cell value display in top line  */
int  showcell  = 1;	/* Causes current cell to be highlighted	  */
int  showrange = 0;	/* Causes ranges to be highlighted		  */
int  showneed  = 0;	/* Causes cells needing values to be highlighted  */
int  showexpr  = 0;	/* Causes cell exprs to be displayed, highlighted */

int  autocalc = 1 ;	/* 1 to calculate after each update */
int  autolabel = 1;     /* If room, causes label to be created after a define*/
int  calc_order = BYROWS;
int  tbl_style = 0;	/* headers for T command output */
int  rndinfinity = 0;
int  numeric_field = 0; /* Started the line editing with a number */
int  craction = 0;	/* 1 for down, 2 for right */
int  rowlimit = -1;
int  collimit = -1;
#ifdef	SIGWINCH
int  hitwinch = 0;	/* got a SIGWINCH? */
#endif

extern	int lastmx, lastmy;	/* Screen address of the cursor */
extern	int lastcol, lcols;	/* Spreadsheet Column the cursor was in last */

/* a linked list of free [struct ent]'s, uses .next as the pointer */
struct ent *freeents = NULL;

extern	int	seenerr;
extern	char	*rev;

#ifdef VMS
int VMS_read_raw = 0;
#endif

/* return a pointer to a cell's [struct ent *], creating if needed */
struct ent *
lookat(row,col)
int	row, col;
{
    register struct ent **pp;

    checkbounds(&row, &col);
    pp = ATBL(tbl, row, col);
    if (*pp == (struct ent *)0) {
        if (freeents != NULL)
	{	*pp = freeents;
		freeents = freeents->next;
	}
	else
		*pp = (struct ent *) scxmalloc((unsigned)sizeof(struct ent));
	if (row>maxrow) maxrow = row;
	if (col>maxcol) maxcol = col;
	(*pp)->label = (char *)0;
	(*pp)->row = row;
	(*pp)->col = col;
	(*pp)->flags = 0;
	(*pp)->expr = (struct enode *)0;
	(*pp)->v = (double) 0.0;
	(*pp)->format = (char *)0;
	(*pp)->cellerror = CELLOK;
	(*pp)->next = NULL;
    }
    return *pp;
}

/*
 * This structure is used to keep ent structs around before they
 * are deleted to allow the sync_refs routine a chance to fix the
 * variable references.
 * We also use it as a last-deleted buffer for the 'p' command.
 */
void
free_ent(p)
register struct ent *p;
{
    p->next = to_fix;
    to_fix = p;
    p->flags |= is_deleted;
    p->flags &= ~is_locked;
}

/* free deleted cells */
void
flush_saved()
{
    register struct ent *p;
    register struct ent *q;

    if (!(p = to_fix))
	return;
    while (p) {
	(void) clearent(p);
	q = p->next;
	p->next = freeents;	/* put this ent on the front of freeents */
	freeents = p;
	p = q;
    }
    to_fix = NULL;
}

char    *progname;

int
main (argc, argv)
int argc;
char  **argv;
{
    int     inloop = 1;
    register int   c;
    int     edistate = -1;
    int     arg = 1;
    int     narg;
    int     nedistate;
    int	    running;
    char    *revi;
    int	    anychanged = FALSE;

    /*
     * Keep command line options around until the file is read so the
     * command line overrides file options
     */

    int Mopt = 0;
    int Nopt = 0;
    int Copt = 0; 
    int Ropt = 0;

    int tempx, tempy; 	/* Temp versions of curx, cury */

#if defined(MSDOS)
    if ((revi = strrchr(argv[0], '\\')) != NULL)
#else
#ifdef VMS
    if ((revi = strrchr(argv[0], ']')) != NULL)
#else
    if ((revi = strrchr(argv[0], '/')) != NULL)
#endif
#endif
	progname = revi+1;
    else
	progname = argv[0];

    while (argc > 1 && argv[1][0] == '-') {
	argv++;
	argc--;
    	switch (argv[0][1]) {
	    case 'x':
#if defined(VMS) || defined(MSDOS) || !defined(CRYPT_PATH)
		    (void) fprintf(stderr, "Crypt not available\n");
		    exit(1);
#else 
		    Crypt = 1;
#endif
		    break;
	    case 'm':
		    Mopt = 1;
		    break;
	    case 'n':
		    Nopt = 1;
		    break;
	    case 'c':
		    Copt = 1;
		    break;
	    case 'r':
		    Ropt = 1;
		    break;
	    case 'C':
		    craction = CRCOLS;
		    break;
	    case 'R':
		    craction = CRROWS;
		    break;
	    default:
		    (void) fprintf(stderr,"%s: unrecognized option: \"%c\"\n",
			progname,argv[0][1]);
		    exit(1);
	}
    }

    *curfile ='\0';

    startdisp();
    signals();

	/* setup the spreadsheet arrays, initscr() will get the screen size */
    if (!growtbl(GROWNEW, 0, 0))
    {	stopdisp();
	exit(1);
    }

    /*
     * Build revision message for later use:
     */

    (void) strcpy (revmsg, progname);
    for (revi = rev; (*revi++) != ':'; );	/* copy after colon */
    (void) strcat (revmsg, revi);
    revmsg [strlen (revmsg) - 2] = 0;		/* erase last character */
    (void) strcat (revmsg, ":  Type '?' for help.");

    if (argc > 1) {
	(void) strcpy(curfile,argv[1]);
	readfile (argv[1], 0);
    }

    if (Mopt)
	autocalc = 0;
    if (Nopt)
	numeric = 1;
    if (Copt)
	calc_order = BYCOLS;
    if (Ropt)
	calc_order = BYROWS;

    modflg = 0;
#ifdef VENIX
    setbuf (stdin, NULL);
#endif
    FullUpdate++;

    while (inloop) { running = 1;
    while (running) {
	nedistate = -1;
	narg = 1;
	if (edistate < 0 && linelim < 0 && autocalc && (changed || FullUpdate))
	{    EvalAll ();
	     if (changed)		/* if EvalAll changed or was before */
		anychanged = TRUE;
	     changed = 0;
	}
	else		/* any cells change? */
	if (changed)
	     anychanged = TRUE;

#ifdef	SIGWINCH
	/* got a SIGWINCH? */
	if (hitwinch)
	{	hitwinch = 0;
		stopdisp();
		startdisp();
		FullUpdate++;
	}
#endif
	update(anychanged);
	anychanged = FALSE;
#ifndef SYSV3	/* HP/Ux 3.1 this may not be wanted */
	(void) refresh(); /* 5.3 does a refresh in getch */ 
#endif
	c = nmgetch();
	getyx(stdscr, tempy, tempx);
	(void) move (1, 0);
	(void) clrtoeol ();
	(void) move(tempy, tempx);
/*	(void) fflush (stdout);*/
	seenerr = 0;
	showneed = 0;	/* reset after each update */
	showexpr = 0;

	/*
	 * there seems to be some question about what to do w/ the iscntrl
	 * some BSD systems are reportedly broken as well
	 */
	/* if ((c < ' ') || ( c == DEL ))   how about international here ? PB */
#if	pyr
	   if ( iscntrl(c) || (c >= 011 && c <= 015) )	/* iscntrl broken in OSx4.1 */
#else
	   if (isascii(c) && (iscntrl(c) || (c == 020)) )	/* iscntrl broken in OSx4.1 */
#endif
	    switch (c) {
#ifdef SIGTSTP
		case ctl('z'):
		    (void) deraw();
		    (void) kill(0, SIGTSTP); /* Nail process group */

		    /* the pc stops here */

		    (void) goraw();
		    break;
#endif
		case ctl('r'):
		    showneed = 1;
		case ctl('l'):
		    FullUpdate++;
		    ClearScreen++;
		    (void) clearok(stdscr,1);
		    /* Centering the display with cursor for middle */
		    if(currow > (LINES-RESROW)/2)
			strow = currow - ((LINES-RESROW)/2);
		    break;
		case ctl('x'):
		    FullUpdate++;
		    showexpr = 1;
		    (void) clearok(stdscr,1);
		    break;
		default:
		    error ("No such command (^%c)", c + 0100);
		    break;
		case ctl('b'):
		    if (numeric_field) {
			write_line(ctl('m'));
			numeric_field = 0;
		    }
		    backcol(arg);
		    break;
		case ctl('c'):
		    running = 0;
		    break;

		case ctl('e'):

		    switch (nmgetch()) {
		    case ctl('p'): case 'k':	doend (-1, 0);	break;
		    case ctl('n'): case 'j':	doend ( 1, 0);	break;
		    case ctl('b'): case 'h':
		    case ctl('h'):		doend ( 0,-1);	break;
		    case ctl('f'): case 'l':
		    case ctl('i'): case ' ':	doend ( 0, 1);	break;

		    case ESC:
		    case ctl('g'):
			break;

		    default:
			error("Invalid ^E command");
			break;
		    }

		    break;

		case ctl('f'):
		    if (numeric_field) {
			write_line(ctl('m'));
			numeric_field = 0;
		    }
		    forwcol(arg);
#ifdef RIGHT_CBUG
		    wasforw++;
#endif
		    break;

		case ctl('g'):
		    showrange = 0;
		    linelim = -1;
		    (void) move (1, 0);
		    (void) clrtoeol ();
		    break;

		case ESC:	/* ctl('[') */
		    write_line(ESC);
		    break;

		case ctl('d'):
		    write_line(ctl('d'));
		    break;

		case DEL:
		case ctl('h'):
		    if (linelim < 0) {	/* not editing line */
			backcol(arg);	/* treat like ^B    */
			break;
		    }
		    write_line(ctl('h'));
		    break;

		case ctl('i'): 		/* tab */
		    if (linelim < 0) {	/* not editing line */
			forwcol(arg);
			break;
		    }
		    if (!showrange) {
			startshow();
		    } else {
			showdr();
			linelim = strlen(line);
			line[linelim++] = ' ';
			line[linelim] = '\0';
			showrange = 0;
		    }
		    linelim = strlen (line);
		    break;

		case ctl('m'):
		case ctl('j'):
		    numeric_field = 0;
		    write_line(ctl('m'));
		    switch(craction) {
		      case CRROWS:
			if ((rowlimit >= 0) && (currow >= rowlimit)) {
			    forwcol(1);
			    currow = 0;
			}
			else {
			    forwrow(1);
			}
			break;
		      case CRCOLS:
			if ((collimit >= 0) && (curcol >= collimit)) {
			    forwrow(1);
			    curcol = 0;
			}
			else {
			    forwcol(1);
			}
			break;
		      default:
			break;
		      }
		    break;

		case ctl('n'):
		    if (numeric_field) {
			write_line(ctl('m'));
			numeric_field = 0;
		    }
		    forwrow(arg);
		    break;

		case ctl('p'):
		    if (numeric_field) {
			write_line(ctl('m'));
			numeric_field = 0;
		    }
		    backrow(arg);
		    break;

		case ctl('q'):
		    break;	/* ignore flow control */

		case ctl('s'):
		    break;	/* ignore flow control */

		case ctl('t'):
#if !defined(VMS) && !defined(MSDOS) && defined(CRYPT_PATH)
		    error(
"Toggle: a:auto,c:cell,e:ext funcs,n:numeric,t:top,x:encrypt,$:pre-scale,<MORE>");
#else 				/* no encryption available */
		    error(
"Toggle: a:auto,c:cell,e:ext funcs,n:numeric,t:top,$:pre-scale,<MORE>");
#endif
		    (void) refresh();

		    switch (nmgetch()) {
			case 'a': case 'A':
			case 'm': case 'M':
			    autocalc ^= 1;
			    error("Automatic recalculation %sabled.",
				autocalc ? "en":"dis");
			    break;
			case 'n': case 'N':
			    numeric = (! numeric);
			    error ("Numeric input %sabled.",
				    numeric ? "en" : "dis");
			    break;
			case 't': case 'T':
			    showtop = (! showtop);
			    error ("Top line %sabled.", showtop ? "en" : "dis");
			    break;
			case 'c': case 'C':
			    showcell = (! showcell);
			    repaint(lastmx, lastmy, fwidth[lastcol]);
			    error ("Cell highlighting %sabled.",
				    showcell ? "en" : "dis");
			    break;
			case 'x': case 'X':
#if defined(VMS) || defined(MSDOS) || !defined(CRYPT_PATH)
			    error ("Encryption not available.");
#else 
			    Crypt = (! Crypt);
			    error ("Encryption %sabled.", Crypt? "en" : "dis");
#endif
			    break;
			case 'l': case 'L':
			    autolabel = (! autolabel);
			    error ("Autolabel %sabled.",
				   autolabel? "en" : "dis");
			    break;
			case '$':
			    if (prescale == 1.0) {
				error ("Prescale enabled.");
				prescale = 0.01;
			    } else {
				prescale = 1.0;
				error ("Prescale disabled.");
			    }
			    break;
			case 'e': case 'E':
			    extfunc = (! extfunc);
			    error ("External functions %sabled.",
				    extfunc? "en" : "dis");
			    break;
			case ESC:
			case ctl('g'):
			    --modflg;	/* negate the modflg++ */
			    break;
			case 'r': case 'R':
			    ++craction;
			    if(craction >= 3)
				craction = 0;
			    switch(craction) {
				default:
				    craction = 0; /* fall through */
				case 0:
				    error("No action after new line");
				    break;
				case CRROWS:
				    error("Down row after new line");
				    break;
				case CRCOLS:
				    error("Right column after new line");
				    break;
			    }
			    break;
			case 'z': case 'Z':
			    rowlimit = currow;
			    collimit = curcol;
			    error("Row and column limits set");
			    break;
			default:
			    error ("Invalid toggle command");
			    --modflg;	/* negate the modflg++ */
		    }
		    FullUpdate++;
		    modflg++;
		    break;

		case ctl('u'):
		    narg = arg * 4;
		    nedistate = 1;
		    break;

		case ctl('v'):	/* insert variable name */
		    if (linelim > 0)
		        ins_string(v_name(currow, curcol));
		    break;

		case ctl('w'):	/* insert variable expression */
		    if (linelim > 0)  {
			static	char *temp = NULL, *temp1 = NULL;
			static	unsigned	templen = 0;
			int templim;

			/* scxrealloc will scxmalloc if needed */
			if (strlen(line)+1 > templen)
			{	templen = strlen(line)+40;

				temp = scxrealloc(temp, templen);
				temp1= scxrealloc(temp1, templen);
			}
			strcpy(temp, line);
			templim = linelim;
			linelim = 0;		/* reset line to empty	*/
			editexp(currow,curcol);
			strcpy(temp1, line);
			strcpy(line, temp);
			linelim = templim;
			ins_string(temp1);
		    }
		    break;

		case ctl('a'):	/* insert variable value */
		    if (linelim > 0) {
			struct ent *p = *ATBL(tbl, currow, curcol);
			char temp[100];

			if (p && p -> flags & is_valid) {
			    (void) sprintf (temp, "%.*f",
					precision[curcol],p -> v);
			    ins_string(temp);
			}
		    }
		    break;

	    } /* End of the control char switch stmt */
	else if (isascii(c) && isdigit(c) && ((numeric && edistate >= 0) ||
			(!numeric && (linelim < 0 || edistate >= 0)))) {
	    /* we got a leading number */
	    if (edistate != 0) {
		/* First char of the count */
		if (c == '0')      /* just a '0' goes to left col */
		    curcol = 0;
		else {
		    nedistate = 0;
		    narg = c - '0';
		}
	    } else {
		/* Succeeding count chars */
		nedistate = 0;
		narg = arg * 10 + (c - '0');
	    }
	} else if (linelim >= 0) {
	    /* Editing line */
	    switch(c) {
	    case ')':
		if (showrange) {
		    showdr();
		    showrange = 0;
		    linelim = strlen (line);
		}
		break;
	    default:
		break;
	    }
	    write_line(c);

	} else if (!numeric && ( c == '+' || c == '-' ) ) {
	    /* increment/decrement ops */
	    register struct ent *p = *ATBL(tbl, currow, curcol);
	    if (!p)
		continue;
	    if (p->expr && !(p->flags & is_strexpr)) {
		error("Can't increment/decrement a formula\n");
		continue;
	    }
	    FullUpdate++;
	    modflg++;
	    if( c == '+' )
	    	p -> v += (double) arg;
	    else
		p -> v -= (double) arg;
	} else
	    /* switch on a normal command character */
	    switch (c) {
		case ':':
		    break;	/* Be nice to vi users */

		case '@':
		    EvalAll ();
		    changed = 0;
		    anychanged = TRUE;
		    break;

		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		case '-': case '.': case '+':
		    if (locked_cell(currow, curcol))
			break;
		    numeric_field = 1;
		    (void) sprintf(line,"let %s = %c",
				v_name(currow, curcol), c);
		    linelim = strlen (line);
		    insert_mode();
		    break;

		case '=':
		    if (locked_cell(currow, curcol))
			break;
		    (void) sprintf(line,"let %s = ",
					v_name(currow, curcol));
		    linelim = strlen (line);
		    insert_mode();
		    break;

		case '!':
		    {
		    /*
		     *  "! command"  executes command
		     *  "!"	forks a shell
		     *  "!!" repeats last command
		     */
#if VMS || MSDOS
		    error("Not implemented on VMS or MS-DOS");
#else /* VMS */
		    char *shl;
		    int pid, temp;
		    char cmd[MAXCMD];
		    static char lastcmd[MAXCMD];

		    if (!(shl = getenv("SHELL")))
			shl = "/bin/sh";

		    deraw();
		    (void) fputs("! ", stdout);
		    (void) fflush(stdout);
		    (void) fgets(cmd, MAXCMD, stdin);
		    cmd[strlen(cmd) - 1] = '\0';	/* clobber \n */
		    if(strcmp(cmd,"!") == 0)		/* repeat? */
			    (void) strcpy(cmd, lastcmd);
		    else
			    (void) strcpy(lastcmd, cmd);

		    if (modflg)
		    {
			(void) puts ("[No write since last change]");
			(void) fflush (stdout);
		    }

		    if (!(pid = fork()))
		    {
			(void) signal (SIGINT, SIG_DFL);  /* reset */
			if(strlen(cmd))
				(void)execl(shl,shl,"-c",cmd,(char *)0);
			else
				(void) execl(shl, shl, (char *)0);
			exit(-127);
		    }

		    while (pid != wait(&temp));

		    (void) printf("Press RETURN to continue ");
		    fflush(stdout);
		    (void)nmgetch();
		    goraw();
#endif /* VMS */
		    break;
		    }

		/*
		 * Range commands:
		 */

		case '/':
		    error (
"Range: x:erase v:value c:copy f:fill d:def l:lock U:unlock s:show u:undef F:fmt");
		    (void) refresh();

		    switch (nmgetch()) {
		    case 'l':
			(void) sprintf(line,"lock [range] ");
			linelim = strlen(line);
			startshow();
			insert_mode();
			break;
		    case 'U':
			(void) sprintf(line,"unlock [range] ");
			linelim = strlen(line);
			startshow();
			insert_mode();
			break;
		    case 'c':
			(void) sprintf(line,"copy [dest_range src_range] ");
			linelim = strlen(line);
			startshow();
			insert_mode();
			break;
		    case 'x':
			(void) sprintf(line,"erase [range] ");
			linelim = strlen(line);
			startshow();
			insert_mode();
			break;
		    case 'v':
			(void) sprintf(line, "value [range] ");
			linelim = strlen(line);
			startshow();
			insert_mode();
			break;
		    case 'f':
			(void) sprintf(line,"fill [range start inc] ");
			linelim = strlen(line);
			startshow();
			insert_mode();
			break;
		    case 'd':
			(void) sprintf(line,"define [string range] \"");
			linelim = strlen(line);
			startshow();
			insert_mode();
			modflg++;
			break;
		    case 'u':
			(void) sprintf(line,"undefine [range] ");
			linelim = strlen(line);
			insert_mode();
			modflg++;
			break;
		    case 's':
			if(are_ranges())
			{
			FILE *f;
			int pid;
			char px[MAXCMD] ;
			char *pager;

			(void) strcpy(px, "| sort | ");
			if(!(pager = getenv("PAGER")))
				pager = DFLT_PAGER;
			(void) strcat(px,pager);
			f = openout(px, &pid);
			if (!f) {
			    error("Can't open pipe to sort");
			    break;
			}
			list_range(f);
			closeout(f, pid);
			}
			else error("No ranges defined");
			break;
		    case 'F':
			(void) sprintf(line, "fmt [range \"format\"] ");
			linelim = strlen(line);
			startshow();
			insert_mode();
			break;
		    case ESC:
		    case ctl('g'):
			break;
		   default:
			error("Invalid region command");
			break;
		   }
		   break;

		/*
		 * Row/column commands:
		 */

		case 'i':
		case 'a':
		case 'd':
		case 'p':
		case 'v':
		case 'z':
		case 's':
		    {
			register rcqual;

			if (! (rcqual = get_rcqual (c))) {
			    error ("Invalid row/column command");
			    break;
			}

			error ("");	/* clear line */

			if ( rcqual == ESC || rcqual == ctl('g'))
			    break;

			switch (c) {

			case 'i':
			    if (rcqual == 'r')	insertrow(arg);
			    else		opencol(curcol, arg);
			    break;

			case 'a':
			    if (rcqual == 'r')	while (arg--) duprow();
			    else		while (arg--) dupcol();
			    break;

			case 'd':
			    if (rcqual == 'r')	deleterow(arg);
			    else		closecol(curcol, arg);
			    break;

			case 'p':
			    while (arg--)	pullcells(rcqual);
			    break;

			/*
			 * turn an area starting at currow/curcol into
			 * constants vs expressions - not reversable
			 */
			case 'v':
			    if (rcqual == 'r')
				valueize_area(currow, 0,
					      currow + arg - 1, maxcol);
			    else
				valueize_area(0, curcol,
					      maxrow, curcol + arg - 1);
			    modflg = 1;
			    break;

			case 'z':
			    if (rcqual == 'r')	hiderow(arg);
			    else		hidecol(arg);
			    break;

			case 's':
			    /* special case; no repeat count */

			    if (rcqual == 'r')	rowshow_op();
			    else		colshow_op();
			    break;
			}
			break;
		    }

		case '$':
		    {
		    register struct ent *p;

		    curcol = maxcols - 1;
		    while (!VALID_CELL(p, currow, curcol) && curcol > 0)
			curcol--;
		    break;
		    }
		case '#':
		    {
		    register struct ent *p;

		    currow = maxrows - 1;
		    while (!VALID_CELL(p, currow, curcol) && currow > 0)
			currow--;
		    break;
		    }
		case 'w':
		    {
		    register struct ent *p;

		    while (--arg>=0) {
			do {
			    if (curcol < maxcols - 1)
				curcol++;
			    else {
				if (currow < maxrows - 1) {
				    while(++currow < maxrows - 1 &&
					    row_hidden[currow]) /* */;
				    curcol = 0;
				} else {
				    error("At end of table");
				    break;
				}
			    }
			} while(col_hidden[curcol] ||
				!VALID_CELL(p, currow, curcol));
		    }
		    break;
		    }
		case 'b':
		    {
		    register struct ent *p;

		    while (--arg>=0) {
			do {
			    if (curcol) 
				curcol--;
			    else {
				if (currow) {
				    while(--currow &&
					row_hidden[currow]) /* */;
				    curcol = maxcols - 1;
				} else {
				    error ("At start of table");
				    break;
				}
			    }
			} while(col_hidden[curcol] ||
				!VALID_CELL(p, currow, curcol));
		    }
		    break;
		    }
		case '^':
		    currow = 0;
		    break;
		case '?':
		    help();
		    break;
		case '"':
		    if (!locked_cell(currow, curcol)) {
		       (void) sprintf (line, "label %s = \"",
					   v_name(currow, curcol));
		       linelim = strlen (line);
		       insert_mode();
		    }
		    break;

		case '<':
		    if (!locked_cell(currow, curcol)) {
		       (void) sprintf (line, "leftstring %s = \"",
			       v_name(currow, curcol));
		       linelim = strlen (line);
		       insert_mode();
		    }
		    break;

		case '>':
		    if (!locked_cell(currow, curcol)) {
		       (void) sprintf (line, "rightstring %s = \"",
			      v_name(currow, curcol));
		       linelim = strlen (line);
		       insert_mode();
		    }
		    break;
		case 'e':
		    if (!locked_cell(currow, curcol)) {
		       editv (currow, curcol);
		       edit_mode();
		    }
		    break;
		case 'E':
		    if (!locked_cell(currow, curcol)) {
		       edits (currow, curcol);
		       edit_mode();
		    }
		    break;
		case 'f':
		    if (arg == 1)
			(void) sprintf (line, "format [for column] %s ",
				coltoa(curcol));
		    else {
			(void) sprintf(line, "format [for columns] %s:",
				coltoa(curcol));
			(void) sprintf(line+strlen(line), "%s ",
				coltoa(curcol+arg-1));
		    }
		    error("Current format is %d %d %d",
			fwidth[curcol],precision[curcol],realfmt[curcol]);
		    linelim = strlen (line);
		    insert_mode();
		    break;
		case 'F': {
		    register struct ent *p = *ATBL(tbl, currow, curcol);
		    if (p && p->format)
		    {	(void) sprintf(line, "fmt [format] %s \"%s",
				   v_name(currow, curcol), p->format);
			edit_mode();
		    }
		    else
		    {	(void) sprintf(line, "fmt [format] %s \"",
				   v_name(currow, curcol));
			insert_mode();
		    }
		    linelim = strlen(line);
		    break;
		}
		case 'g':
		    (void) sprintf (line, "goto [v] ");
		    linelim = strlen (line);
		    insert_mode();
		    break;
		case 'P':
		    (void) sprintf (line, "put [\"dest\" range] \"");
		    if (*curfile)
			error ("Default path is \"%s\"",curfile);
		    linelim = strlen (line);
		    insert_mode();
		    break;
		case 'M':
		    (void) sprintf (line, "merge [\"source\"] \"");
		    linelim = strlen (line);
		    insert_mode();
		    break;
		case 'R':
		    if (mdir)
			(void) sprintf (line,"merge [\"macro_file\"] \"%s/", mdir);
		    else
			(void) sprintf (line,"merge [\"macro_file\"] \"");
		    linelim = strlen (line);
		    insert_mode();
		    break;
		case 'D':
		    (void) sprintf (line, "mdir [\"macro_directory\"] \"");
		    linelim = strlen (line);
		    insert_mode();
		    break;
		case 'G':
		    (void) sprintf (line, "get [\"source\"] \"");
		    if (*curfile)
			error ("Default file is \"%s\"",curfile);
		    linelim = strlen (line);
		    insert_mode();
		    break;
		case 'W':
		    (void) sprintf (line, "write [\"dest\" range] \"");
		    if (*curfile)
                       error ("Default file is \"%s.asc\"",curfile);
		    linelim = strlen (line);
		    insert_mode();
		    break;
		case 'S':	/* set options */
		    (void) sprintf (line, "set ");
		    error("Options:byrows,bycols,iterations=n,tblstyle=(0|tbl|latex|slatex|tex|frame),<MORE>");
		    linelim = strlen (line);
		    insert_mode();
		    break;
		case 'T':	/* tbl output */
		    (void) sprintf (line, "tbl [\"dest\" range] \"");
		    if (*curfile && tbl_style == 0)
                       error ("Default file is \"%s.cln\"",curfile);
                    else if (*curfile && tbl_style == TBL)
                       error ("Default file is \"%s.tbl\"",curfile);
                    else if (*curfile && tbl_style == LATEX)
                       error ("Default file is \"%s.lat\"",curfile);
                    else if (*curfile && tbl_style == SLATEX)
                       error ("Default file is \"%s.stx\"",curfile);
                    else if (*curfile && tbl_style == TEX)
                       error ("Default file is \"%s.tex\"",curfile);
		    linelim = strlen (line);
		    insert_mode();
		    break;
		case 'x':
		    {
		    register struct ent **pp;
		    register int c1;

		    flush_saved();
		    if(calc_order == BYROWS) {
		      for (c1 = curcol; arg-- && c1 < maxcols; c1++) {
			pp = ATBL(tbl, currow, c1);
		        if ((*pp) && !locked_cell(currow, curcol)) {
			   if (*pp) {
			       free_ent(*pp);
			       *pp = (struct ent *)0;
			   }
		        }
		      }
		    }
		    else {
		      for (c1 = currow; arg-- && c1 < maxrows; c1++) {
			pp = ATBL(tbl, c1, curcol);
		        if ((*pp) && !locked_cell(currow, curcol)) {
			   if (*pp) {
			       free_ent(*pp);
			       *pp = (struct ent *)0;
			   }
		        }
		      }
		    }
		    sync_refs();
		    modflg++;
		    FullUpdate++;
		    }
		    break;
		case 'Q':
		case 'q':
		    running = 0;
		    break;
		case 'h':
		    backcol(arg);
		    break;
		case 'j':
		    forwrow(arg);
		    break;
		case 'k':
		    backrow(arg);
		    break;
		case 'H':
			backcol((curcol-stcol+1)+1);
			break;
#ifdef KEY_NPAGE
		case KEY_NPAGE:			/* page precedente */
#endif
		case 'J':
			forwrow(LINES-RESROW-(currow-strow)+1);
			break;
#ifdef	KEY_PPAGE
		case KEY_PPAGE:			/* page suivante */
#endif
		case 'K':
			backrow((currow-strow+1)+3);
			break;
#ifdef KEY_HOME
		case KEY_HOME:
			currow = 0;
			curcol = 0;
			FullUpdate++;
			break;
#endif
		case 'L':
			forwcol(lcols -(curcol-stcol)+1);
			break;
		case ' ':
		case 'l':
		    forwcol(arg);
		    break;
		case 'm':
		    savedrow = currow;
		    savedcol = curcol;
		    break;
		case 'c': {
		    register struct ent *p = *ATBL(tbl, savedrow, savedcol);
		    register c1;
		    register struct ent *n;
		    if (!p)
			break;
		    FullUpdate++;
		    modflg++;
		    for (c1 = curcol; arg-- && c1 < maxcols; c1++) {
			n = lookat (currow, c1);
			(void) clearent(n);
			copyent( n, p, currow - savedrow, c1 - savedcol);
		    }
		    break;
		}
		default:
		    if ((toascii(c)) != c)
			error ("Weird character, decimal %d\n",
				(int) c);
		    else
			    error ("No such command (%c)", c);
		    break;
	    }
	edistate = nedistate;
	arg = narg;
    }				/* while (running) */
    inloop = modcheck(" before exiting");
    }				/*  while (inloop) */
    stopdisp();
#ifdef VMS	/* Until VMS "fixes" exit we should say 1 here */
    exit(1);
#else
    exit(0);
#endif
    /*NOTREACHED*/
}

/* show the current range (see ^I), we are moving around to define a range */
void
startshow()
{
    showrange = 1;
    showsr = currow;
    showsc = curcol;
}

/* insert the range we defined by moving around the screen, see startshow() */
void
showdr()
{
    int     minsr, minsc, maxsr, maxsc;

    minsr = showsr < currow ? showsr : currow;
    minsc = showsc < curcol ? showsc : curcol;
    maxsr = showsr > currow ? showsr : currow;
    maxsc = showsc > curcol ? showsc : curcol;
    (void) sprintf (line+linelim,"%s", r_name(minsr, minsc, maxsr, maxsc));
}

/* set the calculation order */
void
setorder(i)
int i;
{
	if((i == BYROWS)||(i == BYCOLS))
	    calc_order = i;
}

void
setauto(i)
int i;
{
	autocalc = i;
}

void
signals()
{
#ifdef SIGVOID
    void doquit();
    void time_out();
    void dump_me();
#ifdef	SIGWINCH
    void winchg();
#endif
#else
    int doquit();
    int time_out();
    int dump_me();
#ifdef	SIGWINCH
    int winchg();
#endif
#endif

    (void) signal(SIGINT, SIG_IGN);
#if !defined(MSDOS)
    (void) signal(SIGQUIT, dump_me);
    (void) signal(SIGPIPE, doquit);
    (void) signal(SIGALRM, time_out);
    (void) signal(SIGBUS, doquit);
#endif
    (void) signal(SIGTERM, doquit);
    (void) signal(SIGFPE, doquit);
#ifdef	SIGWINCH
    (void) signal(SIGWINCH, winchg);
#endif
}

#ifdef	SIGWINCH
#ifdef SIGVOID
void
#else
int
#endif
winchg()
{	hitwinch++;
	(void) signal(SIGWINCH, winchg);
}
#endif

#ifdef SIGVOID
void
#else
int
#endif
doquit()
{
    diesave();
    stopdisp();
    exit(1);
}

#ifdef SIGVOID
void
#else
int
#endif
dump_me()
{
    diesave();
    deraw();
    abort();
}

/* try to save the current spreadsheet if we can */
void
diesave()
{   char	path[PATHLEN];

    if (modcheck(" before Spreadsheet dies") == 1)
    {	(void) sprintf(path, "~/%s", SAVENAME);
	if (writefile(path, 0, 0, maxrow, maxcol) < 0)
	{
	    (void) sprintf(path, "/tmp/%s", SAVENAME);
	    if (writefile(path, 0, 0, maxrow, maxcol) < 0)
		error("Couldn't save current spreadsheet, Sorry");
	}
    }
}

/* check if tbl was modified and ask to save */
int
modcheck(endstr)
char *endstr;
{
    if (modflg && curfile[0]) {
	int	yn_ans;
	char	lin[100];

	(void) sprintf (lin,"File \"%s\" is modified, save%s? ",curfile,endstr);
	if ((yn_ans = yn_ask(lin)) < 0)
		return(1);
	else
	if (yn_ans == 1)
	{    if (writefile(curfile, 0, 0, maxrow, maxcol) < 0)
 		return (1);
	}
    } else if (modflg) {
	int	yn_ans;

	if ((yn_ans = yn_ask("Do you want a chance to save the data? ")) < 0)
		return(1);
	else
		return(yn_ans);
    }
    return(0);
}

/* Returns 1 if cell is locked, 0 otherwise */
int
locked_cell (r, c)
	int r, c;
{
	struct ent *p = *ATBL(tbl, r, c);
	if (p && (p->flags & is_locked)) {
		error("Cell %s%d is locked", coltoa(c), r) ;
		return(1);
	}
	return(0);
}

/* Check if area contains locked cells */
int
any_locked_cells(r1, c1, r2, c2)
	int r1, c1, r2, c2 ;
{
	int r, c;
	struct ent *p ;

	for (r=r1; r<=r2; r++)
		for (c=c1; c<=c2; c++) {
			p = *ATBL(tbl, r, c);
			if (p && (p->flags & is_locked))
				return(1);
		}
	return(0);
}
