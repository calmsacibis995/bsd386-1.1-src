/* this file is created automatically by buildzsh */
/* if all this is wrong, blame csh ;-) */

#define SIGCOUNT       31

#ifdef GLOBALS

char *sigmsg[SIGCOUNT+2] = {
	"done",
	"SIGhup",
	"SIGint",
	"SIGquit",
	"SIGill",
	"SIGtrap",
	"SIGabrt",
	"SIGemt",
	"SIGfpe",
	"SIGkill",
	"SIGbus",
	"SIGsegv",
	"SIGsys",
	"SIGpipe",
	"SIGalrm",
	"SIGterm",
	"SIGurg",
	"SIGstop",
	"SIGtstp",
	"SIGcont",
	"SIGchld",
	"SIGttin",
	"SIGttou",
	"SIGio",
	"SIGxcpu",
	"SIGxfsz",
	"SIGvtalrm",
	"SIGprof",
	"SIGwinch",
	"SIGinfo",
	"SIGusr1",
	"SIGusr2",
	NULL
};

char *sigs[SIGCOUNT+4] = {
	"EXIT",
	"hup",
	"int",
	"quit",
	"ill",
	"trap",
	"abrt",
	"emt",
	"fpe",
	"kill",
	"bus",
	"segv",
	"sys",
	"pipe",
	"alrm",
	"term",
	"urg",
	"stop",
	"tstp",
	"cont",
	"chld",
	"ttin",
	"ttou",
	"io",
	"xcpu",
	"xfsz",
	"vtalrm",
	"prof",
	"winch",
	"info",
	"usr1",
	"usr2",
	"ERR",
	"DEBUG",
	NULL
};

#else

extern char *sigs[SIGCOUNT+4],*sigmsg[SIGCOUNT+2];

#endif
