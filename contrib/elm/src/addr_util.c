
static char rcsid[] = "@(#)Id: addr_util.c,v 5.11 1993/08/03 19:28:39 syd Exp ";

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
 * $Log: addr_util.c,v $
 * Revision 1.2  1994/01/14  00:54:24  cp
 * 2.4.23
 *
 * Revision 5.11  1993/08/03  19:28:39  syd
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
 * Revision 5.10  1993/05/31  19:32:20  syd
 * With this patch build_address() should treat local mailing
 * lists and other aliases known by the transport agent as valid
 * addresses.
 * I also conditionalized printing the "Expands to: " message
 * in check_only mode to be done only when there is an expanded
 * address to print. Build_address will inform anyway about an
 * alias that does not exist.
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.9  1993/05/14  03:53:46  syd
 * Fix wrong message being displayed and then overwritten
 * for long aliases.
 * From: "Robert L. Howard" <robert.howard@matd.gatech.edu>
 *
 * Revision 5.8  1993/04/16  03:26:50  syd
 * For many embedded X.400 addresses in the format
 * "/.../.../.../.../"@admd.country NLEN was simply too short and part of
 * the address never made it to the reply address. In my opinion 512 bytes
 * should be enough. So make it LONG_STRING.
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.7  1993/01/19  05:07:05  syd
 * Trim erroreous extra log entry
 * From: Syd
 *
 * Revision 5.6  1993/01/19  04:47:12  syd
 * Significant changes to provide consistent Date and From_ header
 * cracking.  Overhauled date utilities and moved into library.  Moved
 * real_from() into library.  Modified frm, newmail, and readmsg utilities
 * to use library version of real_from().  Moved get_word() from Elm
 * source into library.  Added new library routines atonum() and strfcpy().
 * Fixed trailing backslash bug in len_next().
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.5  1992/12/11  01:45:04  syd
 * remove sys/types.h include, it is now included by defs.h
 * and this routine includes defs.h or indirectly includes defs.h
 * From: Syd
 *
 * Revision 5.4  1992/11/26  00:46:13  syd
 * changes to first change screen back (Raw off) and then issue final
 * error message.
 * From: Syd
 *
 * Revision 5.3  1992/10/31  18:52:51  syd
 * Corrections to Unix date parsing and time zone storage
 * From: eotto@hvlpa.att.com
 *
 * Revision 5.2  1992/10/25  02:18:01  syd
 * fix found_year flag
 * From: Syd
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** This file contains addressing utilities 

**/

#include "headers.h"
#include "s_elm.h"


translate_return(addr, ret_addr)
char *addr, *ret_addr;
{
	/** Return ret_addr to be the same as addr, but with the login 
            of the person sending the message replaced by '%s' for 
            future processing... 
	    Fixed to make "%xx" "%%xx" (dumb 'C' system!) 
	**/

	register int loc, loc2, iindex = 0;
	register char *remaining_addr;
	
/*
 *	check for RFC-822 source route: format @site:usr@site
 *	if found, skip to after the first : and then retry.
 *	source routes can be stacked
 */
	remaining_addr = addr;
	while (*remaining_addr == '@') {
	  loc = qchloc(remaining_addr, ':');
	  if (loc == -1)
	    break;

	  remaining_addr += loc + 1;
	}

	loc2 = qchloc(remaining_addr,'@');
	loc = qchloc(remaining_addr, '%');
	if ((loc < loc2) && (loc != -1))
	  loc2 = loc;

	if (loc2 != -1) {	/* ARPA address. */
	  /* algorithm is to get to '@' sign and move backwards until
	     we've hit the beginning of the word or another metachar.
	  */
	  for (loc = loc2 - 1; loc > -1 && remaining_addr[loc] != '!'; loc--)
	     ;
	}
	else {			/* usenet address */
	  /* simple algorithm - find last '!' */

	  loc2 = strlen(remaining_addr);	/* need it anyway! */

	  for (loc = loc2; loc > -1 && remaining_addr[loc] != '!'; loc--)
	      ;
	}
	
	/** now copy up to 'loc' into destination... **/

	while (iindex <= loc) {
	  ret_addr[iindex] = remaining_addr[iindex];
	  iindex++;
	}

	/** now append the '%s'... **/

	ret_addr[iindex++] = '%';
	ret_addr[iindex++] = 's';

	/*
	 *  and, finally, if anything left, add that
	 * however, just pick up the address part, we do
	 * not want any comments.  Thus stop copying at
	 * the first blank character.
	 */

	if ((loc = qchloc(remaining_addr,' ')) == -1)
	  loc = strlen(addr);
	while (loc2 < loc) {
	  ret_addr[iindex++] = remaining_addr[loc2++];
	  if (remaining_addr[loc2-1] == '%')	/* tweak for "printf" */
	    ret_addr[iindex++] = '%';
	}
	
	ret_addr[iindex] = '\0';
}

int
build_address(to, full_to)
char *to, *full_to;
{
	/** loop on all words in 'to' line...append to full_to as
	    we go along, until done or length > len.  Modified to
	    know that stuff in parens are comments...Returns non-zero
	    if it changed the information as it copied it across...
	**/

	register int	i, j, k, l,
			changed = 0, in_parens = 0,
			expanded_information = 0,
			eliminated = 0;
	int too_long = FALSE;
	char word[SLEN], next_word[SLEN], *ptr, buffer[SLEN];
	char new_to_list[VERY_LONG_STRING];
	char elim_list[SLEN], word_a[SLEN], next_word_a[SLEN];
	char *qstrpbrk(), *gecos;
	extern char *get_full_name(), *get_alias_address();

	new_to_list[0] = '\0';

	i = get_word(to, 0, word, sizeof(word));

	full_to[0] = '\0';

	elim_list[0] = '\0';

	/** Look for addresses to be eliminated from aliases **/
	while (i > 0) {

	  j = get_word(to, i, next_word, sizeof(next_word));

	  if(word[0] == '(')
	    in_parens++;

	  if (in_parens) {
	    if(word[strlen(word)-1] == ')')
	      in_parens--;
	  }

	  else if (word[0] == '-'){
	    for (k=0; word[k]; word[k] = word[k+1],k++);
	    if (elim_list[0] != '\0')
	      strcat(elim_list, " ");
	    strcat(elim_list, word);
	  }
	  if ((i = j) > 0)
	    strcpy(word, next_word);
	}

	if (elim_list[0] != '\0')
	  eliminated++;

	i = get_word(to, 0, word, sizeof(word));

	while (i > 0) {

	  j = get_word(to, i, next_word, sizeof(next_word));

try_new_word:
	  if(word[0] == '(')
	    in_parens++;

	  if (in_parens) {
	    if(word[strlen(word)-1] == ')')
	      in_parens--;
	    strcat(full_to, " ");
	    strcat(full_to, word);
	  }

	  else if (word[0] == '-') {
	  }

	  else if (qstrpbrk(word,"!@:") != NULL) {
	    sprintf(full_to, "%s%s%s", full_to,
                    full_to[0] != '\0'? ", " : "", word);
	  }
	  else if ((ptr = get_alias_address(word, TRUE, &too_long)) != NULL) {

	    /** check aliases for addresses to be eliminated **/
	    if (eliminated) {
	      k = get_word(strip_commas(ptr), 0, word_a, sizeof(word_a));

	      while (k > 0) {
		l = get_word(ptr, k, next_word_a, sizeof(next_word_a));
		if (in_list(elim_list, word_a) == 0)
		  sprintf(full_to, "%s%s%s", full_to,
			  full_to[0] != '\0' ? ", " : "", word_a);
		if ((k = l) > 0)
		  strcpy(word_a, next_word_a);
	      }
	    } else
	      sprintf(full_to, "%s%s%s", full_to, 
                      full_to[0] != '\0'? ", " : "", ptr);
	    expanded_information++;
	  }
	  else if (too_long) {
	 /*
	  *   We don't do any real work here.  But we need some
	  *   sort of test in this line of tests to make sure
	  *   that none of the other else's are tried if the 
	  *   alias expansion failed because it was too long.
	  */
	      dprint(2,(debugfile,"Overflowed alias expansion for %s\n", word));
	  }
	  else if (strlen(word) > 0) {
	    if (valid_name(word)) {
	      if (j > 0 && next_word[0] == '(')	/* already has full name */
		gecos = NULL;
	      else				/* needs a full name */
		gecos = get_full_name(word);
#if defined(INTERNET) & defined(USE_DOMAIN)
	      sprintf(full_to, "%s%s%s@%s%s%s%s",
		      full_to,
		      (full_to[0] ? ", " : ""),
		      word,
		      hostfullname,
		      (gecos ? " (" : ""),
		      (gecos ? gecos : ""),
		      (gecos ? ")" : ""));
#else /* INTERNET and USE_DOMAIN */
	      sprintf(full_to, "%s%s%s%s%s%s",
		      full_to,
		      (full_to[0] ? ", " : ""),
		      word,
		      (gecos ? " (" : ""),
		      (gecos ? gecos : ""),
		      (gecos ? ")" : ""));
#endif /* INTERNET and USE_DOMAIN */
	    }
	    else if (check_only) {
		if (! isatty(fileno(stdin)) ) {
		    /*
		     *	batch mode error!
		     */
		    Raw(OFF);
		    fprintf(stderr,
			    catgets(elm_msg_cat, ElmSet, ElmCannotExpandNoCR,
				    "Cannot expand alias '%s'!\n"),
			    word);
		    fprintf(stderr,
			    catgets(elm_msg_cat, ElmSet, ElmUseCheckalias,
				    "Use \"checkalias\" to find valid addresses!\n"));
		    dprint(1, (debugfile,
			       "Can't expand alias %s - bailing out of build_address\n", 
			       word));
		    leave(0);
		}
		else {
		    printf(catgets(elm_msg_cat, ElmSet, ElmAliasUnknown,
			"(alias \"%s\" is unknown)\n\r"), word);
		    changed++;
		}
	    }
	    else {
	      sprintf(full_to, "%s%s%s",
		      full_to,
		      (full_to[0] ? ", " : ""),
		      word);
	    }
	  }

	  /* and this word to the new to list */
	  if(*new_to_list != '\0')
	    strcat(new_to_list, " ");
	  strcat(new_to_list, word);

	  if((i = j) > 0)
	    strcpy(word, next_word);
	}

	/* if new to list is different from original, update original */
	if (changed)
	  strcpy(to, new_to_list);

	return( expanded_information > 0 ? 1 : 0 );
}


forwarded(buffer, entry)
char *buffer;
struct header_rec *entry;
{
	/** Change 'from' and date fields to reflect the ORIGINATOR of 
	    the message by iteratively parsing the >From fields... 
	    Modified to deal with headers that include the time zone
	    of the originating machine... **/

	char machine[SLEN], buff[SLEN], holding_from[SLEN];
	int len;

	machine[0] = holding_from[0] = '\0';

	sscanf(buffer, "%*s %s", holding_from);

	/* after skipping over From and address, process rest as date field */

	while (!isspace(*buffer)) buffer++;	/* skip From */
	while (isspace(*buffer)) buffer++;

	while (*buffer) {
	  len = len_next_part(buffer);
	  if (len > 1) {
	    buffer += len;
	  } else {
	    if (isspace(*buffer))
	      break;
	    buffer++;
	  }
	}
	while (isspace(*buffer)) buffer++;

	parse_arpa_date(buffer, entry);

	/* the following fix is to deal with ">From xyz ... forwarded by xyz"
	   which occasionally shows up within AT&T.  Thanks to Bill Carpenter
	   for the fix! */

	if (strcmp(machine, holding_from) == 0)
	  machine[0] = '\0';

	if (machine[0] == '\0')
	  strcpy(buff, holding_from[0] ? holding_from : "anonymous");
	else
	  sprintf(buff,"%s!%s", machine, holding_from);

	strfcpy(entry->from, buff, STRING);
}


fix_arpa_address(address)
char *address;
{
	/** Given a pure ARPA address, try to make it reasonable.

	    This means that if you have something of the form a@b@b make 
            it a@b.  If you have something like a%b%c%b@x make it a%b@x...
	**/

	register int host_count = 0, i;
	char     hosts[MAX_HOPS][LONG_STRING];	/* array of machine names */
	char     *host, *addrptr;
	extern char *get_token();

	/*  break down into a list of machine names, checking as we go along */
	
	addrptr = (char *) address;

	while ((host = get_token(addrptr, "%@", 2)) != NULL) {
	  for (i = 0; i < host_count && ! equal(hosts[i], host); i++)
	      ;

	  if (i == host_count) {
	    strcpy(hosts[host_count++], host);
	    if (host_count == MAX_HOPS) {
	       dprint(2, (debugfile, 
           "Can't build return address - hit MAX_HOPS in fix_arpa_address\n"));
	       error(catgets(elm_msg_cat, ElmSet, ElmCantBuildRetAddr,
			"Can't build return address - hit MAX_HOPS limit!"));
	       return(1);
	    }
	  }
	  else 
	    host_count = i + 1;
	  addrptr = NULL;
	}

	/** rebuild the address.. **/

	address[0] = '\0';

	for (i = 0; i < host_count; i++)
	  sprintf(address, "%s%s%s", address, 
	          address[0] == '\0'? "" : 
	 	    (i == host_count - 1 ? "@" : "%"),
	          hosts[i]);

	return(0);
}
