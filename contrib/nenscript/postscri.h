/*
 * $Id: postscri.h,v 1.1.1.1 1993/02/10 18:05:36 sanders Exp $
 *
 *   This code was written by Craig Southeren whilst under contract
 *   to Computer Sciences of Australia, Systems Engineering Division.
 *   It has been kindly released by CSA into the public domain.
 *
 *   Neither CSA or me guarantee that this source code is fit for anything,
 *   so use it at your peril. I don't even work for CSA any more, so
 *   don't bother them about it. If you have any suggestions or comments
 *   (or money, cheques, free trips =8^) !!!!! ) please contact me
 *   care of geoffw@extro.ucc.oz.au
 *
 */

#include <stdio.h>

#ifdef __STDC__

extern void StartJob      (FILE *, char *, int, int, char *, char *, int, int, char *, int, int, int, char *);
extern void StartDocument (FILE *, char *);
extern void EndColumn     (FILE *);
extern void WriteLine     (FILE *, char *);
extern void EndDocument   (FILE *);
extern void EndJob        (FILE *);

#else

extern void StartJob      ();
extern void StartDocument ();
extern void EndColumn     ();
extern void WriteLine     ();
extern void EndDocument   ();
extern void EndJob        ();

#endif

