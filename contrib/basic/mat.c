# include "structs.h"
# include <signal.h>
# include <math.h>
# include <errno.h>

ExecDim (t)
    reg struct tree_node *t;
{
    reg struct tree_node *u, *v;
    reg char *s;
    int     d1, d2, ndim, length;

    for (; t != NULL; t = tree1 (t))
    {
	u = tree0 (t);
	v = tree0 (u);
	switch (u->t_op)
	{
	  case T_DIM0:
	  case T_DIM1:
	  case T_DIM2:
	    if (u->t_op == T_DIM0)
		ndim = 0;
	    else if (u->t_op == T_DIM1)
		ndim = 1;
	    else
		ndim = 2;
	    assert (v->t_op == T_DIM_NUM || v->t_op == T_DIM_STRING
		|| v->t_op == T_DIM_INT);
	    s = string0(v);
	    if (ndim >= 1)
	    {
		ExecTree (tree1 (u));
		d1 = (int) PopReal ();
	    }
	    else
		d1 = 1;
	    if (ndim == 2)
	    {
		ExecTree (tree2 (u));
		d2 = (int) PopReal ();
	    }
	    else
		d2 = 1;
/*
	    if (d1 < 1 || d1 > 10000)
	    {
		RunError ("Dimension 1 of '%s' invalid.", s);
	    }
	    if (d2 < 1 || d2 > 100)
	    {
		RunError ("Dimension 2 of '%s' invalid.", s);
	    }
*/
	    if (v->t_op == T_DIM_STRING)
	    {
		length = int1(v);
		if (length < 0)
		    length = -length;
		DimString (ndim, d1, d2, length, v, 0);
	    }
	    else
	    {
		DimReal (ndim, d1, d2, v, 0);
	    }
	    break;

	  default:
	    RunError ("Wrong dim: %s, Basic bug.", OpName (u->t_op));
	}
    }
}


DimReal (ndim, d1, d2, t, argno)
     register struct tree_node *t;
{
    struct name *s;
    register double *dp;
    char   *malloc ();
    register int     k;

    s = FindSymbol (t,0);
    if (s == NULL)
    {
	s = AllocSymbol (stringn(t,argno));
	SetSymbol(t, argno, s); /* note it */
    }
    else
    {
	switch (s->s_val.v_kind)
	{
	  case V_1D_REAL:
	  case V_2D_REAL:
	    free ((char *) s->s_val.v_rmat.m_area);
	    break;

	  case V_REAL_VAR:
	    break;

	  default:
	    Error ("HELP!  bad id type in dimstring (%d)\n", s->s_val.v_kind);
	    abort ();
	}
    }

    assert (ndim >= 1 && ndim <= 2);
    switch (ndim)
    {
      case 1:
      case 2:
	s->s_val.v_kind = (ndim == 1) ? V_1D_REAL : V_2D_REAL;
	k = d1 * d2;
	dp = (double *) malloc ((unsigned) (k * sizeof (double)));
	s->s_val.v_rmat.m_area = dp;
	s->s_val.v_rmat.m_dim1 = d1;
	s->s_val.v_rmat.m_dim2 = d2;
	if (dp == NULL)
	    RunError ("Can't dimension array with %d elements.", k);
	while (k--)
	    *dp++ = 0.0;
	break;
    }
}

DimString (ndim, d1, d2, length, t, argno)
    struct tree_node *t;
{
    struct name *n;
    char   *cp, *malloc ();
    register int     k;

    n = FindSymbol (t, argno);
    if (n == NULL)
    {
	n = AllocSymbol (stringn(t, argno));
	SetSymbol(t, argno, n); /* note it */
    }
    else
    {
	switch (n->s_val.v_kind)
	{
	  case V_STRING_VAR:
	    free ((char *) n->s_val.v_stringvar.s_p);
	    break;
	  case V_1D_STRING:
	  case V_2D_STRING:
	    free ((char *) n->s_val.v_smat.m_area);
	    break;
	  default:
	    Error ("HELP!  bad id type in dimstring (%d)\n",
		n->s_val.v_kind);
	    abort ();
	}
    }

    switch (ndim)
    {
      case 0:
	n->s_val.v_kind = V_STRING_VAR;
	n->s_val.v_stringvar.s_p = cp = malloc ((unsigned) (length + 1));
	cp[length] = cp[0] = '\0';
	n->s_val.v_stringvar.s_strlen = length;
#ifdef DEBUG
	if (Opts['D'])
	    printf ("alloc string %s len %d\n", name, length);
#endif
	break;

      case 1:
      case 2:
	n->s_val.v_kind = (ndim == 1) ? V_1D_STRING : V_2D_STRING;
	n->s_val.v_smat.m_dim1 = d1;
	n->s_val.v_smat.m_dim2 = d2;
	k = d1 * d2;
	n->s_val.v_smat.m_strlen = length;
	n->s_val.v_smat.m_area = cp = malloc ((unsigned) (k * (length + 1)));
	if (cp == NULL)
	{
	    RunError ("Can't dimension string array with %d elements.", k);
	}
	while (k--)
	{
	    cp[0] = cp[length] = '\0';
	    cp += length + 1;
	}
#ifdef DEBUG
	if (Opts['D'])
	    printf ("alloc string %s[%d,%d] len %d, addr %x\n",
		name, d1, d2, length, n->s_val.v_smat.m_area);
#endif
	break;
    }
}

ExecMatInput (kind, t)
    enum treeop kind;
    reg struct tree_node *t;
{
    reg struct tree_node *u;

    for (; t != NULL; t = tree1 (t))
    {
	if (tree0 (t) == NULL)
	    continue;
	u = tree0 (t);
	if (u->t_op != T_REALID && u->t_op != T_INTID && u->t_op != T_STRINGID)
	{
	    RunError ("MAT INPUT takes names, not expressions(%s).",
		OpName (u->t_op));
	}
	InputMat (kind, u, 0);
    }
}

ExecMatPrint (t)
    reg struct tree_node *t;
{
    enum treeop prev = T_LONG_PRINT;
    reg struct tree_node *u;

    for (; t != NULL; t = tree1 (t))
    {
	prev = t->t_op;
	if (tree0 (t) == NULL)
	    continue;
	u = tree0 (t);
	if (u->t_op != T_REALID && u->t_op != T_INTID && u->t_op != T_STRINGID)
	{
	    RunError ("MAT PRINT takes names, not expressions(%s).",
		OpName (u->t_op));
	}
	PrintMat (u, 0, prev == T_LONG_PRINT ? 1 : 0);
    }
    if (prev == T_LONG_PRINT && OutputCol > 0)
	NewLine ();
}

InputMat (kind, t, argno)
    enum treeop kind;
    struct tree_node *t;
{
    reg struct name *s;
    struct value val;
    int     ndims;

    s = FindSymbol (t, argno);
    if ( s == NULL)
	RunError ("Matrix '%s' not dimensioned.", stringn(t,argno));
    switch (s->s_val.v_kind)
    {
      case V_1D_REAL:
      case V_1D_INT:
      case V_1D_STRING:
	ndims = 1;
	break;
      case V_2D_REAL:
      case V_2D_INT:
      case V_2D_STRING:
	ndims = 2;
	break;
      default:
	RunError ("'%s' is the wrong kind of object for MAT PRINT.",
	    stringn(t, argno));
    }

    val = s->s_val;

    switch (val.v_kind)
    {
      case V_1D_REAL:
      case V_2D_REAL:
	InputRealMat (kind, ndims, val.v_rmat.m_dim1, val.v_rmat.m_dim2,
	    val.v_rmat.m_area);
	break;

      case V_1D_STRING:
      case V_2D_STRING:
	InputStrMat (kind, ndims, val.v_smat.m_dim1, val.v_smat.m_dim2,
	    val.v_smat.m_strlen, val.v_smat.m_area);
	break;

      case V_1D_INT:
      case V_2D_INT:
	InputIntMat (kind, ndims, val.v_imat.m_dim1, val.v_imat.m_dim2,
	    val.v_imat.m_area);
	break;
      default:
	RunError ("'%s' is not a matrix.", stringn(t, argno));
    }
}

InputRealMat (kind, ndims, d1, d2, area)
    enum treeop kind;
    unsigned d1, d2;
    double *area;
{
    int     i, j;
    struct value v, *rv;

    v.v_kind = V_REAL_VAR;
    if (ndims == 1)
    {
	for (i = 0; i < d1; ++i)
	{
	    v.v_realvar = &area[i];
	    if (kind == T_MAT_INPUT)
		rv = InputValue ();
	    else
		rv = ReadValue ();
	    InputAssign ((struct tree_node *) NULL, &v, rv);
	}
    }
    else
    {
	for (i = 0; i < d1; ++i)
	{
	    for (j = 0; j < d2; ++j)
	    {
		v.v_realvar = &area[i * d2 + j];
		if (kind == T_MAT_INPUT)
		    rv = InputValue ();
		else
		    rv = ReadValue ();
		InputAssign ((struct tree_node *) NULL, &v, rv);
	    }
#define BSD42
#ifdef  BSD42
	    sigsetmask (sigsetmask (~(1 << SIGINT)));
# else
	    sigrelse (SIGINT);
	    sighold (SIGINT);
#endif
	}
    }
}

InputIntMat (kind, ndims, d1, d2, area)
    enum treeop kind;
    unsigned d1, d2;
    int    *area;
{
    int     i, j;
    struct value v, *rv;

    v.v_kind = V_INT_VAR;
    if (ndims == 1)
    {
	for (i = 0; i < d1; ++i)
	{
	    v.v_intvar = &area[i];
	    if (kind == T_MAT_INPUT)
		rv = InputValue ();
	    else
		rv = ReadValue ();
	    InputAssign ((struct tree_node *) NULL, &v, rv);
	}
    }
    else
    {
	for (i = 0; i < d1; ++i)
	{
	    for (j = 0; j < d2; ++j)
	    {
		v.v_intvar = &area[i * d2 + j];
		if (kind == T_MAT_INPUT)
		    rv = InputValue ();
		else
		    rv = ReadValue ();
		InputAssign ((struct tree_node *) NULL, &v, rv);
	    }
#ifdef  BSD42
	    sigsetmask (sigsetmask (~(1 << SIGINT)));
# else
	    sigrelse (SIGINT);
	    sighold (SIGINT);
#endif
	}
    }
}

InputStrMat (kind, ndims, d1, d2, strlen, area)
    enum treeop kind;
    unsigned d1, d2;
    char   *area;
{
    int     i, j;
    struct value v, *rv;

    v.v_kind = V_STRING_VAR;
    v.v_stringvar.s_strlen = strlen;
    if (ndims == 1)
    {
	for (i = 0; i < d1; ++i)
	{
	    v.v_stringvar.s_p = area + i * strlen;
	    if (kind == T_MAT_INPUT)
		rv = InputValue ();
	    else
		rv = ReadValue ();
	    InputAssign ((struct tree_node *) NULL, &v, rv);
	}
    }
    else
    {
	for (i = 0; i < d1; ++i)
	{
	    for (j = 0; j < d2; ++j)
	    {
		v.v_stringvar.s_p = area + strlen * (i * d2 + j);
		if (kind == T_MAT_INPUT)
		    rv = InputValue ();
		else
		    rv = ReadValue ();
		InputAssign ((struct tree_node *) NULL, &v, rv);
	    }
#ifdef  BSD42
	    sigsetmask (sigsetmask (~(1 << SIGINT)));
# else
	    sigrelse (SIGINT);
	    sighold (SIGINT);
#endif
	}
    }
}

PrintMat (t, argno, long_form)
    struct tree_node *t;
{
    reg struct name *s;
    struct value val;
    int     ndims;

    s = FindSymbol (t, argno);
    if (s == NULL)
	RunError ("Matrix '%s' not dimensioned.",stringn(t,argno));
    switch (s->s_val.v_kind)
    {
      case V_1D_REAL:
      case V_1D_INT:
      case V_1D_STRING:
	ndims = 1;
	break;
      case V_2D_REAL:
      case V_2D_INT:
      case V_2D_STRING:
	ndims = 2;
	break;
      default:
	RunError ("'%s' is the wrong kind of object for MAT PRINT.",
	    stringn(t,argno));
    }

    val = s->s_val;

    NewLine ();
    printf ("\n");
    switch (s->s_val.v_kind)
    {
      case V_1D_REAL:
      case V_2D_REAL:
	PrintRealMat (ndims, val.v_rmat.m_dim1, val.v_rmat.m_dim2,
	    val.v_rmat.m_area, long_form);
	break;

      case V_1D_STRING:
      case V_2D_STRING:
	PrintStrMat (ndims, val.v_smat.m_dim1, val.v_smat.m_dim2,
	    val.v_smat.m_strlen, val.v_smat.m_area, long_form);
	break;

      case V_1D_INT:
      case V_2D_INT:
	PrintIntMat (ndims, val.v_imat.m_dim1, val.v_imat.m_dim2,
	    val.v_imat.m_area, long_form);
	break;

      default:
	RunError ("'%s' is not a matrix.", stringn(t,argno));
    }
}

PrintRealMat (ndims, d1, d2, area, long_form)
    unsigned d1, d2;
    double *area;
{
    int     i, j;
    struct value v;

    v.v_kind = V_REAL;
    if (ndims == 1)
    {
	for (i = 0; i < d1; ++i)
	{
	    v.v_real = area[i];
	    PrintValue (&v, long_form);
	}
    }
    else
    {
	for (i = 0; i < d1; ++i)
	{
	    for (j = 0; j < d2; ++j)
	    {
		v.v_real = area[i * d2 + j];
		PrintValue (&v, long_form);
	    }
	    NewLine ();
#ifdef  BSD42
	    sigsetmask (sigsetmask (~(1 << SIGINT)));
# else
	    sigrelse (SIGINT);
	    sighold (SIGINT);
#endif
	}
    }
}

PrintIntMat (ndims, d1, d2, area, long_form)
    unsigned d1, d2;
    int    *area;
{
    int     i, j;
    struct value v;

    v.v_kind = V_INT;
    if (ndims == 1)
    {
	for (i = 0; i < d1; ++i)
	{
	    v.v_int = area[i];
	    PrintValue (&v, long_form);
	}
    }
    else
    {
	for (i = 0; i < d1; ++i)
	{
	    for (j = 0; j < d2; ++j)
	    {
		v.v_int = area[i * d2 + j];
		PrintValue (&v, long_form);
	    }
	    NewLine ();
#ifdef  BSD42
	    sigsetmask (sigsetmask (~(1 << SIGINT)));
# else
	    sigrelse (SIGINT);
	    sighold (SIGINT);
#endif
	}
    }
}

PrintStrMat (ndims, d1, d2, strlen, area, long_form)
    unsigned d1, d2;
    char   *area;
{

    int     i, j;
    struct value v;

    v.v_kind = V_STRING;
    if (ndims == 1)
    {
	for (i = 0; i < d1; ++i)
	{
	    v.v_string = area + i * (strlen + 1);
	    PrintValue (&v, long_form);
	}
    }
    else
    {
	for (i = 0; i < d1; ++i)
	{
	    for (j = 0; j < d2; ++j)
	    {
		v.v_string = area + (strlen + 1) * (i * d2 + j);
		PrintValue (&v, long_form);
	    }
	    NewLine ();
#ifdef  BSD42
	    sigsetmask (sigsetmask (~(1 << SIGINT)));
# else
	    sigrelse (SIGINT);
	    sighold (SIGINT);
#endif
	}
    }
}

MatAssign (t1,argno1,t2,argno2)	/* to, from */
struct tree_node *t1, *t2;
{
    reg struct name *st, *sf;
    reg double *pf, *pt;
    reg int k;

    st = FindMatrix (t1,argno1);
    sf = FindMatrix (t2,argno2);
    if (st == sf) return;
    SameDims (st, t1, argno1, sf, t2, argno2);
    k = sf->s_val.v_rmat.m_dim1 * sf->s_val.v_rmat.m_dim2;
    pf = sf->s_val.v_rmat.m_area;
    pt = st->s_val.v_rmat.m_area;
    while (k--)
	*pt++ = *pf++;
}

double 
MatDet (tin, argno)
struct tree_node *tin;
{
    struct name *s;
    double  rcond, *z, *l;
    reg double *t, *m;
    int     d1, d_sqr, job, *ipvt;
    double  det[2];
    extern int errno;

    s = FindMatrix (tin, argno);
    MatIsSquare (s, tin, argno);
    d1 = s->s_val.v_rmat.m_dim1;
    d_sqr = d1 * d1;
    t = l = DoubleTemp (d_sqr + d1);
    z = l + d_sqr;
    m = s->s_val.v_rmat.m_area;

 /* Make copy of it first */
    while (d_sqr--)
	*t++ = *m++;
    ipvt = IntTemp (d1);
    dgeco_ (l, &d1, &d1, ipvt, &rcond, z);
    job = 10;
    dgedi_ (l, &d1, &d1, ipvt, det, z, &job);
#ifdef DEBUG
    if (Opts['D'])
    {
	printf ("det[0] %g, det[1] %g\n", det[0], det[1]);
    }
#endif
    if (det[1] >= -38)
    {
	errno = 0;
	det[1] = pow (10.0, det[1]);
	if (errno == ERANGE || errno == EDOM)
	{
	    RunError ("Determinant of '%s' is too large.", stringn(tin,argno));
	}
    }
    else
    {
	det[1] = 0.0;
    }
    det[0] *= det[1];
#ifdef DEBUG
    if (Opts['D'])
	printf ("det %g\n", det[0]);
#endif
    FreeIntTemp (ipvt);
    FreeDoubleTemp (l);
    return det[0];
}

double 
MatDot (t1, argno1, t2, argno2)
struct tree_node *t1, *t2;
{
    int     d1;
    reg double *m1, *m2;
    struct name *s1, *s2;
    reg double d;

    s1 = FindMatrix (t1, argno1);
    s2 = FindMatrix (t2, argno2);
    SameDims (s1, t1,argno1, s2, t2,argno2);
    if (s1->s_val.v_rmat.m_dim2 > 1)
    {
	RunError ("'%s' and '%s' are not column vectors.", stringn(t1, argno1),
							stringn(t2, argno2));
    }
    d1 = s1->s_val.v_rmat.m_dim1;
    d1 *= s1->s_val.v_rmat.m_dim2;
    m1 = s1->s_val.v_rmat.m_area;
    m2 = s2->s_val.v_rmat.m_area;

    d = 0.0;
    while (d1--)
    {
	d += *m1++ * *m2++;
    }
    return d;
}


MatFunc0 (op, t, argno)
    enum treeop op;
    struct tree_node *t;
{
    reg struct name *s;
    reg int d;
    double *m;

    s = FindMatrix (t, argno);
    d = s->s_val.v_rmat.m_dim1;
    d *= s->s_val.v_rmat.m_dim2;
    m = s->s_val.v_rmat.m_area;
    while (d--)
    {
	*m++ = ApplyFunc0 (op);
    }
}

MatFunc1 (op, t1, argno1, t2, argno2)
    enum treeop op;
    struct tree_node *t1, *t2;
{
    reg struct name *s1, *s2;
    reg double *m1, *m2;
    reg double d;
    int     d1;
    extern int errno;

    s1 = FindMatrix (t1, argno1);
    s2 = FindMatrix (t2, argno2);
    SameDims (s1, t1, argno1, s2, t2, argno2);
    d1 = s1->s_val.v_rmat.m_dim1;
    d1 *= s1->s_val.v_rmat.m_dim2;
    m1 = s1->s_val.v_rmat.m_area;
    m2 = s2->s_val.v_rmat.m_area;
    while (d1--)
    {
	d = *m2++;
	d = ApplyFunc1 (op, d);
	*m1++ = d;
    }
}


MatFunc2 (op, t1, argno1, t2, argno2, t3, argno3)
    enum treeop op;
    struct tree_node *t1, *t2, *t3;
{
    reg struct name *s1, *s2, *s3;
    reg int d;
    double *m1, *m2, *m3;

    s1 = FindMatrix (t1, argno1);
    s2 = FindMatrix (t2, argno2);
    s3 = FindMatrix (t3, argno3);
    SameDims (s1, t1,argno1, s2, t2,argno2);
    SameDims (s1, t1,argno2, s3, t3,argno3);
    d = s1->s_val.v_rmat.m_dim1;
    d *= s1->s_val.v_rmat.m_dim2;
    m1 = s1->s_val.v_rmat.m_area;
    m2 = s2->s_val.v_rmat.m_area;
    m3 = s3->s_val.v_rmat.m_area;
    while (d--)
    {
	*m1++ = ApplyFunc2 (op, *m2++, *m3++);
    }
}

MatIdentity (t, argno)
struct tree_node *t;
{
    reg struct name *s;
    reg int d1, d2, i, j;
    reg double *m;

    s = FindMatrix (t, argno);
    MatIsSquare (s, t, argno);
    d1 = s->s_val.v_rmat.m_dim1;
    d2 = s->s_val.v_rmat.m_dim2;
    m = s->s_val.v_rmat.m_area;
    for (i = 0; i < d1; ++i)
    {
	for (j = 0; j < d2; ++j)
	{
	    m[i * d2 + j] = 0.0;
	}
	m[i * d2 + i] = 1.0;
    }
}

MatInverse (t1,argno1,  t2, argno2)
struct tree_node *t1, *t2;
{
    struct name *s1, *s2;
    double  rcond, *z, *m;
    int     d1, job, *ipvt;

    s1 = FindMatrix (t1, argno1);
    s2 = FindMatrix (t2, argno2);
    MatIsSquare (s2, t2,argno2);
    MatIsSquare (s1, t1,argno1);
    MatAssign (t1,argno1,t2,argno2);
    d1 = s1->s_val.v_rmat.m_dim1;
    m = s1->s_val.v_rmat.m_area;
    ipvt = IntTemp (d1);
    z = DoubleTemp (d1);
    dgeco_ (m, &d1, &d1, ipvt, &rcond, z);
    if (rcond == 0.0)
    {
	RunError ("Can't invert matrix '%s': it is singular.", stringn(t2,argno2));
    }
    if (rcond + 1.0 == 1.0)
    {
	RunWarning ("Matrix '%s' is close to singular or badly scaled.",
	    stringn(t2,argno2));
    }
    job = 1;
    dgedi_ (m, &d1, &d1, ipvt, &rcond, z, &job);
    FreeIntTemp (ipvt);
    FreeDoubleTemp (z);
}

MatIsSquare (s, t, argno)
    reg struct name *s;
struct tree_node *t;
{
    int     d1, d2;

    if (s->s_val.v_kind != V_2D_REAL)
    {
	RunError ("'%s' is not a real 2-d matrix.", stringn(t, argno));
    }
    d1 = s->s_val.v_rmat.m_dim1;
    d2 = s->s_val.v_rmat.m_dim2;
    if (d1 != d2)
    {
	RunError ("Matrix '%s' dimensioned %d x %d isn't square.",
	    stringn(t, argno), d1, d2);
    }
}


MatMatOp (op, t1,argno1, t2,argno2, t3,argno3)
    enum treeop op;
    struct tree_node   *t1, *t2, *t3;
{
    struct name *s1, *s2, *s3;
    reg int d1;
    reg double *m1, *m2, *m3;

    s1 = FindMatrix (t1,argno1);
    s2 = FindMatrix (t2,argno2);
    s3 = FindMatrix (t3,argno3);
    SameDims (s1, t1,argno1, s2, t2,argno2);
    SameDims (s1, t1,argno1, s3, t3,argno3);
    d1 = s1->s_val.v_rmat.m_dim1;
    d1 *= s1->s_val.v_rmat.m_dim2;
    m1 = s1->s_val.v_rmat.m_area;
    m2 = s2->s_val.v_rmat.m_area;
    m3 = s3->s_val.v_rmat.m_area;
    switch (op)
    {
      case T_MAT_ADD:
	while (d1--)
	    *m1++ = *m2++ + *m3++;
	break;
      case T_MAT_SUB:
	while (d1--)
	    *m1++ = *m2++ - *m3++;
	break;
      default:
	Error ("HELP!  Bad operator: %s.", OpName (op));
	abort ();
    }
}


MatMul (t1,argno1, t2,argno2, t3,argno3)
struct tree_node *t1, *t2, *t3;
{
    struct name *s1, *s2, *s3;
    int     d1, d2, d3;
    reg int i, j, k;
    reg double aij;
    double *m1, *m2, *m3, *m;

    s1 = FindMatrix (t1,argno1);
    s2 = FindMatrix (t2,argno2);
    s3 = FindMatrix (t3,argno3);
    d1 = s2->s_val.v_rmat.m_dim1;
    d2 = s2->s_val.v_rmat.m_dim2;
    d3 = s3->s_val.v_rmat.m_dim2;
    if (d2 != s3->s_val.v_rmat.m_dim1)
	RunError ("Dimensions of '%s' and '%s' disagree.", stringn(t2,argno2), stringn(t3,argno3));
    if (d1 != s1->s_val.v_rmat.m_dim1)
	RunError ("Dimensions of '%s' and '%s' disagree.", stringn(t1,argno1), stringn(t2,argno2));
    if (d3 != s1->s_val.v_rmat.m_dim2)
	RunError ("Dimensions of '%s' and '%s' disagree.", stringn(t1,argno1), stringn(t3,argno3));

    m = m1 = s1->s_val.v_rmat.m_area;
    m2 = s2->s_val.v_rmat.m_area;
    m3 = s3->s_val.v_rmat.m_area;
    if (m1 == m2 || m1 == m3)
    {
    /* Allocate a temporary */
	m = DoubleTemp (d1 * d3);
    }

    for (i = 0; i < d1; ++i)
    {
	for (j = 0; j < d3; ++j)
	{
	    aij = 0.0;
	    for (k = 0; k < d2; ++k)
	    {
		aij += m2[i * d2 + k] * m3[k * d3 + j];
	    }
	    m[i * d3 + j] = aij;
	}
    }
    if (m != m1)
    {
    /* copy from temporary to destination */
	d1 *= d3;
	m2 = m;
	while (d1--)
	    *m1++ = *m2++;
	FreeDoubleTemp (m);
    }
}

double 
MatPrdSum (op, t, argno)
    enum treeop op;
    struct tree_node *t;
{
    reg struct name *s;
    reg double *m;
    reg double r;
    reg int d1;
    extern int errno;

    s = FindMatrix (t, argno);
    d1 = s->s_val.v_rmat.m_dim1;
    d1 *= s->s_val.v_rmat.m_dim2;
    m = s->s_val.v_rmat.m_area;
    switch (op)
    {
      case T_PRD:
	r = 1.0;
	while (d1--)
	    r *= *m++;
	break;
      case T_SUM:
	r = 0.0;
	while (d1--)
	    r += *m++;
	break;
      default:
	RunError ("Basic Bug!  op '%s' invalid in MatPrdSum.", OpName (op));
    /* NOTREACHED */
    }
    return r;
}

MatScalarOp (op, t1,argno1, t, t2,argno2)
    enum treeop op;
struct tree_node *t1, *t2;
    struct
    tree_node *t;
{
    int     d1;
    reg double *m1, *m2;
    struct name *s1, *s2;
    reg double d;
    double  k;

    s1 = FindMatrix (t1,argno1);
    s2 = FindMatrix (t2,argno2);
    SameDims (s1, t1,argno1, s2, t2,argno2);
    d1 = s1->s_val.v_rmat.m_dim1;
    d1 *= s1->s_val.v_rmat.m_dim2;
    m1 = s1->s_val.v_rmat.m_area;
    m2 = s2->s_val.v_rmat.m_area;
    ExecTree (t);
    k = PopReal ();

    while (d1--)
    {
	d = *m2++;
	switch (op)
	{
	  case T_MAT_SC_ASS:
	    d = k;
	    break;
	  case T_MAT_SC_ADD:
	    d += k;
	    break;
	  case T_MAT_SC_SUB:
	    d = k - d;
	    break;
	  case T_MAT_SC_MUL:
	    d *= k;
	    break;
	}
	*m1++ = d;
    }
}

MatTranspose (t1, argno1, t2, argno2)
   struct tree_node *t1, *t2;
{
    struct name *s1, *s2;
    reg int d1, d2, i, j;
    reg double *m1, *m2;
    double  tmp;

    s1 = FindMatrix (t1, argno1);
    s2 = FindMatrix (t2, argno2); 
    d1 = s1->s_val.v_rmat.m_dim1;
    d2 = s1->s_val.v_rmat.m_dim2;
    if (d1 != s2->s_val.v_rmat.m_dim2 || d2 != s2->s_val.v_rmat.m_dim1)
    {
	RunError ("'%s' and '%s' have incompatible dimensions.",
	    stringn(t1,argno1), stringn(t2, argno2));
    }
    m1 = s1->s_val.v_rmat.m_area;
    m2 = s2->s_val.v_rmat.m_area;
    if (m1 != m2)
    {
	for (i = 0; i < d1; ++i)
	{
	    for (j = 0; j < d2; ++j)
	    {
		m1[i * d2 + j] = m2[j * d1 + i];
	    }
	}
    }
    else
    {
	for (i = 0; i < d1; ++i)
	{
	    for (j = 0; j < i; ++j)
	    {
		tmp = m1[i * d2 + j];
		m1[i * d2 + j] = m1[j * d1 + i];
		m1[j * d2 + i] = tmp;
	    }
	}
    }
}

MatZero (t, argno)
struct tree_node *t;
{
    reg struct name *s;
    reg int d1, d2, i, j;
    reg double *m;

    s = FindMatrix (t, argno);
    d1 = s->s_val.v_rmat.m_dim1;
    d2 = s->s_val.v_rmat.m_dim2;
    m = s->s_val.v_rmat.m_area;
/*YYY should just go screaming through with double*m  for d1*d2 space */
    for (i = 0; i < d1; ++i)
	for (j = 0; j < d2; ++j)
	    m[i * d2 + j] = 0.0;
}

struct name *
FindMatrix (t, argno)
struct tree_node *t;
{
    reg struct name *s1;

    s1 = FindSymbol (t, argno);
    if (s1 == NULL)
	RunError ("Matrix '%s' isn't dimensioned.", stringn(t,argno));
    switch (s1->s_val.v_kind)
    {
      case V_1D_REAL:
      case V_2D_REAL:
	return s1;
      default:
	RunError ("'%s' isn't a numeric matrix.", stringn(t,argno));
    /* NOTREACHED */
    }
}

/*
 * SameDims - do two arrays have the same dimensionality?
 *
 * The arrays are assumed to be numeric items.  Just check dimensions.
 * Return no value, just cause a run-time error.
 */


SameDims (s1, t1,a1, s2, t2,a2)
    struct name *s1, *s2;
struct tree_node *t1, *t2;
{
    switch (s1->s_val.v_kind)
    {
      case V_1D_REAL:
      case V_2D_REAL:
	break;
      default:
	RunError ("'%s' is not a numeric matrix.", stringn(t1,a1));
    }
    switch (s2->s_val.v_kind)
    {
      case V_1D_REAL:
      case V_2D_REAL:
	break;
      default:
	RunError ("'%s' is not a numeric matrix.", stringn(t2,a2));
    }
    if (s1->s_val.v_kind != s2->s_val.v_kind)
	RunError ("'%s' and '%s' have different numbers of subscripts.",
	    stringn(t1,a1), stringn(t2,a2));
    if (s1->s_val.v_rmat.m_dim1 != s2->s_val.v_rmat.m_dim1
	|| s1->s_val.v_rmat.m_dim2 != s2->s_val.v_rmat.m_dim2)
	RunError ("'%s' and '%s' have different matrix bounds.", stringn(t1,a1), stringn(t2,a2));
}
