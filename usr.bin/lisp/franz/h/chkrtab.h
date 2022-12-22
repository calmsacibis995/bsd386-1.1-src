/*					-[Sat Jan 29 13:53:19 1983 by jkf]-
 * 	chkrtab.h			$Locker:  $
 * check if read table valid 
 *
 * $Header: /bsdi/MASTER/BSDI_OS/usr.bin/lisp/franz/h/chkrtab.h,v 1.2 1992/01/04 18:56:26 kolstad Exp $
 *
 * (c) copyright 1982, Regents of the University of California
 */
 
#define chkrtab(p);	\
	if(p!=lastrtab){ if(TYPE(p)!=ARRAY && TYPE(p->ar.data)!=INT) rtaberr();\
			else {lastrtab=p;ctable=(unsigned char*)p->ar.data;}}
extern lispval lastrtab;
