
static char rcsid[] = "@(#)Id: help.c,v 5.4 1993/08/03 19:28:39 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.2 $   $State: Exp $
 *
 *			Copyright (c) 1988-1992 USENET Community Trust
 *			Copyright (c) 1986,1987 Dave Taylor
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein, Elm Coordinator
 *	elm@DSI.COM			dsinc!elm
 *
 *******************************************************************************
 * $Log: help.c,v $
 * Revision 1.2  1994/01/14  00:55:03  cp
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
 * Revision 5.3  1993/04/12  03:15:41  syd
 * These patches makes 'T' (since it was free) do a Tag and Move command in the
 * index and alias page, and in the builtin pager.
 * In the alias help in src/alias.c, there is a tolower done on the character
 * one wants help for.  This is clearly wrong.
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.2  1993/01/20  03:37:16  syd
 * Nits and typos in the NLS messages and corresponding default messages.
 * From: dwolfe@pffft.sps.mot.com (Dave Wolfe)
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/*** help routine for ELM program 

***/

#include "headers.h"
#include "s_elm.h"

help(pager_help)
int pager_help;
{
	/** Process the request for help [section] from the user.
	    If pager_help is TRUE, then act a little differently from
	    if pager_help is FALSE (index screen)
	 **/

	char ch;		/* character buffer for input */
	char *s;		/* string pointer...	      */
	int prompt_line, info_line;
	static char *help_message = NULL;
	static char *help_prompt = NULL;
	static char *unknown_command = NULL;

	if (help_message == NULL) {
		help_message = catgets(elm_msg_cat, ElmSet, ElmHelpMessage,
   "Press the key you want help for, '?' for a key list, or '.' to exit help");
		help_prompt = catgets(elm_msg_cat, ElmSet, ElmHelpPrompt,
			"Help for key: ");
		unknown_command = catgets(elm_msg_cat, ElmSet, ElmHelpUnknownCommand,
			  "Unknown command.  Use '?' for a list of commands.");
	}

	MoveCursor(LINES-4,0);
	CleartoEOS();

	if(pager_help) {
	  put_border();
	  Centerline(LINES, help_message);
	  prompt_line = LINES-3;
	} else {
	  Centerline(LINES-4, catgets(elm_msg_cat, ElmSet, ElmHelpSystem,
		"ELM Help System"));
	  Centerline(LINES-3, help_message);
	  prompt_line = LINES-2;
	}
	info_line = prompt_line + 1;

	PutLine0(prompt_line, 0, help_prompt);

	do {
	  MoveCursor(prompt_line, strlen(help_prompt));
	  ch = ReadCh();
	  
	  if (ch == '.') return(0);	/* zero means footer rewrite only */

	  s = unknown_command;

	  switch (ch) {

	    case '?': display_helpfile(pager_help? PAGER_HELP : MAIN_HELP);
		      return(1);

	    case '$': if(!pager_help) s = catgets(elm_msg_cat, ElmSet, ElmHelpDollar,
"$ = Force resynchronization of the current folder. This will purge deleted mail.");
		      break;

	    case '!': s = catgets(elm_msg_cat, ElmSet, ElmHelpShell,
     "! = Escape to the UNIX shell of your choice, or just to enter commands.");
	              break;

	    case '@': if(!pager_help) s = catgets(elm_msg_cat, ElmSet, ElmHelpDebugSummary,
	   "@ = Debug - display a summary of the messages on the header page.");
		      break;

	    case '|': s = catgets(elm_msg_cat, ElmSet, ElmHelpPipe,
   "| = Pipe the current message or tagged messages to the command specified.");
		      break;

	    case '#': if(!pager_help) s = catgets(elm_msg_cat, ElmSet, ElmHelpDebugAll,
	    "# = Debug - display all information known about current message.");
		      break;

	    case '%': s = catgets(elm_msg_cat, ElmSet, ElmHelpDebugReturnAdd,
     "% = Debug - display the computed return address of the current message.");
		      break;

	    case '*': if(!pager_help)
		       s = catgets(elm_msg_cat, ElmSet, ElmHelpLastMessage,
				"* = Go to the last message in the current folder.");
		      break;

	    case '-': if(!pager_help) s = catgets(elm_msg_cat, ElmSet, ElmHelpPreviousPage,
"- = Go to the previous page of messages.  This is the same as the LEFT arrow.");
		      break;

	    case '=': if(!pager_help) s = catgets(elm_msg_cat, ElmSet, ElmHelpFirstMessage,
			"'=' = Go to the first message in the current folder.");
		      break;

	    case ' ': if(pager_help)
			s = catgets(elm_msg_cat, ElmSet, ElmHelpNextScreen,
  "<space> = Display next screen of current message (or first screen of next).");
		      else
			s = catgets(elm_msg_cat, ElmSet, ElmHelpDisplayCurrent,
				"<space> = Display the current message.");
		      break;

	    case '+': if(!pager_help)
	    		s = catgets(elm_msg_cat, ElmSet, ElmHelpNextPage,
  "+ = Go to the next page of messages.  This is the same as the RIGHT arrow.");
		      break;

	    case '/': if(!pager_help)
			s = catgets(elm_msg_cat, ElmSet, ElmHelpSearchFolder,
				"/ = Search for specified pattern in folder.");
		      break;

#ifdef ENABLE_CALENDAR
	    case '<': s = catgets(elm_msg_cat, ElmSet, ElmHelpCalendar,
	       "< = Scan current message for calendar entries (if enabled).");
		      break;
#endif

	    case '>': s = catgets(elm_msg_cat, ElmSet, ElmHelpSave,
	       "> = Save current message or tagged messages to specified file.");
		      break;

	    case '^': if(!pager_help) s = catgets(elm_msg_cat, ElmSet, ElmHelpToggleStatus,
	       "^ = Toggle the Delete/Undelete status of the current message.");
		      break;

	    case 'a': if(!pager_help) s = catgets(elm_msg_cat, ElmSet, ElmHelpAliasSubmenu,
	   "a = Enter the alias sub-menu section.  Create and display aliases.");
		      break;

	    case 'b': s = catgets(elm_msg_cat, ElmSet, ElmHelpBounce,
      "b = Bounce (remail) a message to someone as if you have never seen it.");
		      break;

	    case 'C': s = catgets(elm_msg_cat, ElmSet, ElmHelpCopy,
               "C = Copy current message or tagged messages to specified file.");
		      break;

	    case 'c': if(!pager_help)
				s = catgets(elm_msg_cat, ElmSet, ElmHelpChangeFolder,
	 "c = Change folders, leaving the current folder as if 'quitting'.");
		      break;

	    case 'd': s = catgets(elm_msg_cat, ElmSet, ElmHelpDelete,
			"d = Mark the current message for future deletion.");
		      break;

	    case ctrl('D') : if(!pager_help) s = catgets(elm_msg_cat, ElmSet, ElmHelpDeletePattern,
	      "^D = Mark for deletion all messages with the specified pattern.");
		      break;

#ifdef ALLOW_MAILBOX_EDITING
	    case 'e': if(!pager_help) s = catgets(elm_msg_cat, ElmSet, ElmHelpEditor,
     "e = Invoke the editor on the entire folder, resynchronizing when done.");
		      break;
#endif

	    case 'f': s = catgets(elm_msg_cat, ElmSet, ElmHelpForward,
	"f = Forward the current message to someone, return address is yours.");
		      break;

	    case 'g': s = catgets(elm_msg_cat, ElmSet, ElmHelpGroupReply,
   "g = Group reply not only to the sender, but to everyone who received msg.");
		      break;

	    case 'h': s = catgets(elm_msg_cat, ElmSet, ElmHelpDisplayHeaders,
		 "h = Display message with all Headers (ignore weedout list).");
	              break;

	    case 'i': if(pager_help)
			s = catgets(elm_msg_cat, ElmSet, ElmHelpReturnToIndex,
				"i = Return to the index.");
		      break;

	    case 'J': s = catgets(elm_msg_cat, ElmSet, ElmHelpNextMessage,
			"J = Go to the next message.");
		      break;

	    case 'j': s = catgets(elm_msg_cat, ElmSet, ElmHelpNextUndeleted,
  "j = Go to the next undeleted message.  This is the same as the DOWN arrow.");
		      break;

	    case 'K': s = catgets(elm_msg_cat, ElmSet, ElmHelpPreviousMessage,
			"K = Go to the previous message.");
		      break;

	    case 'k': s = catgets(elm_msg_cat, ElmSet, ElmHelpPreviousUndeleted,
"k = Go to the previous undeleted message.  This is the same as the UP arrow.");
		      break;

	    case 'l': if(!pager_help)
			s = catgets(elm_msg_cat, ElmSet, ElmHelpLimitDisplay,
               "l = Limit displayed messages based on the specified criteria.");
		      break;

	    case 'm': s = catgets(elm_msg_cat, ElmSet, ElmHelpSendMail,
		 "m = Create and send mail to the specified person or persons.");
		      break;

	    case 'n': if(pager_help)
			s = catgets(elm_msg_cat, ElmSet, ElmHelpDisplayNext,
				"n = Display the next message.");
		      else
			s = catgets(elm_msg_cat, ElmSet, ElmHelpDisplayThenNext,
	   "n = Display the current message, then move current to next message.");
		      break;

	    case 'o': if(!pager_help)
			s = catgets(elm_msg_cat, ElmSet, ElmHelpOptionsMenu,
				"o = Go to the options submenu.");
	              break;

	    case 'p': s = catgets(elm_msg_cat, ElmSet, ElmHelpPrint,
		      "p = Print the current message or the tagged messages.");
	              break;

	    case 'q': if(pager_help)
			s = catgets(elm_msg_cat, ElmSet, ElmHelpQuitPager,
			  "q = Quit the pager and return to the index.");
		      else s = catgets(elm_msg_cat, ElmSet, ElmHelpQuitMailer,
		    "q = Quit the mailer, asking about deletion, saving, etc.");
		      break;

	    case 'r': s = catgets(elm_msg_cat, ElmSet, ElmHelpReplyMessage,
"r = Reply to the message.  This only sends to the originator of the message.");
	              break;

	    case 's': s = catgets(elm_msg_cat, ElmSet, ElmHelpSaveMessage,
               "s = Save current message or tagged messages to specified file.");
		      break;

	    case 't': s = catgets(elm_msg_cat, ElmSet, ElmHelpTagMessage,
               "t = Tag a message for further operations (or untag if tagged).");
		      break;

	    case 'T': s = catgets(elm_msg_cat, ElmSet, ElmHelpTagAndNext,
               "T = Tag a message (or untag if tagged) and go to next message.");
		      break;

	    case ctrl('T') : if(!pager_help)
			s = catgets(elm_msg_cat, ElmSet, ElmHelpTagPattern,
			    "^T = Tag all messages with the specified pattern.");
		      break;

	    case 'u': s = catgets(elm_msg_cat, ElmSet, ElmHelpUndeleteMessage,
		      "u = Undelete - remove the deletion mark on the message.");
		      break;

	    case ctrl('U') : s = catgets(elm_msg_cat, ElmSet, ElmHelpUndeletePattern,
		      "^U = Undelete all messages with the specified pattern.");
		      break;

	    case 'x': s = catgets(elm_msg_cat, ElmSet, ElmHelpExitFolder,
			"x = Exit leaving the folder untouched, ask permission if changed.");
		      break;

	    case 'X': s = catgets(elm_msg_cat, ElmSet, ElmHelpQuickExit,
			"X = Exit leaving the folder untouched, unconditionally.");
		      break;

	    case 'Q': if(!pager_help)
			s = catgets(elm_msg_cat, ElmSet, ElmHelpQuickQuit,
		"Q = Quick quit the mailer, save read, leave unread, delete deleted.");
		      break;

	    case '\n':
	    case '\r': if(pager_help)
			 s = catgets(elm_msg_cat, ElmSet, ElmHelpScrollForward,
  "<return> = Display current message, or (builtin pager only) scroll forward.");
		       else
			 s = catgets(elm_msg_cat, ElmSet, ElmHelpDisplayCurrentMessage,
				"<return> = Display the current message.");
		       break;

	    case ctrl('L'): if(!pager_help)
			s = catgets(elm_msg_cat, ElmSet, ElmHelpRewriteScreen,
				"^L = Rewrite the screen.");	
		       break;

            case ctrl('?'):					    /* DEL */
	    case ctrl('Q'): if(!pager_help)
				s = catgets(elm_msg_cat, ElmSet, ElmHelpExitQuickly,
					"Exit the mail system quickly.");
		       break;

	    default : if (isdigit(ch) && !pager_help) 
	            s = catgets(elm_msg_cat, ElmSet, ElmHelpMakeMessageCurrent,
			"<number> = Make specified number the current message.");
	  }

	  ClearLine(info_line);
	  Centerline(info_line, s);

	} while (ch != '.');
	
	/** we'll never actually get here, but that's okay... **/

	return(0);
}

display_helpfile(section)
int section;
{
	/*** Help me!  Read file 'helpfile.<section>' and echo to screen ***/

	char buffer[SLEN];

	sprintf(buffer, "%s/%s.%d", helphome, helpfile, section);
	return(display_file(buffer));
}

display_file(file)
char *file;
{
	/*** Display file to screen ***/

	FILE *fileptr;
	int  lines=0;
	char buffer[SLEN];

	if ((fileptr = fopen(file,"r")) == NULL) {
	  dprint(1, (debugfile,
		 "Error: Couldn't open file %s (help)\n", file));
	  error1(catgets(elm_msg_cat, ElmSet, ElmHelpCouldntOpenFile,
		"Couldn't open file %s."), file);
	  return(FALSE);
	}
	
	ClearScreen();

	while (fgets(buffer, SLEN, fileptr) != NULL) {
	  if (lines > LINES-3) {
	    PutLine0(LINES, 0, catgets(elm_msg_cat, ElmSet, ElmHelpPressSpaceToContinue,
		"Press <space> to continue, 'q' to return."));
	    if(ReadCh() == 'q') {
	      clear_error();
	      fclose(fileptr);
	      return(TRUE);
	    }
	    lines = 0;
	    ClearScreen();
	    Write_to_screen("%s\r", 1, buffer);
	  }
	  else 
	    Write_to_screen("%s\r", 1, buffer);

	  lines += strlen(buffer)/COLUMNS + 1;
	}

        PutLine0(LINES, 0, catgets(elm_msg_cat, ElmSet, ElmHelpPressAnyKeyToReturn,
		"Press any key to return."));

	(void) ReadCh();
	clear_error();

	fclose(fileptr);
	return(TRUE);
}
