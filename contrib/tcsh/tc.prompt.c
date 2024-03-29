/* $Header: /bsdi/MASTER/BSDI_OS/contrib/tcsh/tc.prompt.c,v 1.2 1994/01/13 23:35:20 polk Exp $ */
/*
 * tc.prompt.c: Prompt printing stuff
 */
/*-
 * Copyright (c) 1980, 1991 The Regents of the University of California.
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
#include "sh.h"

RCSID("$Id: tc.prompt.c,v 1.2 1994/01/13 23:35:20 polk Exp $")

#include "ed.h"

/*
 * kfk 21oct1983 -- add @ (time) and / ($cwd) in prompt.
 * PWP 4/27/87 -- rearange for tcsh.
 * mrdch@com.tau.edu.il 6/26/89 - added ~, T and .# - rearanged to switch()
 *                 instead of if/elseif
 * Luke Mewburn, s902113@minyos.xx.rmit.OZ.AU 6-Sep-91 - changed date format
 */

static char   *month_list[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
			"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
static char   *day_list[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
void
printprompt(promptno, str)
    int     promptno;
    char   *str;
{
    static  Char *ocp = NULL;
    static  char *ostr = NULL;
    time_t  lclock = time(NULL);
    Char   *cp;

    switch (promptno) {
    default:
    case 0:
	cp = value(STRprompt);
	break;
    case 1:
	cp = value(STRprompt2);
	break;
    case 2:
	cp = value(STRprompt3);
	break;
    case 3:
	if (ocp != NULL) {
	    cp = ocp;
	    str = ostr;
	}
	else 
	    cp = value(STRprompt);
	break;
    }

    if (promptno < 2) {
	ocp = cp;
	ostr = str;
    }

    PromptBuf[0] = '\0';
    tprintf(FMT_PROMPT, PromptBuf, cp, 2 * INBUFSIZE - 2, str, lclock, NULL);

    if (!editing) {
	for (cp = PromptBuf; *cp ; )
	    (void) putraw(*cp++);
	SetAttributes(0);
	flush();
    }
}

void
tprintf(what, buf, fmt, siz, str, tim, info)
    int what;
    Char *buf, *fmt;
    size_t siz;
    char *str;
    time_t tim;
    ptr_t info;
{
    Char   *z, *q;
    Char    attributes = 0;
    static int print_prompt_did_ding = 0;
    char   *cz;
    Char    buff[BUFSIZE];
    char    cbuff[BUFSIZE];

    Char *p  = buf;
    Char *ep = &p[siz];
    Char *cp = fmt;
    struct tm *t = localtime(&tim);

    for (; *cp; cp++) {
	if (p >= ep)
	    break;
	if (*cp == '%') {
	    cp++;
	    switch (*cp) {
	    case 'R':
		if (what == FMT_HISTORY)
		    fmthist('R', info, str = cbuff);
		if (str != NULL)
		    while (*str) {
			*p++ = attributes | *str++;
			if (p >= ep) break;
		    }
		break;
	    case '#':
		*p++ = attributes | ((uid == 0) ? '#' : '>');
		break;
	    case '!':
	    case 'h':
		if (what == FMT_HISTORY)  
		    fmthist('h', info, cbuff);
		else 
		    (void) xsprintf(cbuff, "%d", eventno + 1);
		for (cz = cbuff; *cz; cz++) {
		    *p++ = attributes | *cz;
		    if (p >= ep) break;
		}
		break;
	    case 'T':		/* 24 hour format	 */
	    case '@':
	    case 't':		/* 12 hour am/pm format */
	    case 'p':		/* With seconds	*/
	    case 'P':
		{
		    char    ampm = 'a';
		    int     hr = t->tm_hour;

		    if (p >= ep - 10) break;

		    /* addition by Hans J. Albertsson */
		    /* and another adapted from Justin Bur */
		    if (adrof(STRampm) || (*cp != 'T' && *cp != 'P')) {
			if (hr >= 12) {
			    if (hr > 12)
				hr -= 12;
			    ampm = 'p';
			}
			else if (hr == 0)
			    hr = 12;
		    }		/* else do a 24 hour clock */

		    /* "DING!" stuff by Hans also */
		    if (t->tm_min || print_prompt_did_ding || 
			what != FMT_PROMPT) {
			if (t->tm_min)
			    print_prompt_did_ding = 0;
			Itoa(hr, buff);
			*p++ = attributes | buff[0];
			if (buff[1]) 
			    *p++ = attributes | buff[1];
			*p++ = attributes | ':';
			Itoa(t->tm_min, buff);
			if (buff[1]) {
			    *p++ = attributes | buff[0];
			    *p++ = attributes | buff[1];
			}
			else {
			    *p++ = attributes | '0';
			    *p++ = attributes | buff[0];
			}
			if (*cp == 'p' || *cp == 'P') {
			    *p++ = attributes | ':';
			    Itoa(t->tm_sec, buff);
			    if (buff[1]) {
				*p++ = attributes | buff[0];
				*p++ = attributes | buff[1];
			    }
			    else {
				*p++ = attributes | '0';
				*p++ = attributes | buff[0];
			    }
			}
			if (adrof(STRampm) || (*cp != 'T' && *cp != 'P')) {
			    *p++ = attributes | ampm;
			    *p++ = attributes | 'm';
			}
		    }
		    else {	/* we need to ding */
			int     i = 0;

			(void) Strcpy(buff, STRDING);
			while (buff[i]) {
			    *p++ = attributes | buff[i++];
			}
			print_prompt_did_ding = 1;
		    }
		}
		break;

	    case 'M':
#ifndef HAVENOUTMP
		if (what == FMT_WHO)
		    cz = who_info(info, 'M', cbuff);
		else 
#endif /* HAVENOUTMP */
		    cz = getenv("HOST");
		/*
		 * Bug pointed out by Laurent Dami <dami@cui.unige.ch>: don't
		 * derefrence that NULL (if HOST is not set)...
		 */
		if (cz != NULL)
		    while (*cz) {
			if (p >= ep) break;
			*p++ = attributes | *cz++;
		    }
		break;

	    case 'm':
#ifndef HAVENOUTMP
		if (what == FMT_WHO)
		    cz = who_info(info, 'm', cbuff);
		else 
#endif /* HAVENOUTMP */
		    cz = getenv("HOST");

		if (cz != NULL)
		    while (*cz && (what == FMT_WHO || *cz != '.')) {
			if (p >= ep) break;
			*p++ = attributes | *cz++;
		    }
		break;

	    case '~':		/* show ~ whenever possible - a la dirs */
		{
		    static Char *olddir = 0, *olduser = 0, *oldpath = 0;
		    extern int tlength;	/* cache cleared */

		    if ((z = value(STRcwd)) == STRNULL)
			break;	/* no cwd, so don't do anything */
		    /*
		     * Have we changed directory?
		     */
		    if (tlength == 0 || olddir != z) {
			oldpath = olddir = z;
			olduser = getusername(&oldpath);
		    }
		    if (olduser) {
			*p++ = attributes | '~';
			if (p >= ep) break;
			for (q = olduser; *q; *p++ = attributes | *q++)
			    if (p >= ep) break;
			for (z = oldpath; *z; *p++ = attributes | *z++)
			    if (p >= ep) break;
			break;
		    }
		}
		/*FALLTHROUGH*/
	    case '/':
		if ((z = value(STRcwd)) != STRNULL) {
		    while (*z) {
			*p++ = attributes | *z++;
			if (p >= ep) break;
		    }
		}
		break;
	    case '.':
	    case 'c':
	    case 'C':
		{
		    register int j, k;
		    Char    scp;

		    /* modified RWM 8/6/92 - indicate dirs ignored */
		    register int updirs;
		    int pdirs = 0;
		    
		    scp = *cp;
		    /* option to determine fix number of dirs from path */
		    if (*(cp + 1) == '0') {
			pdirs = 1;
			cp++;
		    }
		    if (*(cp + 1) >= '1' && *(cp + 1) <= '9') {
			j = *(cp + 1) - '0';
			cp++;
		    }
		    else {
			j = 1;
		    }
		    if ((z = value(STRcwd)) == STRNULL)
			break;
		    (void) Strcpy(buff, z);
		    if (!buff[1]) /* if CWD == / */
			*p++ = attributes | buff[0];
		    else {
			if ((scp != 'C') && (q = value(STRhome)) != STRNULL &&
			    Strncmp(buff, q, (size_t) (k = Strlen(q))) == 0 &&
			    (buff[k] == '/' || buff[k] == '\0')) {
			    buff[--k] = '~';
			    q = &buff[k];
			    /* RWM - reset the path length */
			    updirs = 0;
			}
			else {
			    q = buff;
			    /* RWM - in case first char is not '/' */
			    if (*q == '/') updirs = 0; else updirs = 1;
			}
			/* RWM - calculate elements in the path */
			for (z = q; *z; z++) {
			    if (*z == '/') updirs++;
			    continue;	/* find the end */
			}
			/* RWM - the ones we will skip can be found here */
			updirs -= j;
			
			while (j-- > 0) {
			    while ((z > q) && (*z != '/'))
				z--;	/* back up */
			    if (j && z > q)
				z--;
			}
			if (*z == '/' && z != q)
			    z++;

			/* RWM - if *q == '~' and *z != '~' then print */
			/*       out the '~' ahead of the partial path */
			if (pdirs && (*q == '~') && (*z != '~'))
			    *p++ = attributes | '~';
			
			/* RWM - tell you how many dirs we've ignored */
			/*       and add '/' at front of this         */
			if (updirs > 0 && pdirs) {
			    *p++ = attributes | '/';
			    *p++ = attributes | '<';
			    if (updirs > 9) {
				*p++ = attributes | '9';
				*p++ = attributes | '+';
			    } else
				*p++ = attributes | ('0' + updirs);
			    *p++ = attributes | '>';
			}
			/* RWM - end of added code */

			while (*z) {
			    *p++ = attributes | *z++;
			    if (p >= ep) break;
			}
		    }
		}
		break;
	    case 'n':
#ifndef HAVENOUTMP
		if (what == FMT_WHO) {
		    if ((cz = who_info(info, 'n', cbuff)) != NULL)
			while (*cz) {
			    *p++ = attributes | *cz++;
			    if (p >= ep) break;
			}
		}
		else  
#endif /* HAVENOUTMP */
		{
		    if ((z = value(STRuser)) != STRNULL)
			while (*z) {
			    *p++ = attributes | *z++;
			    if (p >= ep) break;
			}
		}
		break;
	    case 'l':
#ifndef HAVENOUTMP
		if (what == FMT_WHO) {
		    if ((cz = who_info(info, 'l', cbuff)) != NULL)
			while (*cz) {
			    *p++ = attributes | *cz++;
			    if (p >= ep) break;
			}
		}
		else  
#endif /* HAVENOUTMP */
		{
		    if ((z = value(STRtty)) != STRNULL)
			while (*z) {
			    *p++ = attributes | *z++;
			    if (p >= ep) break;
			}
		}
		break;
	    case 'd':
		for (cz = day_list[t->tm_wday]; *cz;) {
		    *p++ = attributes | *cz++;
		    if (p >= ep) break;
		}
		break;
	    case 'D':
		Itoa(t->tm_mday, buff);
		if (p >= ep - 3) break;
		if (buff[1]) {
		    *p++ = attributes | buff[0];
		    *p++ = attributes | buff[1];
		}
		else {
		    *p++ = attributes | '0';
		    *p++ = attributes | buff[0];
		}
		break;
	    case 'w':
		if (p >= ep - 20) break;
		for (cz = month_list[t->tm_mon]; *cz;) 
		    *p++ = attributes | *cz++;
		break;
	    case 'W':
		if (p >= ep - 3) break;
		Itoa(t->tm_mon + 1, buff);
		if (buff[1]) {
		    *p++ = attributes | buff[0];
		    *p++ = attributes | buff[1];
		}
		else {
		    *p++ = attributes | '0';
		    *p++ = attributes | buff[0];
		}
		break;
	    case 'y':
		if (p >= ep - 3) break;
		Itoa(t->tm_year, buff);
		if (buff[1]) {
		    *p++ = attributes | buff[0];
		    *p++ = attributes | buff[1];
		}
		else {
		    *p++ = attributes | '0';
		    *p++ = attributes | buff[0];
		}
		break;
	    case 'Y':
		if (p >= ep - 5) break;
		Itoa(t->tm_year + 1900, buff);
		*p++ = attributes | buff[0];
		*p++ = attributes | buff[1];
		*p++ = attributes | buff[2];
		*p++ = attributes | buff[3];
		break;
	    case 'S':		/* start standout */
		attributes |= STANDOUT;
		break;
	    case 'B':		/* start bold */
		attributes |= BOLD;
		break;
	    case 'U':		/* start underline */
		attributes |= UNDER;
		break;
	    case 's':		/* end standout */
		attributes &= ~STANDOUT;
		break;
	    case 'b':		/* end bold */
		attributes &= ~BOLD;
		break;
	    case 'u':		/* end underline */
		attributes &= ~UNDER;
		break;
	    case 'L':
		ClearToBottom();
		break;
	    case '?':
		if ((z = value(STRstatus)) != STRNULL)
		    while (*z) {
			*p++ = attributes | *z++;
			if (p >= ep) break;
		    }
		break;
	    case '%':
		*p++ = attributes | '%';
		break;
	    case '{':		/* literal characters start */
#if LITERAL == 0
		/*
		 * No literal capability, so skip all chars in the literal
		 * string
		 */
		while (*cp != '\0' && (*cp != '%' || cp[1] != '}'))
		    cp++;
#endif				/* LITERAL == 0 */
		attributes |= LITERAL;
		break;
	    case '}':		/* literal characters end */
		attributes &= ~LITERAL;
		break;
	    default:
#ifndef HAVENOUTMP
		if (*cp == 'a' && what == FMT_WHO) {
		    cz = who_info(info, 'a', cbuff);
		    while (*cz) {
			*p++ = attributes | *cz++;
			if (p >= ep) break;
		    }
		}
		else 
#endif /* HAVENOUTMP */
		{
		    if (p >= ep - 3) break;
		    *p++ = attributes | '%';
		    *p++ = attributes | *cp;
		}
		break;
	    }
	}
	else if (*cp == '\\' || *cp == '^') 
	    *p++ = attributes | parseescape(&cp);
	else if (*cp == '!') {	/* EGS: handle '!'s in prompts */
	    if (what == FMT_HISTORY) 
		fmthist('h', info, cbuff);
	    else
		(void) xsprintf(cbuff, "%d", eventno + 1);
	    for (cz = cbuff; *cz; cz++) {
		*p++ = attributes | *cz;
		if (p >= ep) break;
	    }
	}
	else 
	    *p++ = attributes | *cp;	/* normal character */
    }
    *p = '\0';
}
