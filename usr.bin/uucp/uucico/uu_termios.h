/* do_termios.h - definitions for our local termios.c module
 * vix 25feb93 [original]
 */

extern void	DoTermios(int fd, int flag, int onoff);

#define	UU_HWFLOW	0x0001
#define	UU_MDMBUF	0x0002
#define	UU_RTSCTS	0x0004
#define	UU_CLOCAL	0x0008
