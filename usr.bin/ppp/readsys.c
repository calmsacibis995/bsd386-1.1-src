
#include "ppp.h"

char   *AT;     /* Auto call unit type. */
int     BR;     /* Baud rate. */
char   *CM;     /* Map of special characters (as in pppconfig) */
char   *CU;     /* Call unit if making a phone call. */
int     DI;     /* Dial-in allowed. */
int     DO;     /* Automatic dial-out allowed. */
int     DU;     /* This is a dial-up line */
char   *DV;     /* Device(s) to open to establish a connection. */
char   *E[10];  /* Strings to wait for on nth step of logging in */
		/* (after sending s0-s9) */
char   *F[10];  /* Strings to send if no expected str appeared */
int     ID;     /* Disconnect on idle (n seconds) */
char   *IF;     /* Space-separated list of ifconfig parameters. */
int     IN;     /* Interface number. */
int     MC;     /* Max PPP config retries. */
int     MR;     /* PPP MRU. */
int     MT;     /* Max PPP terminate retries. */
char   *PF;     /* Comma-separated list of PPP flags (as in pppconfig) */
char   *PN;     /* Telephone number(s) for this host. */
char   *S[10];  /* String to send on nth step of logging in. */
int     T[10];  /* Timeout in seconds for e0-e9 */
int     TO;     /* PPP retrty timeout (1/10 sec). */

/*
 * Capability types
 */
#define STR     0
#define NUM     1
#define BOOL    2

#define P       (char **)

static struct captab {
	char    *name;
	int     type;
	char    **ptr;
} captab[] = {
	"at",   STR,    &AT,
	"br",   NUM,    P &BR,
	"cm",   STR,    &CM,
	"cu",   STR,    &CU,
	"di",   BOOL,   P &DI,
	"do",   BOOL,   P &DO,
	"du",   BOOL,   P &DU,
	"dv",   STR,    &DV,
	"e0",   STR,    &E[0],
	"e1",   STR,    &E[1],
	"e2",   STR,    &E[2],
	"e3",   STR,    &E[3],
	"e4",   STR,    &E[4],
	"e5",   STR,    &E[5],
	"e6",   STR,    &E[6],
	"e7",   STR,    &E[7],
	"e8",   STR,    &E[8],
	"e9",   STR,    &E[9],
	"f0",   STR,    &F[0],
	"f1",   STR,    &F[1],
	"f2",   STR,    &F[2],
	"f3",   STR,    &F[3],
	"f4",   STR,    &F[4],
	"f5",   STR,    &F[5],
	"f6",   STR,    &F[6],
	"f7",   STR,    &F[7],
	"f8",   STR,    &F[8],
	"f9",   STR,    &F[9],
	"id",   NUM,    P &ID,
	"if",   STR,    &IF,
	"in",   NUM,    P &IN,
	"mc",   NUM,    P &MC,
	"mr",   NUM,    P &MR,
	"mt",   NUM,    P &MT,
	"pf",   STR,    &PF,
	"pn",   STR,    &PN,
	"s0",   STR,    &S[0],
	"s1",   STR,    &S[1],
	"s2",   STR,    &S[2],
	"s3",   STR,    &S[3],
	"s4",   STR,    &S[4],
	"s5",   STR,    &S[5],
	"s6",   STR,    &S[6],
	"s7",   STR,    &S[7],
	"s8",   STR,    &S[8],
	"s9",   STR,    &S[9],
	"t0",   NUM,    P &T[0],
	"t1",   NUM,    P &T[1],
	"t2",   NUM,    P &T[2],
	"t3",   NUM,    P &T[3],
	"t4",   NUM,    P &T[4],
	"t5",   NUM,    P &T[5],
	"t6",   NUM,    P &T[6],
	"t7",   NUM,    P &T[7],
	"t8",   NUM,    P &T[8],
	"t9",   NUM,    P &T[9],
	"to",   NUM,    P &TO,
	0,      0,      0
};


char *rgetstr();

void
getsyscap(host, file)
	register char *host;
	char *file;
{
	int stat;
	char tbuf[BUFSIZ];
	static char buf[BUFSIZ/2];
	struct captab *t;
	char *bp;

	if ((stat = xtgetent(tbuf, host, file)) <= 0) {
		fprintf(stderr, stat == 0 ?
			"ppp: unknown host %s\n" :
			"tip: can't open system file\n", host);
		exit(1);
	}

	bp = buf;
	for (t = captab; t->name != 0; t++) {
		switch (t->type) {
		case STR:
			*(t->ptr) = tgetstr(t->name, &bp);
			break;
		case NUM:
			*(int *)(t->ptr) = tgetnum(t->name);
			break;
		case BOOL:
			*(int *)(t->ptr) = tgetflag(t->name);
			break;
		}
	}

	if (DV == 0) {
		fprintf(stderr, "ppp: no device specs for %s\n", host);
		exit(1);
	}
	if (DU && PN == 0) {
		fprintf(stderr, "ppp: missing phone number for %s\n", host);
		exit(1);
	}
}
