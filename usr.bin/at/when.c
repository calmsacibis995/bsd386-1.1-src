/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
 
/*	BSDI when.c,v 1.1 1992/08/24 21:18:11 sanders Exp	*/

#ifndef lint
static char sccsid[] = "@(#)when.c	1.1 (BSDI) 08/24/92";
#endif /* lint */

/*-
 * when.c
 *
 * Function:	Parse argv into a time_t
 *
 * Author:	Tony Sanders
 * Date:	08/17/92
 *
 * Remarks:
 * History:	08/17/92 Tony Sanders -- creation
 */
 
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "at.h"
#include "when.h"
#include "errlib.h"

#ifdef DEBUG_WHEN
main(int argc, char **argv)
{
	time_t event;

	argc--, argv++;
	event = when(&argc, &argv);
	printf("event at %d\n", event);
	print_time(event);
	printf("\n");
	exit(0);
}
#endif

time_t
when(int *argc, char **argv[])
{
	struct	tm *tp;
	time_t	event;

	/* init lexer data */
	yy_ac = *argc;
	yy_av = *argv;

	/* init parser data */
	if (time(&now) < 0)
		Perror("There's no time like the present");
	tp = gmtime(&now);
	gm_time = *tp;
	tp = localtime(&now);
	lc_time = *tp;
	time_v = *tp;
	zulu_offset = 0;

	yyparse();

	Pmsg(("parsed_buf=``%s''\n", parsed_buf+1));

	event = mktime(&time_v) + zulu_offset;
	if (event < now)
		Perror("time specified is before the current time");

	/*
	 * unwind one argument
	 */
	if (!parsed_eof)
		yy_ac++, yy_av--;

	*argc = yy_ac;
	*argv = yy_av;
	return (mktime(&time_v) + zulu_offset);
}

void
print_tm(struct tm *tp)
{
	char *dayname = "SunMonTueWedThuFriSat";
	char *monname = "JanFebMarAprMayJunJulAugSepOctNovDec";

	printf("%.3s %.3s %02d %02d:%02d:%02d %4d %s",
		dayname + (tp->tm_wday*3),
		monname + (tp->tm_mon*3),
		tp->tm_mday,
		tp->tm_hour, tp->tm_min, tp->tm_sec,
		tp->tm_year + 1900,
		tp->tm_zone);
}

void
print_time(time_t t)
{
	print_tm(localtime(&t));
}
