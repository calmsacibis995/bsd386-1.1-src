/* psif.c */

#ifndef lint
static char rcsid[] = "$Id: psif.c,v 1.2 1994/01/13 17:53:37 sanders Exp $";
#endif

#include <stdio.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>

off_t lseek();

#ifndef TEXT_FILTER
#define TEXT_FILTER "/usr/contrib/lib/lprps/psif-text"
#endif /* not TEXT_FILTER */

#ifndef PS_FILTER
#define PS_FILTER "/usr/contrib/lib/lprps/psif-ps"
#endif /* not PS_FILTER */

#define EXIT_SUCCESS 0
#define EXIT_REPRINT 1
#define EXIT_THROW_AWAY 2

char *prog;

void sys_error(s)
char *s;
{
  if (prog)
    (void)fprintf(stderr, "%s: ", prog);
  perror(s);
  exit(EXIT_THROW_AWAY);
}


/* ARGSUSED */
main(argc, argv)
int argc;
char **argv;
{
  char buf[BUFSIZ];
  char *bufp = buf;
  char *command;
  int nread = 0;
  int eof = 0;
  struct stat statb;
  prog = argv[0];

  openlog(prog, LOG_PID|LOG_NDELAY, LOG_LPR);

  /* Read the first block. */
  while (nread < BUFSIZ) {
    int ret = read(0, buf + nread, BUFSIZ - nread);
    if (ret < 0)
      sys_error("read");
    if (ret == 0) {
      eof = 1;
      break;
    }
    nread += ret;
  }
  /* Assume a file is binary if it contains a NUL in the first block. */
  if (memchr(buf, '\0', nread)) {
    fprintf(stderr, "%s: binary file rejected\n", prog);
    syslog(LOG_INFO, "%s: binary file rejected", prog);
    exit(EXIT_THROW_AWAY);
  }
  /* Ignore initial Ctrl-D. */
  if (nread > 0 && buf[0] == '\004') {
    bufp++;
    nread--;
  }
  if (nread < 2 || bufp[0] != '%' || bufp[1] != '!')
    command = TEXT_FILTER;
  else
    command = PS_FILTER;
  if (fstat(0, &statb) < 0)
    sys_error("fstat");
  if ((statb.st_mode & S_IFMT) != S_IFREG) {
    /* this happens with lpr -p */
    int pid;
    int fd[2];
    if (pipe(fd) < 0)
      sys_error("pipe");
    if ((pid = fork()) < 0)
      sys_error("fork");
    else if (pid == 0) {
      /* in the child */
      int d;
      if (close(fd[0]) < 0)
	sys_error("close");
      d = fd[1];
      if (nread != 0) {
	if (write(d, bufp, nread) < 0)
	  sys_error("write");
	if (!eof) {
	  while ((nread = read(0, buf, sizeof(buf))) > 0)
	    if (write(d, buf, nread) < 0)
	      sys_error("write");
	  if (nread < 0)
	    sys_error("read");
	}
      }
      if (close(d) < 0)
	sys_error("close");
      exit(0);
    }
    if (close(fd[1]) < 0)
      sys_error("close");
    if (close(0) < 0)
      sys_error("close");
    if (dup(fd[0]) < 0)
      sys_error("dup");
    if (close(fd[0]) < 0)
      sys_error("close");
  }
  else {
    if (lseek(0, (off_t)-nread, 1) < 0)
      sys_error("lseek");
  }
  execv(command, argv);
  sys_error("execv");
}
