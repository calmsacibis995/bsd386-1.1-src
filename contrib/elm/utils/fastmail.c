
static char rcsid[] = "@(#)Id: fastmail.c,v 5.8 1993/07/20 02:46:36 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.2 $   $State: Exp $
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
 * $Log: fastmail.c,v $
 * Revision 1.2  1994/01/14  00:56:12  cp
 * 2.4.23
 *
 * Revision 5.8  1993/07/20  02:46:36  syd
 * In fastmail, if environment variable $REPLYTO is set, use it as
 * default Reply-To.  Also, eliminate unnecessary strlen() calls.
 * From: decwrl!uunet.UU.NET!fin!chip (Chip Salzenberg)
 *
 * Revision 5.7  1993/06/06  18:31:43  syd
 * fix typo
 *
 * Revision 5.6  1993/06/06  18:08:51  syd
 * Make it use the message catalog already defined
 * From: Super Y.S.T. <tabata@matsumoto.dcl.co.jp>
 *
 * Revision 5.5  1993/02/03  16:49:11  syd
 * added the RFC822 fields Comments, In-Reply-To and References.
 * to fastmail.
 * From: Greg Smith <smith@heliotrope.bucknell.edu>
 *
 * Revision 5.4  1992/11/22  01:26:12  syd
 * The fastmail utility appears to work incorrectly when multiple addresses are
 * supplied. Spaces were inserted between addresses rather than commas.
 * From: little@carina.hks.com (Jim Littlefield)
 *
 * Revision 5.3  1992/10/30  21:12:40  syd
 * Make patchlevel a text string to allow local additions to the variable
 * From: syd via a request from Dave Wolfe
 *
 * Revision 5.2  1992/10/11  00:59:39  syd
 * Fix some compiler warnings that I receive compiling Elm on my SVR4
 * machine.
 * From: Tom Moore <tmoore@fievel.DaytonOH.NCR.COM>
 *
 * Revision 5.1  1992/10/04  00:46:45  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** This program is specifically written for group mailing lists and
    such batch type mail processing.  It does NOT use aliases at all,
    it does NOT read the /etc/password file to find the From: name
    of the user and does NOT expand any addresses.  It is meant 
    purely as a front-end for either /bin/mail or /usr/lib/sendmail
    (according to what is available on the current system).

         **** This program should be used with CAUTION *****

**/

/** The calling sequence for this program is:

	fastmail {args}  [ filename | - ] full-email-address 

   where args could be any (or all) of;

	   -b bcc-list		(Blind carbon copies to)
	   -c cc-list		(carbon copies to)
	   -C comment-line      (Comments:)
	   -d			(debug on)
	   -f from 		(from name)
	   -F from-addr		(the actual address to be put in the From: line)
	   -i msg-id            (In-Reply-To: msgid)
	   -r reply-to-address 	(Reply-To:)
	   -R references        (References:)
	   -s subject 		(subject of message)
**/

#include "elmutil.h"
#include "s_fastmail.h"
#include "patchlevel.h"

#define  binrmail	"/bin/rmail"
#define  temphome	"/tmp/fastmail."

char *get_arpa_date();
static void usage();


main(argc, argv)
int argc;
char *argv[];
{

	extern char *optarg;
	extern int optind;
	FILE *tempfile;
	char hostname[NLEN], username[NLEN], from_string[SLEN], subject[SLEN];
	char filename[SLEN], tempfilename[SLEN], command_buffer[256];
	char replyto[SLEN], cc_list[SLEN], bcc_list[SLEN], to_list[SLEN];
	char from_addr[SLEN], comments[SLEN], inreplyto[NLEN];
	char references[SLEN];
	char *p;
	int  c, sendmail_available, debug = 0;

	elm_msg_cat = catopen("elm2.4", 0);

	from_string[0] = '\0';
	subject[0] = '\0';
	replyto[0] = '\0';
	cc_list[0] = '\0';
	bcc_list[0] = '\0';
	to_list[0] = '\0';
	from_addr[0] = '\0';
	comments[0] = '\0';
	inreplyto[0] = '\0';
	references[0] = '\0';

	if ((p = getenv("REPLYTO")) != NULL)
	  strcpy(replyto, p);

	while ((c = getopt(argc, argv, "b:c:C:df:F:i:r:R:s:")) != EOF) {
	  switch (c) {
	    case 'b' : strcpy(bcc_list, optarg);		break;
	    case 'c' : strcpy(cc_list, optarg);		break;
	    case 'C' : strcpy(comments, optarg);		break;
	    case 'd' : debug++;					break;	
	    case 'f' : strcpy(from_string, optarg);	break;
	    case 'F' : strcpy(from_addr, optarg);		break;
	    case 'i' : strcpy(inreplyto, optarg);		break;
	    case 'r' : strcpy(replyto, optarg);		break;
	    case 'R' : strcpy(references, optarg);		break;
	    case 's' : strcpy(subject, optarg);		break;
	    case '?' : usage();
 	  }
	}	

	if (optind >= argc) {
	  usage();
	}

	strcpy(filename, argv[optind++]);

	if (optind >= argc) {
	  usage();
	}

#ifdef HOSTCOMPILED
	strncpy(hostname, HOSTNAME, sizeof(hostname));
#else
	gethostname(hostname, sizeof(hostname));
#endif

	username[0] = '\0';
	if ((p = getlogin()) != NULL)
	  strcpy(username, p);
	if (!username[0])
	  cuserid(username);

	if (strcmp(filename, "-")) {
	  if (access(filename, READ_ACCESS) == -1) {
	    fprintf(stderr, "Error: can't find file %s!\n", filename);
	    exit(1);
	  }
	}

	sprintf(tempfilename, "%s%d", temphome, getpid());

	if ((tempfile = fopen(tempfilename, "w")) == NULL) {
	  fprintf(stderr, "Couldn't open temp file %s\n", tempfilename);
	  exit(1);
	}

	/** Subject must appear even if "null" and must be first
	    at top of headers for mail because the
	    pure System V.3 mailer, in its infinite wisdom, now
	    assumes that anything the user sends is part of the 
	    message body unless either:
		1. the "-s" flag is used (although it doesn't seem
		   to be supported on all implementations?)
		2. the first line is "Subject:".  If so, then it'll
		   read until a blank line and assume all are meant
		   to be headers.
	    So the gory solution here is to move the Subject: line
	    up to the top.  I assume it won't break anyone elses program
	    or anything anyway (besides, RFC-822 specifies that the *order*
	    of headers is irrelevant).  Gahhhhh....

	    If we have been configured for a smart mailer then we don't want
	    to add a from line.  If the user has specified one then we have
	    to honor their wishes.  If they've just given a 'from name' then
	    we'll just put in the username and hope the mailer can add the
	    correct domain in.
	**/
	fprintf(tempfile, "Subject: %s\n", subject);

	if (*from_string)
	  if (*from_addr)
	      fprintf(tempfile, "From: %s (%s)\n", from_addr, from_string);
	  else
#ifdef DONT_ADD_FROM
	      fprintf(tempfile, "From: %s (%s)\n", username, from_string);
#else
	      fprintf(tempfile, "From: %s!%s (%s)\n", hostname, username, 
		      from_string);
#endif
	else
	  if (*from_addr)
	    fprintf(tempfile, "From: %s\n", from_addr);
#ifndef DONT_ADD_FROM
	  else
	    fprintf(tempfile, "From: %s!%s\n", hostname, username);
#endif

	fprintf(tempfile, "Date: %s\n", get_arpa_date());

	if (replyto[0])
	  fprintf(tempfile, "Reply-To: %s\n", replyto);

	while (optind < argc) {
	  if (to_list[0])
	    strcat(to_list, ",");
	  strcat(to_list, argv[optind++]);
	}
	
	fprintf(tempfile, "To: %s\n", to_list);

	if (cc_list[0])
	  fprintf(tempfile, "Cc: %s\n", cc_list);

	if (references[0])
	  fprintf(tempfile, "References: %s\n", references);

	if (inreplyto[0])
	  fprintf(tempfile, "In-Reply-To: %s\n", inreplyto);

	if (comments[0])
	  fprintf(tempfile, "Comments: %s\n", comments);

#ifndef NO_XHEADER
	fprintf(tempfile, "X-Mailer: fastmail [version %s PL%s]\n",
	  VERSION, PATCHLEVEL);
#endif /* !NO_XHEADER */
	fprintf(tempfile, "\n");

	fclose(tempfile);

	/** now we'll cat both files to /bin/rmail or sendmail... **/

	sendmail_available = (access(sendmail, EXECUTE_ACCESS) != -1);

	if (debug)
		printf("Mailing to %s%s%s%s%s [via %s]\n", to_list,
			(cc_list[0] ? " ":""), cc_list,
			(bcc_list[0] ? " ":""), bcc_list,
			sendmail_available? "sendmail" : "rmail");

	sprintf(command_buffer, "cat %s %s | %s %s %s %s", 
		tempfilename, filename, 
	        sendmail_available? sendmail : mailer,
		to_list, cc_list, bcc_list);

	if (debug)
	  printf("%s\n", command_buffer);

	c = system(command_buffer);

	unlink(tempfilename);

	exit(c != 0);
}

static void usage()
{

	fprintf(stderr, catgets(elm_msg_cat, FastmailSet, FastmailUsage,
"Usage: fastmail {args} [ filename | - ] address(es)\n\
   where {args} can be;\n\
\t-b bcc-list\n\t-c cc-list\n\t-d\n\
\t-C comments\n\t-f from-name\n\t-F from-addr\n\
\t-i msg-id\n\t-r reply-to\n\t-R references\n\
\t-s subject\n\n"));

	exit(1);
}
