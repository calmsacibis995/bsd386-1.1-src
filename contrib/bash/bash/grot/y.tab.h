typedef union {
  WORD_DESC *word;		/* the word that we read. */
  int number;			/* the number that we read. */
  WORD_LIST *word_list;
  COMMAND *command;
  REDIRECT *redirect;
  ELEMENT element;
  PATTERN_LIST *pattern;
} YYSTYPE;
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


extern YYSTYPE yylval;
