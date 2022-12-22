/*
	hacked by bdale 861108 to be more intelligent about result codes, etc.
	then by vixie 861201 to be even more intelligent
	then by vixie 861220 to log more errors, and get positively nitpicking
	then by vixie 870309 to be smarter and faster but just as careful
	then by vixie 870317 to turn auto answer on and off, sub-second sleeps
	then by vixie 871221 to work with the Telebit
	then by vixie 890317 to work with Ultrix and to work better on T'bit
	then by vixie 920912 to work with UUNET's new uucp
	then by vixie 930223 to work with BSDI's version of UUNET uucp
*/
#ifndef lint
static char	*RcsId = "$Header: /bsdi/MASTER/BSDI_OS/usr.bin/uucp/libacu/hysv.c,v 1.2 1994/01/31 01:26:09 donn Exp $";
#endif !lint

#include "condevs.h"

#ifdef HAYESV

#include <ctype.h>
#include <signal.h>
#include <sys/time.h>

#ifdef ultrix
# include <sys/termio.h>
# include <sys/file.h>
static int Zero = 0;	/* for TIOCFLUSH */
#endif /* ultrix */

/* Configuration of modem largely does not matter except that it must drop
 * carrier and reset to whatever you want getty to see when DTR is dropped.
 */

/***
 *	hysvpopn(telno, flds, dev) connect to hayes smartmodem (pulse call)
 *	hysvtopn(telno, flds, dev) connect to hayes smartmodem (tone call)
 *	char *flds[], *dev[];
 *
 *	return codes:
 *		>0  -  file number  -  ok
 *		CF_DIAL,CF_DEVICE  -  failed
 *		CF_CHAT		- failed
 */

extern	void	Delay(int, int);

static int wake_up_modem(int, char *);
static void purge_typeahead(), writestr(int, u_char *);
static char *todispstr(char *);
static int readuntil(int, u_char *, u_char *, int);

CallType	hysvcls(int);
static CallType	hysvopn(char *, char **, Device *, int);

CallType
hysvpopn(telno, flds, dev)
	char *telno, *flds[];
	Device *dev;
{
	return hysvopn(telno, flds, dev, 0);
}

CallType
hysvtopn(telno, flds, dev)
	char *telno, *flds[];
	Device *dev;
{
	return hysvopn(telno, flds, dev, 1);
}

/* ARGSUSED */
static CallType
hysvopn(telno, flds, dev, toneflag)
	char *telno;
	char *flds[];
	Device *dev;
	int toneflag;
{
	int	dh = -1;
	extern errno;
	char dcname[20];
	char response[50];

	sprintf(dcname, "/dev/%s", dev->D_line);
	Debug((4, "dc - %s", dcname));
	if (setjmp(AlarmJmp)) {
		Log("open(%s) TIMEOUT", dcname);
		if (dh >= 0) {
			drop_dtr(dh);
			close_dev(dh);
			dh = -1;
		}
		return CF_DIAL;
	}
	signal(SIGALRM, Timeout);
	getnextfd();
	alarm(10);
	dh = open_dev(dcname);
	alarm(0);
	if (dh < 0) {
		Log("open(%s) CAN'T OPEN", dcname);
		return CF_DIAL;
	}

#ifdef ultrix
	if (0 > ioctl(dh, TIOCSINUSE, NULL)) {
		Log("%s IN USE", dcname);
		return CF_DIAL;
	}
	Debug((6, "TIOCSINUSE ok"));
#endif

	/* modem is open */
	NextFd = -1;
	if ( !SetupTty(dh, dev->D_speed) )
		return CF_DIAL;

#ifdef ultrix
    {
	int x, y;

	x = ioctl(dh, TIOCMODEM, &Zero);	/* turn on modem control */
	y = ioctl(dh, TIOCNCAR, &Zero);		/* DCD=dontcare */
	Debug((6, "TIOCMODEM %d, TIOCNCAR %d", x, y));
    }
#endif /* ultrix */

	if (wake_up_modem(dh, dcname) == -1) {
		Log("%s CAN'T SYNC HYSV", dcname);
		drop_dtr(dh);
		close_dev(dh);   dh = -1;
		return CF_DIAL;
	}

	if (dochat(dev, flds, dh)) {
		Log("%s INITIAL CHAT FAILED", dcname);
		drop_dtr(dh);
		close_dev(dh);   dh = -1;
		return CF_DIAL;
	}

	/* floating block for local variable */
	{
		char	dial[50], msg[100], *todispstr(), *ch;

		sprintf(dial, "ATd%c%s\r", (toneflag ? 't' : 'p'), telno);

		for (ch = &dial[4]; *ch; ch++)
			if (*ch == '-')
				*ch = ',';
			else if (*ch == '=')
				*ch = 'W';

		sprintf(msg, "sending dial command: '%s'",
			todispstr(dial));
		Debug((8, msg));
		writestr(dh, (u_char*)dial);
	}

#define STR_EQ(l, r) (strncmp(l, r, strlen(l)))	/* very tricky, be careful */

	for (;;) {
		int retval = CF_DIAL;
		int rrings = 0;

		if (readuntil(dh, (u_char*)"\n",
			      (u_char*)response, sizeof response) >= 0) {
			char *p, *q, obuf[500];

			if (STR_EQ("\r\n", response) == 0) {
				continue;
			}
			if (STR_EQ("RRING", response) == 0) {
				rrings++;
				continue;
			}
			if (STR_EQ("CONNECT", response) == 0) {
				break;
			}
			if ((p = strchr(response, '\r'))
			    && (q = strchr(response, '\n'))) {
				if (p > q) p = q;
				*p = '\0';
			}
			/* a bad thing has been said - log it and exit */
			strcpy(obuf, todispstr(response));
			if (rrings > 0) {
				sprintf(&obuf[strlen(obuf)],
					" after %d rings", rrings);
			}
			Log("%s %s", obuf, FAILED);
			if (STR_EQ("BUSY", response) == 0) {
				retval = CF_CHAT;
			}
			if (STR_EQ("NO CARRIER", response) == 0) {
				retval = CF_CHAT;
			}
		} else {
			char obuf[100];

			sprintf(obuf, "hysv: no dialer response (rrings=%d)", rrings);
			Log("%s %s", obuf, FAILED);
		}
		strcpy(devSel, dev->D_line);
		hysvcls(dh);
		return retval;
	}

#ifdef ultrix
	(void) ioctl(dh, TIOCCAR, &Zero);	/* DCD=care */
#endif /* ultrix */

	Debug((4, "vhayes ok"));
	return dh;
}

/* wake_up_modem(fd, fn) - clear out anything already in progress, and get
 * the modem into a state where it can accept commands (and will not
 * accept incoming calls)
 *
 * returns: 0 = OK
 *         -1 = try another modem, this one is screwed up
 */
static int
wake_up_modem(fd, fn)
	register int fd;
	char *fn;
{
	register int i, j, ok;

	Debug((8, "waking up modem"));
	for (ok = 0, i = 1;  !ok && (i <= 3);  i++) {
		drop_dtr(fd);
		writestr(fd, (u_char*)"\rA\rA\rA\rA\rA\r");
		purge_typeahead(fd);
		for (ok = 0, j = 1;  !ok && (j <= 3);  j++) {
			writestr(fd, (u_char*)"ATe0q0v1x1s0=0\r");
			ok = (expect("OK~2", fd) == 0);
		}
	}
	if (!ok) {
		Log("hysv/wake(%s): modem is stubborn or dead", fn);
		return -1;
	}
	if (--i > 1 || --j > 1) {
		char obuf[100];
		sprintf(obuf, "hysv/wake: OK after (%d,%d) tries", i, j);
		Log("%s %s", fn, obuf);
	}
	purge_typeahead(fd);
	Debug((8, "modem is awake, checking"));
	writestr(fd, (u_char*)"ATs0?\r");
	/* \015\012000\015\012\015\012OK\015\012
	 *            ^^^^^^^^ optional, ick
	 */
	if (expect("000\r\n~5", fd) ||
	    expect("OK\r\n~5", fd)) {
		Log("hysv/wake(%s): modem didn't act on S0=0", fn);
		return -1;
	}
	Debug((8, "modem seems OK"));
}

/* warning: this is really, really, really slow and inefficient. (vixie)
 */
static jmp_buf Rjbuf;
static void ru_timeout() { longjmp(Rjbuf, 1); }
static int
readuntil(fd, until, buf, bufsiz)
	int fd, bufsiz;
	u_char *until, *buf;
{
	extern int errno;
	u_char ch, *p = buf;

	if (setjmp(Rjbuf)) {
		*p = '\0';
		Log("readuntil: timeout (%s)", todispstr((char*)buf));
		return -1;
	}
	signal(SIGALRM, ru_timeout);
	alarm(120);
	while (bufsiz > 1 && until != NULL) {	/* save one for the null */
		int x = read(fd, &ch, 1);
		if (x <= 0) {
			char pbuf[1000];

			*p = '\0';
			sprintf(pbuf, "buf='%s', read=%d, errno=%d",
				todispstr((char*)buf), x, errno);
			Log("readuntil(%s): lost line", pbuf);
			alarm(0); signal(SIGALRM, SIG_DFL);
			return -1;
		}
		Debug((9, "readuntil: got 0x%02x", ch));
		*p++ = ch;
		bufsiz--;
		if (strchr((char*)until, ch) != NULL)
			until = NULL;
	}
	*p = '\0';
	Debug((6, "readuntil (%s)", todispstr((char*)buf)));
	alarm(0); signal(SIGALRM, SIG_DFL);
	return 0;
}

static char *
todispstr(src)
	register char	*src;
{
	static char	dst_string[100];
	register char	*dst = dst_string;

	for (;  *src;  src++)
		if (isprint(*src))
			*dst++ = *src;
		else {
			sprintf(dst, "\\%03o", *src);
			dst += 4;
		}
	*dst = '\0';
	return dst_string;
}

CallType
hysvcls(fd)
int fd;
{
	char dcname[PATHNAMESIZE];

	if (fd < 0) {
		rmlock(devSel);
		return;
	}

	sprintf(dcname, "/dev/%s", devSel);
	Debug((4, "Hanging up fd = %d", fd));
	drop_dtr(fd);
	close_dev(fd);
	Delay(1, 2);
	rmlock(devSel);
}

static void
purge_typeahead(fd)
{
	int	zero = 0;
	int	slept;
	long	chars;

	Debug((9, "purging typeahead buffer"));
	ioctl(fd, TIOCFLUSH, &zero);
	Delay(1, 2);
	ioctl(fd, TIOCFLUSH, &zero);
}

/* writestr() -- write a string to a given fd at a low speed (10 CPS)
 */
static jmp_buf WSbuf;
static void ws_timeout() { longjmp(WSbuf, 1); }
static void
writestr(fd, str)
	int fd;
	u_char *str;
{
	Debug((5, "writestr(%s)", todispstr((char*)str)));
	purge_typeahead(fd);
	if (setjmp(WSbuf)) {
		Log("hysv/writestr TIMEOUT");
		Debug((6, "writestr: timeout (fd=%d)\n", fd));
		return;
	}
	signal(SIGALRM, ws_timeout);
	alarm(10);
	for (;  *str;  str++) {
		write(fd, str, 1);
		Delay(1, 10);
	}
	alarm(0);
	signal(SIGALRM, SIG_DFL);
}

#endif VHAYES
