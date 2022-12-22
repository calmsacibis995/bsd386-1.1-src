
static char rcsid[] = "@(#)Id: elm.c,v 5.16 1993/09/19 23:46:00 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.2 $   $State: Exp $
 *
 * This file and all associated files and documentation:
 *			Copyright (c) 1988-1992 USENET Community Trust
 *			Copyright (c) 1986,1987 Dave Taylor
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein, Elm Coordinator
 *	elm@DSI.COM			dsinc!elm
 *
 *******************************************************************************
 * $Log: elm.c,v $
 * Revision 1.2  1994/01/14  00:54:46  cp
 * 2.4.23
 *
 * Revision 5.16  1993/09/19  23:46:00  syd
 * Although it doesnt solve the limit/resync problem of new
 * messages, allow them to be accessed anyway.
 * From: austig@solan.unit.no
 *
 * Revision 5.15  1993/08/03  19:28:39  syd
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
 * Revision 5.14  1993/05/31  19:32:20  syd
 * With this patch build_address() should treat local mailing
 * lists and other aliases known by the transport agent as valid
 * addresses.
 * I also conditionalized printing the "Expands to: " message
 * in check_only mode to be done only when there is an expanded
 * address to print. Build_address will inform anyway about an
 * alias that does not exist.
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.13  1993/04/12  03:15:41  syd
 * These patches makes 'T' (since it was free) do a Tag and Move command in the
 * index and alias page, and in the builtin pager.
 * In the alias help in src/alias.c, there is a tolower done on the character
 * one wants help for.  This is clearly wrong.
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.12  1993/01/20  03:37:16  syd
 * Nits and typos in the NLS messages and corresponding default messages.
 * From: dwolfe@pffft.sps.mot.com (Dave Wolfe)
 *
 * Revision 5.11  1993/01/19  04:34:28  syd
 * Just a small bugfix for the '#' (Debug Message) screen.  The columns of the
 * various flags don't all line up properly:
 * From: Gary Bartlett <garyb@abekrd.co.uk>
 *
 * Revision 5.10  1992/12/24  21:42:01  syd
 * Fix messages and nls messages to match.  Plus use want_to
 * where appropriate.
 * From: Syd, via prompting from Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.9  1992/12/16  14:33:25  syd
 * add back missing nl on check only output
 * From: Syd
 *
 * Revision 5.8  1992/12/11  01:45:04  syd
 * remove sys/types.h include, it is now included by defs.h
 * and this routine includes defs.h or indirectly includes defs.h
 * From: Syd
 *
 * Revision 5.7  1992/12/07  02:58:13  syd
 * fix long -> time_t
 * From: Syd
 *
 * Revision 5.6  1992/11/26  00:46:13  syd
 * changes to first change screen back (Raw off) and then issue final
 * error message.
 * From: Syd
 *
 * Revision 5.5  1992/11/22  00:03:56  syd
 * Fix segmentation violation on restricted alias page jump.
 * From: "Robert L. Howard" <robert.howard@matd.gatech.edu>
 *
 * Revision 5.4  1992/11/07  19:37:21  syd
 * Enhanced printing support.  Added "-I" to readmsg to
 * suppress spurious diagnostic messages.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.3  1992/10/31  20:02:26  syd
 * remove duplicate ScreenSize call
 * From: Syd
 *
 * Revision 5.2  1992/10/25  01:47:45  syd
 * fixed a bug were elm didn't call metamail on messages with a characterset,
 * which could be displayed by elm itself, but message is encoded with QP
 * or BASE64
 * From: Klaus Steinberger <Klaus.Steinberger@Physik.Uni-Muenchen.DE>
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/* Main program of the ELM mail system! 
*/

#include "elm.h"
#include "s_elm.h"

#ifdef I_TIME
#  include <time.h>
#endif
#ifdef I_SYSTIME
#  include <sys/time.h>
#endif
#ifdef BSD
#  include <sys/timeb.h>
#endif

long bytes();
char *format_long(), *parse_arguments(), *error_description();

main(argc, argv)
int argc;
char *argv[];
{
	int  ch;
	char address[SLEN], to_whom[SLEN], *req_mfile;
	int  i,j;      		/** Random counting variables (etc)          **/
	int  pageon, 		/** for when we receive new mail...          **/
	     last_in_folder;	/** for when we receive new mail too...      **/
	long num;		/** another variable for fun..               **/
	extern int errno;

#ifdef I_LOCALE
	setlocale(LC_ALL, "");
#endif

	elm_msg_cat = catopen("elm2.4", 0);

	req_mfile = parse_arguments(argc, argv, to_whom);

	initialize(req_mfile);

	if (mail_only) {
	   dprint(3, (debugfile, "Mail-only: mailing to\n-> \"%s\"\n", 
		   format_long(to_whom, 3)));
	   if(!batch_only) {
	     sprintf(address, catgets(elm_msg_cat, ElmSet, ElmSendOnlyMode,
	       "Send only mode [ELM %s]"), version_buff);
	     Centerline(1, address);
	   }
	   (void) send_msg(to_whom, "", batch_subject, TRUE,
	     (batch_only ? NO : allow_forms), FALSE); 
	   leave(0);
	} else if (check_only) {
	   do_check_only(to_whom);
	   leave(0);
	}

	showscreen();

	while (1) {
#ifdef SIGWINCH
	  if (resize_screen) {
	    int	newLINES, newCOLUMNS;

	    ScreenSize(&newLINES, &newCOLUMNS);
	    resize_screen = 0;
	    if (newLINES != LINES || newCOLUMNS != COLUMNS) {
	      LINES = newLINES, COLUMNS = newCOLUMNS;
#define max(a,b)	       ((a) < (b) ? (b) : (a))
	      if (mini_menu)
		headers_per_page = max (LINES - 13, 1);
	      else
		headers_per_page = max (LINES -	 8, 1);	  /* 5 more headers! */
#undef max
	      redraw++;
	    }
	  }
	  else redraw = 0;
#else
    	  redraw = 0;
#endif
	  nufoot = 0;
	  nucurr = 0;
	  if ((num = bytes(cur_folder)) != mailfile_size) {
	    dprint(2, (debugfile, "Just received %d bytes more mail (elm)\n", 
		    num - mailfile_size));
	    error(catgets(elm_msg_cat, ElmSet, ElmNewMailHangOn,
	      "New mail has arrived! Hang on..."));
	    fflush(stdin);	/* just to be sure... */
	    last_in_folder = message_count;
	    pageon = header_page;

	    if ((errno = can_access(cur_folder, READ_ACCESS)) != 0) {
	      dprint(1, (debugfile,
		    "Error: given file %s as folder - unreadable (%s)!\n", 
		    cur_folder, error_description(errno)));
	      Raw(OFF);
	      fprintf(stderr, catgets(elm_msg_cat, ElmSet, ElmCantOpenFolderRead,
		"Can't open folder '%s' for reading!\n"), cur_folder);
	      leave(0);
	      }

	    newmbox(cur_folder, TRUE);	/* last won't be touched! */
	    clear_error();
	    header_page = pageon;

	    if (selected)               /* update count of selected messages */
	      selected += message_count - last_in_folder;

	    if (on_page(current))   /* do we REALLY have to rewrite? */
	      showscreen();
	    else {
	      update_title();
	      ClearLine(LINES-1);	     /* remove reading message... */
	      if ((message_count - last_in_folder) == 1)
	        error(catgets(elm_msg_cat, ElmSet, ElmNewMessageRecv,
		       "1 new message received."));
	      else
	        error1(catgets(elm_msg_cat, ElmSet, ElmNewMessageRecvPlural,
		       "%d new messages received."), 
		       message_count - last_in_folder);
	    }
	    /* mailfile_size = num; */
	    if (cursor_control)
	      transmit_functions(ON);	/* insurance */
	  }

	  prompt(Prompt);

	  CleartoEOLN();
	  ch = GetPrompt();
#ifdef SIGWINCH
	  if (resize_screen) {
	    int	newLINES, newCOLUMNS;

	    ScreenSize(&newLINES, &newCOLUMNS);
	    resize_screen = 0;
	    if (newLINES != LINES || newCOLUMNS != COLUMNS) {
	      LINES = newLINES, COLUMNS = newCOLUMNS;
#define max(a,b)	       ((a) < (b) ? (b) : (a))
	      if (mini_menu)
		headers_per_page = max (LINES - 13, 1);
	      else
		headers_per_page = max (LINES -	 8, 1);	  /* 5 more headers! */
#undef max
	      redraw++;
	    }
	  }
#endif
	  CleartoEOS();
#ifdef DEBUG
	  if (! movement_command(ch))
	    dprint(4, (debugfile, "\nCommand: %c [%d]\n\n", ch, ch));
#endif

	  set_error("");	/* clear error buffer */

	  MoveCursor(LINES-3,strlen(Prompt));

	  switch (ch) {

	    case '?' 	:  if (help(FALSE))
	  		     redraw++;
			   else
			     nufoot++;
			   break;

	    case '$'    :  PutLine0(LINES-3, strlen(Prompt),
			     catgets(elm_msg_cat, ElmSet, ElmResyncFolder,
	    		     "Resynchronize folder"));
			   redraw += resync();
			   nucurr = get_page(current);
			   break;

	    case '|'    :  Writechar('|'); 
			   if (message_count < 1) {
			     error(catgets(elm_msg_cat, ElmSet, ElmNoMailToPipe,
			       "No mail to pipe!"));
			     fflush(stdin);
			   } else {
	    		     softkeys_off();
                             redraw += do_pipe();		
			     softkeys_on();
			   }
			   break;

#ifdef ALLOW_SUBSHELL
	    case '!'    :  Writechar('!'); 
                           redraw += subshell();		
			   break;
#endif

	    case '%'    :  if (current > 0) {
			     get_return(address, current-1);
			     clear_error();
			     PutLine1(LINES,(COLUMNS-strlen(address))/2,
				      "%.78s", address);	
			   } else {
			     error(catgets(elm_msg_cat, ElmSet, ElmNoMailReturnAddress,
			       "No mail to get return address of!")); 
			     fflush(stdin);
			   }
			   break;

	    case '<'    :  /* scan current message for calendar information */
#ifdef ENABLE_CALENDAR
			   if  (message_count < 1) {
			     error(catgets(elm_msg_cat, ElmSet, ElmNoMailToScan,
			       "No mail to scan!"));
			     fflush(stdin);
			   }
			   else {
			       PutLine0(LINES-3, strlen(Prompt), 	
			           catgets(elm_msg_cat, ElmSet, ElmScanForCalendar,
				   "Scan message for calendar entries..."));
			       scan_calendar();
			   }
#else
	 		   error(catgets(elm_msg_cat, ElmSet, ElmSorryNoCalendar,
			     "Sorry. Calendar function disabled."));
			   fflush(stdin);
#endif
			   break;

	    case 'a'    :  alias();
			   redraw++;
			   define_softkeys(MAIN); 	break;
			
	    case 'b'    :  PutLine0(LINES-3, strlen(Prompt), 
			     catgets(elm_msg_cat, ElmSet, ElmBounceMessage,
			     "Bounce message"));
			   fflush(stdout);
			   if (message_count < 1) {
	  		     error(catgets(elm_msg_cat, ElmSet, ElmNoMailToBounce,
			       "No mail to bounce!"));
			     fflush(stdin);
			   }
			   else 
			     nufoot = remail();
			   break;

	    case 'c'    :  PutLine0(LINES-3, strlen(Prompt), 
			     catgets(elm_msg_cat, ElmSet, ElmChangeFolder,
			     "Change folder"));
			   define_softkeys(CHANGE);
			   redraw += change_file();
			   define_softkeys(MAIN);
			   break;

#ifdef ALLOW_MAILBOX_EDITING
	    case 'e'    :  PutLine0(LINES-3,strlen(Prompt),
			     catgets(elm_msg_cat, ElmSet, ElmEditFolder,
			     "Edit folder"));
			   if (current > 0) {
			     edit_mailbox();
	    		     if (cursor_control)
			       transmit_functions(ON);	/* insurance */
	   		   }
			   else {
			     error(catgets(elm_msg_cat, ElmSet, ElmFolderIsEmpty,
			       "Folder is empty!"));
			     fflush(stdin);
			   }
			   break;
#else
	    case 'e'    : error(catgets(elm_msg_cat, ElmSet, ElmNoFolderEdit,
		    "Folder editing isn't configured in this version of ELM."));
			  fflush(stdin);
			  break;
#endif
		
	    case 'f'    :  PutLine0(LINES-3, strlen(Prompt),
			     catgets(elm_msg_cat, ElmSet, ElmForward,
			     "Forward"));
			   define_softkeys(YESNO);
			   if (current > 0) {
			     if(forward()) redraw++;
			     else nufoot++;
			   } else {
			     error(catgets(elm_msg_cat, ElmSet, ElmNoMailToForward,
			       "No mail to forward!"));
			     fflush(stdin);
			   }
			   define_softkeys(MAIN);
			   break;

	    case 'g'    :  PutLine0(LINES-3,strlen(Prompt),
			     catgets(elm_msg_cat, ElmSet, ElmGroupReply,
			     "Group reply"));
			   fflush(stdout);
			   if (current > 0) {
			     if (headers[current-1]->status & FORM_LETTER) {
			       error(catgets(elm_msg_cat, ElmSet, ElmCantGroupReplyForm,
				 "Can't group reply to a Form!!"));
			       fflush(stdin);
			     }
			     else {
			       define_softkeys(YESNO);
			       redraw += reply_to_everyone();	
			       define_softkeys(MAIN);
			     }
			   }
			   else {
			     error(catgets(elm_msg_cat, ElmSet, ElmNoMailToReply,
			       "No mail to reply to!")); 
			     fflush(stdin);
			   }
			   break;

	    case 'h'    :  if (filter)
			     PutLine0(LINES-3, strlen(Prompt), 
			       catgets(elm_msg_cat, ElmSet, ElmMessageWithHeaders,
			       "Message with headers..."));
			   else
			     PutLine0(LINES-3, strlen(Prompt),
			       catgets(elm_msg_cat, ElmSet, ElmDisplayMessage,
			       "Display message"));
			   if(current > 0) {
			     fflush(stdout);
			     j = filter;
			     filter = FALSE;
			     i = show_msg(current);
			     while (i)
				i = process_showmsg_cmd(i);
			     filter = j;
			     redraw++;
			     (void)get_page(current);
			   } else
			     error(catgets(elm_msg_cat, ElmSet, ElmNoMailToRead,
			       "No mail to read!"));
			   break;

	    case 'm'    :  PutLine0(LINES-3, strlen(Prompt),
			     catgets(elm_msg_cat, ElmSet, ElmMail,
			     "Mail"));
 			   redraw += send_msg("", "", "", TRUE,allow_forms,FALSE); 
			   break;

	    case ' '    : 
	    case ctrl('J'):
	    case ctrl('M'): PutLine0(LINES-3, strlen(Prompt), 
			      catgets(elm_msg_cat, ElmSet, ElmDisplayMessage,
	    		      "Display message"));	
			   fflush(stdout);
			   if(current > 0 ) {
			     define_softkeys(READ);

			     i = show_msg(current);
			     while (i)
				i = process_showmsg_cmd(i);
			     redraw++;
			     (void)get_page(current);
			   } else
			     error(catgets(elm_msg_cat, ElmSet, ElmNoMailToRead,
			       "No mail to read!"));
			   break;

	    case 'n'    :  PutLine0(LINES-3,strlen(Prompt),
			     catgets(elm_msg_cat, ElmSet, ElmNextMessage,
			     "Next Message"));
			   fflush(stdout);
			   define_softkeys(READ);

			   if(current > 0 ) {
			     define_softkeys(READ);

			     i = show_msg(current);
			     while (i)
			       i = process_showmsg_cmd(i);
			     redraw++;
			     if (++current > message_count)
			       current = message_count;
			     (void)get_page(current);
			   } else
			     error(catgets(elm_msg_cat, ElmSet, ElmNoMailToRead,
			       "No mail to read!"));
			   break;

	    case 'o'    :  PutLine0(LINES-3, strlen(Prompt),
			     catgets(elm_msg_cat, ElmSet, ElmOptions,
			     "Options"));
			   if((i=options()) > 0)
			     get_page(current);
			   else if(i < 0)
			     leave(0);
			   redraw++;	/* always fix da screen... */
			   break;

	    case 'p'    :  PutLine0(LINES-3, strlen(Prompt),
			     catgets(elm_msg_cat, ElmSet, ElmPrintMail,
			     "Print mail"));
			   fflush(stdout);
			   if (message_count < 1) {
			     error(catgets(elm_msg_cat, ElmSet, ElmNoMailToPrint,
			       "No mail to print!"));
			     fflush(stdin);
			   } else if (print_msg(TRUE) != 0)
			     redraw++;
			   break;

	    case 'q'    :  PutLine0(LINES-3, strlen(Prompt),
			     catgets(elm_msg_cat, ElmSet, ElmQuit,
			     "Quit"));

			   if (mailfile_size != bytes(cur_folder)) {
			     error(catgets(elm_msg_cat, ElmSet, ElmNewMailQuitCancelled,
				"New Mail!  Quit canceled..."));
			     fflush(stdin);
	  		     if (folder_type == SPOOL) unlock();
			   }
			   else
			     quit(TRUE);		

			   break;

	    case 'Q'    :  PutLine0(LINES-3, strlen(Prompt),
			     catgets(elm_msg_cat, ElmSet, ElmQuickQuit,
			     "Quick quit"));

			   if (mailfile_size != bytes(cur_folder)) {
			     error(catgets(elm_msg_cat, ElmSet, ElmNewMailQuickQuitCancelled,
			       "New Mail!  Quick Quit canceled..."));
	  		     if (folder_type == SPOOL) unlock();
			   }
			   else
			     quit(FALSE);		

			   break;

	    case 'r'    :  PutLine0(LINES-3, strlen(Prompt), 
			     catgets(elm_msg_cat, ElmSet, ElmReplyToMessage,
			     "Reply to message"));
			   if (current > 0) 
			     redraw += reply();	
			   else {
			     error(catgets(elm_msg_cat, ElmSet, ElmNoMailToReplyTo,
			       "No mail to reply to!")); 
			     fflush(stdin);
			   }
			   softkeys_on();
			   break;

	    case '>'    : /** backwards compatibility **/

	    case 'C'	:
	    case 's'    :  if  (message_count < 1) {
			     if (ch != 'C')
			       error(catgets(elm_msg_cat, ElmSet, ElmNoMailToSave,
				 "No mail to save!"));
			     else
			       error(catgets(elm_msg_cat, ElmSet, ElmNoMailToCopy,
				 "No mail to copy!"));
			     fflush(stdin);
			   }
			   else {
			     if (ch != 'C')
			       PutLine0(LINES-3, strlen(Prompt),
				      catgets(elm_msg_cat, ElmSet, ElmSaveToFolder,
				      "Save to folder"));
			     else
			       PutLine0(LINES-3, strlen(Prompt),
				      catgets(elm_msg_cat, ElmSet, ElmCopyToFolder,
				      "Copy to folder"));
			     PutLine0(LINES-3,COLUMNS-40,
			        catgets(elm_msg_cat, ElmSet, ElmUseForHelp,
				"(Use '?' for help)"));
			     if (save(&redraw, FALSE, (ch != 'C'))
				 && resolve_mode && ch != 'C') {
			       if((i=next_message(current-1, TRUE)) != -1) {
				 current = i+1;
				 nucurr = get_page(current);
			       }
			     }
			   }
			   ClearLine(LINES-2);		
			   break;

	    case 'X'    :  PutLine0(LINES-3, strlen(Prompt),
			     catgets(elm_msg_cat, ElmSet, ElmQuickExit,
			     "Quick Exit"));
                           fflush(stdout);
			   leave(0);
			   break;

	    case ctrl('Q') :
	    case 'x'    :  PutLine0(LINES-3, strlen(Prompt),
			     catgets(elm_msg_cat, ElmSet, ElmExit,
			     "Exit"));  
                           fflush(stdout);
			   exit_prog();
			   break;

            case EOF :  leave(0);  /* Read failed, control tty died? */
                        break;
	    
	    case '@'    : debug_screen();  redraw++;	break;
	
	    case '#'    : if (message_count) {
			    debug_message(); 
			    redraw++;	
			  } 
			  else {
			    error(catgets(elm_msg_cat, ElmSet, ElmNoMailToCheck,
			      "No mail to check."));
			    fflush(stdin);
			  }
			  break;

	    /* None of the menu specific commands were chosen, therefore
	     * it must be a "motion" command (or an error).               */
	    default	: motion(ch);

	  }

	  if (redraw)
	    showscreen();

          check_range();
	    
	  if (nucurr == NEW_PAGE) 
	    show_headers();
	  else if (nucurr == SAME_PAGE)
	    show_current();
	  else if (nufoot) {
	    if (mini_menu) {
	      MoveCursor(LINES-7, 0);  
              CleartoEOS();
	      show_menu();
	    }
	    else {
	      MoveCursor(LINES-4, 0);
	      CleartoEOS();
	    }
	    show_last_error();	/* for those operations that have to
				 * clear the footer except for a message.
				 */
	  }

	} /* the BIG while loop! */
}

debug_screen()
{
	/**** spit out all the current variable settings and the table
	      entries for the current 'n' items displayed. ****/

	register int i, j;
	char     buffer[SLEN];

	ClearScreen();

	PutLine2(0,0,"Current message number = %d\t\t%d message(s) total\n",
		current, message_count);
	PutLine2(2,0,"Header_page = %d           \t\t%d possible page(s)\n",
		header_page, (int) (message_count / headers_per_page) + 1);

	PutLine1(4,0,"\nCurrent mail file is %s.\n\r\n", cur_folder);

	i = header_page*headers_per_page;	/* starting header */

	if ((j = i + (headers_per_page-1)) >= message_count) 
	  j = message_count-1;

	Write_to_screen(
"Num      From                    Subject                Lines   Offset  Content\n\r\n\r",0);

	while (i <= j) {
	   sprintf(buffer, 
	   "%3d %-16.16s %-35.35s %4d %8d %8d\n\r",
		    i+1,
		    headers[i]->from, 
		    headers[i]->subject,
		    headers[i]->lines,
		    headers[i]->offset,
		    headers[i]->content_length);
	    Write_to_screen(buffer, 0);
	  i++;
	}
	
	PutLine0(LINES,0,"Press any key to return.");
	(void) ReadCh();
}


debug_message()
{
	/**** Spit out the current message record.  Include EVERYTHING
	      in the record structure. **/
	
	char buffer[SLEN];
	time_t header_time;
	register struct header_rec *current_header = headers[current-1];

	ClearScreen();

	Write_to_screen("\t\t\t----- Message %d -----\n\r\n\r\n\r\n\r", 1,
		current);

	Write_to_screen(     "Lines : %-5d\t\t\tStatus: A  C  D  E  F  M  D  N  N  O  P  T  U  V\n\r", 1,
		current_header->lines);
	Write_to_screen("Content-Length: %-12d\t        c  o  e  x  o  i  e  p  e  l  r  a  r  i\n\r", 1,
		current_header->content_length);
	Write_to_screen(     "            \t\t\t        t  n  l  p  r  m  c  l  w  d  i  g  g  s\n\r", 0);
	Write_to_screen(     "            \t\t\t        n  f  d  d  m  e  o  a        v  d  n  i\n\r", 0);

	sprintf(buffer, 
		"\n\rOffset: %ld\t\t\t        %d  %d  %d  %d  %d  %d  %d  %d",
		current_header->offset,
		(current_header->status & ACTION) != 0,
		(current_header->status & CONFIDENTIAL) != 0,
		(current_header->status & DELETED) != 0,
		(current_header->status & EXPIRED) != 0,
		(current_header->status & FORM_LETTER) != 0,
		(current_header->status & MIME_MESSAGE) != 0,
		(current_header->status & MIME_NEEDDECOD) != 0,
		(current_header->status & MIME_NOTPLAIN) != 0);
	sprintf(buffer + strlen(buffer),
		"  %d  %d  %d  %d  %d  %d\n",
		(current_header->status & NEW) != 0,
		(current_header->status & UNREAD) != 0,
		(current_header->status & PRIVATE) != 0,
		(current_header->status & TAGGED) != 0,
		(current_header->status & URGENT) != 0,
		(current_header->status & VISIBLE) != 0);

	Write_to_screen(buffer, 0);

	sprintf(buffer, "\n\rReceived on: %s\r",
		asctime(localtime(&current_header->received_time)));
	Write_to_screen(buffer, 0);

	header_time = current_header->time_sent + current_header->tz_offset;
	sprintf(buffer, "Message sent on: %s\rFrom timezone: %s (%d)\n\r",
		asctime(gmtime(&header_time)),
		current_header->time_zone,
		current_header->tz_offset);
	Write_to_screen(buffer, 0);
	
	Write_to_screen("From: %s\n\rSubject: %s", 2,
		current_header->from,
		current_header->subject);

	Write_to_screen("\n\rPrimary Recipient: %s\nInternal Index Reference Number = %d\n\r", 2,
		current_header->to,
		current_header->index_number);

	Write_to_screen("Message-ID: %s\n\r", 1,
		strlen(current_header->messageid) > 0 ? 
		current_header->messageid : "<none>");

	Write_to_screen("Status: %s\n\r", 1, current_header->mailx_status);
	
	PutLine0(LINES,0,"Please Press any key to return.");
	(void) ReadCh();
}

do_check_only(to_whom)
char *to_whom;
{
	char buffer[VERY_LONG_STRING], *msg;

	dprint(3, (debugfile, "Check-only: checking \n-> \"%s\"\n", 
		format_long(to_whom, 3)));

	if (build_address(strip_commas(to_whom), buffer)) {

	    msg = catgets(elm_msg_cat, ElmSet, ElmExpandsTo,
			  "Expands to: ");
	    printf(msg);

	    printf("%s\n", format_long(buffer, strlen(msg)));
	}
}

check_range()
{
	int i;

	i = compute_visible(current);

	if ((current < 1) || (selected && i < 1)) {
	    if (message_count > 0) {
	      /* We are out of range! Get to first message! */
	      if (selected)
		current = compute_visible(1);
	      else
		current = 1;
	    }
	    else
	      current = 0;
	}
	else if ((current > message_count)
	       || (selected && i > selected)) {
	    if (message_count > 0) {
	      /* We are out of range! Get to last (visible) message! */
	      if (selected)
		current = visible_to_index(selected)+1;
	      else
		current = message_count;
	    }
	    else
	      current = 0;
	}

}

static char *no_mail = NULL;
static char *no_aliases = NULL;

motion(ch)
char ch;
{
	/* Consolidated the standard menu navigation and delete/tag
	 * commands to a function.                                   */

	int  key_offset;        /** Position offset within keyboard string   **/
	int  i;

	if (no_mail == NULL) {
		no_mail = catgets(elm_msg_cat, ElmSet, ElmNoMailInFolder,
		  "No mail in folder!");
		no_aliases = catgets(elm_msg_cat, ElmSet, ElmNoAliases,
		  "No aliases!");
	}

	switch (ch) {

	    case '/'    :  /* scan mbox or aliases for string */
			   if  (message_count < 1) {
			     error1(catgets(elm_msg_cat, ElmSet,
				ElmNoItemToScan, "No %s to scan!"), items);
			     fflush(stdin);
			   }
			   else if (pattern_match())
			     nucurr = get_page(current);
			   else {
			     error(catgets(elm_msg_cat, ElmSet, ElmPatternNotFound,
			       "pattern not found!"));
			     fflush(stdin);
			   }
			   break;

next_page:
	    case '+'	:  /* move to next page if we're not on the last */
			   if((selected &&
			     ((header_page+1)*headers_per_page < selected))
			   ||(!selected &&
			     ((header_page+1)*headers_per_page<message_count))){

			     header_page++;
			     nucurr = NEW_PAGE;

			     if(move_when_paged) {
			       /* move to first message of new page */
			       if(selected)
				 current = visible_to_index(
				   header_page * headers_per_page + 1) + 1;
			       else
				 current = header_page * headers_per_page + 1;
			     }
			   } else
			     error(catgets(elm_msg_cat, ElmSet, ElmAlreadyOnLastPage,
			       "Already on last page."));
			   break;

prev_page:
	    case '-'	:  /* move to prev page if we're not on the first */
			   if(header_page > 0) {
			     header_page--;
			     nucurr = NEW_PAGE;

			     if(move_when_paged) {
			       /* move to first message of new page */
			       if(selected)
				 current = visible_to_index(
				   header_page * headers_per_page + 1) + 1;
			       else
				 current = header_page * headers_per_page + 1;
			     }
			   } else
			     error(catgets(elm_msg_cat, ElmSet, ElmAlreadyOnFirstPage,
			       "Already on first page."));
			   break;

first_msg:
	    case '='    :  if (selected)
			     current = visible_to_index(1)+1;
			   else
			     current = 1;
			   nucurr = get_page(current);
			   break;

last_msg:
	    case '*'    :  if (selected) 
			     current = (visible_to_index(selected)+1);
			   else
			     current = message_count;	
			   nucurr = get_page(current);
			   break;

	    case ctrl('D') :
	    case '^'    :
	    case 'd'    :  if (message_count < 1) {
			     error1(catgets(elm_msg_cat, ElmSet, ElmNoItemToDelete,
			       "No %s to delete!"), item);
			     fflush(stdin);
			   }
			   else {
			     if(ch == ctrl('D')) {

			       /* if current item did not become deleted,
				* don't to move to the next undeleted item */
			       if(!meta_match(DELETED)) break;

			     } else 
 			       delete_msg((ch == 'd'), TRUE);

			     if (resolve_mode) 	/* move after mail resolved */
			       if((i=next_message(current-1, TRUE)) != -1) {
				 current = i+1;
				 nucurr = get_page(current);
			       }
			   }
			   break;

	    case 'J'    :  if(current > 0) {
			     if((i=next_message(current-1, FALSE)) != -1) {
			       current = i+1;
			       nucurr = get_page(current);
			     } else
			       error1(catgets(elm_msg_cat, ElmSet, ElmNoMoreItemBelow,
				 "No more %s below."), items);
			   } else error(ifmain(no_mail, no_aliases));
			   break;

next_undel_msg:
	    case 'j'    :  if(current > 0) {
			     if((i=next_message(current-1, TRUE)) != -1) {
			       current = i+1;
			       nucurr = get_page(current);
			     } else
			       error1(catgets(elm_msg_cat, ElmSet, ElmNoItemUndeletedBelow,
				 "No more undeleted %s below."), items);
			   } else error(ifmain(no_mail, no_aliases));
			   break;

	    case 'K'    :  if(current > 0) {
			     if((i=prev_message(current-1, FALSE)) != -1) {
			       current = i+1;
			       nucurr = get_page(current);
			     } else
			       error1(catgets(elm_msg_cat, ElmSet, ElmNoMoreItemAbove,
				 "No more %s above."), items);
			   } else error(ifmain(no_mail, no_aliases));
			   break;

prev_undel_msg:
	    case 'k'    :  if(current > 0) {
			     if((i=prev_message(current-1, TRUE)) != -1) {
			       current = i+1;
			       nucurr = get_page(current);
			     } else
			       error1(catgets(elm_msg_cat, ElmSet, ElmNoMoreUndeletedAbove,
				 "No more undeleted %s above."), items);
			   } else error(ifmain(no_mail, no_aliases));
			   break;

	    case 'l'    :  PutLine1(LINES-3, strlen(Prompt),
				   catgets(elm_msg_cat, ElmSet, ElmLimitDisplayBy,
				   "Limit displayed %s by..."), items);
			   clear_error();
			   if (limit() != 0) {
			     get_page(current);
			     redraw++;
			   } else {
			     nufoot++;
			   }
			   break;

            case ctrl('T') :
	    case 'T'	   :
	    case 't'       :  if (message_count < 1) {
				error1(catgets(elm_msg_cat, ElmSet, ElmNoItemToTag,
				  "No %s to tag!"), items);
				fflush(stdin);
			      }
			      else if (ch == 't')
				tag_message(TRUE); 
			      else if (ch == 'T') {
				tag_message(TRUE); 
				goto next_undel_msg;
			      }
			      else
				meta_match(TAGGED);
			      break;

	    case 'u'    :  if (message_count < 1) {
			     error1(catgets(elm_msg_cat, ElmSet, ElmNoItemToMarkUndeleted,
			       "No %s to mark as undeleted!"), items);
			     fflush(stdin);
			   }
			   else {
			     undelete_msg(TRUE);
			     if (resolve_mode) 	/* move after mail resolved */
			       if((i=next_message(current-1, FALSE)) != -1) {
				 current = i+1;
				 nucurr = get_page(current);
			       }
/*************************************************************************
 **  What we've done here is to special case the "U)ndelete" command to
 **  ignore whether the next message is marked for deletion or not.  The
 **  reason is obvious upon usage - it's a real pain to undelete a series
 **  of messages without this quirk.  Thanks to Jim Davis @ HPLabs for
 **  suggesting this more intuitive behaviour.
 **
 **  The old way, for those people that might want to see what the previous
 **  behaviour was to call next_message with TRUE, not FALSE.
**************************************************************************/
			   }
			   break;

	    case ctrl('U') : if (message_count < 1) {
			       error1(catgets(elm_msg_cat, ElmSet, ElmNoItemToUndelete,
				 "No %s to undelete!"), items);
			       fflush(stdin);
			     }
			     else 
			       meta_match(UNDELETE);
			     break;

	    case ctrl('L') : redraw++;	break;

	    case NO_OP_COMMAND : break;	/* noop for timeout loop */

	    case ESCAPE : if (cursor_control) {
			    key_offset = 1;
			    ch = ReadCh(); 
  
                            if ( ch == '[' || ch == 'O')
                            {
                              ch = ReadCh();
                              key_offset++;
                            }
  
			    if(ch == up[key_offset]) goto prev_undel_msg;
			    else if(ch == down[key_offset]) goto next_undel_msg;
			    else if(ch == right[key_offset]) goto next_page;
			    else if(ch == left[key_offset]) goto prev_page;
			    else if (hp_terminal) {
			      switch (ch) {
			      case 'U':		goto next_page;
			      case 'V':		goto prev_page;
			      case 'h': 	
			      case 'H':		goto first_msg;
			      case 'F':		goto last_msg;
			      case 'A':
			      case 'D':
			      case 'i':		goto next_undel_msg;
			      case 'B':
			      case 'I':
			      case 'C':		goto prev_undel_msg;
			      default: PutLine2(LINES-3, strlen(Prompt), 
					"%c%c", ESCAPE, ch);
			      }
			    } else /* false hit - output */
			      PutLine2(LINES-3, strlen(Prompt), 
					  "%c%c", ESCAPE, ch);
			  }

			  /* else fall into the default error message! */

	    default	: if (ch > '0' && ch <= '9') {
			    PutLine1(LINES-3, strlen(Prompt), 
				    catgets(elm_msg_cat, ElmSet, ElmNewCurrentItem,
				    "New Current %s"), Item);
			    i = read_number(ch, item);

			    if( i > message_count)
			      error1(catgets(elm_msg_cat, ElmSet, ElmNotThatMany,
				"Not that many %s."), items);
			    else if(selected
				&& isoff(ifmain(headers[i-1]->status,
			                        aliases[i-1]->status), VISIBLE))
			      error1(catgets(elm_msg_cat, ElmSet, ElmNotInLimitedDisplay,
				"%s not in limited display."), Item);
			    else {
			      current = i;
			      nucurr = get_page(current);
			    }
			  }
			  else {
	 		    error(catgets(elm_msg_cat, ElmSet, ElmUnknownCommand,
			      "Unknown command. Use '?' for help."));
			    fflush(stdin);
			  }
	}
}
