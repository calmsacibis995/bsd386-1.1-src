
static char rcsid[] = "@(#)Id: realfrom.c,v 5.4 1993/08/03 19:28:39 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.1 $   $State: Exp $
 *
 * 			Copyright (c) 1993 USENET Community Trust
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein, Elm Coordinator
 *	elm@DSI.COM			dsinc!elm
 *
 *******************************************************************************
 * $Log: realfrom.c,v $
 * Revision 1.1  1994/01/14  01:35:01  cp
 * 2.4.23
 *
 * Revision 5.4  1993/08/03  19:28:39  syd
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
 * Revision 5.3  1993/05/08  17:09:56  syd
 * Fix problem where "<space>From" was recognized as a valid "From_" line.
 * From: decwrl!decvax!gatech!WLK.COM!chip (Chip Rosenthal)
 *
 * Revision 5.2  1993/04/12  01:24:59  syd
 * Have real_from() ignore bogus timezones in From_ lines
 * rather than bailing out with an error.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.1  1993/01/19  04:46:21  syd
 * Initial Checkin
 *
 *
 ******************************************************************************/


#include "headers.h"


extern long make_gmttime();


int
real_from(buffer, entry)
char *buffer;
struct header_rec *entry;
{

    /*
     * Breakup and validate the "From_" line in the "buffer".  If "entry"
     * is not NULL then the structure is filled in with sender and time
     * information.  Returns TRUE if the "From_" line is valid, otherwise
     * FALSE.
     *
     * A valid from line will be in the following format:
     *
     *	From <user> <weekday> <month> <day> <hr:min:sec>
     *	    [TZ1 [TZ2]] <year> [remote from sitelist]
     *
     * We insist that all of the <angle bracket> fields are present.
     * If two timezone fields are present, the first is used for date
     * information.  We do not look at anything beyond the <year> field.
     * We just insist that everything up to the <year> field is present
     * and valid.
     */

    char field[STRING];		/* buffer for current field of line	*/
    char save_tz[STRING];	/* extracted time zone field		*/
    int len;			/* length of current field		*/
    int month, day, year, hours, mins, secs, tz, i;

    /*
     * Zero out the portions of the record we fill in.
     */
    if (entry != NULL) {
	entry->time_zone[0] = '\0';
	entry->from[0] = '\0';
	entry->time_sent = 0;
	entry->received_time = 0;
	entry->tz_offset = 0;
    }

    /* From */
    if (strncmp(buffer, "From ", 5) != 0)
	goto failed;
    buffer += 5;
    dprint(7, (debugfile, "real_from parsing %s", buffer));

    /* <user> */
    if ((len = get_word(buffer, 0, field, sizeof(field))) < 0)
	goto failed;
    buffer += len;
    if (entry != NULL)
	strfcpy(entry->from, field, sizeof(entry->from));
    dprint(7, (debugfile, "  user=%s\n", field));

    /* <weekday> */
    if ((len = get_word(buffer, 0, field, sizeof(field))) < 0)
	goto failed;
    buffer += len;

    /* <month> */
    if ((len = get_word(buffer, 0, field, sizeof(field))) < 0 ||
	    !cvt_monthname_to_monthnum(field, &month))
	goto failed;
    buffer += len;

    /* <day> */
    if ((len = get_word(buffer, 0, field, sizeof(field))) < 0 ||
	    (day = atonum(field)) < 0 || day < 1 || day > 31)
	goto failed;
    buffer += len;

    /* <hr:min:sec> */
    if ((len = get_word(buffer, 0, field, sizeof(field))) < 0 ||
	    !cvt_timestr_to_hhmmss(field, &hours, &mins, &secs))
	goto failed;
    buffer += len;
    dprint(7, (debugfile, "  hours=%d mins=%d secs=%d\n", hours, mins, secs));

    /*
     * [ <tz> ... ] <year>
     *
     * This is messy.  Not only is the timezone field optional, there
     * might be multiple fields (e.g. "MET DST"), or it might be entirely
     * bogus (e.g. some MTAs produce "0600" instead of "+0600".
     */
    tz = 0;
    save_tz[0] = save_tz[1] = '\0';
    for (;;) {

	if ((len = get_word(buffer, 0, field, sizeof(field))) < 0)
	    goto failed;
	buffer += len;

	/*
	 * First check if this is a TZ field.  If so, pull in the info
	 * and continue onto the next field.
	 */
	if (cvt_timezone_to_offset(field, &i)) {
	    tz += i;
	    i = strlen(save_tz);
	    (void) strfcpy(save_tz+i, " ", sizeof(save_tz)-i);
	    ++i;
	    (void) strfcpy(save_tz+i, field, sizeof(save_tz)-i);
	    continue;
	}

	/*
	 * If this isn't a valid TZ then it should be a year.  If so
	 * then save off the year info and break out of this loop.
	 */
	if (cvt_yearstr_to_yearnum(field, &year))
	    break;

	/*
	 * This isn't either a valid TZ or year.  Assume it is a bogus
	 * timezone we don't understand, and continue processing the line.
	 */
	dprint(7, (debugfile,
	    "  assuming \"%s\" is a bogus timezone - skipping it\n", field));

    }
    if (entry != NULL) {
	entry->tz_offset = tz * 60;
	(void) strfcpy(entry->time_zone, save_tz+1, sizeof(entry->time_zone));
	entry->received_time = entry->time_sent =
	    make_gmttime(year, month, day, hours, mins-tz, secs);
    }
    dprint(7, (debugfile, "  tz=%s tz_offset=%d\n", save_tz+1, tz));
    dprint(7, (debugfile, "  month=%d day=%d year=%d\n", month, day, year));

    /*
     * The line is parsed and valid.  There might be more but we don't care.
     */
    dprint(7, (debugfile, "  return success\n"));
    return TRUE;

failed:
    dprint(4, (debugfile, "real_from failed at \"%s\"\n", 
	(len <= 0 ? "<premature eol>" : field)));
    return FALSE;
}


#ifdef _TEST
int debug = 9999;
FILE *debugfile = stdout;
main()
{
    struct header_rec hdr;
    char buf[1024];
    extern char *ctime();

    while (gets(buf) != NULL) {
	if (!real_from(buf, &hdr))
	    printf("FAIL %s\n", buf);
	else {
	    printf("OK %s\n", buf);
	    printf("from=%s time_zone=%s tz_offset=%d\n",
		hdr.from, hdr.time_zone, hdr.tz_offset);
	    printf("time_sent=%ld received_time=%ld %s",
		hdr.time_sent, hdr.received_time, ctime(&hdr.received_time));
	}
	putchar('\n');
    }
    exit(0);
}
#endif

