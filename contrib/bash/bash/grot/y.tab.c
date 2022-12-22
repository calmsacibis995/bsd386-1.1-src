
/*  A Bison parser, made from parse.y with Bison version GNU Bison version 1.22
  */

#define YYBISON 1  /* Identify Bison output.  */

#define	IF	258
#define	THEN	259
#define	ELSE	260
#define	ELIF	261
#define	FI	262
#define	CASE	263
#define	ESAC	264
#define	FOR	265
#define	WHILE	266
#define	UNTIL	267
#define	DO	268
#define	DONE	269
#define	FUNCTION	270
#define	IN	271
#define	BANG	272
#define	WORD	273
#define	ASSIGNMENT_WORD	274
#define	NUMBER	275
#define	AND_AND	276
#define	OR_OR	277
#define	GREATER_GREATER	278
#define	LESS_LESS	279
#define	LESS_AND	280
#define	GREATER_AND	281
#define	SEMI_SEMI	282
#define	LESS_LESS_MINUS	283
#define	AND_GREATER	284
#define	LESS_GREATER	285
#define	GREATER_BAR	286
#define	yacc_EOF	287

#line 21 "parse.y"

#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include "bashansi.h"
#include "shell.h"
#include "flags.h"
#include "input.h"

#if defined (READLINE)
#  include <readline/readline.h>
#endif /* READLINE */

#if defined (HISTORY)
#  include <readline/history.h>
#endif /* HISTORY */

#if defined (JOB_CONTROL)
#  include "jobs.h"
#endif /* JOB_CONTROL */

#if defined (ALIAS)
#  include "alias.h"
#endif /* ALIAS */

#if defined (PROMPT_STRING_DECODE)
#include <sys/param.h>
#include <time.h>
#include "maxpath.h"
#endif /* PROMPT_STRING_DECODE */

#define YYDEBUG 1
extern int eof_encountered;
extern int no_line_editing;
extern int interactive, interactive_shell;
extern int posixly_correct;
extern int last_command_exit_value;

/* **************************************************************** */
/*								    */
/*		    "Forward" declarations			    */
/*								    */
/* **************************************************************** */

/* This is kind of sickening.  In order to let these variables be seen by
   all the functions that need them, I am forced to place their declarations
   far away from the place where they should logically be found. */

static int reserved_word_acceptable ();
static int read_token ();

static void report_syntax_error ();
static void handle_eof_input_unit ();
static void prompt_again ();
static void bash_add_history ();
static void reset_readline_prompt ();

/* PROMPT_STRING_POINTER points to one of these, never to an actual string. */
char *ps1_prompt, *ps2_prompt;

/* Handle on the current prompt string.  Indirectly points through
   ps1_ or ps2_prompt. */
char **prompt_string_pointer = (char **)NULL;
char *current_prompt_string;

char *decode_prompt_string ();

void gather_here_documents ();

/* The number of lines read from input while creating the current command. */
int current_command_line_count = 0;

/* Variables to manage the task of reading here documents, because we need to
   defer the reading until after a complete command has been collected. */
static REDIRECT *redir_stack[10];
int need_here_doc = 0;

#line 99 "parse.y"
typedef union {
  WORD_DESC *word;		/* the word that we read. */
  int number;			/* the number that we read. */
  WORD_LIST *word_list;
  COMMAND *command;
  REDIRECT *redirect;
  ELEMENT element;
  PATTERN_LIST *pattern;
} YYSTYPE;

#ifndef YYLTYPE
typedef
  struct yyltype
    {
      int timestamp;
      int first_line;
      int first_column;
      int last_line;
      int last_column;
      char *text;
   }
  yyltype;

#define YYLTYPE yyltype
#endif

#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		229
#define	YYFLAG		-32768
#define	YYNTBASE	44

#define YYTRANSLATE(x) ((unsigned)(x) <= 287 ? yytranslate[x] : 70)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,    34,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,    32,     2,    42,
    43,     2,     2,     2,    39,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,    33,    38,
     2,    37,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    40,    36,    41,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     2,     3,     4,     5,
     6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
    16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
    26,    27,    28,    29,    30,    31,    35
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     3,     5,     8,    10,    11,    14,    17,    20,    24,
    28,    31,    35,    38,    42,    45,    49,    52,    56,    59,
    63,    66,    70,    73,    77,    80,    84,    87,    91,    94,
    98,   101,   104,   108,   110,   112,   114,   116,   119,   121,
   124,   126,   128,   130,   133,   140,   147,   155,   163,   174,
   185,   192,   200,   207,   209,   215,   221,   225,   227,   229,
   235,   242,   249,   257,   262,   268,   274,   282,   289,   293,
   298,   305,   311,   313,   316,   321,   326,   332,   338,   340,
   343,   349,   355,   362,   369,   371,   375,   378,   380,   384,
   388,   392,   397,   402,   407,   412,   417,   419,   422,   424,
   426,   428,   429,   432,   434,   437,   440,   445,   450,   454,
   458,   460,   463,   468
};

static const short yyrhs[] = {    67,
    34,     0,    34,     0,     1,    34,     0,    35,     0,     0,
    45,    18,     0,    37,    18,     0,    38,    18,     0,    20,
    37,    18,     0,    20,    38,    18,     0,    23,    18,     0,
    20,    23,    18,     0,    24,    18,     0,    20,    24,    18,
     0,    25,    20,     0,    20,    25,    20,     0,    26,    20,
     0,    20,    26,    20,     0,    25,    18,     0,    20,    25,
    18,     0,    26,    18,     0,    20,    26,    18,     0,    28,
    18,     0,    20,    28,    18,     0,    26,    39,     0,    20,
    26,    39,     0,    25,    39,     0,    20,    25,    39,     0,
    29,    18,     0,    20,    30,    18,     0,    30,    18,     0,
    31,    18,     0,    20,    31,    18,     0,    18,     0,    19,
     0,    46,     0,    46,     0,    48,    46,     0,    47,     0,
    49,    47,     0,    49,     0,    51,     0,    52,     0,    52,
    48,     0,    10,    18,    66,    13,    62,    14,     0,    10,
    18,    66,    40,    62,    41,     0,    10,    18,    33,    66,
    13,    62,    14,     0,    10,    18,    33,    66,    40,    62,
    41,     0,    10,    18,    66,    16,    45,    65,    66,    13,
    62,    14,     0,    10,    18,    66,    16,    45,    65,    66,
    40,    62,    41,     0,     8,    18,    66,    16,    66,     9,
     0,     8,    18,    66,    16,    59,    66,     9,     0,     8,
    18,    66,    16,    57,     9,     0,    54,     0,    11,    62,
    13,    62,    14,     0,    12,    62,    13,    62,    14,     0,
    42,    62,    43,     0,    55,     0,    53,     0,    18,    42,
    43,    66,    55,     0,    18,    42,    43,    66,    55,    48,
     0,    15,    18,    42,    43,    66,    55,     0,    15,    18,
    42,    43,    66,    55,    48,     0,    15,    18,    66,    55,
     0,    15,    18,    66,    55,    48,     0,     3,    62,     4,
    62,     7,     0,     3,    62,     4,    62,     5,    62,     7,
     0,     3,    62,     4,    62,    56,     7,     0,    40,    62,
    41,     0,     6,    62,     4,    62,     0,     6,    62,     4,
    62,     5,    62,     0,     6,    62,     4,    62,    56,     0,
    58,     0,    59,    58,     0,    66,    61,    43,    62,     0,
    66,    61,    43,    66,     0,    66,    42,    61,    43,    62,
     0,    66,    42,    61,    43,    66,     0,    60,     0,    59,
    60,     0,    66,    61,    43,    62,    27,     0,    66,    61,
    43,    66,    27,     0,    66,    42,    61,    43,    62,    27,
     0,    66,    42,    61,    43,    66,    27,     0,    18,     0,
    61,    36,    18,     0,    66,    63,     0,    64,     0,    64,
    34,    66,     0,    64,    32,    66,     0,    64,    33,    66,
     0,    64,    21,    66,    64,     0,    64,    22,    66,    64,
     0,    64,    32,    66,    64,     0,    64,    33,    66,    64,
     0,    64,    34,    66,    64,     0,    69,     0,    17,    69,
     0,    34,     0,    33,     0,    35,     0,     0,    66,    34,
     0,    68,     0,    68,    32,     0,    68,    33,     0,    68,
    21,    66,    68,     0,    68,    22,    66,    68,     0,    68,
    32,    68,     0,    68,    33,    68,     0,    69,     0,    17,
    69,     0,    69,    36,    66,    69,     0,    50,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   140,   149,   156,   172,   182,   184,   188,   190,   192,   194,
   196,   198,   200,   205,   210,   215,   220,   225,   230,   235,
   240,   245,   250,   256,   262,   264,   266,   268,   270,   272,
   274,   288,   290,   294,   296,   298,   302,   306,   317,   319,
   323,   325,   329,   331,   335,   337,   339,   341,   344,   346,
   349,   351,   353,   358,   360,   362,   365,   368,   371,   375,
   378,   381,   384,   387,   390,   394,   396,   398,   403,   407,
   409,   411,   415,   416,   420,   422,   424,   426,   430,   432,
   436,   438,   440,   442,   446,   448,   457,   465,   466,   467,
   469,   473,   475,   477,   482,   484,   486,   488,   495,   496,
   497,   500,   501,   510,   516,   523,   531,   533,   535,   540,
   542,   544,   551,   554
};

static const char * const yytname[] = {   "$","error","$illegal.","IF","THEN",
"ELSE","ELIF","FI","CASE","ESAC","FOR","WHILE","UNTIL","DO","DONE","FUNCTION",
"IN","BANG","WORD","ASSIGNMENT_WORD","NUMBER","AND_AND","OR_OR","GREATER_GREATER",
"LESS_LESS","LESS_AND","GREATER_AND","SEMI_SEMI","LESS_LESS_MINUS","AND_GREATER",
"LESS_GREATER","GREATER_BAR","'&'","';'","'\\n'","yacc_EOF","'|'","'>'","'<'",
"'-'","'{'","'}'","'('","')'","inputunit","words","redirection","simple_command_element",
"redirections","simple_command","command","shell_command","shell_command_1",
"function_def","if_command","group_command","elif_clause","case_clause_1","pattern_list_1",
"case_clause_sequence","pattern_list","pattern","list","list0","list1","list_terminator",
"newlines","simple_list","simple_list1","pipeline",""
};
#endif

static const short yyr1[] = {     0,
    44,    44,    44,    44,    45,    45,    46,    46,    46,    46,
    46,    46,    46,    46,    46,    46,    46,    46,    46,    46,
    46,    46,    46,    46,    46,    46,    46,    46,    46,    46,
    46,    46,    46,    47,    47,    47,    48,    48,    49,    49,
    50,    50,    51,    51,    52,    52,    52,    52,    52,    52,
    52,    52,    52,    52,    52,    52,    52,    52,    52,    53,
    53,    53,    53,    53,    53,    54,    54,    54,    55,    56,
    56,    56,    57,    57,    58,    58,    58,    58,    59,    59,
    60,    60,    60,    60,    61,    61,    62,    63,    63,    63,
    63,    64,    64,    64,    64,    64,    64,    64,    65,    65,
    65,    66,    66,    67,    67,    67,    68,    68,    68,    68,
    68,    68,    69,    69
};

static const short yyr2[] = {     0,
     2,     1,     2,     1,     0,     2,     2,     2,     3,     3,
     2,     3,     2,     3,     2,     3,     2,     3,     2,     3,
     2,     3,     2,     3,     2,     3,     2,     3,     2,     3,
     2,     2,     3,     1,     1,     1,     1,     2,     1,     2,
     1,     1,     1,     2,     6,     6,     7,     7,    10,    10,
     6,     7,     6,     1,     5,     5,     3,     1,     1,     5,
     6,     6,     7,     4,     5,     5,     7,     6,     3,     4,
     6,     5,     1,     2,     4,     4,     5,     5,     1,     2,
     5,     5,     6,     6,     1,     3,     2,     1,     3,     3,
     3,     4,     4,     4,     4,     4,     1,     2,     1,     1,
     1,     0,     2,     1,     2,     2,     4,     4,     3,     3,
     1,     2,     4,     1
};

static const short yydefact[] = {     0,
     0,   102,     0,     0,   102,   102,     0,     0,    34,    35,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     2,
     4,     0,     0,   102,   102,    36,    39,    41,   114,    42,
    43,    59,    54,    58,     0,   104,   111,     3,     0,     0,
   102,   102,     0,     0,   102,   112,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,    11,    13,    19,    15,
    27,    21,    17,    25,    23,    29,    31,    32,     7,     8,
     0,     0,    34,    40,    37,    44,     1,   102,   102,   105,
   106,   102,   102,     0,   103,    87,    88,    97,     0,   102,
     0,   102,   102,     0,     0,   102,    12,    14,    20,    16,
    28,    22,    18,    26,    24,    30,    33,     9,    10,    69,
    57,    38,     0,     0,   109,   110,     0,     0,    98,   102,
   102,   102,   102,   102,   102,     0,   102,     5,   102,     0,
     0,   102,    64,     0,   107,   108,     0,     0,   113,   102,
   102,    66,     0,     0,     0,    90,    91,    89,     0,    73,
   102,    79,     0,   102,   102,     0,     0,     0,    55,    56,
     0,    65,    60,     0,     0,    68,    92,    93,    94,    95,
    96,    53,    74,    80,     0,    51,    85,     0,     0,     0,
     0,    45,     6,   100,    99,   101,   102,    46,    62,    61,
    67,   102,   102,   102,   102,    52,     0,     0,   102,    47,
    48,     0,    63,    70,     0,     0,     0,   102,    86,    75,
    76,   102,   102,   102,    72,    77,    78,    81,    82,     0,
     0,    71,    83,    84,    49,    50,     0,     0,     0
};

static const short yydefgoto[] = {   227,
   157,    26,    27,    76,    28,    29,    30,    31,    32,    33,
    34,   143,   149,   150,   151,   152,   179,    39,    86,    87,
   187,    40,    35,   115,    88
};

static const short yypact[] = {   203,
   -20,-32768,    -2,    26,-32768,-32768,    40,   434,   -17,-32768,
   475,    45,    56,    16,    36,    58,    71,    85,    88,-32768,
-32768,    96,    97,-32768,-32768,-32768,-32768,   459,-32768,-32768,
   160,-32768,-32768,-32768,    89,    64,    90,-32768,   121,   302,
-32768,    94,   115,   116,    95,    90,    87,   113,   114,    44,
    59,   120,   122,   126,   127,   129,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
    98,   105,-32768,-32768,-32768,   160,-32768,-32768,-32768,   368,
   368,-32768,-32768,   434,-32768,-32768,    86,    90,     7,-32768,
     5,-32768,-32768,   108,    65,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,-32768,   335,   335,     6,     6,   401,    66,    90,-32768,
-32768,-32768,-32768,-32768,-32768,    17,-32768,-32768,-32768,   138,
   139,-32768,   160,    65,-32768,-32768,   368,   368,    90,-32768,
-32768,-32768,   147,   302,   302,   302,   302,   302,   146,-32768,
-32768,-32768,     4,-32768,-32768,   142,    47,   117,-32768,-32768,
    65,   160,   160,   152,   158,-32768,-32768,-32768,    79,    79,
    79,-32768,-32768,-32768,     8,-32768,-32768,   150,   -12,   156,
   130,-32768,-32768,-32768,-32768,-32768,-32768,-32768,   160,   160,
-32768,-32768,-32768,-32768,-32768,-32768,    -3,   154,-32768,-32768,
-32768,    19,   160,   106,   302,   302,   302,-32768,-32768,   148,
   236,-32768,-32768,-32768,-32768,   149,   269,-32768,-32768,   159,
   136,-32768,-32768,-32768,-32768,-32768,   179,   181,-32768
};

static const short yypgoto[] = {-32768,
-32768,   -29,   164,  -128,-32768,-32768,-32768,-32768,-32768,-32768,
   -91,   -22,-32768,    42,-32768,    48,    18,    -5,-32768,  -138,
-32768,   -30,-32768,     3,    29
};


#define	YYLAST		513


static const short yytable[] = {    43,
    44,    75,    36,   133,   162,   167,   168,   169,   170,   171,
    89,    91,   176,    38,    95,    41,   196,   127,    71,    72,
   128,   177,   125,   198,    47,   177,    78,    79,    37,   154,
   199,   212,   198,    59,   190,    60,    46,    85,    85,   208,
    85,    85,   163,    42,   129,   178,   112,   113,   114,   178,
    85,   117,    85,    62,    61,    63,   155,    45,   213,   126,
   203,    99,    57,   100,   183,   134,   169,   170,   171,   189,
   140,   141,   142,    58,    64,    65,   102,   118,   103,   184,
   185,   186,   101,   116,    78,    79,   130,   131,    66,   144,
   145,   146,   147,   148,   153,    80,    81,   104,    85,   120,
   121,   161,    67,    75,    24,    68,   120,   121,    37,    37,
   214,   141,   119,    69,    70,   135,   136,   122,   123,   124,
   175,   156,    77,   158,    83,    82,    90,    92,    93,    96,
    97,    98,   112,    75,   164,   165,    94,   105,   110,   106,
   116,    37,    37,   107,   108,   139,   109,   111,   180,   181,
   132,   159,   160,   166,   172,   182,   202,   188,   191,    75,
   112,   192,   205,   206,   207,    37,    37,   177,   211,   200,
   201,   209,   225,   112,   218,   223,   226,   217,   228,    11,
   229,   215,    12,    13,    14,    15,   204,    16,    17,    18,
    19,    74,   173,   210,     0,   197,    22,    23,   174,     0,
     0,     0,   216,     1,     0,     2,   220,   221,   222,     0,
     3,     0,     4,     5,     6,     0,     0,     7,     0,     8,
     9,    10,    11,     0,     0,    12,    13,    14,    15,     0,
    16,    17,    18,    19,     0,     0,    20,    21,     2,    22,
    23,     0,    24,     3,    25,     4,     5,     6,     0,     0,
     7,     0,    84,     9,    10,    11,     0,     0,    12,    13,
    14,    15,   219,    16,    17,    18,    19,     0,     0,    85,
     0,     2,    22,    23,     0,    24,     3,    25,     4,     5,
     6,     0,     0,     7,     0,    84,     9,    10,    11,     0,
     0,    12,    13,    14,    15,   224,    16,    17,    18,    19,
     0,     0,    85,     0,     2,    22,    23,     0,    24,     3,
    25,     4,     5,     6,     0,     0,     7,     0,    84,     9,
    10,    11,     0,     0,    12,    13,    14,    15,     0,    16,
    17,    18,    19,     0,     0,    85,     0,     2,    22,    23,
     0,    24,     3,    25,     4,     5,     6,     0,     0,     7,
     0,     8,     9,    10,    11,     0,     0,    12,    13,    14,
    15,     0,    16,    17,    18,    19,     0,     0,    85,     0,
     2,    22,    23,     0,    24,     3,    25,     4,     5,     6,
     0,     0,     7,     0,     8,     9,    10,    11,     0,     0,
    12,    13,    14,    15,     0,    16,    17,    18,    19,     0,
     0,     0,     0,     2,    22,    23,     0,    24,     3,    25,
     4,     5,     6,     0,     0,     7,     0,     0,     9,    10,
    11,     0,     0,    12,    13,    14,    15,     0,    16,    17,
    18,    19,     0,     0,    85,     0,     2,    22,    23,     0,
    24,     3,    25,     4,     5,     6,     0,     0,     7,     0,
     0,     9,    10,    11,     0,     0,    12,    13,    14,    15,
     0,    16,    17,    18,    19,     0,     0,     0,     0,     0,
    22,    23,     0,    24,     0,    25,    73,    10,    11,     0,
     0,    12,    13,    14,    15,     0,    16,    17,    18,    19,
     0,     0,     0,     0,     0,    22,    23,    48,    49,    50,
    51,     0,    52,     0,    53,    54,     0,     0,     0,     0,
     0,    55,    56
};

static const short yycheck[] = {     5,
     6,    31,     0,    95,   133,   144,   145,   146,   147,   148,
    41,    42,     9,    34,    45,    18,     9,    13,    24,    25,
    16,    18,    16,    36,    42,    18,    21,    22,     0,    13,
    43,    13,    36,    18,   163,    20,     8,    34,    34,    43,
    34,    34,   134,    18,    40,    42,    76,    78,    79,    42,
    34,    82,    34,    18,    39,    20,    40,    18,    40,    90,
   189,    18,    18,    20,    18,    96,   205,   206,   207,   161,
     5,     6,     7,    18,    39,    18,    18,    83,    20,    33,
    34,    35,    39,    81,    21,    22,    92,    93,    18,   120,
   121,   122,   123,   124,   125,    32,    33,    39,    34,    21,
    22,   132,    18,   133,    40,    18,    21,    22,    80,    81,
     5,     6,    84,    18,    18,   113,   114,    32,    33,    34,
   151,   127,    34,   129,     4,    36,    33,    13,    13,    43,
    18,    18,   162,   163,   140,   141,    42,    18,    41,    18,
   138,   113,   114,    18,    18,   117,    18,    43,   154,   155,
    43,    14,    14,     7,     9,    14,   187,    41,     7,   189,
   190,     4,   193,   194,   195,   137,   138,    18,   199,    14,
    41,    18,    14,   203,    27,    27,    41,   208,     0,    20,
     0,   204,    23,    24,    25,    26,   192,    28,    29,    30,
    31,    28,   151,   199,    -1,   178,    37,    38,   151,    -1,
    -1,    -1,   208,     1,    -1,     3,   212,   213,   214,    -1,
     8,    -1,    10,    11,    12,    -1,    -1,    15,    -1,    17,
    18,    19,    20,    -1,    -1,    23,    24,    25,    26,    -1,
    28,    29,    30,    31,    -1,    -1,    34,    35,     3,    37,
    38,    -1,    40,     8,    42,    10,    11,    12,    -1,    -1,
    15,    -1,    17,    18,    19,    20,    -1,    -1,    23,    24,
    25,    26,    27,    28,    29,    30,    31,    -1,    -1,    34,
    -1,     3,    37,    38,    -1,    40,     8,    42,    10,    11,
    12,    -1,    -1,    15,    -1,    17,    18,    19,    20,    -1,
    -1,    23,    24,    25,    26,    27,    28,    29,    30,    31,
    -1,    -1,    34,    -1,     3,    37,    38,    -1,    40,     8,
    42,    10,    11,    12,    -1,    -1,    15,    -1,    17,    18,
    19,    20,    -1,    -1,    23,    24,    25,    26,    -1,    28,
    29,    30,    31,    -1,    -1,    34,    -1,     3,    37,    38,
    -1,    40,     8,    42,    10,    11,    12,    -1,    -1,    15,
    -1,    17,    18,    19,    20,    -1,    -1,    23,    24,    25,
    26,    -1,    28,    29,    30,    31,    -1,    -1,    34,    -1,
     3,    37,    38,    -1,    40,     8,    42,    10,    11,    12,
    -1,    -1,    15,    -1,    17,    18,    19,    20,    -1,    -1,
    23,    24,    25,    26,    -1,    28,    29,    30,    31,    -1,
    -1,    -1,    -1,     3,    37,    38,    -1,    40,     8,    42,
    10,    11,    12,    -1,    -1,    15,    -1,    -1,    18,    19,
    20,    -1,    -1,    23,    24,    25,    26,    -1,    28,    29,
    30,    31,    -1,    -1,    34,    -1,     3,    37,    38,    -1,
    40,     8,    42,    10,    11,    12,    -1,    -1,    15,    -1,
    -1,    18,    19,    20,    -1,    -1,    23,    24,    25,    26,
    -1,    28,    29,    30,    31,    -1,    -1,    -1,    -1,    -1,
    37,    38,    -1,    40,    -1,    42,    18,    19,    20,    -1,
    -1,    23,    24,    25,    26,    -1,    28,    29,    30,    31,
    -1,    -1,    -1,    -1,    -1,    37,    38,    23,    24,    25,
    26,    -1,    28,    -1,    30,    31,    -1,    -1,    -1,    -1,
    -1,    37,    38
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/local/lib/bison.simple"

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Bob Corbett and Richard Stallman

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */


#ifndef alloca
#ifdef __GNUC__
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi)
#include <alloca.h>
#else /* not sparc */
#if defined (MSDOS) && !defined (__TURBOC__)
#include <malloc.h>
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
#include <malloc.h>
 #pragma alloca
#else /* not MSDOS, __TURBOC__, or _AIX */
#ifdef __hpux
#ifdef __cplusplus
extern "C" {
void *alloca (unsigned int);
};
#else /* not __cplusplus */
void *alloca ();
#endif /* not __cplusplus */
#endif /* __hpux */
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc.  */
#endif /* not GNU C.  */
#endif /* alloca not defined.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	return(0)
#define YYABORT 	return(1)
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#define YYLEX		yylex(&yylval, &yylloc)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
int yyparse (void);
#endif

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_bcopy(FROM,TO,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_bcopy (from, to, count)
     char *from;
     char *to;
     int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_bcopy (char *from, char *to, int count)
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 184 "/usr/local/lib/bison.simple"
int
yyparse()
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
      yyss = (short *) alloca (yystacksize * sizeof (*yyssp));
      __yy_bcopy ((char *)yyss1, (char *)yyss, size * sizeof (*yyssp));
      yyvs = (YYSTYPE *) alloca (yystacksize * sizeof (*yyvsp));
      __yy_bcopy ((char *)yyvs1, (char *)yyvs, size * sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) alloca (yystacksize * sizeof (*yylsp));
      __yy_bcopy ((char *)yyls1, (char *)yyls, size * sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 1:
#line 141 "parse.y"
{
			  /* Case of regular command.  Discard the error
			     safety net,and return the command just parsed. */
			  global_command = yyvsp[-1].command;
			  eof_encountered = 0;
			  discard_parser_constructs (0);
			  YYACCEPT;
			;
    break;}
case 2:
#line 150 "parse.y"
{
			  /* Case of regular command, but not a very
			     interesting one.  Return a NULL command. */
			  global_command = (COMMAND *)NULL;
			  YYACCEPT;
			;
    break;}
case 3:
#line 158 "parse.y"
{
			  /* Error during parsing.  Return NULL command. */
			  global_command = (COMMAND *)NULL;
			  eof_encountered = 0;
			  discard_parser_constructs (1);
			  if (interactive)
			    {
			      YYACCEPT;
			    }
			  else
			    {
			      YYABORT;
			    }
			;
    break;}
case 4:
#line 173 "parse.y"
{
			  /* Case of EOF seen by itself.  Do ignoreeof or 
			     not. */
			  global_command = (COMMAND *)NULL;
			  handle_eof_input_unit ();
			  YYACCEPT;
			;
    break;}
case 5:
#line 183 "parse.y"
{ yyval.word_list = (WORD_LIST *)NULL; ;
    break;}
case 6:
#line 185 "parse.y"
{ yyval.word_list = make_word_list (yyvsp[0].word, yyvsp[-1].word_list); ;
    break;}
case 7:
#line 189 "parse.y"
{ yyval.redirect = make_redirection ( 1, r_output_direction, yyvsp[0].word); ;
    break;}
case 8:
#line 191 "parse.y"
{ yyval.redirect = make_redirection ( 0, r_input_direction, yyvsp[0].word); ;
    break;}
case 9:
#line 193 "parse.y"
{ yyval.redirect = make_redirection (yyvsp[-2].number, r_output_direction, yyvsp[0].word); ;
    break;}
case 10:
#line 195 "parse.y"
{ yyval.redirect = make_redirection (yyvsp[-2].number, r_input_direction, yyvsp[0].word); ;
    break;}
case 11:
#line 197 "parse.y"
{ yyval.redirect = make_redirection ( 1, r_appending_to, yyvsp[0].word); ;
    break;}
case 12:
#line 199 "parse.y"
{ yyval.redirect = make_redirection (yyvsp[-2].number, r_appending_to, yyvsp[0].word); ;
    break;}
case 13:
#line 201 "parse.y"
{
			  yyval.redirect = make_redirection ( 0, r_reading_until, yyvsp[0].word);
			  redir_stack[need_here_doc++] = yyval.redirect;
			;
    break;}
case 14:
#line 206 "parse.y"
{
			  yyval.redirect = make_redirection (yyvsp[-2].number, r_reading_until, yyvsp[0].word);
			  redir_stack[need_here_doc++] = yyval.redirect;
			;
    break;}
case 15:
#line 211 "parse.y"
{
			  long rd = yyvsp[0].number;
			  yyval.redirect = make_redirection ( 0, r_duplicating_input, rd);
			;
    break;}
case 16:
#line 216 "parse.y"
{
			  long rd = yyvsp[0].number;
			  yyval.redirect = make_redirection (yyvsp[-2].number, r_duplicating_input, rd);
			;
    break;}
case 17:
#line 221 "parse.y"
{
			  long rd = yyvsp[0].number;
			  yyval.redirect = make_redirection ( 1, r_duplicating_output, rd);
			;
    break;}
case 18:
#line 226 "parse.y"
{
			  long rd = yyvsp[0].number;
			  yyval.redirect = make_redirection (yyvsp[-2].number, r_duplicating_output, rd);
			;
    break;}
case 19:
#line 231 "parse.y"
{
			  yyval.redirect = make_redirection
			    (0, r_duplicating_input_word, yyvsp[0].word);
			;
    break;}
case 20:
#line 236 "parse.y"
{
			  yyval.redirect = make_redirection
			    (yyvsp[-2].number, r_duplicating_input_word, yyvsp[0].word);
			;
    break;}
case 21:
#line 241 "parse.y"
{
			  yyval.redirect = make_redirection
			    (1, r_duplicating_output_word, yyvsp[0].word);
			;
    break;}
case 22:
#line 246 "parse.y"
{
			  yyval.redirect = make_redirection
			    (yyvsp[-2].number, r_duplicating_output_word, yyvsp[0].word);
			;
    break;}
case 23:
#line 251 "parse.y"
{
			  yyval.redirect = make_redirection
			    (0, r_deblank_reading_until, yyvsp[0].word);
			  redir_stack[need_here_doc++] = yyval.redirect;
			;
    break;}
case 24:
#line 257 "parse.y"
{
			  yyval.redirect = make_redirection
			    (yyvsp[-2].number, r_deblank_reading_until, yyvsp[0].word);
			  redir_stack[need_here_doc++] = yyval.redirect;
			;
    break;}
case 25:
#line 263 "parse.y"
{ yyval.redirect = make_redirection ( 1, r_close_this, 0L); ;
    break;}
case 26:
#line 265 "parse.y"
{ yyval.redirect = make_redirection (yyvsp[-2].number, r_close_this, 0L); ;
    break;}
case 27:
#line 267 "parse.y"
{ yyval.redirect = make_redirection ( 0, r_close_this, 0L); ;
    break;}
case 28:
#line 269 "parse.y"
{ yyval.redirect = make_redirection (yyvsp[-2].number, r_close_this, 0L); ;
    break;}
case 29:
#line 271 "parse.y"
{ yyval.redirect = make_redirection ( 1, r_err_and_out, yyvsp[0].word); ;
    break;}
case 30:
#line 273 "parse.y"
{ yyval.redirect = make_redirection ( yyvsp[-2].number, r_input_output, yyvsp[0].word); ;
    break;}
case 31:
#line 275 "parse.y"
{
			  REDIRECT *t1, *t2;

			  if (posixly_correct)
			    yyval.redirect = make_redirection (0, r_input_output, yyvsp[0].word);
			  else
			    {
			      t1 = make_redirection (0, r_input_direction, yyvsp[0].word);
			      t2 = make_redirection (1, r_output_direction, copy_word (yyvsp[0].word));
			      t1->next = t2;
			      yyval.redirect = t1;
			    }
			;
    break;}
case 32:
#line 289 "parse.y"
{ yyval.redirect = make_redirection ( 1, r_output_force, yyvsp[0].word); ;
    break;}
case 33:
#line 291 "parse.y"
{ yyval.redirect = make_redirection ( yyvsp[-2].number, r_output_force, yyvsp[0].word); ;
    break;}
case 34:
#line 295 "parse.y"
{ yyval.element.word = yyvsp[0].word; yyval.element.redirect = 0; ;
    break;}
case 35:
#line 297 "parse.y"
{ yyval.element.word = yyvsp[0].word; yyval.element.redirect = 0; ;
    break;}
case 36:
#line 299 "parse.y"
{ yyval.element.redirect = yyvsp[0].redirect; yyval.element.word = 0; ;
    break;}
case 37:
#line 303 "parse.y"
{
			  yyval.redirect = yyvsp[0].redirect;
			;
    break;}
case 38:
#line 307 "parse.y"
{ 
			  register REDIRECT *t = yyvsp[-1].redirect;

			  while (t->next)
			    t = t->next;
			  t->next = yyvsp[0].redirect; 
			  yyval.redirect = yyvsp[-1].redirect;
			;
    break;}
case 39:
#line 318 "parse.y"
{ yyval.command = make_simple_command (yyvsp[0].element, (COMMAND *)NULL); ;
    break;}
case 40:
#line 320 "parse.y"
{ yyval.command = make_simple_command (yyvsp[0].element, yyvsp[-1].command); ;
    break;}
case 41:
#line 324 "parse.y"
{ yyval.command = clean_simple_command (yyvsp[0].command); ;
    break;}
case 42:
#line 326 "parse.y"
{ yyval.command = yyvsp[0].command; ;
    break;}
case 43:
#line 330 "parse.y"
{ yyval.command = yyvsp[0].command; ;
    break;}
case 44:
#line 332 "parse.y"
{ yyvsp[-1].command->redirects = yyvsp[0].redirect; yyval.command = yyvsp[-1].command; ;
    break;}
case 45:
#line 336 "parse.y"
{ yyval.command = make_for_command (yyvsp[-4].word, add_string_to_list ("\"$@\"", (WORD_LIST *)NULL), yyvsp[-1].command); ;
    break;}
case 46:
#line 338 "parse.y"
{ yyval.command = make_for_command (yyvsp[-4].word, add_string_to_list ("$@", (WORD_LIST *)NULL), yyvsp[-1].command); ;
    break;}
case 47:
#line 340 "parse.y"
{ yyval.command = make_for_command (yyvsp[-5].word, add_string_to_list ("\"$@\"", (WORD_LIST *)NULL), yyvsp[-1].command); ;
    break;}
case 48:
#line 342 "parse.y"
{ yyval.command = make_for_command (yyvsp[-5].word, add_string_to_list ("\"$@\"", (WORD_LIST *)NULL), yyvsp[-1].command); ;
    break;}
case 49:
#line 345 "parse.y"
{ yyval.command = make_for_command (yyvsp[-8].word, (WORD_LIST *)reverse_list (yyvsp[-5].word_list), yyvsp[-1].command); ;
    break;}
case 50:
#line 347 "parse.y"
{ yyval.command = make_for_command (yyvsp[-8].word, (WORD_LIST *)reverse_list (yyvsp[-5].word_list), yyvsp[-1].command); ;
    break;}
case 51:
#line 350 "parse.y"
{ yyval.command = make_case_command (yyvsp[-4].word, (PATTERN_LIST *)NULL); ;
    break;}
case 52:
#line 352 "parse.y"
{ yyval.command = make_case_command (yyvsp[-5].word, yyvsp[-2].pattern); ;
    break;}
case 53:
#line 354 "parse.y"
{ /* Nobody likes this...
			     report_syntax_error ("Inserted `;;'"); */
			  yyval.command = make_case_command (yyvsp[-4].word, yyvsp[-1].pattern); ;
    break;}
case 54:
#line 359 "parse.y"
{ yyval.command = yyvsp[0].command; ;
    break;}
case 55:
#line 361 "parse.y"
{ yyval.command = make_while_command (yyvsp[-3].command, yyvsp[-1].command); ;
    break;}
case 56:
#line 363 "parse.y"
{ yyval.command = make_until_command (yyvsp[-3].command, yyvsp[-1].command); ;
    break;}
case 57:
#line 366 "parse.y"
{ yyvsp[-1].command->flags |= CMD_WANT_SUBSHELL; yyval.command = yyvsp[-1].command; ;
    break;}
case 58:
#line 369 "parse.y"
{ yyval.command = yyvsp[0].command; ;
    break;}
case 59:
#line 372 "parse.y"
{ yyval.command = yyvsp[0].command; ;
    break;}
case 60:
#line 376 "parse.y"
{ yyval.command = make_function_def (yyvsp[-4].word, yyvsp[0].command); ;
    break;}
case 61:
#line 379 "parse.y"
{ yyvsp[-1].command->redirects = yyvsp[0].redirect; yyval.command = make_function_def (yyvsp[-5].word, yyvsp[-1].command); ;
    break;}
case 62:
#line 382 "parse.y"
{ yyval.command = make_function_def (yyvsp[-4].word, yyvsp[0].command); ;
    break;}
case 63:
#line 385 "parse.y"
{ yyvsp[-1].command->redirects = yyvsp[0].redirect; yyval.command = make_function_def (yyvsp[-5].word, yyvsp[-1].command); ;
    break;}
case 64:
#line 388 "parse.y"
{ yyval.command = make_function_def (yyvsp[-2].word, yyvsp[0].command); ;
    break;}
case 65:
#line 391 "parse.y"
{ yyvsp[-1].command->redirects = yyvsp[0].redirect; yyval.command = make_function_def (yyvsp[-3].word, yyvsp[-1].command); ;
    break;}
case 66:
#line 395 "parse.y"
{ yyval.command = make_if_command (yyvsp[-3].command, yyvsp[-1].command, (COMMAND *)NULL); ;
    break;}
case 67:
#line 397 "parse.y"
{ yyval.command = make_if_command (yyvsp[-5].command, yyvsp[-3].command, yyvsp[-1].command); ;
    break;}
case 68:
#line 399 "parse.y"
{ yyval.command = make_if_command (yyvsp[-4].command, yyvsp[-2].command, yyvsp[-1].command); ;
    break;}
case 69:
#line 404 "parse.y"
{ yyval.command = make_group_command (yyvsp[-1].command); ;
    break;}
case 70:
#line 408 "parse.y"
{ yyval.command = make_if_command (yyvsp[-2].command, yyvsp[0].command, (COMMAND *)NULL); ;
    break;}
case 71:
#line 410 "parse.y"
{ yyval.command = make_if_command (yyvsp[-4].command, yyvsp[-2].command, yyvsp[0].command); ;
    break;}
case 72:
#line 412 "parse.y"
{ yyval.command = make_if_command (yyvsp[-3].command, yyvsp[-1].command, yyvsp[0].command); ;
    break;}
case 74:
#line 417 "parse.y"
{ yyvsp[0].pattern->next = yyvsp[-1].pattern; yyval.pattern = yyvsp[0].pattern; ;
    break;}
case 75:
#line 421 "parse.y"
{ yyval.pattern = make_pattern_list (yyvsp[-2].word_list, yyvsp[0].command); ;
    break;}
case 76:
#line 423 "parse.y"
{ yyval.pattern = make_pattern_list (yyvsp[-2].word_list, (COMMAND *)NULL); ;
    break;}
case 77:
#line 425 "parse.y"
{ yyval.pattern = make_pattern_list (yyvsp[-2].word_list, yyvsp[0].command); ;
    break;}
case 78:
#line 427 "parse.y"
{ yyval.pattern = make_pattern_list (yyvsp[-2].word_list, (COMMAND *)NULL); ;
    break;}
case 80:
#line 433 "parse.y"
{ yyvsp[0].pattern->next = yyvsp[-1].pattern; yyval.pattern = yyvsp[0].pattern; ;
    break;}
case 81:
#line 437 "parse.y"
{ yyval.pattern = make_pattern_list (yyvsp[-3].word_list, yyvsp[-1].command); ;
    break;}
case 82:
#line 439 "parse.y"
{ yyval.pattern = make_pattern_list (yyvsp[-3].word_list, (COMMAND *)NULL); ;
    break;}
case 83:
#line 441 "parse.y"
{ yyval.pattern = make_pattern_list (yyvsp[-3].word_list, yyvsp[-1].command); ;
    break;}
case 84:
#line 443 "parse.y"
{ yyval.pattern = make_pattern_list (yyvsp[-3].word_list, (COMMAND *)NULL); ;
    break;}
case 85:
#line 447 "parse.y"
{ yyval.word_list = make_word_list (yyvsp[0].word, (WORD_LIST *)NULL); ;
    break;}
case 86:
#line 449 "parse.y"
{ yyval.word_list = make_word_list (yyvsp[0].word, yyvsp[-2].word_list); ;
    break;}
case 87:
#line 458 "parse.y"
{
			  yyval.command = yyvsp[0].command;
			  if (need_here_doc)
			    gather_here_documents ();
			 ;
    break;}
case 90:
#line 468 "parse.y"
{ yyval.command = command_connect (yyvsp[-2].command, 0, '&'); ;
    break;}
case 92:
#line 474 "parse.y"
{ yyval.command = command_connect (yyvsp[-3].command, yyvsp[0].command, AND_AND); ;
    break;}
case 93:
#line 476 "parse.y"
{ yyval.command = command_connect (yyvsp[-3].command, yyvsp[0].command, OR_OR); ;
    break;}
case 94:
#line 478 "parse.y"
{
			  /* $1->flags |= CMD_FORCE_SUBSHELL; */
			  yyval.command = command_connect (yyvsp[-3].command, yyvsp[0].command, '&');
			;
    break;}
case 95:
#line 483 "parse.y"
{ yyval.command = command_connect (yyvsp[-3].command, yyvsp[0].command, ';'); ;
    break;}
case 96:
#line 485 "parse.y"
{ yyval.command = command_connect (yyvsp[-3].command, yyvsp[0].command, ';'); ;
    break;}
case 97:
#line 487 "parse.y"
{ yyval.command = yyvsp[0].command; ;
    break;}
case 98:
#line 489 "parse.y"
{
			  yyvsp[0].command->flags |= CMD_INVERT_RETURN;
			  yyval.command = yyvsp[0].command;
			;
    break;}
case 104:
#line 511 "parse.y"
{
			  yyval.command = yyvsp[0].command;
			  if (need_here_doc)
			    gather_here_documents ();
			;
    break;}
case 105:
#line 517 "parse.y"
{
			  /* $1->flags |= CMD_FORCE_SUBSHELL; */
			  yyval.command = command_connect (yyvsp[-1].command, (COMMAND *)NULL, '&');
			  if (need_here_doc)
			    gather_here_documents ();
			;
    break;}
case 106:
#line 524 "parse.y"
{
			  yyval.command = yyvsp[-1].command;
			  if (need_here_doc)
			    gather_here_documents ();
			;
    break;}
case 107:
#line 532 "parse.y"
{ yyval.command = command_connect (yyvsp[-3].command, yyvsp[0].command, AND_AND); ;
    break;}
case 108:
#line 534 "parse.y"
{ yyval.command = command_connect (yyvsp[-3].command, yyvsp[0].command, OR_OR); ;
    break;}
case 109:
#line 536 "parse.y"
{
			  /* $1->flags |= CMD_WANT_SUBSHELL; */
			  yyval.command = command_connect (yyvsp[-2].command, yyvsp[0].command, '&');
			;
    break;}
case 110:
#line 541 "parse.y"
{ yyval.command = command_connect (yyvsp[-2].command, yyvsp[0].command, ';'); ;
    break;}
case 111:
#line 543 "parse.y"
{ yyval.command = yyvsp[0].command; ;
    break;}
case 112:
#line 545 "parse.y"
{
			  yyvsp[0].command->flags |= CMD_INVERT_RETURN;
			  yyval.command = yyvsp[0].command;
			;
    break;}
case 113:
#line 553 "parse.y"
{ yyval.command = command_connect (yyvsp[-3].command, yyvsp[0].command, '|'); ;
    break;}
case 114:
#line 555 "parse.y"
{ yyval.command = yyvsp[0].command; ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 465 "/usr/local/lib/bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;
}
#line 557 "parse.y"


/* Initial size to allocate for tokens, and the
   amount to grow them by. */
#define TOKEN_DEFAULT_GROW_SIZE 512

/* The token currently being read. */
int current_token = 0;

/* The last read token, or NULL.  read_token () uses this for context
   checking. */
int last_read_token = 0;

/* The token read prior to last_read_token. */
int token_before_that = 0;

/* Global var is non-zero when end of file has been reached. */
int EOF_Reached = 0;

/* yy_getc () returns the next available character from input or EOF.
   yy_ungetc (c) makes `c' the next character to read.
   init_yy_io (get, unget, type, location) makes the function GET the
   installed function for getting the next character, makes UNGET the
   installed function for un-getting a character, sets the type of stream
   (either string or file) from TYPE, and makes LOCATION point to where
   the input is coming from. */

/* Unconditionally returns end-of-file. */
return_EOF ()
{
  return (EOF);
}

/* Variable containing the current get and unget functions.
   See ./input.h for a clearer description. */
BASH_INPUT bash_input;

/* Set all of the fields in BASH_INPUT to NULL. */
void
initialize_bash_input ()
{
  bash_input.type = 0;
  bash_input.name = (char *)NULL;
  bash_input.location.file = (FILE *)NULL;
  bash_input.location.string = (char *)NULL;
  bash_input.getter = (Function *)NULL;
  bash_input.ungetter = (Function *)NULL;
}

/* Set the contents of the current bash input stream from
   GET, UNGET, TYPE, NAME, and LOCATION. */
init_yy_io (get, unget, type, name, location)
     Function *get, *unget;
     int type;
     char *name;
     INPUT_STREAM location;
{
  bash_input.type = type;
  if (bash_input.name)
    free (bash_input.name);

  if (name)
    bash_input.name = savestring (name);
  else
    bash_input.name = (char *)NULL;

  bash_input.location = location;
  bash_input.getter = get;
  bash_input.ungetter = unget;
}

/* Call this to get the next character of input. */
yy_getc ()
{
  return (*(bash_input.getter)) ();
}

/* Call this to unget C.  That is, to make C the next character
   to be read. */
yy_ungetc (c)
{
  return (*(bash_input.ungetter)) (c);
}

#if defined (BUFFERED_INPUT)
int
input_file_descriptor ()
{
  switch (bash_input.type)
    {
    case st_stream:
      return (fileno (bash_input.location.file));
    case st_bstream:
      return (bash_input.location.buffered_fd);
    default:
      return (fileno (stdin));
    }
}
#endif /* BUFFERED_INPUT */

/* **************************************************************** */
/*								    */
/*		  Let input be read from readline ().		    */
/*								    */
/* **************************************************************** */

#if defined (READLINE)
char *current_readline_prompt = (char *)NULL;
char *current_readline_line = (char *)NULL;
int current_readline_line_index = 0;

static int readline_initialized_yet = 0;
int
yy_readline_get ()
{
  if (!current_readline_line)
    {
      extern sighandler sigint_sighandler ();
      extern int interrupt_immediately;
      extern char *readline ();
      SigHandler *old_sigint;
#if defined (JOB_CONTROL)
      extern pid_t shell_pgrp;
      extern int job_control;
#endif /* JOB_CONTROL */

      if (!readline_initialized_yet)
	{
	  initialize_readline ();
	  readline_initialized_yet = 1;
	}

#if defined (JOB_CONTROL)
      if (job_control)
	give_terminal_to (shell_pgrp);
#endif /* JOB_CONTROL */

      old_sigint =
	(SigHandler *) set_signal_handler (SIGINT, sigint_sighandler);
      interrupt_immediately++;

      if (!current_readline_prompt)
	current_readline_line = readline ("");
      else
	current_readline_line = readline (current_readline_prompt);

      interrupt_immediately--;
      set_signal_handler (SIGINT, old_sigint);

      /* Reset the prompt to whatever is in the decoded value of
	 prompt_string_pointer. */
      reset_readline_prompt ();

      current_readline_line_index = 0;

      if (!current_readline_line)
	{
	  current_readline_line_index = 0;
	  return (EOF);
	}

      current_readline_line =
	(char *)xrealloc (current_readline_line,
			  2 + strlen (current_readline_line));
      strcat (current_readline_line, "\n");
    }

  if (!current_readline_line[current_readline_line_index])
    {
      free (current_readline_line);
      current_readline_line = (char *)NULL;
      return (yy_readline_get ());
    }
  else
    {
      int c = current_readline_line[current_readline_line_index++];
      return (c);
    }
}

int
yy_readline_unget (c)
{
  if (current_readline_line_index && current_readline_line)
    current_readline_line[--current_readline_line_index] = c;
  return (c);
}
  
with_input_from_stdin ()
{
  INPUT_STREAM location;

  location.string = current_readline_line;
  init_yy_io (yy_readline_get, yy_readline_unget,
	      st_string, "readline stdin", location);
}

#else  /* !READLINE */

with_input_from_stdin ()
{
  with_input_from_stream (stdin, "stdin");
}
#endif	/* !READLINE */

/* **************************************************************** */
/*								    */
/*   Let input come from STRING.  STRING is zero terminated.	    */
/*								    */
/* **************************************************************** */

int
yy_string_get ()
{
  register char *string;
  register int c;

  string = bash_input.location.string;
  c = EOF;

  /* If the string doesn't exist, or is empty, EOF found. */
  if (string && *string)
    {
      c = *string++;
      bash_input.location.string = string;
    }
  return (c);
}

int
yy_string_unget (c)
     int c;
{
  *(--bash_input.location.string) = c;
  return (c);
}

void
with_input_from_string (string, name)
     char *string;
     char *name;
{
  INPUT_STREAM location;

  location.string = string;

  init_yy_io (yy_string_get, yy_string_unget, st_string, name, location);
}

/* **************************************************************** */
/*								    */
/*		     Let input come from STREAM.		    */
/*								    */
/* **************************************************************** */

int
yy_stream_get ()
{
  int result = EOF;

  if (bash_input.location.file)
#if defined (NO_READ_RESTART_ON_SIGNAL)
    result = getc_with_restart (bash_input.location.file);
#else
    result = getc (bash_input.location.file);
#endif /* !NO_READ_RESTART_ON_SIGNAL */
  return (result);
}

int
yy_stream_unget (c)
     int c;
{
  return (ungetc (c, bash_input.location.file));
}

with_input_from_stream (stream, name)
     FILE *stream;
     char *name;
{
  INPUT_STREAM location;

  location.file = stream;
  init_yy_io (yy_stream_get, yy_stream_unget, st_stream, name, location);
}

typedef struct stream_saver {
  struct stream_saver *next;
  BASH_INPUT bash_input;
  int line;
#if defined (BUFFERED_INPUT)
  BUFFERED_STREAM *bstream;
#endif /* BUFFERED_INPUT */
} STREAM_SAVER;

/* The globally known line number. */
int line_number = 0;

STREAM_SAVER *stream_list = (STREAM_SAVER *)NULL;

push_stream ()
{
  STREAM_SAVER *saver = (STREAM_SAVER *)xmalloc (sizeof (STREAM_SAVER));

  bcopy (&bash_input, &(saver->bash_input), sizeof (BASH_INPUT));

#if defined (BUFFERED_INPUT)
  saver->bstream = (BUFFERED_STREAM *)NULL;
  /* If we have a buffered stream, clear out buffers[fd]. */
  if (bash_input.type == st_bstream)
    {
      saver->bstream = buffers[bash_input.location.buffered_fd];
      buffers[bash_input.location.buffered_fd] = (BUFFERED_STREAM *)NULL;
    }
#endif /* BUFFERED_INPUT */

  saver->line = line_number;
  bash_input.name = (char *)NULL;
  saver->next = stream_list;
  stream_list = saver;
  EOF_Reached = line_number = 0;
}

pop_stream ()
{
  if (!stream_list)
    {
      EOF_Reached = 1;
    }
  else
    {
      STREAM_SAVER *saver = stream_list;

      EOF_Reached = 0;
      stream_list = stream_list->next;

      init_yy_io (saver->bash_input.getter,
		  saver->bash_input.ungetter,
		  saver->bash_input.type,
		  saver->bash_input.name,
		  saver->bash_input.location);

#if defined (BUFFERED_INPUT)
      /* If we have a buffered stream, restore buffers[fd]. */
      if (bash_input.type == st_bstream)
        buffers[bash_input.location.buffered_fd] = saver->bstream;
#endif /* BUFFERED_INPUT */

      line_number = saver->line;

      if (saver->bash_input.name)
	free (saver->bash_input.name);
      free (saver);
    }
}

/*
 * This is used to inhibit alias expansion and reserved word recognition
 * inside case statement pattern lists.  A `case statement pattern list'
 * is:
 *	everything between the `in' in a `case word in' and the next ')'
 *	or `esac'
 *	everything between a `;;' and the next `)' or `esac'
 */
static int in_case_pattern_list = 0;

#if defined (ALIAS)
/*
 * Pseudo-global variables used in implementing token-wise alias expansion.
 */

static int expand_next_token = 0;

/*
 * Pushing and popping strings.  This works together with shell_getc to 
 * implement alias expansion on a per-token basis.
 */

typedef struct string_saver {
  struct string_saver *next;
  int expand_alias;  /* Value to set expand_alias to when string is popped. */
  char *saved_line;
  int saved_line_size, saved_line_index, saved_line_terminator;
} STRING_SAVER;

STRING_SAVER *pushed_string_list = (STRING_SAVER *)NULL;

static void save_expansion ();

/*
 * Push the current shell_input_line onto a stack of such lines and make S
 * the current input.  Used when expanding aliases.  EXPAND is used to set
 * the value of expand_next_token when the string is popped, so that the
 * word after the alias in the original line is handled correctly when the
 * alias expands to multiple words.  TOKEN is the token that was expanded
 * into S; it is saved and used to prevent infinite recursive expansion.
 */
static void
push_string (s, expand, token)
     char *s;
     int expand;
     char *token;
{
  extern char *shell_input_line;
  extern int shell_input_line_size, shell_input_line_index,
	     shell_input_line_terminator;
  STRING_SAVER *temp = (STRING_SAVER *) xmalloc (sizeof (STRING_SAVER));

  temp->expand_alias = expand;
  temp->saved_line = shell_input_line;
  temp->saved_line_size = shell_input_line_size;
  temp->saved_line_index = shell_input_line_index;
  temp->saved_line_terminator = shell_input_line_terminator;
  temp->next = pushed_string_list;
  pushed_string_list = temp;

  save_expansion (token);

  shell_input_line = s;
  shell_input_line_size = strlen (s);
  shell_input_line_index = 0;
  shell_input_line_terminator = '\0';
  expand_next_token = 0;
}

/*
 * Make the top of the pushed_string stack be the current shell input.
 * Only called when there is something on the stack.  Called from shell_getc
 * when it thinks it has consumed the string generated by an alias expansion
 * and needs to return to the original input line.
 */
static void
pop_string ()
{
  extern char *shell_input_line;
  extern int shell_input_line_size, shell_input_line_index,
	     shell_input_line_terminator;
  STRING_SAVER *t;

  if (shell_input_line)
    free (shell_input_line);
  shell_input_line = pushed_string_list->saved_line;
  shell_input_line_index = pushed_string_list->saved_line_index;
  shell_input_line_size = pushed_string_list->saved_line_size;
  shell_input_line_terminator = pushed_string_list->saved_line_terminator;
  expand_next_token = pushed_string_list->expand_alias;

  t = pushed_string_list;
  pushed_string_list = pushed_string_list->next;
  free((char *)t);
}

static void
free_string_list ()
{
  register STRING_SAVER *t = pushed_string_list, *t1;

  while (t)
    {
      t1 = t->next;
      if (t->saved_line)
	free (t->saved_line);
      free ((char *)t);
      t = t1;
    }
  pushed_string_list = (STRING_SAVER *)NULL;
}

/* This is a stack to save the values of all tokens for which alias
   expansion has been performed during the current call to read_token ().
   It is used to prevent alias expansion loops:

      alias foo=bar
      alias bar=baz
      alias baz=foo

   Ideally this would be taken care of by push and pop string, but because
   of when strings are popped the stack will not contain the correct
   strings to test against.  (The popping is done in shell_getc, so that when
   the current string is exhausted, shell_getc can simply pop that string off
   the stack, restore the previous string, and continue with the character
   following the token whose expansion was originally pushed on the stack.)

   What we really want is a record of all tokens that have been expanded for
   aliases during the `current' call to read_token().  This does that, at the
   cost of being somewhat special-purpose (OK, OK vile and unclean). */

typedef struct _exp_saver {
      struct _exp_saver *next;
      char *saved_token;
} EXPANSION_SAVER;

EXPANSION_SAVER *expanded_token_stack = (EXPANSION_SAVER *)NULL;

static void
save_expansion (s)
     char *s;
{
  EXPANSION_SAVER *t;

  t = (EXPANSION_SAVER *) xmalloc (sizeof (EXPANSION_SAVER));
  t->saved_token = savestring (s);
  t->next = expanded_token_stack;
  expanded_token_stack = t;
}

/* Return 1 if TOKEN has already been expanded in the current `stack' of
   expansions.  If it has been expanded already, it will appear as the value
   of saved_token for some entry in the stack of expansions created for the
   current token being expanded. */
static int
token_has_been_expanded (token)
     char *token;
{
  register EXPANSION_SAVER *t = expanded_token_stack;

  while (t)
    {
      if (STREQ (token, t->saved_token))
	return (1);
      t = t->next;
    }
  return (0);
}

static void
free_expansion_stack ()
{
  register EXPANSION_SAVER *t = expanded_token_stack, *t1;

  while (t)
    {
      t1 = t->next;
      free (t->saved_token);
      free (t);
      t = t1;
    }
  expanded_token_stack = (EXPANSION_SAVER *)NULL;
}

#endif /* ALIAS */


#if defined (index)
#  undef index
#endif /* index */

/* Return a line of text, taken from wherever yylex () reads input.
   If there is no more input, then we return NULL.  If REMOVE_QUOTED_NEWLINE
   is non-zero, we remove unquoted \<newline> pairs.  This is used by
   read_secondary_line to read here documents. */
static char *
read_a_line (remove_quoted_newline)
     int remove_quoted_newline;
{
  char *line_buffer = (char *)NULL;
  int index = 0, buffer_size = 0;
  int c, peekc, pass_next;

  pass_next = 0;
  while (1)
    {
      c = yy_getc ();

      /* Allow immediate exit if interrupted during input. */
      QUIT;

      if (c == 0)
	continue;

      /* If there is no more input, then we return NULL. */
      if (c == EOF)
	{
	  c = '\n';
	  if (!line_buffer)
	    return ((char *)NULL);
	}

      /* `+2' in case the final (200'th) character in the buffer is a newline;
	 otherwise the code below that NULL-terminates it will write over the
	 201st slot and kill the range checking in free(). */
      if (index + 2 > buffer_size)
	if (!buffer_size)
	  line_buffer = (char *)xmalloc (buffer_size = 200);
	else
	  line_buffer = (char *)xrealloc (line_buffer, buffer_size += 200);

      /* IF REMOVE_QUOTED_NEWLINES is non-zero, we are reading a
	 here document with an unquoted delimiter.  In this case,
	 the line will be expanded as if it were in double quotes.
	 We allow a backslash to escape the next character, but we
	 need to treat the backslash specially only if a backslash
	 quoting a backslash-newline pair appears in the line. */
      if (pass_next)
        {
	  line_buffer[index++] = c;
	  pass_next = 0;
        }
      else if (c == '\\' && remove_quoted_newline)
	{
	  peekc = yy_getc ();
	  if (peekc == '\n')
	    continue;	/* Make the unquoted \<newline> pair disappear. */
	  else
	    {
	      yy_ungetc (peekc);
	      pass_next = 1;
	      line_buffer[index++] = c;		/* Preserve the backslash. */
	    }
	}
      else
	line_buffer[index++] = c;

      if (c == '\n')
	{
	  line_buffer[index] = '\0';
	  return (line_buffer);
	}
    }
}

/* Return a line as in read_a_line (), but insure that the prompt is
   the secondary prompt.  This is used to read the lines of a here
   document.  REMOVE_QUOTED_NEWLINE is non-zero if we should remove
   newlines quoted with backslashes while reading the line.  It is
   non-zero unless the delimiter of the here document was quoted. */
char *
read_secondary_line (remove_quoted_newline)
     int remove_quoted_newline;
{
  prompt_string_pointer = &ps2_prompt;
  prompt_again ();
  return (read_a_line (remove_quoted_newline));
}


/* **************************************************************** */
/*								    */
/*				YYLEX ()			    */
/*								    */
/* **************************************************************** */

/* Reserved words.  These are only recognized as the first word of a
   command. */
STRING_INT_ALIST word_token_alist[] = {
  { "if", IF },
  { "then", THEN },
  { "else", ELSE },
  { "elif", ELIF },
  { "fi", FI },
  { "case", CASE },
  { "esac", ESAC },
  { "for", FOR },
  { "while", WHILE },
  { "until", UNTIL },
  { "do", DO },
  { "done", DONE },
  { "in", IN },
  { "function", FUNCTION },
  { "{", '{' },
  { "}", '}' },
  { "!", BANG },
  { (char *)NULL, 0}
};

/* Where shell input comes from.  History expansion is performed on each
   line when the shell is interactive. */
char *shell_input_line = (char *)NULL;
int shell_input_line_index = 0;
int shell_input_line_size = 0;	/* Amount allocated for shell_input_line. */
int shell_input_line_len = 0;	/* strlen (shell_input_line) */

/* Either zero, or EOF. */
int shell_input_line_terminator = 0;

/* Return the next shell input character.  This always reads characters
   from shell_input_line; when that line is exhausted, it is time to
   read the next line.  This is called by read_token when the shell is
   processing normal command input. */
static int
shell_getc (remove_quoted_newline)
     int remove_quoted_newline;
{
  int c;

  QUIT;

#if defined (ALIAS)
  /* If shell_input_line[shell_input_line_index] == 0, but there is
     something on the pushed list of strings, then we don't want to go
     off and get another line.  We let the code down below handle it. */

  if (!shell_input_line || ((!shell_input_line[shell_input_line_index]) &&
			    (pushed_string_list == (STRING_SAVER *)NULL)))
#else /* !ALIAS */
  if (!shell_input_line || !shell_input_line[shell_input_line_index])
#endif /* !ALIAS */
    {
      register int i, l;

      restart_read_next_line:

      line_number++;

    restart_read:

      /* Allow immediate exit if interrupted during input. */
      QUIT;

      i = 0;
      shell_input_line_terminator = 0;

#if defined (JOB_CONTROL)
      /* This can cause a problem when reading a command as the result
	 of a trap, when the trap is called from flush_child.  This call
	 had better not cause jobs to disappear from the job table in
	 that case, or we will have big trouble. */
      notify_and_cleanup ();
#else /* !JOB_CONTROL */
      cleanup_dead_jobs ();
#endif /* !JOB_CONTROL */

      if (bash_input.type == st_stream)
	clearerr (stdin);

      while (c = yy_getc ())
	{
	  /* Allow immediate exit if interrupted during input. */
	  QUIT;

	  if (i + 2 > shell_input_line_size)
	    shell_input_line = (char *)
	      xrealloc (shell_input_line, shell_input_line_size += 256);

	  if (c == EOF)
	    {
	      if (bash_input.type == st_stream)
		clearerr (stdin);

	      if (!i)
		shell_input_line_terminator = EOF;

	      shell_input_line[i] = '\0';
	      break;
	    }

	  shell_input_line[i++] = c;

	  if (c == '\n')
	    {
	      shell_input_line[--i] = '\0';
	      current_command_line_count++;
	      break;
	    }
	}
      shell_input_line_index = 0;
      shell_input_line_len = i;		/* == strlen (shell_input_line) */

#if defined (HISTORY)
      if (interactive && shell_input_line && shell_input_line[0])
	{
	  char *pre_process_line (), *expansions;

	  expansions = pre_process_line (shell_input_line, 1, 1);

	  free (shell_input_line);
	  shell_input_line = expansions;
	  shell_input_line_len = shell_input_line ?
				 strlen (shell_input_line) :
				 0;
	  if (!shell_input_line_len)
	    current_command_line_count--;

	  /* We have to force the xrealloc below because we don't know the
	     true allocated size of shell_input_line anymore. */
	  shell_input_line_size = shell_input_line_len;
	}
#endif /* HISTORY */

      if (shell_input_line)
	{
	  /* Lines that signify the end of the shell's input should not be
	     echoed. */
	  if (echo_input_at_read && (shell_input_line[0] ||
				     shell_input_line_terminator != EOF))
	    fprintf (stderr, "%s\n", shell_input_line);
	}
      else
	{
	  shell_input_line_size = 0;
	  prompt_string_pointer = &current_prompt_string;
	  prompt_again ();
	  goto restart_read;
	}

      /* Add the newline to the end of this string, iff the string does
	 not already end in an EOF character.  */
      if (shell_input_line_terminator != EOF)
	{
	  l = shell_input_line_len;	/* was a call to strlen */

	  if (l + 3 > shell_input_line_size)
	    shell_input_line = (char *)xrealloc (shell_input_line,
					1 + (shell_input_line_size += 2));

	  shell_input_line[l] = '\n';
	  shell_input_line[l + 1] = '\0';
	}
    }
  
  c = shell_input_line[shell_input_line_index];

  if (c)
    shell_input_line_index++;

  if (c == '\\' && remove_quoted_newline &&
      shell_input_line[shell_input_line_index] == '\n')
    {
	prompt_again ();
	goto restart_read_next_line;
    }

#if defined (ALIAS)
  /* If C is NULL, we have reached the end of the current input string.  If
     pushed_string_list is non-empty, it's time to pop to the previous string
     because we have fully consumed the result of the last alias expansion.
     Do it transparently; just return the next character of the string popped
     to. */
  if (!c && (pushed_string_list != (STRING_SAVER *)NULL))
    {
      pop_string ();
      c = shell_input_line[shell_input_line_index];
      if (c)
	shell_input_line_index++;
    }
#endif /* ALIAS */

  if (!c && shell_input_line_terminator == EOF)
    {
      if (shell_input_line_index != 0)
	return ('\n');
      else
	return (EOF);
    }

  return (c);
}

/* Put C back into the input for the shell. */
static void
shell_ungetc (c)
     int c;
{
  if (shell_input_line && shell_input_line_index)
    shell_input_line[--shell_input_line_index] = c;
}

/* Discard input until CHARACTER is seen. */
static void
discard_until (character)
     int character;
{
  int c;

  while ((c = shell_getc (0)) != EOF && c != character)
    ;

  if (c != EOF)
    shell_ungetc (c);
}

#if defined (HISTORY) && defined (HISTORY_REEDITING)
/* Tell readline () that we have some text for it to edit. */
re_edit (text)
     char *text;
{
#if defined (READLINE)
  if (strcmp (bash_input.name, "readline stdin") == 0)
    bash_re_edit (text);
#endif /* READLINE */
}
#endif /* HISTORY && HISTORY_REEDITING */

#if defined (HISTORY)
extern int remember_on_history, history_expansion;
extern int history_control, command_oriented_history;
extern int history_expand ();
extern int history_base, where_history ();

/* Non-zero means do no history expansion on this line, regardless
   of what history_expansion says. */
int history_expansion_inhibited = 0;

/* Do pre-processing on LINE.  If PRINT_CHANGES is non-zero, then
   print the results of expanding the line if there were any changes.
   If there is an error, return NULL, otherwise the expanded line is
   returned.  If ADDIT is non-zero the line is added to the history
   list after history expansion.  ADDIT is just a suggestion;
   REMEMBER_ON_HISTORY can veto, and does.
   Right now this does history expansion. */
char *
pre_process_line (line, print_changes, addit)
     char *line;
     int print_changes, addit;
{
  char *history_value;
  char *return_value;
  int expanded = 0;

  return_value = line;

#  if defined (BANG_HISTORY)
  /* History expand the line.  If this results in no errors, then
     add that line to the history if ADDIT is non-zero. */
  if (!history_expansion_inhibited && history_expansion)
    {
      expanded = history_expand (line, &history_value);

      if (expanded)
	{
	  if (print_changes)
	    fprintf (stderr, "%s\n", history_value);

	  /* If there was an error, return NULL. */
	  if (expanded < 0)
	    {
	      free (history_value);

#    if defined (HISTORY_REEDITING)
	      /* New hack.  We can allow the user to edit the
		 failed history expansion. */
	      re_edit (line);
#    endif /* HISTORY_REEDITING */
	      return ((char *)NULL);
	    }
	}

      /* Let other expansions know that return_value can be free'ed,
	 and that a line has been added to the history list.  Note
	 that we only add lines that have something in them. */
      expanded = 1;
      return_value = history_value;
    }
#  endif /* BANG_HISTORY */

  if (addit && remember_on_history && *return_value)
    {
      int h;

      /* Don't use the value of history_control to affect the second
	 and subsequent lines of a multi-line command when
	 command_oriented_history is enabled. */
      if (command_oriented_history && current_command_line_count > 1)
	h = 0;
      else
	h = history_control;

      switch (h)
	{
	  case 0:
	    bash_add_history (return_value);
	    break;
	  case 1:
	    if (*return_value != ' ')
	      bash_add_history (return_value);
	    break;
	  case 3:
	    if (*return_value == ' ')
	      break;
	    /* FALLTHROUGH if case == 3 (`ignoreboth') */
	  case 2:
	    {
	      HIST_ENTRY *temp;

	      using_history ();
	      temp = previous_history ();

	      if (!temp || (strcmp (temp->line, return_value) != 0))
		bash_add_history (return_value);

	      using_history ();
	    }
	    break;
	}
    }

  if (!expanded)
    return_value = savestring (line);

  return (return_value);
}
#endif /* HISTORY */

/* Place to remember the token.  We try to keep the buffer
   at a reasonable size, but it can grow. */
char *token = (char *)NULL;

/* Current size of the token buffer. */
int token_buffer_size = 0;

static void
execute_prompt_command (command)
     char *command;
{
  extern Function *last_shell_builtin, *this_shell_builtin;
  Function *temp_last, *temp_this;
  char *last_lastarg;
  int temp_exit_value, temp_eof_encountered;

  temp_last = last_shell_builtin;
  temp_this = this_shell_builtin;
  temp_exit_value = last_command_exit_value;
  temp_eof_encountered = eof_encountered;
  last_lastarg = get_string_value ("_");
  if (last_lastarg)
    last_lastarg = savestring (last_lastarg);

  parse_and_execute (savestring (command), "PROMPT_COMMAND");

  last_shell_builtin = temp_last;
  this_shell_builtin = temp_this;
  last_command_exit_value = temp_exit_value;
  eof_encountered = temp_eof_encountered;

  bind_variable ("_", last_lastarg);
  if (last_lastarg)
    free (last_lastarg);
}

/* Command to read_token () explaining what we want it to do. */
#define READ 0
#define RESET 1
#define prompt_is_ps1 \
      (!prompt_string_pointer || prompt_string_pointer == &ps1_prompt)

/* Function for yyparse to call.  yylex keeps track of
   the last two tokens read, and calls read_token.  */

yylex ()
{
  if (interactive && (!current_token || current_token == '\n'))
    {
      /* Before we print a prompt, we might have to check mailboxes.
	 We do this only if it is time to do so. Notice that only here
	 is the mail alarm reset; nothing takes place in check_mail ()
	 except the checking of mail.  Please don't change this. */
      if (prompt_is_ps1 && time_to_check_mail ())
	{
	  check_mail ();
	  reset_mail_timer ();
	}

      /* Allow the execution of a random command just before the printing
	 of each primary prompt.  If the shell variable PROMPT_COMMAND
	 is set then the value of it is the command to execute. */
      if (prompt_is_ps1)
	{
	  char *command_to_execute;

	  command_to_execute = get_string_value ("PROMPT_COMMAND");
	  if (command_to_execute)
	    execute_prompt_command (command_to_execute);
	}
      prompt_again ();
    }

  token_before_that = last_read_token;
  last_read_token = current_token;
  current_token = read_token (READ);
  return (current_token);
}

/* Called from shell.c when Control-C is typed at top level.  Or
   by the error rule at top level. */
reset_parser ()
{
  read_token (RESET);
}
  
/* When non-zero, we have read the required tokens
   which allow ESAC to be the next one read. */
static int allow_esac_as_next = 0;

/* When non-zero, accept single '{' as a token itself. */
static int allow_open_brace = 0;

/* DELIMITERS is a stack of the nested delimiters that we have
   encountered so far. */
static char *delimiters = (char *)NULL;

/* Offset into the stack of delimiters. */
static int delimiter_depth = 0;

/* How many slots are allocated to DELIMITERS. */
static int delimiter_space = 0;

void
gather_here_documents ()
{
  int r = 0;
  while (need_here_doc)
    {
      make_here_document (redir_stack[r++]);
      need_here_doc--;
    }
}

/* Macro for accessing the top delimiter on the stack.  Returns the
   delimiter or zero if none. */
#define current_delimiter() \
  (delimiter_depth ? delimiters[delimiter_depth - 1] : 0)

#define push_delimiter(character) \
  do \
    { \
      if (delimiter_depth + 2 > delimiter_space) \
	delimiters = (char *)xrealloc \
	  (delimiters, (delimiter_space += 10) * sizeof (char)); \
      delimiters[delimiter_depth] = character; \
      delimiter_depth++; \
    } \
  while (0)

/* When non-zero, an open-brace used to create a group is awaiting a close
   brace partner. */
static int open_brace_awaiting_satisfaction = 0;

/* If non-zero, it is the token that we want read_token to return regardless
   of what text is (or isn't) present to be read.  read_token resets this. */
int token_to_read = 0;

#define command_token_position(token) \
  (((token) == ASSIGNMENT_WORD) || \
   ((token) != SEMI_SEMI && reserved_word_acceptable(token)))

#define assignment_acceptable(token) command_token_position(token) && \
					(in_case_pattern_list == 0)

/* Check to see if TOKEN is a reserved word and return the token
   value if it is. */
#define CHECK_FOR_RESERVED_WORD(token) \
  do { \
    if (!dollar_present && !quoted && \
	reserved_word_acceptable (last_read_token)) \
      { \
	int i; \
	for (i = 0; word_token_alist[i].word != (char *)NULL; i++) \
	  if (STREQ (token, word_token_alist[i].word)) \
	    { \
	      if (in_case_pattern_list && (word_token_alist[i].token != ESAC)) \
		break; \
\
	      if (word_token_alist[i].token == ESAC) \
		in_case_pattern_list = 0; \
\
	      if (word_token_alist[i].token == '{') \
		open_brace_awaiting_satisfaction++; \
\
	      return (word_token_alist[i].token); \
	    } \
      } \
  } while (0)

/* Read the next token.  Command can be READ (normal operation) or 
   RESET (to normalize state). */
static int
read_token (command)
     int command;
{
  int character;		/* Current character. */
  int peek_char;		/* Temporary look-ahead character. */
  int result;			/* The thing to return. */
  WORD_DESC *the_word;		/* The value for YYLVAL when a WORD is read. */

  if (token_buffer_size < TOKEN_DEFAULT_GROW_SIZE)
    {
      if (token)
	free (token);
      token = (char *)xmalloc (token_buffer_size = TOKEN_DEFAULT_GROW_SIZE);
    }

  if (command == RESET)
    {
      delimiter_depth = 0;	/* No delimiters found so far. */
      open_brace_awaiting_satisfaction = 0;
      in_case_pattern_list = 0;

#if defined (ALIAS)
      if (pushed_string_list)
	{
	  free_string_list ();
	  pushed_string_list = (STRING_SAVER *)NULL;
	}

      if (expanded_token_stack)
	{
	  free_expansion_stack ();
	  expanded_token_stack = (EXPANSION_SAVER *)NULL;
	}

      expand_next_token = 0;
#endif /* ALIAS */

      if (shell_input_line)
	{
	  free (shell_input_line);
	  shell_input_line = (char *)NULL;
	  shell_input_line_size = shell_input_line_index = 0;
	}
      last_read_token = '\n';
      token_to_read = '\n';
      return ('\n');
    }

  if (token_to_read)
    {
      int rt = token_to_read;
      token_to_read = 0;
      return (rt);
    }

#if defined (ALIAS)
  /* If we hit read_token () and there are no saved strings on the
     pushed_string_list, then we are no longer currently expanding a
     token.  This can't be done in pop_stream, because pop_stream
     may pop the stream before the current token has finished being
     completely expanded (consider what happens when we alias foo to foo,
     and then try to expand it). */
  if (!pushed_string_list && expanded_token_stack)
    {
      free_expansion_stack ();
      expanded_token_stack = (EXPANSION_SAVER *)NULL;
    }

  /* This is a place to jump back to once we have successfully expanded a
     token with an alias and pushed the string with push_string () */
 re_read_token:

#endif /* ALIAS */

  /* Read a single word from input.  Start by skipping blanks. */
  while ((character = shell_getc (1)) != EOF && whitespace (character));

  if (character == EOF)
    {
      EOF_Reached = 1;
      return (yacc_EOF);
    }

  if (character == '#' && (!interactive || interactive_comments))
    {
      /* A comment.  Discard until EOL or EOF, and then return a newline. */
      discard_until ('\n');
      shell_getc (0);

      /* If we're about to return an unquoted newline, we can go and collect
	 the text of any pending here documents. */
      if (need_here_doc)
        gather_here_documents ();

#if defined (ALIAS)
      expand_next_token = 0;
#endif /* ALIAS */

      return ('\n');
    }

  if (character == '\n')
    {
      /* If we're about to return an unquoted newline, we can go and collect
	 the text of any pending here document. */
      if (need_here_doc)
	gather_here_documents ();

#if defined (ALIAS)
      expand_next_token = 0;
#endif /* ALIAS */

      return (character);
    }

  if (member (character, "()<>;&|"))
    {
#if defined (ALIAS)
      /* Turn off alias tokenization iff this character sequence would
	 not leave us ready to read a command. */
      if (character == '<' || character == '>')
	expand_next_token = 0;
#endif /* ALIAS */

      /* Please note that the shell does not allow whitespace to
	 appear in between tokens which are character pairs, such as
	 "<<" or ">>".  I believe this is the correct behaviour. */
      if (character == (peek_char = shell_getc (1)))
	{
	  switch (character)
	    {
	      /* If '<' then we could be at "<<" or at "<<-".  We have to
		 look ahead one more character. */
	    case '<':
	      peek_char = shell_getc (1);
	      if (peek_char == '-')
		return (LESS_LESS_MINUS);
	      else
		{
		  shell_ungetc (peek_char);
		  return (LESS_LESS);
		}

	    case '>':
	      return (GREATER_GREATER);

	    case ';':
	      in_case_pattern_list = 1;
#if defined (ALIAS)
	      expand_next_token = 0;
#endif /* ALIAS */
	      return (SEMI_SEMI);

	    case '&':
	      return (AND_AND);

	    case '|':
	      return (OR_OR);
	    }
	}
      else
	{
	  if (peek_char == '&')
	    {
	      switch (character)
		{
		case '<': return (LESS_AND);
		case '>': return (GREATER_AND);
		}
	    }
	  if (character == '<' && peek_char == '>')
	    return (LESS_GREATER);
	  if (character == '>' && peek_char == '|')
	    return (GREATER_BAR);
	  if (peek_char == '>' && character == '&')
	    return (AND_GREATER);
	}
      shell_ungetc (peek_char);

      /* If we look like we are reading the start of a function
	 definition, then let the reader know about it so that
	 we will do the right thing with `{'. */
      if (character == ')' &&
	  last_read_token == '(' && token_before_that == WORD)
	{
	  allow_open_brace = 1;
#if defined (ALIAS)
	  expand_next_token = 0;
#endif /* ALIAS */
	}

      if (in_case_pattern_list && (character == ')'))
	in_case_pattern_list = 0;

#if defined (PROCESS_SUBSTITUTION)
      /* Check for the constructs which introduce process substitution. */
      if (!((character == '>' || character == '<') && peek_char == '('))
#endif /* PROCESS_SUBSTITUTION */
	return (character);
    }

  /* Hack <&- (close stdin) case. */
  if (character == '-')
    {
      switch (last_read_token)
	{
	case LESS_AND:
	case GREATER_AND:
	  return (character);
	}
    }
  
  /* Okay, if we got this far, we have to read a word.  Read one,
     and then check it against the known ones. */
  {
    /* Index into the token that we are building. */
    int token_index = 0;

    /* ALL_DIGITS becomes zero when we see a non-digit. */
    int all_digits = digit (character);

    /* DOLLAR_PRESENT becomes non-zero if we see a `$'. */
    int dollar_present = 0;

    /* QUOTED becomes non-zero if we see one of ("), ('), (`), or (\). */
    int quoted = 0;

    /* Non-zero means to ignore the value of the next character, and just
       to add it no matter what. */
    int pass_next_character = 0;

    /* Non-zero means parsing a dollar-paren construct.  It is the count of
       un-quoted closes we need to see. */
    int dollar_paren_level = 0;

    /* Non-zero means parsing a dollar-bracket construct ($[...]).  It is
       the count of un-quoted `]' characters we need to see. */
    int dollar_bracket_level = 0;

    /* Non-zero means parsing a `${' construct.  It is the count of
       un-quoted `}' we need to see. */
    int dollar_brace_level = 0;

    /* A level variable for parsing '${ ... }' constructs inside of double
       quotes. */
    int delimited_brace_level = 0;

    /* A boolean variable denoting whether or not we are currently parsing
       a double-quoted string embedded in a $( ) or ${ } construct. */
    int embedded_quoted_string = 0;

    /* Another level variable.  This one is for dollar_parens inside of
       double-quotes. */
    int delimited_paren_level = 0;

    for (;;)
      {
	if (character == EOF)
	  goto got_token;

	if (pass_next_character)
	  {
	    pass_next_character = 0;
	    goto got_character;
	  }

	if (current_delimiter () &&
	    (character == '\\') &&
	    (current_delimiter () != '\''))
	  {
	    peek_char = shell_getc (0);
	    if (peek_char != '\\')
	      shell_ungetc (peek_char);
	    else
	      {
		token[token_index++] = character;
		goto got_character;
	      }
	  }

	/* Handle backslashes.  Quote lots of things when not inside of
	   double-quotes, quote some things inside of double-quotes. */
	   
	if (character == '\\' &&
	    (!delimiter_depth ||
	     (current_delimiter () != '\'')))
	  {
	    peek_char = shell_getc (0);

	    /* Backslash-newline is ignored in all cases excepting
	       when quoted with single quotes. */
	    if (peek_char == '\n')
	      {
		character = '\n';
		goto next_character;
	      }
	    else
	      {
		char delim = current_delimiter ();

		shell_ungetc (peek_char);

		/* If the next character is to be quoted, do it now. */
		if (!delim || (delim == '`') ||
		    ((delim == '"') &&
		     (member (peek_char, slashify_in_quotes))))
		  {
		    pass_next_character++;
		    quoted = 1;
		    goto got_character;
		  }
	      }
	  }

	/* This is a hack, in its present form.  If a backquote substitution
	   appears within double quotes, everything within the backquotes
	   should be read as part of a single word.  Jesus.  Now I see why
	   Korn introduced the $() form. */
	if (delimiter_depth &&
	    (current_delimiter () == '"') && (character == '`'))
	  {
	    push_delimiter (character);
	    goto got_character;
	  }

	if (delimiter_depth)
	  {
	    if (character == current_delimiter ())
	      {
	      	/* If we see a double quote while parsing a double-quoted
		  $( ) or ${ }, and we have not seen ) or }, respectively,
	      	   note that we are in the middle of reading an embedded
		   quoted string. */
		if ((delimited_paren_level || delimited_brace_level) &&
		    (character == '"'))
		  {
		    embedded_quoted_string = !embedded_quoted_string;
		    goto got_character;
		  }
		
		delimiter_depth--;
		goto got_character;
	      }
	  }

	if (current_delimiter () != '\'')
	  {
#if defined (PROCESS_SUBSTITUTION)
	    if (character == '$' || character == '<' || character == '>')
#else
	    if (character == '$')
#endif /* !PROCESS_SUBSTITUTION */
	      {
	      	/* If we're in the middle of parsing a $( ) or ${ }
	      	   construct with an embedded quoted string, don't
	      	   bother looking at this character any further. */
	      	if (embedded_quoted_string)
	      	  goto got_character;

		peek_char = shell_getc (1);
		shell_ungetc (peek_char);
		if (peek_char == '(')
		  {
		    if (!delimiter_depth)
		      dollar_paren_level++;
		    else
		      delimited_paren_level++;

		    pass_next_character++;
		    goto got_character;
		  }
		else if (peek_char == '[' && character == '$')
		  {
		    if (!delimiter_depth)
		      dollar_bracket_level++;

		    pass_next_character++;
		    goto got_character;
		  }
		/* This handles ${...} constructs. */
		else if (peek_char == '{' && character == '$')
		  {
		    if (!delimiter_depth)
		      dollar_brace_level++;
		    else
		      delimited_brace_level++;

		    pass_next_character++;
		    goto got_character;
		  }
	      }

	    /* If we are parsing a $() or $[] construct, we need to balance
	       parens and brackets inside the construct.  This whole function
	       could use a rewrite. */
	    if (character == '(' && !embedded_quoted_string)
	      {
		if (delimiter_depth && delimited_paren_level)
		  delimited_paren_level++;

		if (!delimiter_depth && dollar_paren_level)
		  dollar_paren_level++;
	      }

	    if (character == '[')
	      {
		if (!delimiter_depth && dollar_bracket_level)
		  dollar_bracket_level++;
	      }

	    if (character == '{' && !embedded_quoted_string)
	      {
	      	if (delimiter_depth && delimited_brace_level)
	      	  delimited_brace_level++;

	      	if (!delimiter_depth && dollar_brace_level)
	      	  dollar_brace_level++;
	      }

	    /* This code needs to take into account whether we are inside a
	       case statement pattern list, and whether this paren is supposed
	       to terminate it (hey, it could happen).  It's not as simple
	       as just using in_case_pattern_list, because we're not parsing
	       anything while we're reading a $( ) construct.  Maybe we
	       should move that whole mess into the yacc parser. */
	    if (character == ')' && !embedded_quoted_string)
	      {
		if (delimiter_depth && delimited_paren_level)
		  delimited_paren_level--;

		if (!delimiter_depth && dollar_paren_level)
		  {
		    dollar_paren_level--;
		    goto got_character;
		  }
	      }

	    if (character == ']')
	      {
		if (!delimiter_depth && dollar_bracket_level)
		  {
		    dollar_bracket_level--;
		    goto got_character;
		  }
	      }

	    if (character == '}' && !embedded_quoted_string)
	      {
		if (delimiter_depth && delimited_brace_level)
		  delimited_brace_level--;

		if (!delimiter_depth && dollar_brace_level)
		  {
		    dollar_brace_level--;
		    goto got_character;
		  }
	      }
	  }

	if (!dollar_paren_level && !dollar_bracket_level &&
	    !dollar_brace_level && !delimiter_depth &&
	    member (character, " \t\n;&()|<>"))
	  {
	    shell_ungetc (character);
	    goto got_token;
	  }
    
	if (!delimiter_depth)
	  {
	    if (character == '"' || character == '`' || character == '\'')
	      {
		push_delimiter (character);

		quoted = 1;
		goto got_character;
	      }
	  }

	if (all_digits) all_digits = digit (character);
	if (character == '$') dollar_present = 1;

      got_character:

	if (character == CTLESC || character == CTLNUL)
	  token[token_index++] = CTLESC;

	token[token_index++] = character;

	if (token_index == (token_buffer_size - 1))
	  token = (char *)xrealloc (token, (token_buffer_size
					    += TOKEN_DEFAULT_GROW_SIZE));
	{
	next_character:
	  if (character == '\n' && interactive && bash_input.type != st_string)
	    prompt_again ();
	}
	/* We want to remove quoted newlines (that is, a \<newline> pair)
	   unless we are within single quotes or pass_next_character is
	   set (the shell equivalent of literal-next). */
	character = shell_getc
	  ((current_delimiter () != '\'') && (!pass_next_character));
      }

  got_token:

    token[token_index] = '\0';
	
    if ((delimiter_depth || dollar_paren_level || dollar_bracket_level) &&
	character == EOF)
      {
	char reporter = '\0';

	if (!delimiter_depth)
	  {
	    if (dollar_paren_level)
	      reporter = ')';
	    else if (dollar_bracket_level)
	      reporter = ']';
	  }

	if (!reporter)
	  reporter = current_delimiter ();

	report_error ("unexpected EOF while looking for `%c'", reporter);
	return (-1);
      }

    if (all_digits)
      {
	/* Check to see what thing we should return.  If the last_read_token
	   is a `<', or a `&', or the character which ended this token is
	   a '>' or '<', then, and ONLY then, is this input token a NUMBER.
	   Otherwise, it is just a word, and should be returned as such. */

	if ((character == '<' || character == '>') ||
	    (last_read_token == LESS_AND ||
	     last_read_token == GREATER_AND))
	  {
	    yylval.number = atoi (token); /* was sscanf (token, "%d", &(yylval.number)); */
	    return (NUMBER);
	  }
      }

    /* Handle special case.  IN is recognized if the last token
       was WORD and the token before that was FOR or CASE. */
    if ((last_read_token == WORD) &&
	((token_before_that == FOR) || (token_before_that == CASE)) &&
	(STREQ (token, "in")))
      {
	if (token_before_that == CASE)
	  {
	    in_case_pattern_list = 1;
	    allow_esac_as_next++;
	  }
	return (IN);
      }

    /* Ditto for DO in the FOR case. */
    if ((last_read_token == WORD) && (token_before_that == FOR) &&
	(STREQ (token, "do")))
      return (DO);

    /* Ditto for ESAC in the CASE case. 
       Specifically, this handles "case word in esac", which is a legal
       construct, certainly because someone will pass an empty arg to the
       case construct, and we don't want it to barf.  Of course, we should
       insist that the case construct has at least one pattern in it, but
       the designers disagree. */
    if (allow_esac_as_next)
      {
	allow_esac_as_next--;
	if (STREQ (token, "esac"))
	  {
	    in_case_pattern_list = 0;
	    return (ESAC);
	  }
      }

    /* Ditto for `{' in the FUNCTION case. */
    if (allow_open_brace)
      {
	allow_open_brace = 0;
	if (STREQ (token, "{"))
	  {
	    open_brace_awaiting_satisfaction++;
	    return ('{');
	  }
      }

    if (posixly_correct)
      CHECK_FOR_RESERVED_WORD (token);

#if defined (ALIAS)

    /* OK, we have a token.  Let's try to alias expand it, if (and only if)
       it's eligible. 

       It is eligible for expansion if the shell is in interactive mode, and
       the token is unquoted and the last token read was a command
       separator (or expand_next_token is set), and we are currently
       processing an alias (pushed_string_list is non-empty) and this
       token is not the same as the current or any previously
       processed alias.

       Special cases that disqualify:
	 In a pattern list in a case statement (in_case_pattern_list). */
    if (interactive_shell && !quoted && !in_case_pattern_list &&
	(expand_next_token || command_token_position (last_read_token)))
      {
	char *alias_expand_word (), *expanded;

	if (expanded_token_stack && token_has_been_expanded (token))
	  goto no_expansion;

	expanded = alias_expand_word (token);
	if (expanded)
	  {
	    int len = strlen (expanded), expand_next;

	    /* Erase the current token. */
	    token_index = 0;

	    expand_next = (expanded[len - 1] == ' ') ||
			  (expanded[len - 1] == '\t');

	    push_string (expanded, expand_next, token);
	    goto re_read_token;
	  }
	else
	  /* This is an eligible token that does not have an expansion. */
no_expansion:
	  expand_next_token = 0;
      }
    else
      {
	expand_next_token = 0;
      }
#endif /* ALIAS */

    CHECK_FOR_RESERVED_WORD (token);

    /* What if we are attempting to satisfy an open-brace grouper? */
    if (open_brace_awaiting_satisfaction && token[0] == '}' && !token[1])
      {
	open_brace_awaiting_satisfaction--;
	return ('}');
      }

    the_word = (WORD_DESC *)xmalloc (sizeof (WORD_DESC));
    the_word->word = (char *)xmalloc (1 + token_index);
    strcpy (the_word->word, token);
    the_word->dollar_present = dollar_present;
    the_word->quoted = quoted;
    the_word->assignment = assignment (token);

    yylval.word = the_word;
    result = WORD;

    /* A word is an assignment if it appears at the beginning of a
       simple command, or after another assignment word.  This is
       context-dependent, so it cannot be handled in the grammar. */
    if (assignment_acceptable (last_read_token) && the_word->assignment)
      result = ASSIGNMENT_WORD;

    if (last_read_token == FUNCTION)
      allow_open_brace = 1;
  }
  return (result);
}

/* Return 1 if TOKEN is a token that after being read would allow
   a reserved word to be seen, else 0. */
static int
reserved_word_acceptable (token)
     int token;
{
  if (member (token, "\n;()|&{") ||
      token == '}' ||			/* XXX */
      token == AND_AND ||
      token == BANG ||
      token == DO ||
      token == ELIF ||
      token == ELSE ||
      token == FI ||
      token == IF ||
      token == OR_OR ||
      token == SEMI_SEMI ||
      token == THEN ||
      token == UNTIL ||
      token == WHILE ||
      token == DONE ||		/* XXX these two are experimental */
      token == ESAC ||
      token == 0)
    return (1);
  else
    return (0);
}

/* Return the index of TOKEN in the alist of reserved words, or -1 if
   TOKEN is not a shell reserved word. */
int
find_reserved_word (token)
     char *token;
{
  int i;
  for (i = 0; word_token_alist[i].word != (char *)NULL; i++)
    if (STREQ (token, word_token_alist[i].word))
      return i;
  return -1;
}

#if defined (READLINE)
/* Called after each time readline is called.  This insures that whatever
   the new prompt string is gets propagated to readline's local prompt
   variable. */
static void
reset_readline_prompt ()
{
  if (prompt_string_pointer && *prompt_string_pointer)
    {
      char *temp_prompt;

      temp_prompt = decode_prompt_string (*prompt_string_pointer);

      if (!temp_prompt)
	temp_prompt = savestring ("");

      if (current_readline_prompt)
	free (current_readline_prompt);

      current_readline_prompt = temp_prompt;
    }
}
#endif /* READLINE */

#if defined (HISTORY)
/* A list of tokens which can be followed by newlines, but not by
   semi-colons.  When concatenating multiple lines of history, the
   newline separator for such tokens is replaced with a space. */
static int no_semi_successors[] = {
  '\n', '{', '(', ')', ';', '&', '|',
  CASE, DO, ELSE, IF, IN, SEMI_SEMI, THEN, UNTIL, WHILE, AND_AND, OR_OR,
  0
};

/* Add a line to the history list.
   The variable COMMAND_ORIENTED_HISTORY controls the style of history
   remembering;  when non-zero, and LINE is not the first line of a
   complete parser construct, append LINE to the last history line instead
   of adding it as a new line. */
static void
bash_add_history (line)
     char *line;
{
  int add_it = 1;

  if (command_oriented_history && current_command_line_count > 1)
    {
      register int offset;
      register HIST_ENTRY *current, *old;
      char *chars_to_add, *new_line;

      /* If we are not within a delimited expression, try to be smart
	 about which separators can be semi-colons and which must be
	 newlines. */
      if (!delimiter_depth)
	{
	  register int i;

	  chars_to_add = (char *)NULL;

	  for (i = 0; no_semi_successors[i]; i++)
	    {
	      if (token_before_that == no_semi_successors[i])
		{
		  chars_to_add = " ";
		  break;
		}
	    }
	  if (!chars_to_add)
	    chars_to_add = "; ";
	}
      else
	chars_to_add = "\n";

      using_history ();

      current = previous_history ();

      if (current)
	{
	  /* If the previous line ended with an escaped newline (escaped
	     with backslash, but otherwise unquoted), then remove the quoted
	     newline, since that is what happens when the line is parsed. */
	  int curlen;

	  curlen = strlen (current->line);

	  if (!delimiter_depth && current->line[curlen - 1] == '\\' &&
	      current->line[curlen - 2] != '\\')
	    {
	      current->line[curlen - 1] = '\0';
	      curlen--;
	      chars_to_add = "";
	    }

	  offset = where_history ();
	  new_line = (char *) xmalloc (1
				       + curlen
				       + strlen (line)
				       + strlen (chars_to_add));
	  sprintf (new_line, "%s%s%s", current->line, chars_to_add, line);
	  old = replace_history_entry (offset, new_line, current->data);
	  free (new_line);

	  if (old)
	    {
	      /* Note that the old data is not freed, since it was simply
		 copied to the new history entry. */
	      if (old->line)
		free (old->line);

	      free (old);
	    }
	  add_it = 0;
	}
    }

  if (add_it)
    {
      extern int history_lines_this_session;

      add_history (line);
      history_lines_this_session++;
    }
  using_history ();
}
#endif /* HISTORY */

/* Issue a prompt, or prepare to issue a prompt when the next character
   is read. */
static void
prompt_again ()
{
  char *temp_prompt;

  ps1_prompt = get_string_value ("PS1");
  ps2_prompt = get_string_value ("PS2");

  if (!prompt_string_pointer)
    prompt_string_pointer = &ps1_prompt;

  if (*prompt_string_pointer)
    temp_prompt = decode_prompt_string (*prompt_string_pointer);
  else
    temp_prompt = savestring ("");

  current_prompt_string = *prompt_string_pointer;
  prompt_string_pointer = &ps2_prompt;

#if defined (READLINE)
  if (!no_line_editing)
    {
      if (current_readline_prompt)
	free (current_readline_prompt);
      
      current_readline_prompt = temp_prompt;
    }
  else
#endif	/* READLINE */
    {
      if (interactive)
	{
	  fprintf (stderr, "%s", temp_prompt);
	  fflush (stderr);
	}
      free (temp_prompt);
    }
}

/* Return a string which will be printed as a prompt.  The string
   may contain special characters which are decoded as follows:
   
	\t	the time
	\d	the date
	\n	CRLF
	\s	the name of the shell
	\w	the current working directory
	\W	the last element of PWD
	\u	your username
	\h	the hostname
	\#	the command number of this command
	\!	the history number of this command
	\$	a $ or a # if you are root
	\<octal> character code in octal
	\\	a backslash
*/
#define PROMPT_GROWTH 50
char *
decode_prompt_string (string)
     char *string;
{
  int result_size = PROMPT_GROWTH;
  int result_index = 0;
  char *result;
  int c;
  char *temp = (char *)NULL;

#if defined (PROMPT_STRING_DECODE)

  result = (char *)xmalloc (PROMPT_GROWTH);
  result[0] = 0;

  while (c = *string++)
    {
      if (posixly_correct && c == '!')
	{
	  if (*string == '!')
	    {
	      temp = savestring ("!");
	      goto add_string;
	    }
	  else
	    {
#if !defined (HISTORY)
		temp = savestring ("1");
#else /* HISTORY */
		char number_buffer[20];

		using_history ();
		if (get_string_value ("HISTSIZE"))
		  sprintf (number_buffer, "%d",
			   history_base + where_history ());
		else
		  strcpy (number_buffer, "!");
		temp = savestring (number_buffer);
#endif /* HISTORY */
		string--;	/* add_string increments string again. */
		goto add_string;
	    }
	} 
      if (c == '\\')
	{
	  c = *string;

	  switch (c)
	    {
	    case '0':
	    case '1':
	    case '2':
	    case '3':
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	      {
		char octal_string[4];
		int n;

		strncpy (octal_string, string, 3);
		octal_string[3] = '\0';

		n = read_octal (octal_string);

		temp = savestring ("\\");
		if (n != -1)
		  {
		    string += 3;
		    temp[0] = n;
		  }

		c = 0;
		goto add_string;
	      }
	  
	    case 't':
	    case 'd':
	      /* Make the current time/date into a string. */
	      {
		time_t the_time = time (0);
		char *ttemp = ctime (&the_time);
		temp = savestring (ttemp);

		if (c == 't')
		  {
		    strcpy (temp, temp + 11);
		    temp[8] = '\0';
		  }
		else
		  temp[10] = '\0';

		goto add_string;
	      }

	    case 'n':
	      if (!no_line_editing)
		temp = savestring ("\r\n");
	      else
		temp = savestring ("\n");
	      goto add_string;

	    case 's':
	      {
		extern char *shell_name;

		temp = base_pathname (shell_name);
		temp = savestring (temp);
		goto add_string;
	      }
	
	    case 'w':
	    case 'W':
	      {
		/* Use the value of PWD because it is much more effecient. */
#define EFFICIENT
#ifdef EFFICIENT
		char *polite_directory_format (), t_string[MAXPATHLEN];

		temp = get_string_value ("PWD");

		if (!temp)
		  getwd (t_string);
		else
		  strcpy (t_string, temp);
#else
		getwd (t_string);
#endif	/* EFFICIENT */

		if (c == 'W')
		  {
		    char *dir = (char *)strrchr (t_string, '/');
		    if (dir && dir != t_string)
		      strcpy (t_string, dir + 1);
		    temp = savestring (t_string);
		  }
		else
		  temp = savestring (polite_directory_format (t_string));
		goto add_string;
	      }
      
	    case 'u':
	      {
		temp = savestring (current_user.user_name);

		goto add_string;
	      }

	    case 'h':
	      {
		extern char *current_host_name;
		char *t_string;

		temp = savestring (current_host_name);
		if (t_string = (char *)strchr (temp, '.'))
		  *t_string = '\0';
		
		goto add_string;
	      }

	    case '#':
	      {
		extern int current_command_number;
		char number_buffer[20];
		sprintf (number_buffer, "%d", current_command_number);
		temp = savestring (number_buffer);
		goto add_string;
	      }

	    case '!':
	      {
#if !defined (HISTORY)
		temp = savestring ("1");
#else /* HISTORY */
		char number_buffer[20];

		using_history ();
		if (get_string_value ("HISTSIZE"))
		  sprintf (number_buffer, "%d",
			   history_base + where_history ());
		else
		  strcpy (number_buffer, "!");
		temp = savestring (number_buffer);
#endif /* HISTORY */
		goto add_string;
	      }

	    case '$':
	      temp = savestring (geteuid () == 0 ? "#" : "$");
	      goto add_string;

	    case '\\':
	      temp = savestring ("\\");
	      goto add_string;

	    default:
	      temp = savestring ("\\ ");
	      temp[1] = c;

	    add_string:
	      if (c)
		string++;
	      result =
		sub_append_string (temp, result,
				   &result_index, &result_size);
	      temp = (char *)NULL; /* Free ()'ed in sub_append_string (). */
	      result[result_index] = '\0';
	      break;
	    }
	}
      else
	{
	  while (3 + result_index > result_size)
	    result = (char *)xrealloc (result, result_size += PROMPT_GROWTH);

	  result[result_index++] = c;
	  result[result_index] = '\0';
	}
    }
#else /* !PROMPT_STRING_DECODE */
  result = savestring (string);
#endif /* !PROMPT_STRING_DECODE */

  if (!find_variable ("NO_PROMPT_VARS"))
    {
      WORD_LIST *expand_string (), *list;
      char *string_list ();

      list = expand_string (result, 1);
      free (result);
      result = string_list (list);
      dispose_words (list);
    }

  return (result);
}

/* Report a syntax error, and restart the parser.  Call here for fatal
   errors. */
yyerror ()
{
  report_syntax_error ((char *)NULL);
  reset_parser ();
}

/* Report a syntax error with line numbers, etc.
   Call here for recoverable errors.  If you have a message to print,
   then place it in MESSAGE, otherwise pass NULL and this will figure
   out an appropriate message for you. */
static void
report_syntax_error (message)
     char *message;
{
  if (message)
    {
      if (!interactive)
	{
	  char *name = bash_input.name ? bash_input.name : "stdin";
	  report_error ("%s: line %d: `%s'", name, line_number, message);
	}
      else
	{
	  if (EOF_Reached)
	    EOF_Reached = 0;
	  report_error ("%s", message);
	}

      last_command_exit_value = 2;
      return;
    }

  if (shell_input_line && *shell_input_line)
    {
      char *error_token, *t = shell_input_line;
      register int i = shell_input_line_index;
      int token_end = 0;

      if (!t[i] && i)
	i--;

      while (i && (t[i] == ' ' || t[i] == '\t' || t[i] == '\n'))
	i--;

      if (i)
	token_end = i + 1;

      while (i && !member (t[i], " \n\t;|&"))
	i--;

      while (i != token_end && member (t[i], " \n\t"))
	i++;

      if (token_end)
	{
	  error_token = (char *)alloca (1 + (token_end - i));
	  strncpy (error_token, t + i, token_end - i);
	  error_token[token_end - i] = '\0';

	  report_error ("syntax error near unexpected token `%s'", error_token);
	}
      else if ((i == 0) && (token_end == 0))	/* a 1-character token */
	{
	  error_token = (char *) alloca (2);
	  strncpy (error_token, t + i, 1);
	  error_token[1] = '\0';

	  report_error ("syntax error near unexpected token `%s'", error_token);
	}

      if (!interactive)
	{
	  char *temp = savestring (shell_input_line);
	  char *name = bash_input.name ? bash_input.name : "stdin";
	  int l = strlen (temp);

	  while (l && temp[l - 1] == '\n')
	    temp[--l] = '\0';

	  report_error ("%s: line %d: `%s'", name, line_number, temp);
	  free (temp);
	}
    }
  else
    {
      char *name, *msg;
      if (!interactive)
	name = bash_input.name ? bash_input.name : "stdin";
      if (EOF_Reached)
	msg = "syntax error: unexpected end of file";
      else
	msg = "syntax error";
      if (!interactive)
	report_error ("%s: line %d: %s", name, line_number, msg);
      else
	{
	  /* This file uses EOF_Reached only for error reporting
	     when the shell is interactive.  Other mechanisms are 
	     used to decide whether or not to exit. */
	  EOF_Reached = 0;
	  report_error (msg);
	}
    }
  last_command_exit_value = 2;
}

/* ??? Needed function. ??? We have to be able to discard the constructs
   created during parsing.  In the case of error, we want to return
   allocated objects to the memory pool.  In the case of no error, we want
   to throw away the information about where the allocated objects live.
   (dispose_command () will actually free the command. */
discard_parser_constructs (error_p)
     int error_p;
{
}
   
/* Do that silly `type "bye" to exit' stuff.  You know, "ignoreeof". */

/* The number of times that we have encountered an EOF character without
   another character intervening.  When this gets above the limit, the
   shell terminates. */
int eof_encountered = 0;

/* The limit for eof_encountered. */
int eof_encountered_limit = 10;

/* If we have EOF as the only input unit, this user wants to leave
   the shell.  If the shell is not interactive, then just leave.
   Otherwise, if ignoreeof is set, and we haven't done this the
   required number of times in a row, print a message. */
static void
handle_eof_input_unit ()
{
  extern int login_shell, EOF_Reached;

  if (interactive)
    {
      /* shell.c may use this to decide whether or not to write out the
	 history, among other things.  We use it only for error reporting
	 in this file. */
      if (EOF_Reached)
	EOF_Reached = 0;

      /* If the user wants to "ignore" eof, then let her do so, kind of. */
      if (find_variable ("ignoreeof") || find_variable ("IGNOREEOF"))
	{
	  if (eof_encountered < eof_encountered_limit)
	    {
	      fprintf (stderr, "Use \"%s\" to leave the shell.\n",
		       login_shell ? "logout" : "exit");
	      eof_encountered++;
	      /* Reset the prompt string to be $PS1. */
	      prompt_string_pointer = (char **)NULL;
	      prompt_again ();
	      last_read_token = current_token = '\n';
	      return;
	    } 
	}

      /* In this case EOF should exit the shell.  Do it now. */
      reset_parser ();
      exit_builtin ((WORD_LIST *)NULL);
    }
  else
    {
      /* We don't write history files, etc., for non-interactive shells. */
      EOF_Reached = 1;
    }
}
