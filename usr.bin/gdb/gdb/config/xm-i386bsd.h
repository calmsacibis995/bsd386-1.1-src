/*	BSDI $Id: xm-i386bsd.h,v 1.1.1.1 1992/08/27 17:03:50 trent Exp $	*/

/*
 * BSD/386 macro definitions.
 */

#define	FETCH_INFERIOR_REGISTERS	1 
#define	HAVE_BSD_MMAP			1
#define	HAVE_MMAP			1
#define	HAVE_SIGSETMASK			1
#define	HAVE_WAIT_STRUCT		1
#define	HOST_BYTE_ORDER			LITTLE_ENDIAN
#define	MMAP_BASE_ADDRESS		0x81000000
#define	MMAP_INCREMENT			0x01000000
#define	NEED_POSIX_SETPGID		1
#define	NO_MALLOC_CHECK			1
#define	SET_STACK_LIMIT_HUGE		1

#include <limits.h>

static __inline int
mmtrace(void) { return 0; }
