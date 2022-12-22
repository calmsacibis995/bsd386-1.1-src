/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	RCSID $Id: t_proto.c,v 1.1.1.1 1992/09/28 20:08:55 trent Exp $
**
**	$Log: t_proto.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:55  trent
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
#define	SYS_TIME
#define	TCP_IP
#define	TERMIOCTL

#include	"global.h"
#include	"cico.h"
#include	"Protocol.h"

#define TPACKSIZE	512
#define TBUFSIZE	1024
#define min(a,b)	(((a)<(b))?(a):(b))

#if	BSD4 < 2 && SYSV == 0
#define	TC	1024
static int	tc = TC;
#endif	/* BSD4 < 2 && SYSV == 0 */

typedef struct
{
	long	t_nbytes;
	char	t_data[TBUFSIZE];
}
		Tbuf;

static int	Retries;

static jmp_buf	T_AlarmJmp;	/* For timeouts */

static int	trdblk(char *, int, int);
static int	twrblk(char *, int, int);


static void
catch_alarm(int sig)
{
	longjmp(T_AlarmJmp, 1);
}


CallType
twrmsg(
	char		type,
	register char *	str,
	int		fn
)
{
	register char *	s;
	int		len, i;
	char		bufr[TBUFSIZE];

	if ( setjmp(T_AlarmJmp) )
		return FAIL;

	(void)signal(SIGALRM, catch_alarm);
	(void)alarm(MAXMSGTIME*5);

	bufr[0] = type;
	s = &bufr[1];
	while ( *str )
		*s++ = *str++;
	*s = '\0';
	if ( *(--s) == '\n' )
		*s = '\0';
	len = strlen(bufr) + 1;
	if ( (i = len % TPACKSIZE) )
	{
		len = len + TPACKSIZE - i;
		bufr[len - 1] = '\0';
	}

	(void)twrblk(bufr, len, fn);

	(void)alarm(0);

	return SUCCESS;
}

CallType
trdmsg(
	register char *	str,
	int		fn
)
{
	int len, cnt = 0;

	if ( setjmp(T_AlarmJmp) )
		return FAIL;
	(void)signal(SIGALRM, catch_alarm);
	(void)alarm(MAXMSGTIME*5);
	for ( ;; )
	{
		len = read(fn, str, TPACKSIZE);
		if ( len <= 0 )
		{
			(void)alarm(0);
			return FAIL;
		}
		str += len;
		cnt += len;
		if ( *(str - 1) == '\0' && (cnt % TPACKSIZE) == 0 )
			break;
	}
	(void)alarm(0);
	return SUCCESS;
}

CallType
twrdata(
	FILE *	fp1,
	int	fn
)
{
	register int	len;
	int		ret;
	long		bytes;
	TimeBuf		start;
	Tbuf		bufr;

	if ( setjmp(T_AlarmJmp) )
		return FAIL;

	bytes = 0L;
	(void)SetTimes();
	start = TimeNow;

	(void)signal(SIGALRM, catch_alarm);

	while ( (len = read(fileno(fp1), bufr.t_data, TBUFSIZE)) > 0 )
	{
		Debug((8, "twrdata sending %d bytes", len));

		bytes += len;
		bufr.t_nbytes = htonl((long)len);
		len += sizeof(long);

		(void)alarm(MAXMSGTIME*5);

		if ( (ret = twrblk((char *)&bufr, len, fn)) != len )
		{
			(void)alarm(0);
			return FAIL;
		}
		if ( len != (TBUFSIZE+sizeof(long)) )
			break;
	}

	if ( len < 0 )
	{
		(void)alarm(0);
		return FAIL;
	}

	bufr.t_nbytes = 0;
	len = sizeof(long);

	(void)alarm(MAXMSGTIME*5);
	ret = twrblk((char *)&bufr, len, fn);
	(void)alarm(0);

	if ( ret != len )
		return FAIL;

	ReportRate(SENT, &start, bytes, bytes, 0);
	return SUCCESS;
}

CallType
trddata(
	int		fn,
	FILE *		fp2
)
{
	register int	len;
	register int	nread;
	long		bytes;
	long		Nbytes;
	TimeBuf		start;
	char		bufr[TBUFSIZE];

	if ( setjmp(T_AlarmJmp) )
		return FAIL;

	bytes = 0L;
	(void)SetTimes();
	start = TimeNow;

	(void)signal(SIGALRM, catch_alarm);

	for ( ;; )
	{
		(void)alarm(MAXMSGTIME*5);

		if ( (len = trdblk((char *)&Nbytes, sizeof Nbytes, fn)) != sizeof Nbytes )
			return FAIL;

		if ( (nread = ntohl(Nbytes)) == 0 )
			break;

		Debug((8, "trddata expecting %ld bytes", nread));

		if ( (len = trdblk(bufr, nread, fn)) < 0 )
		{
			(void)alarm(0);
			return FAIL;
		}

		bytes += len;
		Debug((11, "trddata got %ld", bytes));

		if ( write(fileno(fp2), bufr, len) != len )
		{
			(void)alarm(0);
			return FAIL;
		}
	}
	(void)alarm(0);

	ReportRate(RECEIVED, &start, bytes, bytes, 0);
	return SUCCESS;
}

static int
trdblk(
	char *		blk,
	int		len,
	int		fn
)
{
	register int	i, ret;

#	if	BSD4 < 2 && SYSV == 0
	/** Call touchlock occasionally **/

	if ( --tc < 0 )
	{
		tc = TC;
		touchlock();
	}
#	endif	/* BSD4 < 2 && SYSV == 0 */

	for ( i = 0 ; i < len ; i += ret )
	{
		ret = read(fn, blk, len - i);
		if ( ret < 0 )
			return FAIL;
		blk += ret;
		if ( ret == 0 )
			return i;
	}
	return i;
}

static int
twrblk(
	char *	blk,
	int	len,
	int	fn
)
{
#	if	BSD4 < 2 && SYSV == 0
	/** Call touchlock occasionally **/

	if ( --tc < 0 )
	{
		tc = TC;
		touchlock();
	}
#	endif	/* BSD4 < 2 && SYSV == 0 */

	return write(fn, blk, len);
}
