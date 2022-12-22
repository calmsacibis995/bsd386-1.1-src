/* $Header: /bsdi/MASTER/BSDI_OS/contrib/gpatch/EXTERN.h,v 1.1.1.1 1994/01/13 21:15:31 polk Exp $
 *
 * $Log: EXTERN.h,v $
 * Revision 1.1.1.1  1994/01/13  21:15:31  polk
 *
 * Revision 2.0  86/09/17  15:35:37  lwall
 * Baseline for netwide release.
 * 
 */

#ifdef EXT
#undef EXT
#endif
#define EXT extern

#ifdef INIT
#undef INIT
#endif
#define INIT(x)

#ifdef DOINIT
#undef DOINIT
#endif
