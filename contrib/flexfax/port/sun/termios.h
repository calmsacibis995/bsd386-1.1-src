#ifndef _TERMIOS_
#define	_TERMIOS_
extern "C" {
#define	KERNEL
#include "/usr/include/termios.h"

speed_t	cfgetispeed(struct termios *);
speed_t	cfgetospeed(struct termios *);
int	cfsetispeed(const struct termios *, speed_t);
int	cfsetospeed(const struct termios *, speed_t);
int	tcdrain(int fildes);
int	tcflow(int fildes, int action);
int	tcflush(int fildes, int queue_selector);
int	tcgetattr(int fildes, struct termios *);
int	tcsendbreak(int fildes, int duration);
int	tcsetattr(int fildes, int optional_actions, const struct termios *);
}
#endif /* _TERMIOS_ */
