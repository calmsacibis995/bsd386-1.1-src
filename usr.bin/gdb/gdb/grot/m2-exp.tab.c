
/*  A Bison parser, made from ./m2-exp.y  */

#define YYBISON 1  /* Identify Bison output.  */

#define	INT	258
#define	HEX	259
#define	ERROR	260
#define	UINT	261
#define	TRUE	262
#define	FALSE	263
#define	CHAR	264
#define	FLOAT	265
#define	STRING	266
#define	NAME	267
#define	BLOCKNAME	268
#define	IDENT	269
#define	VARNAME	270
#define	TYPENAME	271
#define	SIZE	272
#define	CAP	273
#define	ORD	274
#define	HIGH	275
#define	ABS	276
#define	MIN_FUNC	277
#define	MAX_FUNC	278
#define	FLOAT_FUNC	279
#define	VAL	280
#define	CHR	281
#define	ODD	282
#define	TRUNC	283
#define	INC	284
#define	DEC	285
#define	INCL	286
#define	EXCL	287
#define	COLONCOLON	288
#define	LAST	289
#define	REGNAME	290
#define	INTERNAL_VAR	291
#define	ABOVE_COMMA	292
#define	ASSIGN	293
#define	LEQ	294
#define	GEQ	295
#define	NOTEQUAL	296
#define	IN	297
#define	OROR	298
#define	ANDAND	299
#define	DIV	300
#define	MOD	301
#define	UNARY	302
#define	DOT	303
#define	NOT	304
#define	QID	305

#line 31 "./m2-exp.y"

#include <stdio.h>
#include <string.h>
#include "defs.h"
#include "symtab.h"
#include "gdbtypes.h"
#include "frame.h"
#include "expression.h"
#include "language.h"
#include "value.h"
#include "parser-defs.h"
#include "bfd.h"
#include "symfile.h"
#include "objfiles.h"

/* Ensure that if the generated parser contains any calls to malloc/realloc,
   that they get mapped to xmalloc/xrealloc. */

#define malloc	xmalloc
#define realloc	xrealloc

/* These MUST be included in any grammar file!!!!
   Please choose unique names! */
#define	yymaxdepth m2_maxdepth
#define	yyparse	m2_parse
#define	yylex	m2_lex
#define	yyerror	m2_error
#define	yylval	m2_lval
#define	yychar	m2_char
#define	yydebug	m2_debug
#define	yypact	m2_pact
#define	yyr1	m2_r1
#define	yyr2	m2_r2
#define	yydef	m2_def
#define	yychk	m2_chk
#define	yypgo	m2_pgo
#define	yyact	m2_act
#define	yyexca	m2_exca
#define yyerrflag m2_errflag
#define yynerrs	m2_nerrs
#define	yyps	m2_ps
#define	yypv	m2_pv
#define	yys	m2_s
#define	yy_yys	m2_yys
#define	yystate	m2_state
#define	yytmp	m2_tmp
#define	yyv	m2_v
#define	yy_yyv	m2_yyv
#define	yyval	m2_val
#define	yylloc	m2_lloc

#if 0
static char *
make_qualname PARAMS ((char *, char *));
#endif

static int
parse_number PARAMS ((int));

static int
yylex PARAMS ((void));

static void
yyerror PARAMS ((char *));

int
yyparse PARAMS ((void));

/* The sign of the number being parsed. */
int number_sign = 1;

/* The block that the module specified by the qualifer on an identifer is
   contained in, */
struct block *modblock=0;

/* #define	YYDEBUG	1 */


#line 114 "./m2-exp.y"
typedef union
  {
    LONGEST lval;
    unsigned LONGEST ulval;
    double dval;
    struct symbol *sym;
    struct type *tval;
    struct stoken sval;
    int voidval;
    struct block *bval;
    enum exp_opcode opcode;
    struct internalvar *ivar;

    struct type **tvec;
    int *ivec;
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

#ifndef __STDC__
#define const
#endif



#define	YYFINAL		185
#define	YYFLAG		-32768
#define	YYNTBASE	70

#define YYTRANSLATE(x) ((unsigned)(x) <= 305 ? yytranslate[x] : 84)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,    46,     2,     2,    50,     2,    62,
    66,    54,    52,    37,    53,     2,    55,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,    40,
    44,    41,     2,    51,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
    61,     2,    69,    59,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    67,     2,    68,    64,     2,     2,     2,     2,
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
    26,    27,    28,    29,    30,    31,    32,    33,    34,    35,
    36,    38,    39,    42,    43,    45,    47,    48,    49,    56,
    57,    58,    60,    63,    65
};

static const short yyprhs[] = {     0,
     0,     2,     4,     6,     9,    10,    14,    17,    20,    22,
    24,    29,    34,    39,    44,    49,    54,    59,    66,    71,
    76,    81,    84,    89,    96,   101,   108,   112,   114,   118,
   125,   132,   136,   141,   142,   148,   149,   155,   156,   158,
   162,   164,   168,   173,   178,   182,   186,   190,   194,   198,
   202,   206,   210,   214,   218,   222,   226,   230,   234,   238,
   242,   246,   250,   254,   256,   258,   260,   262,   264,   266,
   268,   270,   272,   277,   279,   281,   283,   287,   289,   291,
   295,   297
};

static const short yyrhs[] = {    72,
     0,    71,     0,    83,     0,    72,    59,     0,     0,    53,
    73,    72,     0,    52,    72,     0,    74,    72,     0,    63,
     0,    64,     0,    18,    62,    72,    66,     0,    19,    62,
    72,    66,     0,    21,    62,    72,    66,     0,    20,    62,
    72,    66,     0,    22,    62,    83,    66,     0,    23,    62,
    83,    66,     0,    24,    62,    72,    66,     0,    25,    62,
    83,    37,    72,    66,     0,    26,    62,    72,    66,     0,
    27,    62,    72,    66,     0,    28,    62,    72,    66,     0,
    17,    72,     0,    29,    62,    72,    66,     0,    29,    62,
    72,    37,    72,    66,     0,    30,    62,    72,    66,     0,
    30,    62,    72,    37,    72,    66,     0,    72,    60,    12,
     0,    75,     0,    72,    47,    75,     0,    31,    62,    72,
    37,    72,    66,     0,    32,    62,    72,    37,    72,    66,
     0,    67,    78,    68,     0,    83,    67,    78,    68,     0,
     0,    72,    61,    76,    79,    69,     0,     0,    72,    62,
    77,    78,    66,     0,     0,    72,     0,    78,    37,    72,
     0,    72,     0,    79,    37,    72,     0,    67,    83,    68,
    72,     0,    83,    62,    72,    66,     0,    62,    72,    66,
     0,    72,    51,    72,     0,    72,    54,    72,     0,    72,
    55,    72,     0,    72,    56,    72,     0,    72,    57,    72,
     0,    72,    52,    72,     0,    72,    53,    72,     0,    72,
    44,    72,     0,    72,    45,    72,     0,    72,    46,    72,
     0,    72,    42,    72,     0,    72,    43,    72,     0,    72,
    40,    72,     0,    72,    41,    72,     0,    72,    49,    72,
     0,    72,    50,    72,     0,    72,    48,    72,     0,    72,
    39,    72,     0,     7,     0,     8,     0,     3,     0,     6,
     0,     9,     0,    10,     0,    82,     0,    34,     0,    35,
     0,    17,    62,    83,    66,     0,    11,     0,    81,     0,
    13,     0,    80,    33,    13,     0,    81,     0,    36,     0,
    80,    33,    12,     0,    12,     0,    16,     0
};

#if YYDEBUG != 0
static const short yyrline[] = { 0,
   183,   184,   187,   196,   199,   201,   206,   210,   214,   215,
   218,   222,   226,   230,   234,   240,   246,   250,   256,   260,
   264,   268,   273,   277,   283,   287,   293,   299,   302,   306,
   310,   313,   315,   321,   326,   332,   336,   342,   345,   349,
   354,   359,   364,   370,   376,   384,   388,   392,   396,   400,
   404,   408,   412,   416,   418,   422,   426,   430,   434,   438,
   442,   446,   450,   457,   463,   469,   476,   485,   493,   500,
   504,   510,   516,   523,   530,   534,   543,   555,   562,   569,
   583,   656
};

static const char * const yytname[] = {   "$","error","$illegal.","INT","HEX",
"ERROR","UINT","TRUE","FALSE","CHAR","FLOAT","STRING","NAME","BLOCKNAME","IDENT",
"VARNAME","TYPENAME","SIZE","CAP","ORD","HIGH","ABS","MIN_FUNC","MAX_FUNC","FLOAT_FUNC",
"VAL","CHR","ODD","TRUNC","INC","DEC","INCL","EXCL","COLONCOLON","LAST","REGNAME",
"INTERNAL_VAR","','","ABOVE_COMMA","ASSIGN","'<'","'>'","LEQ","GEQ","'='","NOTEQUAL",
"'#'","IN","OROR","ANDAND","'&'","'@'","'+'","'-'","'*'","'/'","DIV","MOD","UNARY",
"'^'","DOT","'['","'('","NOT","'~'","QID","')'","'{'","'}'","']'","start","type_exp",
"exp","@1","not_exp","set","@2","@3","arglist","non_empty_arglist","block","fblock",
"variable","type",""
};
#endif

static const short yyr1[] = {     0,
    70,    70,    71,    72,    73,    72,    72,    72,    74,    74,
    72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
    72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
    72,    75,    75,    76,    72,    77,    72,    78,    78,    78,
    79,    79,    72,    72,    72,    72,    72,    72,    72,    72,
    72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
    72,    72,    72,    72,    72,    72,    72,    72,    72,    72,
    72,    72,    72,    72,    80,    81,    81,    82,    82,    82,
    82,    83
};

static const short yyr2[] = {     0,
     1,     1,     1,     2,     0,     3,     2,     2,     1,     1,
     4,     4,     4,     4,     4,     4,     4,     6,     4,     4,
     4,     2,     4,     6,     4,     6,     3,     1,     3,     6,
     6,     3,     4,     0,     5,     0,     5,     0,     1,     3,
     1,     3,     4,     4,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     3,     3,     3,     1,     1,     1,     1,     1,     1,     1,
     1,     1,     4,     1,     1,     1,     3,     1,     1,     3,
     1,     1
};

static const short yydefact[] = {     0,
    66,    67,    64,    65,    68,    69,    74,    81,    76,    82,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,    71,    72,    79,     0,
     5,     0,     9,    10,    38,     2,     1,     0,    28,     0,
    78,    70,     3,     0,    22,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     7,     0,     0,    39,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     4,     0,    34,    36,
     8,     0,     0,    38,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     6,    45,     0,    32,     0,    63,    58,    59,    56,    57,
    53,    54,    55,    38,    29,     0,    62,    60,    61,    46,
    51,    52,    47,    48,    49,    50,    27,     0,    38,    80,
    77,     0,     0,    73,    11,    12,    14,    13,    15,    16,
    17,     0,    19,    20,    21,     0,    23,     0,    25,     0,
     0,    40,    43,    41,     0,     0,    44,    33,     0,     0,
     0,     0,     0,     0,    35,    37,    18,    24,    26,    30,
    31,    42,     0,     0,     0
};

static const short yydefgoto[] = {   183,
    36,    65,    63,    38,    39,   138,   139,    66,   165,    40,
    41,    42,    46
};

static const short yypact[] = {   159,
-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
   221,   -53,   -52,   -31,   -23,   -21,   -20,   -19,   -11,    14,
    26,    27,    29,    30,    35,    36,-32768,-32768,-32768,   159,
-32768,   159,-32768,-32768,   159,-32768,   776,   159,-32768,    -4,
    66,-32768,   -22,   159,     5,   -22,   159,   159,   159,   159,
    17,    17,   159,    17,   159,   159,   159,   159,   159,   159,
   159,     5,   159,   306,   776,   -33,   -40,   159,   159,   159,
   159,   159,   159,   159,   159,   -13,   159,   159,   159,   159,
   159,   159,   159,   159,   159,   159,-32768,    50,-32768,-32768,
     5,    -6,   159,   159,    28,   334,   362,   390,   418,    34,
    39,   446,    64,   474,   502,   530,   250,   278,   726,   752,
     5,-32768,   159,-32768,   159,   800,   -36,   -36,   -36,   -36,
   -36,   -36,   -36,   159,-32768,    40,    70,    88,    88,   144,
   204,   204,     5,     5,     5,     5,-32768,   159,   159,-32768,
-32768,   558,   -32,-32768,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,   159,-32768,-32768,-32768,   159,-32768,   159,-32768,   159,
   159,   776,     5,   776,   -35,   -29,-32768,-32768,   586,   614,
   642,   670,   698,   159,-32768,-32768,-32768,-32768,-32768,-32768,
-32768,   776,   102,   108,-32768
};

static const short yypgoto[] = {-32768,
-32768,     0,-32768,-32768,    33,-32768,-32768,   -93,-32768,-32768,
-32768,-32768,    52
};


#define	YYLAST		862


static const short yytable[] = {    37,
   143,   174,    10,   113,   113,   140,   141,   113,    47,    48,
    45,    77,    78,    79,    80,    81,    82,    83,    84,    85,
    86,    93,    87,    88,    89,    90,    94,   115,    92,    62,
    49,    64,    10,   175,   114,   168,   176,    91,    50,    93,
    51,    52,    53,    64,    94,   166,    96,    97,    98,    99,
    54,    43,   102,   124,   104,   105,   106,   107,   108,   109,
   110,   137,   111,    87,    88,    89,    90,   116,   117,   118,
   119,   120,   121,   122,   123,    55,   127,   128,   129,   130,
   131,   132,   133,   134,   135,   136,    67,    56,    57,    93,
    58,    59,   142,   144,    94,    95,    60,    61,   -75,   149,
   152,   184,   100,   101,   150,   103,    94,   185,   125,     0,
     0,     0,   162,     0,   163,     0,     0,     0,    78,    79,
    80,    81,    82,    83,    84,    85,    86,   126,    87,    88,
    89,    90,     0,     0,     0,     0,     0,   164,    80,    81,
    82,    83,    84,    85,    86,     0,    87,    88,    89,    90,
     0,   169,     0,     0,     0,   170,     0,   171,     0,   172,
   173,     1,     0,     0,     2,     3,     4,     5,     6,     7,
     8,     9,     0,   182,    10,    11,    12,    13,    14,    15,
    16,    17,    18,    19,    20,    21,    22,    23,    24,    25,
    26,     0,    27,    28,    29,    81,    82,    83,    84,    85,
    86,     0,    87,    88,    89,    90,     0,     0,     0,     0,
    30,    31,     0,     0,     0,     0,     0,     0,     0,     0,
    32,    33,    34,     1,     0,    35,     2,     3,     4,     5,
     6,     7,     8,     9,     0,     0,    10,    11,    12,    13,
    14,    15,    16,    17,    18,    19,    20,    21,    22,    23,
    24,    25,    26,     0,    27,    28,    29,    83,    84,    85,
    86,     0,    87,    88,    89,    90,     0,     0,     0,     0,
     0,     0,    30,    31,     0,     0,     0,     0,     0,     0,
     0,     0,    44,    33,    34,     0,   156,    35,    68,    69,
    70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
    80,    81,    82,    83,    84,    85,    86,     0,    87,    88,
    89,    90,     0,     0,   158,   157,    68,    69,    70,    71,
    72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
    82,    83,    84,    85,    86,     0,    87,    88,    89,    90,
     0,     0,     0,   159,    68,    69,    70,    71,    72,    73,
    74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
    84,    85,    86,     0,    87,    88,    89,    90,     0,     0,
     0,   112,    68,    69,    70,    71,    72,    73,    74,    75,
    76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
    86,     0,    87,    88,    89,    90,     0,     0,     0,   145,
    68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
    78,    79,    80,    81,    82,    83,    84,    85,    86,     0,
    87,    88,    89,    90,     0,     0,     0,   146,    68,    69,
    70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
    80,    81,    82,    83,    84,    85,    86,     0,    87,    88,
    89,    90,     0,     0,     0,   147,    68,    69,    70,    71,
    72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
    82,    83,    84,    85,    86,     0,    87,    88,    89,    90,
     0,     0,     0,   148,    68,    69,    70,    71,    72,    73,
    74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
    84,    85,    86,     0,    87,    88,    89,    90,     0,     0,
     0,   151,    68,    69,    70,    71,    72,    73,    74,    75,
    76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
    86,     0,    87,    88,    89,    90,     0,     0,     0,   153,
    68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
    78,    79,    80,    81,    82,    83,    84,    85,    86,     0,
    87,    88,    89,    90,     0,     0,     0,   154,    68,    69,
    70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
    80,    81,    82,    83,    84,    85,    86,     0,    87,    88,
    89,    90,     0,     0,     0,   155,    68,    69,    70,    71,
    72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
    82,    83,    84,    85,    86,     0,    87,    88,    89,    90,
     0,     0,     0,   167,    68,    69,    70,    71,    72,    73,
    74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
    84,    85,    86,     0,    87,    88,    89,    90,     0,     0,
     0,   177,    68,    69,    70,    71,    72,    73,    74,    75,
    76,    77,    78,    79,    80,    81,    82,    83,    84,    85,
    86,     0,    87,    88,    89,    90,     0,     0,     0,   178,
    68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
    78,    79,    80,    81,    82,    83,    84,    85,    86,     0,
    87,    88,    89,    90,     0,     0,     0,   179,    68,    69,
    70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
    80,    81,    82,    83,    84,    85,    86,     0,    87,    88,
    89,    90,     0,     0,     0,   180,    68,    69,    70,    71,
    72,    73,    74,    75,    76,    77,    78,    79,    80,    81,
    82,    83,    84,    85,    86,     0,    87,    88,    89,    90,
     0,     0,   160,   181,    68,    69,    70,    71,    72,    73,
    74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
    84,    85,    86,     0,    87,    88,    89,    90,   161,     0,
    68,    69,    70,    71,    72,    73,    74,    75,    76,    77,
    78,    79,    80,    81,    82,    83,    84,    85,    86,     0,
    87,    88,    89,    90,    68,    69,    70,    71,    72,    73,
    74,    75,    76,    77,    78,    79,    80,    81,    82,    83,
    84,    85,    86,     0,    87,    88,    89,    90,-32768,    69,
    70,    71,    72,    73,    74,    75,    76,    77,    78,    79,
    80,    81,    82,    83,    84,    85,    86,     0,    87,    88,
    89,    90
};

static const short yycheck[] = {     0,
    94,    37,    16,    37,    37,    12,    13,    37,    62,    62,
    11,    48,    49,    50,    51,    52,    53,    54,    55,    56,
    57,    62,    59,    60,    61,    62,    67,    68,    33,    30,
    62,    32,    16,    69,    68,    68,    66,    38,    62,    62,
    62,    62,    62,    44,    67,   139,    47,    48,    49,    50,
    62,     0,    53,    67,    55,    56,    57,    58,    59,    60,
    61,    12,    63,    59,    60,    61,    62,    68,    69,    70,
    71,    72,    73,    74,    75,    62,    77,    78,    79,    80,
    81,    82,    83,    84,    85,    86,    35,    62,    62,    62,
    62,    62,    93,    66,    67,    44,    62,    62,    33,    66,
    37,     0,    51,    52,    66,    54,    67,     0,    76,    -1,
    -1,    -1,   113,    -1,   115,    -1,    -1,    -1,    49,    50,
    51,    52,    53,    54,    55,    56,    57,    76,    59,    60,
    61,    62,    -1,    -1,    -1,    -1,    -1,   138,    51,    52,
    53,    54,    55,    56,    57,    -1,    59,    60,    61,    62,
    -1,   152,    -1,    -1,    -1,   156,    -1,   158,    -1,   160,
   161,     3,    -1,    -1,     6,     7,     8,     9,    10,    11,
    12,    13,    -1,   174,    16,    17,    18,    19,    20,    21,
    22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
    32,    -1,    34,    35,    36,    52,    53,    54,    55,    56,
    57,    -1,    59,    60,    61,    62,    -1,    -1,    -1,    -1,
    52,    53,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
    62,    63,    64,     3,    -1,    67,     6,     7,     8,     9,
    10,    11,    12,    13,    -1,    -1,    16,    17,    18,    19,
    20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
    30,    31,    32,    -1,    34,    35,    36,    54,    55,    56,
    57,    -1,    59,    60,    61,    62,    -1,    -1,    -1,    -1,
    -1,    -1,    52,    53,    -1,    -1,    -1,    -1,    -1,    -1,
    -1,    -1,    62,    63,    64,    -1,    37,    67,    39,    40,
    41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
    51,    52,    53,    54,    55,    56,    57,    -1,    59,    60,
    61,    62,    -1,    -1,    37,    66,    39,    40,    41,    42,
    43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
    53,    54,    55,    56,    57,    -1,    59,    60,    61,    62,
    -1,    -1,    -1,    66,    39,    40,    41,    42,    43,    44,
    45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
    55,    56,    57,    -1,    59,    60,    61,    62,    -1,    -1,
    -1,    66,    39,    40,    41,    42,    43,    44,    45,    46,
    47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
    57,    -1,    59,    60,    61,    62,    -1,    -1,    -1,    66,
    39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
    49,    50,    51,    52,    53,    54,    55,    56,    57,    -1,
    59,    60,    61,    62,    -1,    -1,    -1,    66,    39,    40,
    41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
    51,    52,    53,    54,    55,    56,    57,    -1,    59,    60,
    61,    62,    -1,    -1,    -1,    66,    39,    40,    41,    42,
    43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
    53,    54,    55,    56,    57,    -1,    59,    60,    61,    62,
    -1,    -1,    -1,    66,    39,    40,    41,    42,    43,    44,
    45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
    55,    56,    57,    -1,    59,    60,    61,    62,    -1,    -1,
    -1,    66,    39,    40,    41,    42,    43,    44,    45,    46,
    47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
    57,    -1,    59,    60,    61,    62,    -1,    -1,    -1,    66,
    39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
    49,    50,    51,    52,    53,    54,    55,    56,    57,    -1,
    59,    60,    61,    62,    -1,    -1,    -1,    66,    39,    40,
    41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
    51,    52,    53,    54,    55,    56,    57,    -1,    59,    60,
    61,    62,    -1,    -1,    -1,    66,    39,    40,    41,    42,
    43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
    53,    54,    55,    56,    57,    -1,    59,    60,    61,    62,
    -1,    -1,    -1,    66,    39,    40,    41,    42,    43,    44,
    45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
    55,    56,    57,    -1,    59,    60,    61,    62,    -1,    -1,
    -1,    66,    39,    40,    41,    42,    43,    44,    45,    46,
    47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
    57,    -1,    59,    60,    61,    62,    -1,    -1,    -1,    66,
    39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
    49,    50,    51,    52,    53,    54,    55,    56,    57,    -1,
    59,    60,    61,    62,    -1,    -1,    -1,    66,    39,    40,
    41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
    51,    52,    53,    54,    55,    56,    57,    -1,    59,    60,
    61,    62,    -1,    -1,    -1,    66,    39,    40,    41,    42,
    43,    44,    45,    46,    47,    48,    49,    50,    51,    52,
    53,    54,    55,    56,    57,    -1,    59,    60,    61,    62,
    -1,    -1,    37,    66,    39,    40,    41,    42,    43,    44,
    45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
    55,    56,    57,    -1,    59,    60,    61,    62,    37,    -1,
    39,    40,    41,    42,    43,    44,    45,    46,    47,    48,
    49,    50,    51,    52,    53,    54,    55,    56,    57,    -1,
    59,    60,    61,    62,    39,    40,    41,    42,    43,    44,
    45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
    55,    56,    57,    -1,    59,    60,    61,    62,    39,    40,
    41,    42,    43,    44,    45,    46,    47,    48,    49,    50,
    51,    52,    53,    54,    55,    56,    57,    -1,    59,    60,
    61,    62
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
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__)
#include <alloca.h>
#else /* not sparc */
#if defined (MSDOS) && !defined (__TURBOC__)
#include <malloc.h>
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
#include <malloc.h>
 #pragma alloca
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

#line 169 "/usr/local/lib/bison.simple"
int
yyparse()
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yysp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yysp--)
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
     so that they stay on the same level as the state stack.  */

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
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
#ifdef YYLSP_NEEDED
		 &yyls1, size * sizeof (*yylsp),
#endif
		 &yystacksize);

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
  yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symboles being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 3:
#line 188 "./m2-exp.y"
{ write_exp_elt_opcode(OP_TYPE);
		  write_exp_elt_type(yyvsp[0].tval);
		  write_exp_elt_opcode(OP_TYPE);
		;
    break;}
case 4:
#line 197 "./m2-exp.y"
{ write_exp_elt_opcode (UNOP_IND); ;
    break;}
case 5:
#line 200 "./m2-exp.y"
{ number_sign = -1; ;
    break;}
case 6:
#line 202 "./m2-exp.y"
{ number_sign = 1;
			  write_exp_elt_opcode (UNOP_NEG); ;
    break;}
case 7:
#line 207 "./m2-exp.y"
{ write_exp_elt_opcode(UNOP_PLUS); ;
    break;}
case 8:
#line 211 "./m2-exp.y"
{ write_exp_elt_opcode (UNOP_ZEROP); ;
    break;}
case 11:
#line 219 "./m2-exp.y"
{ write_exp_elt_opcode (UNOP_CAP); ;
    break;}
case 12:
#line 223 "./m2-exp.y"
{ write_exp_elt_opcode (UNOP_ORD); ;
    break;}
case 13:
#line 227 "./m2-exp.y"
{ write_exp_elt_opcode (UNOP_ABS); ;
    break;}
case 14:
#line 231 "./m2-exp.y"
{ write_exp_elt_opcode (UNOP_HIGH); ;
    break;}
case 15:
#line 235 "./m2-exp.y"
{ write_exp_elt_opcode (UNOP_MIN);
			  write_exp_elt_type (yyvsp[-1].tval);
			  write_exp_elt_opcode (UNOP_MIN); ;
    break;}
case 16:
#line 241 "./m2-exp.y"
{ write_exp_elt_opcode (UNOP_MAX);
			  write_exp_elt_type (yyvsp[-1].tval);
			  write_exp_elt_opcode (UNOP_MIN); ;
    break;}
case 17:
#line 247 "./m2-exp.y"
{ write_exp_elt_opcode (UNOP_FLOAT); ;
    break;}
case 18:
#line 251 "./m2-exp.y"
{ write_exp_elt_opcode (BINOP_VAL);
			  write_exp_elt_type (yyvsp[-3].tval);
			  write_exp_elt_opcode (BINOP_VAL); ;
    break;}
case 19:
#line 257 "./m2-exp.y"
{ write_exp_elt_opcode (UNOP_CHR); ;
    break;}
case 20:
#line 261 "./m2-exp.y"
{ write_exp_elt_opcode (UNOP_ODD); ;
    break;}
case 21:
#line 265 "./m2-exp.y"
{ write_exp_elt_opcode (UNOP_TRUNC); ;
    break;}
case 22:
#line 269 "./m2-exp.y"
{ write_exp_elt_opcode (UNOP_SIZEOF); ;
    break;}
case 23:
#line 274 "./m2-exp.y"
{ write_exp_elt_opcode(UNOP_PREINCREMENT); ;
    break;}
case 24:
#line 278 "./m2-exp.y"
{ write_exp_elt_opcode(BINOP_ASSIGN_MODIFY);
			  write_exp_elt_opcode(BINOP_ADD);
			  write_exp_elt_opcode(BINOP_ASSIGN_MODIFY); ;
    break;}
case 25:
#line 284 "./m2-exp.y"
{ write_exp_elt_opcode(UNOP_PREDECREMENT);;
    break;}
case 26:
#line 288 "./m2-exp.y"
{ write_exp_elt_opcode(BINOP_ASSIGN_MODIFY);
			  write_exp_elt_opcode(BINOP_SUB);
			  write_exp_elt_opcode(BINOP_ASSIGN_MODIFY); ;
    break;}
case 27:
#line 294 "./m2-exp.y"
{ write_exp_elt_opcode (STRUCTOP_STRUCT);
			  write_exp_string (yyvsp[0].sval);
			  write_exp_elt_opcode (STRUCTOP_STRUCT); ;
    break;}
case 29:
#line 303 "./m2-exp.y"
{ error("Sets are not implemented.");;
    break;}
case 30:
#line 307 "./m2-exp.y"
{ error("Sets are not implemented.");;
    break;}
case 31:
#line 311 "./m2-exp.y"
{ error("Sets are not implemented.");;
    break;}
case 32:
#line 314 "./m2-exp.y"
{ error("Sets are not implemented.");;
    break;}
case 33:
#line 316 "./m2-exp.y"
{ error("Sets are not implemented.");;
    break;}
case 34:
#line 325 "./m2-exp.y"
{ start_arglist(); ;
    break;}
case 35:
#line 327 "./m2-exp.y"
{ write_exp_elt_opcode (BINOP_MULTI_SUBSCRIPT);
			  write_exp_elt_longcst ((LONGEST) end_arglist());
			  write_exp_elt_opcode (BINOP_MULTI_SUBSCRIPT); ;
    break;}
case 36:
#line 335 "./m2-exp.y"
{ start_arglist (); ;
    break;}
case 37:
#line 337 "./m2-exp.y"
{ write_exp_elt_opcode (OP_FUNCALL);
			  write_exp_elt_longcst ((LONGEST) end_arglist ());
			  write_exp_elt_opcode (OP_FUNCALL); ;
    break;}
case 39:
#line 346 "./m2-exp.y"
{ arglist_len = 1; ;
    break;}
case 40:
#line 350 "./m2-exp.y"
{ arglist_len++; ;
    break;}
case 41:
#line 355 "./m2-exp.y"
{ arglist_len = 1; ;
    break;}
case 42:
#line 360 "./m2-exp.y"
{ arglist_len++; ;
    break;}
case 43:
#line 365 "./m2-exp.y"
{ write_exp_elt_opcode (UNOP_MEMVAL);
			  write_exp_elt_type (yyvsp[-2].tval);
			  write_exp_elt_opcode (UNOP_MEMVAL); ;
    break;}
case 44:
#line 371 "./m2-exp.y"
{ write_exp_elt_opcode (UNOP_CAST);
			  write_exp_elt_type (yyvsp[-3].tval);
			  write_exp_elt_opcode (UNOP_CAST); ;
    break;}
case 45:
#line 377 "./m2-exp.y"
{ ;
    break;}
case 46:
#line 385 "./m2-exp.y"
{ write_exp_elt_opcode (BINOP_REPEAT); ;
    break;}
case 47:
#line 389 "./m2-exp.y"
{ write_exp_elt_opcode (BINOP_MUL); ;
    break;}
case 48:
#line 393 "./m2-exp.y"
{ write_exp_elt_opcode (BINOP_DIV); ;
    break;}
case 49:
#line 397 "./m2-exp.y"
{ write_exp_elt_opcode (BINOP_INTDIV); ;
    break;}
case 50:
#line 401 "./m2-exp.y"
{ write_exp_elt_opcode (BINOP_REM); ;
    break;}
case 51:
#line 405 "./m2-exp.y"
{ write_exp_elt_opcode (BINOP_ADD); ;
    break;}
case 52:
#line 409 "./m2-exp.y"
{ write_exp_elt_opcode (BINOP_SUB); ;
    break;}
case 53:
#line 413 "./m2-exp.y"
{ write_exp_elt_opcode (BINOP_EQUAL); ;
    break;}
case 54:
#line 417 "./m2-exp.y"
{ write_exp_elt_opcode (BINOP_NOTEQUAL); ;
    break;}
case 55:
#line 419 "./m2-exp.y"
{ write_exp_elt_opcode (BINOP_NOTEQUAL); ;
    break;}
case 56:
#line 423 "./m2-exp.y"
{ write_exp_elt_opcode (BINOP_LEQ); ;
    break;}
case 57:
#line 427 "./m2-exp.y"
{ write_exp_elt_opcode (BINOP_GEQ); ;
    break;}
case 58:
#line 431 "./m2-exp.y"
{ write_exp_elt_opcode (BINOP_LESS); ;
    break;}
case 59:
#line 435 "./m2-exp.y"
{ write_exp_elt_opcode (BINOP_GTR); ;
    break;}
case 60:
#line 439 "./m2-exp.y"
{ write_exp_elt_opcode (BINOP_AND); ;
    break;}
case 61:
#line 443 "./m2-exp.y"
{ write_exp_elt_opcode (BINOP_AND); ;
    break;}
case 62:
#line 447 "./m2-exp.y"
{ write_exp_elt_opcode (BINOP_OR); ;
    break;}
case 63:
#line 451 "./m2-exp.y"
{ write_exp_elt_opcode (BINOP_ASSIGN); ;
    break;}
case 64:
#line 458 "./m2-exp.y"
{ write_exp_elt_opcode (OP_BOOL);
			  write_exp_elt_longcst ((LONGEST) yyvsp[0].ulval);
			  write_exp_elt_opcode (OP_BOOL); ;
    break;}
case 65:
#line 464 "./m2-exp.y"
{ write_exp_elt_opcode (OP_BOOL);
			  write_exp_elt_longcst ((LONGEST) yyvsp[0].ulval);
			  write_exp_elt_opcode (OP_BOOL); ;
    break;}
case 66:
#line 470 "./m2-exp.y"
{ write_exp_elt_opcode (OP_LONG);
			  write_exp_elt_type (builtin_type_m2_int);
			  write_exp_elt_longcst ((LONGEST) yyvsp[0].lval);
			  write_exp_elt_opcode (OP_LONG); ;
    break;}
case 67:
#line 477 "./m2-exp.y"
{
			  write_exp_elt_opcode (OP_LONG);
			  write_exp_elt_type (builtin_type_m2_card);
			  write_exp_elt_longcst ((LONGEST) yyvsp[0].ulval);
			  write_exp_elt_opcode (OP_LONG);
			;
    break;}
case 68:
#line 486 "./m2-exp.y"
{ write_exp_elt_opcode (OP_LONG);
			  write_exp_elt_type (builtin_type_m2_char);
			  write_exp_elt_longcst ((LONGEST) yyvsp[0].ulval);
			  write_exp_elt_opcode (OP_LONG); ;
    break;}
case 69:
#line 494 "./m2-exp.y"
{ write_exp_elt_opcode (OP_DOUBLE);
			  write_exp_elt_type (builtin_type_m2_real);
			  write_exp_elt_dblcst (yyvsp[0].dval);
			  write_exp_elt_opcode (OP_DOUBLE); ;
    break;}
case 71:
#line 505 "./m2-exp.y"
{ write_exp_elt_opcode (OP_LAST);
			  write_exp_elt_longcst ((LONGEST) yyvsp[0].lval);
			  write_exp_elt_opcode (OP_LAST); ;
    break;}
case 72:
#line 511 "./m2-exp.y"
{ write_exp_elt_opcode (OP_REGISTER);
			  write_exp_elt_longcst ((LONGEST) yyvsp[0].lval);
			  write_exp_elt_opcode (OP_REGISTER); ;
    break;}
case 73:
#line 517 "./m2-exp.y"
{ write_exp_elt_opcode (OP_LONG);
			  write_exp_elt_type (builtin_type_int);
			  write_exp_elt_longcst ((LONGEST) TYPE_LENGTH (yyvsp[-1].tval));
			  write_exp_elt_opcode (OP_LONG); ;
    break;}
case 74:
#line 524 "./m2-exp.y"
{ write_exp_elt_opcode (OP_M2_STRING);
			  write_exp_string (yyvsp[0].sval);
			  write_exp_elt_opcode (OP_M2_STRING); ;
    break;}
case 75:
#line 531 "./m2-exp.y"
{ yyval.bval = SYMBOL_BLOCK_VALUE(yyvsp[0].sym); ;
    break;}
case 76:
#line 535 "./m2-exp.y"
{ struct symbol *sym
			    = lookup_symbol (copy_name (yyvsp[0].sval), expression_context_block,
					     VAR_NAMESPACE, 0, NULL);
			  yyval.sym = sym;;
    break;}
case 77:
#line 544 "./m2-exp.y"
{ struct symbol *tem
			    = lookup_symbol (copy_name (yyvsp[0].sval), yyvsp[-2].bval,
					     VAR_NAMESPACE, 0, NULL);
			  if (!tem || SYMBOL_CLASS (tem) != LOC_BLOCK)
			    error ("No function \"%s\" in specified context.",
				   copy_name (yyvsp[0].sval));
			  yyval.sym = tem;
			;
    break;}
case 78:
#line 556 "./m2-exp.y"
{ write_exp_elt_opcode(OP_VAR_VALUE);
			  write_exp_elt_sym (yyvsp[0].sym);
			  write_exp_elt_opcode (OP_VAR_VALUE); ;
    break;}
case 79:
#line 563 "./m2-exp.y"
{ write_exp_elt_opcode (OP_INTERNALVAR);
			  write_exp_elt_intern (yyvsp[0].ivar);
			  write_exp_elt_opcode (OP_INTERNALVAR); ;
    break;}
case 80:
#line 570 "./m2-exp.y"
{ struct symbol *sym;
			  sym = lookup_symbol (copy_name (yyvsp[0].sval), yyvsp[-2].bval,
					       VAR_NAMESPACE, 0, NULL);
			  if (sym == 0)
			    error ("No symbol \"%s\" in specified context.",
				   copy_name (yyvsp[0].sval));

			  write_exp_elt_opcode (OP_VAR_VALUE);
			  write_exp_elt_sym (sym);
			  write_exp_elt_opcode (OP_VAR_VALUE); ;
    break;}
case 81:
#line 584 "./m2-exp.y"
{ struct symbol *sym;
			  int is_a_field_of_this;

 			  sym = lookup_symbol (copy_name (yyvsp[0].sval),
					       expression_context_block,
					       VAR_NAMESPACE,
					       &is_a_field_of_this,
					       NULL);
			  if (sym)
			    {
			      switch (sym->class)
				{
				case LOC_REGISTER:
				case LOC_ARG:
				case LOC_LOCAL:
				case LOC_REF_ARG:
				case LOC_REGPARM:
				case LOC_LOCAL_ARG:
				  if (innermost_block == 0 ||
				      contained_in (block_found,
						    innermost_block))
				    innermost_block = block_found;
				  break;

				case LOC_UNDEF:
				case LOC_CONST:
				case LOC_STATIC:
				case LOC_TYPEDEF:
				case LOC_LABEL:	/* maybe should go above? */
				case LOC_BLOCK:
				case LOC_CONST_BYTES:
				  /* These are listed so gcc -Wall will reveal
				     un-handled cases.  */
				  break;
				}
			      write_exp_elt_opcode (OP_VAR_VALUE);
			      write_exp_elt_sym (sym);
			      write_exp_elt_opcode (OP_VAR_VALUE);
			    }
			  else
			    {
			      struct minimal_symbol *msymbol;
			      register char *arg = copy_name (yyvsp[0].sval);

			      msymbol = lookup_minimal_symbol (arg,
					  (struct objfile *) NULL);
			      if (msymbol != NULL)
				{
				  write_exp_elt_opcode (OP_LONG);
				  write_exp_elt_type (builtin_type_int);
				  write_exp_elt_longcst ((LONGEST) msymbol -> address);
				  write_exp_elt_opcode (OP_LONG);
				  write_exp_elt_opcode (UNOP_MEMVAL);
				  if (msymbol -> type == mst_data ||
				      msymbol -> type == mst_bss)
				    write_exp_elt_type (builtin_type_int);
				  else if (msymbol -> type == mst_text)
				    write_exp_elt_type (lookup_function_type (builtin_type_int));
				  else
				    write_exp_elt_type (builtin_type_char);
				  write_exp_elt_opcode (UNOP_MEMVAL);
				}
			      else if (!have_full_symbols () && !have_partial_symbols ())
				error ("No symbol table is loaded.  Use the \"symbol-file\" command.");
			      else
				error ("No symbol \"%s\" in current context.",
				       copy_name (yyvsp[0].sval));
			    }
			;
    break;}
case 82:
#line 657 "./m2-exp.y"
{ yyval.tval = lookup_typename (copy_name (yyvsp[0].sval),
						expression_context_block, 0); ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 440 "/usr/local/lib/bison.simple"

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
	  for (x = 0; x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) xmalloc(size + 15);
	  strcpy(msg, "parse error");

	  if (count < 5)
	    {
	      count = 0;
	      for (x = 0; x < (sizeof(yytname) / sizeof(char *)); x++)
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
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

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
#line 662 "./m2-exp.y"


#if 0  /* FIXME! */
int
overflow(a,b)
   long a,b;
{
   return (MAX_OF_TYPE(builtin_type_m2_int) - b) < a;
}

int
uoverflow(a,b)
   unsigned long a,b;
{
   return (MAX_OF_TYPE(builtin_type_m2_card) - b) < a;
}
#endif /* FIXME */

/* Take care of parsing a number (anything that starts with a digit).
   Set yylval and return the token type; update lexptr.
   LEN is the number of characters in it.  */

/*** Needs some error checking for the float case ***/

static int
parse_number (olen)
     int olen;
{
  register char *p = lexptr;
  register LONGEST n = 0;
  register LONGEST prevn = 0;
  register int c,i,ischar=0;
  register int base = input_radix;
  register int len = olen;
  int unsigned_p = number_sign == 1 ? 1 : 0;

  if(p[len-1] == 'H')
  {
     base = 16;
     len--;
  }
  else if(p[len-1] == 'C' || p[len-1] == 'B')
  {
     base = 8;
     ischar = p[len-1] == 'C';
     len--;
  }

  /* Scan the number */
  for (c = 0; c < len; c++)
  {
    if (p[c] == '.' && base == 10)
      {
	/* It's a float since it contains a point.  */
	yylval.dval = atof (p);
	lexptr += len;
	return FLOAT;
      }
    if (p[c] == '.' && base != 10)
       error("Floating point numbers must be base 10.");
    if (base == 10 && (p[c] < '0' || p[c] > '9'))
       error("Invalid digit \'%c\' in number.",p[c]);
 }

  while (len-- > 0)
    {
      c = *p++;
      n *= base;
      if( base == 8 && (c == '8' || c == '9'))
	 error("Invalid digit \'%c\' in octal number.",c);
      if (c >= '0' && c <= '9')
	i = c - '0';
      else
	{
	  if (base == 16 && c >= 'A' && c <= 'F')
	    i = c - 'A' + 10;
	  else
	     return ERROR;
	}
      n+=i;
      if(i >= base)
	 return ERROR;
      if(!unsigned_p && number_sign == 1 && (prevn >= n))
	 unsigned_p=1;		/* Try something unsigned */
      /* Don't do the range check if n==i and i==0, since that special
	 case will give an overflow error. */
      if(RANGE_CHECK && n!=i && i)
      {
	 if((unsigned_p && (unsigned)prevn >= (unsigned)n) ||
	    ((!unsigned_p && number_sign==-1) && -prevn <= -n))
	    range_error("Overflow on numeric constant.");
      }
	 prevn=n;
    }

  lexptr = p;
  if(*p == 'B' || *p == 'C' || *p == 'H')
     lexptr++;			/* Advance past B,C or H */

  if (ischar)
  {
     yylval.ulval = n;
     return CHAR;
  }
  else if ( unsigned_p && number_sign == 1)
  {
     yylval.ulval = n;
     return UINT;
  }
  else if((unsigned_p && (n<0))) {
     range_error("Overflow on numeric constant -- number too large.");
     /* But, this can return if range_check == range_warn.  */
  }
  yylval.lval = n;
  return INT;
}


/* Some tokens */

static struct
{
   char name[4];
   int token;
} tokentab2[] =
{
    {"<>",    NOTEQUAL 	 },
    {":=",    ASSIGN	 },
    {"<=",    LEQ	 },
    {">=",    GEQ	 },
    {"::",    COLONCOLON },

};

/* Some specific keywords */

struct keyword {
   char keyw[10];
   int token;
};

static struct keyword keytab[] =
{
    {"OR" ,   OROR	 },
    {"IN",    IN         },/* Note space after IN */
    {"AND",   ANDAND     },
    {"ABS",   ABS	 },
    {"CHR",   CHR	 },
    {"DEC",   DEC	 },
    {"NOT",   NOT	 },
    {"DIV",   DIV    	 },
    {"INC",   INC	 },
    {"MAX",   MAX_FUNC	 },
    {"MIN",   MIN_FUNC	 },
    {"MOD",   MOD	 },
    {"ODD",   ODD	 },
    {"CAP",   CAP	 },
    {"ORD",   ORD	 },
    {"VAL",   VAL	 },
    {"EXCL",  EXCL	 },
    {"HIGH",  HIGH       },
    {"INCL",  INCL	 },
    {"SIZE",  SIZE       },
    {"FLOAT", FLOAT_FUNC },
    {"TRUNC", TRUNC	 },
};


/* Read one token, getting characters through lexptr.  */

/* This is where we will check to make sure that the language and the operators used are
   compatible  */

static int
yylex ()
{
  register int c;
  register int namelen;
  register int i;
  register char *tokstart;
  register char quote;

 retry:

  tokstart = lexptr;


  /* See if it is a special token of length 2 */
  for( i = 0 ; i < sizeof tokentab2 / sizeof tokentab2[0] ; i++)
     if(!strncmp(tokentab2[i].name, tokstart, 2))
     {
	lexptr += 2;
	return tokentab2[i].token;
     }

  switch (c = *tokstart)
    {
    case 0:
      return 0;

    case ' ':
    case '\t':
    case '\n':
      lexptr++;
      goto retry;

    case '(':
      paren_depth++;
      lexptr++;
      return c;

    case ')':
      if (paren_depth == 0)
	return 0;
      paren_depth--;
      lexptr++;
      return c;

    case ',':
      if (comma_terminates && paren_depth == 0)
	return 0;
      lexptr++;
      return c;

    case '.':
      /* Might be a floating point number.  */
      if (lexptr[1] >= '0' && lexptr[1] <= '9')
	break;			/* Falls into number code.  */
      else
      {
	 lexptr++;
	 return DOT;
      }

/* These are character tokens that appear as-is in the YACC grammar */
    case '+':
    case '-':
    case '*':
    case '/':
    case '^':
    case '<':
    case '>':
    case '[':
    case ']':
    case '=':
    case '{':
    case '}':
    case '#':
    case '@':
    case '~':
    case '&':
      lexptr++;
      return c;

    case '\'' :
    case '"':
      quote = c;
      for (namelen = 1; (c = tokstart[namelen]) != quote && c != '\0'; namelen++)
	if (c == '\\')
	  {
	    c = tokstart[++namelen];
	    if (c >= '0' && c <= '9')
	      {
		c = tokstart[++namelen];
		if (c >= '0' && c <= '9')
		  c = tokstart[++namelen];
	      }
	  }
      if(c != quote)
	 error("Unterminated string or character constant.");
      yylval.sval.ptr = tokstart + 1;
      yylval.sval.length = namelen - 1;
      lexptr += namelen + 1;

      if(namelen == 2)  	/* Single character */
      {
	   yylval.ulval = tokstart[1];
	   return CHAR;
      }
      else
	 return STRING;
    }

  /* Is it a number?  */
  /* Note:  We have already dealt with the case of the token '.'.
     See case '.' above.  */
  if ((c >= '0' && c <= '9'))
    {
      /* It's a number.  */
      int got_dot = 0, got_e = 0;
      register char *p = tokstart;
      int toktype;

      for (++p ;; ++p)
	{
	  if (!got_e && (*p == 'e' || *p == 'E'))
	    got_dot = got_e = 1;
	  else if (!got_dot && *p == '.')
	    got_dot = 1;
	  else if (got_e && (p[-1] == 'e' || p[-1] == 'E')
		   && (*p == '-' || *p == '+'))
	    /* This is the sign of the exponent, not the end of the
	       number.  */
	    continue;
	  else if ((*p < '0' || *p > '9') &&
		   (*p < 'A' || *p > 'F') &&
		   (*p != 'H'))  /* Modula-2 hexadecimal number */
	    break;
	}
	toktype = parse_number (p - tokstart);
        if (toktype == ERROR)
	  {
	    char *err_copy = (char *) alloca (p - tokstart + 1);

	    bcopy (tokstart, err_copy, p - tokstart);
	    err_copy[p - tokstart] = 0;
	    error ("Invalid number \"%s\".", err_copy);
	  }
	lexptr = p;
	return toktype;
    }

  if (!(c == '_' || c == '$'
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
    /* We must have come across a bad character (e.g. ';').  */
    error ("Invalid character '%c' in expression.", c);

  /* It's a name.  See how long it is.  */
  namelen = 0;
  for (c = tokstart[namelen];
       (c == '_' || c == '$' || (c >= '0' && c <= '9')
	|| (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
       c = tokstart[++namelen])
    ;

  /* The token "if" terminates the expression and is NOT
     removed from the input stream.  */
  if (namelen == 2 && tokstart[0] == 'i' && tokstart[1] == 'f')
    {
      return 0;
    }

  lexptr += namelen;

  /* Handle the tokens $digits; also $ (short for $0) and $$ (short for $$1)
     and $$digits (equivalent to $<-digits> if you could type that).
     Make token type LAST, and put the number (the digits) in yylval.  */

  if (*tokstart == '$')
    {
      register int negate = 0;
      c = 1;
      /* Double dollar means negate the number and add -1 as well.
	 Thus $$ alone means -1.  */
      if (namelen >= 2 && tokstart[1] == '$')
	{
	  negate = 1;
	  c = 2;
	}
      if (c == namelen)
	{
	  /* Just dollars (one or two) */
	  yylval.lval = - negate;
	  return LAST;
	}
      /* Is the rest of the token digits?  */
      for (; c < namelen; c++)
	if (!(tokstart[c] >= '0' && tokstart[c] <= '9'))
	  break;
      if (c == namelen)
	{
	  yylval.lval = atoi (tokstart + 1 + negate);
	  if (negate)
	    yylval.lval = - yylval.lval;
	  return LAST;
	}
    }

  /* Handle tokens that refer to machine registers:
     $ followed by a register name.  */

  if (*tokstart == '$') {
    for (c = 0; c < NUM_REGS; c++)
      if (namelen - 1 == strlen (reg_names[c])
	  && !strncmp (tokstart + 1, reg_names[c], namelen - 1))
	{
	  yylval.lval = c;
	  return REGNAME;
	}
    for (c = 0; c < num_std_regs; c++)
     if (namelen - 1 == strlen (std_regs[c].name)
	 && !strncmp (tokstart + 1, std_regs[c].name, namelen - 1))
       {
	 yylval.lval = std_regs[c].regnum;
	 return REGNAME;
       }
  }


  /*  Lookup special keywords */
  for(i = 0 ; i < sizeof(keytab) / sizeof(keytab[0]) ; i++)
     if(namelen == strlen(keytab[i].keyw) && !strncmp(tokstart,keytab[i].keyw,namelen))
	   return keytab[i].token;

  yylval.sval.ptr = tokstart;
  yylval.sval.length = namelen;

  /* Any other names starting in $ are debugger internal variables.  */

  if (*tokstart == '$')
    {
      yylval.ivar = (struct internalvar *) lookup_internalvar (copy_name (yylval.sval) + 1);
      return INTERNAL_VAR;
    }


  /* Use token-type BLOCKNAME for symbols that happen to be defined as
     functions.  If this is not so, then ...
     Use token-type TYPENAME for symbols that happen to be defined
     currently as names of types; NAME for other symbols.
     The caller is not constrained to care about the distinction.  */
 {


    char *tmp = copy_name (yylval.sval);
    struct symbol *sym;

    if (lookup_partial_symtab (tmp))
      return BLOCKNAME;
    sym = lookup_symbol (tmp, expression_context_block,
			 VAR_NAMESPACE, 0, NULL);
    if (sym && SYMBOL_CLASS (sym) == LOC_BLOCK)
      return BLOCKNAME;
    if (lookup_typename (copy_name (yylval.sval), expression_context_block, 1))
      return TYPENAME;

    if(sym)
    {
       switch(sym->class)
       {
       case LOC_STATIC:
       case LOC_REGISTER:
       case LOC_ARG:
       case LOC_REF_ARG:
       case LOC_REGPARM:
       case LOC_LOCAL:
       case LOC_LOCAL_ARG:
       case LOC_CONST:
       case LOC_CONST_BYTES:
	  return NAME;

       case LOC_TYPEDEF:
	  return TYPENAME;

       case LOC_BLOCK:
	  return BLOCKNAME;

       case LOC_UNDEF:
	  error("internal:  Undefined class in m2lex()");

       case LOC_LABEL:
	  error("internal:  Unforseen case in m2lex()");
       }
    }
    else
    {
       /* Built-in BOOLEAN type.  This is sort of a hack. */
       if(!strncmp(tokstart,"TRUE",4))
       {
	  yylval.ulval = 1;
	  return TRUE;
       }
       else if(!strncmp(tokstart,"FALSE",5))
       {
	  yylval.ulval = 0;
	  return FALSE;
       }
    }

    /* Must be another type of name... */
    return NAME;
 }
}

#if 0		/* Unused */
static char *
make_qualname(mod,ident)
   char *mod, *ident;
{
   char *new = xmalloc(strlen(mod)+strlen(ident)+2);

   strcpy(new,mod);
   strcat(new,".");
   strcat(new,ident);
   return new;
}
#endif  /* 0 */

static void
yyerror(msg)
     char *msg;	/* unused */
{
   printf("Parsing:  %s\n",lexptr);
   if (yychar < 256)
     error("Invalid syntax in expression near character '%c'.",yychar);
   else
     error("Invalid syntax in expression");
}

/* Table of operators and their precedences for printing expressions.  */

const static struct op_print m2_op_print_tab[] = {
    {"+",   BINOP_ADD, PREC_ADD, 0},
    {"+",   UNOP_PLUS, PREC_PREFIX, 0},
    {"-",   BINOP_SUB, PREC_ADD, 0},
    {"-",   UNOP_NEG, PREC_PREFIX, 0},
    {"*",   BINOP_MUL, PREC_MUL, 0},
    {"/",   BINOP_DIV, PREC_MUL, 0},
    {"DIV", BINOP_INTDIV, PREC_MUL, 0},
    {"MOD", BINOP_REM, PREC_MUL, 0},
    {":=",  BINOP_ASSIGN, PREC_ASSIGN, 1},
    {"OR",  BINOP_OR, PREC_OR, 0},
    {"AND", BINOP_AND, PREC_AND, 0},
    {"NOT", UNOP_ZEROP, PREC_PREFIX, 0},
    {"=",   BINOP_EQUAL, PREC_EQUAL, 0},
    {"<>",  BINOP_NOTEQUAL, PREC_EQUAL, 0},
    {"<=",  BINOP_LEQ, PREC_ORDER, 0},
    {">=",  BINOP_GEQ, PREC_ORDER, 0},
    {">",   BINOP_GTR, PREC_ORDER, 0},
    {"<",   BINOP_LESS, PREC_ORDER, 0},
    {"^",   UNOP_IND, PREC_PREFIX, 0},
    {"@",   BINOP_REPEAT, PREC_REPEAT, 0},
};

/* The built-in types of Modula-2.  */

struct type *builtin_type_m2_char;
struct type *builtin_type_m2_int;
struct type *builtin_type_m2_card;
struct type *builtin_type_m2_real;
struct type *builtin_type_m2_bool;

struct type ** const (m2_builtin_types[]) = 
{
  &builtin_type_m2_char,
  &builtin_type_m2_int,
  &builtin_type_m2_card,
  &builtin_type_m2_real,
  &builtin_type_m2_bool,
  0
};

const struct language_defn m2_language_defn = {
  "modula-2",
  language_m2,
  m2_builtin_types,
  range_check_on,
  type_check_on,
  m2_parse,			/* parser */
  m2_error,			/* parser error function */
  &builtin_type_m2_int,		/* longest signed   integral type */
  &builtin_type_m2_card,		/* longest unsigned integral type */
  &builtin_type_m2_real,		/* longest floating point type */
  "0%XH", "0%", "XH",		/* Hex   format string, prefix, suffix */
  "%oB",  "%",  "oB",		/* Octal format string, prefix, suffix */
  m2_op_print_tab,		/* expression operators for printing */
  LANG_MAGIC
};

/* Initialization for Modula-2 */

void
_initialize_m2_exp ()
{
  /* FIXME:  The code below assumes that the sizes of the basic data
     types are the same on the host and target machines!!!  */

  /* Modula-2 "pervasive" types.  NOTE:  these can be redefined!!! */
  builtin_type_m2_int =
    init_type (TYPE_CODE_INT, sizeof(int), 0,
	       "INTEGER", (struct objfile *) NULL);
  builtin_type_m2_card =
    init_type (TYPE_CODE_INT, sizeof(int), TYPE_FLAG_UNSIGNED,
	       "CARDINAL", (struct objfile *) NULL);
  builtin_type_m2_real =
    init_type (TYPE_CODE_FLT, sizeof(float), 0,
	       "REAL", (struct objfile *) NULL);
  builtin_type_m2_char =
    init_type (TYPE_CODE_CHAR, sizeof(char), TYPE_FLAG_UNSIGNED,
	       "CHAR", (struct objfile *) NULL);
  builtin_type_m2_bool =
    init_type (TYPE_CODE_BOOL, sizeof(int), TYPE_FLAG_UNSIGNED,
	       "BOOLEAN", (struct objfile *) NULL);

  TYPE_NFIELDS(builtin_type_m2_bool) = 2;
  TYPE_FIELDS(builtin_type_m2_bool) = 
     (struct field *) malloc (sizeof (struct field) * 2);
  TYPE_FIELD_BITPOS(builtin_type_m2_bool,0) = 0;
  TYPE_FIELD_NAME(builtin_type_m2_bool,0) = (char *)malloc(6);
  strcpy(TYPE_FIELD_NAME(builtin_type_m2_bool,0),"FALSE");
  TYPE_FIELD_BITPOS(builtin_type_m2_bool,1) = 1;
  TYPE_FIELD_NAME(builtin_type_m2_bool,1) = (char *)malloc(5);
  strcpy(TYPE_FIELD_NAME(builtin_type_m2_bool,1),"TRUE");

  add_language (&m2_language_defn);
}
