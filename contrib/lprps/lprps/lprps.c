/* lprps.c */

#ifndef lint
static char rcsid[] = "$Id: lprps.c,v 1.2 1994/01/13 17:53:29 sanders Exp $";
#endif

#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <signal.h>
#include <syslog.h>
#include <errno.h>
#if (BSD >= 199103)
#include <paths.h>
#endif

#ifndef errno
extern int errno;
#endif

#ifdef __STDC__
#define VOLATILE volatile
#else
#define VOLATILE /* as nothing */
#endif

#ifndef _PATH_SENDMAIL
#define _PATH_SENDMAIL "/usr/lib/sendmail";
#endif
static char sendmail[] = _PATH_SENDMAIL;

#if !defined(FREAD) || !defined(FWRITE)
#define FREAD           0x0001
#define FWRITE          0x0002
#endif

#define EXIT_SUCCESS 0
#define EXIT_REPRINT 1
#define EXIT_THROW_AWAY 2

#define OBSIZE 1024
#define IBSIZE 1024

/* number of times to try to get the status from the printer at the
beginning of the job */
#define MAX_TRIES 16

char ctrl_d = '\004';
char ctrl_t = '\024';

char *malloc();

/* user's login name */
char *login = 0;
/* user's host */
char *host = 0;
/* name of accounting file */
char *accounting_file = 0;
/* stream to mailer process or log file */
FILE *mailfp = 0;
/* pid of the mailer process */
int mail_pid = -1;
/* set to non-zero on interrupt */
VOLATILE int interrupt_flag = 0;
/* set to non-zero on timeout */
VOLATILE int timeout_flag = 0;
/* if non-zero exit on timeout */
int exit_on_timeout = 0;
/* descriptor of printer device */
int psfd = 1;
/* number of ^D's received from printer */
int eof_count = 0;
/* if non-zero ignore all input from printer other than 004s */
int ignore_input = 0;
/* are we currently printing the users job */
int in_job = 0;
/* the contents of the status file at the beginning of the job */
char *status_file_contents = NULL;
/* name of the printer, or NULL if unknown */
char *printer_name = NULL;
/* non-zero if the contents of the status file are not status_file_contents */
int status_file_changed = 0;
/* non-zero if we need to ask the printer for its status, so that we can tell
when a printer error has been corrected */
int want_status = 0;
/* name of the status file */
char *status_filename = "status";
/* name of the job, or NULL if unknown */
char *job_name = NULL;
/* non-zero if errors from the printer should be mailed back to the user */
int mail_flag = 1;

enum {
  NORMAL,
  HAD_ONE_PERCENT,
  HAD_TWO_PERCENT,
  IN_MESSAGE,
  HAD_RIGHT_BRACKET,
  HAD_RIGHT_BRACKET_AND_PERCENT
  } parse_state;

enum {
  INVALID,
  UNKNOWN,
  WARMING_UP,
  INITIALIZING,
  IDLE,
  BUSY,
  WAITING,
  PRINTING,
  PRINTER_ERROR,
  } status = INVALID;

int start_pagecount = -1;
int end_pagecount = -1;

struct {
  char *ptr;
  int count;
  char buf[OBSIZE];
} out = { out.buf, OBSIZE };

#define MBSIZE 1024

char message_buf[MBSIZE];
int message_buf_i;

/* last printer error message */
char error_buf[128];

static char pagecount_string[] = "(%%[ pagecount: ) print \
statusdict begin pagecount end 20 string cvs print \
( ]%%) print flush\n";

void process_input_char();
void printer_flushing();
void handle_interrupt();
void do_exit();
void print_status_file();
void write_status_file();
void restore_status_file();
void handle_printer_error();
char *xmalloc();
char *strsignal();


void handle_timeout()
{
  syslog(LOG_ERR, "%s is not responding",
	 printer_name ? printer_name : "printer");
  print_status_file("%s is not responding",
		    printer_name ? printer_name : "printer");
  sleep(60);		/* it will take at least this long to warm up */
  if (exit_on_timeout)
    do_exit(EXIT_REPRINT);
}

/*VARARGS1*/
void sys_error(s, a1, a2)
char *s;
char *a1, a2;
{
  char buf[BUFSIZ];

  (void)sprintf(buf, s, a1, a2);
  if (printer_name)
    syslog(LOG_ERR, "%s: %s: %m", printer_name, buf);
  else
    syslog(LOG_ERR, "%s: %m", buf);
  exit(EXIT_THROW_AWAY);
}

/*VARARGS1*/
void error(s, a1, a2)
char *s;
char *a1, a2;
{
  char buf[BUFSIZ];

  (void)sprintf(buf, s, a1, a2);
  if (printer_name)
    syslog(LOG_ERR, "%s: %s", printer_name, buf);
  else
    syslog(LOG_ERR, "%s", buf);
  exit(EXIT_THROW_AWAY);
}

int blocking_flag = 1;

void set_blocking()
{
  if (!blocking_flag) {
    if (fcntl(psfd, F_SETFL, 0) < 0)
      sys_error("fcntl");
    blocking_flag = 1;
  }
}

void set_non_blocking()
{
  if (blocking_flag) {
    if (fcntl(psfd, F_SETFL, FNDELAY) < 0)
      sys_error("fcntl");
    blocking_flag = 0;
  }
}

void blocking_write(s, n)
char *s;
int n;
{
  set_blocking();
  if (write(psfd, s, n) < 0)
    sys_error("write");
}

void ioflush()
{
  int rw = FREAD|FWRITE;
  if (ioctl(psfd, TIOCFLUSH, (char *)&rw) < 0)
    sys_error("ioctl(TIOCFLUSH)");
}

void open_mailfp()
{
  if (mailfp == 0) {
    if (!login || !host || !mail_flag)
      mailfp = stderr;
    else {
      int fd[2];
      if (pipe(fd) < 0)
	sys_error("pipe");
      if ((mail_pid = fork()) == 0) {
	/* child */
	char buf[1024], *cp;

	if (close(fd[1]) < 0)
	  sys_error("close");
	if (dup2(fd[0], 0) < 0)
	  sys_error("dup2");
	if (close(fd[0]) < 0)
	  sys_error("close");
	/* don't leave stdout connected to the printer */
	if (close(1) < 0)
	  sys_error("close");
	if (open("/dev/null", O_WRONLY) < 0)
	  sys_error("can't open /dev/null");
	/* the parent catches SIGINT */
	if (signal(SIGINT, SIG_IGN) == BADSIG)
	  sys_error("signal");
	(void)sprintf(buf, "%s@%s", login, host);
	if ((cp = rindex(sendmail, '/')) != NULL)
	  cp++;
	else
	  cp = sendmail;
	(void)execl(sendmail, cp, buf, (char *)NULL);
	sys_error("can't exec %s", sendmail);
      }
      else if (mail_pid < 0)
	sys_error("fork");
      /* parent */
      if (close(fd[0]) < 0)
	sys_error("close");
      mailfp = fdopen(fd[1], "w");
      if (!mailfp)
	sys_error("fdopen");
      (void)fprintf(mailfp, "To: %s@%s\nSubject: printer job\n\n", login, host);
    }
  }
}

void user_char(c)
char c;
{
  static int done_intro = 0;
  if (in_job && (done_intro || c != '\n')) {
    if (mailfp == 0)
      open_mailfp();
    if (!done_intro) {
      (void)fputs("Your printer job ", mailfp);
      if (job_name && *job_name)
	(void)fprintf(mailfp, "(%s) ", job_name);
      (void)fputs("produced the following output:\n", mailfp);
      done_intro = 1;
    }
    (void)putc(c, mailfp);
  }
}

#if 0
void init_tty()
{
  struct termios t;
  psfd = open("/dev/ttya", O_RDWR);
  if (psfd < 0)
    sys_error("open");
  if (ioctl(psfd, TCGETS, &t) < 0)
    sys_error("ioctl(TCGETS)");
  t.c_cflag = B38400|CS7|CSTOPB|CREAD|CLOCAL|PARENB;
  t.c_oflag &= ~OPOST;
  t.c_iflag = IXON|IXOFF|IGNBRK|ISTRIP|IGNCR;
  t.c_lflag = 0;
  t.c_cc[VMIN] = 1;
  t.c_cc[VTIME] = 0;
  if (ioctl(psfd, TCSETS, &t) < 0)
    sys_error("ioctl(TCSETS)");
}
#endif

#if 0
#include <sys/termios.h>
void debug_tty()
{
  struct termios t;
  if (ioctl(psfd, TCGETS, &t) < 0)
    sys_error("ioctl(TCGETS)");
  syslog(LOG_ERR, "cflag = %o", t.c_cflag);
  syslog(LOG_ERR, "oflag = %o", t.c_oflag);
  syslog(LOG_ERR, "lflag = %o", t.c_lflag);
  syslog(LOG_ERR, "iflag = %o", t.c_iflag);
}
#endif

void do_exit(exit_code)
int exit_code;
{
  if (mailfp && mailfp != stderr) {
    int status;
    int ret;

    if (fclose(mailfp) == EOF)
      sys_error("fclose");
    while ((ret = wait(&status)) != mail_pid)
      if (ret < 0)
	sys_error("wait");
    if ((status & 0377) == 0177)
      syslog(LOG_ERR, "%s stopped", sendmail);
    else if ((status & 0377) != 0)
      syslog(LOG_ERR, "%s: %s%s", sendmail, strsignal(status & 0177),
	     (status & 0200) ? " (core dumped)" : "");
    else {
      int exit_code = (status >> 8) & 0377;
      if (exit_code != 0)
	syslog(LOG_ERR, "%s exited with status %d", sendmail, exit_code);
    }
  }
  restore_status_file();
  exit(exit_code);
}

char *strsignal(n)
int n;
{
  extern char *sys_siglist[];
  static char buf[32];
  if (n >= 0 && n < NSIG)
    return sys_siglist[n];
  (void)sprintf(buf, "Signal %d", n);
  return buf;
}

void flush_output()
{
  char ibuf[IBSIZE];
  char *p = out.buf;
  int n = out.ptr - p;
  /* we daren't block on writes */
  set_non_blocking();
  while (n > 0) {
    fd_set rfds, wfds;
    FD_ZERO(&wfds);
    FD_ZERO(&rfds);
    FD_SET(psfd, &wfds);
    FD_SET(psfd, &rfds);
    if (interrupt_flag)
      handle_interrupt();
    while (select(psfd + 1, &rfds, &wfds, (fd_set *)NULL, (struct timeval *)NULL)
	   < 0)
      if (errno == EINTR) {
	if (interrupt_flag)
	  handle_interrupt();
      }
      else
	sys_error("select");
    if (FD_ISSET(psfd, &rfds)) {
      char *q = ibuf;
      int nread = read(psfd, ibuf, IBSIZE);
      if (nread < 0)
	sys_error("read");
      while (--nread >= 0)
	process_input_char(*q++);
    }
    else if (FD_ISSET(psfd, &wfds)) {
      if (want_status) {
	switch (write(psfd, &ctrl_t, 1)) {
	case 0:
	  break;
	case 1:
	  want_status = 0;
	  break;
	case -1:
	  sys_error("write");
	}
      }
      else {
	int nwritten = write(psfd, p, n);
	if (nwritten < 0)
	  sys_error("write");
	n -= nwritten;
	p += nwritten;
      }
    }
  }
  out.ptr = out.buf;
  out.count = OBSIZE;
}

  
void output_char(c)
char c;
{
  if (out.count <= 0)
    flush_output();
  *out.ptr = c;
  out.ptr += 1;
  out.count -= 1;
}

void message_char(c)
char c;
{
  if (c != '\0' && message_buf_i < MBSIZE - 1)
    message_buf[message_buf_i++] = c;
}

void process_message()
{
  char *p = message_buf;
  message_buf[message_buf_i] = 0;
  
  while (*p != '\0') {
    char *key;
    char *nextp;

    for (nextp = p; *nextp != '\0'; nextp++)
      if (*nextp == ';') {
	*nextp++ = '\0';
	break;
      }
    while (isspace(*p))
      p++;
    key = p;
    p = index(p, ':');
    if (p != NULL) {
      char *q;
      *p++ = '\0';
      while (isspace(*p))
	p++;
      q = index(p, '\0');
      while (q > p && isspace(q[-1]))
	--q;
      *q = '\0';
      if (strcmp(key, "Flushing") == 0)
	printer_flushing();
      else if (strcmp(key, "PrinterError") == 0) {
	handle_printer_error(p);
	want_status = 1;
      }
      else if (strcmp(key, "exitserver") == 0) {
      }
      else if (strcmp(key, "Error") == 0) {
	if (in_job) {
	  if (mailfp == NULL)
	    open_mailfp();
	  (void)fputs("Your printer job ", mailfp);
	  if (job_name && *job_name)
	    (void)fprintf(mailfp, "(%s) ", job_name);
	  (void)fprintf(mailfp, "produced the PostScript error `%s'.\n", p);
	}
      }
      else if (strcmp(key, "status") == 0) {
	if (strcmp(p, "idle") == 0)
	  status = IDLE;
	else if (strcmp(p, "busy") == 0)
	  status = BUSY;
	else if (strcmp(p, "waiting") == 0)
	  status = WAITING;
	else if (strcmp(p, "printing") == 0)
	  status = PRINTING;
	else if (strcmp(p, "warming up") == 0) {
	  status = WARMING_UP;
	  handle_printer_error(p);
	}
	else if (strcmp(p, "initializing") == 0)
	  status = INITIALIZING;
	else if (strncmp(p, "PrinterError: ", sizeof("PrinterError: ") - 1)
		 == 0) {
	  status = PRINTER_ERROR;
	  handle_printer_error(p + sizeof("PrinterError: ") - 1);
	}
	else {
	  syslog(LOG_ERR, "unknown printer status `%s'", p);
	  status = UNKNOWN;
	}
	switch (status) {
	case IDLE:
	case BUSY:
	case WAITING:
	case PRINTING:
	  restore_status_file();
	  error_buf[0] = '\0';
	  break;
	default:
	  want_status = 1;
	  break;
	}
      }
      else if (strcmp(key, "OffendingCommand") == 0) {
	if (in_job) {
	  if (mailfp == NULL)
	    open_mailfp();
	  (void)fprintf(mailfp, "The offending command was `%s'.\n", p);
	}
      }
      else if (strcmp(key, "pagecount") == 0) {
	int n;
	if (sscanf(p, "%d", &n) == 1 && n >= 0) {
	  if (start_pagecount < 0)
	    start_pagecount = n;
	  else
	    end_pagecount = n;
	}
      }
    }
    p = nextp;
  }
}

void handle_printer_error(s)
char *s;
{
  if (strlen(s) + 1 > sizeof(error_buf)
      || strcmp(s, error_buf) == 0)
    return;
  strcpy(error_buf, s);
  syslog(LOG_ERR, "%s: %s",
	 printer_name ? printer_name : "printer error",
	 s);
  print_status_file("%s: %s",
		    printer_name ? printer_name : "printer error",
		    s);
}

void process_input_char(c)
char c;
{
  if (c == '\004')
    ++eof_count;
  else if (ignore_input || c == '\r')
    ;
  else {
    switch (parse_state) {
    case NORMAL:
      if (c == '%')
	parse_state = HAD_ONE_PERCENT;
      else
	user_char(c);
      break;
    case HAD_ONE_PERCENT:
      if (c == '%')
	parse_state = HAD_TWO_PERCENT;
      else {
	user_char('%');
	user_char(c);
	parse_state = NORMAL;
      }
      break;
    case HAD_TWO_PERCENT:
      if (c == '[') {
	message_buf_i = 0;
	parse_state = IN_MESSAGE;
      }
      else {
	user_char('%');
	user_char('%');
	user_char(c);
	parse_state = NORMAL;
      }
      break;
    case IN_MESSAGE:
      if (c == ']')
	parse_state = HAD_RIGHT_BRACKET;
      else
	message_char(c);
      break;
    case HAD_RIGHT_BRACKET:
      if (c == '%')
	parse_state = HAD_RIGHT_BRACKET_AND_PERCENT;
      else {
	message_char(']');
	message_char(c);
	parse_state = IN_MESSAGE;
      }
      break;
    case HAD_RIGHT_BRACKET_AND_PERCENT:
      if (c == '%') {
	parse_state = NORMAL;
	process_message();
      }
      else {
	message_char(']');
	message_char('%');
	message_char(c);
	parse_state = IN_MESSAGE;
      }
      break;
    default:
      abort();
    }
  }
}

void process_some_input()
{
  char ibuf[IBSIZE];
  char *p = ibuf;
  int nread;
  fd_set rfds, wfds;
  set_non_blocking();
  FD_ZERO(&wfds);
  FD_ZERO(&rfds);
  FD_SET(psfd, &rfds);
  if (want_status)
    FD_SET(psfd, &wfds);
  if (interrupt_flag)
    handle_interrupt();
  if (timeout_flag) {
    handle_timeout();
    return;
  }
  while (select(psfd + 1, &rfds, &wfds, (fd_set *)NULL, (struct timeval *)NULL) 
	 < 0)
    if (errno == EINTR) {
      if (timeout_flag)
	handle_timeout();
      else if (interrupt_flag)
	handle_interrupt();
      return;
    }
    else
      sys_error("select");
  if (FD_ISSET(psfd, &wfds)) {
    switch (write(psfd, &ctrl_t, 1)) {
    case 0:
      break;
    case 1:
      want_status = 0;
      break;
    case -1:
      sys_error("write");
    }
    return;
  }
  nread = read(psfd, ibuf, IBSIZE);
  if (nread < 0)
    sys_error("read");
  if (nread == 0)
    error("read unexpectedly returned 0");
  while (--nread >= 0)
    process_input_char(*p++);
}


void do_accounting()
{
  FILE *fp;
  if (end_pagecount > start_pagecount
      && host != NULL
      && login != NULL
      && accounting_file != NULL
      && (fp = fopen(accounting_file, "a")) != NULL) {
    (void)fprintf(fp,
		  "%7.2f %s:%s\n", 
		  (double)(end_pagecount - start_pagecount),
		  host,
		  login);
    if (fclose(fp) == EOF)
      sys_error("fclose %s", accounting_file);
  }
}

void set_timeout_flag()
{
  timeout_flag = 1;
}

void get_end_pagecount()
{
  int ec;
  if (signal(SIGALRM, set_timeout_flag) == BADSIG)
    sys_error("signal");
  (void)alarm(30);
  ioflush();
  blocking_write(pagecount_string, sizeof(pagecount_string) - 1);
  end_pagecount = -1;
  ignore_input = 0;
  in_job = 0;
  parse_state = NORMAL;
  while (end_pagecount < 0)
    process_some_input();
  blocking_write(&ctrl_d, 1);
  ec = eof_count;
  while (ec == eof_count)
    process_some_input();
  if (signal(SIGALRM, SIG_IGN) == BADSIG)
    sys_error("signal");
}

void get_start_pagecount()
{
  int ec;
  if (signal(SIGALRM, set_timeout_flag) == BADSIG)
    sys_error("signal");
  (void)alarm(30);
  ioflush();
  blocking_write(pagecount_string, sizeof(pagecount_string) - 1);
  start_pagecount = -1;
  parse_state = NORMAL;
  in_job = 0;
  ignore_input = 0;
  while (start_pagecount < 0)
    process_some_input();
  blocking_write(&ctrl_d, 1);
  ec = eof_count;
  while (ec == eof_count)
    process_some_input();
  if (signal(SIGALRM, SIG_IGN) == BADSIG)
    sys_error("signal");
}

void set_interrupt_flag()
{
  interrupt_flag = 1;
}

void handle_interrupt()
{
  static char interrupt_string[] = "\003\004";
  int ec;
  interrupt_flag = 0;
  if (signal(SIGINT, SIG_IGN) == BADSIG)
    sys_error("signal");
  if (signal(SIGALRM, set_timeout_flag) == BADSIG)
    sys_error("signal");
  (void)alarm(30);
  ioflush();
  blocking_write(interrupt_string, sizeof(interrupt_string)-1);
  ignore_input = 1;
  ec = eof_count;
  while (eof_count == ec)
    process_some_input();
  if (signal(SIGALRM, SIG_IGN) == BADSIG)
    sys_error("signal");
  get_end_pagecount();
  do_accounting();
  do_exit(EXIT_SUCCESS);
}

void printer_flushing()
{
  int ec;
  if (signal(SIGINT, SIG_IGN) == BADSIG)
    sys_error("signal");
  ioflush();
  blocking_write(&ctrl_d, 1);
  ignore_input = 1;
  ec = eof_count;
  while (eof_count == ec)
    process_some_input();
  get_end_pagecount();
  do_accounting();
  do_exit(EXIT_SUCCESS);
}

void send_file()
{
  int ec;
  int c;

  in_job = 1;
  parse_state = NORMAL;
  ignore_input = 0;
  if (signal(SIGINT, set_interrupt_flag) == BADSIG)
    sys_error("signal");
  while ((c = getchar()) != EOF)
    if (c != '\004')
      output_char(c);
  flush_output();
#ifdef IIg
  /* rauletta@sitevax.gmu.edu says this works around the bug in the
     serial driver of the Laserwriter IIf and IIg. */
  (void)tcdrain(psfd);
#endif
  blocking_write(&ctrl_d, 1);
  ec = eof_count;
  while (ec == eof_count)
    process_some_input();
  restore_status_file();
}

void get_status()
{
  int i;

  for (i = 0; i < MAX_TRIES; i++) {
    ioflush();
    if (signal(SIGALRM, set_timeout_flag) == BADSIG)
      sys_error("signal");
    (void)alarm(5);
    blocking_write(&ctrl_t, 1);
    in_job = 0;
    parse_state = NORMAL;
    ignore_input = 0;
    exit_on_timeout = 0;
    while (status == INVALID) {
      process_some_input();
      if (timeout_flag) {
	timeout_flag = 0;
	break;
      }
    }
    if (signal(SIGALRM, SIG_IGN) == BADSIG)
      sys_error("signal");
    switch (status) {
    case INVALID:
      /* we slept in handle_timeout(), so don't sleep here */
      break;
    case IDLE:
      return;
    case WAITING:
      blocking_write(&ctrl_d, 1);
      sleep(5);
      break;
    default:
      sleep(15);
      break;
    }
    status = INVALID;
  }
  syslog(LOG_ERR, "%s%scouldn't make printer ready to receive job",
	 printer_name ? printer_name : "",
	 printer_name ? ": " : "");
  do_exit(EXIT_REPRINT);
}

/* I know we ought to use varargs, but 4.3 doesn't have vsprintf. */

/*VARARGS1*/
void print_status_file(s, a1, a2)
char *s;
char *a1, *a2;
{
  char buf[BUFSIZ];

  (void)sprintf(buf, s, a1, a2);
  (void)strcat(buf, "\n");
  write_status_file(buf);
  status_file_changed = 1;
}

void restore_status_file()
{
  if (status_file_contents && status_file_changed) {
    write_status_file(status_file_contents);
    status_file_changed = 0;
  }
}

void write_status_file(s)
char *s;
{
  int fd;

  (void)umask(0);
  fd = open(status_filename, O_WRONLY|O_CREAT, 0664);
  if (fd < 0)
    sys_error("can't open %s", status_filename);
  if (flock(fd, LOCK_EX) < 0)
    sys_error("flock %s", status_filename);
  if (ftruncate(fd, (off_t)0) < 0)
    sys_error("ftruncate %s", status_filename);
  if (write(fd, s, strlen(s)) < 0)
    sys_error("write %s", status_filename);
  if (close(fd) < 0)
    sys_error("close %s", status_filename);
}

void read_status_file()
{
  struct stat sb;
  int count;
  char *ptr;

  int fd = open(status_filename, O_RDONLY);
  if (fd < 0)
    sys_error("can't open %s", status_filename);
  if (flock(fd, LOCK_SH) < 0)
    sys_error("flock %s", status_filename);
  if (fstat(fd, &sb) < 0)
    sys_error("fstat %s", status_filename);
  if ((sb.st_mode & S_IFMT) != S_IFREG)
    error("%s is not a regular file", status_filename);
  status_file_contents = xmalloc((int)sb.st_size + 1);
  count = sb.st_size;
  ptr = status_file_contents;
  while (count > 0) {
    int nread = read(fd, ptr, count);
    if (nread < 0)
      sys_error("read %s", status_file_contents);
    if (nread == 0)
      error("%s changed size (locking problem?)", status_filename);
    count -= nread;
    ptr += nread;
  }
  
  if (close(fd) < 0)
    sys_error("close %s", status_filename);
  status_file_contents[sb.st_size] = '\0';

  /* This is a grungy hack. */
#define MESSAGE " is ready and printing\n"
#define MESSAGE_LEN (sizeof(MESSAGE) - 1)
  if (!printer_name
      && sb.st_size > sizeof(MESSAGE)
      && strcmp(status_file_contents + sb.st_size - MESSAGE_LEN, MESSAGE) == 0) {
    int plen = sb.st_size - MESSAGE_LEN;
    printer_name = xmalloc(plen + 1);
    bcopy(status_file_contents, printer_name, plen);
    printer_name[plen] = '\0';
  }
}

char *xmalloc(n)
     int n;
{
  char *p = malloc((unsigned)n);
  if (!p)
    error("out of memory");
  return p;
}

#define usage() error("invalid arguments")    

int main(argc, argv)
int argc;
char **argv;
{
  char *prog;
  int i = 1;
  int Uflag = 0;

  if (argc > 0) {
    prog = rindex(argv[0], '/');
    if (prog)
      prog++;
    else
      prog = argv[0];
  }
  else
    prog = "lprps";
  openlog(prog, LOG_PID, LOG_LPR);

  for (i = 1; i < argc && argv[i][0] == '-'; i++)
    switch (argv[i][1]) {
    case 'c':
      break;
    case 'M':
      /* don't mail errors back to user */
      mail_flag = 0;
      break;
    case 'U':
      /* argument is not a real user (eg daemon) */
      Uflag++;
      break;
    case 'x':
    case 'y':
    case 'w':
    case 'l':
    case 'i':
      /* not interested in these */
      break;
    case 'n':
      if (++i == argc)
	usage();
      else
	login = argv[i];
      break;
    case 'h':
      if (++i == argc)
	usage();
      else
	host = argv[i];
      break;
      /* 'j', 'p', and 's' are non-standard */
    case 'j':
      if (++i == argc)
	usage();
      else
	job_name = argv[i];
      break;
    case 'p':
      if (++i == argc)
	usage();
      else
	printer_name = argv[i];
      break;
    case 's':
      if (++i == argc)
	usage();
      else
	status_filename = argv[i];
      break;
    default:
      usage();
    }
  if (argc - i > 1)
    usage();
  if (i < argc)
    accounting_file = argv[i];
  if (Uflag && mail_flag && login) {
    for (i = 1; i < argc && argv[i][0] == '-'; i++)
      switch(argv[i][1]) {
      case 'U':
	if (strcmp(argv[i] + 2, login) == 0)
	  mail_flag = 0;
	break;
      case 'h':
      case 'n':
      case 'j':
      case 'p':
      case 's':
	i++;
	break;
      }
  }
  read_status_file();
  get_status();
  exit_on_timeout = 1;
  get_start_pagecount();
  send_file();
  get_end_pagecount();
  do_accounting();
  do_exit(EXIT_SUCCESS);
  /*NOTREACHED*/
}
