/*
 * Command line editing - common code
 *
 */

#include "config.h"
#if defined(EMACS) || defined(VI)

#ifndef lint
static char *RCSid = "$Id: edit.c,v 1.2 1993/12/21 01:37:31 polk Exp $";
#endif

#include "stdh.h"
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <setjmp.h>
#ifndef NOSTDHDRS
# include <string.h>
#endif
#include "sh.h"
#include "tty.h"
#define EXTERN
#include "edit.h"
#undef EXTERN

#ifdef _CRAY2
extern unsigned	sleep();
#endif


static int	x_noecho = 0;

/*
 * read an edited command line
 */
int
x_read(fd, buf, len)
	int fd;			/* not used */
	char *buf;
	size_t len;
{
	static int setup_done = 0;
	int	i;
	char *ps1 = prompt;			/* current prompt */
	register char *rcp;
	
	if (setup_done != 42)
	{
		setup_done = 42;		/* these get done once only */
		x_do_init = 1;
		x_col = 0;
		ed_erase = -1, ed_kill = -1, ed_werase = -1, ed_intr = -1, ed_quit = -1;
		x_adj_ok = 1;
		x_adj_done = 0;
	}
	if (x_do_init)
		x_init();

	if (x_noecho)
		return(read(ttyfd, buf, len));

	(void)x_mode(TRUE);
	if (rcp = strrchr(ps1, '\n'))
		prompt = ++rcp;			/* fake 1 line prompt */
#ifdef EMACS
	if (flag[FEMACS])
		i = x_emacs(buf, len);
	else
#endif
#ifdef VI
	if (flag[FVI])
		i = x_vi(buf, len);
	else
#endif
		i = -1;				/* internal error */
	prompt = ps1;				/* restore it */
	(void) x_mode(FALSE);
	if (i > 4 && strstr(buf, "stty"))
		x_do_init = 1;
	if (i < 0 && errno == EINTR)
		trapsig(SIGINT);
	return i;
}

/* tty I/O */

int
x_getc()
{
  char c;

  /*
   * This allows the arival of a SIGCHLD to not disturb us until 
   * we are ready. 
   * BSD and other systems that automatically rety a read after 
   * an interrupt don't need this but it doesn't do any harm 
   * either. 
   */
 retry:
	if (read(ttyfd, &c, 1) != 1)
	{
	  if (sigchld_caught)		/* just a SIGCHLD ? */
	  {
	    goto retry;
	  }
	  return -1;
	}
	return c & 0x7F;
}

void
x_flush()
{
	fflush(stdout);
}


/* NAME:
 *      x_adjust - redraw the line adjusting starting point etc.
 *
 * DESCRIPTION:
 *      This function is called when we have exceeded the bounds 
 *      of the edit window.  It increments x_adj_done so that 
 *      functions like x_ins and x_delete know that we have been 
 *      called and can skip the x_bs() stuff which has already 
 *      been done by x_redraw.
 *
 * RETURN VALUE:
 *      None
 */

void
x_adjust()
{
  x_adj_done++;			/* flag the fact that we were called. */
#ifdef EMACS
 if (flag[FEMACS]) {

   /*
    * we had a promblem if the prompt length > x_cols / 2
    */
   if ((xbp = xcp - (x_displen / 2)) < xbuf)
     xbp = xbuf;
   xlp_valid = FALSE;
   x_redraw(x_cols);
 }
#endif
  x_flush();
}

void
x_putc(c)
	int c;
{
  if (c == '\r' || c == '\n')
    x_col = 0;
  if (x_col < x_cols)
  {
    putc(c, stdout);
    switch(c)
    {
    case BEL:
      break;
    case '\r':
    case '\n':
    break;
    case '\b':
      x_col--;
      break;
    default:
      x_col++;
      break;
    }
  }
  if (x_adj_ok && (x_col < 0 || x_col >= (x_cols - 2)))
  {
    x_adjust();
  }
}

#ifdef DEBUG
int
x_debug_info()
{
  x_flush();
  printf("\nksh debug:\n");
  printf("\tx_col == %d,\t\tx_cols == %d,\tx_displen == %d\n",
	 x_col, x_cols, x_displen);
  printf("\txcp == 0x%lx,\txep == 0x%lx\n", (long) xcp, (long) xep);
  printf("\txbp == 0x%lx,\txbuf == 0x%lx\n", (long) xbp, (long) xbuf);
  printf("\txlp == 0x%lx\n", (long) xlp);
  printf("\txlp == 0x%lx\n", (long) x_lastcp());
  printf("\n");
  x_redraw(-1);
  return 0;
}
#endif

void
x_puts(s)
	register char *s;
{
  register int	adj = x_adj_done;

  while (*s && adj == x_adj_done)
    x_putc(*s++);
}

#if defined(_BSD) && !defined(_POSIX_TERM)
static	struct sgttyb cb, cborig;
#ifdef TIOCGATC
static struct ttychars lchars, lcharsorig;
#else
static struct tchars tchars, tcharsorig;
#ifdef TIOCGLTC
static struct ltchars ltchars, ltcharsorig;
#endif
#endif
#else
#ifdef _POSIX_TERM
static	struct termios cb, cborig;
#else
static	struct termio cb, cborig;
#endif
#endif

/* initialize editing mode */
void
x_init()
{
	x_do_init = 0;
#if defined(_BSD) && !defined(_POSIX_TERM)
	(void)ioctl(ttyfd, TIOCGETP, &cborig);
	if ((cborig.sg_flags & ECHO) == 0)
		x_noecho = 1;
	cb = cborig;
	ed_erase = cb.sg_erase;
	ed_kill = cb.sg_kill;
	cb.sg_flags &= ~ECHO;
	cb.sg_flags |= CBREAK;
#ifdef TIOCGATC
	(void)ioctl(ttyfd, TIOCGATC, &lcharsorig);
	lchars = lcharsorig;
	ed_werase = lchars.tc_werasc;
	lchars.tc_suspc = -1;
	lchars.tc_dsuspc = -1;
	lchars.tc_lnextc = -1;
	lchars.tc_statc = -1;
	lchars.tc_intrc = -1;
	lchars.tc_quitc = -1;
	lchars.tc_rprntc = -1;
#else
	(void)ioctl(ttyfd, TIOCGETC, &tcharsorig);
#ifdef TIOCGLTC
	(void)ioctl(ttyfd, TIOCGLTC, &ltcharsorig);
#endif
	tchars = tcharsorig;
#ifdef TIOCGLTC
	ltchars = ltcharsorig;
	ed_werase = ltchars.t_werasc;
	ltchars = ltcharsorig;
	ltchars.t_suspc = -1;
	ltchars.t_dsuspc = -1;
	ltchars.t_lnextc = -1;
#endif
	tchars.t_intrc = -1;
	tchars.t_quitc = -1;
#ifdef TIOCGLTC
	ltchars.t_rprntc = -1;
#endif
#endif
#else /* !_BSD */
#ifdef _POSIX_TERM
	(void) tcgetattr(ttyfd, &cborig);
#else
	(void)ioctl(ttyfd, TCGETA, &cborig);
#endif
	if ((cborig.c_lflag & ECHO) == 0)
		x_noecho = 1;
	cb = cborig;
	ed_erase = cb.c_cc[VERASE]; /* TODO */
	ed_kill = cb.c_cc[VKILL]; /* TODO */
	ed_intr = cb.c_cc[VINTR];
	ed_quit = cb.c_cc[VQUIT];
#ifdef _CRAY2		/* brain-damaged terminal handler */
	cb.c_lflag &= ~(ICANON|ECHO);
	/* rely on print routine to map '\n' to CR,LF */
#else
	cb.c_iflag &= ~(INLCR|ICRNL);
#ifdef _BSD_SYSV	/* need to force CBREAK instead of RAW (need CRMOD on output) */
	cb.c_lflag &= ~(ICANON|ECHO);
#else
#ifdef SWTCH	/* need CBREAK to handle swtch char */
	cb.c_lflag &= ~(ICANON|ECHO);
	cb.c_lflag |= ISIG;
	cb.c_cc[VINTR] = 0377;
	cb.c_cc[VQUIT] = 0377;
#else
	cb.c_lflag &= ~(ISIG|ICANON|ECHO);
#endif
#endif
	cb.c_cc[VTIME] = 0;
	cb.c_cc[VMIN] = 1;
#endif	/* _CRAY2 */
#endif
#ifdef EMACS
	x_emacs_keys(ed_erase, ed_kill, ed_werase, ed_intr, ed_quit);
#endif
}

static	bool_t	x_cur_mode = FALSE;

/* set/clear tty cbreak mode */

#if defined(_BSD) && !defined(_POSIX_TERM)
bool_t
x_mode(onoff)
	bool_t	onoff;
{
	bool_t	prev;

	if (x_cur_mode == onoff) return x_cur_mode;
	prev = x_cur_mode;
	x_cur_mode = onoff;
	if (onoff)  {
		(void)ioctl(ttyfd, TIOCSETN, &cb);
#ifdef TIOCGATC
		(void)ioctl(ttyfd, TIOCSATC, &lchars);
#else
		(void)ioctl(ttyfd, TIOCSETC, &tchars);
#ifdef TIOCGLTC
		(void)ioctl(ttyfd, TIOCSLTC, &ltchars);
#endif
#endif
	}
	else {
		(void)ioctl(ttyfd, TIOCSETN, &cborig);
#ifdef TIOCGATC
		(void)ioctl(ttyfd, TIOCSATC, &lcharsorig);
#else
		(void)ioctl(ttyfd, TIOCSETC, &tcharsorig);
#ifdef TIOCGLTC
		(void)ioctl(ttyfd, TIOCSLTC, &ltcharsorig);
#endif
#endif
	}
	return prev;
}

#else	/* !_BSD */

bool_t
x_mode(onoff)
bool_t	onoff;
{
	bool_t	prev;

	if (x_cur_mode == onoff) return x_cur_mode;
	prev = x_cur_mode;
	x_cur_mode = onoff;

	if (onoff)  {
#ifdef _POSIX_TERM
		(void) tcsetattr(ttyfd, TCSADRAIN, &cb);
#else
#ifndef TCSETAW				/* e.g. Cray-2 */
		/* first wait for output to drain */
#ifdef TCSBRK
		(void)ioctl(ttyfd, TCSBRK, 1);
#else	/* the following kludge is minimally intrusive, but sometimes fails */
		(void)sleep((unsigned)1);	/* fake it */
#endif
#endif
#if defined(_BSD_SYSV) || !defined(TCSETAW)
/* _BSD_SYSV needs to force TIOCSETN instead of TIOCSETP (preserve type-ahead) */
		(void)ioctl(ttyfd, TCSETA, &cb);
#else
		(void)ioctl(ttyfd, TCSETAW, &cb);
#endif
#endif
	}
	else {
#ifdef _POSIX_TERM
		(void) tcsetattr(ttyfd, TCSADRAIN, &cborig);
#else
#ifndef TCSETAW				/* e.g. Cray-2 */
		/* first wait for output to drain */
#ifdef TCSBRK
		(void)ioctl(ttyfd, TCSBRK, 1);
#else
/* doesn't seem to be necessary when leaving xmode */
/*		(void)sleep((unsigned)1);	/* fake it */
#endif
#endif
#if defined(_BSD_SYSV) || !defined(TCSETAW)
/* _BSD_SYSV needs to force TIOCSETN instead of TIOCSETP (preserve type-ahead) */
		(void)ioctl(ttyfd, TCSETA, &cborig);
#else
		(void)ioctl(ttyfd, TCSETAW, &cborig);
#endif
#endif
	}
	return prev;
}
#endif	/* _BSD */


/* NAME:
 *      promptlen - calculate the length of PS1 etc.
 *
 * DESCRIPTION:
 *      This function is based on a fix from guy@demon.co.uk
 *      It fixes a bug in that if PS1 contains '!', the length 
 *      given by strlen() is probably wrong.
 *
 * RETURN VALUE:
 *      length
 */
 
int
promptlen(cp)
  register char  *cp;
{
  register int count = 0;

  while (*cp)
  {
    if (*cp == '\n') 
    {
      count = 0;
      *cp++;
    }
    else if ( *cp++ != '!' )
      count++;
    else if ( *cp == '!' )
      {
	cp++;
	count++;
      }
      else
      {
	register int i = source->line;

	do
	{
	count++;
      } while( ( i /= 10 ) > 0 );
      }
  }
  return count;
}


/*
 * this function check the environment
 * for FCEDIT,EDITOR or VISUAL
 * as a hint to what edit mode is desired.
 */
init_editmode()
{
  static char *ev[] = { "FCEDIT", "EDITOR", "VISUAL", NULL };
  register int i;
  register char *rcp;

  for (i = 0; ev[i]; i++)
  {
#ifdef DEBUG
    (void) fprintf(stderr, "check %s\n", ev[i]);
#endif
    if ((rcp = strval(global(ev[i]))) && *rcp)
      break;
  }
  if (ev[i] && rcp)
  {
    set_editmode(rcp);
  }
  return 0;
}

void
set_editmode(ed)
  char *ed;
{
  register char *rcp;
  
#ifdef DEBUG
  (void) fprintf(stderr, "set_editmode(%s)\n", ed);
#endif
  if (rcp = strrchr(ed, '/'))
    ed = ++rcp;
#ifdef EMACS
  if (strstr(ed, "emacs"))
  {
    flag[FVI] = 0;
    flag[FEMACS] = 1;
  }
#endif
#if defined(EMACS) && defined(VI)
  else
#endif
#ifdef VI
    if (strstr(ed, "vi"))
    {
      flag[FVI] = 1;
      flag[FEMACS] = 0;
    }
#endif
}
#endif
