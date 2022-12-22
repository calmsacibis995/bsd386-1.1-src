# include "structs.h"
# include <signal.h>
# include <setjmp.h>
#include <sys/ioctl.h>
struct winsize winsize;

/*ARGSUSED*/
main(argc, argv)	char *argv[]; {
	reg char *arg;
	extern jmp_buf	CmdJmpBuf;
	extern	void CatchInterrupt();

	if (isatty(0))
		printf("\tWelcome to Basic.  Type 'help' if you need it.\n");
	AllocFastTab();
	InitProgram();
	while ((arg = *++argv) != NULL) {
		if (arg[0] != '-') {
			LoadFile(arg, 1);
		} else {
#ifdef DEBUG
			while (*++arg != '\0') 
				Opts[*arg] = 1;
#else
			fprintf (stderr, "Must recompile with DEBUG option\n");
#endif
		}
	}
	TerminalInput = isatty(fileno(stdin));
	screenwidth =80;
	if (TerminalInput || Opts['p']) {
		ioctl(0, TIOCGWINSZ, &winsize);
		screenwidth = winsize.ws_col;
		if (setjmp(CmdJmpBuf) != 0) {
			fflush(stdout);
			fflush(stderr);
			printf("\tInterrupt!\n");
		}
		signal(SIGINT, CatchInterrupt);
		TerminalInput = isatty(fileno(stdin));
	}
	DoFile(stdin, TerminalInput || Opts['p']);
	PrintUsage();
}

char	*PromptString = "> ";

DoFile(f, prompt)	FILE *f; {
	while (!TimeToQuit) {
		if (prompt) {
			NewLine();
			printf("%s", PromptString);
			FlushOutput();
		}
		if (fgets(InputLine, LINE_LEN, f) == NULL) {
			if (!isatty(fileno(f))) break;
			clearerr(f);
			if (CanQuit()) break;
			continue;
		} 
		if (InputLine[0] == '!') {
			system(InputLine + 1);
			continue;
		}
		InputPrinted = 0;
		InitLexicon();
		yyparse();
	}
}

LoadFile(name, verbose)	char *name; {
	FILE	*f;
	int	tsave;

	f = fopen(name, "r");
	if (f == NULL) {
		Error("Can't open file `%s'", name);
		return;
	}
	ClearBuffer();
	(void)strcpy(CurFile, name);
	tsave = TerminalInput;
	TerminalInput = 0;
	DoFile(f, 0);
	fclose(f);
	TerminalInput = tsave;
	ClearChanges();
	if (verbose)
		printf("\t\"%s\", %d lines loaded.\n", name, LineCount());
}

# include <sys/time.h>
# include <sys/resource.h>

PrintUsage() {
	struct rusage	r;
	int		cpu, textcore, datacore;

	getrusage(RUSAGE_SELF, &r);
	NewLine();
	cpu = (r.ru_utime.tv_sec + r.ru_stime.tv_sec) * 1000 + 
			(r.ru_utime.tv_usec + r.ru_stime.tv_usec) / 1000;
	datacore = (r.ru_idrss + r.ru_isrss)/(2*cpu);
	textcore = r.ru_ixrss/(2*cpu);
	printf("\t%.2f cpu secs used, %d+%dk memory, %d+%d page faults.\n",
		cpu/1000.0, textcore, datacore, r.ru_majflt, r.ru_minflt);
}


jmp_buf	CmdJmpBuf;

void
CatchInterrupt() {
	longjmp(CmdJmpBuf, 1);
}
