/*					-[Sat Jan 29 13:56:06 1983 by jkf]-
 * 	gc.h			$Locker:  $
 * garbage collector metering definitions
 *
 * $Header: /bsdi/MASTER/BSDI_OS/usr.bin/lisp/franz/h/gc.h,v 1.2 1992/01/04 18:56:35 kolstad Exp $
 *
 * (c) copyright 1982, Regents of the University of California
 */
 
struct gchead
	{  int version;	/* version number of this dump file */
	   int lowdata;	/* low address of sharable lisp data */
	   int dummy,dummy2,dummy3; 	/* to be used later	*/
	};

