#ifndef _FLOCK_H
#define	_FLOCK_H

/*
 * flock (fd, operation)
 *
 * This routine performs some file locking like the BSD 'flock'
 * on the object described by the int file descriptor 'fd',
 * which must already be open.
 *
 * The operations that are available are:
 *
 * LOCK_SH  -  get a shared lock.
 * LOCK_EX  -  get an exclusive lock.
 * LOCK_NB  -  don't block (must be ORed with LOCK_SH or LOCK_EX).
 * LOCK_UN  -  release a lock.
 *
 * Return value: 0 if lock successful, -1 if failed.
 *
 * Note that whether the locks are enforced or advisory is
 * controlled by the presence or absence of the SETGID bit on
 * the executable.
 *
 * Note that there is no difference between shared and exclusive
 * locks, since the 'lockf' system call in SYSV doesn't make any
 * distinction.
 *
 * The file "<sys/file.h>" should be modified to contain the definitions
 * of the available operations, which must be added manually (see below
 * for the values).
 */

#include <unistd.h>
#include <sys/file.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif
int flock(int, int);
#ifdef __cplusplus
}
#endif

#ifndef LOCK_SH
#define LOCK_SH 1
#endif
#ifndef LOCK_EX
#define LOCK_EX 2
#endif
#ifndef LOCK_NB
#define LOCK_NB 4
#endif
#ifndef LOCK_UN
#define LOCK_UN 8
#endif
#endif
