/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	Krystal $Id: cmos.c,v 1.1 1994/01/14 23:23:40 polk Exp $ */
#include "doscmd.h"

#define ALARM_ON     ((unsigned char) 0x20)
#define FAST_TIMER ((unsigned char) 0x40)
#define SEC_SIZE     1
#define MIN_SIZE     60
#define HOUR_SIZE    (MIN_SIZE * 60)
#define DAY_SIZE     (HOUR_SIZE * 24)
#define YEAR_DAY     365

#define SEC_MS 1000000
#define FAST_TICK_BSD 0x3D00

#define Jan 31
#define Feb 28
#define Mar 31
#define Apr 30
#define May 31
#define Jun 30
#define Jul 31
#define Aug 31
#define Sep 31
#define Oct 31
#define Nov 30
#define Dec 31

static unsigend char cmos_last_port_70 = 0;
static unsigned char cmos_data[0x40] = {
    0x00,                /* Current Second */
    0x00,                /* Alarm Second */
    0x00,                /* Current minute */
    0x00,                /* Alarm minute */
    0x00,                /* Current hour */
    0x00,                /* Alarm hour */
    0x00,                /* Current week day */
    0x00,                /* Current day */
    0x00,                /* Current month */
    0x00,                /* Current year */
    0x26,                /* Status register A */
    0x02,                /* Status register B */
    0x00,                /* Status register C */
    0x80,                /* Status register D */
    0x00,                /* Diagnostic status */
    0x00,                /* Shutdown Code */
    0x00,                /* Drive types (1 FDHD disk) */
    0x00,                /* Fixed disk 0 type */
    0x00,                /* Fixed disk 1 type */
    0x00,
    0x00,                /* Installed equipment */
};

int day_in_year [12] = {
    0, Jan, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov
};

static int fast_delta_uclock;
static struct timeval fast_clock;
static int fast_tick;

struct timeval glob_clock;
struct timezone tz = { 0, 0};
struct tm *f_t;
long delta_clock =0;
static int cmos_alarm_time = 0;
static int cmos_alarm_daytime = 0;
/*static int zzz = 0;*/

static inline int
day_in_mon_year (mon, year)
{
    return day_in_year[mon] + (mon > 2 && (year % 4 == 0));
}


static inline int
to_BCD (int n)
{
    n &= 0xFF;
    return n%10 + ((n/10)<<4);
}

static inline int
from_BCD (int n)
{
    n &= 0xFF;
    return (n & 0xF) + (n >> 4) * 10;
}

unsigned char
cmos_inb(int portnum)
{
    unsigned char ret_val = 0xff;
    int cmos_reg;
    long loc_clock;

    switch(portnum) {
    case 0x70:
	ret_val = cmos_last_port_70;
        break;
    case 0x71:
        cmos_reg = cmos_last_port_70 & 0x3f;
        if (cmos_reg < 0xa) {
            gettimeofday (&glob_clock, &tz);
            loc_clock = glob_clock.tv_sec + delta_clock;
            f_t = localtime ((const time_t *) (&loc_clock));
            switch (cmos_reg) {
                case 0: ret_val = to_BCD (f_t->tm_sec);
                    break;
                case 2: ret_val = to_BCD (f_t->tm_min);
                    break;
                case 4: ret_val = to_BCD (f_t->tm_hour);
                    break;
                case 6: ret_val = to_BCD (f_t->tm_wday);
                    break;
                case 7: ret_val = to_BCD (f_t->tm_mday);
                    break;
                case 8: ret_val = to_BCD (f_t->tm_mon + 1);
                    break;
                case 9: ret_val = to_BCD (f_t->tm_year);
                    break;
            }
        } else
	    ret_val = cmos_data[cmos_reg];
        break;
    }
    return(ret_val);
}

void
cmos_outb(int portnum, unsigned char byte)
{
    int cmos_reg;
    int year;
    int time00;
    long loc_clock;

    cmos_reg = cmos_last_port_70 & 0x3f;

    switch(portnum) {
    case 0x70:
        cmos_last_port_70 = byte;
        break;
    case 0x71:
        if (cmos_reg < 0xa) {
            gettimeofday (&glob_clock, &tz);
            loc_clock = glob_clock.tv_sec + delta_clock;
            f_t = localtime ((const time_t *) (&loc_clock));
        }
        switch (cmos_reg) {
	case 0: delta_clock += SEC_SIZE * (from_BCD (byte) - f_t->tm_sec);
		break;
	case 1: cmos_alarm_daytime += from_BCD (byte) - from_BCD (cmos_data[1]);
	    break;
	case 2: delta_clock += MIN_SIZE * (from_BCD (byte) - f_t->tm_min);
		break;
	case 3: cmos_alarm_daytime += MIN_SIZE * (   from_BCD (byte)
						   - from_BCD (cmos_data[3]));
	    break;
	case 4: delta_clock += HOUR_SIZE * (from_BCD (byte) - f_t->tm_hour);
		break;
	case 5: cmos_alarm_daytime += 
		     HOUR_SIZE * (from_BCD (byte) - from_BCD (cmos_data[5]));
	    break;
	case 7: delta_clock += DAY_SIZE * (from_BCD (byte) - f_t->tm_mday);
		break;
	case 8: delta_clock += DAY_SIZE *
		    (day_in_mon_year( from_BCD (byte), f_t->tm_year) -
		     day_in_mon_year( f_t->tm_mon + 1, f_t->tm_year));
		break;

	case 9: year = from_BCD (byte);
		delta_clock += DAY_SIZE * (YEAR_DAY * (year - f_t->tm_year)
			    + (year/4 - f_t->tm_year/4));
		break;
	case 0xB:
	    cmos_data[0xc] = byte;
	    if (byte & ALARM_ON) {
		debug(D_ALWAYS, "Alarm turned on\n");
		time00 = glob_clock.tv_sec + delta_clock -
		    	(f_t->tm_sec + MIN_SIZE * f_t->tm_min
				     + HOUR_SIZE * f_t->tm_hour);
		cmos_alarm_time = time00 + cmos_alarm_daytime;
		if (cmos_alarm_time < (glob_clock.tv_sec + delta_clock))
		    cmos_alarm_time += DAY_SIZE;
	    }
	    if (byte & FAST_TIMER) {
		debug(D_ALWAYS, "Fast timer turned on\n");
		fast_clock.tv_sec = glob_clock.tv_sec;
		fast_clock.tv_usec = glob_clock.tv_usec;
		fast_tick = 0;
	    }
	    break;
        }
        cmos_data[cmos_reg] = byte;
        break;
    default:
	return;
    }
}

#if 0
void
alarm_check ()
{
    if (cmos_data[0xc] & ALARM_ON) {
        if ((glob_clock.tv_sec + delta_clock) >= cmos_alarm_time) {
            cmos_alarm_time += DAY_SIZE;
            set_irq_request (8);
        }
    }
    if (cmos_data[0xc] & FAST_TIMER) {
        fast_delta_uclock = (glob_clock.tv_sec - fast_clock.tv_sec) * SEC_MS;
        fast_delta_uclock += glob_clock.tv_usec - fast_clock.tv_usec;

        fast_tick += fast_delta_uclock / FAST_TICK_BSD;
        fast_clock.tv_sec = glob_clock.tv_sec - 1;
        fast_clock.tv_usec = (glob_clock.tv_usec + SEC_MS) -
            fast_delta_uclock % FAST_TICK_BSD;
        set_irq_request (8);
    }
}

int fast_timer_ready(void)
{
    if ((cmos_data[0xc] & FAST_TIMER) && fast_tick)
        fast_tick--;
    else
        fast_tick = 0;
    return fast_tick;
}
#endif

static inline unsigned char
flop_type(int id)
{
    int ret_val;
    
    switch(id) {
    case 360:
        ret_val = 1;
        break;
    case 720:
        ret_val = 3;
        break;
    case 1200:
        ret_val = 2;
        break;
    case 1440:
        ret_val = 4;
        break;
    }
    return(ret_val);
}

void
cmos_init(void)
{
#if 0
    struct device *dp;
#endif
    int numflops = 0;
    int checksum = 0;
    int i;

    cmos_data[0x0e] = 0;
#if 0
    if ((dp = search_device(DEV_FLOP, 1)) != 0) {
        numflops++;
        cmos_data[0x10] = flop_type(dp->dev_id) << 4;
    }
    if ((dp = search_device(DEV_FLOP, 2)) != 0) {
        numflops++;
        cmos_data[0x10] |= flop_type(dp->dev_id);
    }
    if ((dp = search_device(DEV_FLOP, 3)) != 0) {
        numflops++;
        cmos_data[0x11] = flop_type(dp->dev_id) << 4;
    }
    if ((dp = search_device(DEV_FLOP, 4)) != 0) {
        numflops++;
        cmos_data[0x11] |= flop_type(dp->dev_id);
    }
    if ((dp = search_device(DEV_HARD, 1)) != 0) {
        if (dp->dev_id < 15)
            cmos_data[0x12] = dp->dev_id << 4;
        else {
            cmos_data[0x12] = 15 << 4;
            cmos_data[0x19] = dp->dev_id;
        }
    }
    if ((dp = search_device(DEV_HARD, 2)) != 0) {
        if (dp->dev_id < 15)
            cmos_data[0x12] |= dp->dev_id;
        else {
            cmos_data[0x12] |= 15;
            cmos_data[0x1a] = dp->dev_id;
        }
    }
#else
    numflops = 1;
    cmos_data[0x10] = flop_type(1440) << 4;
#endif
    if (numflops)		/* floppy drives present + numflops */
        cmos_data[0x14] = ((numflops - 1) << 6) | 1;

    cmos_data[0x15] = 0x80;                /* base memory 640k */
    cmos_data[0x16] = 0x2;
    for (i=0x10; i<=0x2d; i++)
        checksum += cmos_data[i];
    cmos_data[0x2e] = checksum >>8;        /* High byte */
    cmos_data[0x2f] = checksum & 0xFF; /* Low    byte */

    cmos_data[0x32] = 0x19; /* Century in BCD ; temporary */

    for (i = 1; i < 12; i++){
        day_in_year[i] += day_in_year[i-1];
    }
    
    define_input_port_handler(0x70, cmos_inb);
    define_input_port_handler(0x71, cmos_inb);
    define_output_port_handler(0x70, cmos_outb);
    define_output_port_handler(0x71, cmos_outb);
}
