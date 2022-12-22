/*	BSDI	$Id: bsd2dos.c,v 1.2 1993/12/21 03:51:06 polk Exp $	*/

/*
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

#include <stdio.h> 

main(argc, argv)
	int argc;
	char *argv[];
{
	FILE *sfp, *dfp;
	char buf[8192];
	int bufsiz;

	sfp = stdin;
	dfp = stdout;

	if (argc > 3) {
		printf("usage: bsd2dos [sourcefile [convertedfile]]\n");
		exit(-1);
	} 
	if (argc >= 2)
		if (!(sfp = fopen(argv[1], "r"))) {
			perror(argv[1]);
			exit(-1);
		}
	if (argc == 3)
		if (!(dfp = fopen(argv[2], "w"))) {
			perror(argv[2]);
			exit(-1);
		}

	while (fgets(buf, sizeof(buf)-1, sfp)) {
		bufsiz = strlen(buf);
		if (buf[bufsiz-1] == '\n') {
			buf[bufsiz-1] = '\r';		/* add a ^M */
			buf[bufsiz] = '\n';
			buf[bufsiz+1] = '\0';
		}
		fputs(buf, dfp);
	}
	
	fclose(sfp);
	fclose(dfp);

	exit(0);
}
