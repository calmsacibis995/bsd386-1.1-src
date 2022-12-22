/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	Krystal $Id: bios.c,v 1.1 1994/01/14 23:23:16 polk Exp $ */
#include "doscmd.h"

#define	BIOS_copyright         0xfe000
#define	BIOS_reset             0xfe05b
#define	BIOS_nmi               0xfe2c3
#define	BIOS_hdisk_table       0xfe401
#define	BIOS_boot              0xfe6f2
#define	BIOS_comm_table        0xfe729
#define	BIOS_comm_io           0xfe739
#define	BIOS_keyboard_io       0xfe82e
#define	BIOS_keyboard_isr      0xfe987
#define	BIOS_fdisk_io          0xfec59
#define	BIOS_fdisk_isr         0xfef57
#define	BIOS_disk_parms        0xfefc7
#define	BIOS_printer_io        0xfefd2
#define	BIOS_video_io          0xff065
#define	BIOS_video_parms       0xff0a4
#define	BIOS_mem_size          0xff841
#define	BIOS_equipment         0xff84d
#define	BIOS_cassette_io       0xff859
#define	BIOS_video_font        0xffa6e
#define	BIOS_time_of_day       0xffe6e
#define	BIOS_timer_int         0xffea5
#define	BIOS_vector            0xffef3
#define	BIOS_dummy_iret        0xfff53
#define	BIOS_print_screen      0xfff54
#define	BIOS_hard_reset        0xffff0
#define	BIOS_date_stamp        0xffff5
#define	BIOS_hardware_id       0xffffe

static u_char video_parms[] = {
    0x38, 40, 0x2d, 10, 0x1f, 6, 0x19, 0x1c, 2,  7,  6,  7, 0, 0, 0, 0,
    0x71, 80, 0x5a, 10, 0x1f, 6, 0x19, 0x1c, 2,  7,  6,  7, 0, 0, 0, 0,
    0x38, 40, 0x2d, 10, 0x7f, 6, 0x64, 0x70, 2,  1,  6,  7, 0, 0, 0, 0,
    0x61, 80, 0x52, 15, 0x19, 6, 0x19, 0x19, 2, 13, 11, 12, 0, 0, 0, 0,
};

static u_char disk_params[] = {
    0xdf, 2, 0x25, 2, 0x0f, 0x1b, 0xff, 0x54, 0xf6, 0x0f, 8,
};

static u_short comm_table[] = {
    1047, 768, 384, 192, 96, 48, 24, 12,
};

int nfloppies = 0;
int ndisks = 0;
int nserial = 0;
int nparallel = 0;

bios_init()
{
    int i;
    extern u_char *InDOS;
    u_char *jtab;
    struct timeval tv;
    struct timezone tz;
    struct tm tm;


    strcpy((char *)BIOS_copyright,
	   "Copyright (C) 1993 Krystal Technologies/BSDI");

    *(u_short *)BIOS_reset = 0xffcd;
    *(u_short *)BIOS_nmi = 0xffcd;
    *(u_short *)BIOS_boot = 0xffcd;
    *(u_short *)BIOS_comm_io = 0xffcd;
    *(u_short *)BIOS_keyboard_io = 0xffcd;
    *(u_short *)BIOS_keyboard_isr = 0xffcd;
    *(u_short *)BIOS_fdisk_io = 0xffcd;
    *(u_short *)BIOS_fdisk_isr = 0xffcd;
    *(u_short *)BIOS_printer_io = 0xffcd;
    *(u_short *)BIOS_video_io = 0xffcd;
    *(u_short *)BIOS_cassette_io = 0xffcd;
    *(u_short *)BIOS_time_of_day = 0xffcd;
    *(u_short *)BIOS_timer_int = 0xffcd;
    *(u_short *)BIOS_dummy_iret = 0xffcd;
    *(u_short *)BIOS_print_screen = 0xffcd;
    *(u_short *)BIOS_hard_reset = 0xffcd;
    *(u_short *)BIOS_mem_size = 0xffcd;
    *(u_short *)BIOS_equipment = 0xffcd;
    *(u_short *)BIOS_vector = 0xffcd;

    memcpy((u_char *)BIOS_video_parms, video_parms, sizeof(video_parms));
    memcpy((u_char *)BIOS_disk_parms, disk_params, sizeof(disk_params));
    memcpy((u_char *)BIOS_comm_table, comm_table, sizeof(comm_table));

    *(u_short *)BIOS_video_font = 0xffcd;

    jtab = (u_char *)BIOS_date_stamp;
    *jtab++ = '1';
    *jtab++ = '0';
    *jtab++ = '/';
    *jtab++ = '3';
    *jtab++ = '1';
    *jtab++ = '/';
    *jtab++ = '9';
    *jtab++ = '3';

    *(u_char *)BIOS_hardware_id = 0xfe;           /* Identify as a PC/XT */
    *(u_char *)BIOS_hardware_id = 0xff;           /* Identify as a PC */
    *(u_char *)BIOS_hardware_id = 0xfc;           /* Identify as a PC/AT */
    /*
     * Interrupt revectors F000:0000 - F000:03ff
     */
    jtab = (u_char *)0xF0000;

    for (i = 0; i < 0x100; ++i) {
	    if (i >= 0x60 && i < 0x68)
		continue;
	    if (i >= 0x78 && i < 0xE2)
		continue;
	    ivec[i] = 0xF0000000L | (i * 4);
	    jtab[4 * i    ] = 0xcd;         /* INT */
	    jtab[4 * i + 1] = i;
	    jtab[4 * i + 2] = 0xcf;         /* IRET */
    }

#if 0
    jtab[4 * 8    ] = 0xcd;                 /* INT 1C */
    jtab[4 * 8 + 1] = 0x1c;
    jtab[4 * 8 + 2] = 0xcf;                 /* IRET */

    jtab[4 * 0x1c] = 0xcf;                  /* IRET on int1c */
#endif

    /*
     * Misc variables from F000:0400 - F000:0fff
     */
    jtab = (unsigned char *)0xF2400;        /* moved to un-accessable mem */
    rom_config = 0xF0002400;
    *(u_short *)jtab = 20;  /* Length of entry */
    jtab += 2;
    *jtab++ = *(u_char *)BIOS_hardware_id;

    *jtab++ = 0x00;         /* Sub model */
    *jtab++ = 0x01;         /* Bios Rev Enhanced kbd w/3.5" floppy */
    *jtab++ = 0x20;         /* real time clock present */
    *jtab++ = 0;            /* Reserved */
    *jtab++ = 0;
    *jtab++ = 0;
    *jtab++ = 0;
    strcpy((char *)jtab, "BSDI BIOS");
    *jtab += 10;

    InDOS = jtab++;
    *InDOS = 0;

    *(u_short *)&BIOSDATA[0x10] = (1 << 0)	/* Diskette avail for boot */
			| (1 << 1)		/* Math co-processor */
			| (0 << 2)		/* No pointing device */
			| (2 << 4)		/* Initial video (80 x 25 C) */
			| (nfloppies - 1 << 6)	/* Number of floppies - 1 */
			| (nserial << 9)	/* Number of serial devices */
			| (nparallel << 14)	/* Number of parallel devices */
				;

    *(u_short *)&BIOSDATA[0x13] = 640;		/* Amount of memory */
    BIOSDATA[0x75] = ndisks;			/* number of fixed disks */

    gettimeofday (&tv, &tz);
    tm = *localtime (&tv.tv_sec);
    *(u_long *)&BIOSDATA[0x6c] =
	    (((tm.tm_hour * 60 + tm.tm_min) * 60) + tm.tm_sec) * 182 / 10; 
}
