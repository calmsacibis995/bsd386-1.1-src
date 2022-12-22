/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Routines to control virtual circuit establishment.
**
**	RCSID $Id: Connect.c,v 1.3 1994/01/31 01:26:36 donn Exp $
**
**	$Log: Connect.c,v $
 * Revision 1.3  1994/01/31  01:26:36  donn
 * Latest version from Paul Vixie.
 *
 * Revision 1.2  1994/01/29  20:58:30  vixie
 * 1.1
 *
 * Revision 1.1  1994/01/28  06:42:44  vixie
 * Initial revision
 *
 * Revision 1.2  1993/02/28  15:36:28  pace
 * Add recent uunet changes; add P_HWFLOW_ON, etc; add hayesv dialer
 *
 * Revision 1.1.1.1  1992/09/28  20:08:54  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	CTYPE
#define	ERRNO
#define	FILES
#define	FILE_CONTROL
#define	SETJMP
#define	SIGNALS
#define	STDIO
#define	SYSEXITS
#define	TERMIOCTL
#define	TIME

#include	"global.h"
#include	"cico.h"
#include	"devices.h"

/*
**	Line control during login procedure.
*/

typedef enum { P_ZERO, P_ONE, P_EVEN, P_ODD } Parity;

/*
**	Miscellaneous.
*/

#define	ABORT		CF_DIAL
#define	MAXREADCHARS	128			/* Even number! */
#define isoctal(x)	((x >= '0') && (x <= '7'))

CallType	(*CU_end)(int)	= nullcls;
char 		LineType[11];
int		NextFd		= SYSERROR;	/* Predicted fd to close interrupted opens */

static char *	AbortOn;
static int	Dcf		= SYSERROR;
static char	ParTab[128];			/* Must be power of two */
static jmp_buf	WriteJmp;

static void	BuildParTab(Parity);
static CallType	login(int, char **, int);
static bool	notin(char *, char *);
static CallType	OpenVC(char **);
static void	SendLine(char *, int);
static bool	wprefix(char *, char *);
static void	WriteVC(int, char);

/*
**	Generate parity table for use by WriteVC().
*/

static void
BuildParTab(
	register Parity	type
)
{
	register int	n;
	register int	j;
	register int	i;

	for ( i = 0 ; i < sizeof(ParTab) ; i++ )
	{
		n = 0;
		for ( j = i&(sizeof(ParTab)-1) ; j ; j = (j-1)&j )
			n++;

		ParTab[i] = i;

		if
		(
			type == P_ONE
			||
			(type == P_EVEN && (n&01) != 0)
			||
			(type == P_ODD && (n&01) == 0)
		)
			ParTab[i] |= sizeof(ParTab);
	}
}

/*
**	Place a call to system and login, etc.
**
**	return codes:
**		CF_SYSTEM:	don't know system
**		CF_TIME:	wrong time to call
**		CF_DIAL:	call failed
**		CF_NODEV:	no devices available to place call
**		CF_LOGIN:	login/password dialog failed
**
**		>=0  - file no.  -  connect ok
*/

int
Connect()
{
	register char *		cp;
	register char *		dp;
	int			nf;
	register CallType	fcode = CF_SYSTEM;
	char *			fields[MAXFIELDS];

	Trace((1, "Connect()"));

	if ( !FindSysEntry(&RemoteNode, NEEDSYS, NOALIAS) )
		return (int)fcode;

	/** NB: `SysDetails' start *after* SYS_NAME **/

	for ( dp = SysDetails ; dp != NULLSTR ; dp = cp )
	{
		if ( (cp = strchr(dp, '\n')) != NULLSTR )
			*cp++ = '\0';

		if ( (nf = SplitSpace(fields, dp, MAXFIELDS)) < SYS_LINE )
		{
			Trace((3, "%s: entry with no time/line field", RemoteNode));
			continue;
		}

		dp = fields[SYS_LINE-1];

		if ( strcmp("SLAVE", dp) == STREQUAL )
			continue;

		(void)strncpy(LineType, dp, (sizeof LineType)-1);

		if ( LocalOnly )
		{
			if
			(
				strcmp("TCP", dp) != STREQUAL
				&&
				strcmp("DIR", dp) != STREQUAL
				&&
				strcmp("LOCAL", dp) != STREQUAL
			)
			{
				Debug((1, "Non-local: %s", dp));
				fcode = CF_TIME;
				continue;
			}
		}

		if
		(
			!IgnoreTimeToCall
			&&
			(fcode = CheckFieldTime(fields[SYS_TIME-1])) == CF_TIME
		)
			continue;

		if ( CheckForWork && !find_work() )
		{
			Debug((1, "No work for %s, so no call", RemoteNode));
			fcode = CF_TIME;
			break;
		}

		if ( (int)(fcode = OpenVC(fields)) >= 0 )
		{
#if	0
			if ( SysDebug > Debugflag && Debugflag == 0 )
				Debugflag = SysDebug;	/* XXX Must setaudit() */
#endif	/* 0 */
			Dcf = (int)fcode;
			break;
		}

keeplooking:	;
	}

	if ( dp == NULLSTR )
		return (int)fcode;

	if ( (int)fcode >= 0 )
		fcode = login(nf, fields, Dcf);

	if ( (int)fcode < 0 )
	{
		CloseACU();

		if ( fcode == ABORT )
		{
			fcode = CF_DIAL;
			goto keeplooking;
		}
		else
			return (int)fcode;
	}

	fioclex(Dcf);

	return Dcf;
}

/*
**	Close call unit.
*/

void
CloseACU()
{
	if ( Dcf == SYSERROR )
		return;

	/*
	**	Make *sure* Dcf is no longer exclusive.
	**	Otherwise dual call-in/call-out modems could get stuck.
	**	Unfortunately, doing this here is not ideal, but it is the
	**	easiest place to put the call.
	**	Hopefully everyone honors the LCK protocol, of course.
	*/

#ifdef	TIOCNXCL
	if ( Is_a_tty && ioctl(Dcf, TIOCNXCL, (void *)0) == SYSERROR )
		Debug((5, "CloseACU ioctl %s", ErrnoStr(errno)));
#endif

	if ( setjmp(AlarmJmp) )
		Log("CLOSE TIMEOUT %s", RemoteNode);
	else
	{
		(void)signal(SIGALRM, Timeout);
		(void)alarm(20);
		(void)(*(CU_end))(Dcf);
		(void)alarm(0);
	}

	if ( close_dev(Dcf) != SYSERROR )
	{
		Debug((4, "fd %d NOT CLOSED by CU_clos", Dcf));
		Log("fd NOT CLOSED by CU_clos");
	}

	Dcf = SYSERROR;
	CU_end = nullcls;
}

/*
**	Do chat script.
**	Occurs after local port is opened,
**	before 'dialing' the other machine.
*/

CallType
dochat(
	register Device *	dev,
	char *			flds[],
	int			fd
)
{
	register int		i;
	register char *		p;
	CallType		ct;
	char			bfr[MAXDEVCHARS];

	if ( dev->D_nargs <= D_CHAT )
		return SUCCESS;

	Debug((4, "dochat called %d", dev->D_nargs));

	for ( i = 0 ; i < dev->D_nargs-D_CHAT ; i++ )
	{
		(void)sprintf(bfr, dev->D_arg[D_CHAT+i], flds[F_PHONE]);

		if ( strcmp(bfr, dev->D_arg[D_CHAT+i]) != STREQUAL )
			dev->D_arg[D_CHAT+i] = newstr(bfr);
			/** XXX Where is this free'd? **/
	}

	if ( (ct = login(dev->D_nargs-(D_CHAT-F_LOGIN), &dev->D_arg[D_CHAT-F_LOGIN], fd)) == CF_LOGIN )
		ct = CF_CHAT;

	/*
	**	If login() last did a SendLine(), must pause so things can settle.
	**	But don't bother if chat failed.
	*/

	if ( ct == SUCCESS && ((dev->D_nargs-(D_CHAT-F_LOGIN))&01) == 0 )
		sleep(2);

	return ct;
}

/*
**	Look for expected string.
*/

CallType
expect(
	char *		strIn,
	int		fn
)
{
	register char *	rp;
	register char *	cp;
	register int	c;
	int		kr;
	char		nextch;
	int		timo;
	char		rdvec[MAXREADCHARS];
	static char *	str = NULL;
	static int	strLen = 0;

	if ( *strIn == '\0' || strcmp(strIn, "\"\"") == STREQUAL )
		return SUCCESS;

	/* all this because of unwritable strings */
	kr = strlen(strIn);
	if (kr > strLen) {
		if (str) {
			free(str);
		}
		str = malloc(kr + 1);
		strLen = kr;
	}
	strcpy(str, strIn);

	if ( Debugflag>=4 && !isdigit(str[0]) && str[0]!='\n' )
		Debug((4, "wanted %s", ExpandString(str, -1)));

	/** Cleanup str, convert \0xx strings to one char **/

	for ( cp = str ; c = *cp++ ; )
	{
		if ( c != '\\' )
			continue;

		switch ( *cp )
		{
		case '\0':
			Debug((6, "TRAILING BACKSLASH IGNORED"));
			break;

		case 's':
			Debug((6, "BLANK"));
			cp[-1] = ' ';
			(void)strcpy(cp, &cp[1]);
			break;

		default:
			cp[-1] = otol(cp);
			Debug((6, "BACKSLASHED %#x", cp[-1]));
			(void)strcpy(cp, &cp[3]);
		}
	}

	if ( (cp = strchr(str, '~')) != NULLSTR )
	{
		*cp++ = '\0';

		if ( (timo = atoi(cp)) <= 0 )
			timo = MAXMSGTIME;
	}
	else
		timo = MAXMSGTIME;

	if ( setjmp(AlarmJmp) )
	{
		Debug((5, EmptyStr));
		Debug((4, "TIMEOUT"));
		return FAIL;
	}

	(void)signal(SIGALRM, Timeout);
	(void)alarm(timo);

	rp = rdvec;
	*rp = '\0';

	while ( notin(str, rdvec) )
	{
		if ( AbortOn != NULLSTR && !notin(AbortOn, rdvec) )
		{
			alarm(0);
			Debug((5, EmptyStr));
			Debug((1, "Call aborted on '%s'", AbortOn));
			return ABORT;
		}

		if ( (kr = read(fn, &nextch, 1)) <= 0 )
		{
			(void)alarm(0);
			Log("LOGIN LOST LINE");
			Debug((5, EmptyStr));
			Debug((4, "lost line kr - %d", kr));
			return FAIL;
		}

		if ( (c = nextch & 0177) == '\0' )
		{
			Debug((5, "\000\\c"));
			continue;
		}

		DODEBUG(
			if ( Debugflag >= 5 )
			{
				nextch = c;
				Debug((5, "%s\\c", ExpandString(&nextch, 1)));
			}
		);

		*rp++ = c;

		if ( rp >= &rdvec[MAXREADCHARS] )
		{
			(void)strncpy(rdvec, &rdvec[MAXREADCHARS/2], MAXREADCHARS/2);

			rp = &rdvec[MAXREADCHARS/2];
		}

		*rp = '\0';
	}

	(void)alarm(0);

	Debug((5, EmptyStr));
	Debug((4, "got that"));
	return SUCCESS;
}

/*
**	Expand phone number for given prefix and number.
*/

void
ExpandTelno(
	register char *	in,
	register char *	out
)
{
	register char *	cp;
	char *		data;
	char *		bufp;
	char		pre[MAXPH], npart[MAXPH], tpre[MAXPH], p[MAXPH];

	if ( !isascii(*in) || !isalpha(*in) )
	{
		strcpy(out, in);
		return;
	}

	cp = pre;
	while ( isascii(*in) && isalpha(*in) )
		*cp++ = *in++;
	*cp = '\0';
	cp = npart;
	while ( *in != '\0' )
		*cp++ = *in++;
	*cp = '\0';

	tpre[0] = '\0';
	if ( (cp = ReadFile(DIALFILE)) == NULLSTR )
		Debug((2, "CAN'T OPEN %s", DIALFILE));
	else
	{
		bufp = data = cp;

		while ( (cp = GetLine(&bufp)) != NULLSTR )
		{
			if ( sscanf(cp, "%s%s", p, tpre) != 2 )
				continue;
			if ( strcmp(p, pre) == STREQUAL )
				goto found;
			tpre[0] = '\0';
		}
		Debug((2, "CAN'T FIND dialcodes prefix '%s'", pre));
found:
		free(data);
	}

	cp = strcpyend(out, tpre);
	(void)strcpy(cp, npart);
}

/*
**	Arrange to close fd on exec(II).
**	Otherwise unwanted file descriptors are inherited
**	by other programs.  And that may be a security hole.
*/

void
fioclex(
	int	fd
)
{
#ifdef	F_SETFD
	if ( fcntl(fd, F_SETFD, 1) == SYSERROR )
		Debug((2, CouldNot, "F_SETFD", ErrnoStr(errno)));
#endif	/* F_SETFD */
}

/*
**	Determine next file descriptor that would be allocated.
**	This permits later closing of a file whose open was interrupted.
**	It is a UNIX kernel problem, but it has to be handled.
**	unc!smb (Steve Bellovin) probably first discovered it.
*/

void
getnextfd()
{
	(void)close(NextFd = open(DevNull, 0));
}

/*
**	Login conversation.
*/

static CallType
login(
	int		nf,
	register char *	flds[],
	int		fn
)
{
	register char *	altern;
	register char *	want;
	register int	k;
	CallType	ok;

	if ( nf < 4 )
	{
		ErrVal = EX_DATAERR;
		Error("Too few SYSFILE fields for %s: %d", RemoteNode, nf);
	}

	Trace((1, "login(%d, {%s,%s,%s,%s}, %d)", nf, flds[0], flds[1], flds[2], flds[3], fn));

	name_prog("login");

	if ( setjmp(WriteJmp) )
		return FAIL;

	AbortOn = NULLSTR;

	for ( k = F_LOGIN ; k < nf ; k += 2 )
	{
		Trace((2, "chat pair: %s %s", flds[k], ((k+1)<nf)?flds[k+1]:EmptyStr));

		if ( (want = flds[k]) == NULLSTR )
			want = EmptyStr;

		ok = FAIL;

		while ( ok != SUCCESS )
		{
			if ( (altern = strchr(want, '-')) != NULLSTR )
				*altern++ = '\0';

			if ( strcmp(want, "ABORT") == STREQUAL )
			{
				AbortOn = ((k+1)<nf)?flds[k+1]:EmptyStr;
				Debug((4, "ABORT ON: %s", AbortOn));
				goto nextfield;
			}

			ok = expect(want, fn);

			if ( ok == FAIL )
			{
				if ( altern == NULLSTR )
				{
					Log("%s LOGIN", FAILED);
					return (int)CF_LOGIN;
				}

				if ( (want = strchr(altern, '-')) != NULLSTR )
					*want++ = '\0';

				SendLine(altern, fn);
			}
			else
			if ( ok == ABORT )
			{
				Log("%s LOGIN ABORTED on \"%s\"", FAILED, AbortOn);
				return ABORT;
			}
		}

		(void)sleep(1);

		if ( (k+1) < nf )
			SendLine(flds[k+1], fn);
nextfield: ;
	}

	return SUCCESS;
}

/*
**	Check for no occurrence of "substr" in "str".
*/

static bool
notin(
	register char *	substr,
	register char *	str
)
{
	while ( *str != '\0' )
	{
		if ( wprefix(substr, str) )
			return false;
		else
			str++;
	}

	return true;
}

/*
**	Connect to remote machine.
*/

static CallType
OpenVC(
	register char *		flds[]
)
{
	register ConDev *	cd;
	char *			line;

	Debug((4, "call no. %s for sys %s", flds[F_PHONE], RemoteNode));

	if ( strccmp(flds[F_LINE], "LOCAL") == STREQUAL )
		line = "ACU";
	else
		line = flds[F_LINE];

	if ( strccmp(line, "ACU") != STREQUAL )
		Reenable();

	CU_end = nullcls;

	for ( cd = condevs ; cd->CU_meth != NULLSTR ; cd++ )
	{
		if ( strccmp(cd->CU_meth, line) == STREQUAL )
		{
			Debug((4, "Using %s to call", cd->CU_meth));
			return (*(cd->CU_gen))(flds);
		}
	}

	Debug((1, "Can't find %s, assuming DIR", flds[F_LINE]));
	return diropn(flds);	/* search failed, so use direct */
}

/*
**	Send line of login sequence.
*/

static struct ParityCtl {
	char *token;
	Parity parity;
} ParityCtl[] = {
	{ "P_ONE",  P_ONE },
	{ "P_ZERO", P_ZERO },
	{ "P_EVEN", P_EVEN },
	{ "P_ODD",  P_ODD },
	{ NULL }
};

static struct TermioCtl {
	char *prefix;
	int flag;
} TermioCtl[] = {
	{ "P_HWFLOW", UU_HWFLOW },
	{ "P_MDMBUF", UU_MDMBUF },
	{ "P_RTSCTS", UU_RTSCTS },
	{ "P_CLOCAL", UU_CLOCAL },
	{ NULL }
};

static void
SendLine(
	register char *	str,
	int		fn
)
{
	register char *	cp;
	register char	c;
	int		i;
	int		n;
	int		cr;
	struct TermioCtl *tc;
	struct ParityCtl *pc;

	static int	p_init;

	if ( !p_init )
	{
		p_init++;
		BuildParTab(P_EVEN);
		DoTermios(fn, UU_HWFLOW|UU_MDMBUF|UU_RTSCTS, 0);
	}

	Debug((5, "send \"%s\"", str));

	if ( strncmp("BREAK", str, 5) == STREQUAL )
	{
		(void)sscanf(&str[5], "%1d", &i);
		if ( i <= 0 || i > 10 )
			i = 3;
		SendBRK(fn, i);
		return;
	}

	if ( strncmp("PAUSE", str, 5) == STREQUAL )
	{
		(void)sscanf(&str[5], "%1d", &i);
		if ( i <= 0 || i > 10 )
			i = 3;
		sleep((unsigned)i);
		return;
	}

	if ( strcmp(str, "EOT") == STREQUAL )
	{
		WriteVC(fn, '\04');
		return;
	}

	if ( strcmp(str, "LF") == STREQUAL )
	{
		WriteVC(fn, '\n');
		return;
	}

	if ( strcmp(str, "CR") == STREQUAL )
	{
		WriteVC(fn, '\r');
		return;
	}

	for (pc = ParityCtl;  pc->token != NULL;  pc++) {
		if (strcmp(str, pc->token) != STREQUAL)
			continue;
		BuildParTab(pc->parity);
		return;
	}

	for (tc = TermioCtl;  tc->prefix != NULL;  tc++) {
		register int len = strlen(tc->prefix);
		register int onoff;

		if (strncmp(str, tc->prefix, len) != STREQUAL)
			continue;
		if (strcmp(str+len, "_ON") == STREQUAL)
			onoff = 1;
		else if (strcmp(str+len, "_OFF") == STREQUAL)
			onoff = 0;
		else
			continue;
		DoTermios(fn, tc->flag, onoff);
		return;
	}

	if ( strcmp(str, "\"\"") == STREQUAL )
	{
		WriteVC(fn, '\r');	/* If "", just send '\r' */
		return;
	}

	cr = 1;
	cp = str;

	while ( (c = *cp++) != '\0' )
	{
		if ( c == '\\' )
		{
			switch ( *cp++ )
			{
			case '\0':
				Debug((5, "TRAILING BACKSLASH IGNORED"));
				--cp;
				continue;
			case 's':
				Debug((5, "BLANK"));
				c = ' ';
				break;
			case 'd':
				Debug((5, "DELAY"));
				sleep(1);
				continue;
			case 'n':
				Debug((5, "NEW LINE"));
				c = '\n';
				break;
			case 'r':
				Debug((5, "RETURN"));
				c = '\r';
				break;
			case 'b':
				if ( isdigit(*cp) )
				{
					i = (*cp++ - '0');
					if ( i <= 0 || i > 10 )
						i = 3;
				}
				else
					i = 3;
				SendBRK(fn, i);
				if ( *cp == '\0' )
					cr = 0;
				continue;
			case 'c':
				if ( *cp == '\0' )
				{
					Debug((5, "NO CR"));
					cr = 0;
				} 
				else
					Debug((5, "NO CR - IGNORED NOT EOL"));
				continue;
			default:
				if ( isoctal(cp[-1]) )
				{
					i = 0;
					n = 0;
					--cp;
					while ( isoctal(*cp) && ++n <= 3 )
						i = i * 8 + (*cp++ - '0');
					Debug((5, "\\%o", i));
					WriteVC(fn, (char)i);
					continue;
				}
			}
		}

		WriteVC(fn, c);
	}

	if ( cr )
		WriteVC(fn, '\r');

	Debug((4, "sent \"%s%s\"", str, cr?"\\r":EmptyStr));
}

/*
**	Catch alarm routine for `expect()' && `CloseACU()'.
*/

void
Timeout(int sig)
{
	(void)signal(sig, Timeout);

	if ( NextFd != SYSERROR )
	{
		if ( close(NextFd) == SYSERROR )
			Log("ACU LINE CLOSE FAIL");

		NextFd = SYSERROR;
	}

	longjmp(AlarmJmp, 1);
}

/*
**      Check s2 for prefix s1 with a wildcard character ?.
*/

static bool
wprefix(
	register char *	s1,
	register char *	s2
)
{
        register char	c;

        while ( (c = *s1++) != '\0' )
                if ( *s2 == '\0' || (c != *s2++ && c != '?' ) )
                        return false;

        return true;
}

/*
**	Write to remote.
*/

static void
WriteVC(
	int	fd,
	char	c
)
{
	c = ParTab[c&0177];

	if ( write(fd, (char *)&c, 1) != 1 )
	{
		char *		ttyn;
		extern char *	ttyname(int);

		if ( (ttyn = ttyname(fd)) != NULLSTR )
		{
			if ( strncmp(ttyn, DevNull, 5) == STREQUAL )
				ttyn += 5;
		}
		else
			ttyn = EmptyStr;

		Log("BAD WRITE %s: %s", ttyn, ErrnoStr(errno));

		longjmp(WriteJmp, 2);
	}
}
