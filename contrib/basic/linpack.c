/* linpack.f -- translated by f2c (version of 23 April 1993  18:34:30).
   You must link the resulting object file with the libraries:
	-lf2c -lm   (in that order)
*/

/* #include "f2c.h" *//* f2c.h  --  Standard Fortran to C header file */

/**  barf  [ba:rf]  2.  "He suggested using FORTRAN, and everybody barfed."

	- From The Shogakukan DICTIONARY OF NEW ENGLISH (Second edition) */

#ifndef F2C_INCLUDE
#define F2C_INCLUDE

typedef long int integer;
typedef char *address;
typedef short int shortint;
typedef float real;
typedef double doublereal;
typedef struct { real r, i; } complex;
typedef struct { doublereal r, i; } doublecomplex;
typedef long int logical;
typedef short int shortlogical;
typedef char logical1;
typedef char integer1;

#define TRUE_ (1)
#define FALSE_ (0)

/* Extern is for use with -E */
#ifndef Extern
#define Extern extern
#endif

/* I/O stuff */

#ifdef f2c_i2
/* for -i2 */
typedef short flag;
typedef short ftnlen;
typedef short ftnint;
#else
typedef long flag;
typedef long ftnlen;
typedef long ftnint;
#endif

/*external read, write*/
typedef struct
{	flag cierr;
	ftnint ciunit;
	flag ciend;
	char *cifmt;
	ftnint cirec;
} cilist;

/*internal read, write*/
typedef struct
{	flag icierr;
	char *iciunit;
	flag iciend;
	char *icifmt;
	ftnint icirlen;
	ftnint icirnum;
} icilist;

/*open*/
typedef struct
{	flag oerr;
	ftnint ounit;
	char *ofnm;
	ftnlen ofnmlen;
	char *osta;
	char *oacc;
	char *ofm;
	ftnint orl;
	char *oblnk;
} olist;

/*close*/
typedef struct
{	flag cerr;
	ftnint cunit;
	char *csta;
} cllist;

/*rewind, backspace, endfile*/
typedef struct
{	flag aerr;
	ftnint aunit;
} alist;

/* inquire */
typedef struct
{	flag inerr;
	ftnint inunit;
	char *infile;
	ftnlen infilen;
	ftnint	*inex;	/*parameters in standard's order*/
	ftnint	*inopen;
	ftnint	*innum;
	ftnint	*innamed;
	char	*inname;
	ftnlen	innamlen;
	char	*inacc;
	ftnlen	inacclen;
	char	*inseq;
	ftnlen	inseqlen;
	char 	*indir;
	ftnlen	indirlen;
	char	*infmt;
	ftnlen	infmtlen;
	char	*inform;
	ftnint	informlen;
	char	*inunf;
	ftnlen	inunflen;
	ftnint	*inrecl;
	ftnint	*innrec;
	char	*inblank;
	ftnlen	inblanklen;
} inlist;

#define VOID void

union Multitype {	/* for multiple entry points */
	shortint h;
	integer i;
	real r;
	doublereal d;
	complex c;
	doublecomplex z;
	};

typedef union Multitype Multitype;

typedef long Long;	/* No longer used; formerly in Namelist */

struct Vardesc {	/* for Namelist */
	char *name;
	char *addr;
	ftnlen *dims;
	int  type;
	};
typedef struct Vardesc Vardesc;

struct Namelist {
	char *name;
	Vardesc **vars;
	int nvars;
	};
typedef struct Namelist Namelist;

#define abs(x) ((x) >= 0 ? (x) : -(x))
#define dabs(x) (doublereal)abs(x)
#define min(a,b) ((a) <= (b) ? (a) : (b))
#define max(a,b) ((a) >= (b) ? (a) : (b))
#define dmin(a,b) (doublereal)min(a,b)
#define dmax(a,b) (doublereal)max(a,b)

/* procedure parameter types for -A and -C++ */

#define F2C_proc_par_types 1
#ifdef __cplusplus
typedef int /* Unknown procedure type */ (*U_fp)(...);
typedef shortint (*J_fp)(...);
typedef integer (*I_fp)(...);
typedef real (*R_fp)(...);
typedef doublereal (*D_fp)(...), (*E_fp)(...);
typedef /* Complex */ VOID (*C_fp)(...);
typedef /* Double Complex */ VOID (*Z_fp)(...);
typedef logical (*L_fp)(...);
typedef shortlogical (*K_fp)(...);
typedef /* Character */ VOID (*H_fp)(...);
typedef /* Subroutine */ int (*S_fp)(...);
#else
typedef int /* Unknown procedure type */ (*U_fp)();
typedef shortint (*J_fp)();
typedef integer (*I_fp)();
typedef real (*R_fp)();
typedef doublereal (*D_fp)(), (*E_fp)();
typedef /* Complex */ VOID (*C_fp)();
typedef /* Double Complex */ VOID (*Z_fp)();
typedef logical (*L_fp)();
typedef shortlogical (*K_fp)();
typedef /* Character */ VOID (*H_fp)();
typedef /* Subroutine */ int (*S_fp)();
#endif
/* E_fp is for real functions when -R is not specified */
typedef VOID C_f;	/* complex function */
typedef VOID H_f;	/* character function */
typedef VOID Z_f;	/* double complex function */
typedef doublereal E_f;	/* real function with -R not specified */

/* undef any lower-case symbols that your C compiler predefines, e.g.: */

#ifndef Skip_f2c_Undefs
#undef cray
#undef gcos
#undef mc68010
#undef mc68020
#undef mips
#undef pdp11
#undef sgi
#undef sparc
#undef sun
#undef sun2
#undef sun3
#undef sun4
#undef u370
#undef u3b
#undef u3b2
#undef u3b5
#undef unix
#undef vax
#endif
#endif


/* Table of constant values */

static integer c__1 = 1;

/* Subroutine */ int dgeco_(a, lda, n, ipvt, rcond, z)
doublereal *a;
integer *lda, *n, *ipvt;
doublereal *rcond, *z;
{
    /* System generated locals */
    integer a_dim1, a_offset, i__1, i__2;
    doublereal d__1, d__2;

    /* Builtin functions */
    double d_sign();

    /* Local variables */
    extern doublereal ddot_();
    static integer info;
    extern /* Subroutine */ int dgefa_();
    static integer j, k, l;
    static doublereal s, t;
    extern /* Subroutine */ int dscal_();
    extern doublereal dasum_();
    static doublereal anorm;
    extern /* Subroutine */ int daxpy_();
    static doublereal ynorm;
    static integer kb;
    static doublereal ek, sm, wk;
    static integer kp1;
    static doublereal wkm;


/*     dgeco factors a double precision matrix by gaussian elimination */
/*     and estimates the condition of the matrix. */

/*     if  rcond  is not needed, dgefa is slightly faster. */
/*     to solve  a*x = b , follow dgeco by dgesl. */
/*     to compute  inverse(a)*c , follow dgeco by dgesl. */
/*     to compute  determinant(a) , follow dgeco by dgedi. */
/*     to compute  inverse(a) , follow dgeco by dgedi. */

/*     on entry */

/*        a       double precision(lda, n) */
/*                the matrix to be factored. */

/*        lda     integer */
/*                the leading dimension of the array  a . */

/*        n       integer */
/*                the order of the matrix  a . */

/*     on return */

/*        a       an upper triangular matrix and the multipliers */
/*                which were used to obtain it. */
/*                the factorization can be written  a = l*u  where */
/*                l  is a product of permutation and unit lower */
/*                triangular matrices and  u  is upper triangular. */

/*        ipvt    integer(n) */
/*                an integer vector of pivot indices. */

/*        rcond   double precision */
/*                an estimate of the reciprocal condition of  a . */
/*                for the system  a*x = b , relative perturbations */
/*                in  a  and  b  of size  epsilon  may cause */
/*                relative perturbations in  x  of size  epsilon/rcond . 
*/
/*                if  rcond  is so small that the logical expression */
/*                           1.0 + rcond .eq. 1.0 */
/*                is true, then  a  may be singular to working */
/*                precision.  in particular,  rcond  is zero  if */
/*                exact singularity is detected or the estimate */
/*                underflows. */

/*        z       double precision(n) */
/*                a work vector whose contents are usually unimportant. */
/*                if  a  is close to a singular matrix, then  z  is */
/*                an approximate null vector in the sense that */
/*                norm(a*z) = rcond*norm(a)*norm(z) . */

/*     linpack. this version dated 03/11/78 . */
/*     cleve moler, university of new mexico, argonne national labs. */

/*     subroutines and functions */

/*     linpack dgefa */
/*     blas daxpy,ddot,dscal,dasum */
/*     fortran dabs,dmax1,dsign */

/*     internal variables */



/*     compute 1-norm of a */

    /* Parameter adjustments */
    --z;
    --ipvt;
    a_dim1 = *lda;
    a_offset = a_dim1 + 1;
    a -= a_offset;

    /* Function Body */
    anorm = 0.;
    i__1 = *n;
    for (j = 1; j <= i__1; ++j) {
/* Computing MAX */
	d__1 = anorm, d__2 = dasum_(n, &a[j * a_dim1 + 1], &c__1);
	anorm = max(d__1,d__2);
/* L10: */
    }

/*     factor */

    dgefa_(&a[a_offset], lda, n, &ipvt[1], &info);

/*     rcond = 1/(norm(a)*(estimate of norm(inverse(a)))) . */
/*     estimate = norm(z)/norm(y) where  a*z = y  and  trans(a)*y = e . */
/*     trans(a)  is the transpose of a .  the components of  e  are */
/*     chosen to cause maximum local growth in the elements of w  where */
/*     trans(u)*w = e .  the vectors are frequently rescaled to avoid */
/*     overflow. */

/*     solve trans(u)*w = e */

    ek = 1.;
    i__1 = *n;
    for (j = 1; j <= i__1; ++j) {
	z[j] = 0.;
/* L20: */
    }
    i__1 = *n;
    for (k = 1; k <= i__1; ++k) {
	if (z[k] != 0.) {
	    d__1 = -z[k];
	    ek = d_sign(&ek, &d__1);
	}
	if ((d__1 = ek - z[k], abs(d__1)) <= (d__2 = a[k + k * a_dim1], abs(
		d__2))) {
	    goto L30;
	}
	s = (d__1 = a[k + k * a_dim1], abs(d__1)) / (d__2 = ek - z[k], abs(
		d__2));
	dscal_(n, &s, &z[1], &c__1);
	ek = s * ek;
L30:
	wk = ek - z[k];
	wkm = -ek - z[k];
	s = abs(wk);
	sm = abs(wkm);
	if (a[k + k * a_dim1] == 0.) {
	    goto L40;
	}
	wk /= a[k + k * a_dim1];
	wkm /= a[k + k * a_dim1];
	goto L50;
L40:
	wk = 1.;
	wkm = 1.;
L50:
	kp1 = k + 1;
	if (kp1 > *n) {
	    goto L90;
	}
	i__2 = *n;
	for (j = kp1; j <= i__2; ++j) {
	    sm += (d__1 = z[j] + wkm * a[k + j * a_dim1], abs(d__1));
	    z[j] += wk * a[k + j * a_dim1];
	    s += (d__1 = z[j], abs(d__1));
/* L60: */
	}
	if (s >= sm) {
	    goto L80;
	}
	t = wkm - wk;
	wk = wkm;
	i__2 = *n;
	for (j = kp1; j <= i__2; ++j) {
	    z[j] += t * a[k + j * a_dim1];
/* L70: */
	}
L80:
L90:
	z[k] = wk;
/* L100: */
    }
    s = 1. / dasum_(n, &z[1], &c__1);
    dscal_(n, &s, &z[1], &c__1);

/*     solve trans(l)*y = w */

    i__1 = *n;
    for (kb = 1; kb <= i__1; ++kb) {
	k = *n + 1 - kb;
	if (k < *n) {
	    i__2 = *n - k;
	    z[k] += ddot_(&i__2, &a[k + 1 + k * a_dim1], &c__1, &z[k + 1], &
		    c__1);
	}
	if ((d__1 = z[k], abs(d__1)) <= 1.) {
	    goto L110;
	}
	s = 1. / (d__1 = z[k], abs(d__1));
	dscal_(n, &s, &z[1], &c__1);
L110:
	l = ipvt[k];
	t = z[l];
	z[l] = z[k];
	z[k] = t;
/* L120: */
    }
    s = 1. / dasum_(n, &z[1], &c__1);
    dscal_(n, &s, &z[1], &c__1);

    ynorm = 1.;

/*     solve l*v = y */

    i__1 = *n;
    for (k = 1; k <= i__1; ++k) {
	l = ipvt[k];
	t = z[l];
	z[l] = z[k];
	z[k] = t;
	if (k < *n) {
	    i__2 = *n - k;
	    daxpy_(&i__2, &t, &a[k + 1 + k * a_dim1], &c__1, &z[k + 1], &c__1)
		    ;
	}
	if ((d__1 = z[k], abs(d__1)) <= 1.) {
	    goto L130;
	}
	s = 1. / (d__1 = z[k], abs(d__1));
	dscal_(n, &s, &z[1], &c__1);
	ynorm = s * ynorm;
L130:
/* L140: */
	;
    }
    s = 1. / dasum_(n, &z[1], &c__1);
    dscal_(n, &s, &z[1], &c__1);
    ynorm = s * ynorm;

/*     solve  u*z = v */

    i__1 = *n;
    for (kb = 1; kb <= i__1; ++kb) {
	k = *n + 1 - kb;
	if ((d__1 = z[k], abs(d__1)) <= (d__2 = a[k + k * a_dim1], abs(d__2)))
		 {
	    goto L150;
	}
	s = (d__1 = a[k + k * a_dim1], abs(d__1)) / (d__2 = z[k], abs(d__2));
	dscal_(n, &s, &z[1], &c__1);
	ynorm = s * ynorm;
L150:
	if (a[k + k * a_dim1] != 0.) {
	    z[k] /= a[k + k * a_dim1];
	}
	if (a[k + k * a_dim1] == 0.) {
	    z[k] = 1.;
	}
	t = -z[k];
	i__2 = k - 1;
	daxpy_(&i__2, &t, &a[k * a_dim1 + 1], &c__1, &z[1], &c__1);
/* L160: */
    }
/*     make znorm = 1.0 */
    s = 1. / dasum_(n, &z[1], &c__1);
    dscal_(n, &s, &z[1], &c__1);
    ynorm = s * ynorm;

    if (anorm != 0.) {
	*rcond = ynorm / anorm;
    }
    if (anorm == 0.) {
	*rcond = 0.;
    }
    return 0;
} /* dgeco_ */

/* Subroutine */ int dgedi_(a, lda, n, ipvt, det, work, job)
doublereal *a;
integer *lda, *n, *ipvt;
doublereal *det, *work;
integer *job;
{
    /* System generated locals */
    integer a_dim1, a_offset, i__1, i__2;

    /* Local variables */
    static integer i, j, k, l;
    static doublereal t;
    extern /* Subroutine */ int dscal_(), dswap_(), daxpy_();
    static integer kb, kp1, nm1;
    static doublereal ten;


/*     dgedi computes the determinant and inverse of a matrix */
/*     using the factors computed by dgeco or dgefa. */

/*     on entry */

/*        a       double precision(lda, n) */
/*                the output from dgeco or dgefa. */

/*        lda     integer */
/*                the leading dimension of the array  a . */

/*        n       integer */
/*                the order of the matrix  a . */

/*        ipvt    integer(n) */
/*                the pivot vector from dgeco or dgefa. */

/*        work    double precision(n) */
/*                work vector.  contents destroyed. */

/*        job     integer */
/*                = 11   both determinant and inverse. */
/*                = 01   inverse only. */
/*                = 10   determinant only. */

/*     on return */

/*        a       inverse of original matrix if requested. */
/*                otherwise unchanged. */

/*        det     double precision(2) */
/*                determinant of original matrix if requested. */
/*                otherwise not referenced. */
/*                determinant = det(1) * 10.0**det(2) */
/*                with  1.0 .le. dabs(det(1)) .lt. 10.0 */
/*                or  det(1) .eq. 0.0 . */

/*     error condition */

/*        a division by zero will occur if the input factor contains */
/*        a zero on the diagonal and the inverse is requested. */
/*        it will not occur if the subroutines are called correctly */
/*        and if dgeco has set rcond .gt. 0.0 or dgefa has set */
/*        info .eq. 0 . */

/*     linpack. this version dated 03/11/78 . */
/*     cleve moler, university of new mexico, argonne national labs. */

/*     subroutines and functions */

/*     blas daxpy,dscal,dswap */
/*     fortran dabs,mod */

/*     internal variables */



/*     compute determinant */

    /* Parameter adjustments */
    --work;
    --det;
    --ipvt;
    a_dim1 = *lda;
    a_offset = a_dim1 + 1;
    a -= a_offset;

    /* Function Body */
    if (*job / 10 == 0) {
	goto L70;
    }
    det[1] = 1.;
    det[2] = 0.;
    ten = 10.;
    i__1 = *n;
    for (i = 1; i <= i__1; ++i) {
	if (ipvt[i] != i) {
	    det[1] = -det[1];
	}
	det[1] = a[i + i * a_dim1] * det[1];
/*        ...exit */
	if (det[1] == 0.) {
	    goto L60;
	}
L10:
	if (abs(det[1]) >= 1.) {
	    goto L20;
	}
	det[1] = ten * det[1];
	det[2] += -1.;
	goto L10;
L20:
L30:
	if (abs(det[1]) < ten) {
	    goto L40;
	}
	det[1] /= ten;
	det[2] += 1.;
	goto L30;
L40:
/* L50: */
	;
    }
L60:
L70:

/*     compute inverse(u) */

    if (*job % 10 == 0) {
	goto L150;
    }
    i__1 = *n;
    for (k = 1; k <= i__1; ++k) {
	a[k + k * a_dim1] = 1. / a[k + k * a_dim1];
	t = -a[k + k * a_dim1];
	i__2 = k - 1;
	dscal_(&i__2, &t, &a[k * a_dim1 + 1], &c__1);
	kp1 = k + 1;
	if (*n < kp1) {
	    goto L90;
	}
	i__2 = *n;
	for (j = kp1; j <= i__2; ++j) {
	    t = a[k + j * a_dim1];
	    a[k + j * a_dim1] = 0.;
	    daxpy_(&k, &t, &a[k * a_dim1 + 1], &c__1, &a[j * a_dim1 + 1], &
		    c__1);
/* L80: */
	}
L90:
/* L100: */
	;
    }

/*        form inverse(u)*inverse(l) */

    nm1 = *n - 1;
    if (nm1 < 1) {
	goto L140;
    }
    i__1 = nm1;
    for (kb = 1; kb <= i__1; ++kb) {
	k = *n - kb;
	kp1 = k + 1;
	i__2 = *n;
	for (i = kp1; i <= i__2; ++i) {
	    work[i] = a[i + k * a_dim1];
	    a[i + k * a_dim1] = 0.;
/* L110: */
	}
	i__2 = *n;
	for (j = kp1; j <= i__2; ++j) {
	    t = work[j];
	    daxpy_(n, &t, &a[j * a_dim1 + 1], &c__1, &a[k * a_dim1 + 1], &
		    c__1);
/* L120: */
	}
	l = ipvt[k];
	if (l != k) {
	    dswap_(n, &a[k * a_dim1 + 1], &c__1, &a[l * a_dim1 + 1], &c__1);
	}
/* L130: */
    }
L140:
L150:
    return 0;
} /* dgedi_ */

/* Subroutine */ int dgefa_(a, lda, n, ipvt, info)
doublereal *a;
integer *lda, *n, *ipvt, *info;
{
    /* System generated locals */
    integer a_dim1, a_offset, i__1, i__2, i__3;

    /* Local variables */
    static integer j, k, l;
    static doublereal t;
    extern /* Subroutine */ int dscal_(), daxpy_();
    extern integer idamax_();
    static integer kp1, nm1;


/*     dgefa factors a double precision matrix by gaussian elimination. */

/*     dgefa is usually called by dgeco, but it can be called */
/*     directly with a saving in time if  rcond  is not needed. */
/*     (time for dgeco) = (1 + 9/n)*(time for dgefa) . */

/*     on entry */

/*        a       double precision(lda, n) */
/*                the matrix to be factored. */

/*        lda     integer */
/*                the leading dimension of the array  a . */

/*        n       integer */
/*                the order of the matrix  a . */

/*     on return */

/*        a       an upper triangular matrix and the multipliers */
/*                which were used to obtain it. */
/*                the factorization can be written  a = l*u  where */
/*                l  is a product of permutation and unit lower */
/*                triangular matrices and  u  is upper triangular. */

/*        ipvt    integer(n) */
/*                an integer vector of pivot indices. */

/*        info    integer */
/*                = 0  normal value. */
/*                = k  if  u(k,k) .eq. 0.0 .  this is not an error */
/*                     condition for this subroutine, but it does */
/*                     indicate that dgesl or dgedi will divide by zero */
/*                     if called.  use  rcond  in dgeco for a reliable */
/*                     indication of singularity. */

/*     linpack. this version dated 03/11/78 . */
/*     cleve moler, university of new mexico, argonne national labs. */

/*     subroutines and functions */

/*     blas daxpy,dscal,idamax */

/*     internal variables */



/*     gaussian elimination with partial pivoting */

    /* Parameter adjustments */
    --ipvt;
    a_dim1 = *lda;
    a_offset = a_dim1 + 1;
    a -= a_offset;

    /* Function Body */
    *info = 0;
    nm1 = *n - 1;
    if (nm1 < 1) {
	goto L70;
    }
    i__1 = nm1;
    for (k = 1; k <= i__1; ++k) {
	kp1 = k + 1;

/*        find l = pivot index */

	i__2 = *n - k + 1;
	l = idamax_(&i__2, &a[k + k * a_dim1], &c__1) + k - 1;
	ipvt[k] = l;

/*        zero pivot implies this column already triangularized */

	if (a[l + k * a_dim1] == 0.) {
	    goto L40;
	}

/*           interchange if necessary */

	if (l == k) {
	    goto L10;
	}
	t = a[l + k * a_dim1];
	a[l + k * a_dim1] = a[k + k * a_dim1];
	a[k + k * a_dim1] = t;
L10:

/*           compute multipliers */

	t = -1. / a[k + k * a_dim1];
	i__2 = *n - k;
	dscal_(&i__2, &t, &a[k + 1 + k * a_dim1], &c__1);

/*           row elimination with column indexing */

	i__2 = *n;
	for (j = kp1; j <= i__2; ++j) {
	    t = a[l + j * a_dim1];
	    if (l == k) {
		goto L20;
	    }
	    a[l + j * a_dim1] = a[k + j * a_dim1];
	    a[k + j * a_dim1] = t;
L20:
	    i__3 = *n - k;
	    daxpy_(&i__3, &t, &a[k + 1 + k * a_dim1], &c__1, &a[k + 1 + j * 
		    a_dim1], &c__1);
/* L30: */
	}
	goto L50;
L40:
	*info = k;
L50:
/* L60: */
	;
    }
L70:
    ipvt[*n] = *n;
    if (a[*n + *n * a_dim1] == 0.) {
	*info = *n;
    }
    return 0;
} /* dgefa_ */

doublereal dasum_(n, dx, incx)
integer *n;
doublereal *dx;
integer *incx;
{
    /* System generated locals */
    integer i__1, i__2;
    doublereal ret_val, d__1, d__2, d__3, d__4, d__5, d__6;

    /* Local variables */
    static integer i, m;
    static doublereal dtemp;
    static integer nincx, mp1;


/*     takes the sum of the absolute values. */
/*     jack dongarra, linpack, 6/17/77. */


    /* Parameter adjustments */
    --dx;

    /* Function Body */
    ret_val = 0.;
    dtemp = 0.;
    if (*n <= 0) {
	return ret_val;
    }
    if (*incx == 1) {
	goto L20;
    }

/*        code for increment not equal to 1 */

    nincx = *n * *incx;
    i__1 = nincx;
    i__2 = *incx;
    for (i = 1; i__2 < 0 ? i >= i__1 : i <= i__1; i += i__2) {
	dtemp += (d__1 = dx[i], abs(d__1));
/* L10: */
    }
    ret_val = dtemp;
    return ret_val;

/*        code for increment equal to 1 */


/*        clean-up loop */

L20:
    m = *n % 6;
    if (m == 0) {
	goto L40;
    }
    i__2 = m;
    for (i = 1; i <= i__2; ++i) {
	dtemp += (d__1 = dx[i], abs(d__1));
/* L30: */
    }
    if (*n < 6) {
	goto L60;
    }
L40:
    mp1 = m + 1;
    i__2 = *n;
    for (i = mp1; i <= i__2; i += 6) {
	dtemp = dtemp + (d__1 = dx[i], abs(d__1)) + (d__2 = dx[i + 1], abs(
		d__2)) + (d__3 = dx[i + 2], abs(d__3)) + (d__4 = dx[i + 3], 
		abs(d__4)) + (d__5 = dx[i + 4], abs(d__5)) + (d__6 = dx[i + 5]
		, abs(d__6));
/* L50: */
    }
L60:
    ret_val = dtemp;
    return ret_val;
} /* dasum_ */

/* Subroutine */ int dscal_(n, da, dx, incx)
integer *n;
doublereal *da, *dx;
integer *incx;
{
    /* System generated locals */
    integer i__1, i__2;

    /* Local variables */
    static integer i, m, nincx, mp1;


/*     scales a vector by a constant. */
/*     uses unrolled loops for increment equal to one. */
/*     jack dongarra, linpack, 6/17/77. */


    /* Parameter adjustments */
    --dx;

    /* Function Body */
    if (*n <= 0) {
	return 0;
    }
    if (*incx == 1) {
	goto L20;
    }

/*        code for increment not equal to 1 */

    nincx = *n * *incx;
    i__1 = nincx;
    i__2 = *incx;
    for (i = 1; i__2 < 0 ? i >= i__1 : i <= i__1; i += i__2) {
	dx[i] = *da * dx[i];
/* L10: */
    }
    return 0;

/*        code for increment equal to 1 */


/*        clean-up loop */

L20:
    m = *n % 5;
    if (m == 0) {
	goto L40;
    }
    i__2 = m;
    for (i = 1; i <= i__2; ++i) {
	dx[i] = *da * dx[i];
/* L30: */
    }
    if (*n < 5) {
	return 0;
    }
L40:
    mp1 = m + 1;
    i__2 = *n;
    for (i = mp1; i <= i__2; i += 5) {
	dx[i] = *da * dx[i];
	dx[i + 1] = *da * dx[i + 1];
	dx[i + 2] = *da * dx[i + 2];
	dx[i + 3] = *da * dx[i + 3];
	dx[i + 4] = *da * dx[i + 4];
/* L50: */
    }
    return 0;
} /* dscal_ */

/* Subroutine */ int dswap_(n, dx, incx, dy, incy)
integer *n;
doublereal *dx;
integer *incx;
doublereal *dy;
integer *incy;
{
    /* System generated locals */
    integer i__1;

    /* Local variables */
    static integer i, m;
    static doublereal dtemp;
    static integer ix, iy, mp1;


/*     interchanges two vectors. */
/*     uses unrolled loops for increments equal one. */
/*     jack dongarra, linpack, 6/17/77. */


    /* Parameter adjustments */
    --dy;
    --dx;

    /* Function Body */
    if (*n <= 0) {
	return 0;
    }
    if (*incx == 1 && *incy == 1) {
	goto L20;
    }

/*       code for unequal increments or equal increments not equal */
/*         to 1 */

    ix = 1;
    iy = 1;
    if (*incx < 0) {
	ix = (-(*n) + 1) * *incx + 1;
    }
    if (*incy < 0) {
	iy = (-(*n) + 1) * *incy + 1;
    }
    i__1 = *n;
    for (i = 1; i <= i__1; ++i) {
	dtemp = dx[ix];
	dx[ix] = dy[iy];
	dy[iy] = dtemp;
	ix += *incx;
	iy += *incy;
/* L10: */
    }
    return 0;

/*       code for both increments equal to 1 */


/*       clean-up loop */

L20:
    m = *n % 3;
    if (m == 0) {
	goto L40;
    }
    i__1 = m;
    for (i = 1; i <= i__1; ++i) {
	dtemp = dx[i];
	dx[i] = dy[i];
	dy[i] = dtemp;
/* L30: */
    }
    if (*n < 3) {
	return 0;
    }
L40:
    mp1 = m + 1;
    i__1 = *n;
    for (i = mp1; i <= i__1; i += 3) {
	dtemp = dx[i];
	dx[i] = dy[i];
	dy[i] = dtemp;
	dtemp = dx[i + 1];
	dx[i + 1] = dy[i + 1];
	dy[i + 1] = dtemp;
	dtemp = dx[i + 2];
	dx[i + 2] = dy[i + 2];
	dy[i + 2] = dtemp;
/* L50: */
    }
    return 0;
} /* dswap_ */

integer idamax_(n, dx, incx)
integer *n;
doublereal *dx;
integer *incx;
{
    /* System generated locals */
    integer ret_val, i__1;
    doublereal d__1;

    /* Local variables */
    static doublereal dmax_;
    static integer i, ix;


    /* Parameter adjustments */
    --dx;

    /* Function Body */
    ret_val = 0;
    if (*n <= 0) {
	return ret_val;
    }
    ret_val = 1;
    if (*n == 1) {
	return ret_val;
    }
    if (*incx == 1) {
	goto L20;
    }

/*        code for increment not equal to 1 */

    ix = 1;
    dmax_ = abs(dx[1]);
    ix += *incx;
    i__1 = *n;
    for (i = 2; i <= i__1; ++i) {
	if ((d__1 = dx[ix], abs(d__1)) <= dmax_) {
	    goto L5;
	}
	ret_val = i;
	dmax_ = (d__1 = dx[ix], abs(d__1));
L5:
	ix += *incx;
/* L10: */
    }
    return ret_val;

/*        code for increment equal to 1 */


L20:
    dmax_ = abs(dx[1]);
    i__1 = *n;
    for (i = 2; i <= i__1; ++i) {
	if ((d__1 = dx[i], abs(d__1)) <= dmax_) {
	    goto L30;
	}
	ret_val = i;
	dmax_ = (d__1 = dx[i], abs(d__1));
L30:
	;
    }
    return ret_val;
} /* idamax_ */

double d_sign(doublereal *a, doublereal *b)
{
double x;
x = (*a >= 0 ? *a : - *a);
return( *b >= 0 ? x : -x);
}
