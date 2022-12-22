/* psof.c - lprps `of' filter for banner printing */

/* Assumes that the `sb' capability is present, that the `sh' capability
is not present, and that there is an `if' filter. */

/* TODO: parse height and width command line arguments and pass them
as definitions to banner prog */

#ifndef lint
static char rcsid[] = "$Id: psof.c,v 1.2 1994/01/13 17:53:40 sanders Exp $";
#endif

#include <stdio.h>
#include <strings.h>
#include <signal.h>
#include <syslog.h>
#include <errno.h>
#include <ctype.h>

#ifndef errno
extern int errno;
#endif

#ifndef LPRPS
#define LPRPS "/usr/contrib/bin/lprps"
#endif /* not LPRPS */

#ifndef BANNER
#define BANNER "/usr/contrib/lib/lprps/banner.ps"
#endif /* not BANNER */

#define JOB_PREFIX "  Job: "
#define DATE_PREFIX "  Date: "
#define LINE_MAX 1024

static char class[32];
static char user[32];
static char job[100];
static char date[100];

static void sys_error();
static void error();
static int parse_banner();
static int define_string();
static int do_banner();
static void print_banner();

static char *xstrstr();

int main(argc, argv)
int argc;
char **argv;
{
  char buf[LINE_MAX];
  char *prog;

  if (argc > 0) {
    if ((prog = rindex(argv[0], '/')) == 0)
      prog = argv[0];
    else
      prog++;
  }
  else
    prog = "psof";

  openlog(prog, LOG_PID, LOG_LPR);

  /* lpd gets indigestion if the output filter exits unexpectedly,
     so we press on even when errors are encountered. */
  for (;;) {
    int i = 0;
    int c;
    int too_long_message = 0;
    int junk_message = 0;

    while ((c = getchar()) != EOF && c != '\n' && c != '\031') {
      if (c != '\f') {
	if (i >= LINE_MAX - 1) {
	  if (!too_long_message) {
	    error("banner line too long");
	    too_long_message = 1;
	  }
	}
	else
	  buf[i++] = c;
      }
    }

    if (!too_long_message && i > 0) {
      buf[i] = '\0';
      if (parse_banner(buf))
	print_banner();
      else
	error("bad banner line");
    }

    /* Skip till Control-Y Control-A sequence or end of file. */
    for (;;) {
      if (c == '\031') {
	c = getchar();
	if (c == '\001')
	  break;
	if (c != EOF)
	  ungetc(c, stdin);
	c = '\031';
      }
      if (c == EOF)
	exit(0);
      if (c != '\n' && c != '\f') {
	if (!junk_message) {
	  if (i > 0)
	    error("junk after banner line");
	  else
	    error("bad banner format (missing `sb' capability?)");
	  junk_message = 1;
	}
      }
      c = getchar();
    }

    if (kill(getpid(), SIGSTOP) < 0)
      sys_error("kill");
  }
  /*NOTREACHED*/
}

static int parse_banner(s)
     char *s;
{
  char *p, *q;

  class[0] = date[0] = user[0] = job[0] = '\0';
  
  p = xstrstr(s, JOB_PREFIX);
  if (!p)
    return 0;
  *p = 0;
  q = index(s, ':');
  if (q == 0) {
    if (strlen(s) + 1 > sizeof(user))
      return 0;
    (void)strcpy(user, s);
  }
  else {
    if (strlen(q + 1) + 1 > sizeof(user))
      return 0;
    (void)strcpy(user, q + 1);
    if (q - s + 1 > sizeof(class))
      return 0;
    bcopy(s, class, q - s);
    class[q - s] = '\0';
  }
  p += sizeof(JOB_PREFIX) - 1;
  q = xstrstr(p, DATE_PREFIX);
  if (!q)
    return 0;
  if (q - p + 1 > sizeof(job))
    return 0;
  bcopy(p, job, q - p);
  job[q - p] = '\0';
  q += sizeof(DATE_PREFIX) - 1;
  if (strlen(q) + 1 > sizeof(date))
    return 0;
  (void)strcpy(date, q);
  return 1;
}

static void print_banner()
{
  int fd[2];
  FILE *psfp;
  int status;
  int pid;
  int ret;
  int write_error = 0;

  if (signal(SIGPIPE, SIG_IGN) == BADSIG) /* in case lprps dies */
    sys_error("signal");
  for (;;) {
    if (pipe(fd) < 0)
      sys_error("pipe");
    if ((pid = fork()) < 0)
      sys_error("fork");
    else if (pid == 0) {
      /* child */
      if (close(fd[1]) < 0)
	sys_error("close");
      if (dup2(fd[0], 0) < 0)
	sys_error("dup2");
      if (close(fd[0]) < 0)
	sys_error("close");
      (void)execl(LPRPS, LPRPS, (char *)0);
      sys_error("exec");
    }
    /* parent */
    if (close(fd[0]) < 0)
      sys_error("close");
    if ((psfp = fdopen(fd[1], "w")) == NULL)
      sys_error("fdopen");
    if (do_banner(psfp) < 0) {
      if (errno != EPIPE)
	sys_error("write");
      write_error = 1;
      clearerr(psfp);
      (void)fclose(psfp);
    }
    if (fclose(psfp) < 0) {
      if (errno != EPIPE)
	sys_error("fclose");
      write_error = 1;
    }
    while ((ret = wait(&status)) != pid)
      if (ret < 0)
	sys_error("wait");
    status &= 0xffff;		/* anal */
    if ((status & 0xff) != 0) {
      int sig = status & 0x7f;
      if (sig < NSIG) {
	extern char *sys_siglist[];
	syslog(LOG_ERR, "%s: %s%s", LPRPS, sys_siglist[sig],
	       (status & 0x80) ? " (core dumped)" : "");
      }
      else
	syslog(LOG_ERR, "%s: signal %d%s", LPRPS, sig,
	      (status & 0x80) ? " (core dumped)" : "");
      return;
    }
    else {
      int exit_status = (status >> 8) & 0xff;
      switch (exit_status) {
      case 0:
	if (write_error)
	  syslog(LOG_ERR, "got EPIPE but exit status was 0");
	return;
      case 1:
	/* this will happen if the printer's not turned on */
	continue;
      case 2:
	return;
      default:
	syslog(LOG_ERR, "`%s' exited with status %d", LPRPS, exit_status);
	return;
      }
    }
  }
}

/* Return -1 for a write error, 0 otherwise. */

static int do_banner(psfp)
     FILE *psfp;
{
  FILE *infp;
  int c;

  infp = fopen(BANNER, "r");
  if (!infp)
    sys_error(BANNER);
  if (fputs("%!\n", psfp) < 0)
    return -1;
  if (define_string("Job", job, psfp) < 0
      || define_string("Class", class, psfp) < 0
      || define_string("User", user, psfp) < 0
      || define_string("Date", date, psfp) < 0)
    return -1;
  
  while ((c = getc(infp)) != EOF)
    if (putc(c, psfp) < 0)
      return -1;
  return 0;
}

/* Return -1 for a write error, 0 otherwise. */

static int define_string(name, str, psfp)
     char *name, *str;
     FILE *psfp;
{
  if (fprintf(psfp, "/%s (", name) < 0)
    return -1; 
  for (; *str; str++) {
    if (*str == ')' || *str == '(' || *str == '\\') {
      if (fprintf(psfp, "\\%c", *str) < 0)
	return -1;
    }
    else if (isascii(*str) && isprint(*str)) {
      if (putc(*str, psfp) < 0)
	return -1;
    }
    else  {
      if (fprintf(psfp, "\\%03o", (unsigned char)*str) < 0)
	return -1;
    }
  }
  if (fprintf(psfp, ") def\n") < 0)
    return -1;
  return 0;
}

static void sys_error(s)
char *s;
{
  syslog(LOG_ERR, "%s: %m", s);
  exit(1);
}

static void error(s)
char *s;
{
  syslog(LOG_ERR, "%s", s);
}

/* same as ANSI strstr */

static char *xstrstr(p, q)
     char *p, *q;
{
  int len = strlen(q);
  do {
    if (strncmp(p, q, len) == 0)
      return p;
  } while (*p++ != '\0');
  return 0;
}
