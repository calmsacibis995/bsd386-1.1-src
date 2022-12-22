
static char rcsid[] = "@(#)Id: mime.c,v 5.15 1993/08/23 02:55:05 syd Exp ";

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
 ******************************************************************************
 * $Log: mime.c,v $
 * Revision 1.2  1994/01/14  00:55:17  cp
 * 2.4.23
 *
 * Revision 5.15  1993/08/23  02:55:05  syd
 * Add missing parens
 * From: dwolfe@pffft.sps.mot.com (Dave Wolfe)
 *
 * Revision 5.14  1993/08/10  18:53:31  syd
 * I compiled elm 2.4.22 with Purify 2 and fixed some memory leaks and
 * some reads of unitialized memory.
 * From: vogt@isa.de
 *
 * Revision 5.13  1993/08/03  19:28:39  syd
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
 * Revision 5.12  1993/07/20  02:41:24  syd
 * Three changes to expand_env() in src/read_rc.c:  make it non-destructive,
 * have it return an error code instead of bailing out, and add a buffer
 * size argument to avoid overwritting the destination.  The first is to
 * avoid all of the gymnastics Elm needed to go through (and occasionally
 * forgot to go through) to protect the value handed to expand_env().
 * The second is because expand_env() was originally written to support
 * "elmrc" and bailing out was a reasonable thing to do there -- but not
 * in the other places where it has since been used.  The third is just
 * a matter of practicing safe source code.
 *
 * This patch changes all invocations to expand_env() to eliminate making
 * temporary copies (now that the routine is non-destructive) and to pass
 * in a destination length.  Since expand_env() no longer bails out on
 * error, a do_expand_env() routine was added to src/read_rc.c handle
 * this.  Moreover, the error message now gives some indication of what
 * the problem is rather than just saying "can't expand".
 *
 * Gratitous change to src/editmsg.c renaming filename variables to
 * clarify the purpose.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.11  1993/06/10  03:12:10  syd
 * Add missing rcs id lines
 * From: Syd
 *
 * Revision 5.10  1993/05/14  03:56:19  syd
 * A MIME body-part must end with a newline even when there was no newline
 * at the end of the actual body or the body is null. Otherwise the next
 * mime boundary may not be recognized.  The same goes with the closing
 * boundary too.
 * From: Jukka Ukkonen <ukkonen@csc.fi>
 *
 * Revision 5.9  1993/05/08  20:25:33  syd
 * Add sleepmsg to control transient message delays
 * From: Syd
 *
 * Revision 5.8  1992/12/11  01:45:04  syd
 * remove sys/types.h include, it is now included by defs.h
 * and this routine includes defs.h or indirectly includes defs.h
 * From: Syd
 *
 * Revision 5.7  1992/11/26  00:46:13  syd
 * changes to first change screen back (Raw off) and then issue final
 * error message.
 * From: Syd
 *
 * Revision 5.6  1992/11/22  01:22:48  syd
 * According to the MIME BNF, quoted strings are allowed in the value portion
 * of a parameter.
 * From: chk@alias.com (C. Harald Koch)
 *
 * Revision 5.5  1992/11/07  16:21:56  syd
 * There is no need to write out the MIME-Version header in subparts
 * From: Klaus Steinberger <Klaus.Steinberger@Physik.Uni-Muenchen.DE>
 *
 * Revision 5.4  1992/10/30  21:10:39  syd
 * it invokes metamail (the pseudo is because "text" isn't a legal Content-Type).
 * in src/mime.c notplain() tries to check for text but fails because it should
 * look for "text\n" not "text".
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.3  1992/10/25  01:47:45  syd
 * fixed a bug were elm didn't call metamail on messages with a characterset,
 * which could be displayed by elm itself, but message is encoded with QP
 * or BASE64
 * From: Klaus Steinberger <Klaus.Steinberger@Physik.Uni-Muenchen.DE>
 *
 * Revision 5.2  1992/10/24  13:44:41  syd
 * There is now an additional elmrc option "displaycharset", which
 * sets the charset supported on your terminal. This is to prevent
 * elm from calling out to metamail too often.
 * Plus a slight documentation update for MIME composition (added examples)
 * From: Klaus Steinberger <Klaus.Steinberger@Physik.Uni-Muenchen.DE>
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/


#include "headers.h"
#include "s_elm.h"

#ifdef MIME

#include <errno.h>
#include <sys/stat.h>

int check_for_multipart(filedesc)
FILE *filedesc;
{
  char buffer[SLEN];
  int Multi_Part = FALSE;
  char *ptr;
  char *incptr;
  char Include_Filename[SLEN];
  char Expanded_Filename[SLEN];

  while (mail_gets(buffer, SLEN, filedesc))
    if (buffer[0] == '[') {
      if (strncmp(buffer, MIME_INCLUDE, strlen(MIME_INCLUDE)) == 0) {
      	Multi_Part = TRUE;
	if (Include_Part((FILE *)NULL, buffer, TRUE) == -1) {
	   return(-1);
	}
      }
    }
    rewind(filedesc);
  return(Multi_Part);
}

Include_Part(dest, buffer, check)
FILE *dest;
char *buffer;
int	check;
{
  char *ptr;
  char *incptr;
  char Include_Filename[SLEN];
  char Expanded_Filename[SLEN];
  char tmp_fn[SLEN];
  char *filename;
  char Content_Type[SLEN];
  char Encoding[SLEN];
  char sh_buffer[SLEN];
  char Encode_Flag[3];
  int  Enc_Type;
  FILE *incfile;
  struct stat	file_status;
  int  line_len;

  ptr = buffer + strlen(MIME_INCLUDE);
  while ((*ptr != '\0') && (*ptr == ' '))
    ptr++;
  incptr = Include_Filename;
  while ((*ptr != ' ') && (*ptr != ']') && (*ptr != '\0'))
    *incptr++ = *ptr++;
  *incptr = '\0';

  while ((*ptr != '\0') && (*ptr == ' '))
    ptr++;
  incptr = Content_Type;
  while ((*ptr != ' ') && (*ptr != ']') && (*ptr != '\0'))
    *incptr++ = *ptr++;
  *incptr = '\0';

  while ((*ptr != '\0') && (*ptr == ' '))
    ptr++;
  incptr = Encoding;
  while ((*ptr != ' ') && (*ptr != ']') && (*ptr != '\0'))
    *incptr++ = *ptr++;
  *incptr = '\0';

  if (strlen(Include_Filename) == 0) {
    Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmNoIncludeFilename,
                    "\n\rNo Filename given, include line ignored\n\r"), 0);
    if (sleepmsg > 0)
	    sleep(sleepmsg);
    return(-1);
  }
  (void) expand_env(Expanded_Filename, Include_Filename, sizeof(Expanded_Filename));

  if (strlen(Content_Type) == 0) {
    Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmNoContentTypeGiven,
                    "\n\rNo Content-type given, include line ignored\n\r"), 0);
    if (sleepmsg > 0)
	    sleep(sleepmsg);
    return(-1);
  }

  Enc_Type = check_encoding(Encoding);

  if (Enc_Type == ENCODING_ILLEGAL) {
        Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmEncodingIsIllegal,
			"\n\rEncoding is illegal\n\r"), 0);
	if (sleepmsg > 0)
		sleep(sleepmsg);
	return(-1);
  }

  if (can_open(Expanded_Filename, "r")) {
        Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmIncludeCannotAccess,
			"\n\rInclude File can't be accessed\n\r"), 0);
	if (sleepmsg > 0)
		sleep(sleepmsg);
	return(-1);
  }
  if (check) {
	return(0);
  }
  if (Enc_Type == ENCODING_7BIT || Enc_Type == ENCODING_8BIT ||
	Enc_Type == ENCODING_BINARY || Enc_Type == ENCODING_NONE ||
	Enc_Type == ENCODING_EXPERIMENTAL) {
    /* No explicit encoding, assume 7-BIT */
    filename = Expanded_Filename;
  } else {
    sprintf(tmp_fn, "%semm.%d.%d", temp_dir, getpid(), getuid());
    filename = tmp_fn;
    if (Enc_Type == ENCODING_BASE64) {
      strcpy(Encode_Flag, "-b");
    } else if (Enc_Type == ENCODING_QUOTED) {
      strcpy(Encode_Flag, "-q");
    } else {
      Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmUnknownEncodingInInclude,
		      "\n\rUnknown Encoding, include line ignored\n\r"), 0);
      if (sleepmsg > 0)
	    sleep(sleepmsg);
      return(-1);
    }
    sprintf(sh_buffer, "mmencode %s %s >%s", Encode_Flag, Expanded_Filename,
			tmp_fn);
    (void) system_call(sh_buffer, 0);
  }
  if ((incfile = fopen(filename, "r")) != NULL) {
    if (stat(filename, &file_status) != 0) {
        Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmCantStatIncludedFile,
			"\n\rCan't stat included File,ignored\n\r"), 0);
	if (sleepmsg > 0)
		sleep(sleepmsg);
	return(-1);
    }
    fprintf(dest, "%s %s\n", MIME_CONTENTTYPE, Content_Type);
    fprintf(dest, "Content-Name: %s\n", Include_Filename);
    fprintf(dest, "Content-Length: %d\n", file_status.st_size);
    if (Enc_Type != ENCODING_NONE) {
	fprintf(dest, "Content-Transfer-Encoding: %s\n", Encoding);
    }
    fprintf(dest, "\n");
    while (line_len = fread(buffer, 1, sizeof(buffer), incfile)) {
      if (fwrite(buffer, 1, line_len, dest) != line_len) {
	MoveCursor(LINES, 0);
	Raw(OFF);
        Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmWriteFailedCopyAcross,
	        "\nWrite failed in copy_message_across\n"), 0);
        emergency_exit();
      }
    } 

/*  write CRLF after body-part */

    fprintf(dest, "\n");

    fclose(incfile);
    if (filename == tmp_fn) {
	unlink(tmp_fn);
    }
  } else {
    MoveCursor(LINES, 0);
    Raw(OFF);
    Write_to_screen(catgets(elm_msg_cat, ElmSet, ElmCantOpenIncludedFile,
		    "\nCan't open included File\n"), 0);
    emergency_exit();
  }
  return(0);
}

int check_encoding(Encoding)
char *Encoding;
{
	if (strlen(Encoding) == 0) return (ENCODING_NONE);
	if (strincmp(Encoding, ENC_NAME_7BIT, strlen(ENC_NAME_7BIT)) == 0)
		return (ENCODING_7BIT);
	if (strincmp(Encoding, ENC_NAME_8BIT, strlen(ENC_NAME_8BIT)) == 0)
		return (ENCODING_8BIT);
	if (strincmp(Encoding, ENC_NAME_BINARY, strlen(ENC_NAME_BINARY)) == 0)
		return (ENCODING_BINARY);
	if (strincmp(Encoding, ENC_NAME_QUOTED, strlen(ENC_NAME_QUOTED)) == 0)
		return (ENCODING_QUOTED);
	if (strincmp(Encoding, ENC_NAME_BASE64, strlen(ENC_NAME_BASE64)) == 0)
		return (ENCODING_BASE64);
	if (strincmp(Encoding, "x-", 2) == 0) return (ENCODING_EXPERIMENTAL);
	return(ENCODING_ILLEGAL);
}

needs_mmdecode(s)
char *s;
{
	char buf[SLEN];
	char *t;
	int EncType;

	if (!s) return(1);
	while (*s && isspace(*s)) ++s;
	t = buf;
	while (*s && !isspace(*s) && ((t-buf) < (SLEN-1))) *t++ = *s++;
	*t = '\0';
	EncType = check_encoding(buf);
	if ((EncType == ENCODING_NONE) ||
	    (EncType == ENCODING_7BIT) ||
	    (EncType == ENCODING_8BIT) ||
	    (EncType == ENCODING_BINARY)) {
	    /* We don't need to go out to mmdecode, return 0 */
	    return(0);
	} else {
	    return(1);
	}
}

notplain(s)
char *s;
{
	char *t;
	if (!s) return(1);
	while (*s && isspace(*s)) ++s;
	if (istrcmp(s, "text\n") == 0) {
		/* old MIME spec, subtype now obligat, accept it as
		   "text/plain; charset=us-ascii" for compatibility
		   reason */
		return(0);
	}
	if (strincmp(s, "text/plain", 10)) return(1);
	t = (char *) index(s, ';');
	while (t) {
		++t;
		while (*t && isspace(*t)) ++t;
		if (!strincmp(t, "charset", 7)) {
			s = (char *) index(t, '=');
			if (s) {
				++s;
				while (*s && (isspace(*s) || *s == '\"')) ++s;
				if (!strincmp(s, display_charset, strlen(display_charset)))
					return(0);
				if (!strincmp(s, "us-ascii", 8)) {
					/* check if configured charset could
					   display us-ascii */
					if(charset_ok(display_charset)) return(0);
				}
			}
			return(1);
		}
		t = (char *) index(t, ';');
	}
	return(0); /* no charset, was text/plain */
}

int charset_ok(s)
char *s;
{
    /* Return true if configured charset could display us-ascii too */
    char buf[SLEN];	/* assumes sizeof(charset_compatlist) <= SLEN */
    char *bp, *chset;

    /* the "charset_compatlist[]" format is: */
    /*   charset charset charset ... */
    bp = strcpy(buf, charset_compatlist);
    while ((chset = strtok(bp, " \t\n")) != NULL) {
	bp = NULL;
	if (istrcmp(chset, s) == 0)
	    break;
    }

    /* see if we reached the end of the list without a match */
    if (chset == NULL) {
	return(FALSE);
    }
    return(TRUE);
}
#endif /* MIME */
