/*					-[Sat Jan 29 14:02:34 1983 by jkf]-
 * 	vaxframe.h			$Locker:  $
 * vax calling frame definition
 *
 * $Header: /bsdi/MASTER/BSDI_OS/usr.bin/lisp/franz/h/vaxframe.h,v 1.2 1992/01/04 18:56:53 kolstad Exp $
 *
 * (c) copyright 1982, Regents of the University of California
 */

struct machframe {
	lispval	(*handler)();
	long	mask;
	lispval	*ap;
struct 	machframe	*fp;
	lispval	(*pc)();
	lispval	*r6;
	lispval *r7;
};
