
static char rcsid[] = "@(#)Id: ldstate.c,v 5.6 1993/08/23 02:46:51 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.2 $   $State: Exp $
 *
 *			Copyright (c) 1992 USENET Community Trust
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein, Elm Coordinator
 *	elm@DSI.COM			dsinc!elm
 *
 *******************************************************************************
 * $Log: ldstate.c,v $
 * Revision 1.2  1994/01/14  00:53:19  cp
 * 2.4.23
 *
 * Revision 5.6  1993/08/23  02:46:51  syd
 * Test ANSI_C, not __STDC__ (which is not set on e.g. AIX).
 * From: decwrl!uunet.UU.NET!fin!chip (Chip Salzenberg)
 *
 * Revision 5.5  1993/02/03  15:26:13  syd
 * protect atol in ifndef __STDC__ as some make it a macro, and its in stdlib.h
 *
 * Revision 5.4  1992/12/24  21:48:07  syd
 * Make fgetline elm_fgetline, as BSD 4.4 now has such a routine, and
 * it causes compile problems.
 * From: Syd
 *
 * Revision 5.3  1992/12/11  01:45:04  syd
 * remove sys/types.h include, it is now included by defs.h
 * and this routine includes defs.h or indirectly includes defs.h
 * From: Syd
 *
 * Revision 5.2  1992/12/07  04:51:34  syd
 * add sys/types.h for time_t declaration
 *
 * Revision 5.1  1992/10/03  22:41:36  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

#include <stdio.h>
#include "defs.h"

/*
 * Retrieve Elm folder state.
 *
 * The SY_DUMPSTATE option to "system_call()" causes Elm to dump the
 * current folder state before spawning a shell.  This allows programs
 * running as an Elm subprocess (e.g. "readmsg") to obtain information
 * on the folder.  (See the "system_call()" code for additional info
 * on the format of the state file.)
 *
 * This procedure returns -1 on the event of an error (corrupt state
 * file or malloc failed).
 *
 * A zero return does NOT necessarily mean that folder state information
 * was retrieved.  On a zero return, inspect the "folder_name" element.
 * If it was NULL then there was no state file found.  If it is non-NULL
 * then valid folder state information was found and loaded into
 * the (struct folder_state) record.
 */

#if !ANSI_C  /* avoid problems with systems that declare atol as a macro */
extern long atol();
#endif

static char *elm_fgetline(buf, buflen, fp)
char *buf;
unsigned buflen;
FILE *fp;
{
    if (fgets(buf, buflen, fp) == NULL)
	return (char *) NULL;
    buf[strlen(buf)-1] = '\0';
    return buf;
}


int load_folder_state_file(fst)
struct folder_state *fst;
{
    char buf[SLEN], *state_fname;
    int status, i;
    FILE *fp;

    /* clear out the folder status record */
    fst->folder_name = NULL;
    fst->num_mssgs = -1;
    fst->idx_list = NULL;
    fst->num_sel = -1;
    fst->sel_list = NULL;

    /* see if we can find a state file */
    if ((state_fname = getenv(FOLDER_STATE_ENV)) == NULL)
	return 0;
    if ((fp = fopen(state_fname, "r")) == NULL)
	return 0;

    /* initialize status to failure */
    status = -1;

    /* retrieve pathname of the folder */
    if (elm_fgetline(buf, sizeof(buf), fp) == NULL || buf[0] != 'F')
	goto done;
    if ((fst->folder_name = malloc(strlen(buf+1) + 1)) == NULL)
	goto done;
    (void) strcpy(fst->folder_name, buf+1);

    /* retrieve number of messages in the folder */
    if (elm_fgetline(buf, sizeof(buf), fp) == NULL || buf[0] != 'N')
	goto done;
    fst->num_mssgs = atoi(buf+1);

    /* allocate space to hold the indices */
    fst->idx_list = (long *) malloc(fst->num_mssgs * sizeof(long));
    if (fst->idx_list == NULL)
	goto done;

    /* load in the indices of the messages */
    for (i = 0 ; i < fst->num_mssgs ; ++i) {
	if (elm_fgetline(buf, sizeof(buf), fp) == NULL || buf[0] != 'I')
	    goto done;
	fst->idx_list[i] = atol(buf+1);
    }

    /* load in the number of messages selected */
    if (elm_fgetline(buf, sizeof(buf), fp) == NULL || buf[0] != 'C')
	goto done;
    fst->num_sel = atoi(buf+1);

    /* it is possible that there are no selections */
    if (fst->num_sel > 0) {

	/* allocate space to hold the list of selected messages */
	fst->sel_list = (int *) malloc(fst->num_sel * sizeof(int));
	if (fst->sel_list == NULL)
	    goto done;

	/* load in the list of selected messages */
	for (i = 0 ; i < fst->num_sel ; ++i) {
	    if (elm_fgetline(buf, sizeof(buf), fp) == NULL || buf[0] != 'S')
		goto done;
	    fst->sel_list[i] = atoi(buf+1);
	}

    }

    /* that should be the end of the file */
    if (elm_fgetline(buf, sizeof(buf), fp) != NULL)
	goto done;

    /* success */
    status = 0;

done:
    (void) fclose(fp);
    return status;
}

