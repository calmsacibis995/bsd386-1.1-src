#ifndef _STDDEF_H
#define _STDDEF_H

#ifndef __sys_stdtypes_h
/* This avoids lossage on Sunos but only if stdtypes.h comes first.
   There's no way to win with the other order!  Sun lossage.  */

/* Signed type of difference of two pointers.  */

#ifndef _PTRDIFF_T	/* in case <sys/types.h> has defined it. */
#ifndef _T_PTRDIFF
#ifndef __PTRDIFF_T
#ifndef _PTRDIFF_T_
#ifndef ___int_ptrdiff_t_h
#define _PTRDIFF_T
#define _T_PTRDIFF
#define __PTRDIFF_T
#define _PTRDIFF_T_
#define ___int_ptrdiff_t_h
typedef long ptrdiff_t;
#endif /* ___int_ptrdiff_t_h */
#endif /* _PTRDIFF_T_ */
#endif /* __PTRDIFF_T */
#endif /* _T_PTRDIFF */
#endif /* _PTRDIFF_T */

/* Unsigned type of `sizeof' something.  */

#ifndef _SIZE_T	/* in case <sys/types.h> has defined it. */
#ifndef _T_SIZE
#ifndef __SIZE_T
#ifndef _SIZE_T_
#ifndef ___int_size_t_h
#define _SIZE_T
#define _T_SIZE
#define __SIZE_T
#define _SIZE_T_
#define ___int_size_t_h
typedef unsigned long size_t;
#endif /* ___int_size_t_h */
#endif /* _SIZE_T_ */
#endif /* __SIZE_T */
#endif /* _T_SIZE */
#endif /* _SIZE_T */

/* Data type for wide chars.  */

#ifndef _WCHAR_T
#ifndef _WCHAR_T_
#define _WCHAR_T
#define _WCHAR_T_
typedef int wchar_t;
#endif
#endif

#endif /* __sys_stdtypes_h */

/* A null pointer constant.  */

#undef NULL		/* in case <stdio.h> has defined it. */
#define NULL ((void *)0)

/* Offset of member MEMBER in a struct of type TYPE.  */

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#endif /* _STDDEF_H */
