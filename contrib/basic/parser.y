%{
# include <ctype.h>
# include "structs.h"
#define IncTabs(w)	(w & ~7) + 8	/* parser.c only */
#define IncSpaces(w)	w+1	/* parser.c only */
#define ClearSpace()	0	/* parser.c only */
%}

%union {
	int		i_val;
	long	l_val;
	double		n_val;
	char		*s_val;
	int		v_val;		/* void */
	struct tree_node		*t_val;
	enum treeop		o_val;
	struct {
		long	l_first;
		long	l_last;
	}		r_val;
	struct {
		struct tree_node	*l_head;
		struct tree_node	*l_tail;
	}		list_val;
	int		w_val;
}

%token	<n_val>	KNumber
%token	<s_val>	KId	KString	KIntId	KStringId
%token	<v_val>	KClear	KLet	KList	KLoad	KSave	KRenumber
		KQuit	KDebug	KRun	KDump	KDelete	KUsage
		KVi	KEd	KHelp	KContinue
		KIf	KThen	KElse	KGoto	KGosub	KReturn
		KStop	KEnd	KMat	KFor	KTo	KStep
		KPrint	KInput	KData	KNext	KDim	KRem
		KBreak	KInv	KTrn	KIdn	KStr	KRnd
		KDet	KSum	KPrd	KDot	KIdx	KChr
		KZer	KLen	KNum	KRead	KRestore

%token	<o_val>	KAddop	KMulop	KExpop	KRelop	KLogop	KFunction

%left	KLogop
%binary	KRelop	'='
%left	KAddop
%left	KMulop
%right	UMINUS	KNot
%right	KExpop

%type	<l_val>	LineNo
%type	<s_val>	FileName
%type	<v_val>	Cmd	ListCmd	LoadCmd	SaveCmd	RenumberCmd	DumpCmd
		DeleteCmd
%type	<t_val>	Stmt	Expr	PExpr	ForStmt	LetStmt	IfStmt	DataStmt
		DimStmt	DimItem	DimVar	MatStmt	ObjStmt	Variable
		PrintStmt	InputStmt	SimpleVar	Constant
		MExpr

%type	<r_val>	Range
%type	<list_val>	DExprList	VarList	PExprList	DimList
			MExprList	SimpleVarList
%type	<w_val>	WhiteSpace

%{
char	*HelpText =
"Enter a Basic subsystem command or Basic statement. Subsystem commands are:\n\
\n\
<lno> statement		insert the statement at the specified line.\n\
list <range>		list the specified lines (e.g. list 110-200).\n\
run			run the program.\n\
delete <range>		delete the specified lines.\n\
renumber <range>,init	renumber the specified lines.  Init is first new no.\n\
quit			exit Basic.\n\
step			single-step the program.\n\
vi			edit the program with 'vi'.\n\
ex			edit the program with 'ex'.\n\
load <name>		load the file called 'name'.\n\
save <name>		save the program on the file called 'name'.\n\
\n\
where	<range>		is a line-number range:\n\
			either a single line number (e.g. 110),\n\
			two line numbers separated by '-' (e.g. 1-9999),\n\
			or '%', meaning all lines (e.g. renumber %,100).\n\
	<name>		is a string of letters or digits that starts\n\
			with a letter.\n\
	<lno>		is an integer number.\n";
%}

%%
Cmd: /* empty */ { }
	| error { }
	| KClear { if (CanQuit()) { ClearBuffer(); CurFile[0] = '\0'; } }
	| KRun {
		FromTerminal = 0;
		if (!SingleStepping) { ExecProgram(); }
		else { SingleStepping = 0; Continue(); }
  	}
	| KStep
	  { FromTerminal = 0; SingleStepping = 1; InitProgram(); Continue(); }
	| KContinue { FromTerminal = 0; Continue(); }
	| KContinue KNumber {
		if (Stop) Error("Nothing left to execute.");
		else {
			int	i;
			FromTerminal = 0;
			SingleStepping = 1;
			for (i = 0; !Stop && i < (int)$2; ++i) { Continue(); }
		}
	}
	| KUsage { PrintUsage(); }
	| KVi { RunEditor("vi"); }
	| KEd { RunEditor("ex"); }
	| KHelp { printf("%s", HelpText); }
	| ListCmd
	| DumpCmd
	| LoadCmd
	| SaveCmd
	| RenumberCmd
	| DeleteCmd
	| KQuit { if (CanQuit()) TimeToQuit = 1; }
	| KDebug KString {
		char *v;
		v = $2;
		while (*v) {
		    if (v[1] == '+') { Opts[*v] = 1; v += 2; }
		    else if (v[1] == '-') { Opts[*v] = 0; v += 2; }
		    else if (isdigit(v[1])) { Opts[*v] = v[1] - '0'; v += 2; }
		    else { Opts[*v] = 1; ++v; }
		}
	}
	| Stmt {
		switch ($1->t_op) {
		case T_FOR: case T_NEXT: case T_DATA:
			Error("You can't execute that %s %s",
				"kind of statement without a line",
				"number.");
			break;
		default:
			FromTerminal = 1;
			OutputCol = 0;
			if(setjmp(ExecJmpBuf)==0) ExecStmt($1);
			FromTerminal = 0;
		}
	}
	| LineNo WhiteSpace Stmt { if ($3 != NULL) SaveLine($1, $2, $3); }
	;

WhiteSpace: /* empty */ {
		extern char	*IPtr;
		char		*cp;

		/*
		 * KLUDGE!
		 * back up cp to beginning of line,
		 * read past line number, and then find
		 * whitespace to recover indentation typed
		 * by user.
		 * If only a single space, replace with tab.
		 */
		cp = InputLine;
		SkipBlanks(cp);
		while (isdigit(*cp)) ++cp;
		$$ = ClearSpace();
		while (cp < IPtr && isspace(*cp)) {
			switch (*cp++) {
			case ' ':
				$$ = IncSpaces($$);
				break;
			case '\t':
				$$ = IncTabs($$);
				break;
			case '\f':
			case '\n':
			case '\r':
				$$ = ClearSpace();
				break;
			}
		}
		if (TabCount($$) == 0 && SpaceCount($$) == 1) {
			$$ = ClearSpace();
			$$ = IncTabs($$);
		}
	};

LoadCmd: KLoad FileName {
		if ($2 == NULL) {
		   if (CurFile[0] == '\0') { Error("No file name specified."); }
		   else { LoadFile(CurFile, 1); InitProgram(); }
		} else { LoadFile($2, 1); InitProgram(); }
	}
	;

SaveCmd: KSave FileName {
		if ($2 == NULL) {
		    if (CurFile[0] == '\0') Error("No file name specified.");
		    else SaveFile(CurFile, 1);
		} else {
		    if (CurFile[0] == '\0') strcpy(CurFile, $2);
		    SaveFile($2, 1);
		}
	}
	;

FileName: /* empty */ {
		char	*cp;
		extern char *IPtr;

		/* KLUDGE!
		 * go back to the front of the line and
		 * skip the command name, pick up the
		 * rest as the file name
		 */
		cp = InputLine;
		SkipBlanks(cp);
		while (!isspace(*++cp))
			;
		SkipBlanks(cp);
		if (*cp == '\n') *cp = '\0';
		if (*cp == '\0') { $$ = NULL; }
		else {
			IPtr = cp;
			while (*IPtr && !isspace(*IPtr)) ++IPtr;
			*IPtr = '\0';
			$$ = SaveString(cp);
		}
	}
	;

DeleteCmd: KDelete Range {
			struct line_descr	*l;
			int	ct;

			ct = 0;
			l = FindLine($2.l_first);
			while (IsValidLine(l) && l->l_no <= $2.l_last) {
				/* KNOWS ABOUT DELETE STRATEGY!!! */
				DeleteLine(l->l_no);
				++ct;
			}
			if (ct == 0) Error("No lines in range.");
			else if (ct == 1) printf("\t1 line deleted.\n");
			else printf("\t%d lines deleted.\n", ct);
		}
	| LineNo { DeleteLine($1); }
	;


Range:	LineNo { $$.l_first = $$.l_last = $1; }
	| LineNo KAddop LineNo { $$.l_first = $1; $$.l_last = $3; }
	| '%' { $$.l_first = 0; $$.l_last = INFINITY; }
	;

RenumberCmd: KRenumber { RenumberLines(ZERO, INFINITY, (long) 1000); }
	| KRenumber Range
	  { RenumberLines($2.l_first, $2.l_last, (long) 1000); }
	| KRenumber Range ',' LineNo
	  { RenumberLines($2.l_first, $2.l_last, $4); }
	;

LineNo: KNumber {
	    if ((int)$1 != $1) Error("You should use integer line numbers.");
		$$ = $1;
		if ($$ >= INFINITY) {
			Error("Line number %d too big.", $$);
			$$ = 10000000;
		}
	}
	;

ListCmd: KList Range { ListLines(stdout, $2.l_first, $2.l_last); }
	| KList {
		if (CurFile[0] != '\0') { printf("%s:\n", CurFile); }
		ListLines(stdout, ZERO, INFINITY);
	}
	;


DumpCmd: KDump Range { ListCode($2.l_first, $2.l_last); }
	| KDump { ListCode(ZERO, INFINITY); }
	;

Stmt:	LetStmt
	| MatStmt
	| IfStmt
	| ForStmt
	| PrintStmt
	| InputStmt
	| DimStmt
	| DataStmt
	| KRestore { $$ = Tree(0, T_RESTORE); }
	| KEnd { $$ = Tree(0, T_END); }
	| KGoto LineNo { $$ = Tree(1, T_GOTO, $2); }
	| KGosub LineNo { $$ = Tree(1, T_GOSUB, $2); }
	| KNext KId { $$ = Tree(1, T_NEXT, $2); }
	| KReturn { $$ = Tree(0, T_RETURN); }
	| KStop { $$ = Tree(0, T_STOP); }
	| KBreak { $$ = Tree(0, T_BREAK); }
	| KRem {
		FlushInput();
		if (Opts['B'])
			printf("'%s'\n", SaveString(FindStart(InputLine)));
		$$ = Tree(1, T_REM, SaveString(FindStart(InputLine)));
	}
	| error {
		FlushInput();
		$$ = Tree(1, T_SYNTAX, SaveString(FindStart(InputLine)));
	}
	;

DimStmt: KDim DimList { $$ = Tree(2, T_DIM, $2.l_head); }
	;

DimList: DimItem {
	    if ($1 == NULL) { $$.l_head = $$.l_tail = NULL; }
	    else {
		$$.l_head = Tree(2, T_DIMLIST, $1, NULL);
		$$.l_tail = $$.l_head;
	    }
	}
	| DimList ',' DimItem {
		if ($3 == NULL) { $$ = $1; }
		else {
			struct tree_node	*t;
			$$ = $1;
			t = Tree(2, T_DIMLIST, $3, NULL);
			if ($1.l_head == NULL) {
				$$.l_head = t;
				$$.l_tail = $$.l_head;
			} else {
				tree1($$.l_tail) = t;
				$$.l_tail = t;
			}
		}
	}
	;

DimItem: DimVar '[' Expr ']' { $$ = Tree(2, T_DIM1, $1, $3); }
	| DimVar '(' Expr ')' { $$ = Tree(2, T_DIM1, $1, $3); }
	| DimVar '[' Expr ',' Expr ']' { $$ = Tree(3, T_DIM2, $1, $3, $5); }
	| DimVar '(' Expr ',' Expr ')' { $$ = Tree(3, T_DIM2, $1, $3, $5); }
	| DimVar {
		if ($1->t_op != T_DIM_STRING) {
			SyntaxError(LexColumn(), "Dimensions missing.");
			$$ = NULL;
		}
		else { $$ = Tree(2, T_DIM0, $1); }
	}
	;

DimVar: KId { $$ = Tree(1, T_DIM_NUM, $1); }
	| KStringId { $$ = Tree(2, T_DIM_STRING, $1, -DEFAULT_STRLEN); }
	| KIntId { $$ = Tree(1, T_DIM_INT, $1); }
	| KStringId KNumber {
		int	i = (int)$2;

		if ((float)i != $2) {
			SyntaxError(LexColumn(),
				"Length of string should be integer.");
		} else if (i > MAX_STRLEN) {
			SyntaxError(LexColumn(), "String length too large.");
			i = MAX_STRLEN;
		} else if (i < 1) {
			SyntaxError(LexColumn(),
				"Length of string should be positive.");
			i = -DEFAULT_STRLEN;
		}
		$$ = Tree(2, T_DIM_STRING, $1, i);
	}
	;

LetStmt: KLet Variable '=' Expr {
		$$ = Tree(2, T_ASSIGN, $2, $4);
		$$ = TypeCheck($$);
	}
	| Variable '=' Expr {
		$$ = Tree(2, T_ASSIGN, $1, $3);
		$$ = TypeCheck($$);
	}
	;


MatStmt: KMat KId '=' KId { $$ = Tree(2, T_MAT_ASSIGN, $2, $4); }
	| KMat KId '=' KId KAddop KId
	  { $$ = Tree(3, MatOpMap($5), $2, $4, $6); }
	| KMat KId '=' KId KMulop KId
	  { $$ = Tree(3, MatOpMap($5), $2, $4, $6); }
	| KMat KId '=' '(' Expr ')' { $$ = Tree(2, T_MAT_SC_ASS, $2, $5); }
	| KMat KId '=' '(' Expr ')' KAddop KId
	  { $$ = Tree(3, MatScOpMap($7), $2, $5, $8); }
	| KMat KId '=' '(' Expr ')' KMulop KId
	  { $$ = Tree(3, MatScOpMap($7), $2, $5, $8); }
	| KMat KId '=' KFunction {
		$$ = Tree(2, T_MAT_FUNCALL0, $2, $4);
		$$ = TypeCheck($$);
	}
	| KMat KId '=' KFunction '(' KId ')' {
		$$ = Tree(3, T_MAT_FUNCALL1, $2, $4, $6);
		$$ = TypeCheck($$);
	}
	| KMat KId '=' KAddop KId {
		if ($4 == T_MINUS) { $$ = Tree(2, T_MAT_UMINUS, $2, $5); }
		else { $$ = Tree(2, T_MAT_ASSIGN, $2, $5); }
	}
	| KMat KId '=' KIdn { $$ = Tree(1, T_MAT_REALIDN, $2); }
	| KMat KId '=' KZer { $$ = Tree(1, T_MAT_ZER, $2); }
	| KMat KId '=' KRnd { $$ = Tree(2, T_MAT_FUNCALL0, $2, T_RND); }
	| KMat KId '=' KInv '(' KId ')' { $$ = Tree(2, T_MAT_INV, $2, $6); }
	| KMat KId '=' KTrn '(' KId ')' { $$ = Tree(2, T_MAT_TRN, $2, $6); }
	;

IfStmt:	KIf Expr KThen ObjStmt KElse ObjStmt {
		$$ = Tree(3, T_IF, $2, $4, $6);
		$$ = TypeCheck($$);
	}
	| KIf Expr KThen ObjStmt {
		$$ = Tree(3, T_IF, $2, $4, NULL);
		$$ = TypeCheck($$);
	}
	| KIf Expr KGoto LineNo {
		$$ = Tree(3, T_IF, $2, Tree(1, T_GOTO, $4), NULL);
		$$ = TypeCheck($$);
	}
	;

ObjStmt:LineNo { $$ = Tree(1, T_GOTO, $1); }
	| Stmt {
		$$ = $1;
		switch ($1->t_op) {
		case T_FOR:
		case T_NEXT:
		case T_DATA:
		case T_DIM:
			Error("Invalid object of if statement.");
				break;
		}
	}
	;

ForStmt: KFor KId '=' Expr KTo Expr KStep Expr {
		$$ = Tree(4, T_FOR, $2, $4, $6, $8);
		$$ = TypeCheck($$);
	}
	| KFor KId '=' Expr KTo Expr {
		$$ = Tree(4, T_FOR, $2, $4, $6, NULL);
		$$ = TypeCheck($$);
	}
	| KFor KIntId '=' Expr KTo Expr KStep Expr {
		$$ = Tree(4, T_FOR_INT, $2, $4, $6, $8);
		$$ = TypeCheck($$);
	}
	| KFor KIntId '=' Expr KTo Expr {
		$$ = Tree(4, T_FOR_INT, $2, $4, $6, NULL);
		$$ = TypeCheck($$);
	}
	;

PrintStmt: KPrint PExprList { $$ = Tree(1, T_PRINT, $2.l_head); }
	| KPrint PExprList Expr {
		struct tree_node	*t;

		t = Tree(3, T_LONG_PRINT, $3, NULL);
		if ($2.l_head == NULL) { $2.l_head = $2.l_tail = t; }
		else { tree1($2.l_tail) = t; }
		$$ = Tree(2, T_PRINT, $2.l_head);
	}
	| KMat KPrint MExprList { $$ = Tree(1, T_MAT_PRINT, $3.l_head); }
	| KMat KPrint MExprList SimpleVar {
		struct tree_node	*t;

		t = Tree(3, T_LONG_PRINT, $4, NULL);
		if ($3.l_head == NULL) { $3.l_head = $3.l_tail = t; }
		else { tree1($3.l_tail) = t; }
		$$ = Tree(2, T_MAT_PRINT, $3.l_head);
  	}
	;

PExprList: /* empty */ { $$.l_head = NULL; $$.l_tail = NULL; }
	| PExprList PExpr {
		$$ = $1;
		if ($$.l_head == NULL) { $$.l_head = $$.l_tail = $2; }
		else { tree1($$.l_tail) = $2; $$.l_tail = $2; }
	}
	;

PExpr: ',' { $$ = Tree(3, T_LONG_PRINT, NULL, NULL); }
	| ';' { $$ = Tree(3, T_SHORT_PRINT, NULL, NULL); }
	| Expr ',' { $$ = Tree(3, T_LONG_PRINT, $1, NULL); }
	| Expr ';' { $$ = Tree(3, T_SHORT_PRINT, $1, NULL); }
	;

MExprList: /* empty */ {
		$$.l_head = NULL;
		$$.l_tail = NULL;
	}
	| MExprList MExpr {
		$$ = $1;
		if ($$.l_head == NULL) { $$.l_head = $$.l_tail = $2; }
		else { tree1($$.l_tail) = $2; $$.l_tail = $2; }
	}
	;

MExpr: ',' { $$ = Tree(3, T_LONG_PRINT, NULL, NULL); }
	| ';' { $$ = Tree(3, T_SHORT_PRINT, NULL, NULL); }
	| SimpleVar ',' { $$ = Tree(3, T_LONG_PRINT, $1, NULL); }
	| SimpleVar ';' { $$ = Tree(3, T_SHORT_PRINT, $1, NULL); }
	;

InputStmt: KInput VarList { $$ = Tree(1, T_INPUT, $2.l_head); }
	| KRead VarList { $$ = Tree(1, T_READ, $2.l_head); }
	| KMat KInput SimpleVarList { $$ = Tree(1, T_MAT_INPUT, $3.l_head); }
	| KMat KRead SimpleVarList { $$ = Tree(1, T_MAT_READ, $3.l_head); }
	;

VarList: Variable {
		$$.l_head = Tree(2, T_VARLIST, $1, NULL);
		$$.l_tail = $$.l_head;
	}
	| VarList ',' Variable {
		struct tree_node	*t;

		$$ = $1;
		t = Tree(2, T_VARLIST, $3, NULL);
		tree1($$.l_tail) = t;
		$$.l_tail = t;
	}
	;

SimpleVarList: SimpleVar {
			$$.l_head = Tree(2, T_VARLIST, $1, NULL);
			$$.l_tail = $$.l_head;
	}
	| SimpleVarList ',' SimpleVar {
		struct tree_node	*t;

		$$ = $1;
		t = Tree(2, T_VARLIST, $3, NULL);
		tree1($$.l_tail) = t;
		$$.l_tail = t;
	}
	;

DataStmt: KData DExprList { $$ = Tree(1, T_DATA, $2.l_head); }
	;

DExprList: Constant {
		$$.l_head = Tree(3, T_DATAEXPRLIST, 1, $1, NULL);
		$$.l_tail = $$.l_head;
	}
	| KNumber KMulop Constant {
		struct tree_node	*t;

		if ($2 != T_TIMES) {
			Error("Invalid data statement.");
			$$.l_head = $$.l_tail = NULL;
		} else {
			t = Tree(3, T_DATAEXPRLIST, (int) $1, $3, NULL);
			$$.l_head = t;
			$$.l_tail = t;
		}
	}
	| DExprList ',' Constant {
		struct tree_node	*t;

		t = Tree(3, T_DATAEXPRLIST, 1, $3, NULL);
		$$ = $1;
		if ($$.l_head == NULL) { $$.l_head = $$.l_tail = t; }
		else {
			tree2($$.l_tail) = t;
			$$.l_tail = t;
		}
	}
	| DExprList ',' KNumber KMulop Constant {
		struct tree_node	*t;

		if ($4 != T_TIMES) {
			Error("Invalid data statement.");
			$$ = $1;
		} else {
			t = Tree(3, T_DATAEXPRLIST, (int) $3, $5, NULL);
			$$ = $1;
			if ($$.l_head == NULL) { $$.l_head = $$.l_tail = t; }
			else {
				tree2($$.l_tail) = t;
				$$.l_tail = t;
			}
		}
	}
	;

Constant: KNumber { $$ = Tree(2, T_REAL, $1); }
	| KString { $$ = Tree(1, T_STRING, $1); }
	;

Variable: SimpleVar
	| SimpleVar '[' Expr ']' { $$ = Tree(2, T_SUB1, $1, $3); }
	| SimpleVar '(' Expr ')' { $$ = Tree(2, T_SUB1, $1, $3); }
	| SimpleVar '[' Expr ',' Expr ']' { $$ = Tree(3, T_SUB2, $1, $3, $5); }
	| SimpleVar '(' Expr ',' Expr ')' { $$ = Tree(3, T_SUB2, $1, $3, $5); }
	;

SimpleVar: KId { $$ = Tree(1, T_REALID, $1); }
	| KStringId { $$ = Tree(1, T_STRINGID, $1); }
	| KIntId { $$ = Tree(1, T_INTID, $1); }
	;

Expr: Constant
	| Variable
	| Expr KAddop Expr { $$ = Tree(2, $2, $1, $3); $$ = TypeCheck($$); }
	| Expr KMulop Expr { $$ = Tree(2, $2, $1, $3); $$ = TypeCheck($$); }
	| Expr KExpop Expr { $$ = Tree(2, $2, $1, $3); $$ = TypeCheck($$); }
	| Expr '=' Expr { $$ = Tree(2, T_EQUAL, $1, $3); $$ = TypeCheck($$); }
	| Expr KRelop Expr { $$ = Tree(2, $2, $1, $3); $$ = TypeCheck($$); }
	| Expr KLogop Expr { $$ = Tree(2, $2, $1, $3); $$ = TypeCheck($$); }
	| KAddop Expr	%prec UMINUS {
		if ($1 == T_MINUS) {
			$$ = Tree(1, T_UMINUS, $2);
			$$ = TypeCheck($$);
		} else { $$ = $2; }
	}
	| KNot Expr { $$ = Tree(1, T_NOT, $2); $$ = TypeCheck($$); }
	| '(' Expr ')' { $$ = Tree(1, T_PARENS, $2); }
	| KLen '(' Expr ')' { $$ = Tree(1, T_LEN, $3); $$ = TypeCheck($$); }
	| KNum '(' Expr ')' = { $$ = Tree(1, T_NUM, $3); $$ = TypeCheck($$); }
	| KChr '(' Expr ')' { $$ = Tree(1, T_CHR, $3); $$ = TypeCheck($$); }
	| KIdx '(' Expr ',' Expr ')'
	  { $$ = Tree(2, T_REALIDX, $3, $5); $$ = TypeCheck($$); }
	| KStr '(' Expr ',' Expr ')'
	  { $$ = Tree(2, T_STR2, $3, $5); $$ = TypeCheck($$); }
	| KStr '(' Expr ',' Expr ',' Expr ')'
	  { $$ = Tree(3, T_STR3, $3, $5, $7); $$ = TypeCheck($$); }
	| KRnd { $$ = Tree(1, T_FUNCALL0, T_RND); }
	| KRnd '(' Expr ')' { $$ = Tree(2, T_FUNCALL1, T_SET_RND, $3); }
	| KDet '(' KId ')' { $$ = Tree(1, T_DET, $3); }
	| KDot '(' KId ',' KId ')' { $$ = Tree(2, T_DOT, $3, $5); }
	| KSum '(' KId ')' { $$ = Tree(1, T_SUM, $3); }
	| KPrd '(' KId ')' { $$ = Tree(1, T_PRD, $3); }
	| KFunction { $$ = Tree(1, T_FUNCALL0, $1); $$ = TypeCheck($$); }
	| KFunction '(' Expr ')'
	  { $$ = Tree(2, T_FUNCALL1, $1, $3); $$ = TypeCheck($$); }
	| KFunction '(' Expr ',' Expr ')'
	  { $$ = Tree(3, T_FUNCALL2, $1, $3, $5); $$ = TypeCheck($$); }
	;
