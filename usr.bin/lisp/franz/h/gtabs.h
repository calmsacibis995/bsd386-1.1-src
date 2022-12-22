/*					-[Sat Jan 29 13:57:36 1983 by jkf]-
 * 	gtabs.h				$Locker:  $
 * global lispval table
 *
 * $Header: /bsdi/MASTER/BSDI_OS/usr.bin/lisp/franz/h/gtabs.h,v 1.2 1992/01/04 18:56:41 kolstad Exp $
 *
 * (c) copyright 1982, Regents of the University of California
 */

/*  these are the tables of global lispvals known to the interpreter	*/
/*  and compiler.  They are not used by the garbage collector.		*/
#define GFTABLEN 200
#define GCTABLEN 8
extern lispval gftab[GFTABLEN];
extern lispval gctab[GCTABLEN];
