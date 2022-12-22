
static char rcsid[] = "@(#)Id: putenv.c,v 5.1 1992/10/03 22:41:36 syd Exp ";

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
 * $Log: putenv.c,v $
 * Revision 1.2  1994/01/14  00:53:40  cp
 * 2.4.23
 *
 * Revision 5.1  1992/10/03  22:41:36  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/*
 * This code was stolen from cnews.  Modified to make "newenv" static so
 * that realloc() can be used on subsequent calls to avoid memory leaks.
 *
 * We only need this if Configure said there isn't a putenv() in libc.
 */

#include "headers.h"

#ifndef PUTENV /*{*/

/* peculiar return values */
#define WORKED 0
#define FAILED 1

int
putenv(var)			/* put var in the environment */
char *var;
{
	register char **envp;
	register int oldenvcnt;
	extern char **environ;
	static char **newenv = NULL;

	/* count variables, look for var */
	for (envp = environ; *envp != 0; envp++) {
		register char *varp = var, *ep = *envp;
		register int namesame;

		namesame = NO;
		for (; *varp == *ep && *varp != '\0'; ++ep, ++varp)
			if (*varp == '=')
				namesame = YES;
		if (*varp == *ep && *ep == '\0')
			return WORKED;	/* old & new var's are the same */
		if (namesame) {
			*envp = var;	/* replace var with new value */
			return WORKED;
		}
	}
	oldenvcnt = envp - environ;

	/* allocate new environment with room for one more variable */
	if (newenv == NULL)
	    newenv = (char **)malloc((unsigned)((oldenvcnt+1+1)*sizeof(*envp)));
	else
	    newenv = (char **)realloc((char *)newenv, (unsigned)((oldenvcnt+1+1)*sizeof(*envp)));
	if (newenv == NULL)
		return FAILED;

	/* copy old environment pointers, add var, switch environments */
	(void) bcopy((char *)environ, (char *)newenv, oldenvcnt*sizeof(*envp));
	newenv[oldenvcnt] = var;
	newenv[oldenvcnt+1] = NULL;
	environ = newenv;
	return WORKED;
}

#endif /*}PUTENV*/

