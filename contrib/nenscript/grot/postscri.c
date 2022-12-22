/*
 *   $Id: postscri.c,v 1.1.1.1 1993/02/10 18:05:37 sanders Exp $
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
#include <time.h>

#ifdef MSDOS
#include <ctype.h>
#include <stdlib.h>
#else
#include <pwd.h>
#endif

#include "defs.h"
#include "postscri.h"
#include "fontwidt.h"
#include "font_lis.h"

/********************************
  defines
 ********************************/
#ifndef True
#define True	1
#define False	0
#endif

#ifndef MIN
#define MIN(a,b) ((a)<=(b)?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b) ((a)>=(b)?(a):(b))
#endif

#define	TAB 	0x9

/********************************
  imports
 ********************************/

#include "main.h"

/********************************
  exports
 ********************************/


/********************************
  globals
 ********************************/

static int  touched_page = False;
static char *CurrentFilename;
static int  WrapFlag;
static int  GaudyFlag;

static long Columns;			/* number of columns */
static long X;
static long Y;
static long BFH;				/* body font height */
static long BFW;				/* body font width */
static long TFH;				/* title font height */
static long PageNum;			/* page number within each documenr printed */
static long PageCount;			/* total number of pages printed */
static int TitleEnabled;		/* True if title to be displayed */

static long PW;				/* page width transformed for landscape mode */
static long PH;				/* page height transformed for landscape mode */
static long LM;				/* left margin */
static long RM;				/* right margin */
static long TM;				/* top */
static long BM;				/* bottom margin */
static long WW;				/* width of current print window */
static long CPL;				/* characters per line */
static long CC;				/* current column */
static long LineNumber;			/* number of line on page (1 top n) */
static long LinesPerPage;		/* number of lines per page, or 0 if to use default */
static char *Classification;		/* points to string to use for classification */

static long ClassY;			/* Y location to print classification at */
static long TitleY;			/* Y location to print title at */
static long StartY;			/* Y location to print text from */
static long EndY;			/* Y location to print text down to */
static long ClassBottomY;		/* Y location to print bottom classification at */

static char *BodyFont;
static char *TitleFont;
static char *GaudyPNFont	= GAUDYPNFONT;
static char *GaudyTitleFont	= GAUDYTITLEFONT;
static char *GaudyDateFont	= GAUDYDATEFONT;
static char *ClassFont          = CLASSIFICATIONFONT;

static long  GaudyPNFontSize;
static long  GaudyTitleFontSize;
static long  GaudyDateFontSize;
static long  ClassFontSize;

/*
 * forward declarations if in ANSI mode
 */

#ifdef __STDC__
void        PrintPSString   (FILE *, char *, long);
void        EndPage         (FILE *);
void        StartPage       (FILE *);
void        PrintLine       (FILE *, char *, long, int);
static long ExtractFontSize (char *);

char *font;

#endif


/********************************
  PrintPSString
    Print a string of ASCII characters of length "len" as a
    PostScript string, i.e. enclosed in brackets and with appropriate
    escape characters.
 ********************************/

void PrintPSString (stream, line, len)

FILE *stream;
char *line;
long len;

{
  register int i;
  register char * str = line;

  fprintf (stream, "(");
  for (i = 0; i < len ; i++)
    fprintf (stream, "%s%c", str[i] == ')'  ||
                             str[i] == '('  ||
			     str[i] == '\\' ? "\\" : "", str[i]);
  fprintf (stream, ")");
}

/********************************
  ExtractFontSize
    Extracts the fontsize from a font type description like "Courier10". Note that
    it also mangles the font definition so that the font size disappears
 ********************************/

static long ExtractFontSize (font)

char *font;

{
  char *p;
  long  s;

  p = &font[strlen(font)-1];

  while (p > font && isdigit (*p))
    p--;

  s = atol (++p);

  *p = 0;

  return s * SCALE;
}




/********************************
  StartPage
    Called to start a new page
 ********************************/

void StartPage (stream)

FILE *stream;

{
  /* insert a page marker as per the Structuring conventions */
  fprintf (stream, "%%%%Page: %li %li\n", PageNum, PageCount);

  /* call the StartPage procedure with the appropriate arguments */
  fprintf (stream, "(%li) ", PageNum);
  PrintPSString (stream, CurrentFilename, strlen (CurrentFilename));
  fprintf (stream, " StartPage\n", PageNum);

  /* set X and Y location */
  X = LM;
  Y = StartY;

  /* set the column back to column 0 */
  CC = 0;

  /* set the line number to line # 1 */
  LineNumber = 1;

  /* we have now touched the page */
  touched_page = True;
}


/********************************
  EndPage
    Called to complete a page
 ********************************/

void EndPage (stream)

FILE *stream;

{
  /* call the end page procedure */
  fprintf (stream, "EndPage\n");

  /* increment both the job and document page counters */
  PageNum++;
  PageCount++;

  /* now we haven't touched the page */
  touched_page = False;
}


/*******************************

  EndColumn

 *******************************/

void EndColumn (stream)

FILE *stream;

{
  LineNumber = 1;
  CC++;
  if (CC == Columns)
    EndPage (stream);
  else {
    X += (COLUMN_SEP * SCALE) + WW;
    Y = StartY;
  }
}


/********************************
  PrintLine
    Called to print a line which has been chopped to
    the correct length already. The "first" flag indicates
    whether the line is a normal line or a continuation line.
    This routine performs the skipping for indents.
 ********************************/

void PrintLine (stream, line, count, first)

FILE *stream;
char *line;
long  count;
int  first;

{
  char *p;
  int i;

  if (!touched_page)
    StartPage (stream);

  /* make i point to the first non-blank character on the line */
  for (i = 0; i < count && line[i] == ' ';i++)
    ;

  if (i < count) {
    if (!first)
      fprintf (stream, "%li %li K ", Y, X);
    PrintPSString (stream, &line[i], count - i);
    if (i > 0)
      fprintf (stream, " %li %li %i L\n", Y, X, i);
    else
      fprintf (stream, " %li %li T\n", Y, X);
  }

  Y -= BFH * 11 / 10;		/* put a little bit of extra spacing between the characters */
  LineNumber++;

  if (Y < EndY || (LinesPerPage > 0 && LineNumber > LinesPerPage))
    EndColumn (stream);
}

/********************************
  WriteLine
    Writes a line to the output. This routine performs the wrapping and tab expansion
 ********************************/

void WriteLine (stream, line)

FILE *stream;
char *line;

{
  int l;
  int first = True;
  char *p;
  char *q;
  int col;
  int i;

  char full_line[8192];

  /* expand tabs if we have to */
  if (strchr (line, TAB) != NULL) {
    col = 0;
    q = full_line;
    for (p = line; *p ; p++)
      if (*p != TAB) {
        *q++ = *p;
        col = (col + 1) & 0x7;
      } else {
        for (i = 8 - col; i > 0; i--)
          *q++ = ' ';
        col = 0;
      }
    *q = 0;
    line = full_line;
  }

  if (WrapFlag) {
    while (strlen(line) > CPL) {
      PrintLine (stream, line, CPL, first);
      first = False;
      line += CPL;
    }
    PrintLine (stream, line, strlen(line), first);
  } else
    PrintLine (stream, line, MIN(strlen(line), CPL), True);
}


/********************************
  StartDocument
    Called when a new document is to be printed. Not to be confused with
    starting the job.
 ********************************/

void StartDocument (stream, filename)

FILE *stream;
char *filename;

{
  /* restart the internal page number */
  PageNum = 1;

  /* indicate that the first page has not yet been touched */
  touched_page = False;

  /* and set the current filename */
  CurrentFilename = filename;
}


/********************************
  EndDocument
    Called when a document has been completed.
 ********************************/

void EndDocument (stream)

FILE *stream;

{
  /* if we have drawn a partial page, finish it off properly */
  if (touched_page)
    EndPage (stream);
}

/********************************
  StartJob
    Called when a new job is to be started. This performs all of the
    PostScript initialisation
 ********************************/

void StartJob (stream, filename, landscape, columns, bodyfont,
               titlefont, wrap, enabletitle, title, copies, gaudy, force_lines, classification)


FILE * stream;
int  landscape;
int  columns;
char *bodyfont;
char *titlefont;
int  wrap;
int  enabletitle;
char *title;
int  copies;
int  gaudy;
int  force_lines;
char *filename;
char *classification;

{
#ifndef MSDOS
  struct passwd * pwdent;
#endif
  time_t thetime = time(NULL);
  char  * timestring = strtok(ctime (&thetime), "\n");  /* a simple time string */
  struct tm *tm      = localtime (&thetime);
  char tm_string[15];
  char dt_string[15];

  /* get the time and date strings */
#ifdef MSDOS
  strftime (tm_string, 15, "%X", tm);

#ifdef US_DATE
   strftime (dt_string, 15, "%b %d %y", tm);
#else
   strftime (dt_string, 15, "%d %b %y", tm);
#endif

#else
  strftime (tm_string, 15, "%I:%M %p", tm);

#ifdef US_DATE
  strftime (dt_string, 15, "%h %d %y", tm);
#else
  strftime (dt_string, 15, "%d %h %y", tm);
#endif

#endif

  /* make local copies of various flags */
  WrapFlag = wrap;
  GaudyFlag = gaudy && enabletitle;
  TitleEnabled = enabletitle;
  Columns = columns;
  LinesPerPage = force_lines;
  Classification = classification;


  /* get the size and width of the font used for the body of the text */
  BFH = ExtractFontSize (BodyFont = bodyfont);
  BFW = GetFontWidth (BodyFont, BFH);

  GaudyPNFontSize    = ExtractFontSize (GaudyPNFont);
  GaudyTitleFontSize = ExtractFontSize (GaudyTitleFont);
  GaudyDateFontSize  = ExtractFontSize (GaudyDateFont);

  /* get the size of the font used for the title */
  TFH = ExtractFontSize (TitleFont = titlefont);

  /* now the classification font, if there is one */
  if (Classification != NULL)
    ClassFontSize = ExtractFontSize (ClassFont);

  /* output obligatory Postscript header */
  fprintf (stream, "%%!PS-Adobe-1.0\n");

  /* output the filename of the first file as the title */
  fprintf (stream, "%%%%Title: %s\n", filename);

  /* put version of program in as creator */
  fprintf (stream, "%%%%Creator: %s\n", version_string);

  /* extract the users full name and put in as the creator */
#ifdef MSDOS
  fprintf (stream, "%%%%For: %s\n", getenv ("LOGNAME") != NULL ? getenv ("LOGNAME") :
                                    getenv ("USER") != NULL    ? getenv ("USER") : "Unknown");
#else
  pwdent = getpwuid (getuid());
  fprintf (stream, "%%%%For: %s\n", pwdent->pw_name);
#endif

  /* put in the time as a string */
  fprintf (stream, "%%%%CreationDate: %s\n", timestring);

  /* output the font list */
  add_font_to_list (TitleFont);
  add_font_to_list (BodyFont);
  if (GaudyFlag) {
    add_font_to_list (GaudyPNFont);
    add_font_to_list (GaudyDateFont);
    add_font_to_list (GaudyTitleFont);
  }
  if (Classification != NULL)
    add_font_to_list (ClassFont);

  fprintf (stream, "%%%%DocumentFonts: ");
  enumerate_fonts (stream);
  fprintf (stream, "\n");

  /* initialise the document page counter and indicate that the page count info
     can be found at the end of the document. This copout means we don't have
     to store the output data in a temporary file (like enscript) just so we can
     find out how many pages there are */
  PageCount = 1;
  fprintf (stream, "%%%%Pages: (atend)\n");

  /* End of header marker */
  fprintf (stream, "%%%%EndComments\n");

  /* scale the coordinate system by SCALE so we can use integer arithmetic
     without losing accuracy */
  fprintf (stream, "1 %li div dup scale\n", SCALE);

  /* use the Postscript mechanism for duplicating pages, rather than using a flag to lpr.
     This mechanism means less data (I think!!) but it requires you to edit the file
     if you want to print a different number of copies later. It is so rarely used that
     it isn't really an issue anyway!! */
  fprintf (stream, "/#copies %i def\n", copies);

  /* rotate the page if using landscape mode, and set PW and PH to the page width and height
     as determined by the SCALE factor */
  if (landscape) {
    PW = PAGE_HEIGHT * SCALE;
    PH = PAGE_WIDTH * SCALE;

    LM = PAGE_BOT_MARGIN * SCALE;
    RM = PAGE_TOP_MARGIN * SCALE;

    TM = PAGE_LEFT_MARGIN * SCALE;
    BM = PAGE_RIGHT_MARGIN * SCALE;

    fprintf (stream, "90 rotate %li %li translate\n",PAGE_LANDSCAPE_XOFFS, PAGE_LANDSCAPE_YOFFS - PH);
  } else {
    PH = PAGE_HEIGHT * SCALE;
    PW = PAGE_WIDTH * SCALE;

    LM = PAGE_LEFT_MARGIN * SCALE;
    RM = PAGE_RIGHT_MARGIN * SCALE;

    TM = PAGE_TOP_MARGIN * SCALE;
    BM = PAGE_BOT_MARGIN * SCALE;
  }

   /* calculate the width of the "window" in which we draw text, and from that calculate the
     number of characters per line (CPL) */
  WW = (PW - (LM + RM) - ((columns - 1) * COLUMN_SEP * SCALE)) / columns;
  CPL = WW / BFW;

  /* calculate the top position at which we start drawing text */
  StartY = PH - TM;

  /* adjust starting position if we are printing security classification */
  if (Classification != NULL) {
    StartY -= ClassFontSize;
    ClassY = StartY;
  }

  /* adjust starting position if we are printing title */
  if (TitleEnabled) {
    if (GaudyFlag)  {
      TitleY = StartY;
      StartY -= BH + BFH;
    } else {
      StartY -= TFH;
      TitleY = StartY;
      StartY -= BFH;
    }
  }

  /* move down one line below for start of text */
  StartY -= BFH;

  /* calculate the last location on the page to be printing text */
  EndY = BM;
  if (Classification != NULL) {
    ClassBottomY = BM;
    EndY += ClassFontSize;
  }

  /* define a variable for our body font, and calculate the character width for later use */
  fprintf (stream, "/BodyF /%s findfont %li scalefont def\n", BodyFont, BFH);
  fprintf (stream, "/CW BodyF setfont ( ) stringwidth pop def\n");

  /* define variables for various other font used - title, gaudy page number, gaudy date, gaudy title */
  fprintf (stream, "/Titlef  /%s findfont %li scalefont def\n", TitleFont, TFH);
  if (GaudyFlag) {
    fprintf (stream, "/Gpnf    /%s findfont %li scalefont def\n", GaudyPNFont,    GaudyPNFontSize);
    fprintf (stream, "/Gdatef  /%s findfont %li scalefont def\n", GaudyDateFont,  GaudyDateFontSize);
    fprintf (stream, "/Gtitlef /%s findfont %li scalefont def\n", GaudyTitleFont, GaudyTitleFontSize);
  }

  /* define procedures for drawing continuation line markers, continuation lines, normal lines, and performing indents */
  fprintf (stream, "/K         { -2 CW mul add exch moveto (+) show } def\n");
  fprintf (stream, "/L         { CW mul add exch moveto show } def\n");
  fprintf (stream, "/T         { exch moveto show } def\n");
  fprintf (stream, "/M         { CW mul 0 rmoveto } def\n");
  fprintf (stream, "/Centre    { dup stringwidth pop 2 div neg 0 rmoveto } def\n");

  /* define procedures for drawing gaudy page numbers, gaudy boxes, gaudy bars and gaudy titles */
  if (GaudyFlag) {
    fprintf (stream, "/Gb        { newpath moveto %li 0 rlineto 0 %li rlineto -%li 0 rlineto fill } def\n", BW, BH, BW);
    fprintf (stream, "/Gr        { newpath moveto %li 0 rlineto 0 %li rlineto -%li 0 rlineto fill } def\n",
                                       PW - (LM + RM) - (2 * BW), BS,
                                       PW - (LM + RM) - (2 * BW));
    fprintf (stream, "/G         { %s setgray %li %li Gb %li %li Gb %s setgray %li %li Gr } def\n",
					BOXGRAY,
					LM, TitleY - BH,                          /* pos of left box */
				  	PW - RM - BW, TitleY - BH,                /* pos of right box */
					BARGRAY,
					LM + BW, TitleY - BH);			   /* pos of bar */
  }

  /* define stuff for security strings */
  if (Classification != NULL) {
    fprintf (stream, "/Classf  /%s findfont %li scalefont def\n", ClassFont, ClassFontSize);
    fprintf (stream, "/ClassString ");
    PrintPSString (stream, Classification, strlen(classification));
    fprintf (stream, " def\n");
  }

  /* if we have titles enabled, define an appropriate procedure for drawing it */
  /* define the start page procedure used to start every page */
  fprintf (stream, "/StartPage { /SavedPage save def\n");
  if (Classification != NULL)
      fprintf (stream, "  Classf setfont %li %li moveto ClassString Centre 0 setgray show\n", PW / 2, ClassY);

  if (TitleEnabled) {
    if (GaudyFlag) {
      fprintf (stream, "  G\n");                                                   /* draw boxes */
      fprintf (stream, "  Gtitlef setfont %li %li moveto Centre 0 setgray show\n",   /* title */
                                         ((LM + BW) + (PW - RM - BW)) / 2L,
                                         TitleY - BH + ((BS * 7L / 10L) / 2L));
      if (title != NULL) {
        fprintf (stream, "%li %li moveto ",
                                         ((LM + BW) + (PW - RM - BW)) / 2L,
                                         TitleY - ((BS * 7L / 10L) / 2L));
        PrintPSString (stream, title, strlen(title));
        fprintf (stream, " Centre show\n");
      }
      fprintf (stream, "  Gpnf    setfont %li %li moveto Centre 1 setgray show\n",   /* page number */
                                         PW - RM - (BW / 2L),
                                         TitleY - (BH / 2L) - GaudyPNFontSize * 7L / 20L);
      fprintf (stream, "  Gdatef  setfont %li %li moveto (%s) Centre 0 setgray show\n",
					 LM + (BW / 2L), TitleY - (BH * 3L / 5L) - GaudyDateFontSize * 7L / 10L, tm_string);
      fprintf (stream, "                  %li %li moveto (%s) Centre show\n",
					 LM + (BW / 2L), TitleY - (BH * 3L / 5L) + GaudyDateFontSize * 7L / 10L, dt_string);
    } else {
      fprintf (stream, "  0 setgray Titlef setfont %li %li moveto ", LM, TitleY);
      if (title != NULL) {
        fprintf (stream, "pop pop ");
        PrintPSString (stream, title, strlen(title));
        fprintf (stream, " show\n");
      } else {
        fprintf (stream, "show 8 M ");
        PrintPSString (stream, timestring, strlen (timestring));
        fprintf (stream, " show 8 M show\n");
      }
    }
  }
  fprintf (stream, "  BodyF setfont 0 setgray } def\n");

  /* define end page procedure */
  fprintf (stream, "/EndPage   {");
  if (GaudyFlag && Columns == 2)
    fprintf (stream, " %li %li moveto %li -%li rlineto stroke ", LM + WW + (COLUMN_SEP * SCALE / 2), StartY, 0L, StartY - EndY);
  if (Classification != NULL)
      fprintf (stream, "  Classf setfont %li %li moveto ClassString Centre 0 setgray show\n", PW / 2, ClassBottomY);
  fprintf (stream, "showpage SavedPage restore } def\n");

  /* end of the header */
  fprintf (stream, "%%%%EndProlog\n");
}

/********************************
   EndJob
 ********************************/

void EndJob (stream)

FILE *stream;

{
  /* indicate this is the end of the file */
  fprintf (stream, "%%%%Trailer\n");

  /* now we know how many pages there are!! */
  fprintf (stream, "%%%%Pages: %li\n", PageCount - 1);
}
