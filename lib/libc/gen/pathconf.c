/*	BSDI $Id: pathconf.c,v 1.2 1993/03/08 06:56:39 polk Exp $	*/

/*
 * Contributed to BSD/386 by Andrew H. Marrinson (andy@terasil.terasil.com).
 *
 * These values really need to be obtained from the kernel with a syscall.
 */

#include <errno.h>
#include <limits.h>
#include <unistd.h>

long
fpathconf(int fd, int name)
{
	long result;

	switch (name) {
	case _PC_LINK_MAX:
		result = LINK_MAX;
		break;
	case _PC_MAX_CANON:
		result = MAX_CANON;
		break;
	case _PC_MAX_INPUT:
		result = MAX_INPUT;
		break;
	case _PC_NAME_MAX:
		result = NAME_MAX;
		break;
	case _PC_PATH_MAX:
		result = PATH_MAX;
		break;
	case _PC_PIPE_BUF:
		result = PIPE_BUF;
		break;
	case _PC_CHOWN_RESTRICTED:
#if !defined(_POSIX_CHOWN_RESTRICTED) || _POSIX_CHOWN_RESTRICTED != -1
		result = 1;
#else
		result = 0;
#endif
		break;
	case _PC_NO_TRUNC:
#if !defined(_POSIX_NO_TRUNC) || _POSIX_NO_TRUNC != -1
		result = 1;
#else
		result = 0;
#endif
		break;
	case _PC_VDISABLE:
		result = _POSIX_VDISABLE;
		break;
	default:
		errno = EINVAL;
		result = -1;
		break;
	}
	return result;
}

long
pathconf(const char *path, int name)
{
	return fpathconf(0, name);
}

long
sysconf(int name)
{
	long result;

	switch (name) {
	case _SC_ARG_MAX:
		result = ARG_MAX;
		break;
	case _SC_CHILD_MAX:
		result = CHILD_MAX;
		break;
	case _SC_CLK_TCK:
		result = CLK_TCK;
		break;
	case _SC_NGROUPS_MAX:
		result = NGROUPS_MAX;
		break;
	case _SC_OPEN_MAX:
		result = OPEN_MAX;
		break;
	case _SC_JOB_CONTROL:
#ifdef _POSIX_JOB_CONTROL
		result = 1;
#else
		result = 0;
#endif
		break;
	case _SC_SAVED_IDS:
#ifdef _POSIX_SAVED_IDS
		result = 1;
#else
		result = 0;
#endif
		break;
	case _SC_VERSION:
		result = _POSIX_VERSION;
		break;
	case _SC_STREAM_MAX:
		result = STREAM_MAX;
		break;
	case _SC_TZNAME_MAX:
		result = TZNAME_MAX;
		break;
	default:
		errno = EINVAL;
		result = -1;
		break;
	}
	return result;
}
