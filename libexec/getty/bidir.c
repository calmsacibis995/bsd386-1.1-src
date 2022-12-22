/*	BSDI $Id: bidir.c,v 1.3 1993/12/18 19:12:07 sanders Exp $	*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/termios.h>
#include <dirent.h>

#include "../../usr.bin/tip/pathnames.h"

int
test_clocal(fd)
	int fd;
{
	struct termios t;

	tcgetattr(fd, &t);
	if (t.c_cflag & CLOCAL) {
		t.c_cflag &= ~CLOCAL;
		tcsetattr(fd, TCSANOW, &t);
		return (1);
	}

	return (0);
}

set_clocal(fd, t)
	int fd;
	struct termios *t;
{

	t->c_cflag |= CLOCAL;
	(void) tcsetattr(fd, TCSANOW, t);
}

void
flush_echos(fd)
	int fd;
{
	struct termios t;

	tcgetattr(fd, &t);
	tcsetattr(fd, TCSADRAIN, &t);
	usleep(250000);
	tcflush(fd, TCIOFLUSH);
}

char lockname[sizeof(_PATH_LOCKDIRNAME) + MAXNAMLEN];

int
seize_lock(ttyname, block)
	char *ttyname;
	int block;
{
	int start_stat_val, stat_val;
	struct stat start_stat_buf, stat_buf;

	sprintf(lockname, _PATH_LOCKDIRNAME, ttyname);
	start_stat_val = stat(lockname, &start_stat_buf);

	if (uu_lock(ttyname) == 0)
		return (0);

	while (block) {
		sleep(15);
		stat_val = stat(lockname, &stat_buf);

		if (stat_val != start_stat_val ||
		    stat_buf.st_mtime != start_stat_buf.st_mtime)
			break;
	}
			
	return (-1);
}

void
release_lock(ttyname)
	char *ttyname;
{
	uu_unlock(ttyname);
}
