/* flock emulation for System V using fcntl
 *
 * flock is just mapped to fcntl using a static function.
 * This adds some overhead to every module that includes this header file.
 * It is assumed that the system header files included here protect themselves 
 * against multiple inclusion.
 */

#ifndef _FLOCK_H
#define	_FLOCK_H

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define LOCK_SH        1    /* shared lock */
#define LOCK_EX        2    /* exclusive lock */
#define LOCK_NB        4    /* don't block when locking */
#define LOCK_UN        8    /* unlock */

static int flock(int fd, int operation)
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
#endif
