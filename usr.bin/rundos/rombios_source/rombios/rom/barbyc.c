#include "comarea.h"

/***********************************************/
#ifndef BYTE_WORD_DEF
typedef unsigned char byte;
typedef unsigned int word;
typedef unsigned long dword;
typedef unsigned int uint;
typedef unsigned long ulong;
#define BYTE_WORD_DEF
#endif
/***********************************************/
/*
 * hard disk parameters:
 *	  structure definition and
 *	  description of 46 types
 */

typedef struct HD_PAR{				
	word hdrive_cyls;			
	byte hdrive_heads;		
	word pad;					
	word hdrive_precomp ;		
	byte pad1;				
	byte hdrive_contrl;		
	byte pad3[3];				
	word hdrive_lndng;			
	byte hdrive_sectors;		
	byte pad0;				
} HD_PAR;	
									
HD_PAR  hdisk_table[]= {
{ 306, 4,	0,128,0,0,0,0,0,	 305,17, 0},	//type 01   10M
{ 615, 4,	0,300,0,0,0,0,0,	 615,17, 0},	//type 02   20M
{ 615, 6,	0,300,0,0,0,0,0,	 615,17, 0},	//type 03   30M
{ 940, 8,	0,512,0,0,0,0,0,	 940,17, 0},	//type 04   62M
{ 940, 6,	0,512,0,0,0,0,0,	 940,17, 0},	//type 05   46M
{ 615, 4,	0,-1,0,0,0,0,0,		 615,17, 0},	//type 06   20M
{ 462, 8,	0,256,0,0,0,0,0,	 511,17, 0},	//type 07   30M
{ 733, 5,	0,-1,0,0,0,0,0,		 733,17, 0},	//type 08   30M
{ 900,15,	0,-1,0,8,0,0,0,		 901,17, 0},	//type 09   112M
{ 820, 3,	0,-1,0,0,0,0,0,		 820,17, 0},	//type 10   20M
{ 855, 5,	0,-1,0,0,0,0,0,		 855,17, 0},	//type 11   35M
{ 855, 7,	0,-1,0,0,0,0,0,		 855,17, 0},	//type 12   49M
{ 306, 8,	0,128,0,0,0,0,0,	 319,17, 0},	//type 13   20M
{ 733, 7,	0,-1,0,0,0,0,0,		 733,17, 0},	//type 14   42M
{  0,  0,	0,0,0,0,0,0,0,		    0,0, 0},	//type 15   not used
{ 612, 4,	0,0,0,0,0,0,0,		 663,17, 0},	//type 16   20M
{ 977, 5,	0,300,0,0,0,0,0,	 977,17, 0},	//type 17   40M
{ 977, 7,	0,-1,0,0,0,0,0,		 977,17, 0},	//type 18   56M
{1024, 7,	0,512,0,0,0,0,0,	1023,17, 0},	//type 19   59M
{ 733, 5,	0,300,0,0,0,0,0,	 732,17, 0},	//type 20   30M
{ 733, 7,	0,300,0,0,0,0,0,	 732,17, 0},	//type 21   42M
{ 733, 5,	0,300,0,0,0,0,0,	 733,17, 0},	//type 22   30M
{ 306, 4,	0,0,0,0,0,0,0,		 336,17, 0},	//type 23   10M
{ 925, 7,	0,0,0,0,0,0,0,		 925,17, 0},	//type 24   53M
//--------------------------------------------------------------
{ 925, 9,	0,-1,0,8,0,0,0,		 925,17, 0},	//type 25   69M
{ 754, 7,	0,754,0,0,0,0,0,	 754,17, 0},	//type 26   43M
{ 754,11,	0,-1,0,8,0,0,0,		 754,17, 0},	//type 27   68M
{ 699, 7,	0,256,0,0,0,0,0,	 699,17, 0},	//type 28   40M
{ 823,10,	0,-1,0,8,0,0,0,		 823,17, 0},	//type 29   68M
{ 918, 7,	0,918,0,0,0,0,0,	 918,17, 0},	//type 30   53M
{1024,11,	0,-1,0,8,0,0,0,		1024,17, 0},	//type 31   93M
{1024,15,	0,-1,0,8,0,0,0,		1024,17, 0},	//type 32   127M
{1024, 5,	0,1024,0,0,0,0,0,	1024,17, 0},	//type 33   42M
{ 612, 2,	0,128,0,0,0,0,0, 	 612,17, 0},	//type 34   10M
{1024, 9,	0,-1,0,8,0,0,0,		1024,17, 0},	//type 35   76M
{1024, 8,	0,512,0,0,0,0,0, 	1024,17, 0},	//type 36   68M
{ 615, 8,	0,128,0,0,0,0,0, 	 615,17, 0},	//type 37   40M
{ 987, 3,	0,987,0,0,0,0,0, 	 987,17, 0},	//type 38   24M
{ 987, 7,	0,987,0,0,0,0,0, 	 987,17, 0},	//type 39   57M
{ 820, 6,	0,820,0,0,0,0,0, 	 820,17, 0},	//type 40   40M
{ 977, 5,	0,977,0,0,0,0,0, 	 977,17, 0},	//type 41   40M
{ 981, 5,	0,981,0,0,0,0,0, 	 981,17, 0},	//type 42   40M
{ 830, 7,	0,512,0,0,0,0,0, 	 830,17, 0},	//type 43   48M
{ 830,10,	0,-1,0,8,0,0,0,		 830,17, 0},	//type 44   68M
{ 917,15,	0,-1,0,8,0,0,0,		 918,17, 0},	//type 45   114M
{1224,15,	0,-1,0,8,0,0,0,		1223,17, 0},	//type 46   152M
{   0, 0,	0,0,0,0,0,0,0,		   0, 0, 0},	//type 47   reserved
};	// end of table

/***********************************************/

byte *floppy_types[] = {
	"No Drive",
	"5\" 360k",
	"5\" 1.2M",
	"3\" 720k",
	"3\" 1.44M",
	"Unknown", /* "2.88M could go here if implemented */
	"Unknown",
	"Unknown",
};

byte * display_list[] = {
	"VGA",
	"CGA 40 x 25",
	"CGA 80 x 25",
	"Monochrome"
};

/***********************************************/

#define EQUIP_BITS (*((byte _far *) 0x410))
#define EQUIP_BITS2 (*((byte _far *) 0x411))
#define BasePrPORTS ((word _far *) 0x408)
#define BaseSrPORTS ((word _far *) 0x400)
#define HDSK_COUNT (*((byte _far *) 0x475))
#define TIMER_LONG (*((ulong _far *) 0x46c))

#define INT_41h (*((void _far * _far *) (0x41*4)))
#define INT_46h (*((void _far * _far *) (0x46*4)))
#define INT_1Eh (*((void _far * _far *) (0x1E*4)))

/***********************************************/

static byte scratchm[] = { 0, 0, 0, 0, 0, '\r', '\n', 0};

byte CMOS_equimp;
byte CMOS_HDs;
byte CMOS_HD0;
byte CMOS_HD1;
byte CMOS_FL01;
byte CMOS_FL23;

extern byte _far FL_types[4];
extern byte _far numFLs;

byte numHDs;
byte HD_types[2] = { 0, 0 };

/***********************************************/
/**************** SUBROUTINES ******************/

void tto (byte sym)			// write in TTY-mode to display
{
	_asm {
		mov	ah,0x0E
		mov	al,sym
		mov	bx,0
		int	0x10
	}
}

void printl (byte *addr)	// output string to display
{
	while (*addr != 0) {
		tto (*addr);
		addr++;
	}
}



void b2d (word num)		  // convert to dec
{
	int i;

	for ( i=4 ; i>=0 ; i-- ) {
		scratchm[i] = (num % 10) + '0';
		num /= 10;
	}
}

void b2h (word num)			// convert to hex
{
	int i;

	for ( i=3 ; i>=0 ; i-- ) {
		scratchm[i] = (num & 0x0F) + '0';
		if (scratchm[i] > '9')
			scratchm[i] += ('A' - 1) - '9';
		num >>= 4;
	}
	scratchm[4] = 'h';
}

void b2dex (word num, word pos, word skip)
{
	word i;

	b2d (num);
	scratchm[5] = 0;
	printl (scratchm+pos);
	for ( i=0 ; i<skip ; i++ )
		tto (' ');
}

word bcd_bin (byte bcd_dig)
{
	return ( 10 * ((bcd_dig>>4) & 0x0f) + (bcd_dig & 0x0f) );
}

/***********************************************/

byte Read_CMOS (byte address)
{
	byte i;

	_disable ();
	outp (0x70, address | 0x80);
	_asm {jmp $+2}
	_asm {jmp $+2}
	i = inp (0x71);
	_enable ();
	return i;
}

/***********************************************/
/************ DISPLAY CONFIGURATION ************/

void display_mem (void)
{
	printl ("\r\nBase Memory:     640K");
	printl ("\r\nExtended Memory:   0K\r\n");
}

void display_video (void)
{
	printl ("\r\nVideo: ");
	printl ( display_list[(CMOS_equimp >> 4) & 0x03] );
}

/***********************************************/

word Pports[] = { 0x378, 0x278, 0x3BC };
word Sports[] = { 0x3F8, 0x2F8, 0x3E8, 0x2E8 };

void display_adapters (void)
{
	word i;

	printl ("\r\nParallel ports:  (");
	b2dex ( (word)common_in_SNG.num_printers, 4, 0);
	printl (")   ");
	for ( i=0 ; i<3 ; i++ ) {
		if (common_in_SNG.adr_bufLPTx[i] != 0) {
			b2h (Pports[i]);
			printl (scratchm+1);
			printl("   ");
			*(BasePrPORTS+i) = Pports[i];
		}
		else
			*(BasePrPORTS+i) = 0;
	}

	printl ("\r\nSerial ports:    (");
	b2dex ( (word)common_in_SNG.num_COMs, 4, 0);
	printl (")   ");
	for ( i=0 ; i<common_in_SNG.num_COMs ; i++ ) {
		b2h (Sports[i]);
		printl (scratchm+1);
		printl ("   ");
		*(BaseSrPORTS+i) = Sports[i];
	}
	EQUIP_BITS2 = (common_in_SNG.num_printers<<6)+(common_in_SNG.num_COMs<<1);
}

/***********************************************/

byte about_floppies (byte _far *flt)
{
	if ( (CMOS_equimp & 0x01) != 0 ) {
		flt[0] = CMOS_FL01 >> 4;
		flt[1] = CMOS_FL01 & 0x0f;
	//????????????????????????????????????
		flt[2] = CMOS_FL23 >> 4;
		flt[3] = CMOS_FL23 & 0x0f;
	//????????????????????????????????????
		return ((CMOS_equimp >> 6) & 0x03) + 1;
	}
	else
		return (0);
}

byte about_hards (byte _far *hdt)
{
	if ( (CMOS_HDs >> 4) == 0x0F )
		hdt[0] =	CMOS_HD0;
	else
		hdt[0] = CMOS_HDs >> 4;

	if ( (CMOS_HDs & 0x0F) == 0x0F )
		hdt[1] =	CMOS_HD1;
	else
		hdt[1] = CMOS_HDs & 0x0F;

	if (hdt[0] == 0)
		return 0;
	else
		if (hdt[1] == 0) 
			return 1;
		else
			return 2;
}

void disply_dsks_conf (void)
{
	byte i;

	for ( i=0 ; i<numFLs ; i++ ) {
		printl ("\r\nFloppy Disk ");
		tto ('0'+i);
		printl(":  ");
		printl (floppy_types[FL_types[i]]);
	}
	printl("\r\n");
	for ( i=0 ; i<numHDs ; i++ ) {
		HD_PAR *p = & hdisk_table[HD_types[i]-1];
		word tmp  =	(p->hdrive_cyls * p->hdrive_heads)/2048 ; //Sector = 1/2 K
		word tmpr =	(p->hdrive_cyls * p->hdrive_heads)%2048 ; 
		word dskmemM =	tmp * p->hdrive_sectors +
						(tmpr * p->hdrive_sectors)/2048;  //Size in Megabytes
		printl ("\r\nHard Disk   ");
		tto ('0'+i);
		printl (":  Type ");
		b2dex (HD_types[i],3,4);
		printl("(");
		b2dex (dskmemM,2,0);
		printl("M)");

	}
}

/***********************************************/

/* set the timer counter on boot up */

#define ERR 1
#define OK 0

#define SECONDS 0
#define MINUTES 2
#define HOURS 4

#define ticks_per_sec 18
#define ticks_per_min 1092
#define ticks_per_hr 60 * ticks_per_min		//0x10000 - 0x10

word set_count (void)
{
	byte temp;
	ulong count;
	ulong long_temp;

	if ((temp = Read_CMOS (HOURS)) > 0x23)
		return ERR;
	temp = bcd_bin (temp);
	count = ((ulong)temp <<16) + ((word)temp * 22)/3;

	if ((temp = Read_CMOS (MINUTES)) > 0x59)
		return ERR;
	temp = bcd_bin (temp);
	count += (ulong)((word)(temp) * ticks_per_min);

	if ((temp = Read_CMOS (SECONDS)) > 0x59)
		return ERR;
	temp = bcd_bin (temp);
	count += (ulong) ((word)(temp) * ticks_per_sec + (word) temp / 5);

	TIMER_LONG = count ;
	return OK;
}

/***********************************************/

void main()
{
	word i;

	CMOS_equimp = Read_CMOS(0x14);
	CMOS_HDs = Read_CMOS(0x12);
	CMOS_HD0 = Read_CMOS(0x19);
	CMOS_HD1 = Read_CMOS(0x1a);
	CMOS_FL01 = Read_CMOS(0x10);
	CMOS_FL23 = Read_CMOS(0x11);

	for ( i=0 ; i<12 ; i++ )
		printl ("\r\n\r\n\r\n");
	printl ("\r\n***  BIOS for RUNDOS under BSD/386  ***\r\n");
	printl ("\r\n***    Designed in Moscow, 1992     ***\r\n");
	printl ("\r\nVersion: ");
	printl (__DATE__);
	printl ("    ");
	printl (__TIME__);
	printl ("\r\n");

	display_mem ();
	display_video ();
	display_adapters ();
	printl ("\r\n");

	numFLs = about_floppies (FL_types);
	numHDs = about_hards (HD_types);
	disply_dsks_conf ();
	HDSK_COUNT = numHDs;
	EQUIP_BITS = CMOS_equimp;
	INT_41h = & hdisk_table[HD_types[0]-1];
	INT_46h = & hdisk_table[HD_types[1]-1];

	if ( set_count () != OK )
		printl ("\r\nError in CMOS\r\n");
	printl("\r\n\r\n\r\n");
}

/***********************************************/
