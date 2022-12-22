# include "structs.h"

/*VARARGS1*/
Error(fmt, a1, a2, a3, a4)	char *fmt; {

	if (!TerminalInput && !InputPrinted) {
		InputPrinted = 1;
		printf("%s\n", InputLine);
	}

	fflush(stdout);
	NewLine();
	putc('\t', stdout);
	printf(fmt, a1, a2, a3, a4);
	putc('\n', stdout);
}


/*VARARGS1*/
SyntaxError(col, fmt, a1, a2, a3, a4)	int col; char *fmt; {
	int		i, curcol, c;
	extern char	*PromptString;

	if (!TerminalInput && !InputPrinted) {
		InputPrinted = 1;
		printf("%s\n", InputLine);
	}

	curcol = 1;
	for (i = 0; i < col+(TerminalInput? strlen(PromptString): 0)-1; ++i) {
		if (InputLine[i] == '\t') {
			c = 8 - (curcol&7);
			curcol += c;
			while (c--) putc('-', stderr);
		} else {
			putc('-', stderr);
			++curcol;
		}
	}
	printf("^---");
	printf(fmt, a1, a2, a3, a4);
	putc('\n', stdout);
}
