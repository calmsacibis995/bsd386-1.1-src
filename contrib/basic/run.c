#include "structs.h"
#include <signal.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <setjmp.h>

extern jmp_buf	ExecJmpBuf;
struct rusage rusage1, rusage2;

InitProgram() {
	extern	void FloatingExcp();

	InitSymbols();
	OutputCol = 0;
	PCP = FirstLine;
	PC = FirstLine->l_no;
	Stop = 0;
	Break = 0;
	StmtCount = 0;
	GP = GosubStack-1;
	FP = ForStack-1;
	DataPC = 0;
	NextData = (struct tree_node *) NULL;
	AdvanceData();
	DataCount = 0;
	signal(SIGFPE, FloatingExcp);
	getrusage(RUSAGE_SELF, &rusage1);
}



Continue() {
	reg struct line_descr	*ln;
	reg struct tree_node	*t;
	double cputime;

        setjmp(ExecJmpBuf);
	while (!Stop && IsValidLine(ln = PCP)) {
		OldPC = PC;
		PCP = LineAfter(ln);
		PC = PCP->l_no;

		if ( (t = ln->l_tree) == NULL) continue;
#ifdef NO
		if (StmtCount > STMT_LIMIT) {
			Stop = 1;
			Error("Statement Limit Exceeded.");
			break;
		}
#endif
#ifdef DEBUG
		if (Opts['T']) {
			printf("%d ", ln->l_no);
			fflush(stdout);
			if (StmtCount % 10 == 9) printf("\n");
		}
#endif
		if (t->t_op!=T_REM) ExecStmt(t);
		StmtCount++;
		if (Break || SingleStepping || t->t_op == T_BREAK) {
			ln = FindLine(PC);
			if (Stop || !IsValidLine(ln)) {
				printf("\n\tEnd of program");
				break;
			} else {
				if (Break) {
					printf("\tBreak: ");
					Break = 0;
				} else printf("\t");
				printf("Next statement: %6d  ", PC);
				PrintTree(stdout, FindLine(PC)->l_tree);
				putc('\n', stdout);
				return;
			}
		}
	}
	getrusage(RUSAGE_SELF, &rusage2);
	cputime = (rusage2.ru_stime.tv_sec-rusage1.ru_stime.tv_sec) +
          (rusage2.ru_utime.tv_sec-rusage1.ru_utime.tv_sec) +
	  ((rusage2.ru_stime.tv_usec-rusage1.ru_stime.tv_usec) +
	  (rusage2.ru_utime.tv_usec-rusage1.ru_utime.tv_usec))/1000000.0;

	printf("\n%d statements executed, %.2f seconds cpu\n", StmtCount,
			cputime);
	/**InitProgram();**/
}

FlushOutput() {
	fflush(stdout);
	fflush(stderr);
}

NewLine() {
	if (OutputCol > 0) {
		printf("\n");
		OutputCol = 0;
	}
}


# include <setjmp.h>

/*VARARGS1*/
RunError(fmt, a1, a2, a3, a4)	char *fmt; {
	extern jmp_buf	ExecJmpBuf;
	extern int	FromTerminal;

	NewLine();
	printf("\tError");
	if (!FromTerminal) printf(", line %d", OldPC);
	printf(": ");
	printf(fmt, a1, a2, a3, a4);
	putchar('\n');

	Stop = 1;
	longjmp(ExecJmpBuf, 1);
}

/*VARARGS1*/
RunWarning(fmt, a1, a2, a3, a4)	char *fmt; {
	extern int	FromTerminal;

	NewLine();
	printf("\tWarning");
	if (!FromTerminal) printf(", line %d", OldPC);
	printf(": ");
	printf(fmt, a1, a2, a3, a4);
	putchar('\n');
}

FloatingExcp() {
	RunError("Floating-point overflow.");
}
