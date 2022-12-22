/*
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

static const char rcsid[] = "$Id: line.c,v 1.1.1.1 1993/03/08 06:43:33 polk Exp $";
static const char copyright[] = "Copyright (c) 1993 Berkeley Software Design, Inc.";

/* line - read one line from stdin and echo it
 * vix 16feb93 [original - for bsdi and jay libove]
 */

#include <stdio.h>

main(argc, argv)
	int argc;
	char **argv;
{
	int c;

	if (argc != 1) {
		fprintf(stderr, "usage: %s\n", argv[0]);
		return (2);
	}

	while ((c = getchar()) != EOF)
		if (putchar(c) == '\n')
			return (0);

	putchar('\n');
	return (1);
}
