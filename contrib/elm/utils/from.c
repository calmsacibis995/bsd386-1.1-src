
static char rcsid[] = "@(#)Id: from.c,v 5.14 1993/05/31 19:36:07 syd Exp ";

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
 * $Log: from.c,v $
 * Revision 1.2  1994/01/14  00:56:14  cp
 * 2.4.23
 *
 * Revision 5.14  1993/05/31  19:36:07  syd
 * Dave Thomas forgot to update the NLS message file when he added the tidy
 * option to frm.  While I was at it, I did a little cleanup to keep things
 * alphabetized.
 * From: dwolfe@pffft.sps.mot.com (Dave Wolfe)
 *
 * Revision 5.13  1993/05/14  03:58:37  syd
 * This is a trivial patch to 'from.c' to tidy up the output is
 * the cases where the 'from' part is longer that 20 characters.
 * It adds the new '-t' (for tidy) option:
 * From: dave@devteq.co.uk (Dave Thomas)
 *
 * Revision 5.12  1993/05/14  03:57:10  syd
 * When frm checked for file access on a POSIX system there
 * was a test `&& S_ISREG(mode)' instead of `&& ! S_ISREG(mode)'
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.11  1993/04/21  01:18:09  syd
 * frm treated a line with From_ preceeded by whitespace as a valid
 * message delimiter.
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.10  1993/04/12  03:33:39  syd
 * the posix macros to interpret the result of the stat-call.
 * From: vogt@isa.de (Gerald Vogt)
 *
 * Revision 5.9  1993/04/12  02:25:16  syd
 * remove extra blank in the new  messages line
 *
 * Revision 5.8  1993/01/27  21:07:55  syd
 * if no files are given to frm, and it cannot open the mail spool,
 * print No mail.
 *
 * Revision 5.7  1993/01/27  20:52:20  syd
 * fixed the behaviour of the tool nfrm or frm -snew to be inconsistent
 * with elm itself. In from.c it never recognized the file in the MAIL
 * environment variable to be a SPOOL file as you say in the source.
 * From: Erick Otto <eotto@hvlpa.ns-nl.att.com>
 *
 * Revision 5.6  1993/01/19  05:07:05  syd
 * Trim erroreous extra log entry
 * From: Syd
 *
 * Revision 5.5  1993/01/19  04:47:12  syd
 * Significant changes to provide consistent Date and From_ header
 * cracking.  Overhauled date utilities and moved into library.  Moved
 * real_from() into library.  Modified frm, newmail, and readmsg utilities
 * to use library version of real_from().  Moved get_word() from Elm
 * source into library.  Added new library routines atonum() and strfcpy().
 * Fixed trailing backslash bug in len_next().
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.4  1992/12/11  01:45:04  syd
 * remove sys/types.h include, it is now included by defs.h
 * and this routine includes defs.h or indirectly includes defs.h
 * From: Syd
 *
 * Revision 5.3  1992/11/07  21:03:33  syd
 * fix typo
 *
 * Revision 5.2  1992/11/07  20:05:52  syd
 * change to use header_cmp to allow for linear white space around the colon
 * From: Syd
 *
 * Revision 5.1  1992/10/04  00:46:45  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** print out whom each message is from in the pending folder or specified 
    one, including a subject line if available.

**/

#include "elmutil.h"
#include "s_from.h"
     
#ifdef PWDINSYS
#  include <sys/pwd.h>
#else
#  include <pwd.h>
#endif
#include <sys/stat.h>

#define LINEFEED	(char) 10

#define metachar(c)	(c == '=' || c == '+' || c == '%')

/* ancient wisdom */
#ifndef TRUE
#  define TRUE	1
#  define FALSE	0
#endif

/* for explain(), positive and negative */
#define POS	1
#define NEG	0

/* defines for selecting messages by Status: */
#define NEW_MSG		0x1
#define OLD_MSG		0x2
#define READ_MSG	0x4
#define UNKNOWN		0x8

#define ALL_MSGS	0xf

/* exit statuses */
#define	EXIT_SELECTED	0	/* Selected messages present */
#define	EXIT_MAIL	1	/* Mail present, but no selected messages */
#define	EXIT_NO_MAIL	2	/* No messages at all */
#define	EXIT_ERROR	3	/* Error */

FILE *mailfile;

int   number = FALSE,	/* should we number the messages?? */
      veryquiet = FALSE,/* should we be print any output at all? */
      quiet = FALSE,	/* only print mail/no mail and/or summary */
      selct = FALSE,	/* select these types of messages */
      tidy  = FALSE,    /* tidy output with long 'from's */
      summarize = FALSE,/* print a summary of how many messages of each type */
      verbose = FALSE;	/* and should we prepend a header? */

char infile[SLEN];	/* current file name */
char defaultfile[SLEN];	/* default file name */
char realname[SLEN];	/* the username of the user who ran the program */

extern char *whos_mail(), *explain();

#ifdef DEBUG
int debug = 0;
FILE *debugfile = stderr;
#endif


main(argc, argv)
int argc;
char *argv[];
{
	char *cp;
	int  multiple_files = FALSE, output_files = FALSE;
	int  user_mailbox = FALSE, no_files, c;
	struct passwd *pass;
#ifndef	_POSIX_SOURCE
	struct passwd *getpwuid();
#endif
	int hostlen, domlen;
	int total_msgs = 0, selected_msgs = 0;
	int file_exists;
	struct stat statbuf;

	extern int optind;
	extern char *optarg;

#ifdef I_LOCALE
	setlocale(LC_ALL, "");
#endif

	elm_msg_cat = catopen("elm2.4", 0);

	/*
	 * check the first character of the command basename to
	 * use as the selection criterion.
	 */
	cp = argv[0] + strlen(argv[0]) - 1;
	while (cp != argv[0] && cp[-1] != '/')
	  cp--;
	switch (*cp) {
	  case 'n': selct |= NEW_MSG;  break;
	  case 'u':
	  case 'o': selct |= OLD_MSG;  break;
	  case 'r': selct |= READ_MSG; break;
	}

	while ((c = getopt(argc, argv, "hnQqSs:tv")) != EOF) 
	  switch (c) {
	    case (int)'n': number++;	break;
	    case (int)'Q': veryquiet++;	break;
	    case (int)'q': quiet++;	break;
	    case (int)'S': summarize++; break; 
	    case (int)'t': tidy++;      break;
	    case (int)'v': verbose++;	break;
	    case (int)'s': if (optarg[1] == '\0') {
			     switch (*optarg) {
			       case 'n':
			       case 'N': selct |= NEW_MSG;  break;
			       case 'o':
			       case 'O':
			       case 'u':
			       case 'U': selct |= OLD_MSG;  break;
			       case 'r':
			       case 'R': selct |= READ_MSG; break;
			       default:       usage(argv[0]);
					      exit(EXIT_ERROR);
			     }
			   } else if (istrcmp(optarg,"new") == 0)
			     selct |= NEW_MSG;
			   else if (istrcmp(optarg,"old") == 0)
			     selct |= OLD_MSG;
			   else if (istrcmp(optarg,"unread") == 0)
			     selct |= OLD_MSG;
			   else if (istrcmp(optarg,"read") == 0)
			     selct |= READ_MSG;
			   else {
			     usage(argv[0]);
			     exit(EXIT_ERROR);
			   }
			   break;
	    case (int)'h': print_help();
			   exit(EXIT_ERROR);
	    case (int)'?': usage(argv[0]);
			   printf(catgets(elm_msg_cat,
					  FromSet,FromForMoreInfo,
				"For more information, type \"%s -h\"\n"),
				   argv[0]);
	                   exit(EXIT_ERROR);
	  }

	if (quiet && verbose) {
	  fprintf(stderr,catgets(elm_msg_cat,FromSet,FromNoQuietVerbose,
				 "Can't have quiet *and* verbose!\n"));
	  exit(EXIT_ERROR);
	}

	if (veryquiet) {
	  if (freopen("/dev/null", "w", stdout) == NULL) {
	    fprintf(stderr,catgets(elm_msg_cat,FromSet,FromCantOpenDevNull,
			"Can't open /dev/null for \"very quiet\" mode.\n"));
	    exit(EXIT_ERROR);
	  }
	}

	/* default is all messages */
	if (selct == 0 || selct == (NEW_MSG|OLD_MSG|READ_MSG))
	  selct = ALL_MSGS;

	if((pass = getpwuid(getuid())) == NULL) {
	  fprintf(stderr,catgets(elm_msg_cat,FromSet,FromNoPasswdEntry,
				 "You have no password entry!"));
	  exit(EXIT_ERROR);
	}

	strcpy(username,pass->pw_name);
	strcpy(realname,username);

	/*
	 * from init.c: Get the host name as per configured behavior.
	 */
#ifdef HOSTCOMPILED
	strncpy(hostname, HOSTNAME, sizeof(hostname) - 1);
	hostname[sizeof(hostname) - 1] = '\0';
#else
	gethostname(hostname, sizeof(hostname));
#endif
	gethostdomain(hostdomain, sizeof(hostdomain));

	/*
	 * see init.c for an explanation of this!
	 */
	hostlen = strlen(hostname);
	domlen = strlen(hostdomain);
	if (hostlen >= domlen) {
	  if (istrcmp(&hostname[hostlen - domlen], hostdomain) == 0)
	    strcpy(hostfullname, hostname);
	  else {
	    strcpy(hostfullname, hostname);
	    strcat(hostfullname, hostdomain);
	  }
	} else {
	  if (istrcmp(hostname, hostdomain + 1) == 0)
	    strcpy(hostfullname, hostname);
	  else {
	    strcpy(hostfullname, hostname);
	    strcat(hostfullname, hostdomain);
	  }
	}

	infile[0] = '\0';
	defaultfile[0] = '\0';

	if (no_files = optind == argc) { /* assignment intentional */
	/*
	 *	determine mail file from environment variable if found,
	 *	else use password entry
	 */
	  if ((cp = getenv("MAIL")) == NULL) {
	    sprintf(infile,"%s%s",mailhome, username);
	  }
	  else {
	    strcpy(defaultfile, cp);
	    strcpy(infile, cp);
	  }
	  optind -= 1;	/* ensure one pass through loop */
	}

	multiple_files = (argc - optind > 1);

	for ( ; optind < argc; optind++) {
	
	  /* copy next argument into infile */

	  if (multiple_files) {
	    strcpy(infile, argv[optind]);
	    printf("%s%s: \n", output_files++ > 0 ? "\n":"", infile);
	  }
	  else if (infile[0] == '\0')
	    strcpy(infile, argv[optind]);

	  if (metachar(infile[0])) {
	    if (expand(infile) == 0) {
	       fprintf(stderr,catgets(elm_msg_cat,
				      FromSet,FromCouldntExpandFilename,
				      "%s: couldn't expand filename %s!\n"), 
		       argv[0], infile);
	       exit(EXIT_ERROR);
	    }
	  }


	  /* check if this is a mailbox or not, and attempt to open it */

	  if (strncmp(infile, mailhome, strlen(mailhome)) == 0)
	    user_mailbox = TRUE;
	  else
	    user_mailbox = FALSE;

	  /* if file name == default mailbox, its a spool file also
	   * even if its not in the spool directory. (SVR4)
	   */
	   if (strcmp(infile, defaultfile) == 0)
	     user_mailbox = TRUE;

	  /* pardon the oversimplification here */
	  if (stat(infile, &statbuf) == -1)
	    file_exists = FALSE;
	  else
	    file_exists = TRUE;

	  if (file_exists
#ifndef	_POSIX_SOURCE
	      && (statbuf.st_mode & S_IFMT) != S_IFREG
#else
	      && ! S_ISREG(statbuf.st_mode)
#endif
	      ) {
	    printf(catgets(elm_msg_cat,FromSet,FromNotRegularFile,
			   "\"%s\" is not a regular file!\n"), infile);
	    continue;
	  }

	  if ((mailfile = fopen(infile,"r")) == NULL) {
	    if ((user_mailbox && file_exists && statbuf.st_size == 0) || no_files) {
		if (!veryquiet)
		  printf(catgets(elm_msg_cat,FromSet,FromNoMail,"No mail.\n"));
		}
	    else {
	      if (infile[0] == '/' || file_exists == TRUE) 
	        printf(catgets(elm_msg_cat,FromSet,FromCouldntOpenFolder,
			       "Couldn't open folder \"%s\".\n"), infile);
	      else {
		/* only try mailhome if file not found */
		sprintf(infile,"%s%s", mailhome, argv[optind]);
		if ((mailfile = fopen(infile,"r")) == NULL) {
		  printf(catgets(elm_msg_cat,
				 FromSet,FromCouldntOpenFolderPlural,
				 "Couldn't open folders \"%s\" or \"%s\".\n"),
			 argv[optind], infile);
		  continue;	/* let's try the next file */
		} else
		  user_mailbox = TRUE;
	      }
	    }
	  }

	  /* if the open was successful, read the headers */

	  if (mailfile != NULL) {
	    /*
	     * if this is a mailbox, use the identity of the mailbox owner.
	     * this affects the "To" processing.
	     */
	    if (strncmp(infile, mailhome, strlen(mailhome)) == 0)
	      strcpy(username, infile+strlen(mailhome));
	    else
	      strcpy(username, realname);

	    /*
	     * then get full username
	     */
	    if((cp = get_full_name(username)) != NULL)
	      strcpy(full_username, cp);
	    else
	      strcpy(full_username, username);

	    read_headers(user_mailbox, &total_msgs, &selected_msgs);

	    /*
	     * we know what to say; now we have to figure out *how*
	     * to say it!
	     */

	    /* no messages at all? */
	    if (total_msgs == 0) {
	      if (user_mailbox)
		printf(catgets(elm_msg_cat,FromSet,FromStringNoMail,
			       "%s no mail.\n"), whos_mail(infile));
	      else
		if (!summarize)
		  printf(catgets(elm_msg_cat,FromSet,FromNoMesgInFolder,
				 "No messages in that folder!\n"));
	    }
	    else
	      /* no selected messages then? */
	      if (selected_msgs == 0) {
		if (user_mailbox)
		  printf(catgets(elm_msg_cat,FromSet,FromNoExplainMail,
				 "%s no %s mail.\n"), whos_mail(infile),
			 explain(selct,NEG));
		else
		  if (!summarize)
		    printf(catgets(elm_msg_cat,
				   FromSet,FromNoExplainMessages,
				   "No %s messages in that folder.\n"),
			   explain(selct,NEG));
	      }
	      else
		/* there's mail, but we just want a one-liner */
		if (quiet && !summarize) {
		  if (user_mailbox)
		    printf(catgets(elm_msg_cat,FromSet,FromStringStringMail,
				   "%s %s mail.\n"), whos_mail(infile),
			   explain(selct,POS));
		  else
		    printf(catgets(elm_msg_cat,FromSet,FromThereAreMesg,
				   "There are %s messages in that folder.\n"),
			    explain(selct,POS));
		}
	    fclose(mailfile);
	  }

	} /* for each arg */

	/*
	 * return "shell true" (0) if there are selected messages;
	 * 1 if there are messages, but no selected messages;
	 * 2 if there are no messages at all.
	 */
	if (selected_msgs > 0)
	  exit(EXIT_SELECTED);
	else if (total_msgs > 0)
	  exit(EXIT_MAIL);
	else
	  exit(EXIT_NO_MAIL);
}

read_headers(user_mailbox, total_msgs, selected)
int user_mailbox;
int *total_msgs;
int *selected;
{
	/** Read the headers, output as found.  User-Mailbox is to guarantee
	    that we get a reasonably sensible message from the '-v' option
	 **/

	struct header_rec hdr;
	char buffer[SLEN], to_whom[SLEN], from_whom[SLEN], subject[SLEN];
	char who[SLEN];
	register int in_header = FALSE, count = 0, selected_msgs = 0;
	int status, i;
	int lenWho;
	int summary[ALL_MSGS];
#ifdef MMDF
	int newheader = FALSE;
#endif /* MMDF */

	for (i=0; i<ALL_MSGS; i++)
	  summary[i] = 0;

	while (mail_gets(buffer, SLEN, mailfile) != 0) {
	  if (index(buffer, '\n') == NULL && !feof(mailfile)) {
	    int c;
	    while ((c = getc(mailfile)) != EOF && c != '\n')
	      ; /* keep reading */
	  }

#ifdef MMDF
          if (strcmp(buffer, MSG_SEPARATOR) == 0) {
	    newheader = !newheader;
	    if (newheader) {
	      subject[0] = '\0';
	      to_whom[0] = '\0';
	      in_header = TRUE;
	      if (user_mailbox)
		status = NEW_MSG;
	      else
		status = READ_MSG;
	    }
	  }
#else
	  if (first_word(buffer, "From ") && real_from(buffer, &hdr)) {
	    strcpy(from_whom, hdr.from);
	    subject[0] = '\0';
	    to_whom[0] = '\0';
	    in_header = TRUE;
	    if (user_mailbox)
	      status = NEW_MSG;
	    else
	      status = READ_MSG;
	  }
#endif /* MMDF */
	  else if (in_header) {
#ifdef MMDF
	    if (real_from(buffer, &hdr))
	      strcpy(from_whom, hdr.from);
	    else
#endif /* MMDF */
	    if (first_word(buffer,">From ")) 
	      forwarded(buffer, from_whom); /* return address */
	    else if (header_cmp(buffer,"Subject", NULL) ||
		     header_cmp(buffer,"Re", NULL)) {
	      if (subject[0] == '\0') {
	        remove_header_keyword(buffer);
		strcpy(subject, buffer);
	      }
	    }
	    else if (header_cmp(buffer,"From", NULL) ||
		     header_cmp(buffer, ">From", NULL))
	      parse_arpa_who(buffer, from_whom, FALSE);
	    else if (header_cmp(buffer, "To", NULL))
	      figure_out_addressee(index(buffer, ':') + 1, to_whom);
	    else if (header_cmp(buffer, "Status", NULL)) {
	      remove_header_keyword(buffer);
	      switch (*buffer) {
		case 'N': status = NEW_MSG;	break;
		case 'O': status = OLD_MSG;	break;
		case 'R': status = READ_MSG;	break;
		default:  status = UNKNOWN;	break;
	      }
	      if (buffer[0] == 'O' && buffer[1] == 'R')
		status = READ_MSG;
	    }
	    else if (buffer[0] == LINEFEED) {
	      in_header = FALSE;
#ifdef MMDF
	      if (*from_whom == '\0')
                strcpy(from_whom,username);
#endif /* MMDF */
	      count++;
	      summary[status]++;

	      if ((status & selct) != 0) {

		/* what a mess! */
		if (verbose && selected_msgs == 0) {
		  if (user_mailbox) {
		    if (selct == ALL_MSGS)
		      printf(catgets(elm_msg_cat,FromSet,FromFollowingMesg,
				     "%s the following mail messages:\n"),
			      whos_mail(infile));
		    else
		      printf(catgets(elm_msg_cat,FromSet,FromStringStringMail,
				     "%s %s mail.\n"), whos_mail(infile),
			     explain(selct,POS));
		  }
		  else
		    printf(catgets(elm_msg_cat,
				   FromSet,FromFolderContainsFollowing,
			"Folder contains the following %s messages:\n"),
			    explain(selct,POS));
		}

		selected_msgs++;
		if (! quiet) {
		  if (tail_of(from_whom, who, to_whom) == 1) {
		    strcpy(buffer, "To ");
		    strcat(buffer, who);
		    strcpy(who, buffer);
		  }
 
		  /***
		  *	Print subject on next line if the Who part blows
		  *	the alignment
		  ***/
		  
		  if (tidy) 
		    lenWho = strlen(who);
		  else
		    lenWho = 0;			/* forces op on same line */
		  
		  if (number)
		    printf("%3d: %-20s%c%*s%s\n", count, who, 
				lenWho > 20 ? '\n' : ' ',
				lenWho > 20 ? 27   :   1, "",
			        subject);
		  else
		    printf("%-20s%c%*s%s\n", who, 
				lenWho > 20 ? '\n' : ' ',
				lenWho > 20 ? 22   :   1, "",
				subject);
		}
	      }
	    }
	  }
	}

	*selected = selected_msgs;
	*total_msgs = count;

	/* print a message type summary */

	if (summarize) {
	  int output=FALSE, unknown = 0;

	  if (user_mailbox)
	    printf("%s ", whos_mail(infile));
	  else
	    printf(catgets(elm_msg_cat,FromSet,FromFolderContains,
			   "Folder contains "));

	  for (i=0; i<ALL_MSGS; i++) {
	    if (summary[i] > 0) {
	      if (output)
		printf(", ");
	      switch (i) {
		case NEW_MSG:
		case OLD_MSG:
		case READ_MSG:
		  printf("%d %s ",summary[i], explain(i,POS));
		  if (summary[i] == 1)
		       printf("%s",catgets(elm_msg_cat,
				   FromSet,FromMessage,"message"));
		  else
		       printf("%s",catgets(elm_msg_cat,
				   FromSet,FromMessagePlural,"messages"));
		  
		  output = TRUE;
		  break;
		default:
		  unknown += summary[i];
	      }
	    }
	  }
	  if (unknown)
	  {
	       printf("%d ",unknown);
	       
	       if (unknown == 1)
		    printf("%s",catgets(elm_msg_cat,
					FromSet,FromMessage,"message"));
	       else
		    printf("%s",catgets(elm_msg_cat,
					FromSet,FromMessagePlural,"messages"));
	       
	       printf("%s "," of unknown status");
	       output = TRUE;
	  }
	  
	  if (output)
	    printf(".\n");
	  else
	    printf(catgets(elm_msg_cat,FromSet,FromNoMessages,
                       "no messages.\n"));
	}
}


forwarded(buffer, who)
char *buffer, *who;
{
	/** change 'from' and date fields to reflect the ORIGINATOR of 
	    the message by iteratively parsing the >From fields... **/

	char machine[SLEN], buff[SLEN], holding_from[SLEN];

	machine[0] = '\0';
	holding_from[0] = '\0';
	sscanf(buffer, "%*s %s %*s %*s %*s %*s %*s %*s %*s %*s %s",
	            holding_from, machine);

	if(machine[0] == '\0')	/* try for address with timezone in date */
	sscanf(buffer, "%*s %s %*s %*s %*s %*s %*s %*s %*s %s",
	            holding_from, machine);

	if (machine[0] == '\0') /* try for srm address */
	  sscanf(buffer, "%*s %s %*s %*s %*s %*s %*s %*s %s",
	            holding_from, machine);

	if (machine[0] == '\0')
	  sprintf(buff, holding_from[0] ? holding_from :
		  catgets(elm_msg_cat,FromSet,FromAnon, "anonymous"));
	else
	  sprintf(buff,"%s!%s", machine, holding_from);

	strncpy(who, buff, SLEN);
}

/*
 * Return an appropriate string as to whom this mailbox belongs.
 */
char *
whos_mail(filename)
char *filename;
{
	static char whos_who[SLEN];
	char *mailname;

	if (strncmp(filename, mailhome, strlen(mailhome)) == 0) {
	  mailname = filename + strlen(mailhome);
	  if (*mailname == '/')
	    mailname++;
	  if (strcmp(mailname, realname) == 0)
	    strcpy(whos_who,catgets(elm_msg_cat,
				    FromSet,FromYouHave,"You have"));
	  else {
	    strcpy(whos_who, mailname);
	    strcat(whos_who,catgets(elm_msg_cat,FromSet,FromHas, " has"));
	  }
	}
	else
	/* punt... */
	     strcpy(whos_who,catgets(elm_msg_cat,
				     FromSet,FromYouHave,"You have"));

	return whos_who;
}

usage(prog)
char *prog;
{
     printf(catgets(elm_msg_cat,FromSet,FromUsage,
	"Usage: %s [-n] [-v] [-t] [-s {new|old|read}] [filename | username] ...\n"),
	    prog);
}

print_help()
{

     printf(catgets(elm_msg_cat,FromSet,FromHelpTitle,
 "frm -- list from and subject lines of messages in mailbox or folder\n"));
		    
     usage("frm");
     printf(catgets(elm_msg_cat,FromSet,FromHelpText,
"\noption summary:\n\
-h\tprint this help message.\n\
-n\tdisplay the message number of each message printed.\n\
-Q\tvery quiet -- no output is produced.  This option allows shell\n\
\tscripts to check frm's return status without having output.\n\
-q\tquiet -- only print summaries for each mailbox or folder.\n\
-S\tsummarize the number of messages in each mailbox or folder.\n\
-s status only -- select messages with the specified status.\n\
\t'status' is one of \"new\", \"old\", \"unread\" (same as \"old\"),\n\
\tor \"read\".  Only the first letter need be specified.\n\
-t\ttry to align subjects even if 'from' text is long.\n\
-v\tprint a verbose header.\n"));

}

/* explanation of messages visible after selection */
/* usage: "... has the following%s messages ...", explain(selct,POS) */

char *
explain(selection, how_to_say)
int selection;
int how_to_say;
{
	switch (selection) {
	  case NEW_MSG:
	    return catgets(elm_msg_cat,FromSet,FromNew,"new");
	  case OLD_MSG:
	    return catgets(elm_msg_cat,FromSet,FromUnread,"unread");
	  case READ_MSG:
	    return catgets(elm_msg_cat,FromSet,FromRead,"read");
	  case (NEW_MSG|OLD_MSG):
	    if (how_to_say == POS)
	      return catgets(elm_msg_cat,FromSet,FromNewAndUnread,
			     "new and unread");
	    else
	      return catgets(elm_msg_cat,FromSet,FromNewOrUnread,
			     "new or unread");
	  case (NEW_MSG|READ_MSG):
	    if (how_to_say == POS)
	      return catgets(elm_msg_cat,FromSet,FromNewAndRead,
			     "new and read");
	    else
	      return catgets(elm_msg_cat,FromSet,FromNewOrRead,
			     "new or read");
	  case (READ_MSG|OLD_MSG):
	    if (how_to_say == POS)
	      return catgets(elm_msg_cat,FromSet,FromReadAndUnread,
			     "read and unread");
	    else
	      return catgets(elm_msg_cat,FromSet,FromReadOrUnread,
			     "read or unread");
	  case ALL_MSGS:
	    return "";
	  default:
	    return catgets(elm_msg_cat,FromSet,FromUnknown,"unknown");
	}
}
