/*
 *   $Id: font_lis.c,v 1.2 1993/02/18 15:49:13 polk Exp $
 *
 *   This code was written by Craig Southeren whilst under contract
 *   to Computer Sciences of Australia, Systems Engineering Division.
 *   It has been kindly released by CSA into the public domain.
 *
 *   Neither CSA or me guarantee that this source code is fit for anything,
 *   so use it at your peril. I don't even work for CSA any more, so
 *   don't bother them about it. If you have any suggestions or comments
 *   (or money, cheques, free trips =8^) !!!!! ) please contact me
 *   care of geoffw@extro.ucc.oz.au
 */

#include <stdio.h>
#include <string.h>
#ifndef __bsdi__
#include <malloc.h>
#endif
#include <string.h>

#include "font_lis.h"

/********************************
  defines
 ********************************/
#ifndef True
#define True	1
#define False	0
#endif

#define	strdup(s)	(strcpy ((char *)malloc (strlen(s)+1), s))


struct font_name_struct {
  char 			  * name;
  struct font_name_struct * next;
};

struct font_name_struct * font_list = NULL;

/********************************
  imports
 ********************************/
extern char * progname;


/********************************
  exports
 ********************************/


/********************************
  globals
 ********************************/

/********************************
 enumerate_fonts
 ********************************/

void enumerate_fonts (stream)

FILE *stream;

{
  struct font_name_struct * p;

  for (p = font_list;p != NULL; p = p->next)
    fprintf (stream, "%s ", p->name);
}

/********************************
 add_font_to_list
 ********************************/

void add_font_to_list (fontname)

char *fontname;

{
  struct font_name_struct * p;

  /* make sure there is no font with this name */
  for (p = font_list;p != NULL; p = p->next)
    if (strcmp (fontname, p->name) == 0)
      return;

  /* insert new font record at the start of the list */
  p = (struct font_name_struct *)malloc (sizeof (struct font_name_struct));
  p->name = strdup (fontname);
  p->next = font_list;
  font_list = p;
}
