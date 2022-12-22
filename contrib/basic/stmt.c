# include "structs.h"
# include <signal.h>
# include <ctype.h>



ExecStmt (t)
    reg struct tree_node *t;
{
    double  ld;
    long    lno;
    register enum treeop op;

    if (t == NULL || t->t_op == T_REM)
	return;
    SP = Stack - 1;
 /* was InitTemps() */
    StringTempPtr = StringSpace;
    *StringTempPtr = '\0';
    if (DoubleSpace != NULL)
	FreeDoubleTemp (DoubleSpace);
    if (IntSpace != NULL)
	FreeIntTemp (IntSpace);
    switch (op = t->t_op)
    {
      case T_SYNTAX:
	RunError ("Syntax Error.");
	break;

      case T_ASSIGN:
	ExecAssign (tree0 (t), tree1 (t));
	break;

      case T_STR_ASSIGN:
	ExecStrAssign (tree0 (t), tree1 (t));
	break;

      case T_MAT_ASSIGN:
	MatAssign (t, 0, t, 1);
	break;

      case T_MAT_SC_ASS:
	MatScalarOp (op, t, 0, tree1 (t), t, 0);
	break;

      case T_MAT_SC_ADD:
      case T_MAT_SC_SUB:
      case T_MAT_SC_MUL:
	MatScalarOp (op, t, 0, tree1 (t), t, 2);
	break;

      case T_MAT_INPUT:
      case T_MAT_READ:
	ExecMatInput (op, tree0 (t));
	break;

      case T_MAT_ADD:
      case T_MAT_SUB:
	MatMatOp (op, t, 0, t, 1, t, 2);
	break;

      case T_MAT_MUL:
	MatMul (t, 0, t, 1, t, 2);
	break;

      case T_MAT_FUNCALL0:
	MatFunc0 (op1 (t), t, 0);
	break;

      case T_MAT_FUNCALL1:
	MatFunc1 (op1 (t), t, 0, t, 2);
	break;

      case T_MAT_FUNCALL2:
	MatFunc2 (op1 (t), t, 0, t, 2, t, 3);
	break;

      case T_MAT_REALIDN:
	MatIdentity (t, 0);
	break;

      case T_MAT_ZER:
	MatZero (t, 0);
	break;

      case T_MAT_TRN:
	MatTranspose (t, 0, t, 1);
	break;

      case T_MAT_INV:
	MatInverse (t, 0, t, 1);
	break;

      case T_DATA:
	break;

      case T_DIM:
	ExecDim (tree0 (t));
	break;

      case T_FOR:
      case T_FOR_INT:
	ExecFor (t->t_op, t, 0, tree1 (t), tree2 (t), tree3 (t));
	break;

      case T_GOSUB:
/*YYY fix this with a ever-so-slightly more complex gosub stack */
	if (++GP >= GosubStack + GOSUB_DEPTH)
	{
	    RunError ("Gosub stack overflow: more than %d gosubs.",
		GOSUB_DEPTH);
	}
	*GP = PC;
	PC = lineno0 (t);
	if (t->t_version[0] == MasterSymbolVersion)
	    PCP = (struct line_descr *)t->t_symbols[0];
	else
	{
	    struct line_descr *l = FindLine(PC);
	    if (l->l_no != PC)
		RunError("Line number %d doesn't exist.", PC);
	    t->t_symbols[0] = (struct name *)(PCP = l);
	    t->t_version[0] = MasterSymbolVersion;
	}
	break;

      case T_GOTO:
	PC = lineno0 (t);
	if (t->t_version[0] == MasterSymbolVersion)
	    PCP = (struct line_descr *)t->t_symbols[0];
	else
	{
	    struct line_descr *l = FindLine(PC);
	    if (l->l_no != PC)
		RunError("Line number %d doesn't exist.", PC);
	    t->t_symbols[0] = (struct name *)(PCP = l);
	    t->t_version[0] = MasterSymbolVersion;
	}
	break;

      case T_IF:
	ExecTree (tree0 (t));
	ld = PopReal ();
	if (ld != 0.0)
	    ExecStmt (tree1 (t));
	else
	    ExecStmt (tree2 (t));
	break;

      case T_INPUT:
      case T_READ:
	ExecInput (op, tree0 (t));
	break;

      case T_RESTORE:
	DataPC = 0;
	NextData = (struct tree_node *) NULL;
	AdvanceData ();
	DataCount = 0;
	break;

      case T_NEXT:
	ExecNext (string0 (t));
	break;

      case T_PRINT:
	ExecPrint (tree0 (t));
	break;

      case T_MAT_PRINT:
	ExecMatPrint (tree0 (t));
	break;

      case T_RETURN:
	if (GP < GosubStack)
	{
	    RunError ("Gosub stack underflow: too many RETURNs");
	}
	PC = *GP--;
	PCP = FindLine (PC);
	break;

      case T_STOP:
      case T_END:
	Stop = 1;
	break;

      case T_BREAK:
	Break = 1;
	break;

      default:
	printf ("You forgot stmt %s (%d)\n", OpName (op), op);
	break;
    }
}

ExecAssign (lhs, rhs)
    reg struct tree_node *lhs, *rhs;
{
    reg struct value *lv, *rv;

    LValue (lhs);
    ExecTree (rhs);
    rv = PopValue ();
    if (rv->v_kind != V_REAL)
	RunError ("Need numeric quantity on right-hand side.");
    lv = PopValue ();
    switch (lv->v_kind)
    {
      case V_REAL_VAR:
	*(lv->v_realvar) = rv->v_real;
	break;
      case V_INT_VAR:
	*(lv->v_intvar) = (int) rv->v_real;
	break;
      default:
	RunError ("Can't assign to %s", ValueName (lv->v_kind));
	break;
    }
}

ExecFor (kind, t, argno, init, lim, step)
    enum treeop kind;
    reg struct tree_node *init, *lim, *step, *t;
{
    double  initval, limval, stepval;
    struct for_context *fp;
    char   *name;

    name = stringn (t, argno);
    for (fp = FP; fp >= ForStack && fp->f_var->s_name != name; --fp)
	;
    if (fp >= ForStack)
    {
    /* Pop (presumably) inactive for loops */
	FP = fp - 1;
    }
    ExecTree (init);
    if (kind == T_FOR_INT)
	initval = PopInt ();
    else
	initval = PopReal ();
    ExecTree (lim);
    if (kind == T_FOR_INT)
	limval = PopInt ();
    else
	limval = PopReal ();
    if (step != NULL)
    {
	ExecTree (step);
	if (kind == T_FOR_INT)
	    stepval = PopInt ();
	else
	    stepval = PopReal ();
    }
    else
	stepval = 1;

    if (++FP >= ForStack + FOR_DEPTH)
	RunError ("For stack overflow: too many nested for-statements.");
    FP->f_line = PC;
    FP->f_linep = PCP;
    FP->f_var = FindSymbol (t, argno);
    if (FP->f_var == NULL)
    {
	FP->f_var = AllocSymbol (name);
	SetSymbol (t, argno, FP->f_var);	/* note it */
    }
    FP->f_var->s_val.v_kind = V_REAL_VAR;
    FP->f_var->s_val.v_real = initval - stepval;
    FP->f_limit = limval;
    FP->f_step = stepval;
    SkipToNext (t, argno);
}

ExecNext (name)
    reg char *name;
{
    reg struct name *n;
    reg struct for_context *fp;
    register char *p;

    if (FP < ForStack)
	return;
    p = FP->f_var->s_name;
    if (!(
	    (p[0] == name[0] && p[1] == 0 && name[1] == 0) || strcmp (p, name) == 0
	))
    {
    /* Have we popped ourselves out of a loop? */
	fp = FP;
	while (--fp >= ForStack &&
	    ((p = fp->f_var->s_name)[0] != name[0] || strcmp (p, name) != 0))
	    ;
	if (fp < ForStack)
	    return;		/* No, just ignore this */
#ifdef DEBUG
	if (Opts['F'])
	{
	    printf ("\tpopping %d to %d %s\n",
		FP - ForStack, fp - ForStack, fp->f_var->s_name);
	}
#endif
	FP = fp;		/* Yes, let's mark the new place */
    }
    n = FP->f_var;
    if (n == NULL)
	RunError ("For variable %s not initialized.  Basic bug!", name);
    if (n->s_val.v_kind != V_REAL_VAR)
	RunError ("For variable %s not a numeric variable.", name);
    n->s_val.v_real += FP->f_step;
    if (FP->f_step < 0)
    {
	if (FP->f_limit > n->s_val.v_real)
	{
	    --FP;
	}
	else
	{
	    PC = FP->f_line;
	    PCP = FP->f_linep;
	}
    }
    else if (FP->f_step > 0)
    {
	if (FP->f_limit < n->s_val.v_real)
	{
	    --FP;
	}
	else
	{
	    PC = FP->f_line;
	    PCP = FP->f_linep;
	}
    }
    else
    {
	RunError ("Step on for-statement is zero.");
    }
}

ExecInput (kind, t)
    enum treeop kind;
    reg struct tree_node *t;
{
    static struct value *v;

    for (; t != NULL; t = tree1 (t))
    {
	if (tree0 (t) == NULL)
	    continue;
	LValue (tree0 (t));
	if (kind == T_INPUT)
	    v = InputValue ();
	else
	    v = ReadValue ();
	if (v != NULL)
	    InputAssign (tree0 (t), PopValue (), v);
	else
	    PopValue ();
    }
}

ExecPrint (t)
    reg struct tree_node *t;
{
    enum treeop prev = T_LONG_PRINT;

    for (; t != NULL; t = tree1 (t))
    {
	if (tree0 (t) == NULL)
	    continue;
	prev = t->t_op;
	ExecTree (tree0 (t));
	PrintValue (PopValue (),
	    prev == T_LONG_PRINT && tree1 (t) != NULL ? 1 : 0);
    }
    if (prev == T_LONG_PRINT )
    {
	printf ("\n");
	OutputCol = 0;
    }
}

long    PC = 0;
long   *GP = GosubStack - 1;
struct for_context *FP = ForStack - 1;

ExecProgram ()
{
    if (LineCount () < 1)
    {
	Error ("No lines to run!");
	return;
    }
    InitProgram ();
    Continue ();
}

ExecStrAssign (lhs, rhs)
    reg struct tree_node *lhs, *rhs;
{
    reg struct value *lv, *rv;

    LValue (lhs);
    ExecTree (rhs);
    rv = PopValue ();
    lv = PopValue ();
    switch (lv->v_kind)
    {
      case V_STRING_VAR:
	if (rv->v_kind != V_STRING)
	    RunError ("Need string quantity on right-hand side.");
    /***lv->v_stringvar = SaveString(rv->v_string);**/
	(void) strncpy (lv->v_stringvar.s_p, rv->v_string,
	    lv->v_stringvar.s_strlen);
	break;

      default:
	RunError ("Can't assign to %s", ValueName (lv->v_kind));
	break;
    }
}

/*ARGSUSED*/
InputAssign (t, var, val)
    struct tree_node *t;
    struct value *var, *val;
{
    switch (var->v_kind)
    {
      case V_STRING_VAR:
	if (val->v_kind != V_STRING)
	{
	    RunError ("Read %s when expecting string.",
		ValueName (val->v_kind));
	}
	(void) strncpy (var->v_stringvar.s_p, val->v_string,
	    var->v_stringvar.s_strlen);
	break;
      case V_REAL_VAR:
	if (val->v_kind != V_REAL)
	{
	    RunError ("Read %s when expecting number.",
		ValueName (val->v_kind));
	}
	*(var->v_realvar) = val->v_real;
	break;
      case V_INT_VAR:
	if (val->v_kind != V_REAL)
	{
	    RunError ("Read %s when expecting number.",
		ValueName (val->v_kind));
	}
	*(var->v_intvar) = (int) val->v_real;
	break;
    }
}


struct value *
InputValue ()
{
    static struct value iv;

    for (;;)
    {
	SkipBlanks (IPtr);
	while (*IPtr == '\0')
	{
	    printf ("? ");
	    FlushOutput ();
	    if (fgets (InputLine, LINE_LEN, stdin) == NULL)
	    {
		clearerr (stdin);
		RunError ("Ran out of input!");
		return NULL;
	    }
	    OutputCol = 0;
	    InitLexicon ();
	    SkipBlanks (IPtr);
	    SaveStrings = 0;
	}

	FlushOutput ();
	switch (*IPtr)
	{

	  case ',':
	    ++IPtr;
	    break;

	  case '-':
	  case '+':
	  case '0':
	  case '1':
	  case '2':
	  case '3':
	  case '4':
	  case '5':
	  case '6':
	  case '7':
	  case '8':
	  case '9':
	    (void) NumberLex ();
	    iv.v_kind = V_REAL;
	    iv.v_real = yylval.n_val;
	    return &iv;

	  case '\'':
	  case '"':
	    (void) StringLex ();
	    iv.v_kind = V_STRING;
	    iv.v_string = yylval.s_val;
	    return &iv;

	  default:
	    ++IPtr;
	    RunError ("Invalid input, near '%.12s'", IPtr - 1);
	}
    }
 /* NOTREACHED */
}

Print (s)
    reg char *s;
{
    reg     ch;

    while (ch = *s++)
    {
	if (ch == '\b')
	{
	    putchar ('\b');
	    --OutputCol;
	}
	else if (ch == '\f' || ch == '\n' || ch == '\r')
	{
	    putchar (ch);
	    OutputCol = 0;
	}
	else if (ch == '\t')
	{
	    putchar (ch);
	    OutputCol += 8;
	    OutputCol &= ~7;
	}
	else if (iscntrl (ch))
	{
	    putchar (ch);
	}
	else if (!isascii (ch))
	{
	}
	else
	{
	    putchar (ch);
	    ++OutputCol;
	}
    }
}

# define FWIDTH	14

int     OutputCol = 0;

PrintValue (v, long_form)
    reg struct value *v;
{
    reg int wid;
    char    str[300];

    switch (v->v_kind)
    {
      case V_REAL:
	if (long_form)
	{
	    (void) sprintf (str, "%*g ", -FWIDTH + 1, v->v_real);
	}
	else
	{
	    (void) sprintf (str, "%g", v->v_real);
	}
	break;

      case V_STRING:
	if (long_form)
	{
	    wid = (strlen (v->v_string) + FWIDTH - 1) / FWIDTH;
	    (void) sprintf (str, "%*s ", -wid * FWIDTH + 1, v->v_string);
	}
	else
	{
	    (void) strcpy (str, v->v_string);
	}
	break;

      default:
	RunError ("Can't PrintValue %s.", ValueName (v->v_kind));
	break;
    }
    wid = strlen (str);
    if (wid + OutputCol >= screenwidth)
    {
	NewLine ();
	Print (str);
    }
    else
    {
	Print (str);
    }
}

struct value *
ReadValue ()
{
    static struct value v;
    struct tree_node *u;

    if (NextData == NULL || DataCount >= int0 (NextData))
    {
	AdvanceData ();
    }
    if (NextData == NULL)
    {
	RunError ("No more data to read.");
    }
    assert (DataCount < int0 (NextData));
    ++DataCount;
    u = tree1 (NextData);
    switch (u->t_op)
    {
      case T_REAL:
	v.v_kind = V_REAL;
	v.v_real = number0 (u);
	break;
      case T_STRING:
	v.v_kind = V_STRING;
	v.v_string = string0 (u);
	break;
      default:
	RunError ("Basic Bug!  Bad op %s in data list.", OpName (u->t_op));
    }
    return &v;
}

SkipToNext (tin, argno)
    struct tree_node *tin;
{
    reg struct line_descr *l;
    reg struct tree_node *t;
    register char *p;
    char   *name = stringn (tin, argno);

    for (l = FindLine (PC); IsValidLine (l); l = LineAfter (l))
    {
	PC = l->l_no;
	PCP = l;
	if (l->l_tree == NULL)
	    continue;
	if (l->l_tree->t_op == T_NEXT)
	{
	    p = string0 (l->l_tree);
	    if (p[0] != name[0])
		continue;
	    if (p[1] == 0 && name[1] == 0)
		return;
	    if (strcmp (p, name) == 0)
		return;
	}
    }
    RunError ("For-loop for '%s' has no corresponding 'next %s' statement.",
	name, name);
}

AdvanceData ()
{
    struct line_descr *ln;

    DataCount = 0;
    if (NextData != NULL)
	NextData = tree2 (NextData);
    ln = FindLine (DataPC);
    if (NextData != NULL || !IsValidLine (ln))
	return;
    while (IsValidLine (ln))
    {
	if (ln->l_tree && ln->l_tree->t_op == T_DATA)
	{
	    DataPC = LineAfter (ln)->l_no;
	    NextData = tree0 (ln->l_tree);
	    return;
	}
	ln = LineAfter (ln);
    }
    if (IsValidLine (ln))
	DataPC = LineAfter (ln)->l_no;
    else
	DataPC = ln->l_no;
    return;
}
