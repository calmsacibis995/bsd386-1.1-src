/* uu_termios.c - local interface to termios
 * vix 25feb93 [original]
 */

/* this doesn't include any of the standard uucp include files
 * because we might be using termios here and sgtty everywhere
 * else.
 */

#include <sys/param.h>
#include <stdio.h>

#include "uu_termios.h"

#if USE_TERMIOS
#include <termios.h>
#endif

/*
**	Control the termios parameters
*/

void
DoTermios(
	  int fd,
	  int flags,
	  int onoff
)
{
#if USE_TERMIOS
	struct termios t;
	int cflag = 0;

	if (flags & UU_HWFLOW) {
		cflag |= CRTS_IFLOW|CCTS_OFLOW;
		flags &= ~UU_HWFLOW;
	}
	if (flags & UU_MDMBUF) {
		cflag |= MDMBUF;
		flags &= ~UU_MDMBUF;
	}
	if (flags & UU_RTSCTS) {
		cflag |= CRTSCTS;
		flags &= ~UU_RTSCTS;
	}
	if (flags & UU_CLOCAL) {
		cflag |= CLOCAL;
		flags &= ~UU_CLOCAL;
	}
	if (flags) {
		fprintf(stderr, "unsupported flag used in DoTermios(): 0x%x\n",
			flags);
		abort();
	}

	tcgetattr(fd, &t);

	if (onoff) {
		t.c_cflag |= cflag;
	} else {
		t.c_cflag &= ~cflag;
	}

	tcsetattr(fd, TCSANOW, &t);
#endif
}
