/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
 
/*	BSDI $Id: configsl.c,v 1.2 1993/03/18 17:46:20 polk Exp $	*/

#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include "pathnames.h"

char *query();
char *laddr, *raddr, *port, *speed, *mdial, *login, *passwd;

/*
 * EZ-SLIP link creation tool.
 */
main(argc, argv)
	int argc;
	char *argv[];
{
	char *ans;
	char cmd[BUFSIZ];
	char *p;

	printf("\n\nBSD/386 dialup SLIP configuration utility\n");
	printf("\nTo use this tool, you will need to know: \n");
	printf("\n\t1) Your local SLIP link IP address\n");
	printf("\t2) The remote SLIP link IP address\n");
	printf("\t3) What serial port your modem is connected to\n");
	printf("\t4) String needed to dial your modem\n");
	printf("\t5) Your login name and password on remote system\n");

	ans = query("Proceed with dialup SLIP configuration?", "yes", NULL);
	if ((ans[0] != 'Y') && (ans[0] != 'y')) {
		exit(-1);
	}

	getinfo();

	ans = query("Ready to write /usr/local/bin/slipup. Proceed?", "yes", 
	    NULL);
	if ((ans[0] != 'Y') && (ans[0] != 'y')) {
		exit(-1);
	}

	printf("\nWriting slipup script..."); 
	fflush(stdout);
	writeinfo();
	printf("complete.\n"); 

	printf("\n");
	printf("You should now be able to bring up your SLIP line by typing\n");
	printf("\n\t/usr/local/bin/slipup\n");
	printf("\nIf you have difficulties, re-run configsl(8) or edit /usr/local/bin/slipup\n");
	printf("by hand.\n");

	exit(0);
}

/*
 * Prompt the user for data using the given prompt string.  Accept
 * the default if one is specified and/or require the response to
 * match one of the specified choices.
 */
char *
query(prompt, deflt, nonnull)
	char *prompt, *deflt;
	int nonnull;
{
	char **p;
	char pbuf[BUFSIZ];
	static char abuf[BUFSIZ];
	char *strdup();

	if (deflt == NULL)
		(void) sprintf(pbuf, "\n%s: ", prompt);
	else
		(void) sprintf(pbuf, "\n%s [%s]: ", prompt, deflt);
	for (;;) {
		printf(pbuf);
		fflush(stdout);
		if (fgets(abuf, BUFSIZ, stdin) == NULL)
			fprintf(stderr, "EOF -- exiting\n");
		abuf[strlen(abuf) - 1] = '\0';
		if (abuf[0] == '\0' && deflt != NULL) {
			(void) strcpy(abuf, deflt);
			break;
		}
		if (nonnull == NULL)
			break;
		if (abuf[0] != '\0')
			goto done;
		printf("Invalid response: %s\n", abuf);
	}
done:
	return strdup(abuf);
}

/*
 * get pertinent information about the slip line
 */
getinfo()
{

	laddr = query("What is your local SLIP link address?",
		NULL, 1);
	raddr = query("What is your remote SLIP link address?",
		NULL, 1);
	port = query("What port is your modem attached to?",
		"/dev/tty00", 1);
	speed = query("What baud rate is your modem interface?",
		"9600", 1);
	mdial = query("What string should be used to dial your modem?",
		"ATDT5551234", NULL);
	login = query("What is your SLIP login name on the remote system?",
		"Smylogin", 1);
	passwd = query("What is your SLIP login password on the remote system?",
		"go,slip", 1);
}

/*
 * write out script to bring up SLIP line
 */
writeinfo()
{
	FILE *fp;

	if (!(fp = fopen(_PATH_SLIPUP, "w"))) {
		fprintf(stderr, "Couldn't open %s\n", _PATH_SLIPUP);
		exit(-1);
	}
	
	fprintf(fp, "#!/bin/sh\n");
	fprintf(fp, "%s sl0 inet %s %s\n", _PATH_IFCONFIG, 
		laddr, raddr);
	fprintf(fp, "%s -b %s -s \"%s\" %s %s %s\n",
		_PATH_STARTSLIP, speed, mdial, port, login, passwd);
	fprintf(fp, "%s add default %s\n", _PATH_ROUTE, raddr);

	fclose(fp);
	chmod(_PATH_SLIPUP, 0755);
}
