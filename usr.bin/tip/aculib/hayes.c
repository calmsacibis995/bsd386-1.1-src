/*	BSDI $Id: hayes.c,v 1.4 1994/01/28 01:58:53 sanders Exp $	*/

/*
 * Copyright (c) 1983 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
static char sccsid[] = "@(#)hayes.c	5.4 (Berkeley) 3/2/91";
#endif /* not lint */

/*
 * Routines for dialing a Hayes Modem (based on the old VenTel driver).
 * The switches enabling the DTR and CD lines must be set correctly.
 * DTR should reset the modem and CD should follow modem carrier,
 * typically set with &C1 &D3.
 * 
 * NOTICE:
 * The easy way to hang up a modem is always simply to clear the DTR signal.
 * However, if the +++ sequence (which switches the modem back to local mode)
 * is sent before modem is hung up, removal of the DTR signal has no effect
 * (except that it prevents the modem from recognizing commands).
 * 
 * (by Helge Skrivervik, Calma Company, Sunnyvale, CA. 1984) 
 * Later modified by BSDI.
 */

#include "tip.h"
#include <vis.h>
#include <ctype.h>
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#define HAYES_SYNC_RETRY	5
#define HAYES_BUFSIZ		1024

/* misc state */
static	int verbose;
static	int dial_timeout;
static	volital int timedout = 0;
static	jmp_buf timeoutbuf;
static	char iobuf[HAYES_BUFSIZ+1];

/* exported routines */
extern	int hay_dialer();
extern	int hay_disconnect();
extern	int hay_abort();
extern	int hay_sync();

/* support routines */
static 	void status __P((const char *fmt, ...));
static 	int sendcmd();
static 	char *gobble();
static 	char *lazycmd();
static	char *showstr();
static	void goodbye();
static	void sigALRM();

/* dialer state */
#define	DIALING		1
#define IDLE		2
#define CONNECTED	3
#define	FAILED		4
static	int state = IDLE;

#define	min(a,b)	((a < b) ? a : b)

hay_dialer(phonenum, acu)
	char *phonenum;
	char *acu;
{
	register char *cp;
	verbose = boolean(value(VERBOSE));
	dial_timeout = number(value(DIALTIMEOUT));

	/* make sure we can talk to the modem */
	if (hay_sync() == 0)
		return(state == CONNECTED);

	set_hupcl(FD);

	status("Initialize modem (ATV1)...");
	if (sendcmd("ATV1") < 0)
		return(state == CONNECTED);
	if (strncmp(gobble(), "OK", 2) != NULL) {
		status("Init Failed");
		return(state == CONNECTED);
	}

	status("Dialing...");
	state = DIALING;
	sprintf(iobuf, "ATDT%s", phonenum);
	if (sendcmd(iobuf) < 0)
		return(state == CONNECTED);
	do {
        	cp = gobble();
		if (strncmp(cp, "CONNECT", 7) == NULL) {
			state = CONNECTED;
			sleep(1);
		} else if (strcmp(cp, "NO CARRIER") == NULL) {
			state = FAILED;
		} else if (strcmp(cp, "ERROR") == NULL) {
			state = FAILED;
		} else if (strcmp(cp, "NO DIALTONE") == NULL) {
			state = FAILED;
		} else if (strcmp(cp, "NO ANSWER") == NULL) {
			state = FAILED;
		} else if (strcmp(cp, "BUSY") == NULL) {
			state = FAILED;
		}
		status("Modem status: %s\n", cp);
	} while (state == DIALING);

	tcflush(FD, TCIOFLUSH);
	if (timedout) {
		state = FAILED;
#ifdef ACULOG
		sprintf(iobuf, "%d second dial timeout", dial_timeout);
		logent(value(HOST), phonenum, "hayes", iobuf);
#endif
		hay_disconnect();	/* insurance */
	}
	return (state == CONNECTED);
}

hay_abort()
{
	status("\n\rAborting call");
	tcflush(FD, TCIOFLUSH);
	write(FD, "\r", 1);	/* send anything to abort the call */
	hay_disconnect();
}

hay_disconnect()
{
	/* first hang up the modem */
	status("\n\rDisconnecting modem...");
	ioctl(FD, TIOCCDTR, 0);
	sleep(1);
	ioctl(FD, TIOCSDTR, 0);
	goodbye();
}

hay_sync()
{
	int len, retry = 0;
	char *cp;

	status("Synchronizing with modem (ATE1)...");
	while (retry++ <= HAYES_SYNC_RETRY) {
#ifdef DEBUG
		status("SYNC: sending ``ATE1''");
#endif
		cp = lazycmd("ATE1");
		if (index(cp, '0') || 
		    (index(cp, 'O') && index(cp, 'K'))) {
			status("Modem synchronized");
			return(1);
		}
		status("Trying sync again (%d)", retry);
		if (retry > 2) {
			ioctl(FD, TIOCCDTR, 0);
			sleep(1);
			ioctl(FD, TIOCSDTR, 0);
			sleep(2);
		}
	}
	status("Cannot synchronize with modem...");
	return(0);
}

/*
 * Only called from inside gobble
 */
static void
sigALRM()
{
	status("\07timedout waiting for reply");
	timedout = 1;
	longjmp(timeoutbuf, 1);
}

static char *
showstr(str, len)
	const char *str;
	int len;
{
	static char vbuf[HAYES_BUFSIZ+1];
	strvisx(vbuf, str, min(len, HAYES_BUFSIZ/4), VIS_WHITE|VIS_CSTYLE);
	return(vbuf);
}

/*
 * reads FD and returns a pointer to the string
 */
static char *
gobble(void) {
	char c;
	volatile sig_t f;
	static	char buf[HAYES_BUFSIZ+1];		/* return buffer */
	char *cp = buf;
	int gotone = 0;				/* got a non-whietspace */
#ifdef DEBUG
	char vbuf[4];
#endif

#ifdef DEBUG
	printf("gobble: "); fflush(stdout);
#endif
	bzero(buf, HAYES_BUFSIZ);

	f = signal(SIGALRM, sigALRM);
	timedout = 0;
	if (setjmp(timeoutbuf)) {
		signal(SIGALRM, f);
		return (0);
	}

	alarm(dial_timeout);
	for(;;) {
		read(FD, &c, 1);
		c &= 0177;
#ifdef DEBUG
		vis(vbuf, c, VIS_WHITE|VIS_CSTYLE, '\0');
		printf("%s", vbuf); fflush(stdout);
#endif
		if (!gotone && isspace(c)) continue;
		if (c == '\r' || c == '\n') break;
		*cp++ = c;
		gotone = 1;
	}
	alarm(0);
	*cp = '\0';
	signal(SIGALRM, f);
#ifdef DEBUG
	printf(" [%s]\n\r", buf); fflush(stdout);
#endif
	return (buf);
}

static char *
lazycmd(cmd)
	char *cmd;
{
	static char buf[HAYES_BUFSIZ+1];
	int len;

	tcflush(FD, TCIOFLUSH);
	write(FD, cmd, strlen(cmd));
	write(FD, "\r", 1);
	sleep(2);
	ioctl(FD, FIONREAD, &len);
	if (len) {
		len = read(FD, buf, min(len, HAYES_BUFSIZ/4));
#ifdef DEBUG
		status("lazy: got ``%s''", showstr(buf, len));
#endif
	}
	buf[len] = '\0';
	return(buf);
}

/*
 * send a command
 */
static int
sendcmd(cmd)
	char *cmd;
{
#ifdef DEBUG
	status("Sending ``%s''", cmd);
#endif
	write(FD, cmd, strlen(cmd));
	write(FD, "\r", 1);
	if (strncmp(gobble(), cmd, strlen(cmd)) != NULL) {
		status("Command Failed!!!");
		return(-1);
	}
	return(0);
}

/*
 * reset modem
 */
static void
goodbye()
{
	int len, rlen;
	char *cp;

	tcflush(FD, TCIOFLUSH);			/* get rid of trash */
	if (hay_sync()) {
		sleep(1);
		tcflush(FD, TCIFLUSH);
		/* insurance */
		status("Hanging up (ATH0)...");
		cp = lazycmd("ATH0");
		if (strstr(cp, "OK") == NULL) {
			status("Cannot hang up modem, please reconnect");
			status("and make sure the line is hung up.");
		}
		sleep(1);
#ifdef DEBUG
		ioctl(FD, FIONREAD, &len);
		printf("goodbye1: len=%d -- ", len);
		rlen = read(FD, iobuf, min(len, HAYES_BUFSIZ));
		iobuf[rlen] = '\0';
		printf("read (%d): %s\r\n", rlen, iobuf);
#endif
		tcflush(FD, TCIFLUSH);
		write(FD, "ATZ\r", 4);
		sleep(1);
#ifdef DEBUG
		ioctl(FD, FIONREAD, &len);
		printf("goodbye2: len=%d -- ", len);
		rlen = read(FD, iobuf, min(len, HAYES_BUFSIZ));
		iobuf[rlen] = '\0';
		printf("read (%d): %s\r\n", rlen, iobuf);
#endif
	}
	tcflush(FD, TCIOFLUSH);		/* clear the input buffer */
	ioctl(FD, TIOCCDTR, 0);		/* clear DTR (insurance) */
	close(FD);
}

static void
#ifdef __STDC__
status(const char *fmt, ...)
#else
status(fmt, va_alist)
	const char *fmt;
	va_dcl
#endif
{
	va_list ap;
	if (verbose) {
#if __STDC__
		va_start(ap, fmt);
#else
		va_start(ap);
#endif
		vfprintf(stdout, fmt, ap);
		fprintf(stdout, "\n\r");
		fflush(stdout);
		va_end(ap);
	}
}
