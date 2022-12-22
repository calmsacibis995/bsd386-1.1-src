/*	BSDI	$Id: terminal.h,v 1.1.1.1 1994/01/03 23:14:05 polk Exp $	*/

#if !defined(USE_TERMIOS)
#if !defined(USE_TERMIO)
#if !defined(USE_SGTTY)
#define USE_TERMIOS  /* <termios.h> */
#undef USE_TERMIO    /* <termio.h> */
#undef USE_SGTTY     /* <sys/ioctl.h> */
#endif
#endif
#endif
