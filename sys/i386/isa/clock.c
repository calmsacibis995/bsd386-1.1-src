/*-
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: clock.c,v 1.9 1993/11/08 00:40:40 karels Exp $
 */
 
/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
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
 *
 *	@(#)clock.c	7.2 (Berkeley) 5/12/91
 */

/*
 * Primitive clock interrupt routines.
 */
#include "param.h"
#include "time.h"
#include "kernel.h"
#include "device.h"
#include "machine/cpu.h"
#include "icu.h"
#include "isa.h"
#include "rtc.h"
#include "timerreg.h"

#define DAYST 119
#define DAYEN 303
#define GCODE_SEL       1		/* XXX Kernel Code Descriptor */
#define IDTVEC(name)    __CONCAT(X,name)

int	timer0_divider;

/*
 * Initialize real-time clock, timer 0,
 * called early in startup to set clock mode and rate.
 */
initrtclock()
{

	/* initialize 8253 clock */
	outb(TIMER_MODE, TIMER_SEL0|TIMER_RATEGEN|TIMER_16BIT);

	/*
	 * Divider for 1193182 Hz clock to get closest approximation
	 * to hz.  Should also adjust estimated clock tick, but we're
	 * not quite ready for that.
	 */
	timer0_divider = (TIMER_FREQ + hz / 2) / hz;
	outb(TIMER_CNTR0, timer0_divider);
	outb(TIMER_CNTR0, timer0_divider >> 8);
}

int
readrtclock()
{
	int lo, hi;

	outb(TIMER_MODE, TIMER_SEL0|TIMER_LATCH);
	lo = inb(TIMER_CNTR0);
	hi = inb(TIMER_CNTR0);
	hi = (hi << 8) | lo;
	/*
	 * Check for broken hardware; unfortunately, it exists.
	 */
	if (hi > timer0_divider)
		return (-1);
	return (hi);
}

/*
 * Start real-time clock.  Most of the work was done earlier, in initrtclock.
 */
startrtclock()
{
	int s;

	/* initialize brain-dead battery powered clock */
	outb(IO_RTC, RTC_STATUSA);
	outb(IO_RTC+1, 0x26);
	outb(IO_RTC, RTC_STATUSB);
	outb(IO_RTC+1, 2);

	outb(IO_RTC, RTC_DIAG);
	if (s = inb(IO_RTC+1))
		printf("RTC BIOS diagnostic error %b\n", s, RTCDG_BITS);
#if 0
	/*
	 * Attempting to clear any error here
	 * messes up BIOS settings on some machines.
	 */
	outb(IO_RTC, RTC_DIAG);
	outb(IO_RTC+1, 0);
#endif
}

/* convert 2 digit BCD number to integer */
int bcd2i(b) 
{ 
	return (b >> 4) * 10 + (b & 0xf);
}

/* convert integer to 2 digit BCD number */
int i2bcd(i)
{
	return ((i / 10) << 4) + i % 10;
}

#define	SEC_HOUR	(60 * 60)
#define	SEC_DAY		(SEC_HOUR * 24)
#define	SEC_YEAR	(SEC_DAY * 365)		/* non leap year */
#define SEC_4YEARS	(SEC_YEAR * 4 + SEC_DAY)

#define START_YEAR	70	/* Unix date from 1970; CMOS holds 00-99 */
	/* 1 Jan 1970 was Thu (START_DAY_WEEK), and MON is 0  */
#define START_DAY_WEEK	3

/* convert years to seconds (from 1970) */
unsigned long
ytos(y)
	unsigned y;
{

	y -= START_YEAR;
	return (y * SEC_YEAR + ((y + (START_YEAR - 1) % 4) / 4) * SEC_DAY);
}
		/*    31, 28, 31, 30,  31,  30,  31,  31,  30,  31,  30,  31 */
static int days_ym[] ={0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365};

/* convert months to seconds */
unsigned long
mtos(m, leap)
	int m, leap;
{
	int d = days_ym[m - 1];

	if (leap && m > 2)
		++d;
	return (d * SEC_DAY);
 }
 
/*
 * Initialize the time of day register, based on the time base which is, e.g.
 * from a filesystem.
 */
/* ARGSUSED */
inittodr(base)
	time_t base;
{
	unsigned long sec;
	int leap, day_week, t, yd;
	int sa;

	/* do we have a realtime clock present? (otherwise we loop below) */
	sa = rtcin(RTC_STATUSA);
	if (sa == 0xff || sa == 0)
		return;

	/* ready for a read? */
	while ((sa&RTCSA_TUP) == RTCSA_TUP)
		sa = rtcin(RTC_STATUSA);

	yd = bcd2i(rtcin(RTC_YEAR));
	leap = !(yd % 4); 				/* works until 2100 */
	sec = ytos(yd);					/* year */
	yd = mtos(bcd2i(rtcin(RTC_MONTH)), leap);	/* month */
	sec += yd;
	t = (bcd2i(rtcin(RTC_DAY))-1) * SEC_DAY;	/* date */
	sec += t;
	yd += t;
	day_week = rtcin(RTC_WDAY);			/* day */
	sec += bcd2i(rtcin(RTC_HRS)) * SEC_HOUR;	/* hour */
	sec += bcd2i(rtcin(RTC_MIN)) * 60;		/* minutes */
	sec += bcd2i(rtcin(RTC_SEC));			/* seconds */

#ifdef notdef
	/* XXX off by one? Need to calculate DST on SUNDAY */
	/* Perhaps we should have the RTC hold GMT time to save */
	/* us the bother of converting. */
	yd = yd / 24*60*60;
	if ((yd >= DAYST) && ( yd <= DAYEN)) {
		sec -= 60*60;
	}
#endif
	sec += tz.tz_minuteswest * 60;

	time.tv_sec = sec;
}

int	rtc_reset = 1;

/*
 * Reset the rtc clock to our current notion of time.
 */
resettodr()
{
	long sec;
	int yr, mo, day, hr, mi, n;
	int mleap;
	
	if (rtc_reset == 0)
		return;
	sec = time.tv_sec;
	sec -= tz.tz_minuteswest * 60;

	rtcout(RTC_WDAY, n = ((sec / SEC_DAY + START_DAY_WEEK) % 7) + 1);
	
	yr = START_YEAR + (sec / SEC_4YEARS) * 4; 
	n = sec % SEC_4YEARS;
	if (n > SEC_YEAR * (START_YEAR % 4 + 1) + SEC_DAY)
		yr += (n - SEC_DAY) / SEC_YEAR;
	else
		yr += n / SEC_YEAR;
	rtcout(RTC_YEAR, i2bcd(yr));
	sec -= ytos(yr);
	day = sec / SEC_DAY;
	mleap = (yr % 4 == 0 && day >= days_ym[2]) ? 1 : 0;
	for (mo = 0; day - mleap >= days_ym[mo]; mo ++)
		;
	rtcout(RTC_MONTH, i2bcd(mo));
	day -= days_ym[mo - 1] + mleap;
	rtcout(RTC_DAY, i2bcd(day + 1));
	hr = (sec % SEC_DAY) / SEC_HOUR;
	rtcout(RTC_HRS, i2bcd(hr));
	mi = (sec % SEC_HOUR) / 60;
	rtcout(RTC_MIN, i2bcd(mi));
	rtcout(RTC_SEC, i2bcd(sec % 60));
}

extern IDTVEC(clkintr);

/*
 * Wire clock interrupt in.
 */
enablertclock()
{
	int sel = GSEL(GCODE_SEL, SEL_KPL);

	/* 
	 * We'll make hardclock the one hardwired interrupt for
	 * a quick performance enhancement.  (as opposed to making
	 * it fall through the interrupt switch like everything else)
	 */
	setidt(ICU_OFFSET+0, &IDTVEC(clkintr), SDT_SYS386IGT, SEL_KPL, sel);
	(void) spl0();
}
