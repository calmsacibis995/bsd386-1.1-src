/*	SC	A Table Calculator
 *		Common definitions
 *
 *		original by James Gosling, September 1982
 *		modified by Mark Weiser and Bruce Israel,
 *			University of Maryland
 *		R. Bond  12/86
 *		More mods by Alan Silverstein, 3-4/88, see list of changes.
 *		$Revision: 1.1.1.1 $
 *
 */

#if defined(MSDOS)
#include <stdio.h>
#endif

#define	ATBL(tbl, row, col)	(*(tbl + row) + (col))

#define MINROWS 100 	/* minimum size at startup */
#define MINCOLS 30 
#define	ABSMAXCOLS 702	/* absolute cols: ZZ (base 26) */

#define CRROWS 1
#define CRCOLS 2
#define RESCOL 4	/* columns reserved for row numbers */
#define RESROW 3 /* rows reserved for prompt, error, and column numbers */

/* formats for engformat() */
#define REFMTFIX	0
#define REFMTFLT	1
#define REFMTENG	2
#define REFMTDATE	3

#define DEFWIDTH 10	/* Default column width and precision */
#define DEFPREC   2
#define DEFREFMT  REFMTFIX /* Make default format fixed point  THA 10/14/90 */

#define HISTLEN  10	/* Number of history entries for vi emulation */
#ifdef PSC
# define error(msg)	fprintf(stderr, msg);
#else
# define error (void)move(1,0), (void)clrtoeol(), (void) printw
#endif
#define	FBUFLEN	1024	/* buffer size for a single field */
#define	PATHLEN	1024	/* maximum path length */

#ifndef A_CHARTEXT	/* Should be defined in curses.h */
# ifdef INTERNATIONAL
#  define A_CHARTEXT 0xff
# else
#  define A_CHARTEXT 0x7f
# endif
#endif

#if (defined(BSD42) || defined(BSD43)) && !defined(strrchr)
#define strrchr rindex
#endif

#if (defined(BSD42) || defined(BSD43)) && !defined(strchr)
#define strchr index
#endif

#ifdef SYSV4
size_t	strlen();
#endif

#ifndef FALSE
# define	FALSE	0
# define	TRUE	1
#endif /* !FALSE */

/*
 * ent_ptr holds the row/col # and address type of a cell
 *
 * vf is the type of cell address, 0 non-fixed, or bitwise OR of FIX_ROW or
 *	FIX_COL
 * vp : we just use vp->row or vp->col, vp may be a new cell just for holding
 *	row/col (say in gram.y) or a pointer to an existing cell
 */
struct ent_ptr {
    int vf;
    struct ent *vp;
};

/* holds the beginning/ending cells of a range */
struct range_s {
	struct ent_ptr left, right;
};

/*
 * Some not too obvious things about the flags:
 *    is_valid means there is a valid number in v.
 *    is_locked means that the cell cannot be edited.
 *    label set means it points to a valid constant string.
 *    is_strexpr set means expr yields a string expression.
 *    If is_strexpr is not set, and expr points to an expression tree, the
 *        expression yields a numeric expression.
 *    So, either v or label can be set to a constant. 
 *        Either (but not both at the same time) can be set from an expression.
 */

#define VALID_CELL(p, r, c) ((p = *ATBL(tbl, r, c)) && \
			     ((p->flags & is_valid) || p->label))

/* info for each cell, only alloc'd when something is stored in a cell */
struct ent {
    double v;		/* v && label are set in EvalAll() */
    char *label;
    struct enode *expr;	/* cell's contents */
    short flags;	
    short row, col;
    struct ent *next;	/* next deleted ent (pulled, deleted cells) */
    char *format;	/* printf format for this cell */
    char cellerror;	/* error in a cell? */
};

/* stores a range (left, right) */
struct range {
    struct ent_ptr r_left, r_right;
    char *r_name;			/* possible name for this range */
    struct range *r_next, *r_prev;	/* chained ranges */
    int r_is_range;
};

#define FIX_ROW 1
#define FIX_COL 2

/* stores type of operation this cell will preform */
struct enode {
    int op;
    union {
	int gram_match;         /* some compilers (hp9000ipc) need this */
	double k;		/* constant # */
	struct ent_ptr v;	/* ref. another cell */
	struct range_s r;	/* op is on a range */
	char *s;		/* string part of a cell */
	struct {		/* other cells use to eval()/seval() */
	    struct enode *left, *right;
	} o;
    } e;
};

/* op values */
#define O_VAR 'v'
#define O_CONST 'k'
#define O_ECONST 'E'	/* constant cell w/ an error */
#define O_SCONST '$'
#define REDUCE 0200	/* Or'ed into OP if operand is a range */

#define OP_BASE 256
#define ACOS OP_BASE + 0
#define ASIN OP_BASE + 1
#define ATAN OP_BASE + 2
#define CEIL OP_BASE + 3
#define COS OP_BASE + 4
#define EXP OP_BASE + 5 
#define FABS OP_BASE + 6 
#define FLOOR OP_BASE + 7
#define HYPOT OP_BASE + 8
#define LOG OP_BASE + 9
#define LOG10 OP_BASE + 10
#define POW OP_BASE + 11
#define SIN OP_BASE + 12
#define SQRT OP_BASE + 13
#define TAN OP_BASE + 14
#define DTR OP_BASE + 15
#define RTD OP_BASE + 16
#define MIN OP_BASE + 17
#define MAX OP_BASE + 18
#define RND OP_BASE + 19
#define HOUR OP_BASE + 20
#define MINUTE OP_BASE + 21
#define SECOND OP_BASE + 22
#define MONTH OP_BASE + 23
#define DAY OP_BASE + 24
#define YEAR OP_BASE + 25
#define NOW OP_BASE + 26
#define DATE OP_BASE + 27
#define FMT OP_BASE + 28
#define SUBSTR OP_BASE + 29
#define STON OP_BASE + 30
#define EQS OP_BASE + 31
#define EXT OP_BASE + 32
#define ELIST OP_BASE + 33	/* List of expressions */
#define LMAX  OP_BASE + 34
#define LMIN  OP_BASE + 35
#define NVAL OP_BASE + 36
#define SVAL OP_BASE + 37
#define PV OP_BASE + 38
#define FV OP_BASE + 39
#define PMT OP_BASE + 40
#define STINDEX OP_BASE + 41
#define LOOKUP OP_BASE + 42
#define ATAN2 OP_BASE + 43
#define INDEX OP_BASE + 44
#define DTS OP_BASE + 45
#define TTS OP_BASE + 46
#define ABS OP_BASE + 47 
#define HLOOKUP OP_BASE + 48
#define VLOOKUP OP_BASE + 49
#define ROUND OP_BASE + 50
#define IF OP_BASE + 51
#define MYROW OP_BASE + 52
#define MYCOL OP_BASE + 53
#define COLTOA OP_BASE + 54
#define UPPER OP_BASE + 55
#define LOWER OP_BASE + 56
#define CAPITAL OP_BASE + 57
#define NUMITER	OP_BASE + 58

/* flag values */
#define is_valid     0001
#define is_changed   0002
#define is_strexpr   0004
#define is_leftflush 0010
#define is_deleted   0020
#define is_locked    0040
#define is_label     0100

/* cell error (1st generation (ERROR) or 2nd+ (INVALID)) */
#define	CELLOK		0
#define	CELLERROR	1
#define	CELLINVALID	2

#define ctl(c) ((c)&037)
#define ESC 033
#define DEL 0177

/* calculation order */
#define BYCOLS 1
#define BYROWS 2

/* tblprint style output for: */
#define	TBL	1		/* 'tbl' */
#define	LATEX	2		/* 'LaTeX' */
#define	TEX	3		/* 'TeX' */
#define	SLATEX	4		/* 'SLaTeX' (Scandinavian LaTeX) */
#define	FRAME	5		/* tblprint style output for FrameMaker */

/* Types for etype() */
#define NUM	1
#define STR	2

#define	GROWAMT	30	/* default minimum amount to grow */

#define	GROWNEW		1	/* first time table */
#define	GROWROW		2	/* add rows */
#define	GROWCOL		3	/* add columns */
#define	GROWBOTH	4	/* grow both */
extern	struct ent ***tbl;	/* data table ref. in vmtbl.c and ATBL() */

extern	char curfile[];
extern	int strow, stcol;
extern	int currow, curcol;
extern	int savedrow, savedcol;
extern	int FullUpdate;
extern	int maxrow, maxcol;
extern	int maxrows, maxcols;	/* # cells currently allocated */
extern	int *fwidth;
extern	int *precision;
extern  int *realfmt;
extern	char *col_hidden;
extern	char *row_hidden;
extern	char line[FBUFLEN];
extern	int linelim;
extern	int changed;
extern	struct ent *to_fix;
extern	int showsc, showsr;

extern	FILE *openout();
extern	FILE *fdopen(), *fopen();
extern	char *coltoa();
extern	char *findhome();
extern	char *r_name();
extern	char *scxmalloc();
extern	char *scxrealloc();
extern	char *strrchr();
extern	char *v_name();
extern	int any_locked_cells();
extern	int are_ranges();
extern	int atocol();
extern	int cwritefile();
extern	int engformat();
extern	int etype();
extern	int fork();
extern	int format();
extern	int get_rcqual();
extern	int growtbl();
extern	int locked_cell();
extern	int modcheck();
extern	int nmgetch();
extern	int writefile();
extern	int yn_ask();
extern	struct enode *copye();
extern	struct enode *new();
extern	struct enode *new_const();
extern	struct enode *new_range();
extern	struct enode *new_str();
extern	struct enode *new_var();
extern	struct ent *lookat();
extern	struct range *find_range();
extern	void EvalAll();
extern	void add_range();
extern	void backcol();
extern	void backrow();
extern	void checkbounds();
extern	void clearent();
extern	void clean_range();
extern	void closecol();
extern	void closeout();
extern	void closerow();
extern	void colshow_op();
extern	void copy();
extern	void copyent();
extern	void creadfile();
extern	void deleterow();
extern	void del_range();
extern	void deraw();
extern	void diesave();
extern	void doend();
extern	void doformat();
extern	void dupcol();
extern	void duprow();
extern	void editexp();
extern	void editfmt();
extern	void edit_mode();
extern	void edits();
extern	void editv();
extern	void efree();
extern	void erase_area();
extern	void erasedb();
extern	void eraser();
extern	void fill();
extern	void flush_saved();
extern	void format_cell();
extern	void forwcol();
extern	void forwrow();
extern	void free_ent();
extern	void go_last();
extern	void goraw();
extern	void help();
extern	void hide_col();
extern	void hide_row();
extern	void hidecol();
extern	void hiderow();
extern	void initkbd();
extern	void ins_string();
extern	void insert_mode();
extern	void insertrow();
extern	void kbd_again();
extern	void label();
extern	void let();
extern	void list_range();
extern	void lock_cells();
extern	void moveto();
extern	void num_search();
extern	void opencol();
extern	void printfile();
extern	void pullcells();
extern	void readfile();
extern	void resetkbd();
extern	void rowshow_op();
extern	void scxfree();
extern	void setauto();
extern	void setiterations();
extern	void setorder();
extern	void showcol();
extern	void showdr();
extern	void showrow();
extern	void showstring();
extern	void signals();
extern	void slet();
extern	void startshow();
extern	void str_search();
extern	void sync_ranges();
extern	void sync_refs();
extern	void tblprintfile();
extern	void unlock_cells();
extern	void valueize_area();
extern	void write_fd();
extern	void write_line();
extern	void write_range();
extern	void yyerror();
#ifdef DOBACKUPS
extern	int backup_file();
#endif

extern	int modflg;
#if !defined(VMS) && !defined(MSDOS) && defined(CRYPT_PATH)
extern	int Crypt;
#endif
extern	char *mdir;
extern	double prescale;
extern	int extfunc;
extern	int propagation;
extern	int repct;
extern	int calc_order;
extern	int autocalc;
extern	int autolabel;
extern	int numeric;
extern	int showcell;
extern	int showtop;
extern	int loading;
extern	int getrcqual;
extern	int tbl_style;
extern	int rndinfinity;
extern	char *progname;
extern	int craction;
extern	int rowlimit;
extern	int collimit;

#if BSD42 || SYSIII

#ifndef cbreak
#define	cbreak		crmode
#define	nocbreak	nocrmode
#endif

#endif

#if defined(BSD42) || defined(BSD43) && !defined(ultrix)
#define	memcpy(dest, source, len)	bcopy(source, dest, (unsigned int)len);
#define	memset(dest, zero, len)		bzero((dest), (unsigned int)(len));
#else
#include <memory.h>
#endif
