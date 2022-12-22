# include <stdio.h>
# include <assert.h>
# include <setjmp.h>

struct strvar
{
    char   *s_p;
    short   s_strlen;
};

enum valkind
{
    V_NULL, V_REAL, V_INT, V_STRING, V_BOOLEAN, V_INT_VAR,
    V_STRING_VAR, V_REAL_VAR, V_1D_REAL, V_2D_REAL, V_1D_INT,
    V_2D_INT, V_1D_STRING, V_2D_STRING

};

struct value
{
    enum valkind v_kind;
    union
    {
	double  V_number;
	double *V_numbervar;
	char   *V_string;
	struct strvar V_stringvar;
	int    *V_intvar;
	int     V_int;

	struct
	{
	    char   *m_area;
	    unsigned m_dim1:16, m_dim2:16;
	    short   m_strlen;
	}       V_smat;

	struct
	{
	    double *m_area;
	    unsigned m_dim1:16, m_dim2:16;
	}       V_rmat;

	struct
	{
	    int    *m_area;
	    unsigned m_dim1:16, m_dim2:16;
	}       V_imat;
    }       v_un;
};


enum treeop
{
    T_ASSIGN, T_DATA, T_DATAEXPRLIST, T_DIM, T_DIMLIST, T_DIM0, T_DIM1,
    T_DIM2, T_DIM_INT, T_DIM_STRING, T_DIM_NUM, T_SYNTAX, T_DIV, T_EQUAL,
    T_LONG_PRINT, T_SHORT_PRINT, T_FOR, T_FOR_INT, T_GEQ, T_GOSUB, T_GOTO,
    T_GT, T_REALID, T_IF, T_INPUT, T_READ, T_RESTORE, T_LEQ, T_LT,
    T_MINUS, T_NEQ, T_NEXT, T_NULL, T_REAL, T_PLUS, T_POWER, T_PRINT,
    T_RETURN, T_STOP, T_STRING, T_SUB1, T_SUB2, T_TIMES, T_UMINUS,
    T_VARLIST, T_END, T_REM, T_OR, T_AND, T_PRINT_NOCR, T_NOT, T_INTID,
    T_STRINGID, T_CONCAT, T_BREAK, T_PARENS, T_MAT_ASSIGN, T_MAT_ADD,
    T_MAT_SUB, T_MAT_MUL, T_MAT_SC_ASS, T_MAT_SC_ADD, T_MAT_SC_SUB,
    T_MAT_SC_MUL, T_MAT_REALIDN, T_MAT_ZER, T_MAT_INV, T_MAT_TRN,
    T_MAT_UMINUS, T_MAT_INPUT, T_MAT_READ, T_MAT_PRINT, T_STR_EQUAL,
    T_STR_GEQ, T_STR_GT, T_STR_NEQ, T_STR_LEQ, T_STR_LT, T_STR_ASSIGN,
    T_MAT_FUNCALL0, T_MAT_FUNCALL1, T_MAT_FUNCALL2, T_MAT_FUNCALLN,
    T_FUNCALL0, T_FUNCALL1, T_FUNCALL2, T_FUNCALLN, T_SIN, T_COS, T_TAN,
    T_SQR, T_EXP, T_LOG, T_ABS, T_INT, T_ACS, T_ASN, T_ATN, T_COT, T_CSC,
    T_HCS, T_HSN, T_HTN, T_LGT, T_LTW, T_SEC, T_RND, T_SET_RND, T_DEG,
    T_RAD, T_MAX, T_MIN, T_CHR, T_NUM, T_LEN, T_SGN, T_SUM, T_PRD, T_DOT,
    T_TIM, T_CPU, T_REALIDX, T_STR2, T_STR3, T_DET
};

typedef union
{
    int     i_val;
    long    l_val;
    double  n_val;
    char   *s_val;
    int     v_val;		/* void */
    struct tree_node *t_val;
    enum treeop o_val;
    struct
    {
	long    l_first;
	long    l_last;
    }       r_val;
    struct
    {
	struct tree_node *l_head;
	struct tree_node *l_tail;
    }       list_val;
    int     w_val;
}       YYSTYPE;

#define reg register

extern char    CurFile[50];

extern char    Opts[128];
extern int    TimeToQuit;
extern int    TerminalInput;

extern char   *strcpy (), *strcat ();
extern struct tree_node *NextData;
extern long    DataPC;
extern int     DataCount;
extern char   *malloc (), *calloc (), *realloc ();

#define TabCount(w)	(w>>3)
#define SpaceCount(w)	(w & 7)
extern char   *IPtr;			/* input pointer */
extern YYSTYPE yylval;

#define	DEFAULT_STRLEN	32
#define	MAX_STRLEN	256

extern char   *SaveString ();
#define GOSUB_DEPTH	120
#define EXPR_DEPTH	40
#define FOR_DEPTH	10
/*#define STMT_LIMIT	500000*/

extern long    PC;
extern long    OldPC;
extern struct line_descr *PCP;

extern int     Break;
extern int     Stop;
extern int     SingleStepping;
extern int     FromTerminal;
extern int     OutputCol;

extern long StmtCount;

extern long    GosubStack[GOSUB_DEPTH], *GP;

extern struct value Stack[EXPR_DEPTH], *SP;

struct for_context
{
    long    f_line;
    struct line_descr *f_linep;
    struct name *f_var;
    double  f_limit;
    double  f_step;
};
extern struct for_context        ForStack[FOR_DEPTH], *FP;

extern double *DoubleTemp ();
extern double  ApplyFunc0 ();
extern double  ApplyFunc1 ();
extern double  ApplyFunc2 ();
extern int    *IntTemp ();
extern char   *StringTemp ();
extern struct value *InputValue (), *ReadValue ();
extern struct name *FindMatrix ();
#define LINE_LEN	256

extern char    InputLine[LINE_LEN];
extern int    InputPrinted, SaveStrings;
extern char   *IPtr;			/* input pointer */
extern int    LookForCmd;
extern char   *FindStart ();

#define	SkipBlanks(ip)	while (isspace(*ip)) ++ip

#define INFINITY	(long) 2147483647
#define ZERO		(long) 0
#define LineAfter(l)	(l+1)
#define LineBefore(l)	(l-1)
#define IsValidLine(l)	((l) < LastLine)
#define LineCount()	(FastCount-1)

#define MadeChanges()	ChangesMade = 1
#define ClearChanges()	ChangesMade = 0

struct line_descr
{
    long    l_no;
    int     l_indent;
    struct tree_node *l_tree;
    char pad[4];		/*XXX*/
};

extern struct line_descr *FirstLine;
extern struct line_descr *LastLine;
extern struct line_descr *FastTab;
extern unsigned FastCount, FastSize;
extern int     ChangesMade;

extern struct line_descr *FindLine ();

struct name
{
    char   *s_name;
    struct name *s_next;
    struct value s_val;
};

#define SYMBOL_HASH_SIZE	1327

extern struct name *SymBuckets[];

extern struct name *AllocSymbol (), *FindSymbol ();
/*
 * String temporary space
 *
 * It is assumed that a string temporary is used before the next
 * is requested.
 */

#define STRING_TEMP_SIZE	(3*MAX_STRLEN+10)

extern char    StringSpace[STRING_TEMP_SIZE], *StringTempPtr;
extern double *DoubleSpace;
extern int    *IntSpace;

#define tree0(t)	t->t_args[0].t_tree
#define tree1(t)	t->t_args[1].t_tree
#define tree2(t)	t->t_args[2].t_tree
#define tree3(t)	t->t_args[3].t_tree
#define treei(t, i)	t->t_args[i].t_tree

#define stringn(t,n)	t->t_args[n].t_string
#define string0(t)	t->t_args[0].t_string
#define string1(t)	t->t_args[1].t_string
#define string2(t)	t->t_args[2].t_string
#define string3(t)	t->t_args[3].t_string

#define lineno0(t)	t->t_args[0].t_lineno
#define lineno1(t)	t->t_args[1].t_lineno
#define lineno2(t)	t->t_args[2].t_lineno
#define lineno3(t)	t->t_args[3].t_lineno

#define int0(t)	t->t_args[0].t_int
#define int1(t)	t->t_args[1].t_int
#define int2(t)	t->t_args[2].t_int
#define int3(t)	t->t_args[3].t_int
#define int4(t)	t->t_args[4].t_int

#define number0(t)	t->t_args[0].t_number

#define op0(t)		t->t_args[0].t_op
#define op1(t)		t->t_args[1].t_op

struct tree_node
{
    int t_len;				/* though it's only 1-5 */
    enum treeop t_op;
    int t_version[5];			/* which pointer version we're on */
    struct name *t_symbols[5];		/* when we've already look up strings */
    union
    {
	int     t_int;
	long    t_lineno;
	double  t_number;
	char   *t_string;
	struct tree_node *t_tree;
	enum treeop t_op;
    }       t_args[5];
};

extern struct tree_node *Tree (), *TypeCheck ();
extern char   *OpName (), *VarName ();
extern enum treeop NumStrOpMap ();


#define v_real	v_un.V_number
#define v_string	v_un.V_string
#define v_int		v_un.V_int
#define	v_realvar	v_un.V_numbervar
#define v_stringvar	v_un.V_stringvar
#define v_intvar	v_un.V_intvar
#define	v_smat		v_un.V_smat
#define	v_rmat		v_un.V_rmat
#define	v_imat		v_un.V_imat

extern char   *ValueName ();

struct op_descr
{
    char   *o_name;
    int     o_nargs;
    char   *o_fmt;
};
extern struct op_descr OpDescrs[];

extern enum treeop MatOpMap (), MatScOpMap ();

#define FreeDoubleTemp(p)	{     \
	if (p != DoubleSpace) {     \
		RunError("Basic Bug!  Bad double space returned.");     \
		abort();     \
	}     \
	free((char *)p);     \
	DoubleSpace = NULL;     \
}

#define FreeIntTemp(p)	{  \
	if (p != IntSpace) {  \
		RunError("Basic Bug!  Bad int space returned.");  \
		abort();  \
	}  \
	free((char *)p);  \
	IntSpace = NULL;  \
}
/* NOTE:  Many of these are missing below -- */
#ifdef NO
#ifdef DEBUG
    if (Opts['S']) { printf ("pop %g (PopInt)\n", v->v_real); }
#endif
#endif

/*extern double  PopReal ();*/
#define PopReal() (   \
    SP < Stack ? RunError ("Stack underflow (PopReal).  Basic bug!"):0, \
    SP->v_kind != V_REAL ? RunError ("Non-numeric value popped."):0,    \
    (SP--)->v_real)

#define PopInt() (   \
    SP < Stack ? RunError ("Stack underflow (PopInt).  Basic bug!"):0,   \
    SP->v_kind != V_REAL ? RunError ("Non-numeric value popped."):0,   \
    (int) (SP--)->v_real)

/*extern char   *PopString ();*/
#define PopString() (  \
    SP < Stack? RunError ("Stack underflow (PopString).  Basic bug!"):0,  \
    SP->v_kind != V_STRING? RunError ("Non-string value popped."):0,  \
    (SP--)->v_string)

/*extern struct value *PopValue ();*/
#define PopValue() (  \
    SP < Stack ? RunError ("Stack underflow (PopValue).  Basic bug!"):0,  \
    SP--)
extern jmp_buf	ExecJmpBuf;
extern long MasterSymbolVersion;

extern struct name *LookupSymbol();

#define SetSymbol(t,argno,val)  \
	(t->t_version[argno] = MasterSymbolVersion, t->t_symbols[argno] = val)

#define FindSymbol(t,argno) ( \
	t->t_version[argno] == MasterSymbolVersion ?  t->t_symbols[argno] : \
        SetSymbol(t, argno, LookupSymbol(t->t_args[argno].t_string)))

char *strncpy();
char *strncat();
extern long screenwidth;
