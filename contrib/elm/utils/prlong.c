
static char rcsid[] = "@(#)Id: prlong.c,v 5.3 1993/08/03 19:28:39 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.1 $
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
 * $Log: prlong.c,v $
 * Revision 1.1  1994/01/14  01:35:16  cp
 * 2.4.23
 *
 * Revision 5.3  1993/08/03  19:28:39  syd
 * Elm tries to replace the system toupper() and tolower() on current
 * BSD systems, which is unnecessary.  Even worse, the replacements
 * collide during linking with routines in isctype.o.  This patch adds
 * a Configure test to determine whether replacements are really needed
 * (BROKE_CTYPE definition).  The <ctype.h> header file is now included
 * globally through hdrs/defs.h and the BROKE_CTYPE patchup is handled
 * there.  Inclusion of <ctype.h> was removed from *all* the individual
 * files, and the toupper() and tolower() routines in lib/opt_utils.c
 * were dropped.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.2  1993/04/21  01:41:14  syd
 * Needs ctype.h
 * From: Syd
 *
 * Revision 5.1  1993/04/12  02:09:44  syd
 * Initial checkin
 *
 *
 ******************************************************************************/


/*
 * Format large amounts of output, folding across multiple lines.
 *
 * Input is read in a line at a time, and the input records are joined
 * together into output records up to a maximum line width.  A field
 * seperator is placed between every input record, and a configurable
 * leader is printed at the front of each line.  The default field
 * seperator is a single space.  The default output leader is an empty
 * string for the first output line, and a single tab for all subsequent
 * lines.
 *
 * Usage:
 *
 *	prlong [-w wid] [-1 first_leader] [-l leader] [-f sep] < data
 *
 * Options:
 *
 *	-w wid		Maximum output line width.
 *	-1 leader	Leader for the first line of output.
 *	-l leader	Leader for all subsequent lines of output.
 *	-f sep		Field seperator.
 *
 * Example:
 *
 *    $ elmalias -en friends | /usr/lib/elm/prlong -w40
 *    tom@sleepy.acme.com (Tom Smith) 
 *	    dick@dopey.acme.com (Dick Jones) 
 *	    harry@grumpy.acme.com
 *    $ elmalias -en friends | /usr/lib/elm/prlong -w40 -1 "To: " -f ", "
 *    To: tom@sleepy.acme.com (Tom Smith), 
 *	    dick@dopey.acme.com (Dick Jones), 
 *	    harry@grumpy.acme.com
 */


#include <stdio.h>
#include "defs.h"

#define MAXWID		78	/* default maximum line width		*/
#define ONE_LDR		""	/* default leader for first line	*/
#define DFLT_LDR	"\t"	/* default leader for other lines	*/
#define FLDSEP		" "	/* default field seperator		*/

char inbuf[1024];		/* space to hold an input record	*/
char outbuf[4096];		/* space to accumulate output record	*/

int calc_col();			/* calculate output column position	*/


void usage_error(prog)
char *prog;
{
    fprintf(stderr,
	"usage: %s [-w wid] [-1 first_leader] [-l leader] [-f sep]\n", prog);
    exit(1);
}


main(argc, argv)
int argc;
char *argv[];
{
    char *one_ldr;		/* output leader to use on first line	*/
    char *dflt_ldr;		/* output leader for subsequent lines	*/
    char *fld_sep;		/* selected field seperator		*/
    char *curr_sep;		/* text to output before next field	*/
    int maxwid;			/* maximum output line width		*/
    int outb_col;		/* current column pos in output rec	*/
    int outb_len;		/* current length of output record	*/
    int i;
    extern int optind;
    extern char *optarg;

#ifdef I_LOCALE
    setlocale(LC_ALL, "");
#endif

    /*
     * Initialize defaults.
     */
    maxwid = MAXWID;
    one_ldr = ONE_LDR;
    dflt_ldr = DFLT_LDR;
    fld_sep = FLDSEP;

    /*
     * Crack command line.
     */
    while ((i = getopt(argc, argv, "w:1:l:f:")) != EOF) {
	switch (i) {
	    case 'w':	maxwid = atoi(optarg);	break;
	    case '1':	one_ldr = optarg;	break;
	    case 'l':	dflt_ldr = optarg;	break;
	    case 'f':	fld_sep = optarg;	break;
	    default:	usage_error(argv[0]);
	}
    }
    if (optind != argc)
	usage_error(argv[0]);

    /*
     * Initialize output buffer.
     */
    (void) strfcpy(outbuf, one_ldr, sizeof(outbuf));
    outb_col = calc_col(0, one_ldr);
    outb_len = strlen(one_ldr);
    curr_sep = "";

    /*
     * Process the input a line at a time.
     */
    while (fgets(inbuf, sizeof(inbuf), stdin) != NULL) {

	/*
	 * Trim trailing space.  Skip blank lines.
	 */
	for (i = strlen(inbuf) - 1 ; i >= 0 && isspace(inbuf[i]) ; --i)
		;
	inbuf[i+1] = '\0';
	if (inbuf[0] == '\0')
		continue;

	/*
	 * If this text exceeds the line length then print the stored
	 * info and reset the line.
	 */
	if (calc_col(calc_col(outb_col, curr_sep), inbuf) >= maxwid) {
	    printf("%s%s\n", outbuf, curr_sep);
	    curr_sep = dflt_ldr;
	    outb_col = 0;
	    outb_len = 0;
	    outbuf[0] = '\0';
	}

	/*
	 * Append the current field seperator to the stored info.
	 */
	(void) strfcpy(outbuf+outb_len, curr_sep, sizeof(outbuf)-outb_len);
	outb_col = calc_col(outb_col, outbuf+outb_len);
	outb_len += strlen(outbuf+outb_len);

	/*
	 * Append the text to the stored info.
	 */
	(void) strfcpy(outbuf+outb_len, inbuf, sizeof(outbuf)-outb_len);
	outb_col = calc_col(outb_col, outbuf+outb_len);
	outb_len += strlen(outbuf+outb_len);

	/*
	 * Enable the field seperator.
	 */
	curr_sep = fld_sep;

    }

    if (*outbuf != '\0')
	puts(outbuf);
    exit(0);
}


int calc_col(col, s)
register int col;
register char *s;
{
    while (*s != '\0') {
	switch (*s) {
	    case '\b':	--col;			break;
	    case '\r':	col = 0;		break;
	    case '\t':	col = ((col + 8) & ~7);	break;
	    default:	++col;			break;
	}
	++s;
    }
    return col;
}

