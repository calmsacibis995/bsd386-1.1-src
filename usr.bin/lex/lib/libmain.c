/* libmain - flex run-time support library "main" function */

/* $Header: /vol/bsdi/MASTER/BSDI_OS/usr.bin/lex/lib/libmain.c,v 1.3 1994/01/12 11:36:10 donn Exp $ */

extern int yylex();

int main( argc, argv )
int argc;
char *argv[];
	{
	return yylex();
	}
