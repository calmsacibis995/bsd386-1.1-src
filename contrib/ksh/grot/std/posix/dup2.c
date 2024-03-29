/*
 * Cheap imitation of BSD dup2()
 */
/* $Id: dup2.c,v 1.2 1993/12/21 02:19:49 polk Exp $ */

#include <fcntl.h>

#if _SYSV
int
dup2( oldd, newd )
int	oldd, newd;
{
	int	fd;

	if (fcntl( oldd, F_GETFL, 0 ) < 0)
		return( -1 );

	(void) close( newd );
	fd = fcntl( oldd, F_DUPFD, newd );

	return( (fd > newd) ? -1 : fd );
}
#endif
