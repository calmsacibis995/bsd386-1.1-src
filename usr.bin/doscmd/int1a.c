/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	Krystal $Id: int1a.c,v 1.1 1994/01/14 23:34:05 polk Exp $ */
#include "doscmd.h"

void
int1a(tf)
struct trapframe *tf;
{
	struct	timeval	tod;
	struct	timezone zone;
	struct	tm *tm;
	long	value;
	static long midnight = 0;

	struct byteregs *b = (struct byteregs *)&tf->tf_bx;
        tf->tf_eflags &= ~PSL_C;

	switch (b->tf_ah) {
	case 0x00:
		gettimeofday(&tod, &zone);

		if (midnight == 0) {
		    tm = localtime(&boot_time.tv_sec);
		    midnight = boot_time.tv_sec - (((tm->tm_hour * 60)
					           + tm->tm_min) * 60
						   + tm->tm_sec);
		}

		b->tf_al = (tod.tv_sec - midnight) / (24 * 60 * 60);

		if (b->tf_al) {
		    tm = localtime(&boot_time.tv_sec);
		    midnight = boot_time.tv_sec - (((tm->tm_hour * 60)
					           + tm->tm_min) * 60
						   + tm->tm_sec);
		}

		if (tod.tv_usec < boot_time.tv_usec) {
		    tod.tv_usec += 1000000L;
		    tod.tv_sec -= 1;
		}

		tod.tv_sec -= boot_time.tv_sec;
		tod.tv_usec -= boot_time.tv_usec;

		value = (tod.tv_sec * 182 + tod.tv_usec / (1000000L/182)) / 10;
		tf->tf_cx = value >> 16;
		tf->tf_dx = value;
		break;

	case 0x02:
		gettimeofday(&tod, &zone);
		tm = localtime(&tod.tv_sec);
		b->tf_ch = tm->tm_hour;
		b->tf_cl = tm->tm_min;
		b->tf_dh = tm->tm_sec;
		break;
	case 0x03:
		/*
		 * XXX - we probably should keep track of a delta here to
		 * convice DOS that we really did change the time
		 */
		break;

	case 0x04:
		gettimeofday(&tod, &zone);
		tm = localtime(&tod.tv_sec);
		b->tf_ch = tm->tm_year > 99 ? 20 : 19;
		b->tf_cl = tm->tm_year % 100;
		b->tf_dh = tm->tm_mon;
		b->tf_dl = tm->tm_mday;
		break;

	default:
		unknown_int2(0x1a, b->tf_ah, tf);
		break;
	}
}
