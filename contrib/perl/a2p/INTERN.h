/* $RCSfile: INTERN.h,v $$Revision: 1.1.1.1 $$Date: 1992/07/27 23:23:59 $
 *
 *    Copyright (c) 1991, Larry Wall
 *
 *    You may distribute under the terms of either the GNU General Public
 *    License or the Artistic License, as specified in the README file.
 *
 * $Log: INTERN.h,v $
 * Revision 1.1.1.1  1992/07/27  23:23:59  polk
 * Latest and greatest perl
 * Includes undump functionality
 *
 * Revision 1.1.1.1  1992/02/18  22:28:23  trent
 * Initial import of PERL
 *
 * Revision 4.0.1.1  91/06/07  12:11:20  lwall
 * patch4: new copyright notice
 * 
 * Revision 4.0  91/03/20  01:56:58  lwall
 * 4.0 baseline.
 * 
 */

#undef EXT
#define EXT

#undef INIT
#define INIT(x) = x

#define DOINIT
