#ifndef lint
static char yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93 (BSDI)";
#endif
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define YYEMPTY (-1)
#define YYLEX yylex()
#define yyclearin (yychar=YYEMPTY)
#define yyerrok (yyerrflag=0)
#define YYRECOVERING (yyerrflag!=0)
#define YYPREFIX "yy"
#line 42 "ftpcmd.y"

#ifndef lint
static char sccsid[] = "@(#)ftpcmd.y    5.24 (Berkeley) 2/25/91";
#endif /* not lint */

#include "config.h"
#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/ftp.h>
#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include <pwd.h>
#include <setjmp.h>
#include <syslog.h>
#include <time.h>
#include <string.h>
#include "ftw.h"
#include "extensions.h"
#include "pathnames.h"

extern  int dolreplies;
extern  char ls_long[50];
extern  char ls_short[50];
extern  struct sockaddr_in data_dest;
extern  int logged_in;
extern  struct passwd *pw;
extern  int anonymous;
extern  int logging;
extern  int log_commands;
extern  int type;
extern  int form;
extern  int debug;
extern  int timeout;
extern  int maxtimeout;
extern  int pdata;
extern  char hostname[], remotehost[];
#ifdef SETPROCTITLE
extern  char proctitle[];
#endif
extern  char *globerr;
extern  int usedefault;
extern  int transflag;
extern  char tmpline[];
extern  int data;
char    **ftpglob();
off_t   restart_point;

extern  char    *strunames[];
extern  char    *typenames[];
extern  char    *modenames[];
extern  char    *formnames[];

static  int cmd_type;
static  int cmd_form;
static  int cmd_bytesz;
char    cbuf[512];
char    *fromname;

static void toolong();

#line 127 "ftpcmd.y"
typedef union {
    char    *String;
    int     Number;
} YYSTYPE;
#line 84 "y.tab.c"
#define A 257
#define B 258
#define C 259
#define E 260
#define F 261
#define I 262
#define L 263
#define N 264
#define P 265
#define R 266
#define S 267
#define T 268
#define SP 269
#define CRLF 270
#define COMMA 271
#define STRING 272
#define NUMBER 273
#define USER 274
#define PASS 275
#define ACCT 276
#define REIN 277
#define QUIT 278
#define PORT 279
#define PASV 280
#define TYPE 281
#define STRU 282
#define MODE 283
#define RETR 284
#define STOR 285
#define APPE 286
#define MLFL 287
#define MAIL 288
#define MSND 289
#define MSOM 290
#define MSAM 291
#define MRSQ 292
#define MRCP 293
#define ALLO 294
#define REST 295
#define RNFR 296
#define RNTO 297
#define ABOR 298
#define DELE 299
#define CWD 300
#define LIST 301
#define NLST 302
#define SITE 303
#define STAT 304
#define HELP 305
#define NOOP 306
#define MKD 307
#define RMD 308
#define PWD 309
#define CDUP 310
#define STOU 311
#define SMNT 312
#define SYST 313
#define SIZE 314
#define MDTM 315
#define UMASK 316
#define IDLE 317
#define CHMOD 318
#define GROUP 319
#define GPASS 320
#define NEWER 321
#define MINFO 322
#define INDEX 323
#define EXEC 324
#define ALIAS 325
#define CDPATH 326
#define LEXERR 327
#define YYERRCODE 256
short yylhs[] = {                                        -1,
    0,    0,    0,   11,   11,   11,   11,   11,   11,   11,
   11,   11,   11,   11,   11,   11,   11,   11,   11,   11,
   11,   11,   11,   11,   11,   11,   11,   11,   11,   11,
   11,   11,   11,   11,   11,   11,   11,   11,   11,   11,
   11,   11,   11,   11,   11,   11,   11,   11,   11,   11,
   11,   11,   11,   12,   12,   12,   12,   12,    4,    1,
    1,    5,   13,    7,    7,    7,   14,   14,   14,   14,
   14,   14,   14,   14,   10,   10,   10,    8,    8,    8,
    2,    3,    9,    6,
};
short yylen[] = {                                         2,
    0,    2,    2,    4,    4,    4,    2,    4,    4,    4,
    4,    8,    5,    5,    5,    3,    5,    3,    5,    5,
    2,    5,    4,    2,    3,    5,    2,    4,    2,    5,
    5,    3,    3,    4,    6,    5,    7,    9,    4,    6,
    7,    7,    7,    9,    9,    6,    6,    5,    2,    5,
    5,    2,    2,    5,    4,    4,    6,    4,    1,    0,
    1,    1,   11,    1,    1,    1,    1,    3,    1,    3,
    1,    1,    3,    2,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    0,
};
short yydefred[] = {                                      1,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   84,   84,   84,    0,    0,   84,    0,    0,   84,   84,
   84,   84,    0,    0,    0,    0,   84,   84,   84,   84,
   84,    0,   84,   84,    2,    3,   53,    0,    0,   52,
    0,    7,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   24,    0,    0,    0,    0,    0,   21,    0,
    0,   27,   29,    0,    0,    0,    0,    0,   49,    0,
    0,   59,    0,   61,    0,    0,    0,    0,    0,   71,
    0,    0,   75,   77,   76,    0,   79,   80,   78,    0,
    0,    0,    0,    0,   62,    0,    0,   82,    0,   81,
    0,    0,   25,    0,   18,    0,   16,    0,   84,    0,
   84,   84,   84,   84,   84,    0,    0,    0,    0,    0,
    0,    0,    0,   32,   33,    0,    0,    0,    4,    5,
    0,    6,    0,    0,    0,   74,    8,    9,   10,    0,
    0,    0,    0,   11,   55,    0,   23,    0,    0,    0,
    0,    0,   34,    0,    0,   39,    0,    0,    0,    0,
    0,    0,    0,    0,   56,   58,    0,   28,    0,    0,
    0,    0,    0,    0,   66,   64,   65,   68,   70,   73,
   13,   14,   15,    0,   54,   22,   26,   19,   17,    0,
    0,   36,    0,    0,    0,    0,    0,    0,    0,    0,
    0,   20,   30,   31,   48,   50,   51,    0,    0,   35,
   83,    0,   40,    0,    0,    0,    0,    0,   46,   47,
   57,    0,    0,   37,    0,   41,   42,    0,   43,    0,
    0,   12,    0,    0,    0,    0,   38,   44,   45,    0,
    0,    0,   63,
};
short yydgoto[] = {                                       1,
   75,   99,  100,   73,   96,   46,  178,   90,  212,   86,
   35,   36,   77,   82,
};
short yysindex[] = {                                      0,
 -210, -265, -239, -236, -258, -230, -218, -188, -167, -151,
    0,    0,    0, -150, -149,    0, -148, -147,    0,    0,
    0,    0, -145, -144, -242, -143,    0,    0,    0,    0,
    0, -142,    0,    0,    0,    0,    0, -141, -140,    0,
 -138,    0, -180, -257, -233, -139, -136, -133, -128, -127,
 -122, -124,    0, -120, -215, -203, -191, -302,    0, -119,
 -121,    0,    0, -117, -116, -115, -114, -112,    0, -111,
 -110,    0, -109,    0, -108, -146, -107, -105, -104,    0,
 -229, -103,    0,    0,    0, -102,    0,    0,    0, -101,
 -124, -124, -124, -163,    0, -100, -124,    0,  -99,    0,
 -124, -124,    0, -124,    0, -118,    0, -161,    0, -159,
    0,    0,    0,    0,    0,  -97,  -96, -157,  -95, -124,
  -94, -124, -124,    0,    0, -124, -124, -124,    0,    0,
 -113,    0, -221, -221, -127,    0,    0,    0,    0,  -93,
  -92,  -90, -137,    0,    0,  -89,    0,  -88,  -87,  -86,
  -85, -106,    0, -155,  -84,    0,  -83,  -82,  -81,  -79,
  -78,  -98,  -80,  -77,    0,    0,  -76,    0,  -73,  -72,
  -71,  -70,  -69,  -75,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  -67,    0,    0,    0,    0,    0,  -66,
  -68,    0,  -64,  -68, -141, -140,  -65,  -63,  -62,  -60,
  -59,    0,    0,    0,    0,    0,    0,  -61,  -58,    0,
    0,  -57,    0,  -55,  -54,  -53, -153,  -51,    0,    0,
    0,  -52,  -50,    0, -124,    0,    0, -124,    0, -124,
  -49,    0,  -48,  -47,  -45,  -44,    0,    0,    0,  -43,
  -42,  -41,    0,
};
short yyrindex[] = {                                      0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  -38,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,  -37,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,  -36, -142,    0,
  -35,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,  -37,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,
};
short yygindex[] = {                                      0,
  -17,  -91,    0,   -2,  -74,   29,  -12,    0,    9,    0,
    0,    0,    0,    0,
};
#define YYTABLESIZE 235
short yytable[] = {                                     140,
  141,  142,  108,   83,   37,  146,  136,   84,   85,  148,
  149,   40,  150,  109,  110,  111,  112,  113,  114,  115,
  116,  117,  118,  119,   87,   88,   61,   62,  167,   38,
  169,  170,   39,   89,  171,  172,  173,  175,   41,  135,
   47,   48,  176,   95,   51,    2,  177,   54,   55,   56,
   57,   42,   60,  102,  103,   64,   65,   66,   67,   68,
  180,   70,   71,    3,    4,  104,  105,    5,    6,    7,
    8,    9,   10,   11,   12,   13,   78,  106,  107,   79,
   43,   80,   81,   14,   15,   16,   17,   18,   19,   20,
   21,   22,   23,   24,   25,   26,   27,   28,   29,   30,
   31,   44,   32,   33,   34,  143,  144,  152,  153,  155,
  156,  164,  165,  191,  192,  228,  229,   45,   49,   50,
   52,  179,   53,   58,  131,   59,   63,   69,  184,   91,
   72,   74,   92,  233,   76,   93,  234,  154,  235,  157,
  158,  159,  160,  161,   94,   95,   97,   98,  101,  120,
  121,  122,  123,  151,  124,  125,  126,  127,  128,  174,
  129,  130,  132,  133,  134,  190,  137,  138,  139,  145,
  147,  162,  163,  199,  166,  168,  181,  182,  216,  183,
  185,  186,  187,  188,  189,  194,  195,  196,  193,  197,
  198,  200,  215,  202,  201,  208,  203,  204,  205,  206,
  207,  209,  214,  210,  211,  213,  217,  219,  218,  220,
  221,  222,  224,  225,  223,  226,  227,  230,  231,  232,
    0,  237,  238,  236,  239,    0,  240,    0,  242,  241,
   84,  243,   60,   67,   72,
};
short yycheck[] = {                                      91,
   92,   93,  305,  261,  270,   97,   81,  265,  266,  101,
  102,  270,  104,  316,  317,  318,  319,  320,  321,  322,
  323,  324,  325,  326,  258,  259,  269,  270,  120,  269,
  122,  123,  269,  267,  126,  127,  128,  259,  269,  269,
   12,   13,  264,  273,   16,  256,  268,   19,   20,   21,
   22,  270,   24,  269,  270,   27,   28,   29,   30,   31,
  135,   33,   34,  274,  275,  269,  270,  278,  279,  280,
  281,  282,  283,  284,  285,  286,  257,  269,  270,  260,
  269,  262,  263,  294,  295,  296,  297,  298,  299,  300,
  301,  302,  303,  304,  305,  306,  307,  308,  309,  310,
  311,  269,  313,  314,  315,  269,  270,  269,  270,  269,
  270,  269,  270,  269,  270,  269,  270,  269,  269,  269,
  269,  134,  270,  269,  271,  270,  270,  270,  266,  269,
  272,  272,  269,  225,  273,  269,  228,  109,  230,  111,
  112,  113,  114,  115,  273,  273,  269,  272,  269,  269,
  272,  269,  269,  272,  270,  270,  269,  269,  269,  273,
  270,  270,  270,  269,  269,  272,  270,  270,  270,  270,
  270,  269,  269,  272,  270,  270,  270,  270,  196,  270,
  270,  270,  270,  270,  270,  269,  269,  269,  273,  269,
  269,  272,  195,  270,  272,  271,  270,  270,  270,  270,
  270,  269,  194,  270,  273,  270,  272,  270,  272,  270,
  270,  273,  270,  269,  273,  270,  270,  269,  271,  270,
   -1,  270,  270,  273,  270,   -1,  271,   -1,  271,  273,
  269,  273,  270,  270,  270,
};
#define YYFINAL 1
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 327
#if YYDEBUG
char *yyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"A","B","C","E","F","I","L","N",
"P","R","S","T","SP","CRLF","COMMA","STRING","NUMBER","USER","PASS","ACCT",
"REIN","QUIT","PORT","PASV","TYPE","STRU","MODE","RETR","STOR","APPE","MLFL",
"MAIL","MSND","MSOM","MSAM","MRSQ","MRCP","ALLO","REST","RNFR","RNTO","ABOR",
"DELE","CWD","LIST","NLST","SITE","STAT","HELP","NOOP","MKD","RMD","PWD","CDUP",
"STOU","SMNT","SYST","SIZE","MDTM","UMASK","IDLE","CHMOD","GROUP","GPASS",
"NEWER","MINFO","INDEX","EXEC","ALIAS","CDPATH","LEXERR",
};
char *yyrule[] = {
"$accept : cmd_list",
"cmd_list :",
"cmd_list : cmd_list cmd",
"cmd_list : cmd_list rcmd",
"cmd : USER SP username CRLF",
"cmd : PASS SP password CRLF",
"cmd : PORT SP host_port CRLF",
"cmd : PASV CRLF",
"cmd : TYPE SP type_code CRLF",
"cmd : STRU SP struct_code CRLF",
"cmd : MODE SP mode_code CRLF",
"cmd : ALLO SP NUMBER CRLF",
"cmd : ALLO SP NUMBER SP R SP NUMBER CRLF",
"cmd : RETR check_login SP pathname CRLF",
"cmd : STOR check_login SP pathname CRLF",
"cmd : APPE check_login SP pathname CRLF",
"cmd : NLST check_login CRLF",
"cmd : NLST check_login SP STRING CRLF",
"cmd : LIST check_login CRLF",
"cmd : LIST check_login SP pathname CRLF",
"cmd : STAT check_login SP pathname CRLF",
"cmd : STAT CRLF",
"cmd : DELE check_login SP pathname CRLF",
"cmd : RNTO SP pathname CRLF",
"cmd : ABOR CRLF",
"cmd : CWD check_login CRLF",
"cmd : CWD check_login SP pathname CRLF",
"cmd : HELP CRLF",
"cmd : HELP SP STRING CRLF",
"cmd : NOOP CRLF",
"cmd : MKD check_login SP pathname CRLF",
"cmd : RMD check_login SP pathname CRLF",
"cmd : PWD check_login CRLF",
"cmd : CDUP check_login CRLF",
"cmd : SITE SP HELP CRLF",
"cmd : SITE SP HELP SP STRING CRLF",
"cmd : SITE SP UMASK check_login CRLF",
"cmd : SITE SP UMASK check_login SP octal_number CRLF",
"cmd : SITE SP CHMOD check_login SP octal_number SP pathname CRLF",
"cmd : SITE SP IDLE CRLF",
"cmd : SITE SP IDLE SP NUMBER CRLF",
"cmd : SITE SP GROUP check_login SP username CRLF",
"cmd : SITE SP GPASS check_login SP password CRLF",
"cmd : SITE SP NEWER check_login SP STRING CRLF",
"cmd : SITE SP NEWER check_login SP STRING SP pathname CRLF",
"cmd : SITE SP MINFO check_login SP STRING SP pathname CRLF",
"cmd : SITE SP INDEX SP STRING CRLF",
"cmd : SITE SP EXEC SP STRING CRLF",
"cmd : STOU check_login SP pathname CRLF",
"cmd : SYST CRLF",
"cmd : SIZE check_login SP pathname CRLF",
"cmd : MDTM check_login SP pathname CRLF",
"cmd : QUIT CRLF",
"cmd : error CRLF",
"rcmd : RNFR check_login SP pathname CRLF",
"rcmd : REST SP byte_size CRLF",
"rcmd : SITE SP ALIAS CRLF",
"rcmd : SITE SP ALIAS SP STRING CRLF",
"rcmd : SITE SP CDPATH CRLF",
"username : STRING",
"password :",
"password : STRING",
"byte_size : NUMBER",
"host_port : NUMBER COMMA NUMBER COMMA NUMBER COMMA NUMBER COMMA NUMBER COMMA NUMBER",
"form_code : N",
"form_code : T",
"form_code : C",
"type_code : A",
"type_code : A SP form_code",
"type_code : E",
"type_code : E SP form_code",
"type_code : I",
"type_code : L",
"type_code : L SP byte_size",
"type_code : L byte_size",
"struct_code : F",
"struct_code : R",
"struct_code : P",
"mode_code : S",
"mode_code : B",
"mode_code : C",
"pathname : pathstring",
"pathstring : STRING",
"octal_number : NUMBER",
"check_login :",
};
#endif
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 500
#define YYMAXDEPTH 500
#endif
#endif
int yydebug;
int yynerrs;
int yyerrflag;
int yychar;
short *yyssp;
YYSTYPE *yyvsp;
YYSTYPE yyval;
YYSTYPE yylval;
short yyss[YYSTACKSIZE];
YYSTYPE yyvs[YYSTACKSIZE];
#define yystacksize YYSTACKSIZE
#line 853 "ftpcmd.y"

extern jmp_buf errcatch;

#define CMD 0   /* beginning of command */
#define ARGS    1   /* expect miscellaneous arguments */
#define STR1    2   /* expect SP followed by STRING */
#define STR2    3   /* expect STRING */
#define OSTR    4   /* optional SP then STRING */
#define ZSTR1   5   /* SP then optional STRING */
#define ZSTR2   6   /* optional STRING after SP */
#define SITECMD 7   /* SITE command */
#define NSTR    8   /* Number followed by a string */
#define STR3    9   /* expect STRING followed by optional SP then STRING */

struct tab {
    char    *name;
    short   token;
    short   state;
    short   implemented;    /* 1 if command is implemented */
    char    *help;
};

struct tab cmdtab[] = {     /* In order defined in RFC 765 */
    { "USER", USER, STR1, 1,    "<sp> username" },
    { "PASS", PASS, ZSTR1, 1,   "<sp> password" },
    { "ACCT", ACCT, STR1, 0,    "(specify account)" },
    { "SMNT", SMNT, ARGS, 0,    "(structure mount)" },
    { "REIN", REIN, ARGS, 0,    "(reinitialize server state)" },
    { "QUIT", QUIT, ARGS, 1,    "(terminate service)", },
    { "PORT", PORT, ARGS, 1,    "<sp> b0, b1, b2, b3, b4" },
    { "PASV", PASV, ARGS, 1,    "(set server in passive mode)" },
    { "TYPE", TYPE, ARGS, 1,    "<sp> [ A | E | I | L ]" },
    { "STRU", STRU, ARGS, 1,    "(specify file structure)" },
    { "MODE", MODE, ARGS, 1,    "(specify transfer mode)" },
    { "RETR", RETR, STR1, 1,    "<sp> file-name" },
    { "STOR", STOR, STR1, 1,    "<sp> file-name" },
    { "APPE", APPE, STR1, 1,    "<sp> file-name" },
    { "MLFL", MLFL, OSTR, 0,    "(mail file)" },
    { "MAIL", MAIL, OSTR, 0,    "(mail to user)" },
    { "MSND", MSND, OSTR, 0,    "(mail send to terminal)" },
    { "MSOM", MSOM, OSTR, 0,    "(mail send to terminal or mailbox)" },
    { "MSAM", MSAM, OSTR, 0,    "(mail send to terminal and mailbox)" },
    { "MRSQ", MRSQ, OSTR, 0,    "(mail recipient scheme question)" },
    { "MRCP", MRCP, STR1, 0,    "(mail recipient)" },
    { "ALLO", ALLO, ARGS, 1,    "allocate storage (vacuously)" },
    { "REST", REST, ARGS, 1,    "(restart command)" },
    { "RNFR", RNFR, STR1, 1,    "<sp> file-name" },
    { "RNTO", RNTO, STR1, 1,    "<sp> file-name" },
    { "ABOR", ABOR, ARGS, 1,    "(abort operation)" },
    { "DELE", DELE, STR1, 1,    "<sp> file-name" },
    { "CWD",  CWD,  OSTR, 1,    "[ <sp> directory-name ]" },
    { "XCWD", CWD,  OSTR, 1,    "[ <sp> directory-name ]" },
    { "LIST", LIST, OSTR, 1,    "[ <sp> path-name ]" },
    { "NLST", NLST, OSTR, 1,    "[ <sp> path-name ]" },
    { "SITE", SITE, SITECMD, 1, "site-cmd [ <sp> arguments ]" },
    { "SYST", SYST, ARGS, 1,    "(get type of operating system)" },
    { "STAT", STAT, OSTR, 1,    "[ <sp> path-name ]" },
    { "HELP", HELP, OSTR, 1,    "[ <sp> <string> ]" },
    { "NOOP", NOOP, ARGS, 1,    "" },
    { "MKD",  MKD,  STR1, 1,    "<sp> path-name" },
    { "XMKD", MKD,  STR1, 1,    "<sp> path-name" },
    { "RMD",  RMD,  STR1, 1,    "<sp> path-name" },
    { "XRMD", RMD,  STR1, 1,    "<sp> path-name" },
    { "PWD",  PWD,  ARGS, 1,    "(return current directory)" },
    { "XPWD", PWD,  ARGS, 1,    "(return current directory)" },
    { "CDUP", CDUP, ARGS, 1,    "(change to parent directory)" },
    { "XCUP", CDUP, ARGS, 1,    "(change to parent directory)" },
    { "STOU", STOU, STR1, 1,    "<sp> file-name" },
    { "SIZE", SIZE, OSTR, 1,    "<sp> path-name" },
    { "MDTM", MDTM, OSTR, 1,    "<sp> path-name" },
    { NULL,   0,    0,    0,    0 }
};

struct tab sitetab[] = {
    { "UMASK", UMASK, ARGS, 1,  "[ <sp> umask ]" },
    { "IDLE",  IDLE,  ARGS, 1,  "[ <sp> maximum-idle-time ]" },
    { "CHMOD", CHMOD, NSTR, 1,  "<sp> mode <sp> file-name" },
    { "HELP",  HELP,  OSTR, 1,  "[ <sp> <string> ]" },
    { "GROUP", GROUP, STR1, 1,  "<sp> access-group" },
    { "GPASS", GPASS, STR1, 1,  "<sp> access-password" },
    { "NEWER", NEWER, STR3, 1,  "<sp> YYYYMMDDHHMMSS [ <sp> path-name ]" },
    { "MINFO", MINFO, STR3, 1,  "<sp> YYYYMMDDHHMMSS [ <sp> path-name ]" },
    { "INDEX", INDEX, STR1, 1,  "<sp> pattern" },
    { "EXEC",  EXEC,  STR1, 1,  "<sp> command [ <sp> arguments ]" },
	{ "ALIAS", ALIAS, OSTR, 1,  "[ <sp> alias ] " },
	{ "CDPATH", CDPATH, OSTR, 1,  "[ <sp> ] " },
    { NULL,    0,     0,    0,  0 }
};

struct tab *
lookup(p, cmd)
    register struct tab *p;
    char *cmd;
{

    for (; p->name != NULL; p++)
        if (strcmp(cmd, p->name) == 0)
            return (p);
    return (0);
}

#include <arpa/telnet.h>

/*
 * getline - a hacked up version of fgets to ignore TELNET escape codes.
 */
char *
getline(s, n, iop)
    char *s;
    register FILE *iop;
{
    register c;
    register char *cs;

    cs = s;
/* tmpline may contain saved command from urgent mode interruption */
    for (c = 0; tmpline[c] != '\0' && --n > 0; ++c) {
        *cs++ = tmpline[c];
        if (tmpline[c] == '\n') {
            *cs++ = '\0';
            if (debug)
                syslog(LOG_DEBUG, "command: %s", s);
            tmpline[0] = '\0';
            return(s);
        }
        if (c == 0)
            tmpline[0] = '\0';
    }
    while ((c = getc(iop)) != EOF) {
        c &= 0377;
        if (c == IAC) {
            if ((c = getc(iop)) != EOF) {
            c &= 0377;
            switch (c) {
            case WILL:
            case WONT:
                c = getc(iop);
                printf("%c%c%c", IAC, DONT, 0377&c);
                (void) fflush(stdout);
                continue;
            case DO:
            case DONT:
                c = getc(iop);
                printf("%c%c%c", IAC, WONT, 0377&c);
                (void) fflush(stdout);
                continue;
            case IAC:
                break;
            default:
                continue;   /* ignore command */
            }
            }
        }
        *cs++ = c;
        if (--n <= 0 || c == '\n')
            break;
    }
    if (c == EOF && cs == s)
        return (NULL);
    *cs++ = '\0';
    if (debug)
        syslog(LOG_DEBUG, "command: %s", s);
    return (s);
}

static void
toolong()
{
    time_t now;

    reply(421,
      "Timeout (%d seconds): closing control connection.", timeout);
    (void) time(&now);
    if (logging) {
        syslog(LOG_INFO,
            "User %s timed out after %d seconds at %.24s",
            (pw ? pw -> pw_name : "unknown"), timeout, ctime(&now));
    }
    dologout(1);
}

yylex()
{
    static int cpos, state;
    register char *cp, *cp2;
    register struct tab *p;
    int n;
    char c, *copy();

    for (;;) {
        switch (state) {

        case CMD:
            (void) signal(SIGALRM, toolong);
            (void) alarm((unsigned) timeout);
            if (is_shutdown(!logged_in) != 0) {
                reply(221, "Server shutting down.  Goodbye.");
                dologout(0);
            }
#ifdef SETPROCTITLE
            setproctitle("%s: IDLE", proctitle);
#endif
            if (getline(cbuf, sizeof(cbuf)-1, stdin) == NULL) {
                reply(221, "You could at least say goodbye.");
                dologout(0);
            }
            (void) alarm(0);
#ifdef SETPROCTITLE
            if (strncasecmp(cbuf, "PASS", 4) != 0 &&
                strncasecmp(cbuf, "SITE GPASS", 10) != 0)
                setproctitle("%s: %s", proctitle, cbuf);
#endif /* SETPROCTITLE */
            if ((cp = strchr(cbuf, '\r'))) {
                *cp++ = '\n';
                *cp = '\0';
            }
            if ((cp = strpbrk(cbuf, " \n")))
                cpos = cp - cbuf;
            if (cpos == 0)
                cpos = 4;
            c = cbuf[cpos];
            cbuf[cpos] = '\0';
            upper(cbuf);
            p = lookup(cmdtab, cbuf);
            cbuf[cpos] = c;
            if (p != 0) {
                if (p->implemented == 0) {
                    nack(p->name);
                    longjmp(errcatch,0);
                    /* NOTREACHED */
                }
                state = p->state;
                yylval.String = p->name;
                return (p->token);
            }
            break;

        case SITECMD:
            if (cbuf[cpos] == ' ') {
                cpos++;
                return (SP);
            }
            cp = &cbuf[cpos];
            if ((cp2 = strpbrk(cp, " \n")))
                cpos = cp2 - cbuf;
            c = cbuf[cpos];
            cbuf[cpos] = '\0';
            upper(cp);
            p = lookup(sitetab, cp);
            cbuf[cpos] = c;
            if (p != 0) {
                if (p->implemented == 0) {
                    state = CMD;
                    nack(p->name);
                    longjmp(errcatch,0);
                    /* NOTREACHED */
                }
                state = p->state;
                yylval.String = p->name;
                return (p->token);
            }
            state = CMD;
            break;

        case OSTR:
            if (cbuf[cpos] == '\n') {
                state = CMD;
                return (CRLF);
            }
            /* FALLTHROUGH */

        case STR1:
        case ZSTR1:
        dostr1:
            if (cbuf[cpos] == ' ') {
                cpos++;
                state = state == OSTR ? STR2 : ++state;
                return (SP);
            }
            break;

        case ZSTR2:
            if (cbuf[cpos] == '\n') {
                state = CMD;
                return (CRLF);
            }
            /* FALLTHROUGH */

        case STR2:
            cp = &cbuf[cpos];
            n = strlen(cp);
            cpos += n - 1;
            /*
             * Make sure the string is nonempty and \n terminated.
             */
            if (n > 1 && cbuf[cpos] == '\n') {
                cbuf[cpos] = '\0';
                yylval.String = copy(cp);
                cbuf[cpos] = '\n';
                state = ARGS;
                return (STRING);
            }
            break;

        case NSTR:
            if (cbuf[cpos] == ' ') {
                cpos++;
                return (SP);
            }
            if (isdigit(cbuf[cpos])) {
                cp = &cbuf[cpos];
                while (isdigit(cbuf[++cpos]))
                    ;
                c = cbuf[cpos];
                cbuf[cpos] = '\0';
                yylval.Number = atoi(cp);
                cbuf[cpos] = c;
                state = STR1;
                return (NUMBER);
            }
            state = STR1;
            goto dostr1;

        case STR3:
            if (cbuf[cpos] == ' ') {
                cpos++;
                return (SP);
            }

            cp = &cbuf[cpos];
            cp2 = strpbrk(cp, " \n");
            if (cp2 != NULL) {
                c = *cp2;
                *cp2 = '\0';
            }
            n = strlen(cp);
            cpos += n;
            /*
             * Make sure the string is nonempty and SP terminated.
             */
            if ((cp2 - cp) > 1) {
                yylval.String = copy(cp);
                cbuf[cpos] = c;
                state = OSTR;
                return (STRING);
            }
            break;

        case ARGS:
            if (isdigit(cbuf[cpos])) {
                cp = &cbuf[cpos];
                while (isdigit(cbuf[++cpos]))
                    ;
                c = cbuf[cpos];
                cbuf[cpos] = '\0';
                yylval.Number = atoi(cp);
                cbuf[cpos] = c;
                return (NUMBER);
            }
            switch (cbuf[cpos++]) {

            case '\n':
                state = CMD;
                return (CRLF);

            case ' ':
                return (SP);

            case ',':
                return (COMMA);

            case 'A':
            case 'a':
                return (A);

            case 'B':
            case 'b':
                return (B);

            case 'C':
            case 'c':
                return (C);

            case 'E':
            case 'e':
                return (E);

            case 'F':
            case 'f':
                return (F);

            case 'I':
            case 'i':
                return (I);

            case 'L':
            case 'l':
                return (L);

            case 'N':
            case 'n':
                return (N);

            case 'P':
            case 'p':
                return (P);

            case 'R':
            case 'r':
                return (R);

            case 'S':
            case 's':
                return (S);

            case 'T':
            case 't':
                return (T);

            }
            break;

        default:
            fatal("Unknown state in scanner.");
        }
        yyerror((char *)NULL);
        state = CMD;
        longjmp(errcatch,0);
    }
}

upper(s)
    register char *s;
{
    while (*s != '\0') {
        if (islower(*s))
            *s = toupper(*s);
        s++;
    }
}

char *
copy(s)
    char *s;
{
    char *p;

    p = malloc((unsigned) strlen(s) + 1);
    if (p == NULL)
        fatal("Ran out of memory.");
    (void) strcpy(p, s);
    return (p);
}

help(ctab, s)
    struct tab *ctab;
    char *s;
{
    register struct tab *c;
    register int width, NCMDS;
    char *type;

    if (ctab == sitetab)
        type = "SITE ";
    else
        type = "";
    width = 0, NCMDS = 0;
    for (c = ctab; c->name != NULL; c++) {
        int len = strlen(c->name);

        if (len > width)
            width = len;
        NCMDS++;
    }
    width = (width + 8) &~ 7;
    if (s == 0) {
        register int i, j, w;
        int columns, lines;

        lreply(214, "The following %scommands are recognized %s.",
            type, "(* =>'s unimplemented)");
        columns = 76 / width;
        if (columns == 0)
            columns = 1;
        lines = (NCMDS + columns - 1) / columns;
        for (i = 0; i < lines; i++) {
            printf("   ");
            for (j = 0; j < columns; j++) {
                c = ctab + j * lines + i;
                printf("%s%c", c->name,
                    c->implemented ? ' ' : '*');
                if (c + lines >= &ctab[NCMDS])
                    break;
                w = strlen(c->name) + 1;
                while (w < width) {
                    putchar(' ');
                    w++;
                }
            }
            printf("\r\n");
        }
        (void) fflush(stdout);
        reply(214, "Direct comments to ftp-bugs@%s.", hostname);
        return;
    }
    upper(s);
    c = lookup(ctab, s);
    if (c == (struct tab *)NULL) {
        reply(502, "Unknown command %s.", s);
        return;
    }
    if (c->implemented)
        reply(214, "Syntax: %s%s %s", type, c->name, c->help);
    else
        reply(214, "%s%-*s\t%s; unimplemented.", type, width,
            c->name, c->help);
}

sizecmd(filename)
char *filename;
{
    switch (type) {
    case TYPE_L:
    case TYPE_I: {
        struct stat stbuf;
        if (stat(filename, &stbuf) < 0 ||
            (stbuf.st_mode&S_IFMT) != S_IFREG)
            reply(550, "%s: not a plain file.", filename);
        else
            reply(213, "%lu", stbuf.st_size);
        break;}
    case TYPE_A: {
        FILE *fin;
        register int c;
        register long count;
        struct stat stbuf;
        fin = fopen(filename, "r");
        if (fin == NULL) {
            perror_reply(550, filename);
            return;
        }
        if (fstat(fileno(fin), &stbuf) < 0 ||
            (stbuf.st_mode&S_IFMT) != S_IFREG) {
            reply(550, "%s: not a plain file.", filename);
            (void) fclose(fin);
            return;
        }

        count = 0;
        while((c=getc(fin)) != EOF) {
            if (c == '\n')  /* will get expanded to \r\n */
                count++;
            count++;
        }
        (void) fclose(fin);

        reply(213, "%ld", count);
        break;}
    default:
        reply(504, "SIZE not implemented for Type %c.", "?AEIL"[type]);
    }
}

site_exec(cmd)
char *cmd;
{
    char buf[MAXPATHLEN];
    char *sp = (char *) strchr(cmd, ' '), *slash, *t;
    FILE *cmdf, *ftpd_popen();

    /* sanitize the command-string */
    while (sp && (slash = (char *) strchr(cmd, '/')) && (slash < sp)) {
        cmd = slash+1;
    }
    for (t = cmd;  *t && !isspace(*t);  t++) {
        if (isupper(*t)) {
            *t = tolower(*t);
        }
    }

    /* build the command */
	if (strlen(_PATH_EXECPATH) + strlen(cmd) + 1 > sizeof(buf))
		return;
    sprintf(buf, "%s/%s", _PATH_EXECPATH, cmd);

    cmdf = ftpd_popen(buf, "r", 0);
    if (!cmdf) {
        perror_reply(550, cmd);
        if (log_commands)
            syslog(LOG_INFO, "SITE EXEC (FAIL: %m): %s", cmd);
    } else {
        int lines = 0;

        lreply(200, cmd);
        while (fgets(buf, sizeof buf, cmdf)) {
            int len = strlen(buf);

            if (len>0 && buf[len-1]=='\n')
                buf[--len] = '\0';
            lreply(200, buf);
            if (++lines >= 20) {
                lreply(200, "*** Truncated ***");
                break;
            }
        }
        reply(200, " (end of '%s')", cmd);
        if (log_commands)
            syslog(LOG_INFO, "SITE EXEC (lines: %d): %s", lines, cmd);
        ftpd_pclose(cmdf);
    }
}

alias (s)
char *s;
{
    struct aclmember *entry = NULL;

    if (s != (char *)NULL) {
        while (getaclentry("alias", &entry) && ARG0 && ARG1 != NULL)
            if (!strcmp(ARG0, s)) {
                reply (214, "%s is an alias for %s.", ARG0, ARG1);
                return;
            }
        reply (502, "Unknown alias %s.", s);
        return;
    }

    lreply(214, "The following aliases are available.");

    while (getaclentry("alias", &entry) && ARG0 && ARG1 != NULL)
        printf ("   %-8s %s\r\n", ARG0, ARG1);
    (void) fflush (stdout);

    reply(214, "");
}

cdpath ()
{
    struct aclmember *entry = NULL;

    lreply(214, "The cdpath is:");
    while (getaclentry("cdpath", &entry) && ARG0 != NULL)
        printf ("  %s\r\n", ARG0);
    (void) fflush (stdout);
    reply(214, "");
}

#line 1100 "y.tab.c"
#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab
int
yyparse()
{
    register int yym, yyn, yystate;
#if YYDEBUG
    register char *yys;
    extern char *getenv();

    if (yys = getenv("YYDEBUG"))
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = YYEMPTY;

    yyssp = yyss;
    yyvsp = yyvs;
    *yyssp = yystate = 0;

yyloop:
    if (yyn = yydefred[yystate]) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = YYLEX) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yyssp >= yyss + yystacksize - 1)
        {
            goto yyoverflow;
        }
        *++yyssp = yystate = yytable[yyn];
        *++yyvsp = yylval;
        yychar = YYEMPTY;
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;
#ifdef lint
    goto yynewerror;
#endif
yynewerror:
    yyerror("syntax error");
#ifdef lint
    goto yyerrlab;
#endif
yyerrlab:
    ++yynerrs;
yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yyssp, yytable[yyn]);
#endif
                if (yyssp >= yyss + yystacksize - 1)
                {
                    goto yyoverflow;
                }
                *++yyssp = yystate = yytable[yyn];
                *++yyvsp = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yyssp);
#endif
                if (yyssp <= yyss) goto yyabort;
                --yyssp;
                --yyvsp;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = YYEMPTY;
        goto yyloop;
    }
yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    yyval = yyvsp[1-yym];
    switch (yyn)
    {
case 2:
#line 142 "ftpcmd.y"
 {
            fromname = 0;
            restart_point = 0;
        }
break;
case 4:
#line 150 "ftpcmd.y"
 {
            user(yyvsp[-1].String);
            if (log_commands) syslog(LOG_INFO, "USER %s", yyvsp[-1].String);
            free(yyvsp[-1].String);
        }
break;
case 5:
#line 156 "ftpcmd.y"
 {
            if (log_commands)
                if (anonymous)
                    syslog(LOG_INFO, "PASS %s", yyvsp[-1].String);
                else
                    syslog(LOG_INFO, "PASS password");

            pass(yyvsp[-1].String);
            free(yyvsp[-1].String);
        }
break;
case 6:
#line 167 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "PORT");
            usedefault = 0;
            if (pdata >= 0) {
                (void) close(pdata);
                pdata = -1;
            }
            reply(200, "PORT command successful.");
        }
break;
case 7:
#line 177 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "PASV");
            passive();
        }
break;
case 8:
#line 182 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "TYPE %s", typenames[cmd_type]);
            switch (cmd_type) {

            case TYPE_A:
                if (cmd_form == FORM_N) {
                    reply(200, "Type set to A.");
                    type = cmd_type;
                    form = cmd_form;
                } else
                    reply(504, "Form must be N.");
                break;

            case TYPE_E:
                reply(504, "Type E not implemented.");
                break;

            case TYPE_I:
                reply(200, "Type set to I.");
                type = cmd_type;
                break;

            case TYPE_L:
#if NBBY == 8
                if (cmd_bytesz == 8) {
                    reply(200,
                        "Type set to L (byte size 8).");
                    type = cmd_type;
                } else
                    reply(504, "Byte size must be 8.");
#else /* NBBY == 8 */
                UNIMPLEMENTED for NBBY != 8
#endif /* NBBY == 8 */
            }
        }
break;
case 9:
#line 218 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "STRU %s", strunames[yyvsp[-1].Number]);
            switch (yyvsp[-1].Number) {

            case STRU_F:
                reply(200, "STRU F ok.");
                break;

            default:
                reply(504, "Unimplemented STRU type.");
            }
        }
break;
case 10:
#line 231 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "MODE %s", modenames[yyvsp[-1].Number]);
            switch (yyvsp[-1].Number) {

            case MODE_S:
                reply(200, "MODE S ok.");
                break;

            default:
                reply(502, "Unimplemented MODE type.");
            }
        }
break;
case 11:
#line 244 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "ALLO %d", yyvsp[-1].Number);
            reply(202, "ALLO command ignored.");
        }
break;
case 12:
#line 249 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "ALLO %d R %d", yyvsp[-5].Number, yyvsp[-1].Number);
            reply(202, "ALLO command ignored.");
        }
break;
case 13:
#line 254 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "RETR %s", yyvsp[-1].String);
            if (yyvsp[-3].Number && yyvsp[-1].String != NULL)
                retrieve((char *) NULL, yyvsp[-1].String);
            if (yyvsp[-1].String != NULL)
                free(yyvsp[-1].String);
        }
break;
case 14:
#line 262 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "STOR %s", yyvsp[-1].String);
            if (yyvsp[-3].Number && yyvsp[-1].String != NULL)
                store(yyvsp[-1].String, "w", 0);
            if (yyvsp[-1].String != NULL)
                free(yyvsp[-1].String);
        }
break;
case 15:
#line 270 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "APPE %s", yyvsp[-1].String);
            if (yyvsp[-3].Number && yyvsp[-1].String != NULL)
                store(yyvsp[-1].String, "a", 0);
            if (yyvsp[-1].String != NULL)
                free(yyvsp[-1].String);
        }
break;
case 16:
#line 278 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "NLST");
            if (yyvsp[-1].Number)
                send_file_list(".");
        }
break;
case 17:
#line 284 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "NLST %s", yyvsp[-1].String);
            if (yyvsp[-3].Number && yyvsp[-1].String) {
                send_file_list(yyvsp[-1].String);
                free(yyvsp[-1].String);
            }
        }
break;
case 18:
#line 292 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "LIST");
            if (yyvsp[-1].Number)
		if (anonymous && dolreplies)
                retrieve(ls_long, "");
            else
                retrieve(ls_short, "");
        }
break;
case 19:
#line 301 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "LIST %s", yyvsp[-1].String);
            if (yyvsp[-3].Number && yyvsp[-1].String != NULL)
	    if (anonymous && dolreplies)
                retrieve(ls_long, yyvsp[-1].String);
            else
                retrieve(ls_short, yyvsp[-1].String);
            if (yyvsp[-1].String != NULL)
                free(yyvsp[-1].String);
        }
break;
case 20:
#line 312 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "STAT %s", yyvsp[-1].String);
            if (yyvsp[-3].Number && yyvsp[-1].String != NULL)
                statfilecmd(yyvsp[-1].String);
            if (yyvsp[-1].String != NULL)
                free(yyvsp[-1].String);
        }
break;
case 21:
#line 320 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "STAT");
            statcmd();
        }
break;
case 22:
#line 325 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "DELE %s", yyvsp[-1].String);
            if (yyvsp[-3].Number && yyvsp[-1].String != NULL)
                delete(yyvsp[-1].String);
            if (yyvsp[-1].String != NULL)
                free(yyvsp[-1].String);
        }
break;
case 23:
#line 333 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "RNTO %s", yyvsp[-1].String);
            if (fromname) {
                renamecmd(fromname, yyvsp[-1].String);
                free(fromname);
                fromname = (char *) NULL;
            } else {
                reply(503, "Bad sequence of commands.");
            }
            free(yyvsp[-1].String);
        }
break;
case 24:
#line 345 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "ABOR");
            reply(225, "ABOR command successful.");
        }
break;
case 25:
#line 350 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "CWD");
            if (yyvsp[-1].Number)
                cwd(pw->pw_dir);
        }
break;
case 26:
#line 356 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "CWD %s", yyvsp[-1].String);
            if (yyvsp[-3].Number && yyvsp[-1].String != NULL)
                cwd(yyvsp[-1].String);
            if (yyvsp[-1].String != NULL)
                free(yyvsp[-1].String);
        }
break;
case 27:
#line 364 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "HELP");
            help(cmdtab, (char *) NULL);
        }
break;
case 28:
#line 369 "ftpcmd.y"
 {
            register char *cp = (char *)yyvsp[-1].String;

            if (log_commands) syslog(LOG_INFO, "HELP %s", yyvsp[-1].String);
            if (strncasecmp(cp, "SITE", 4) == 0) {
                cp = (char *)yyvsp[-1].String + 4;
                if (*cp == ' ')
                    cp++;
                if (*cp)
                    help(sitetab, cp);
                else
                    help(sitetab, (char *) NULL);
            } else
                help(cmdtab, yyvsp[-1].String);
        }
break;
case 29:
#line 385 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "NOOP");
            reply(200, "NOOP command successful.");
        }
break;
case 30:
#line 390 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "MKD %s", yyvsp[-1].String);
            if (yyvsp[-3].Number && yyvsp[-1].String != NULL)
                makedir(yyvsp[-1].String);
            if (yyvsp[-1].String != NULL)
                free(yyvsp[-1].String);
        }
break;
case 31:
#line 398 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "RMD %s", yyvsp[-1].String);
            if (yyvsp[-3].Number && yyvsp[-1].String != NULL)
                removedir(yyvsp[-1].String);
            if (yyvsp[-1].String != NULL)
                free(yyvsp[-1].String);
        }
break;
case 32:
#line 406 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "PWD");
            if (yyvsp[-1].Number)
                pwd();
        }
break;
case 33:
#line 412 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "CDUP");
            if (yyvsp[-1].Number)
                cwd("..");
        }
break;
case 34:
#line 418 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "SITE HELP");
            help(sitetab, (char *) NULL);
        }
break;
case 35:
#line 423 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "SITE HELP %s", yyvsp[-1].String);
            help(sitetab, yyvsp[-1].String);
        }
break;
case 36:
#line 428 "ftpcmd.y"
 {
            mode_t oldmask;

            if (log_commands) syslog(LOG_INFO, "SITE UMASK");
            if (yyvsp[-1].Number) {
                oldmask = umask(0);
                (void) umask(oldmask);
                reply(200, "Current UMASK is %03o", oldmask);
            }
        }
break;
case 37:
#line 439 "ftpcmd.y"
 {
            mode_t oldmask;
			struct aclmember *entry = NULL;
			int ok = 1;

            if (log_commands) syslog(LOG_INFO, "SITE UMASK %d", yyvsp[-1].Number);
            if (yyvsp[-3].Number) {
				/* check for umask permission */
    			while (getaclentry("umask", &entry) && ARG0 && ARG1 != NULL) {
					if (type_match(ARG1)) 
          				if (*ARG0 == 'n')  ok = 0;
    			}
                if (ok) {
					if ((yyvsp[-1].Number == -1) || (yyvsp[-1].Number > 0777)) {
                   		reply(501, "Bad UMASK value");
                	} else {
                    	oldmask = umask((mode_t)yyvsp[-1].Number);
                    	reply(200, "UMASK set to %03o (was %03o)", yyvsp[-1].Number, oldmask);
                	}
				} else 
                   	reply(553, "Permission denied. (umask)");
            }
        }
break;
case 38:
#line 463 "ftpcmd.y"
 {
			struct aclmember *entry = NULL;
			int ok = 1;

            if (log_commands) syslog(LOG_INFO, "SITE CHMOD %d %s", yyvsp[-3].Number, yyvsp[-1].String);
            if (yyvsp[-5].Number && yyvsp[-3].Number && yyvsp[-1].String) {
				/* check for chmod permission */
    			while (getaclentry("chmod", &entry) && ARG0 && ARG1 != NULL) {
					if (type_match(ARG1)) 
          				if (*ARG0 == 'n')  ok = 0;
    			}
                if (ok) {
					if (yyvsp[-3].Number > 0777)
						reply(501, 
						    "CHMOD: Mode value must be between 0 and 0777");
					else if (chmod(yyvsp[-1].String, (mode_t) yyvsp[-3].Number) < 0)
						perror_reply(550, yyvsp[-1].String);
					else
						reply(200, "CHMOD command successful.");
					free(yyvsp[-1].String);
				} else
                   	reply(553, "Permission denied. (chmod)");
            }
        }
break;
case 39:
#line 488 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "SITE IDLE");
            reply(200,
                "Current IDLE time limit is %d seconds; max %d",
                timeout, maxtimeout);
        }
break;
case 40:
#line 495 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "SITE IDLE %d", yyvsp[-1].Number);
            if (yyvsp[-1].Number < 30 || yyvsp[-1].Number > maxtimeout) {
                reply(501,
            "Maximum IDLE time must be between 30 and %d seconds",
                    maxtimeout);
            } else {
                timeout = yyvsp[-1].Number;
                (void) alarm((unsigned) timeout);
                reply(200, "Maximum IDLE time set to %d seconds", timeout);
            }
        }
break;
case 41:
#line 508 "ftpcmd.y"
 {
#ifndef NO_PRIVATE
            if (log_commands) syslog(LOG_INFO, "SITE GROUP %s", yyvsp[-1].String);
            if (yyvsp[-3].Number && yyvsp[-1].String) priv_group(yyvsp[-1].String);
            free(yyvsp[-1].String);
#endif /* !NO_PRIVATE */
        }
break;
case 42:
#line 516 "ftpcmd.y"
 {
#ifndef NO_PRIVATE
            if (log_commands) syslog(LOG_INFO, "SITE GPASS password");
            if (yyvsp[-3].Number && yyvsp[-1].String) priv_gpass(yyvsp[-1].String);
            free(yyvsp[-1].String);
#endif /* !NO_PRIVATE */
        }
break;
case 43:
#line 524 "ftpcmd.y"
 {
            if (yyvsp[-3].Number && yyvsp[-1].String) newer(yyvsp[-1].String, ".", 0);
            free(yyvsp[-1].String);
        }
break;
case 44:
#line 529 "ftpcmd.y"
 {
            if (yyvsp[-5].Number && yyvsp[-3].String && yyvsp[-1].String) newer(yyvsp[-3].String, yyvsp[-1].String, 0);
            free(yyvsp[-3].String);
            free(yyvsp[-1].String);
        }
break;
case 45:
#line 535 "ftpcmd.y"
 {
            if (yyvsp[-5].Number && yyvsp[-3].String && yyvsp[-1].String) newer(yyvsp[-3].String, yyvsp[-1].String, 1);
            free(yyvsp[-3].String);
            free(yyvsp[-1].String);
        }
break;
case 46:
#line 541 "ftpcmd.y"
 {
            /* this is just for backward compatibility since we
             * thought of INDEX before we thought of EXEC
             */
            if (yyvsp[-1].String != NULL) {
                char buf[MAXPATHLEN];
				if (strlen(yyvsp[-1].String) + 7 <= sizeof(buf)) {
                    sprintf(buf, "index %s", (char*)yyvsp[-1].String);
                    (void) site_exec(buf);
				}
            }
        }
break;
case 47:
#line 554 "ftpcmd.y"
 {
            if (yyvsp[-1].String != NULL) {
                (void) site_exec((char*)yyvsp[-1].String);
            }
        }
break;
case 48:
#line 560 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "STOU %s", yyvsp[-1].String);
            if (yyvsp[-3].Number && yyvsp[-1].String) {
                store(yyvsp[-1].String, "w", 1);
                free(yyvsp[-1].String);
            }
        }
break;
case 49:
#line 568 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "SYST");
#ifdef unix
#ifdef BSD
            reply(215, "UNIX Type: L%d Version: BSD-%d",
                NBBY, BSD);
#else  /* BSD */
            reply(215, "UNIX Type: L%d", NBBY);
#endif /* BSD */
#else  /* unix */
            reply(215, "UNKNOWN Type: L%d", NBBY);
#endif /* unix */
        }
break;
case 50:
#line 590 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "SIZE %s", yyvsp[-1].String);
            if (yyvsp[-3].Number && yyvsp[-1].String) {
                sizecmd(yyvsp[-1].String);
                free(yyvsp[-1].String);
            }
        }
break;
case 51:
#line 608 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "MDTM %s", yyvsp[-1].String);
            if (yyvsp[-3].Number && yyvsp[-1].String) {
                struct stat stbuf;

                if (stat(yyvsp[-1].String, &stbuf) < 0)
                    perror_reply(550, yyvsp[-1].String);
                else if ((stbuf.st_mode&S_IFMT) != S_IFREG) {
                    reply(550, "%s: not a plain file.",
                        yyvsp[-1].String);
                } else {
                    register struct tm *t;
                    struct tm *gmtime();
                    t = gmtime(&stbuf.st_mtime);
                    reply(213,
                        "19%02d%02d%02d%02d%02d%02d",
                        t->tm_year, t->tm_mon+1, t->tm_mday,
                        t->tm_hour, t->tm_min, t->tm_sec);
                }
                free(yyvsp[-1].String);
            }
        }
break;
case 52:
#line 631 "ftpcmd.y"
 {
            if (log_commands) syslog(LOG_INFO, "QUIT");
            reply(221, "Goodbye.");
            dologout(0);
        }
break;
case 53:
#line 637 "ftpcmd.y"
 {
            yyerrok;
        }
break;
case 54:
#line 642 "ftpcmd.y"
 {
            char *renamefrom();

            if (log_commands) syslog(LOG_INFO, "RNFR %s", yyvsp[-1].String);
            restart_point = (off_t) 0;
            if (yyvsp[-3].Number && yyvsp[-1].String) {
                fromname = renamefrom(yyvsp[-1].String);
                if (fromname == 0 && yyvsp[-1].String) {
                    free(yyvsp[-1].String);
                }
            }
        }
break;
case 55:
#line 655 "ftpcmd.y"
 {
            long atol();

            fromname = 0;
            restart_point = yyvsp[-1].Number;
            if (log_commands) syslog(LOG_INFO, "REST %d", restart_point);
            reply(350, "Restarting at %ld. %s", restart_point,
                "Send STORE or RETRIEVE to initiate transfer.");
        }
break;
case 56:
#line 666 "ftpcmd.y"
 {
           if (log_commands) syslog(LOG_INFO, "SITE ALIAS");
           alias ((char *)NULL);
        }
break;
case 57:
#line 671 "ftpcmd.y"
 {
           if (log_commands) syslog(LOG_INFO, "SITE ALIAS %s", yyvsp[-1].String);
           alias (yyvsp[-1].String);
        }
break;
case 58:
#line 676 "ftpcmd.y"
 {
           if (log_commands) syslog(LOG_INFO, "SITE CDPATH");
           cdpath () ;
        }
break;
case 60:
#line 686 "ftpcmd.y"
 {
            yyval.String = malloc(1);
            yyval.String[0] = '\0';
        }
break;
case 63:
#line 698 "ftpcmd.y"
 {
            register char *a, *p;

            a = (char *)&data_dest.sin_addr;
            a[0] = yyvsp[-10].Number; a[1] = yyvsp[-8].Number; a[2] = yyvsp[-6].Number; a[3] = yyvsp[-4].Number;
            p = (char *)&data_dest.sin_port;
            p[0] = yyvsp[-2].Number; p[1] = yyvsp[0].Number;
            data_dest.sin_family = AF_INET;
        }
break;
case 64:
#line 710 "ftpcmd.y"
 {
        yyval.Number = FORM_N;
    }
break;
case 65:
#line 714 "ftpcmd.y"
 {
        yyval.Number = FORM_T;
    }
break;
case 66:
#line 718 "ftpcmd.y"
 {
        yyval.Number = FORM_C;
    }
break;
case 67:
#line 724 "ftpcmd.y"
 {
        cmd_type = TYPE_A;
        cmd_form = FORM_N;
    }
break;
case 68:
#line 729 "ftpcmd.y"
 {
        cmd_type = TYPE_A;
        cmd_form = yyvsp[0].Number;
    }
break;
case 69:
#line 734 "ftpcmd.y"
 {
        cmd_type = TYPE_E;
        cmd_form = FORM_N;
    }
break;
case 70:
#line 739 "ftpcmd.y"
 {
        cmd_type = TYPE_E;
        cmd_form = yyvsp[0].Number;
    }
break;
case 71:
#line 744 "ftpcmd.y"
 {
        cmd_type = TYPE_I;
    }
break;
case 72:
#line 748 "ftpcmd.y"
 {
        cmd_type = TYPE_L;
        cmd_bytesz = NBBY;
    }
break;
case 73:
#line 753 "ftpcmd.y"
 {
        cmd_type = TYPE_L;
        cmd_bytesz = yyvsp[0].Number;
    }
break;
case 74:
#line 759 "ftpcmd.y"
 {
        cmd_type = TYPE_L;
        cmd_bytesz = yyvsp[0].Number;
    }
break;
case 75:
#line 766 "ftpcmd.y"
 {
        yyval.Number = STRU_F;
    }
break;
case 76:
#line 770 "ftpcmd.y"
 {
        yyval.Number = STRU_R;
    }
break;
case 77:
#line 774 "ftpcmd.y"
 {
        yyval.Number = STRU_P;
    }
break;
case 78:
#line 780 "ftpcmd.y"
 {
        yyval.Number = MODE_S;
    }
break;
case 79:
#line 784 "ftpcmd.y"
 {
        yyval.Number = MODE_B;
    }
break;
case 80:
#line 788 "ftpcmd.y"
 {
        yyval.Number = MODE_C;
    }
break;
case 81:
#line 794 "ftpcmd.y"
 {
        /*
         * Problem: this production is used for all pathname
         * processing, but only gives a 550 error reply.
         * This is a valid reply in some cases but not in others.
         */
        if (logged_in && yyvsp[0].String && strncmp(yyvsp[0].String, "~", 1) == 0) {
            yyval.String = *ftpglob(yyvsp[0].String);
            if (globerr) {
                reply(550, globerr);
                yyval.String = NULL;
            }
            free(yyvsp[0].String);
        } else
            yyval.String = yyvsp[0].String;
    }
break;
case 83:
#line 816 "ftpcmd.y"
 {
        register int ret, dec, multby, digit;

        /*
         * Convert a number that was read as decimal number
         * to what it would be if it had been read as octal.
         */
        dec = yyvsp[0].Number;
        multby = 1;
        ret = 0;
        while (dec) {
            digit = dec%10;
            if (digit > 7) {
                ret = -1;
                break;
            }
            ret += digit * multby;
            multby *= 8;
            dec /= 10;
        }
        yyval.Number = ret;
    }
break;
case 84:
#line 841 "ftpcmd.y"
 {
        if (logged_in)
            yyval.Number = 1;
        else {
            if (log_commands) syslog(LOG_INFO, "cmd failure - not logged in");
            reply(530, "Please login with USER and PASS.");
            yyval.Number = 0;
        }
    }
break;
#line 2053 "y.tab.c"
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
        if (yychar < 0)
        {
            if ((yychar = YYLEX) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yyssp, yystate);
#endif
    if (yyssp >= yyss + yystacksize - 1)
    {
        goto yyoverflow;
    }
    *++yyssp = yystate;
    *++yyvsp = yyval;
    goto yyloop;
yyoverflow:
    yyerror("yacc stack overflow");
yyabort:
    return (1);
yyaccept:
    return (0);
}
