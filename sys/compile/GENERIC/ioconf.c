/*
 * ioconf.c 
 * Generated by config program
 */

#include "sys/param.h"
#include "sys/device.h"
#include "i386/isa/isa.h"
#include "i386/isa/icu.h"

extern struct cfdriver isacd;
extern struct cfdriver pcconscd;
extern struct cfdriver comcd;
extern struct cfdriver lpcd;
extern struct cfdriver pecd;
extern struct cfdriver xircd;
extern struct cfdriver fdccd;
extern struct cfdriver fdcd;
extern struct cfdriver wdccd;
extern struct cfdriver wdcd;
extern struct cfdriver wdccd;
extern struct cfdriver wdcd;
extern struct cfdriver mcdcd;
extern struct cfdriver wtcd;
extern struct cfdriver npxcd;
extern struct cfdriver vgacd;
extern struct cfdriver bmscd;
extern struct cfdriver lmscd;
extern struct cfdriver ahacd;
extern struct cfdriver tgcd;
extern struct cfdriver ahacd;
extern struct cfdriver sdcd;
extern struct cfdriver stcd;
extern struct cfdriver necd;
extern struct cfdriver eocd;
extern struct cfdriver epcd;
extern struct cfdriver tncd;
extern struct cfdriver wecd;
extern struct cfdriver efcd;
extern struct cfdriver elcd;
extern struct cfdriver excd;
extern struct cfdriver eahacd;
extern struct cfdriver tgcd;

 /* ISA Locators */
static int loc[] = {
/*	iobase	iosiz	maddr	msize	irq	drq	*/
/*0*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* isa0 */
/*1*/	IO_KBD,	0,	0,	    0,	IRQUNK,	 0,	/* pccons0 */
/*2*/	IO_COM1,	0,	0,	    0,	IRQUNK,	 0,	/* com0 */
/*3*/	IO_COM2,	0,	0,	    0,	IRQUNK,	 0,	/* com1 */
/*4*/	0x378,	0,	0,	    0,	IRQ7,	 0,	/* lp0 */
/*5*/	0x3bc,	0,	0,	    0,	IRQ7,	 0,	/* lp2 */
/*6*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* pe0 */
/*7*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* pe2 */
/*8*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* xir0 */
/*9*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* xir2 */
/*10*/	IO_FD1,	0,	0,	    0,	IRQUNK,	 2,	/* fdc0 */
/*11*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* fd0 */
/*12*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* fd1 */
/*13*/	IO_WD1,	0,	0,	    0,	IRQUNK,	 0,	/* wdc0 */
/*14*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* wd0 */
/*15*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* wd1 */
/*16*/	IO_WD2,	0,	0,	    0,	IRQUNK,	 0,	/* wdc1 */
/*17*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* wd2 */
/*18*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* wd3 */
/*19*/	0x334,	0,	0,	    0,	IRQ9,	-1,	/* mcd0 */
/*20*/	0x340,	0,	0,	    0,	IRQ9,	-1,	/* mcd0 */
/*21*/	0x300,	0,	0,	    0,	IRQUNK,	 1,	/* wt0 */
/*22*/	IO_NPX,	0,	0,	    0,	IRQUNK,	 0,	/* npx0 */
/*23*/	IO_VGA,	0,	0xa0000,	65536,	IRQUNK,	 0,	/* vga0 */
/*24*/	0x23c,	0,	0,	    0,	IRQ5,	 0,	/* bms0 */
/*25*/	0x23c,	0,	0,	    0,	IRQ5,	 0,	/* lms0 */
/*26*/	0x330,	0,	0,	    0,	IRQUNK,	 5,	/* aha0 */
/*27*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* tg0 */
/*28*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* tg1 */
/*29*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* tg2 */
/*30*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* tg3 */
/*31*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* tg4 */
/*32*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* tg5 */
/*33*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* tg6 */
/*34*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* sd0 */
/*35*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* sd1 */
/*36*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* sd2 */
/*37*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* sd3 */
/*38*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* sd4 */
/*39*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* sd5 */
/*40*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* sd6 */
/*41*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* st0 */
/*42*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* st1 */
/*43*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* st2 */
/*44*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* st3 */
/*45*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* st4 */
/*46*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* st5 */
/*47*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* st6 */
/*48*/	0x320,	0,	0,	    0,	IRQUNK,	 0,	/* ne0 */
/*49*/	0x340,	0,	0,	    0,	IRQUNK,	 0,	/* ne0 */
/*50*/	0x360,	0,	0,	    0,	IRQUNK,	 0,	/* ne0 */
/*51*/	0x320,	0,	0,	    0,	IRQUNK,	 0,	/* eo0 */
/*52*/	0x240,	0,	0,	    0,	IRQUNK,	 0,	/* ep0 */
/*53*/	0x320,	0,	0,	    0,	IRQUNK,	 0,	/* ep0 */
/*54*/	0x320,	0,	0,	    0,	IRQUNK,	 3,	/* tn0 */
/*55*/	0x340,	0,	0,	    0,	IRQUNK,	 3,	/* tn0 */
/*56*/	0x360,	0,	0,	    0,	IRQUNK,	 3,	/* tn0 */
/*57*/	0x300,	0,	0,	    0,	IRQUNK,	 3,	/* tn0 */
/*58*/	0x280,	0,	0xd0000,	16384,	IRQUNK,	 0,	/* we0 */
/*59*/	0x2a0,	0,	0xd0000,	16384,	IRQUNK,	 0,	/* we0 */
/*60*/	0x2c0,	0,	0xd0000,	16384,	IRQUNK,	 0,	/* we0 */
/*61*/	0x2e0,	0,	0xd0000,	16384,	IRQUNK,	 0,	/* we0 */
/*62*/	0x300,	0,	0xd0000,	16384,	IRQUNK,	 0,	/* we0 */
/*63*/	0x320,	0,	0xd0000,	16384,	IRQUNK,	 0,	/* we0 */
/*64*/	0x340,	0,	0xd0000,	16384,	IRQUNK,	 0,	/* we0 */
/*65*/	0x360,	0,	0xd0000,	16384,	IRQUNK,	 0,	/* we0 */
/*66*/	0x380,	0,	0xd0000,	16384,	IRQUNK,	 0,	/* we0 */
/*67*/	0x3a0,	0,	0xd0000,	16384,	IRQUNK,	 0,	/* we0 */
/*68*/	0x3e0,	0,	0xd0000,	16384,	IRQUNK,	 0,	/* we0 */
/*69*/	0x280,	0,	0,	    0,	IRQUNK,	 0,	/* we0 */
/*70*/	0x2a0,	0,	0,	    0,	IRQUNK,	 0,	/* we0 */
/*71*/	0x2e0,	0,	0,	    0,	IRQUNK,	 0,	/* we0 */
/*72*/	0x300,	0,	0,	    0,	IRQUNK,	 0,	/* we0 */
/*73*/	0x310,	0,	0,	    0,	IRQUNK,	 0,	/* we0 */
/*74*/	0x330,	0,	0,	    0,	IRQUNK,	 0,	/* we0 */
/*75*/	0x350,	0,	0,	    0,	IRQUNK,	 0,	/* we0 */
/*76*/	0x250,	0,	0,	    0,	IRQUNK,	 0,	/* ef0 */
/*77*/	0x310,	0,	0xd0000,	65536,	IRQUNK,	 0,	/* el0 */
/*78*/	0x260,	0,	0,	    0,	IRQUNK,	 0,	/* ex0 */
/*79*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* eaha0 */
/*80*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* tg0 */
/*81*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* tg1 */
/*82*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* tg2 */
/*83*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* tg3 */
/*84*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* tg4 */
/*85*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* tg5 */
/*86*/	0x0,	0,	0,	    0,	IRQUNK,	 0,	/* tg6 */
};

static short pv[] = {
/*0*/	  0,  -1,	/* 'at isa0' */
/*2*/	 10,  -1,	/* 'at fdc0' */
/*4*/	 13,  -1,	/* 'at wdc0' */
/*6*/	 16,  -1,	/* 'at wdc1' */
/*8*/	 26,  -1,	/* 'at aha0' */
/*10*/	 27,  -1,	/* 'at tg0' */
/*12*/	 28,  -1,	/* 'at tg1' */
/*14*/	 29,  -1,	/* 'at tg2' */
/*16*/	 30,  -1,	/* 'at tg3' */
/*18*/	 31,  -1,	/* 'at tg4' */
/*20*/	 32,  -1,	/* 'at tg5' */
/*22*/	 33,  -1,	/* 'at tg6' */
/*24*/	 79,  -1,	/* 'at eaha0' */
/*26*/	  0,  -1,	/* 'at isa?' */
/*28*/	 10,  -1,	/* 'at fdc?' */
/*30*/	 13,   16,  -1,	/* 'at wdc?' */
/*33*/	 26,  -1,	/* 'at aha?' */
/*35*/	 27,   28,   29,   30,   31,   32,   33,  -1,	/* 'at tg?' */
/*43*/	 79,  -1,	/* 'at eaha?' */
};

/* vectors for autoconfig routines */
	/* driver	unit fnd	loc  flags parents ivstubs */
struct cfdata cfdata[] = {
/*0*/	{&isacd,   	0,   0,	loc+0,   	0,	0,	0  },
/*1*/	{&pcconscd,	0,   0,	loc+6,   	0,	pv+26,	0  },
/*2*/	{&comcd,   	0,   0,	loc+12,  	0,	pv+26,	0  },
/*3*/	{&comcd,   	1,   0,	loc+18,  	0,	pv+26,	0  },
/*4*/	{&lpcd,    	0,   0,	loc+24,  	1,	pv+26,	0  },
/*5*/	{&lpcd,    	2,   0,	loc+30,  	1,	pv+26,	0  },
/*6*/	{&pecd,    	0,   0,	loc+36,  	0,	pv+26,	0  },
/*7*/	{&pecd,    	2,   0,	loc+42,  	0,	pv+26,	0  },
/*8*/	{&xircd,   	0,   0,	loc+48,  	0,	pv+26,	0  },
/*9*/	{&xircd,   	2,   0,	loc+54,  	0,	pv+26,	0  },
/*10*/	{&fdccd,   	0,   0,	loc+60,  	0,	pv+26,	0  },
/*11*/	{&fdcd,    	0,   0,	loc+66,  	0,	pv+2,	0  },
/*12*/	{&fdcd,    	1,   0,	loc+72,  	0,	pv+2,	0  },
/*13*/	{&wdccd,   	0,   0,	loc+78,  	0,	pv+26,	0  },
/*14*/	{&wdcd,    	0,   0,	loc+84,  	0,	pv+4,	0  },
/*15*/	{&wdcd,    	1,   0,	loc+90,  	0,	pv+4,	0  },
/*16*/	{&wdccd,   	1,   0,	loc+96,  	0,	pv+26,	0  },
/*17*/	{&wdcd,    	2,   0,	loc+102, 	0,	pv+6,	0  },
/*18*/	{&wdcd,    	3,   0,	loc+108, 	0,	pv+6,	0  },
/*19*/	{&mcdcd,   	0,   0,	loc+114, 	0,	pv+26,	0  },
/*20*/	{&mcdcd,   	0,   0,	loc+120, 	0,	pv+26,	0  },
/*21*/	{&wtcd,    	0,   0,	loc+126, 	0,	pv+26,	0  },
/*22*/	{&npxcd,   	0,   0,	loc+132, 	0,	pv+26,	0  },
/*23*/	{&vgacd,   	0,   0,	loc+138, 	0,	pv+26,	0  },
/*24*/	{&bmscd,   	0,   0,	loc+144, 	0,	pv+26,	0  },
/*25*/	{&lmscd,   	0,   0,	loc+150, 	0,	pv+26,	0  },
/*26*/	{&ahacd,   	0,   0,	loc+156, 	0,	pv+26,	0  },
/*27*/	{&tgcd,    	0,   0,	loc+162, 	0,	pv+8,	0  },
/*28*/	{&tgcd,    	1,   0,	loc+168, 	0,	pv+8,	0  },
/*29*/	{&tgcd,    	2,   0,	loc+174, 	0,	pv+8,	0  },
/*30*/	{&tgcd,    	3,   0,	loc+180, 	0,	pv+8,	0  },
/*31*/	{&tgcd,    	4,   0,	loc+186, 	0,	pv+8,	0  },
/*32*/	{&tgcd,    	5,   0,	loc+192, 	0,	pv+8,	0  },
/*33*/	{&tgcd,    	6,   0,	loc+198, 	0,	pv+8,	0  },
/*34*/	{&sdcd,    	0,   0,	loc+204, 	0,	pv+35,	0  },
/*35*/	{&sdcd,    	1,   0,	loc+210, 	0,	pv+36,	0  },
/*36*/	{&sdcd,    	2,   0,	loc+216, 	0,	pv+37,	0  },
/*37*/	{&sdcd,    	3,   0,	loc+222, 	0,	pv+38,	0  },
/*38*/	{&sdcd,    	4,   0,	loc+228, 	0,	pv+39,	0  },
/*39*/	{&sdcd,    	5,   0,	loc+234, 	0,	pv+40,	0  },
/*40*/	{&sdcd,    	6,   0,	loc+240, 	0,	pv+41,	0  },
/*41*/	{&stcd,    	0,   0,	loc+246, 	0,	pv+35,	0  },
/*42*/	{&stcd,    	1,   0,	loc+252, 	0,	pv+36,	0  },
/*43*/	{&stcd,    	2,   0,	loc+258, 	0,	pv+37,	0  },
/*44*/	{&stcd,    	3,   0,	loc+264, 	0,	pv+38,	0  },
/*45*/	{&stcd,    	4,   0,	loc+270, 	0,	pv+39,	0  },
/*46*/	{&stcd,    	5,   0,	loc+276, 	0,	pv+40,	0  },
/*47*/	{&stcd,    	6,   0,	loc+282, 	0,	pv+41,	0  },
/*48*/	{&necd,    	0,   0,	loc+288, 	0,	pv+26,	0  },
/*49*/	{&necd,    	0,   0,	loc+294, 	0,	pv+26,	0  },
/*50*/	{&necd,    	0,   0,	loc+300, 	0,	pv+26,	0  },
/*51*/	{&eocd,    	0,   0,	loc+306, 	0,	pv+26,	0  },
/*52*/	{&epcd,    	0,   0,	loc+312, 	0,	pv+26,	0  },
/*53*/	{&epcd,    	0,   0,	loc+318, 	0,	pv+26,	0  },
/*54*/	{&tncd,    	0,   0,	loc+324, 	0,	pv+26,	0  },
/*55*/	{&tncd,    	0,   0,	loc+330, 	0,	pv+26,	0  },
/*56*/	{&tncd,    	0,   0,	loc+336, 	0,	pv+26,	0  },
/*57*/	{&tncd,    	0,   0,	loc+342, 	0,	pv+26,	0  },
/*58*/	{&wecd,    	0,   0,	loc+348, 	0,	pv+26,	0  },
/*59*/	{&wecd,    	0,   0,	loc+354, 	0,	pv+26,	0  },
/*60*/	{&wecd,    	0,   0,	loc+360, 	0,	pv+26,	0  },
/*61*/	{&wecd,    	0,   0,	loc+366, 	0,	pv+26,	0  },
/*62*/	{&wecd,    	0,   0,	loc+372, 	0,	pv+26,	0  },
/*63*/	{&wecd,    	0,   0,	loc+378, 	0,	pv+26,	0  },
/*64*/	{&wecd,    	0,   0,	loc+384, 	0,	pv+26,	0  },
/*65*/	{&wecd,    	0,   0,	loc+390, 	0,	pv+26,	0  },
/*66*/	{&wecd,    	0,   0,	loc+396, 	0,	pv+26,	0  },
/*67*/	{&wecd,    	0,   0,	loc+402, 	0,	pv+26,	0  },
/*68*/	{&wecd,    	0,   0,	loc+408, 	0,	pv+26,	0  },
/*69*/	{&wecd,    	0,   0,	loc+414, 	0,	pv+26,	0  },
/*70*/	{&wecd,    	0,   0,	loc+420, 	0,	pv+26,	0  },
/*71*/	{&wecd,    	0,   0,	loc+426, 	0,	pv+26,	0  },
/*72*/	{&wecd,    	0,   0,	loc+432, 	0,	pv+26,	0  },
/*73*/	{&wecd,    	0,   0,	loc+438, 	0,	pv+26,	0  },
/*74*/	{&wecd,    	0,   0,	loc+444, 	0,	pv+26,	0  },
/*75*/	{&wecd,    	0,   0,	loc+450, 	0,	pv+26,	0  },
/*76*/	{&efcd,    	0,   0,	loc+456, 	0,	pv+26,	0  },
/*77*/	{&elcd,    	0,   0,	loc+462, 	0,	pv+26,	0  },
/*78*/	{&excd,    	0,   0,	loc+468, 	0,	pv+26,	0  },
/*79*/	{&eahacd,  	0,   0,	loc+474, 	0,	pv+26,	0  },
/*80*/	{&tgcd,    	0,   0,	loc+480, 	0,	pv+24,	0  },
/*81*/	{&tgcd,    	1,   0,	loc+486, 	0,	pv+24,	0  },
/*82*/	{&tgcd,    	2,   0,	loc+492, 	0,	pv+24,	0  },
/*83*/	{&tgcd,    	3,   0,	loc+498, 	0,	pv+24,	0  },
/*84*/	{&tgcd,    	4,   0,	loc+504, 	0,	pv+24,	0  },
/*85*/	{&tgcd,    	5,   0,	loc+510, 	0,	pv+24,	0  },
/*86*/	{&tgcd,    	6,   0,	loc+516, 	0,	pv+24,	0  },
	0
};

