/*	BSDI $Id: msconfig.c,v 1.1.1.1 1993/03/23 05:31:45 polk Exp $	*/

/*
 * msconfig - configuration program for Maxpeed driver.
 *
 * Copyright (c) 1993, Douglas L. Urner (dlu@tfm.com), All rights reserved.
 */

static char *RCSid = " Id: msconfig.c,v 1.3.0.3 1993/03/12 19:43:56 dlu Rel  ";

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <ctype.h>
#include <syslog.h>
#include <string.h>

#include <i386/isa/msioctl.h>

extern char * strerror(int);

extern char * optarg;

void chat(int, const char *, ...);
void display_config(int);
void eioctl(int, int, int, void *, char *, int);
void error(int, const char *, ...);
void usage();

char * prog;
int    errors = 0;

#define BOARD(d)	((d) >> 3 & 0x1f)
#define ERR         0
#define FATAL       1
#define TTY(d)	((d) & 0x07)

int
main(int argc, char **argv)
{
  int          data;
  char         dbuf[MAXPATHLEN + 1];
  int          debug = -1;
  int          diag = -1;
  char *       donor = 0;
  struct stat  dsb;
  int          fd;
  int          flags = -1;
  int          freq = -1;
  int          log = 0;
  int          lowat = -1;
  int          opt;
  char         pbuf[MAXPATHLEN + 1];
  char *       port = 0;
  struct stat  psb;
  int          reset_defaults = 0;
  int          sig = 0;
  int          txqsz;
  int          verbose = 0;
  int          work = 0;

  prog = argv[0];

  while ((opt = getopt(argc, argv, "Dd:F:f:h:Ll:svX:x:")) != EOF) {
	switch(opt) {
	case 'D':					/* Reset default state of driver. */
	  ++reset_defaults;
	  ++work;
	  break;
	case 'd':					/* Donor for MSIOSTEAL. */
	  donor = optarg;
	  ++work;
	  break;
	case 'F':					/* Set config flags. */
	  flags = strtol(optarg, (char **) NULL, 0);
	  if (flags == -1 || !isdigit(*optarg)) {
		fprintf(stderr, "%s: -F requires an argument and \"%s\" won't do\n",
				prog, optarg);
		++errors;
	  }
	  ++work;
	  break;
	case 'f':					/* Specify target port (or board). */
	  port = optarg;
	  break;
	case 'h':					/* Hz for MSIOFREQ. */
	  freq = atoi(optarg);
	  ++work;
	  break;
	case 'L':					/* Log actions via syslog. */
	  openlog("msconfig", LOG_PERROR, LOG_USER);
	  ++log;
	  ++verbose;
	  break;
	case 'l':					/* Low water mark on output. */
	  lowat = atoi(optarg);
	  if (lowat < 0) {
		fprintf(stderr, "%s: output low water mark must be postive\n", prog);
		++errors;
	  }
	  ++work;
	  break;
	case 's':					/* Print ROM signature. */
	  ++sig;
	  ++work;
	  break;
	case 'v':					/* Be verbose. */
	  ++verbose;
	  break;
	case 'x':					/* Enable/disable driver diagnotics. */
	  diag = atoi(optarg);
	  ++work;
	  break;
	case 'X':					/* Enable/disable driver debug code. */
	  debug = atoi(optarg);
	  ++work;
	  break;
	case '?':
	default:
	  usage();
	  break;
	}
  }

  if (port == 0) {
	error(log, "a device must be specified (-f dev)");
	if (!log)
	  usage();
  } else {
	if (*port != '/') {
	  sprintf(pbuf, "/dev/%s", port);
	  port = pbuf;
	}
  	if ((fd = open(port, O_RDONLY|O_NONBLOCK)) == -1)
	  error(log, "%s: %s", port, strerror(errno));
	else
	  if (stat(port, &psb) != 0)
		error(log, "%s: %s", port, strerror(errno));
  }
  if (donor) {
	if (*donor != '/') {
	  sprintf(dbuf, "/dev/%s", donor);
	  donor = dbuf;
	}
	if (stat(donor, &dsb) != 0)
	  error(log, "%s: %s", donor, strerror(errno));
  }

  if (!work) {
	if (!errors) {
	  display_config(fd);
	  exit(0);
	} else
	  exit(1);
  }

  if (sig) {
	char sig[MSMAXSIG];

	eioctl(log, fd, MSIOSIG, sig, "MSIOSIG", FATAL);
	chat(log, "%s", sig);
  }

  if (errors)
	exit(1);

  if (reset_defaults) {
	/*
	 * Do reset first so that other configuration can happen
	 * at the same time.
	 */
	eioctl(log, fd, MSIODEFAULT, (int *) 0, "MSIODEFAULT", FATAL);
	if (verbose)
	  chat(log, "reset driver to defaults and ended signal sharing on ms%d",
			 BOARD(psb.st_rdev));
  }

  if (donor) {
	data = TTY(dsb.st_rdev);
	eioctl(log, fd, MSIOSTEAL, &data, "MISOSTEAL", ERR);
	if (verbose && !errors)
	  if (log)
		syslog(LOG_NOTICE, "%s is borrowing signals from %s", port, donor);
	  else
		chat(log, "%s is borrowing signals from %s", port, donor);
  }

  if (flags != -1) {
	if (flags & ~0x1b) {
	  error(log, "unrecognized flag(s) in 0x%x", flags);
	}
	eioctl(log, fd, MSIOSFLAGS, &flags, "MSIOSFLAGS", ERR);
	if (!errors && verbose) {
	  eioctl(log, fd, MSIOGFLAGS, &flags, "MSIOGFLAGS", ERR);
	  chat(log, "ms%d: set flags to 0x%.2x", BOARD(psb.st_rdev), flags);
	}
  }	  
  
  if (freq != -1) {
	if (freq < 1 || freq > HZ) {
	  error(log, "polling frequency must be between 1 and %d hz", HZ);
	}	  
	if (!errors) {
	  data = HZ/freq;
	  eioctl(log, fd, MSIOSFREQ, &data, "MSIOSFREQ", ERR);
	  if (!errors && verbose) {
		eioctl(log, fd, MSIOGFREQ, &data, "MSIOGFREQ", ERR);
		if (!errors)
		  chat(log, "set polling frequency to %d hz", HZ/data);
	  }
	}
  }
  
  if (lowat != -1) {
	eioctl(log, fd, MSIOTXQSZ, &txqsz, "MSIOTXQZ", ERR);
	if (!errors && lowat >= txqsz) {
	  error(log, "output low water mark must be less than queue size (%d)",
			  txqsz);
	}
	if (!errors) {
	  eioctl(log, fd, MSIOSLOWAT, &lowat, "MSIOSLOWAT", ERR);
	  if (!errors && verbose)
		chat(log, "set output low water mark to %d", lowat);
	}
  }

  if (debug != -1) {
	if (!errors) {
	  eioctl(log, fd, MSIOSDEBUG, &debug, "MSIOSDEBUG", ERR);
	  if (!errors && verbose)
		chat(log, "set debug level to %d", debug);
	}
  }

  if (diag != -1) {
	if (!errors) {
	  eioctl(log, fd, MSIOSDIAG, &diag, "MSIOSDIAG", ERR);
	  if (!errors && verbose)
		chat(log, "set diagnostic level to %d", diag);
	}
  }
  return (errors);
}

void
usage()
{
  fprintf(stderr, "usage: %s -f dev [-d dev] [-h hz] [-l lowat] [-v]\n", prog);
  fprintf(stderr, "                [-D] [-F flags] [-s] [-x lvl] [-X lvl] [-L]\n");
  exit(1);
}

void
display_config(int fd)
{
  int donor;
  int flags;
  int freq;
  int log = 0;			/* Do you want syslog() to see errors from here? */
  int lowat;
  int nports;
  int rxqsz;
  char sig[MSMAXSIG];
  int txqsz;
  int i;

  eioctl(log, fd, MSIOSIG, sig, "MSIOSIG", FATAL);
  printf("ROM signature = \"%s\"\n", sig);

  eioctl(log, fd, MSIOGFLAGS, &flags, "MSIOGFLAGS", FATAL);
  eioctl(log, fd, MSIOGFREQ, &freq, "MSIOGFREQ", FATAL);
  eioctl(log, fd, MSIOGLOWAT, &lowat, "MSIOGLOWAT", FATAL);
  eioctl(log, fd, MSIONPORTS, &nports, "MSIONPORTS", FATAL);
  
printf("%d ports, polling frequency = %d, low water mark = %d, flags = 0x%x\n",
		 nports, HZ/freq, lowat, flags);

  eioctl(log, fd, MSIORXQSZ, &rxqsz, "MSIORXQSZ", FATAL);
  eioctl(log, fd, MSIOTXQSZ, &txqsz, "MSIOTXQSZ", FATAL);
  printf("receive queues: %d bytes   transmit queues: %d bytes\n",
		 rxqsz, txqsz);
  
  for (i = 0; i < nports; i++) {
	donor = i;
	eioctl(log, fd, MSIODONOR, &donor, "MSIODONOR", FATAL);
	if (donor != i)
	  printf("port %d is borrowing DTR and DCD from port %d\n", i, donor);
  }
}

void
eioctl(int log, int fd, int cmd, void * data, char * msg, int action)
{
  if (ioctl(fd, cmd, data) != 0) {
	switch (errno) {
	case ENOTTY:
	  if (log)
		syslog(LOG_NOTICE,  "%s support not included in driver", msg);
	  else
		fprintf(stderr, "%s: %s support not included in driver\n", prog, msg);
	  break;
	default:
	  if (log)
		syslog(LOG_WARNING, "%s: %s", msg, strerror(errno));
	  else
		fprintf(stderr, "%s: %s: %s\n", prog, msg, strerror(errno));
	  break;
	}
	switch (action) {
	case ERR:
	  ++errors;
	  break;
	case FATAL:
	  exit(1);
	}
  }
}

void
error(int log, const char *fmt, ...)
{
  char    errbuf[BUFSIZ];
  va_list ap;

  if (!log)
	sprintf(errbuf, "%s: ", prog);

  va_start(ap, fmt);
  vsprintf(errbuf + strlen(errbuf), fmt, ap);
  if (!log)
	strcat(errbuf, "\n");
  va_end(ap);

  if (log)
	syslog(LOG_ERR, errbuf);
  else
	fputs(errbuf, stderr);

  ++errors;
}

void
chat(int log, const char *fmt, ...)
{
  char    buf[BUFSIZ];
  va_list ap;

  if (log)
	sprintf(buf, "%s: ", prog);

  va_start(ap, fmt);
  vsprintf(buf + strlen(buf), fmt, ap);
  if (!log)
	strcat(buf, "\n");
  va_end(ap);

  if (log)
	syslog(LOG_NOTICE, buf);
  else
	fputs(buf, stderr);
}
