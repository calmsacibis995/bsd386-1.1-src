# include "structs.h"
# include <math.h>
# include <errno.h>
# include <sys/types.h>
# include <time.h>
# include <sys/times.h>

#ifdef DEBUG

#define PushReal(realv) { \
    double vxx = realv; \
    if (++SP >= Stack + EXPR_DEPTH) \
	RunError ("Stack overflow (PushReal).  Basic bug!"); \
    if (Opts['S']) { printf ("push %g (PushReal)\n", SP->v_real); } \
    SP->v_kind = V_REAL;
    SP->v_real = vxx; \
}

#define PushInt(realv) { \
    double vxx = realv; \
    if (++SP >= Stack + EXPR_DEPTH) \
	RunError ("Stack overflow (PushInt).  Basic bug!"); \
    SP->v_kind = V_REAL; \
    SP->v_real = vxx; \
    if (Opts['S']) { printf ("push %d (PushInt)\n", SP->v_real); } \
}

#define PushString(realv) { \
    char *vxx = realv; \
    if (++SP >= Stack + EXPR_DEPTH) \
	RunError ("Stack overflow (PushString).  Basic bug!"); \
    SP->v_kind = V_STRING; SP->v_string = vxx; \
    if (Opts['S']) { printf ("push %s (PushString)\n", SP->v_string); } \
}


#define PushValue(realv) { \
    struct value *vxx = realv; \
    if (++SP >= Stack + EXPR_DEPTH) \
	RunError ("Stack overflow (PushValue).  Basic bug!"); \
    if (Opts['S']) { printf ("SP == %d (PushValue)\n", SP + 1); } \
    *SP = *vxx; \
}

#else

#define PushReal(realv) { \
    double vxx = realv; \
    if (++SP >= Stack + EXPR_DEPTH) \
	RunError ("Stack overflow (PushReal).  Basic bug!"); \
    SP->v_kind = V_REAL; SP->v_real = vxx; \
}

#define PushInt(realv) { \
    double vxx = realv; \
    if (++SP >= Stack + EXPR_DEPTH) \
	RunError ("Stack overflow (PushInt).  Basic bug!"); \
    SP->v_kind = V_REAL; SP->v_real = vxx; \
}

#define PushString(realv) { \
    char *vxx = realv; \
    if (++SP >= Stack + EXPR_DEPTH) \
	RunError ("Stack overflow (PushString).  Basic bug!"); \
    SP->v_kind = V_STRING; SP->v_string = vxx; \
}

#define PushValue(realv) { \
    struct value *vxx = realv; \
    if (++SP >= Stack + EXPR_DEPTH) \
	RunError ("Stack overflow (PushValue).  Basic bug!"); \
    *SP = *vxx; \
}
#endif

ExecTree (t)
    reg struct tree_node *t;
{
    reg struct name *n;
    reg struct value *lv;
    register double ld, rd;
    double  atof (), MatDet (), MatPrdSum (), MatDot ();
    char   *strtmp, *ls, *rs, *ts, *index ();
    enum treeop op;
    int     li, ri;
    extern int errno;

    if (t == NULL)
    {
	PushReal (1.0);
	return;
    }
    op = t->t_op;
    switch (op)
    {
      case T_DIV:
	ExecTree (tree0 (t));
	ExecTree (tree1 (t));
	rd = PopReal ();
	ld = PopReal ();
	if (rd == 0.0)
	    RunError ("Attempt to divide by zero.");
	PushReal (ld / rd);
	break;
      case T_MINUS:
	ExecTree (tree0 (t));
	ExecTree (tree1 (t));
	rd = PopReal ();
	ld = PopReal ();
	PushReal (ld - rd);
	break;
      case T_PLUS:
	ExecTree (tree0 (t));
	ExecTree (tree1 (t));
	rd = PopReal ();
	ld = PopReal ();
	PushReal (ld + rd);
	break;
      case T_TIMES:
	ExecTree (tree0 (t));
	ExecTree (tree1 (t));
	rd = PopReal ();
	ld = PopReal ();
	PushReal (ld * rd);
	break;
      case T_POWER:
	ExecTree (tree0 (t));
	ExecTree (tree1 (t));
	rd = PopReal ();
	ld = PopReal ();
	errno = 0;
	PushReal (pow (ld, rd));
	if (errno != 0)
		RunError ("Invalid exponentiation: %g^%g.", ld, rd);
	break;

      case T_EQUAL:
	ExecTree (tree0 (t));
	ExecTree (tree1 (t));
	rd = PopReal ();
	ld = PopReal ();
	PushReal ((double)(rd == ld));
	break;
      case T_GEQ:
	ExecTree (tree0 (t));
	ExecTree (tree1 (t));
	rd = PopReal ();
	ld = PopReal ();
	PushReal ((double)(ld >= rd));
	break;
      case T_GT:
	ExecTree (tree0 (t));
	ExecTree (tree1 (t));
	rd = PopReal ();
	ld = PopReal ();
	PushReal ((double)(ld > rd));
	break;
      case T_LEQ:
	ExecTree (tree0 (t));
	ExecTree (tree1 (t));
	rd = PopReal ();
	ld = PopReal ();
	PushReal ((double)(ld <= rd));
	break;
      case T_LT:
	ExecTree (tree0 (t));
	ExecTree (tree1 (t));
	rd = PopReal ();
	ld = PopReal ();
	PushReal ((double)(ld<rd));
	break;
      case T_NEQ:
	ExecTree (tree0 (t));
	ExecTree (tree1 (t));
	rd = PopReal ();
	ld = PopReal ();
	PushReal ((double)(ld!=rd));
	break;

      case T_STR_EQUAL:
      case T_STR_GEQ:
      case T_STR_GT:
      case T_STR_LEQ:
      case T_STR_LT:
      case T_STR_NEQ:
	ExecTree (tree0 (t));
	ExecTree (tree1 (t));
	rs = PopString ();
	ls = PopString ();
	ld = strcmp (ls, rs);
	switch (op)
	{
	  case T_STR_EQUAL:
	    ld = ld == 0;
	    break;
	  case T_STR_GEQ:
	    ld = ld >= 0;
	    break;
	  case T_STR_GT:
	    ld = ld > 0;
	    break;
	  case T_STR_LEQ:
	    ld = ld <= 0;
	    break;
	  case T_STR_LT:
	    ld = ld < 0;
	    break;
	  case T_STR_NEQ:
	    ld = ld != 0;
	    break;
	}
	PushReal (ld);
	break;

      case T_OR:
	ExecTree (tree0 (t));
	ld = PopReal ();
	if (ld != 0.0)
	{
	    PushReal (ld);
	}
	else
	{
	    ExecTree (tree1 (t));
	    ld = PopReal ();
	    PushReal (ld);
	}
	break;

      case T_AND:
	ExecTree (tree0 (t));
	ld = PopReal ();
	if (ld == 0.0)
	{
	    PushReal (0.0);
	}
	else
	{
	    ExecTree (tree1 (t));
	    ld = PopReal ();
	    PushReal (ld);
	}
	break;

      case T_CONCAT:
	ExecTree (tree0 (t));
	ExecTree (tree1 (t));
	ts = PopString ();
	strtmp = StringTemp ();
	(void)strncpy (strtmp, PopString (), MAX_STRLEN);
	(void)strncat (strtmp, ts, MAX_STRLEN);
	strtmp[MAX_STRLEN] = '\0';
	PushString (strtmp);
	break;

      case T_REALID:
      case T_INTID:
      case T_STRINGID:
	n = FindSymbol (t,0);
	if (n == NULL)
	    RunError ("Value of %s undefined.", string0(t));
	if (op == T_REALID) { PushReal (n->s_val.v_real); }
	else if (op == T_INTID) { PushInt (n->s_val.v_int);}
	else { PushString (n->s_val.v_string); }
	break;

      case T_REAL:
	PushReal (number0(t));
	break;

      case T_STRING:
	PushString (string0(t));
	break;

      case T_SUB1:
      case T_SUB2:
	LValue (t);
	lv = PopValue ();
	if (lv->v_kind == V_REAL_VAR)
	{
	    lv->v_kind = V_REAL;
	    lv->v_real = *(lv->v_realvar);
	}
	else if (lv->v_kind == V_INT_VAR)
	{
	    lv->v_kind = V_REAL;
	    lv->v_real = *(lv->v_intvar);
	}
	else if (lv->v_kind == V_STRING_VAR)
	{
	    lv->v_kind = V_STRING;
	/**lv->v_string = *(lv->v_stringvar);**/
	}
	else
	{
	    RunError ("Bad value: %s.", ValueName (lv->v_kind));
	}
	PushValue (lv);
	break;

      case T_UMINUS:
	ExecTree (tree0 (t));
	ld = PopReal ();
	PushReal (-ld);
	break;
      case T_NOT:
	ExecTree (tree0 (t));
	ld = PopReal ();
	PushReal ((float) (ld != 0.0));
	break;

      case T_PARENS:
	ExecTree (tree0 (t));
	break;

      case T_FUNCALL0:
	op = op0 (t);
	PushReal (ApplyFunc0 (op));
	break;

      case T_FUNCALL1:
	op = op0 (t);
	ExecTree (tree1 (t));
	ld = PopReal ();
	PushReal (ApplyFunc1 (op, ld));
	break;

      case T_FUNCALL2:
	op = op0 (t);
	ExecTree (tree1 (t));
	ld = PopReal ();
	ExecTree (tree2 (t));
	rd = PopReal ();
	PushReal (ApplyFunc2 (op, ld, rd));
	break;

      case T_DET:
	PushReal (MatDet (t, 0));
	break;

      case T_CHR:
	ExecTree (tree0 (t));
	strtmp = StringTemp ();
	(void)sprintf (strtmp, "%g", PopReal ());
	PushString (strtmp);
	break;
      case T_NUM:
	ExecTree (tree0 (t));
	PushReal (atof (PopString ()));
	break;
      case T_LEN:
	ExecTree (tree0 (t));
	PushReal ((float) strlen (PopString ()));
	break;

      case T_PRD:
	PushReal (MatPrdSum (op, t, 0));
	break;

      case T_SUM:
	PushReal (MatPrdSum (op, t, 0));
	break;

      case T_DOT:
	PushReal (MatDot (t, 0, t, 1));
	break;

      case T_REALIDX:
	ExecTree (tree0 (t));
	ls = PopString ();
	ExecTree (tree1 (t));
	rs = PopString ();
	ts = index (ls, rs[0]);
	if (ts == NULL)
	{
	    PushReal (0.0);
	}
	else
	{
	    PushReal ((float) (ts - ls + 1));
	}
	break;
      case T_STR2:
	ExecTree (tree0 (t));
	ls = PopString ();
	ExecTree (tree1 (t));
	li = PopReal ();
	strtmp = StringTemp ();
	(void)strcpy (strtmp, ls + li - 1);
	PushString (strtmp);
	break;
      case T_STR3:
	ExecTree (tree0 (t));
	ls = PopString ();
	ExecTree (tree1 (t));
	li = PopReal ();
	ExecTree (tree2 (t));
	ri = PopReal ();
	if (ri < 0)
	    ri = 0;
	else if (ri >= MAX_STRLEN)
	    ri = MAX_STRLEN;
	if (li <= 0)
	    li = 1;
	else if (li > strlen (ls))
	    li = strlen (ls);
	strtmp = StringTemp ();
	(void)strncpy (strtmp, ls + li - 1, ri);
	strtmp[ri] = '\0';
	PushString (strtmp);
	break;
      default:
	printf ("bad ExecTree op %d\n", op);
    /**printf("You forgot expr %s (%d)\n", OpName(op), op);**/
	abort ();
	break;
    }
}


double 
ApplyFunc0 (op)
    enum treeop op;
{
    double  v;
    struct tm *tb, *localtime ();
    struct tms tmsb;
    time_t  clock, time ();

    switch (op)
    {
      case T_RND:
	v = rand () / (float) 0x7fffffff;
	break;
      case T_TIM:
	clock = time ((time_t *) 0);
	tb = localtime (&clock);
	v = tb->tm_sec + tb->tm_min * 60 + tb->tm_hour * 3600;
	break;
      case T_CPU:
	(void)times (&tmsb);
	v = (tmsb.tms_utime + tmsb.tms_stime) / 60.0;
	break;
      default:
	Error ("Basic Bug!  Forgot op %d (%s) in Func0().",
	    op, OpName (op));
	v = 1.0;
    }
    return v;
}

double 
ApplyFunc1 (op, v)
    enum treeop op;
    double  v;
{
    extern int errno;

    errno = 0;
    switch (op)
    {
      case T_SIN:
	v = sin (v);
	break;
      case T_COS:
	v = cos (v);
	break;
      case T_TAN:
	v = tan (v);
	break;
      case T_SQR:
	v = sqrt (v);
	break;
      case T_LOG:
	v = log (v);
	break;
      case T_EXP:
	v = exp (v);
	break;
      case T_ACS:
	v = acos (v);
	break;
      case T_ASN:
	v = asin (v);
	break;
      case T_ATN:
	v = atan (v);
	break;
      case T_COT:
	v = 1.0 / atan (v);
	break;
      case T_CSC:
	v = 1.0 / sin (v);
	break;
      case T_HCS:
	v = cosh (v);
	break;
      case T_HSN:
	v = sinh (v);
	break;
      case T_HTN:
	v = tanh (v);
	break;
      case T_LGT:
	v = log10 (v);
	break;
      case T_LTW:
	v = log (v / log (2.0));
	break;
      case T_SEC:
	v = 1.0 / cos (v);
	break;
      case T_SET_RND:
	(void)srand ((int) v);
	v = rand () / (float) 0x7fffffff;
	break;
      case T_RAD:
	v = v / 180.0 * 3.1415926;
	break;
      case T_DEG:
	v = v / 3.1415926 * 180.0;
	break;
      case T_SGN:
	if (v < 0)
	    v = -1;
	else if (v > 0)
	    v = 1;
	break;
      case T_INT:
	v = (int) v;
	break;
      case T_UMINUS:
	v = -v;
	break;
      default:
	Error ("Basic Bug!  Function %d (%s) missing in ApplyFunc1.",
	    op, OpName (op));
	v = 0.0;
	break;
    }
    if (errno == ERANGE || errno == EDOM)
    {
	RunError ("Bad argument to '%s'.", OpName (op));
    }
    return v;
}

/*ARGSUSED*/
double 
ApplyFunc2 (op, lv, rv)
    enum treeop op;
    double  lv, rv;
{
    double  v;

    switch (op)
    {
      case T_MAX:
	if (lv >= rv)
	    v = lv;
	else
	    v = rv;
	break;

      case T_MIN:
	if (lv <= rv)
	    v = lv;
	else
	    v = rv;
	break;

      default:
	Error ("Basic Bug!  Forgot op %d (%s) in Func2().",
	    op, OpName (op));
	v = 1.0;
	break;
    }
    return v;
}

LValue (t)
    reg struct tree_node *t;
{
    struct value v;
    reg struct name *n;
    char   *ts;
    int     li, ri, d2;

    switch (t->t_op)
    {
      case T_REALID:
	n = FindSymbol (t,0);
	if (n == NULL)
	{
	    n = AllocSymbol (string0(t));
	    SetSymbol(t,0,n);		/* note it */
	    n->s_val.v_kind = V_REAL_VAR;
	    n->s_val.v_real = 0.0;
	}
	v.v_kind = n->s_val.v_kind;
	v.v_realvar = &(n->s_val.v_real);
	PushValue (&v);
	break;
      case T_INTID:
	n = FindSymbol (t,0);
	if (n == NULL)
	{
	    n = AllocSymbol (string0 (t));
	    SetSymbol(t,0,n);		/* note it */
	    n->s_val.v_kind = V_INT_VAR;
	    n->s_val.v_int = 0;
	}
	v = n->s_val;
	v.v_intvar = &(n->s_val.v_int);
	PushValue (&v);
	break;
      case T_STRINGID:
	n = FindSymbol (t,0);
	if (n == NULL)
	{
	    n = AllocSymbol (stringn(t,0));
	    SetSymbol(t,0,n);		/* note it */
	    n->s_val.v_kind = V_STRING_VAR;
	    ts = malloc (DEFAULT_STRLEN + 1);
	    n->s_val.v_stringvar.s_p = ts;
	    ts[0] = ts[DEFAULT_STRLEN] = '\0';
	    n->s_val.v_stringvar.s_strlen = DEFAULT_STRLEN;
	    
	}
	v.v_kind = n->s_val.v_kind;
	v.v_stringvar = n->s_val.v_stringvar;
	PushValue (&v);
	break;

      case T_SUB1:
	n = FindSymbol (tree0(t),0);
	if (n == NULL)
	    RunError ("Array '%s' not dimensioned!", string0(tree0(t)));
	ExecTree (tree1 (t));
	li = (int) PopReal ();

	switch (n->s_val.v_kind)
	{
	  case V_2D_INT:
	  case V_2D_REAL:
	  case V_2D_STRING:
	    RunError ("Wrong number of subscripts to array '%s'",
							string0(tree0(t)));
/* I'm a bit confused as to where V_1D_INT comes in! */
	  case V_1D_REAL:
	    if (li < 1 || li > n->s_val.v_rmat.m_dim1)
		RunError ("Subscript 1 of '%s' out of range: %d",
						string0(tree0(t)), li);
	    v.v_kind = V_REAL_VAR;
/*************************/
	    d2 = n->s_val.v_rmat.m_dim2;
	    if (d2 != 1){
		printf("ERROR:  li=%d  d2=%d\n", li, d2);
		RunError("expr.c, line 660, d2 != 1 ???");
	    }
/*************************/
/*
	    d2 = n->s_val.v_rmat.m_dim2;
	    v.v_realvar = n->s_val.v_rmat.m_area + ((li - 1) * d2);
*/
	    v.v_realvar = n->s_val.v_rmat.m_area + li - 1;
	    PushValue (&v);
	    break;

	  case V_1D_STRING:
	    if (li < 1 || li > n->s_val.v_smat.m_dim1)
		RunError ("Subscript 1 of '%s' out of range: %d",
						string0(tree0(t)), li);
	    v.v_kind = V_STRING_VAR;
/**YYY d2 is probably 1 */
	    d2 = n->s_val.v_smat.m_dim2;
	    v.v_stringvar.s_p =
		n->s_val.v_smat.m_area +
		(n->s_val.v_smat.m_strlen + 1) * ((li - 1) * d2);
	    v.v_stringvar.s_strlen = n->s_val.v_smat.m_strlen;
	    PushValue (&v);
	    break;

	  default:
	    RunError ("Can't subscript %s yet.", OpName (t->t_op));
	}
	break;

      case T_SUB2:
	ts = string0 (tree0 (t));
	n = FindSymbol (tree0(t),0);
	if (n == NULL)
	    RunError ("Array '%s' not dimensioned!", ts);
	ExecTree (tree1 (t));
	li = (int) PopReal ();
	ExecTree (tree2 (t));
	ri = (int) PopReal ();

	switch (n->s_val.v_kind)
	{
	  case V_1D_REAL:
	  case V_1D_INT:
	  case V_1D_STRING:
	    RunError ("Wrong number of subscripts to array '%s'", ts);

	  case V_2D_REAL:
	    if (ri < 1 || ri > n->s_val.v_rmat.m_dim2)
		RunError ("Subscript 2 of '%s' out of range: %d", ts, ri);
	    if (li < 1 || li > n->s_val.v_rmat.m_dim1)
		RunError ("Subscript 1 of '%s' out of range: %d", ts, li);
	    v.v_kind = V_REAL_VAR;
	    d2 = n->s_val.v_rmat.m_dim2;
	    v.v_realvar =
		n->s_val.v_rmat.m_area + ((li - 1) * d2 + ri - 1);
	    PushValue (&v);
	    break;

	  case V_2D_STRING:
	    if (ri < 1 || ri > n->s_val.v_smat.m_dim2)
		RunError ("Subscript 2 of '%s' out of range: %d", ts, ri);
	    if (li < 1 || li > n->s_val.v_smat.m_dim1)
		RunError ("Subscript 1 of '%s' out of range: %d", ts, li);
	    v.v_kind = V_STRING_VAR;
	    d2 = n->s_val.v_smat.m_dim2;
	    v.v_stringvar.s_p = n->s_val.v_smat.m_area +
		(n->s_val.v_smat.m_strlen + 1) * ((li - 1) * d2 + ri - 1);
	    v.v_stringvar.s_strlen = n->s_val.v_smat.m_strlen;
	    PushValue (&v);
	    break;

	  default:
	    RunError ("Can't subscript %s yet.", OpName (t->t_op));
	}
	break;

      default:
	RunError ("Can't do %s yet.", OpName (t->t_op));
	break;
    }
}

char   *
ValueName (v)
    enum valkind v;
{
    char    val[10];

    switch (v)
    {
      case V_NULL:
	return "(null)";
      case V_REAL:
	return "number";
      case V_INT:
	return "int";
      case V_STRING:
	return "string";
      case V_BOOLEAN:
	return "boolean";
      case V_STRING_VAR:
	return "string variable";
      case V_INT_VAR:
	return "integer variable";
      case V_REAL_VAR:
	return "real variable";
      case V_1D_REAL:
	return "one-dimensional real matrix";
      case V_2D_REAL:
	return "two-dimensional real matrix";
      case V_1D_INT:
	return "one-dimensional integer matrix";
      case V_2D_INT:
	return "two-dimensional integer matrix";
      default:
	(void)sprintf (val, "(%d value)", v);
	return val;
    }
}
