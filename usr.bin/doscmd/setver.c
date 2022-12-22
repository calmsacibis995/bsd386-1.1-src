/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	Krystal $Id: setver.c,v 1.1 1994/01/14 23:37:04 polk Exp $ */
#include "doscmd.h"

int ms_version = 410;

typedef struct setver_t {
    short		version;
    char		command[14];
    struct setver_t	*next;
} setver_t;

static setver_t *setver_root;

void
setver(char *cmd, short version)
{
    if (cmd) {
	setver_t *s = (setver_t *)malloc(sizeof(setver_t));

	strncpy(s->command, cmd, 14);
	s->version = version;
	s->next = setver_root;
	setver_root = s;
    } else {
	ms_version = version;
    }
}

short
getver(char *cmd)
{
    if (cmd) {
	setver_t *s = setver_root;

	while (s) {
	    if (strncasecmp(cmd, s->command, 14) == 0)
		return(s->version);
	    s = s->next;
	}
    }
    return(ms_version);
}
