/* flock emulation for System V using fcntl
 *
 * flock is just mapped to fcntl 
 */

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "flock.h"

int
flock(int fd, int operation)
{
    struct flock flock;
    int r;
    
    memset(&flock, '\0', sizeof(flock));
    if (operation & LOCK_EX)
	flock.l_type = F_WRLCK;
    else if (operation & LOCK_SH)
	flock.l_type = F_RDLCK;
    else
	flock.l_type = F_UNLCK;
    flock.l_whence = SEEK_SET;
    
    if (((r=fcntl(fd, (operation & LOCK_NB) ? F_SETLK:F_SETLKW, &flock)) == -1)
		&& (errno == EACCES || errno == EAGAIN))
			errno = EWOULDBLOCK;
    return r;
}
