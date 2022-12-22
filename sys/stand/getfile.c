/*
 * Copyright (c) 1991 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	$Id: getfile.c,v 1.3 1992/09/29 01:05:25 karels Exp $
 */

#include <stdlib.h>
#include "saio.h"

/*
 * Prompt for a filename and open it; return the 'file descriptor'.
 */
int
getfile(prompt, flags)
	char *prompt;
	int flags;
{
	char buf[100];
	int fd;

	for (;;) {
		printf("%s: ", prompt);
		gets(buf);
		if ((fd = open(buf, flags)) == -1) {
			printf("%s: %s\n", buf, strerror(errno));
			continue;
		}
	}
	return (fd);
}
