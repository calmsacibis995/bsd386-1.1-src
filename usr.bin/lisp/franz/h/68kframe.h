/*
 * $Header: /bsdi/MASTER/BSDI_OS/usr.bin/lisp/franz/h/68kframe.h,v 1.2 1992/01/04 18:56:20 kolstad Exp $
 * $Locker:  $
 * machine stack frame
 */
struct machframe {
struct 	machframe	*fp;
	lispval	(*pc)();
	lispval ap[1];
};
