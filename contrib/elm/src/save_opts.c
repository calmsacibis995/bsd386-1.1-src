
static char rcsid[] = "@(#)Id: save_opts.c,v 5.7 1993/09/27 01:51:38 syd Exp ";

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
 * $Log: save_opts.c,v $
 * Revision 1.2  1994/01/14  00:55:39  cp
 * 2.4.23
 *
 * Revision 5.7  1993/09/27  01:51:38  syd
 * Add elm_chown to consolidate for Xenix not allowing -1
 * From: Syd
 *
 * Revision 5.6  1993/08/23  03:26:24  syd
 * Try setting group id separate from user id in chown to
 * allow restricted systems to change group id of file
 * From: Syd
 *
 * Revision 5.5  1993/08/10  18:54:45  syd
 * Elm was failing to write an empty "alternatives" list to elmrc.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.4  1993/08/03  19:03:52  syd
 * "*clear-weed-list*" in elmrc was wiped out when one saved the options in elm.
 * From: Jan.Djarv@sa.erisoft.se (Jan Djarv)
 *
 * Revision 5.3  1993/06/10  02:55:34  syd
 * Write options to elmrc even if their values are empty strings.
 * Rationalize code that reads and writes weedouts and alternates.
 * From: chip%fin@myrddin.sybus.com
 *
 * Revision 5.2  1993/04/12  03:10:54  syd
 * The onoff macro assumes a boolean option only has values 1 or 0.
 * This is not true for forms option (may be 2 == MAYBE).
 *
 * This is known bug EB51 BTW. I'm looking at the list now and there are some
 * bugs that I think are simple, and I'll try to fix some of them.
 * From: Jan Djarv <Jan.Djarv@sa.erisoft.se>
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** This file contains the routine needed to allow the users to change the
    Elm parameters and then save the configuration in a ".elm/elmrc" file in
    their home directory.  With any luck this will allow them never to have
    to actually EDIT the file!!

**/

#include "headers.h"
#include "s_elmrc.h"
#include <errno.h>

#undef onoff
#define   onoff(n)	(n == 0? "OFF":"ON")

#define absolute(x)		((x) < 0? -(x) : (x))

extern  int errno;
extern char version_buff[];

char *error_description(), *sort_name(), *alias_sort_name(), *level_name();
long  ftell();

#include "save_opts.h"

FILE *elminfo;		/* informational file as needed... */

save_options()
{
	/** Save the options currently specified to a file.  This is a
	    fairly complex routine since it tries to put in meaningful
	    comments and such as it goes along.  The comments are
	    extracted from the file ELMRC_INFO as defined in the sysdefs.h
	    file.  THAT file has the format;

		varname
		  <comment>
		  <comment>
		<blank line>

	    and each comment is written ABOVE the variable to be added.  This
	    program also tries to make 'pretty' stuff like the alternatives
	    and such.
	**/

	FILE *newelmrc; 
	char  oldfname[SLEN], newfname[SLEN];

	sprintf(newfname, "%s/%s", home, elmrcfile);
	sprintf(oldfname, "%s/%s", home, old_elmrcfile);

	/** first off, let's see if they already HAVE a .elm/elmrc file **/

	save_file_stats(newfname);
	if (access(newfname, ACCESS_EXISTS) != -1) {
	  /** YES!  Copy it to the file ".old.elmrc".. **/
	  if (rename(newfname, oldfname) < 0)
	    dprint(2, (debugfile, "Unable to rename %s to %s\n", 
		   newfname, oldfname));
	  (void) elm_chown(oldfname, userid, groupid);

	}

	/** now let's open the datafile if we can... **/

	if ((elminfo = fopen(ELMRC_INFO, "r")) == NULL) 
	  error1(catgets(elm_msg_cat, ElmrcSet, ElmrcSavingWithoutComments,
		  "Warning: saving without comments! Can't get to %s."), 
		  ELMRC_INFO);

	/** next, open the new .elm/elmrc file... **/

	if ((newelmrc = fopen(newfname, "w")) == NULL) {
	   error2(catgets(elm_msg_cat, ElmrcSet, ElmrcCantSaveConfig,
		   "Can't save configuration! Can't write to %s [%s]."),
		   newfname, error_description(errno));
	   return;
	}
	
	save_user_options(elminfo, newelmrc);
	restore_file_stats(newfname);

	error1(catgets(elm_msg_cat, ElmrcSet, ElmrcOptionsSavedIn,
		"Options saved in file %s."), newfname);
}

static char *str_opt(x, f)
register int x;
int f;
{
	static char buf[SLEN];
	register char *s, *t;

	s = strcpy(buf, "*ERROR*");
	switch(save_info[x].flags & DT_MASK) {
	    case DT_STR:
		if (save_info[x].flags & FL_NOSPC) {
		    for (t = SAVE_INFO_STR(x), s = buf; *t; ++t, ++s)
			if ((*s = *t) == SPACE) *s='_';
		    *s = '\0';
		    s = buf;
		} else
		    s = SAVE_INFO_STR(x);
		break;

	    case DT_CHR:
		sprintf(buf, "%c", *SAVE_INFO_CHR(x));
		s = buf;
		break;

	    case DT_BOL:
		s = onoff(*SAVE_INFO_BOL(x));
		break;

	    case DT_SRT:
		if (f == FULL) {
		    s = sort_name(SHORT);
		    break;
		}
		sprintf(buf, "%d", *SAVE_INFO_NUM(x));
		s = buf;
		break;

	    case DT_ASR:
		if (f == FULL) {
		    s = alias_sort_name(SHORT);
		    break;
		}
		sprintf(buf, "%d", *SAVE_INFO_NUM(x));
		s = buf;
		break;

	    case DT_NUM:
		if (f == FULL) {
		    if (equal(save_info[x].name, "userlevel")) {
			s = level_name(*SAVE_INFO_NUM(x));
			break;
		    }
		}
		sprintf(buf, "%d", *SAVE_INFO_NUM(x));
		s = buf;
		break;

	    default:
		break;
	}
	return(s);
}

find_opt(s)
char *s;
{
	register int x, y;

	for (x = 0; x < NUMBER_OF_SAVEABLE_OPTIONS; x++) {
	    y = strcmp(s, save_info[x].name);
	    if (y <= 0)
		break;
	}

	if (y != 0)
	    return(-1);
	return(x);
}

char *str_opt_nam(s, f)
char *s;
int f;
{
	char *t = NULL;
	int x;

	x = find_opt(s);
	if (x >= 0)
	    t = str_opt(x, f);
	return(t);
}


save_user_options(elminfo_fd, newelmrc)
FILE *elminfo_fd, *newelmrc;
{
	register int x, local_value;
	register char *s;

	/** save the information in the file.  If elminfo_fd == NULL don't look
	    for comments!
	**/

	if (elminfo_fd != NULL) 
	  build_offset_table(elminfo_fd);

	fprintf(newelmrc, catgets(elm_msg_cat, ElmrcSet, ElmrcOptionsFile,
	      "#\n# .elm/elmrc - options file for the ELM mail system\n#\n"));

	if (strlen(full_username) > 0)
	  MCfprintf(newelmrc, catgets(elm_msg_cat, ElmrcSet, ElmrcSavedAutoFor,
		  "# Saved automatically by ELM %s for %s\n#\n\n"),
		  version_buff, full_username);
	else
	  fprintf(newelmrc, catgets(elm_msg_cat, ElmrcSet, ElmrcSavedAuto,
		"# Saved automatically by ELM %s\n#\n\n"), version_buff);

	fprintf(newelmrc, catgets(elm_msg_cat, ElmrcSet, ElmrcYesNoMeans,
		"# For yes/no settings with ?, ON means yes, OFF means no\n"));

	for (x = 0; x < NUMBER_OF_SAVEABLE_OPTIONS; x++) {
	    char buf[SLEN];

	    /** skip system-only options **/
	    if (save_info[x].flags & FL_SYS)
		continue;

	    local_value = save_info[x].flags & FL_LOCAL;
	    s = NULL;
	    switch(save_info[x].flags & DT_MASK) {
		case DT_MLT:
		case DT_SYN:
		    break;

		case DT_ASR:
		case DT_SRT:
		    s = str_opt(x, FULL);
		    break;

		case DT_STR:
		case DT_CHR:
		case DT_BOL:
		case DT_NUM:
		    s = str_opt(x, SHORT);
		    break;

		case DT_WEE:
		    {	int len, i;
			char *w;

			add_comment(x, newelmrc);
			if (!local_value)
			    fprintf(newelmrc, "### ");
			fprintf(newelmrc, "%s =", save_info[x].name);
			len = strlen(save_info[x].name) + 6;

			i = 0;
			while (i < weedcount
			       && istrcmp(weedlist[i], "*end-of-defaults*"))
			    i++;
			while (i < weedcount
			       && !istrcmp(weedlist[i], "*end-of-defaults*"))
			    i++;
			if (i == 1) {
			    /* end-of-defaults in the first position means
			    ** that there are no defaults, i.e.
			    ** a clear-weed-list has been done.
			    */
			    fprintf(newelmrc, " \"*clear-weed-list*\"");
			    len += 20;
			}

			while (i <= weedcount) {
			    char *w;

			    w = (i < weedcount) ? weedlist[i]
				: "*end-of-user-headers*";
			    if (strlen(w) + len > 72) {
				if (local_value)
				    fprintf(newelmrc, "\n\t");
				else
				    fprintf(newelmrc, "\n###\t");
				len = 8;
			    }
			    else {
				fprintf(newelmrc, " ");
				++len;
			    }
			    fprintf(newelmrc, "\"%s\"", w);
			    len += strlen(w) + 3;
			    i++;
			}
			fprintf(newelmrc, "\n");
		    }
		    break;

		case DT_ALT:
		    {   struct addr_rec *alts;
			int len=0;

			alts = *SAVE_INFO_ALT(x);
			add_comment(x, newelmrc);
			if (!local_value)
			    fprintf(newelmrc, "### ");
			fprintf(newelmrc, "%s =", save_info[x].name);
			len = strlen(save_info[x].name) + 6;
			for ( ;alts; alts = alts->next) {
			    if (strlen(alts->address) + len > 72) {
				if (local_value)
				    fprintf(newelmrc, "\n\t");
				else
				    fprintf(newelmrc, "\n###\t");
				len = 8;
			    }
			    else {
				fprintf(newelmrc, " ");
				++len;
			    }
			    fprintf(newelmrc, "%s", alts->address);
			    len += strlen(alts->address);
			}
			fprintf(newelmrc,"\n");
		    }
		    break;
		}

	    if (s) {
		add_comment(x, newelmrc);
		if (!local_value)
		    fprintf(newelmrc, "### ");
		fprintf(newelmrc, "%s = %s\n", save_info[x].name, s);
	    }
	}

	fclose(newelmrc);
	if ( elminfo_fd != NULL ) {
	    fclose(elminfo_fd);
	}
}

add_comment(iindex, fd)
int iindex;
FILE *fd;
{	
	/** get to and add the comment to the file **/
	char buffer[SLEN];

	/** always put a blank line between options */
	fputc('\n', fd);

	/** add the comment from the comment file, if available **/
	if (save_info[iindex].offset > 0L) {
	  if (fseek(elminfo, save_info[iindex].offset, 0) == -1) {
	    dprint(1,(debugfile,
		   "** error %s seeking to %ld in elm-info file!\n",
		   error_description(errno), save_info[iindex].offset));
	  }
	  else {
	    while (fgets(buffer, SLEN, elminfo) != NULL) {
	      if (buffer[0] != '#') 
		break;
	      fputs(buffer, fd);
	    }
	  }
	}
}

build_offset_table(elminfo_fd)
FILE *elminfo_fd;
{
	/** read in the info file and build the table of offsets.
	    This is a rather laborious puppy, but at least we can
	    do a binary search through the array for each element and
	    then we have it all at once!
	**/

	char line_buffer[SLEN];
	
	while (fgets(line_buffer, SLEN, elminfo_fd) != NULL) {
	  if (strlen(line_buffer) > 1)
	    if (line_buffer[0] != '#' && !whitespace(line_buffer[0])) {
	       no_ret(line_buffer);
	       if (find_and_store_loc(line_buffer, ftell(elminfo_fd))) {
	         dprint(1, (debugfile,"** Couldn't find and store \"%s\" **\n", 
			 line_buffer));
	       }
	    }
	}
}

find_and_store_loc(name, offset)
char *name;
long  offset;
{
	/** given the name and offset, find it in the table and store it **/

	int first = 0, last, middle, compare;

	last = NUMBER_OF_SAVEABLE_OPTIONS;

	while (first <= last) {

	  middle = (first+last) / 2;

	  if ((compare = strcmp(name, save_info[middle].name)) < 0) /* a < b */
	    last = middle - 1;
	  else if (compare == 0) {				    /* a = b */
	    save_info[middle].offset = offset;
	    return(0);
	  }
	  else  /* greater */					    /* a > b */
	    first = middle + 1; 
	}

	return(-1);
}
