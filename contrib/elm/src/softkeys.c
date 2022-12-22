
static char rcsid[] = "@(#)Id: softkeys.c,v 5.2 1993/04/16 03:51:38 syd Exp ";

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
 * $Log: softkeys.c,v $
 * Revision 1.2  1994/01/14  00:55:49  cp
 * 2.4.23
 *
 * Revision 5.2  1993/04/16  03:51:38  syd
 * I found a small screen corruption problem with elm.  If you select 'c' for
 * change mailbox and then back out by just typing <Return> at the prompt for the
 * new mailbox, the prompt 'Command: ' seems to be reprinted on the wrong line
 * leaving the previous prompt and the 'Change Mailbox' string in place.
 *
 * Further investiagtion tracked the problem to the softkey programming.  When a
 * newline (LF) character is sent to the terminal in the sofkey's "send" string,
 * the terminal still performs a linefeed just as if the linefeed were sent at any
 * other time.  The particlar set of softkeys for change mode has four such
 * newlines and this is enought to cause the screen to scroll up one line.  This
 * in turn causes the prompt to appear on the wrong line.
 *
 * I have fixed this by replacing the newlines with carriage returns instead.  I
 * checked and this still performs the desired action without actually scrolling
 * the screen.
 * From: Steve Fosdick <stevef@aom.bt.co.uk>
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 *******************************************************************************
 */

#include "headers.h"
#include "s_elm.h"

define_softkeys(level)
int level;
{
	if (! hp_softkeys) return;

	if (level == MAIN) {

	  define_key(f_key1, catgets(elm_msg_cat, ElmSet, ElmKeyF1,
		" Display   Msg"),   "\r");
	  define_key(f_key2, catgets(elm_msg_cat, ElmSet, ElmKeyF2,
		"  Mail     Msg"),   "m");
	  define_key(f_key3, catgets(elm_msg_cat, ElmSet, ElmKeyF3,
		"  Reply  to Msg"),  "r");

	  if (user_level == 0) {
	    define_key(f_key4, catgets(elm_msg_cat, ElmSet, ElmKey0F4,
		"  Save     Msg"),   "s");
	    define_key(f_key5, catgets(elm_msg_cat, ElmSet, ElmKey0F5
		," Delete    Msg"),   "d");
	    define_key(f_key6, catgets(elm_msg_cat, ElmSet, ElmKey0F6,
		"Undelete   Msg"),   "u");
   	  }
	  else {
	    define_key(f_key4, catgets(elm_msg_cat, ElmSet, ElmKey1F4,
		" Change  Folder"), "c");
	    define_key(f_key5, catgets(elm_msg_cat, ElmSet, ElmKey1F5,
		"  Save     Msg"),   "s");
	    define_key(f_key6, catgets(elm_msg_cat, ElmSet, ElmKey1F6,
		" Delete/Undelete"), "^");
	  }

	  define_key(f_key7, catgets(elm_msg_cat, ElmSet, ElmKeyF7,
		" Print     Msg"),   "p");
	  define_key(f_key8, catgets(elm_msg_cat, ElmSet, ElmKeyF8,
		"  Quit     ELM"),   "q");
	}
	else if (level == ALIAS) {
	  define_key(f_key1, catgets(elm_msg_cat, ElmSet, ElmKeyAF1,
		" Alias  Current"),  "a");
	  define_key(f_key2, catgets(elm_msg_cat, ElmSet, ElmKeyAF2,
		" Check  Person"),   "p");
	  define_key(f_key3, catgets(elm_msg_cat, ElmSet, ElmKeyAF3,
		" Check  System"),   "s");
	  define_key(f_key4, catgets(elm_msg_cat, ElmSet, ElmKeyAF4,
		" Make    Alias"),   "m");
	  clear_key(f_key5);
	  clear_key(f_key6);
	  clear_key(f_key7);
	  define_key(f_key8, catgets(elm_msg_cat, ElmSet, ElmKeyAF8,
		" Return  to ELM"),  "r");
	}
	else if (level == YESNO) {
	  define_key(f_key1, catgets(elm_msg_cat, ElmSet, ElmKeyYF1,
		"  Yes"),  "y");
	  clear_key(f_key2);
	  clear_key(f_key3);
	  clear_key(f_key4);
	  clear_key(f_key5);
	  clear_key(f_key6);
	  clear_key(f_key7);
	  define_key(f_key8, catgets(elm_msg_cat, ElmSet, ElmKeyYF8,
		"   No"),  "n");
	}
	else if (level == READ) {
	  define_key(f_key1, catgets(elm_msg_cat, ElmSet, ElmKeyRF1,
		"  Next    Page  "), " ");
	  clear_key(f_key2);
	  define_key(f_key3, catgets(elm_msg_cat, ElmSet, ElmKeyRF3,
		"  Next    Msg   "), "j");
	  define_key(f_key4, catgets(elm_msg_cat, ElmSet, ElmKeyRF4,
		"  Prev    Msg   "), "k");
	  define_key(f_key5, catgets(elm_msg_cat, ElmSet, ElmKeyRF5,
		"  Reply  to Msg "), "r");
	  define_key(f_key6, catgets(elm_msg_cat, ElmSet, ElmKeyRF6,
		" Delete   Msg   "), "d");
	  define_key(f_key7, catgets(elm_msg_cat, ElmSet, ElmKeyRF7,
		"  Send    Msg   "), "m");
	  define_key(f_key8, catgets(elm_msg_cat, ElmSet, ElmKeyRF8,
		" Return  to ELM "), "q");
	}
	else if (level == CHANGE) {
	  define_key(f_key1, catgets(elm_msg_cat, ElmSet, ElmKeyCF1,
		"  Mail  Directry"), "=/");
	  define_key(f_key2, catgets(elm_msg_cat, ElmSet, ElmKeyCF2,
		"  Home  Directry"), "~/");
	  clear_key(f_key3);
	  define_key(f_key4, catgets(elm_msg_cat, ElmSet, ElmKeyCF4,
		"Incoming Mailbox"), "!\r");
	  define_key(f_key5, catgets(elm_msg_cat, ElmSet, ElmKeyCF5,
		"\"Received\" Folder"), ">\r");
	  define_key(f_key6, catgets(elm_msg_cat, ElmSet, ElmKeyCF6,
		"\"Sent\"   Folder "), "<\r");
	  clear_key(f_key7);
	  define_key(f_key8, catgets(elm_msg_cat, ElmSet, ElmKeyCF8,
		" Cancel"), "\n");
	}

	softkeys_on();
}

define_key(key, display, send)
int key;
char *display, *send;
{

	char buffer[30];

	sprintf(buffer,"%s%s", display, send);

	fprintf(stderr, "%c&f%dk%dd%dL%s", ESCAPE, key,
		strlen(display), strlen(send), buffer);
	fflush(stdout);
}

softkeys_on()	
{ 
	/* enable (esc&s1A) turn on softkeys (esc&jB) and turn on MENU 
	   and USER/SYSTEM options. */

	if (hp_softkeys) {
	  fprintf(stderr, "%c&s1A%c&jB%c&jR", ESCAPE, ESCAPE, ESCAPE); 
	  fflush(stdout);
	}
	
}

softkeys_off()	
{ 
	/* turn off softkeys (esc&j@) */

	if (hp_softkeys) {
	  fprintf(stderr, "%c&s0A%c&j@", ESCAPE, ESCAPE); 
	  fflush(stdout);
	}
}

clear_key(key)  
{ 	
	/** set a key to nothing... **/

	if (hp_softkeys) 
	   define_key(key, "                ", ""); 
}
