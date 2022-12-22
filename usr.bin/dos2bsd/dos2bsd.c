
/* Copyright 1992 BSDI */

#include <stdio.h> 

main(argc, argv)
	int argc;
	char *argv[];
{
	FILE *sfp, *dfp;
	char buf[8192];
	int bufsiz;

	if (argc != 3) {
		printf("usage: dos2bsd sourcefile convertedfile\n");
		exit(-1);
	} 

	if (!(sfp = fopen(argv[1], "r"))) {
		perror(argv[1]);
		exit(-1);
	}
	if (!(dfp = fopen(argv[2], "w"))) {
		perror(argv[2]);
		exit(-1);
	}
	while (fgets(buf, 8192, sfp)) {
		bufsiz = strlen(buf);
		if (buf[bufsiz-2] == '\r') {		/* remove ^M */
			buf[bufsiz-2] = '\n';
			buf[bufsiz-1] = '\0';
		}
		fputs(buf, dfp);
	}
	
	fclose(sfp);
	fclose(dfp);

	exit(0);
}
