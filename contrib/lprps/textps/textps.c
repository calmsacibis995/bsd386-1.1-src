/* textps.c */

#ifndef lint
static char rcsid[] = "$Id: textps.c,v 1.2 1994/01/13 17:53:48 sanders Exp $";
#endif

#ifndef TAB_WIDTH
#define TAB_WIDTH 8
#endif /* not TAB_WIDTH */

#ifndef LINES_PER_PAGE
#define LINES_PER_PAGE 66
#endif /* not LINES_PER_PAGE */

#ifndef PAGE_LENGTH
#ifdef A4
#define PAGE_LENGTH 842.0
#else /* not A4 */
#define PAGE_LENGTH 792.0
#endif /* not A4 */
#endif /* not PAGE_LENGTH */

#ifndef BASELINE_OFFSET
#define BASELINE_OFFSET 8.0
#endif /* not BASELINE_OFFSET */

#ifndef VERTICAL_SPACING
#define VERTICAL_SPACING 12.0
#endif /* not VERTICAL_SPACING */

#ifndef LEFT_MARGIN
#define LEFT_MARGIN 18.0
#endif /* not LEFT_MARGIN */

#ifndef FONT
#define FONT "Courier"
#endif /* not FONT */

#ifndef BOLD_FONT
#define BOLD_FONT "Courier-Bold"
#endif /* not BOLD_FONT */

#include <stdio.h>
#include <ctype.h>
#include <string.h>

extern char *malloc();
extern char *optarg;
extern int optind;

typedef struct output_char {
  struct output_char *next;
  char c;
  char is_bold;
  int pos;
} output_char;

output_char *free_output_char_list = 0;

int tab_width = TAB_WIDTH;
int lines_per_page = LINES_PER_PAGE;
double page_length = PAGE_LENGTH; /* in points */
/* distance in points from top of page to first baseline */
double baseline_offset = BASELINE_OFFSET;
double vertical_spacing = VERTICAL_SPACING; /* in points */
double left_margin = LEFT_MARGIN; /* in points */
char *font = FONT;
char *bold_font = BOLD_FONT;

double char_width;

int pageno = 0;

enum { NONE, ROMAN, BOLD } current_font;

void do_file();
void prologue();
void trailer();
char *prog;

void usage()
{
  fprintf(stderr,
	  "usage: %s [-c n] [-l n] [-m n] [-t n] [-v n] [files ...]\n",
	  prog);
  exit(1);
}

main(argc, argv)
int argc;
char **argv;
{
  int bad_files = 0;
  double cpi = 12.0;			/* characters per inch */
  int opt;

  prog = argv[0];

  while ((opt = getopt(argc, argv, "c:l:m:t:v:")) != EOF)
    switch (opt) {
    case 'c':
      if (sscanf(optarg, "%lf", &cpi) != 1)
	usage();
      break;
    case 'l':
      if (sscanf(optarg, "%d", &lines_per_page) != 1)
	usage();
      break;
    case 'm':
      if (sscanf(optarg, "%lf", &left_margin) != 1)
	usage();
      break;
    case 't':
      if (sscanf(optarg, "%lf", &baseline_offset) != 1)
	usage();
      break;
    case 'v':
      if (sscanf(optarg, "%lf", &vertical_spacing) != 1)
	usage();
      break;
    case '?':
      usage();
    default:
      abort();
    }

  char_width = 72.0/cpi;
  prologue();
  if (optind >= argc)
    do_file(stdin);
  else {
    int i;
    for (i = optind; i < argc; i++)
      if (strcmp(argv[i], "-") != 0
	  && freopen(argv[i], "r", stdin) == NULL) {
	perror(argv[i]);
	bad_files++;
      }
      else
	do_file();
  }
  trailer();
  exit(bad_files);
}


output_char *new_output_char()
{
  if (free_output_char_list) {
    output_char *tem = free_output_char_list;
    free_output_char_list = free_output_char_list->next;
    return tem;
  }
  else {
    output_char *tem;
    if ((tem = (output_char *)malloc(sizeof(output_char))) == NULL) {
      fprintf(stderr, "%s: out of memory\n", prog);
      exit(1);
    }
    return tem;
  }
}

void delete_output_char(p)
output_char *p;
{
  p->next = free_output_char_list;
  free_output_char_list = p;
}

void pschar(c)
int c;
{
  if (!isascii(c) || iscntrl(c))
    printf("\\%03o", c & 0377);
  else if (c == '(' || c == ')' || c == '\\') {
    putchar('\\');
    putchar(c);
  }
  else
    putchar(c);
}

void psnum(f)
double f;
{
  static char buf[100];
  char *p;
  sprintf(buf, "%f", f);
  for (p = buf + strlen(buf) - 1; *p != '.';  --p)
    if (*p != '0') {
      p++;
      break;
    }
  *p = '\0';
  fputs(buf, stdout);
}

/* output_line is ordered greatest position first */

void print_line(output_line, vpos)
output_char *output_line;
int vpos;
{
  output_char *rev = output_line;
  output_line = 0;
  while (rev != 0) {
    output_char *tem = rev;
    rev = rev->next;
    tem->next = output_line;
    output_line = tem;
  }
  while (output_line != NULL) {
    output_char *tem;
    output_char **p = &output_line;
    int start_pos = output_line->pos;
    int is_bold = output_line->is_bold;
    int pos;
    if (is_bold) {
      if (current_font != BOLD) {
	printf("B");
	current_font = BOLD;
      }
    }
    else {
      if (current_font != ROMAN) {
	printf("R");
	current_font = ROMAN;
      }
    }
    putchar('(');
    pschar(output_line->c);
    pos = output_line->pos + 1;
    tem = output_line;
    output_line = output_line->next;
    delete_output_char(tem);
    for (;;) {
      while (*p != NULL
	     && ((*p)->pos < pos || (*p)->is_bold != is_bold))
	p = &(*p)->next;
      if (*p == NULL)
	break;
      while (pos < (*p)->pos) {
	pschar(' ');
	pos++;
      }
      pschar((*p)->c);
      pos++;
      tem = *p;
      *p = tem->next;
      delete_output_char(tem);
    }
    putchar(')');
    psnum(left_margin + start_pos*char_width);
    putchar(' ');
    psnum(page_length - baseline_offset - vpos*vertical_spacing);
    fputs(" L\n", stdout);
  }
}

void page_start()
{
  printf("%%%%Page: ? %d\n%%%%BeginPageSetup\nPS\n%%%%EndPageSetup\n",
	 ++pageno);
  current_font = NONE;
}

void page_end()
{
  printf("PE\n");
}

char *defs[] = {
  "/L { moveto show } bind def",
  "/PS { /level0 save def } bind def",
  "/PE { level0 restore showpage } bind def",
  "/RE {",
  "\tfindfont",
  "\tdup maxlength dict begin",
  "\t{",
  "\t\t1 index /FID ne { def } { pop pop } ifelse",
  "\t} forall",
  "\t/Encoding exch def",
  "\tdup /FontName exch def",
  "\tcurrentdict end definefont pop",
  "} bind def",
};

char *latin1[] = {
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  "space",
  "exclam",
  "quotedbl",
  "numbersign",
  "dollar",
  "percent",
  "ampersand",
  "quoteright",
  "parenleft",
  "parenright",
  "asterisk",
  "plus",
  "comma",
  "hyphen",  /* this should be `minus', but not all PS printers have it */
  "period",
  "slash",
  "zero",
  "one",
  "two",
  "three",
  "four",
  "five",
  "six",
  "seven",
  "eight",
  "nine",
  "colon",
  "semicolon",
  "less",
  "equal",
  "greater",
  "question",
  "at",
  "A",
  "B",
  "C",
  "D",
  "E",
  "F",
  "G",
  "H",
  "I",
  "J",
  "K",
  "L",
  "M",
  "N",
  "O",
  "P",
  "Q",
  "R",
  "S",
  "T",
  "U",
  "V",
  "W",
  "X",
  "Y",
  "Z",
  "bracketleft",
  "backslash",
  "bracketright",
  "asciicircum",
  "underscore",
  "quoteleft",
  "a",
  "b",
  "c",
  "d",
  "e",
  "f",
  "g",
  "h",
  "i",
  "j",
  "k",
  "l",
  "m",
  "n",
  "o",
  "p",
  "q",
  "r",
  "s",
  "t",
  "u",
  "v",
  "w",
  "x",
  "y",
  "z",
  "braceleft",
  "bar",
  "braceright",
  "asciitilde",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  ".notdef",
  "dotlessi",
  "grave",
  "acute",
  "circumflex",
  "tilde",
  "macron",
  "breve",
  "dotaccent",
  "dieresis",
  ".notdef",
  "ring",
  "cedilla",
  ".notdef",
  "hungarumlaut",
  "ogonek",
  "caron",
  ".notdef",
  "exclamdown",
  "cent",
  "sterling",
  "currency",
  "yen",
  "brokenbar",
  "section",
  "dieresis",
  "copyright",
  "ordfeminine",
  "guilsinglleft",
  "logicalnot",
  "hyphen",
  "registered",
  "macron",
  "degree",
  "plusminus",
  "twosuperior",
  "threesuperior",
  "acute",
  "mu",
  "paragraph",
  "periodcentered",
  "cedilla",
  "onesuperior",
  "ordmasculine",
  "guilsinglright",
  "onequarter",
  "onehalf",
  "threequarters",
  "questiondown",
  "Agrave",
  "Aacute",
  "Acircumflex",
  "Atilde",
  "Adieresis",
  "Aring",
  "AE",
  "Ccedilla",
  "Egrave",
  "Eacute",
  "Ecircumflex",
  "Edieresis",
  "Igrave",
  "Iacute",
  "Icircumflex",
  "Idieresis",
  "Eth",
  "Ntilde",
  "Ograve",
  "Oacute",
  "Ocircumflex",
  "Otilde",
  "Odieresis",
  "multiply",
  "Oslash",
  "Ugrave",
  "Uacute",
  "Ucircumflex",
  "Udieresis",
  "Yacute",
  "Thorn",
  "germandbls",
  "agrave",
  "aacute",
  "acircumflex",
  "atilde",
  "adieresis",
  "aring",
  "ae",
  "ccedilla",
  "egrave",
  "eacute",
  "ecircumflex",
  "edieresis",
  "igrave",
  "iacute",
  "icircumflex",
  "idieresis",
  "eth",
  "ntilde",
  "ograve",
  "oacute",
  "ocircumflex",
  "otilde",
  "odieresis",
  "divide",
  "oslash",
  "ugrave",
  "uacute",
  "ucircumflex",
  "udieresis",
  "yacute",
  "thorn",
  "ydieresis",
};
  
void prologue()
{
  int col, i;

  printf("%%!PS-Adobe-3.0\n");
  printf("%%%%DocumentNeededResources: font %s\n", font);
  printf("%%%%+ font %s\n", bold_font);
  printf("%%%%Pages: (atend)\n");
  printf("%%%%EndComments\n");
  printf("%%%%BeginProlog\n");
  printf("/textps 10 dict def textps begin\n");
  for (i = 0; i < sizeof(defs)/sizeof(defs[0]); i++)
    printf("%s\n", defs[i]);
  printf("/ISOLatin1Encoding where{pop}{/ISOLatin1Encoding[\n");

  col = 0;
  for (i = 0; i < 256; i++) {
    int len = strlen(latin1[i]) + 1;
    col += len;
    if (col > 79) {
      putchar('\n');
      col = len;
    }
    printf("/%s", latin1[i]);
  }
  printf("\n] def}ifelse\nend\n");
  printf("%%%%BeginSetup\n");
  printf("%%%%IncludeResource: font %s\n", font);
  printf("%%%%IncludeResource: font %s\n", bold_font);
  printf("textps begin\n");
  printf("/__%s ISOLatin1Encoding /%s RE\n", font, font);
  printf("/R [ /__%s findfont %f scalefont /setfont load ] cvx def\n",
	 font, char_width/.6);
  printf("/__%s ISOLatin1Encoding /%s RE\n", bold_font, bold_font);
  printf("/B [ /__%s findfont %f scalefont /setfont load ] cvx def\n", 
	 bold_font, char_width/.6);
  printf("%%%%EndSetup\n");
  printf("%%%%EndProlog\n");
}

void trailer()
{
  printf("%%%%Trailer\nend\n%%%%Pages: %d\n", pageno);
}

/* p is ordered greatest position first */

void add_char(c, pos, p)
int c;
int pos;
output_char **p;
{
  for (;; p = &(*p)->next) {
    if (*p == NULL || (*p)->pos < pos) {
      output_char *tem = new_output_char();
      tem->next = *p;
      *p = tem;
      tem->c = c;
      tem->is_bold = 0;
      tem->pos = pos;
      break;
    }
    else if ((*p)->pos == pos) {
      if (c == (*p)->c) {
	(*p)->is_bold = 1;
	break;
      }
    }
  }
}

void do_file()
{
  int c;
  int vpos = 0;
  int hpos = 0;
  int page_started = 0;
  int esced = 0;
  output_char *output_line = 0;
  while ((c = getchar()) != EOF)
    if (esced)
      switch(c) {
      case '7':
	if (vpos > 0) {
	  if (output_line != NULL) {
	    if (!page_started) {
	      page_started = 1;
	      page_start();
	    }
	    print_line(output_line, vpos);
	    output_line = 0;
	  }
	  vpos -= 1;
	}
	/* hpos = 0; */
	esced = 0;
	break;
      default:
	/* silently ignore */
	esced = 0;
	break;
      }
    else
      switch (c) {
      case '\033':
	esced = 1;
	break;
      case '\b':
	if (hpos > 0)
	  hpos--;
	break;
      case '\f':
	if (!page_started)
	  page_start();
	print_line(output_line, vpos);
	output_line = 0;
	page_end();
	hpos = 0;		/* ?? */
	vpos = 0;
	page_started = 0;
	break;
      case '\r':
	hpos = 0;
	break;
      case '\n':
	if (output_line != NULL) {
	  if (!page_started) {
	    page_started = 1;
	    page_start();
	  }
	  print_line(output_line, vpos);
	  output_line = 0;
	}
	vpos += 1;
	if (vpos >= lines_per_page) {
	  if (!page_started)
	    page_start();
	  page_end();
	  page_started = 0;
	  vpos = 0;
	}
	hpos = 0;
	break;
      case ' ':
	hpos++;
	break;
      case '\t':
	hpos = ((hpos + tab_width)/tab_width)*tab_width;
	break;
      default:
	if (!(isascii(c) && iscntrl(c))) {
	  add_char(c, hpos, &output_line);
	  hpos++;
	}
	break;
      }
  if (output_line != NULL) {
    if (!page_started) {
      page_started = 1;
      page_start();
    }
    print_line(output_line, vpos);
    output_line = 0;
  }
  if (page_started)
    page_end();
}
