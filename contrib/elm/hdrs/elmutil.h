
/* $Id: elmutil.h,v 5.1 1992/10/03 22:34:39 syd Exp  */

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.2 $   $State: Exp $
 *
 * 			Copyright (c) 1988-1992 USENET Community Trust
 * 			Copyright (c) 1986,1987 Dave Taylor
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein, Elm Coordinator
 *	elm@DSI.COM			dsinc!elm
 *
 *******************************************************************************
 * $Log: elmutil.h,v $
 * Revision 1.2  1994/01/14  00:52:35  cp
 * 2.4.23
 *
 * Revision 5.1  1992/10/03  22:34:39  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/**  Main header file for ELM utilities.  **/


#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

#include "../hdrs/curses.h"
#include "../hdrs/defs.h"

/******** static character string containing the version number  *******/

static char ident[] = { WHAT_STRING };

/******** and another string for the copyright notice            ********/

static char copyright[] = { 
		"@(#)          (C) Copyright 1986,1987, Dave Taylor\n@(#)          (C) Copyright 1988-1992, The Usenet Community Trust\n" };

/******** global variables accessable by all pieces of the program *******/

char hostname[SLEN] = {0};      /* name of machine we're on*/
char hostdomain[SLEN] = {0};    /* name of domain we're in */
char hostfullname[SLEN] = {0};  /* name of FQDN we're in */
char username[SLEN] = {0};      /* return address name!    */
char full_username[SLEN] = {0}; /* Full username - gecos   */
nl_catd elm_msg_cat = 0;	/* message catalog	    */
int userid;			/* uid for current user	      */
int groupid;			/* groupid for current user   */

struct addr_rec *alternative_addresses = NULL;	/* can't do it without elmrc */

extern char *gcos_name();
extern char *get_full_name();
