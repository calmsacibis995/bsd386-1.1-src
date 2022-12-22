/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	RCSID $Id: f_proto.c,v 1.2 1994/01/31 01:26:49 donn Exp $
**
**	$Log: f_proto.c,v $
 * Revision 1.2  1994/01/31  01:26:49  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:58:30  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:42:44  vixie
 * Initial revision
 *
 * Revision 1.1.1.1  1992/09/28  20:08:56  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	ERRNO
#define	FILES
#define	SETJMP
#define	SIGNALS
#define	STDIO
#define	SYSEXITS
#define	SYS_TIME
#define	TERMIOCTL

#include	"global.h"
#include	"cico.h"
#include	"Protocol.h"

#ifdef	X25_PAD
/*
**	flow control protocol.
**
**	This protocol relies on flow control of the data stream.
**	It is meant for working over links that can ( almost ) be
**	guaranteed to be errorfree, specifically X.25/PAD links.
**	A sumcheck is carried out over a whole file only. If a
**	transport fails the receiver can request retransmission(s).
**	This protocol uses a 7-bit datapath only, so it can be
**	used on links that are not 8-bit transparent.
**
**	When using this protocol with an X.25 PAD:
**	Although this protocol uses no control chars except CR,
**	control chars NUL and ^P are used before this protocol
**	is started; since ^P is the default char for accessing
**	PAD X.28 command mode, be sure to disable that access
**	(PAD par 1). Also make sure both flow control pars
**	(5 and 12) are set. The CR used in this proto is meant
**	to trigger packet transmission, hence par 3 should be
**	set to 2; a good value for the Idle Timer ( par 4 ) is 10.
**	All other pars should be set to 0.
**
**	Normally a calling site will take care of setting the
**	local PAD pars via an X.28 command and those of the remote
**	PAD via an X.29 command, unless the remote site has a
**	special channel assigned for this protocol with the proper
**	par settings.
**
**	Additional comments for hosts with direct X.25 access:
**	- the global variable Is_a_tty, when set, allows the ioctl's,
**	  so the same binary can run on X.25 and non-X.25 hosts;
**	- reads are done in small chunks, which can be smaller than
**	  the packet size; your X.25 driver must support that.
**
**
**	Author:
**	Piet Beertema, CWI, Amsterdam, Sep 1984
**	Modified for X.25 hosts:
**	Robert Elz, Melbourne Univ, Mar 1985
*/


#define FIBUFSIZ	4096	/* for X.25 interfaces: set equal to packet size,
				 * but see comment above
				 */

#define FOBUFSIZ	4096	/* for X.25 interfaces: set equal to packet size ;
				 * otherwise make as large as feasible to reduce
				 * number of write system calls
				 */

#ifndef MAXMSGLEN
#define MAXMSGLEN	BUFSIZ
#endif	/* MAXMSGLEN */

/*
**	Max. attempts to retransmit a file.
*/

#define MAXRETRIES(A)	(A < 10000L ? 2 : 1)

#ifdef USE_TERMIOS
static struct termios	ttbuf;
#else
static struct sgttyb	ttbuf;
#endif

static int	fchksum;
static jmp_buf	Ffailbuf;
static void	(*fsig)();
static int	Retries;

static int	fwrblk(int, FILE *, int *);
static int	frdblk(char *, int, long *);



static void
falarm(int sig)
{
	(void)signal(sig, falarm);
	longjmp(Ffailbuf, 1);
}

CallType
fturnon()
{
	int ttbuf_flags;

	if (Is_a_tty) {
#ifdef USE_TERMIOS
		(void)tcgetattr(Ifd, &ttbuf);	/* XXX - check return */
		ttbuf_flags = ttbuf.c_iflag;
		ttbuf.c_iflag = IXOFF|IXON|ISTRIP;
		ttbuf.c_cc[VMIN] = FIBUFSIZ > 64 ? 64 : FIBUFSIZ;
		ttbuf.c_cc[VTIME] = 5;
		if (tcsetattr(Ifd, TCSAFLUSH, &ttbuf) == SYSERROR) {
			SysError("tcsetattr failed: %m");
			finish(EX_IOERR);
		}
		ttbuf.c_iflag = ttbuf_flags;
#else	/* USE_TERMIOS */
		ioctl(Ifd, TIOCGETP, &ttbuf);
		ttbuf_flags = ttbuf.sg_flags;
		ttbuf.sg_flags = ANYP|CBREAK;
		if (ioctl(Ifd, TIOCSETP, &ttbuf) < 0) {
			SysError("ioctl(TIOCSETP) failed: %m");
			finish(EX_IOERR);
		}
		/* this is two seperate ioctls to set the x.29 params */
		ttbuf.sg_flags |= TANDEM;
		if (ioctl(Ifd, TIOCSETP, &ttbuf) < 0) {
			SysError("ioctl(TIOCSETP) failed: %m");
			finish(EX_IOERR);
		}
		ttbuf.sg_flags = ttbuf_flags;
#endif	/* USE_TERMIOS */
	}
	fsig = (void(*)())signal(SIGALRM, falarm);
	/* give the other side time to perform its ioctl;
	 * otherwise it may flush out the first data this
	 * side is about to send.
	 */
	sleep(2);
	return (SUCCESS);
}

CallType
fturnoff()
{
	if (Is_a_tty) {
#ifdef USE_TERMIOS
		tcsetattr(Ifd, TCSAFLUSH, &ttbuf);
#else
		ioctl(Ifd, TIOCSETP, &ttbuf);
#endif
	}
	(void)signal(SIGALRM, fsig);
	sleep(2);
	return (SUCCESS);
}

CallType
fwrmsg(
	char		type,
	register char *	str,
	int		fn
)
{
	register char *	s;
	char		bufr[MAXMSGLEN];

	s = bufr;
	*s++ = type;
	while ( *str )
		*s++ = *str++;
	if ( *(s-1) == '\n' )
		s--;
	*s++ = '\r';
	*s = 0;
	(void)write(fn, bufr, s - bufr);
	return SUCCESS;
}

CallType
frdmsg(
	register char *	str,
	register int	fn
)
{
	register char *	smax;

	if ( setjmp(Ffailbuf) )
		return FAIL;
	smax = str + MAXMSGLEN - 1;
	(void)alarm(2*MAXMSGTIME);
	for ( ;; )
	{
		if ( read(fn, str, 1) <= 0 )
			goto msgerr;
		*str &= 0177;
		if ( *str == '\r' )
			break;
		if ( *str < ' ' )
		{
			continue;
		}
		if ( str++ >= smax )
			goto msgerr;
	}
	*str = '\0';
	(void)alarm(0);
	return SUCCESS;
msgerr:
	(void)alarm(0);
	return FAIL;
}

CallType
fwrdata(
	FILE *		fp1,
	int		fn
)
{
	register int	ret;
	register int	alen;
	int		flen;
	long		abytes;
	long		fbytes;
	TimeBuf		start;
	char		ack;
	char		ibuf[MAXMSGLEN];

	ret = FAIL;
	Retries = 0;
retry:
	fchksum = 0xffff;
	abytes = fbytes = 0L;
	ack = '\0';
	(void)SetTimes();
	start = TimeNow;

	do
	{
		alen = fwrblk(fn, fp1, &flen);
		fbytes += flen;
		if ( alen <= 0 )
		{
			abytes -= alen;
			goto acct;
		}
		abytes += alen;
	}
	while
		( !feof(fp1) && !ferror(fp1) );

	Debug((8, "checksum: %04x", fchksum));
	if ( frdmsg(ibuf, fn) != FAIL )
	{
		if ( (ack = ibuf[0]) == 'G' )
			ret = SUCCESS;
		Debug((4, "ack - '%c'", ack));
	}
acct:
	ReportRate(SENT, &start, fbytes, abytes, Retries);
	if ( ack == 'R' )
	{
		Debug((4, "RETRY:"));
		(void)fseek(fp1, 0L, 0);
		Retries++;
		goto retry;
	}
#ifdef	SYS_ACCT
	if ( ret == FAIL )
		sysaccf(NULLSTR);		/* force accounting */
#endif	/* SYS_ACCT */
	return ret;
}

CallType
frddata(
	register int	fn,
	register FILE *	fp2
)
{
	register int	flen;
	long		alen;
	char		eof;
	int		ret;
	long		abytes;
	long		fbytes;
	TimeBuf		start;
	char		ibuf[FIBUFSIZ];

	Retries = 0;
	ret = FAIL;
retry:
	fchksum = 0xffff;
	abytes = fbytes = 0L;
	(void)SetTimes();
	start = TimeNow;

	do
	{
		flen = frdblk(ibuf, fn, &alen);
		abytes += alen;
		if ( flen < 0 )
			goto acct;
		if ( eof = flen > FIBUFSIZ )
			flen -= FIBUFSIZ + 1;
		fbytes += flen;
		if ( fwrite(ibuf, sizeof (char), flen, fp2) != flen )
			goto acct;
	}
	while
		( !eof );

	ret = SUCCESS;
acct:
	ReportRate(RECEIVED, &start, fbytes, abytes, Retries);
	if ( ret == FAIL )
	{
		if ( Retries++ < MAXRETRIES(fbytes) )
		{
			Debug((8, "send ack: 'R'"));
			fwrmsg('R', EmptyStr, fn);
			fseek(fp2, 0L, 0);
			Debug((4, "RETRY:"));
			goto retry;
		}
		Debug((8, "send ack: 'Q'"));
		fwrmsg('Q', EmptyStr, fn);
#ifdef	SYS_ACCT
		sysaccf(NULLSTR);		/* force accounting */
#endif	/* SYS_ACCT */
	}
	else {
		Debug((8, "send ack: 'G'"));
		fwrmsg('G', EmptyStr, fn);
	}
	return ret;
}

static int
frdbuf(
	char *		blk,
	int		len,
	int		fn
)
{
	static int	ret = FIBUFSIZ / 2;

	if ( setjmp(Ffailbuf) )
		return FAIL;
	(void)alarm(MAXMSGTIME);
	ret = read(fn, blk, len);
	alarm(0);
	return ret <= 0 ? FAIL : ret;
}

/* Byte conversion:
**
**	  from	 pre	   to
**	000-037	 172	 100-137
**	040-171		 040-171
**	172-177	 173	 072-077
**	200-237	 174	 100-137
**	240-371	 175	 040-171
**	372-377	 176	 072-077
*/

static int
fwrblk(
	int		fn,
	register FILE *	fp,
	int *		lenp
)
{
	register char *	op;
	register int	c, sum, nl, len;
	char		obuf[FOBUFSIZ + 8];
	int		ret;

	op = obuf;
	nl = 0;
	len = 0;
	sum = fchksum;
	while ( (c = getc(fp)) != EOF )
	{
		len++;
		if ( sum & 0x8000 )
		{
			sum <<= 1;
			sum++;
		} else
			sum <<= 1;
		sum += c;
		sum &= 0xffff;
		if ( c & 0200 )
		{
			c &= 0177;
			if ( c < 040 )
			{
				*op++ = '\174';
				*op++ = c + 0100;
			} else
			if ( c <= 0171 )
			{
				*op++ = '\175';
				*op++ = c;
			}
			else {
				*op++ = '\176';
				*op++ = c - 0100;
			}
			nl += 2;
		} else {
			if ( c < 040 )
			{
				*op++ = '\172';
				*op++ = c + 0100;
				nl += 2;
			} else
			if ( c <= 0171 )
			{
				*op++ = c;
				nl++;
			} else {
				*op++ = '\173';
				*op++ = c - 0100;
				nl += 2;
			}
		}
		if ( nl >= FOBUFSIZ - 1 )
		{
			/*
			 * peek at next char, see if it will fit
			 */
			c = getc(fp);
			if ( c == EOF )
				break;
			(void)ungetc(c, fp);
			if ( nl >= FOBUFSIZ || c < 040 || c > 0171 )
				goto writeit;
		}
	}
	/*
	 * At EOF - append checksum, there is space for it...
	 */
	sprintf(op, "\176\176%04x\r", sum);
	nl += strlen(op);
writeit:
	*lenp = len;
	fchksum = sum;
	Debug((8, "%d/%d", len, nl));
	ret = write(fn, obuf, nl);
	return ret == nl ? nl : ret < 0 ? 0 : -ret;
}

static int
frdblk(
	register char *	ip,
	int		fn,
	long *		rlen
)
{
	register char *op, c;
	register int sum, len, nl;
	char buf[5], *erbp = ip;
	int i;
	static char special = 0;

	if ( (len = frdbuf(ip, FIBUFSIZ, fn)) == FAIL )
	{
		*rlen = 0;
		goto dcorr;
	}
	*rlen = len;
	Debug((8, "%d/\\c", len));
	op = ip;
	nl = 0;
	sum = fchksum;
	do {
		if ( (*ip &= 0177) >= '\172' )
		{
			if ( special )
			{
				Debug((8, "%d\\c", nl));
				special = 0;
				op = buf;
				if ( *ip++ != '\176' || (i = --len) > 5 )
					goto dcorr;
				while ( i-- )
					*op++ = *ip++ & 0177;
				while ( len < 5 )
				{
					i = frdbuf(&buf[len], 5 - len, fn);
					if ( i == FAIL )
					{
						len = FAIL;
						goto dcorr;
					}
					Debug((8, ",%d\\c", i));
					len += i;
					*rlen += i;
					while ( i-- )
						*op++ &= 0177;
				}
				if ( buf[4] != '\r' )
					goto dcorr;
				sscanf(buf, "%4x", &fchksum);
				Debug((8, "\nchecksum: %04x", sum));
				if ( fchksum == sum )
					return FIBUFSIZ + 1 + nl;
				else {
					Debug((4, "Bad checksum"));
					return FAIL;
				}
			}
			special = *ip++;
		} else {
			if ( *ip < '\040' )
			{
				/* error: shouldn't get control chars */
				goto dcorr;
			}
			switch ( special )
			{
			case 0:
				c = *ip++;
				break;
			case '\172':
				c = *ip++ - 0100;
				break;
			case '\173':
				c = *ip++ + 0100;
				break;
			case '\174':
				c = *ip++ + 0100;
				break;
			case '\175':
				c = *ip++ + 0200;
				break;
			case '\176':
				c = *ip++ + 0300;
				break;
			}
			*op++ = c;
			if ( sum & 0x8000 )
			{
				sum <<= 1;
				sum++;
			} else
				sum <<= 1;
			sum += c & 0377;
			sum &= 0xffff;
			special = 0;
			nl++;
		}
	} while ( --len );
	fchksum = sum;
	Debug((8, "%d", nl));
	return nl;
dcorr:
	Debug((4, "Data corrupted"));
	while ( len != FAIL )
	{
		if ( (len = frdbuf(erbp, FIBUFSIZ, fn)) != FAIL )
			*rlen += len;
	}
	return FAIL;
}
#endif	/* X25_PAD */
