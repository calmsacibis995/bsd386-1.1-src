
/*
 *   $Id: print.c,v 1.1.1.1 1993/02/10 18:05:35 sanders Exp $
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
 *
 */

#include <stdio.h>
#include <string.h>

#include "print.h"
#include "postscri.h"

/********************************
  defines
 ********************************/
#ifndef True
#define True	1
#define False	0
#endif

#define	PAGEBREAK	('L' - 0x40)

/********************************
  imports
 ********************************/

#include "main.h"
#include "postscri.h"

/********************************
  exports
 ********************************/


/********************************
  globals
 ********************************/


/********************************
  print_file
 ********************************/

void print_file (input, output, filename, line_numbers)

FILE *input;
FILE *output;
char *filename;
int  line_numbers;

{
  char line[8192+1];
  int touched = False;
  char *p;
  long line_num;
  char *buffer;
  int  bufflen;

  buffer  = line;
  bufflen = 8192;

  if (line_numbers) {
    buffer  += 8;
    bufflen -= 8;
    sprintf (line, "%7lu:", line_num = 1);
  }

  StartDocument (output, filename);

  while (fgets (buffer, bufflen, input) != NULL) {

    /* remove the trailing newline from the line */
    buffer [strlen(buffer)-1] = 0;

    /* if the line is a page break, then handle it */
    for (p = buffer; *p == ' ' || *p == '\t'; p++)
      ;
    if (*p == PAGEBREAK) {
      if (touched)
        EndColumn (output);
    } else {
      WriteLine (output, line);
      touched = True;
      line_num++;
    }
    if (line_numbers)
      sprintf (line, "%7lu:", line_num);
  }

  EndDocument (output);
}
