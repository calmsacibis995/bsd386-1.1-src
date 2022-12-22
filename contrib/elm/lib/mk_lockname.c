
static char rcsid[] = "@(#)Id: mk_lockname.c,v 5.1 1992/10/03 22:41:36 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.2 $   $State: Exp $
 *
 *			Copyright (c) 1988-1992 USENET Community Trust
 *			Copyright (c) 1986,1987 Dave Taylor
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein, Elm Coordinator
 *	elm@DSI.COM			dsinc!elm
 *
 *******************************************************************************
 * $Log: mk_lockname.c,v $
 * Revision 1.2  1994/01/14  00:53:28  cp
 * 2.4.23
 *
 * Revision 5.1  1992/10/03  22:41:36  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include "defs.h"

static char lock_name[SLEN];

char * mk_lockname(file_to_lock)
char *file_to_lock;
{
      /** Create the proper name of the lock file for file_to_lock,
            which is presumed to be a spool file full path (see
            get_folder_type()), and put it in the static area lock_name.
            Return lock_name for informational purposes.
       **/

#ifdef XENIX
        /* lock is /tmp/[basename of file_to_lock].mlk */
        sprintf(lock_name, "/tmp/%.10s.mlk", rindex(file_to_lock, '/')+1);
#else
        /* lock is [file_to_lock].lock */
        sprintf(lock_name, "%s.lock", file_to_lock);
#endif
        return(lock_name);
}

