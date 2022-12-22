/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Classic uucp `g' protocol.
**
**	RCSID $Id: g_proto.c,v 1.3 1994/01/31 01:26:51 donn Exp $
**
**	$Log: g_proto.c,v $
 * Revision 1.3  1994/01/31  01:26:51  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:58:30  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:42:44  vixie
 * Initial revision
 *
 * Revision 1.2  1993/02/28  15:36:37  pace
 * Add recent uunet changes; add P_HWFLOW_ON, etc; add hayesv dialer
 *
 * Revision 1.1.1.1  1992/09/28  20:08:56  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.2  1992/04/17  21:58:04  piers
 * Tracing added everywhere
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
#define	TERMIOCTL

#include	"global.h"
#include	"cico.h"
#include	"Protocol.h"

/*
**	writev(2) (!)
*/

#if	BSD4 >= 2
#include	<sys/uio.h>
#else	/* BSD4 >= 2 */

struct iovec
{
	char *	iov_base;
	int	iov_len;
};

#endif /* BSD4 >= 2 */

/*
**	Natural file-system buffer size.
*/

#ifndef	FSBUFSIZE
#define	FSBUFSIZE	8192
#endif	/* FSBUFSIZE */

/*
**	Packet magic numbers.
*/

#define	DATASIZE	64		/* Standard packet data size */

#define	MAXSEQ		8
#define	MAXWINDOWS	(MAXSEQ-1)
#define WINDOWS		MAXWINDOWS	/* Default windows */

#define	RETRIES		10
#define	MAXNONDATAPKTS	12
#define	MAXPKTTIMEOUT	32
#define	MAXSTARTMSGS	40
#define	MAXTIMEOUT	32

#define READTIMEOUT	4
#define READTIMOINCR	3
#define WRITETIMEOUT	4
#define WRITETIMOINCR	2

#define	CHECK		0xAAAA
#define	SYNC		020
#define	ISCNTRL(a)	((a&0300)==0)	/* Control packet has top two bits == 0 */

/*
**	Packet format:
**
**	S|Z|A|B|C|X| ... data ... |
**
**	where 	S	- initial sync byte
**		Z	- log data size
**		A, B	- sumcheck bytes
**		C	- control byte
**		X	- XOR of header (Z^A^B^C)
**		data	- 0 or more data bytes
*/

/*
**	Packet header.
*/

typedef struct
{
	Uchar	sync;		/* always `^P' */
	Uchar	size;		/* log data size */
	Ushort	sum;		/* checksum of data ^ cntrl */
	Uchar	cntrl;		/* packet control byte */
	Uchar	hxor;		/* header xor of size->cntrl */
}
		Hdr;

#define	HDRSIZE		6	/* Packet header size */

/*
**	Packet buffer.
*/

typedef struct
{
	char *	buf;		/* data */
	Ushort	sum;		/* checksum */
	short	size;		/* data size */
	char	status;		/* buffer status */
}
		Buf;

/*
**	Receive/transmit status.
*/

typedef struct
{
	char *	pool;		/* buffer pool */
	short	fd;		/* file descriptor for circuit */
	short	size;		/* negotiated data size */
	Hdr	hdr;		/* header */
	char	count;		/* active buffer count */
	char	window;		/* negotiated window size */
}
		State;

/*
**	Control structure.
*/

typedef struct
{
	State	p_in;		/* input status */
	State	p_out;		/* output status */
	char *	p_rptr;		/* residual data in an input block */
	short	p_state;	/* line state */
	char	p_ps;		/* last packet sent */
	char	p_pr;		/* last packet received */
	char	p_rpr;		/* ACKed sequence */
	char	p_nextseq;	/* next output sequence */
	char	p_msg;		/* control packet */
	char	p_rmsg;		/* retry control packet */
	char	p_imap;		/* bit map of input buffers */
	char	p_oseq;		/* newest output packet */
	char	p_obusy;	/* output busy (output() not recursive) */
	Buf	p_ibuf[MAXSEQ];	/* input buffers */
	Buf	p_obuf[MAXSEQ];	/* output buffers */
}
		Pkt;

/*
**	Driver states.
*/

#define	DEAD		00000
#define	INITa		00001
#define	INITb		00002
#define	INITc		00004
#define	LIVE		00010
#define	RXMIT		00020
#define	RREJ		00040
#define	FLUSH		00100
#define	DOWN		00200
#define	RCLOSE		00400

#define	INITab	(INITa|INITb)

/*
**	Buffer states.
*/

#define	B_NULL		0000
#define	B_READY		0001
#define	B_SENT		0002
#define	B_RESID		0010
#define	B_COPY		0020
#define	B_MARK		0040
#define	B_SHORT		0100

/*
**	Control messages.
*/

#define	CLOSE		1
#define	RJ		2
#define	SRJ		3
#define	RR		4
#define	INITC		5
#define	INITB		6
#define	INITA		7

#define	C_CLOSE		0002	/* These are: 1<<CLOSE, etc. */
#define	C_RJ		0004
#define	C_SRJ		0010
#define	C_RR		0020
#define	C_INITC		0040
#define	C_INITB		0100
#define	C_INITA		0200

/*
**	Packet sequence numbers
*/

static char	NextSeq[MAXSEQ]	= { 1, 2, 3,   4,   5,   6,    7,    0 };
static char	MaskSeq[MAXSEQ]	= { 1, 2, 4, 010, 020, 040, 0100, 0200 };

/*
**	Packet sizes.
*/

static short	PktSizes[] =
{
/*     (0)  1   2    3    4    5     6     7     8 (9)	[log2(size)-4]	*/
	1, 32, 64, 128, 256, 512, 1024, 2048, 4096, 1
};

#define	MINDATASIZE	32
#define	MAXDATASIZE	4096

/*
**	Local data.
*/

static bool	Gproto;		/* 'G' selected - other end does fixed-size negotiation */
static int	NonDataPkts;
static int	Ntimeout;
static Pkt *	Pk;		/* The one and only Packet */
static char	PkErr[]		= "`g' protocol error: %s %s";
static char	PkSizeErr[]	= "Illegal %s size specified (%d), using %d";
static int	PktTimeout	= WRITETIMEOUT;
static int	PktTimeIncr	= WRITETIMOINCR;
static Ulong	RdPauses;
static int	Reacks;
static int	Retries;

static jmp_buf	Failbuf;	/* For public interface */
static jmp_buf	RdAlrmJmp;	/* For read timeouts */

/*
**	Public <> private interface routines.
*/

#define		gwrblk(A,B)		writedata(A, B)

static int	grdblk(char *, int);
#ifndef	gwrblk
static int	gwrblk(char *, int);
#endif	/* gwrblk */

/*
**	Packet driver routines.
*/

static int	acceptpkt(void);
static void	ackpkts(void);
static void	bufferdata(char, Ushort, char *, int);
static int	chkdatasize(int);
static void	docontrol(char);
static void	freepool(State *);
static char *	getpool(char **, int);
static bool	init(int, int);
static int	logsize(int);
static void	output(void);
static void	pktfail(void);
static int	pktsize(int);
static void	putpool(char **, char **);
static int	readdata(char *, int);
static void	readpause(int, long *);
static bool	readVC(int, char *, int);
static void	recvpkt(void);
static void	reset(void);
static void	sendpkt(char, int);
static void	terminate(void);
static int	writedata(char *, int);

/*
**	Debugging.
*/

static void	prt_cntrl(char);
static void	prt_state(int);

/*
**	Miscellaneous.
*/

#define	Fprintf		(void)fprintf
#define	Fputc		(void)fputc



/*
**	Protocol interface routines.
*/

/*
**	Turn on/off -- `Pk' is the global control point.
*/

CallType
Gturnon()
{
	Trace((1, "Gturnon"));

	Gproto = true;
	return gturnon();
}

CallType
gturnon()
{
	Trace((1, "gturnon"));

	if ( setjmp(Failbuf) )
		return FAIL;

	if ( !init(Ifd, Ofd) )
		return FAIL;

	return SUCCESS;
}

CallType
gturnoff()
{
	Trace((1, "gturnoff"));

	if ( setjmp(Failbuf) )
		return FAIL;

	terminate();

	return SUCCESS;
}

/*
**	Send a typed, null-terminated, message.
**
**	Write <type><str><\0> to `fn'.
**
**	Remove any trailing '\n'.
*/

CallType
gwrmsg(
	char		type,
	register char *	str,
	int		fn
)
{
	register char *	s;
	int		i;
	int		len;
	char		bufr[BUFSIZ];

	Trace((4, "gwrmsg(%c, %.8s, %d)", type, str, fn));

	if ( setjmp(Failbuf) || Pk == (Pkt *)0 )
		return FAIL;

	s = bufr;

	*s++ = type;

	while ( *s++ = *str++ );

	if ( s[-2] == '\n' )
	{
		s -= 2;
		*s++ = '\0';
	}

	len = s - bufr;

	if ( (Gproto || Pk->p_out.size == DATASIZE) && (i = len % DATASIZE) )
	{
		len += DATASIZE - i;	/* Round up to nearest packet boundary */
		bufr[len - 1] = '\0';	/* For fixed-length messages to old uucico */
	}

	if ( gwrblk(bufr, len) > 0 )
		return SUCCESS;

	return FAIL;
}

/*
**	Read a null-terminated message from `fn'.
*/

CallType
grdmsg(
	register char *	str,
	int		fn
)
{
	register int	len;
	register int	n;

	Trace((4, "grdmsg(%#x, %d)", str, fn));

	if ( setjmp(Failbuf) || Pk == (Pkt *)0 )
		return FAIL;

	if ( Gproto || Pk->p_out.size == DATASIZE )
		n = DATASIZE;		/* Read fixed length message packets from old uucico */
	else
		n = Pk->p_in.size;	/* Read unknown length message packet */

	for ( ;; )
	{
		if ( (len = readdata(str, n)) == 0 )
			continue;

		str += len;

		if ( str[-1] == '\0' )
			break;
	}

	return SUCCESS;
}

/*
**	Copy data from `fp1' to `fn`.
*/

CallType
gwrdata(
	FILE *		fp1,
	int		fn
)
{
	register int	len;
	long		bytes;
	int		ret;
	TimeBuf		start;
	char		bufr[FSBUFSIZE];

	Trace((4, "gwrdata(%d, %d)", fileno(fp1), fn));

	if ( setjmp(Failbuf) || Pk == (Pkt *)0 )
		return FAIL;

	bytes = 0L;
	Retries = 0;

	(void)SetTimes();
	start = TimeNow;

	while ( (len = read(fileno(fp1), bufr, sizeof(bufr))) > 0 )
	{
		bytes += len;

		if ( (ret = gwrblk(bufr, len)) != len )
			return FAIL;

		if ( len != sizeof(bufr) )
			break;
	}

	if ( len < 0 || gwrblk(bufr, 0) < 0 )
		return FAIL;

	ReportRate(SENT, &start, bytes, bytes, Retries);

	return SUCCESS;
}

/*
**	Copy data from `fn' to `fp2`.
*/

CallType
grddata(
	int		fn,
	FILE *		fp2
)
{
	register int	len;
	long		bytes;
	TimeBuf		start;
	char		bufr[FSBUFSIZE];

	Trace((4, "grddata(%d, %d)", fn, fileno(fp2)));

	if ( setjmp(Failbuf) || Pk == (Pkt *)0 )
		return FAIL;

	bytes = 0L;
	Retries = 0;

	(void)SetTimes();
	start = TimeNow;

	for ( ;; )
	{
		if ( (len = grdblk(bufr, sizeof(bufr))) < 0 )
			return FAIL;

		bytes += len;

		if ( write(fileno(fp2), bufr, len) != len )
			return FAIL;

		if ( len < sizeof(bufr) )
			break;
	}

	ReportRate(RECEIVED, &start, bytes, bytes, Retries);

	return SUCCESS;
}

/*
**	Public/private interface routines.
*/

/*
**	Assemble block of data from packets.
*/

static int
grdblk(
	char *		blk,
	int		len
)
{
	register int	i;
	register int	ret;

	Trace((5, "grdblk(%#x, %d)", blk, len));

	for ( i = 0 ; i < len ; i += ret )
	{
		if ( (ret = readdata(blk, len - i)) < 0 )
			return FAIL;

		blk += ret;

		if ( ret == 0 )
			return i;
	}

	return i;
}

/*
**	Disassemble block of data to packets.
*/

#ifndef	gwrblk

static int
gwrblk(
	char *	blk,
	int	len
)
{
	Trace((5, "gwrblk(%#x, %d)", blk, len));

	return writedata(blk, len);
}
#endif	/* gwrblk */

/*
**	Packet driver routines.
*/

/*
**	Read packet, sumcheck data, allocate to input buffer.
*/

static int
acceptpkt()
{
	register Pkt *	pk	= Pk;
	register Buf *	bp;
	register int	x;
	register int	cc;
	register int	seq;
	int		accept;
	int		bad;
	char		cntrl;
	char		imask;
	char		m;
	char *		p;
	int		skip;
	Ushort		sum;
	int		t;

	Trace((6, "acceptpkt()"));

	bad = accept = skip = 0;

	/*
	**	Wait for input.
	*/

	x = NextSeq[pk->p_pr];

	while ( (imask = pk->p_imap) == 0 && pk->p_in.count == 0 )
		recvpkt();

	pk->p_imap = 0;

	/*
	**	Determine input window in m.
	*/

	t = (~(-1<<(int)(pk->p_in.window))) << x;
	m = t;
	m |= t>>8;

	/*
	**	Mark newly accepted input buffers.
	*/

	for ( x = 0 ; x < MAXSEQ ; x++ )
	{
		if ( (imask & MaskSeq[x]) == 0 )
			continue;

		bp = &pk->p_ibuf[x];

		if ( ((cntrl = bp->status) & 0200) == 0 )
		{
			bad++;
freeb:
			putpool(&pk->p_in.pool, &bp->buf);
			bp->status = 0;
			continue;
		}

		bp->status = ~(B_COPY|B_MARK);

		sum = (unsigned)chksum(bp->buf, bp->size) ^ (unsigned)(cntrl&0377);
		sum += bp->sum;

		if ( sum == CHECK )
		{
			seq = (cntrl>>3) & 07;

			if ( m & MaskSeq[seq] )
			{
				if ( pk->p_ibuf[seq].status & (B_COPY|B_MARK) )
				{
dupb:					pk->p_msg |= C_RR;
					skip++;
					goto freeb;
				}

				if ( x != seq )
				{
					register Buf *	ob = &pk->p_ibuf[seq];

					cc = bp->size;
					bp->size = ob->size;
					ob->size = cc;

					p = bp->buf;
					bp->buf = ob->buf;
					ob->buf = p;

					bp->status = ob->status;
					bp = ob;
				}

				bp->status = B_MARK;
				accept++;
				cc = 0;

				if ( cntrl & B_SHORT )
				{
					bp->status = B_MARK|B_SHORT;

					p = bp->buf;
					cc = (unsigned)*p++ & 0377;
					if ( cc & 0200 )
					{
						cc &= 0177;
						cc |= *p << 7;
					}

					bp->size -= cc;
				}
			}
			else
				goto dupb;
		}
		else
		{
			bad++;
			goto freeb;
		}
	}

	/*
	**	Scan window again turning marked buffers into
	**	COPY buffers and looking for missing sequence
	**	numbers.
	*/

	accept = 0;
	t = -1;

	for ( x = NextSeq[pk->p_pr] ; m & MaskSeq[x] ; x = NextSeq[x] )
	{
		bp = &pk->p_ibuf[x];

		if ( bp->status & B_MARK )
			bp->status |= B_COPY;

		if ( bp->status & B_COPY )
		{
			if ( t >= 0 )
			{
				putpool(&pk->p_in.pool, &bp->buf);
				bp->status = 0;
				skip++;
			}
			else
				accept++;
		}
		else
		if ( t < 0 )
			t = x;
	}

	if ( bad )
		pk->p_msg |= C_RJ;

	if ( skip )
		pk->p_msg |= C_RR;

	pk->p_in.count = accept;
	return accept;
}

/*
**	Mark acknowledged all packets sent with seq < rpr.
*/

static void
ackpkts()
{
	register Pkt *	pk = Pk;
	register int	x;

	Trace((6, "ackpkts()"));

	for ( x = pk->p_ps ; x != pk->p_rpr ; )
	{
		x = NextSeq[x];

		if ( pk->p_obuf[x].status & B_SENT )
		{
			pk->p_obuf[x].status = B_NULL;
			pk->p_out.count--;
			putpool(&pk->p_out.pool, &pk->p_obuf[x].buf);
			pk->p_ps = x;
		}
	}
}

/*
**	Receive data buffer in spare input slot, else free.
*/

static void
bufferdata(
	char		c,
	Ushort		sum,
	char *		buf,
	int		n
)
{
	register Pkt *	pk = Pk;
	register Buf *	bp;
	register int	x;
	register int	t;

	Trace((6, "bufferdata(%0o, %#x, %#x, %d)", (int)c&0xff, sum, buf, n));

	if ( (pk->p_state & FLUSH) || !(pk->p_state & LIVE) )
	{
		pk->p_msg |= pk->p_rmsg;
		output();
	}
	else
	if ( c != '\0' )
	{
		t = NextSeq[pk->p_pr];

		for ( x = pk->p_pr ; x != t ; x = (x-1)&7 )
			if ( (bp = &pk->p_ibuf[x])->status == 0 )
			{
				pk->p_imap |= MaskSeq[x];
				bp->buf = buf;
				bp->sum = sum;
				bp->size = n;
				bp->status = c;
				return;
			}
	}
	else
		Warn("data packet has zero control byte");

	putpool(&pk->p_in.pool, &buf);
}

/*
**	Alarms set on read from VC.
*/

static void
catch_rd_alarm()
{
	longjmp(RdAlrmJmp, 1);
}

/*
**	Check requested packet size.
*/

static int
chkdatasize(
	int		size
)
{
	register short	*ip;
	register short	*ep;
	register int	s;

	Trace((6, "chkdatasize(%d)", size));

	if ( size == 0 )
		size = DATASIZE;	/* Default */

	Debug((1, "Accepting packet size %d", size));

	if ( size == DATASIZE )
		return size;		/* Default */

	for ( ip = PktSizes, ep = &ip[ARR_SIZE(PktSizes)] ; ip < ep ; )
	{
		if ( (s = *ip++) < MINDATASIZE || s > MAXDATASIZE )
			continue;

		if ( s == size )
			return size;
	}

	Warn(PkSizeErr, "packet", size, DATASIZE);
	return DATASIZE;
}

/*
**	Receive control messages.
*/

static void
docontrol(
	char		c
)
{
	register Pkt *	pk = Pk;
	register int	cntrl;
	register int	val;

	Trace((6, "docontrol(%0o)", (int)c&0xff));

	if ( !ISCNTRL(c) )
	{
		Log(PkErr, "not control", EmptyStr);
		return;
	}

	val = c & 07;
	cntrl = (c>>3) & 07;

	switch ( cntrl )
	{
	case INITA:
		if ( val == 0 && (pk->p_state & LIVE) )
		{
			Log(PkErr, "dynamic size change not implemented", EmptyStr);
			break;
		}

		if ( val )
		{
			pk->p_state |= INITa;
			pk->p_msg |= C_INITB;
			pk->p_rmsg |= C_INITB;

			if ( pk->p_in.window != val && SysWindows == 0 )
			{
				Debug((1, "Receiving %d windows", val));
				pk->p_in.window = val;	/* Will go out in INITC */
			}
set_window:
			Debug((1, "Sending %d windows", val));
			pk->p_out.window = val;
		}
		break;

	case INITB:
		pk->p_out.size = PktSizes[++val];	/* 0..7 => 1..8 [32 .. 4096] */

		Debug((1, "Sending packet size %d", pk->p_out.size));

		if ( pk->p_state & LIVE )
		{
			pk->p_msg |= C_INITC;
			break;
		}

		pk->p_state |= INITb;

		if ( (pk->p_state & INITa) == 0 )
			break;

		pk->p_rmsg &= ~C_INITA;

		/** 'G' gets upset at repeated INITB **/

		if ( pk->p_in.size != pk->p_out.size && SysPktSize == 0 && !Gproto )
		{
			Debug((1, "Receiving packet size %d", pk->p_out.size));
			pk->p_in.size = pk->p_out.size;
			pk->p_state |= INITc;
			pk->p_msg |= C_INITB;	/* Re-negotiate to match remote */
		}
		else
			pk->p_msg |= C_INITC;
		break;

	case INITC:
		if ( (pk->p_state & INITab) == INITab )
		{
			if ( pk->p_state & INITc )
				pk->p_msg |= C_INITC;
			pk->p_state = LIVE;
			pk->p_rmsg &= ~C_INITB;
		}
		else
		if ( !(pk->p_state & LIVE) )
			pk->p_msg |= (pk->p_state&INITa)?C_INITB:C_INITA;

		if ( val && pk->p_out.window != val )
			goto set_window;
		break;

	case RJ:
		pk->p_state |= RXMIT;
		pk->p_msg |= C_RR;
		pk->p_rpr = val;
		ackpkts();
		break;

	case RR:
		pk->p_rpr = val;

		if ( pk->p_rpr == pk->p_ps )
		{
			++Reacks;
			Debug((9, "Reack count is %d", Reacks));
			if ( Reacks >= 4 )
			{
				Debug((6, "Reack overflow on %d", val));
				pk->p_state |= RXMIT;
				pk->p_msg |= C_RR;
				Reacks = 0;
			}
		}
		else
		{
			Reacks = 0;
			ackpkts();
		}
		break;

	case SRJ:
		Log(PkErr, "SRJ not implemented", EmptyStr);
		break;

	case CLOSE:
		pk->p_state = DOWN|RCLOSE;
		return;
	}

	if ( pk->p_msg )
		output();
}

/*
**	Free up a buffer pool.
*/

static void
freepool(
	State *		sp
)
{
	register char **pp = &sp->pool;
	register char *	bp;
	register int	i;

	Trace((6, "freepool(%#x)", (int)sp));

	for ( i = 0 ; (bp = *pp) != NULLSTR ; i++ )
	{
		*pp = *(char **)bp;
		free(bp);
	}

	if ( i > MAXSEQ )	/* XXX why is this > `sp->window' ? */
		Warn("free'd %d buffers for %d windows", i, sp->window);
}

/*
**	Get next free pool buffer.
*/

static char *
getpool(
	char **	pp,
	int	size
)
{
	char *	bp;

	Trace((6, "getpool(%#x, %d)", (int)pp, size));

	if ( (bp = *pp) == NULLSTR )
	{
		Debug((4, "New %s buffer [%d]", (pp==&Pk->p_in.pool)?"input":"output", size));
		return Malloc(size);
	}

	*pp = *(char **)bp;	/* Chain */
	return bp;
}

/*
**	Start initial synchronization.
*/

static bool
init(
	int		ifd,
	int		ofd
)
{
	register Pkt *	pk;
	register char **bp;
	register int	i;

	Trace((5, "init(%d, %d)", ifd, ofd));

	if ( (pk = Pk) != (Pkt *)0 )
		return true;	/* Already open */

	Pk = pk = (Pkt *)Malloc0(sizeof(Pkt));
	pk->p_in.fd = ifd;
	pk->p_out.fd = ofd;

	if ( (i = SysWindows) == 0 )
		i = WINDOWS;
	else
	if ( i > MAXWINDOWS )
	{
		Warn(PkSizeErr, "window", i, WINDOWS);
		i = WINDOWS;
	}

	Debug((1, "Accepting %d windows", i));

	pk->p_in.window = pk->p_out.window = i;
		
	pk->p_out.size = pk->p_in.size = chkdatasize(SysPktSize);

	/** Start synchronization **/

	pk->p_msg = pk->p_rmsg = C_INITA;

	output();

	for ( i = 0 ; i < MAXSTARTMSGS ; i++ )
	{
		recvpkt();

		if ( pk->p_state & LIVE )
		{
			reset();
			return true;
		}
	}

	Debug((1, "init: i >= MAXSTARTMSGS"));
	FreeStr((char **)&Pk);
	return false;
}

/*
**	Calculate log2(n)-4 for packet header data size byte.
*/

static int
logsize(
	int		an
)
{
	register int	n;
	register int	k;

	for ( n = an >> 5, k = 0 ; (n >>= 1) != 0 ; k++ );

	Trace((6, "logsize(%d) => %d", an, k));

	return k;
}

/*
**	Find packets to transmit.
*/

static void
output()
{
	register Pkt *	pk = Pk;
	register int	x;
	register int	i;
	char		obufst;

	Trace((6, "output()"));

	if ( pk->p_obusy++ )
	{
		pk->p_obusy--;
		return;
	}

	/*
	**	Find sequence number and buffer state
	**	of next output packet.
	*/

	if ( pk->p_state & RXMIT )
		pk->p_nextseq = NextSeq[pk->p_rpr];

	x = pk->p_nextseq;
	obufst = pk->p_obuf[x].status;

	/*
	**	Send control packet if indicated.
	*/

	if ( (i = pk->p_msg) && ((i & ~C_RR) || !(obufst & B_READY)) )
	{
		x = i;

		for ( i = 0 ; i < MAXSEQ ; i++ )
		{
			if ( x & 1 )
				break;
			x >>= 1;	/* Turn C_CLOSE into CLOSE, etc. */
		}

		x = i << 3;

		switch ( i )
		{
		case CLOSE:
		case SRJ:
			break;

		case RJ:
		case RR:
			x |= pk->p_pr;
			break;

		case INITA:
		case INITC:
			x |= pk->p_in.window;
			break;

		case INITB:
			x |= logsize(pk->p_in.size);
			break;
		}

		pk->p_msg &= ~MaskSeq[i];
		sendpkt((char)x, -1);
		pk->p_obusy = 0;
		return;
	}

	/*
	**	Don't send data packets if line is marked dead.
	*/

	if ( pk->p_state & DOWN )
	{
		pk->p_obusy = 0;
		return;
	}

	/*
	**	Start transmission (or retransmission) of data packets.
	*/

	if ( obufst & (B_READY|B_SENT) )
	{
		obufst |= B_SENT;
		pk->p_nextseq = NextSeq[x];

		i = 0200 | pk->p_pr | (x<<3);
		if ( obufst & B_SHORT )
			i |= 0100;

		sendpkt((char)i, x);

		pk->p_obuf[x].status = obufst;
		pk->p_state &= ~RXMIT;
	}

	pk->p_obusy = 0;
}

/*
**	Error control.
*/

static void
pktfail()
{
	longjmp(Failbuf, 1);
}

/*
**	Find power of 2 packet size that will fit.
**
**	Called from writedata() only if `size' < Pk->p_out.size.
*/

static int
pktsize(
	int		size
)
{
	register short	*ip;
	register short	*ep;
	register int	s;

	Trace((6, "pktsize(%d)", size));

	if ( Pk->p_out.size == DATASIZE )
		return DATASIZE;	/* Back-compat. */

	if ( Gproto )
		return Pk->p_out.size;

	for ( ip = PktSizes, ep = &ip[ARR_SIZE(PktSizes)] ; ip < ep ; )
	{
		if ( (s = *ip++) < MINDATASIZE || s > MAXDATASIZE )
			continue;

		if ( s >= size )
			return s;
	}

	return DATASIZE;
}

/*
**	Return buffer to pool.
*/

static void
putpool(
	char **		pp,
	char **		bpp
)
{
	register char *	bp;

	Trace((6, "putpool(%#x, %#x)", pp, bpp));

	if ( (bp = *bpp) == NULLSTR )
		return;

	*(char **)bp = *pp;	/* Chain */
	*pp = bp;
	*bpp = NULLSTR;
}

/*
**	Receive a packet, copy contents to `ibuf'.
*/

static int
readdata(
	char *		ibuf,
	int		icount
)
{
	register Pkt *	pk = Pk;
	register int	x;
	register int	cc;
	int		is;
	int		xfr;
	int		count;
	char *		cp;

	Trace((5, "readdata(%#x, %d)", ibuf, icount));

	xfr = 0;
	count = 0;
	PktTimeout = READTIMEOUT;
	PktTimeIncr = READTIMOINCR;
	Ntimeout = 0;

	while ( acceptpkt() == 0 );

	while ( icount )
	{
		x = NextSeq[pk->p_pr];
		is = pk->p_ibuf[x].status;

		if ( !(is & B_COPY) )
			break;

		if ( (cc = pk->p_ibuf[x].size) > icount )
			cc = icount;

		if ( cc == 0 && xfr )
			break;

		if ( is & B_RESID )
			cp = pk->p_rptr;
		else
		{
			cp = pk->p_ibuf[x].buf;

			if ( (is & B_SHORT) && (*cp++ & 0200) )
				cp++;
		}

		bcopy(cp, ibuf, cc);

		ibuf += cc;
		icount -= cc;
		count += cc;
		xfr++;

		if ( (pk->p_ibuf[x].size -= cc) == 0 )
		{
			pk->p_pr = x;
			putpool(&pk->p_in.pool, &pk->p_ibuf[x].buf);
			pk->p_ibuf[x].status = 0;
			pk->p_in.count--;
			pk->p_msg |= C_RR;
		}
		else
		{
			pk->p_rptr = cp+cc;
			pk->p_ibuf[x].status |= B_RESID;
		}

		if ( cc == 0 )
			break;
	}

	output();

	return count;
}

/*
**	Pause if warranted (POSIX "termios" has VMIN/VTIME).
*/

#ifndef USE_TERMIOS
static void
readpause(
	int		n,
	long *		itime
)
{
	register long	r;
	int		bps;
	struct timeval	tv;

	Trace((9, "readpause(%d, %ld)", n, *itime));

	/** Use calculated effective throughput if available **/

	if ( Stats.bps_count > 0 )
		bps = Stats.bps_total / Stats.bps_count;
	else
		bps = (Baud * 8) / 10;	/* Data bits/sec. */

	if ( bps <= 0 )
		return;

	/*
	**	Calculate r = usecs to read `n' bytes.
	**
	**	(usecs = bytes * 8000000 / bps)
	*/

	r = n;
	r *= 80000L;
	r /= bps;
	r *= 100;
	r -= *itime;
	*itime = 0;

	if ( r < 20000 )
		return;		/* < 1/50th second */

	tv.tv_sec = r / 1000000L;
	tv.tv_usec = r % 1000000L;

	Debug((9, "read pause for %d.%06d sec", tv.tv_sec, tv.tv_usec));

	(void)select(0, (int *)0, (int *)0, (int *)0, &tv);

	DODEBUG(RdPauses++);
}
#endif	/* USE_TERMIOS */

/*
**	Read `n' bytes from VC.
*/

static bool
readVC(
	int		fn,
	register char *	b,
	register int	n
)
{
	register int	ret;
#ifndef USE_TERMIOS
	/** Guess: it's been 1/100th second since we last read the line **/
	long		itime	= 10000L;
#endif

	Trace((8, "readVC(%d, %#x, %d)", fn, b, n));

	if ( setjmp(RdAlrmJmp) )
	{
		Ntimeout++;
		Debug((4, "circuit read alarm %d (%d secs)", Ntimeout, PktTimeout));
		PktTimeout += PktTimeIncr;
		if ( PktTimeout > MAXPKTTIMEOUT )
			PktTimeout = MAXPKTTIMEOUT;
		return false;
	}

	(void)signal(SIGALRM, catch_rd_alarm);
	(void)alarm(PktTimeout);

	while ( n > 0 )
	{
#ifndef USE_TERMIOS
		if ( Is_a_tty )
			readpause(n, &itime);
#endif
		Trace((9, "read(%d, %#x, %d)", fn, b, n));

		if ( (ret = read(fn, b, n)) == 0 )
		{
			Trace((4, "readVC 0"));
			(void)alarm(0);
			return false;
		}

		Trace((9, "readVC => %d", ret));

		if ( ret < 0 )
		{
			(void)alarm(0);
			Log(PkErr, "read:", ErrnoStr(errno));
			pktfail();
		}

		b += ret;
		n -= ret;
	}

	(void)alarm(0);

	return true;
}

/*
**	Read in packet header, determine/check type, read in data bytes.
*/

static void
recvpkt()
{
	register Pkt *	pk = Pk;
	register Hdr *	h;
	register Uchar *p;
	register Ushort	sum;
	register int	k;
	register int	n;
	char *		bp;
	Uchar		cntrl;
	Uchar		hdchk;
	int		noise;
	int		tries;

	static int	same_cntrls;
	static Uchar	last_cntrl;

	Trace((6, "recvpkt()"));

	if ( (pk->p_state & DOWN) || NonDataPkts > MAXNONDATAPKTS || Ntimeout > MAXTIMEOUT )
		pktfail();

	/* Find packet header */

	p = (Uchar *)&pk->p_in.hdr;

	for ( tries = 0, noise = 0 ; tries < RETRIES ; )
	{
		if ( readVC(pk->p_in.fd, (char *)p, 1) )
		{
			if ( p[0] == SYNC )
			{
				if ( readVC(pk->p_in.fd, (char *)&p[1], HDRSIZE-1) )
					break;
			}
			else
			if ( ++noise < (3*pk->p_in.size) )
				continue;

			Debug((4, "Noisy line - retransmit"));
			noise = 0;
		}

		/** Set up retransmit or REJ **/

		tries++;
		Retries++;

		if ( (pk->p_msg |= pk->p_rmsg) == 0 )
			pk->p_msg |= C_RR;

		if ( pk->p_state & LIVE )
			pk->p_state |= RXMIT;

		output();
	}

	if ( tries >= RETRIES )
	{
		Debug((4, "retries = %d", tries));
		pktfail();
	}

	NonDataPkts++;

	h = &pk->p_in.hdr;
	p = (Uchar *)h;
	hdchk = p[1] ^ p[2] ^ p[3] ^ p[4];
	p += 2;
	sum = *p++;
	sum |= *p << 8;
	h->sum = sum;
	k = h->size;
	cntrl = h->cntrl;

	if ( Debugflag > 7 )
	{
		Fprintf(DebugFd, "\trecv\t");

		if ( ISCNTRL(cntrl) )
			prt_cntrl(cntrl);
		else
			Fprintf(DebugFd, "0%3o\tDATA\tpacket %d [%d]\n", cntrl, (cntrl>>3)&07, k);
	}

	if ( hdchk != h->hxor )
	{
		Debug((7, "bad header 0%o, h->hxor 0%o", hdchk, h->hxor));
		return;
	}

	if ( cntrl == last_cntrl )
	{
		if ( ++same_cntrls >= MAXNONDATAPKTS )
		{
			Warn("excessive repeated packets");
			pktfail();
		}
	}
	else
	{
		same_cntrls = 0;
		last_cntrl = cntrl;
	}

	if ( k == 9 )	/* Control packet */
	{
		if ( ((sum + cntrl) & 0xffff) == CHECK )
			docontrol(cntrl);
		else
		{
			static char	badhdr[] = "bad control packet header (ctrl=0%o, sum=0%o, ctrl+sum=0%o, chk=0%o)";

			/** Bad header **/

			Warn(badhdr, cntrl, sum, ((sum + cntrl) & 0xffff), hdchk);
		}

		if ( Debugflag > 7 )
		{
			static int	laststate = -1;

			if ( pk->p_state != laststate )
				prt_state(laststate = pk->p_state);
		}

		return;
	}

	/** Data packet **/

	if ( k > 0 && k < 9 && (n = PktSizes[k]) <= pk->p_in.size )
	{
		pk->p_rpr = cntrl & 07;

		ackpkts();

		bp = getpool(&pk->p_in.pool, pk->p_in.size);

		NonDataPkts = 0;

		if ( readVC(pk->p_in.fd, bp, n) )
			bufferdata(cntrl, sum, bp, n);
		else
			putpool(&pk->p_in.pool, &bp);
	}
	else
	{
		if ( k > 0 && k < 9 )
		{
			k = PktSizes[k];
			bp = EmptyStr;
		}
		else
			bp = "index ";

		Warn("invalid packet size %s%d", bp, k);
	}
}

/*
**	Reset `Packet'.
*/

static void
reset()
{
	register Pkt *	pk = Pk;

	Trace((4, "reset()"));

	pk->p_ps = pk->p_pr = pk->p_rpr = 0;
	pk->p_nextseq = 1;
}

/*
**	Start transmission of packet `x'.
*/

static void
sendpkt(
	char		cntrl,
	register int	x
)
{
	register Pkt *	pk = Pk;
	register Buf *	bp;
	register char *	p;
	short		checkword;
	Uchar		hdchk;

	Trace((6, "sendpkt(%0o, %d)", (int)cntrl&0xff, x));

	p = (char *)&pk->p_out.hdr;

	*p++ = SYNC;

	if ( x < 0 )
	{
		*p++ = hdchk = 9;
		checkword = cntrl;
	}
	else
	{
		bp = &pk->p_obuf[x];
		*p++ = hdchk = logsize(bp->size) + 1;
		checkword = bp->sum ^ (unsigned)(cntrl & 0377);
	}

	checkword = CHECK - checkword;
	*p = checkword;
	hdchk ^= *p++;
	*p = checkword>>8;
	hdchk ^= *p++;
	*p = cntrl;
	hdchk ^= *p++;
	*p = hdchk;

	if ( Debugflag > 7 )
	{
		Fprintf(DebugFd, "\tsend\t");

		if ( ISCNTRL(cntrl) )
			prt_cntrl(cntrl);
		else
			Fprintf(DebugFd, "0%3o\tDATA\tpacket %d [%d]\n", cntrl&0xff, (cntrl>>3)&07, pk->p_out.hdr.size);
	}

	if ( setjmp(RdAlrmJmp) )
	{
		if ( ++Ntimeout > MAXTIMEOUT )
		{
			Log(PkErr, "write timeout");
			pktfail();
		}
		Debug((4, "circuit write alarm %d (%d secs)", Ntimeout, PktTimeout));
		PktTimeout += PktTimeIncr;
		if ( PktTimeout > MAXPKTTIMEOUT )
			PktTimeout = MAXPKTTIMEOUT;
	}

	(void)signal(SIGALRM, catch_rd_alarm);
	(void)alarm(PktTimeout);

	p = (char *)&pk->p_out.hdr;

	if ( x < 0 )
	{
		if ( write(pk->p_out.fd, p, HDRSIZE) != HDRSIZE )
		{
			(void)alarm(0);
			Log(PkErr, "write:", ErrnoStr(errno));
			pktfail();
		}
	}
	else
	{
		struct iovec	iov[2];

		iov[0].iov_base = p;
		iov[0].iov_len = HDRSIZE;
		iov[1].iov_base = bp->buf;
		iov[1].iov_len = bp->size;

		if ( writev(pk->p_out.fd, iov, 2) == SYSERROR )
#if	0
		register int	i;
		register char *	b;
		char		buf[MAXDATASIZE+HDRSIZE];

		for ( i = HDRSIZE, b = buf ; --i >= 0 ; )
			*b++ = *p++;
		for ( i = bp->size, p = bp->buf ; --i >= 0 ; )
			*b++ = *p++;

		i = HDRSIZE + bp->size;

		if ( write(pk->p_out.fd, buf, i) != i )
#endif	/* 0 */
		{
			(void)alarm(0);
			Log(PkErr, "write:", ErrnoStr(errno));
			pktfail();
		}

		NonDataPkts = 0;
	}

	(void)alarm(0);

	if ( pk->p_msg )
		output();
}

/*
**	Shut down line by:
**	ignoring new input,
**	letting output drain,
**	and releasing space.
*/

static void
terminate()
{
	register Pkt *	pk;
	register int	i;

	Trace((4, "terminate()"));

	if ( (pk = Pk) == (Pkt *)0 )
		return;	/* Already closed */

	/*
	**	Try to flush output.
	*/

	pk->p_state |= FLUSH;

	for
	(
		i = 0
		;
		pk->p_out.count
		&&
		(pk->p_state & (LIVE|RCLOSE|DOWN)) == LIVE
		&&
		i < 2
		;
		i++
	)
		output();

	pk->p_state |= DOWN;

	/*
	**	Try to exchange CLOSE messages.
	*/

	for
	(
		i = 0
		;
		!(pk->p_state & RCLOSE) && i < 2
		;
		i++
	)
	{
		pk->p_msg = C_CLOSE;
		output();
	}

	/*
	**	Free space.
	*/

	for ( i = 0 ; i < MAXSEQ ; i++ )
	{
		if ( pk->p_obuf[i].status != B_NULL )
		{
			FreeStr(&pk->p_obuf[i].buf);
			pk->p_out.count--;
		}

		if ( pk->p_ibuf[i].status != B_NULL )
			FreeStr(&pk->p_ibuf[i].buf);
	}

	freepool(&pk->p_in);
	freepool(&pk->p_out);

	FreeStr((char **)&Pk);

	if ( RdPauses )
		Debug((4, "`g' protocol - %lu read pauses", RdPauses));
}

/*
**	Write data to packet(s).
*/

static int
writedata(
	char *		ibuf,
	int		icount
)
{
	register Pkt *	pk = Pk;
	register Buf *	bp;
	register int	cc;
	register char *	cp;
	int		x;
	int		count;
	int		fc;
	int		partial;

	Trace((6, "writedata(%#x, %d)", ibuf, icount));

	if ( (pk->p_state & DOWN) || !(pk->p_state & LIVE) )
		return -1;

	PktTimeout = WRITETIMEOUT;
	PktTimeIncr = WRITETIMOINCR;
	Ntimeout = 0;
	count = icount;

	do
	{
		while ( pk->p_out.count >= pk->p_out.window )
		{
			output();
			recvpkt();
		}

		x = NextSeq[pk->p_oseq];
		bp = &pk->p_obuf[x];

		while ( bp->status != B_NULL )
			recvpkt();

		bp->status = B_MARK;
		pk->p_oseq = x;
		pk->p_out.count++;

		bp->buf = cp = getpool(&pk->p_out.pool, pk->p_out.size);

		if ( icount < pk->p_out.size )
		{
			if ( (cc = pktsize(icount)) == icount )
			{
				partial = 0;
				bp->size = cc;
			}
			else
			{
				fc = cc - icount;
				*cp = fc & 0177;

				if ( fc > 127 )
				{
					*cp++ |= 0200;
					*cp++ = fc >> 7;
				}
				else
					cp++;

				partial = B_SHORT;
				bp->size = cc;
				cc = icount;
			}
		}
		else
		{
			partial = 0;
			bp->size = cc = pk->p_out.size;
		}

		bcopy(ibuf, cp, cc);

		ibuf += cc;
		icount -= cc;

		bp->sum = chksum(bp->buf, bp->size);

		bp->status = B_READY|partial;

		output();
	} while
		( icount );

	return count;
}

/*
**	DEBUGGING.
*/

/*
**	Print packet state in a readable format.
*/

static void
prt_state(
	register int	state
)
{
	register FILE *	fp = DebugFd;

	Fprintf(fp, "\tSTATE\t0%03o\t", state);

	if ( state == DEAD )
	{
		Fprintf(fp, "DEAD\n");
		return;
	}

	if ( state & (INITa|INITb|INITc) )
	{
		Fprintf(fp, "INIT");
		if ( state & INITa )	Fputc('a', fp);
		if ( state & INITb )	Fputc('b', fp);
		if ( state & INITc )	Fputc('c', fp);
		Fputc(' ', fp);
	}

	if ( state & LIVE )	Fprintf(fp, "LIVE ");
	if ( state & RXMIT )	Fprintf(fp, "RXMIT ");
	if ( state & RREJ )	Fprintf(fp, "RREJ ");
	if ( state & FLUSH )	Fprintf(fp, "FLUSH ");
	if ( state & DOWN )	Fprintf(fp, "DOWN ");
	if ( state & RCLOSE )	Fprintf(fp, "RCLOSE ");

	Fputc('\n', fp);
}

/*
**	Print control packets in a readable format.
*/

static void
prt_cntrl(char c)
{
	register FILE *	fp = DebugFd;
	register int	cntrl;
	register int	val;

	val = c & 07;
	cntrl = (c>>3) & 07;

	Fprintf(fp, "0%o\t", c);

	switch ( cntrl )
	{
	case INITA:
		Fprintf(fp, "INITA\t%d sending windows", val);
		break;
	case INITB:
		val++;
		Fprintf(fp, "INITB\t%d byte packet size", PktSizes[val]);
		break;
	case INITC:
		Fprintf(fp, "INITC\t%d receiving windows", val);
		break;
	case RJ:
		Fprintf(fp, "RJ NAK\tpacket %d", val);
		break;
	case RR:
		Fprintf(fp, "RR ACK\tpacket %d", val);
		break;
	case SRJ:
		Fprintf(fp, "SRJ\t%d", val);
		break;
	case CLOSE:
		Fprintf(fp, "CLOSE");
		break;
	}

	Fputc('\n', fp);
}
