/*
**  This file records official patches.  RCS records the edit log.
**
**  $Header: /bsdi/MASTER/BSDI_OS/contrib/cshar/cshar/patchlevel.h,v 1.1.1.1 1993/02/23 18:00:02 polk Exp $
**
**  $Log: patchlevel.h,v $
 * Revision 1.1.1.1  1993/02/23  18:00:02  polk
 *
**  Revision 2.3  88/06/06  22:04:33  rsalz
**  patch03:  Fix typo in makekit manpage, and getopt call in the program.
**  patch03:  Add NEED_RENAME and BACKUP_PREFIX to config.*; edit llib.c
**  patch03:  and makekit.c to use them.
**  
**  Revision 2.2  88/06/03  16:08:37  rsalz
**  patch02:  Fix order of chdir/mkdir commands for unshar.
**  
**  Revision 2.1  88/06/03  12:16:40  rsalz
**  patch01:  Add config.x386 & config.sVr3; change "dirent.h" to <dirent.h>
**  patch01:  In Makefile, use $(DIRLIB) only in actions, not dependencies;
**  patch01:  add /usr/man/local option for MANDIR.
**  patch01:  Put isascii() before every use of a <ctype.h> macro. 
**  patch01:  Initialize Flist in shar.c/main().
**  patch01:  Add -x to synopsis in makekit.man; improve the usage message &
**  patch01:  put comments around note after an #endif (ANSI strikes again).
**  patch01:  Remove extraneous declaration of Dispatch[] in parser.c
**  patch01:  Add missing argument in fprintf call in findsrc.
**  
**  Revision 2.0  88/05/27  13:32:13  rsalz
**  First comp.sources.unix release
*/
#define PATCHLEVEL 3
