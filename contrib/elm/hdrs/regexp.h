
/* $Id: regexp.h,v 5.3 1992/11/15 01:38:23 syd Exp  */

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.2 $   $State: Exp $
 *
 * 			Copyright (c) 1992 USENET Community Trust
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein, Elm Coordinator
 *	elm@DSI.COM			dsinc!elm
 *
 *******************************************************************************
 * $Log: regexp.h,v $
 * Revision 1.2  1994/01/14  00:52:45  cp
 * 2.4.23
 *
 * Revision 5.3  1992/11/15  01:38:23  syd
 * fix headers
 *
 * Revision 5.2  1992/11/15  01:37:57  syd
 * add proper headers
 *
 *
 ******************************************************************************/

/*
 * Definitions etc. for regexp(3) routines.
 *
 * Caveat:  this is V8 regexp(3) [actually, a reimplementation thereof],
 * not the System V one.
 */
#define NSUBEXP  10
typedef struct regexp {
	char *startp[NSUBEXP];
	char *endp[NSUBEXP];
	char regstart;		/* Internal use only. */
	char reganch;		/* Internal use only. */
	char *regmust;		/* Internal use only. */
	int regmlen;		/* Internal use only. */
	char program[1];	/* Unwarranted chumminess with compiler. */
} regexp;

extern regexp *regcomp();
extern int regexec();
extern void regsub();
extern void regerror();
