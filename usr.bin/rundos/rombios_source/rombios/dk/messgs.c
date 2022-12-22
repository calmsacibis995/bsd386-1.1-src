#include "rom.h"
#include "comarea.h"

/*******************************************************/

#define MY_NULL ((void _far *) 0L)

/*******************************************************/

#define B_BGND 0x00
#define ERR_BGND 0x04
#define NORM_BGND 0x02

#define B_FGND 0x0E
#define ERR_FGND 0x0F
#define NORM_FGND 0x0F

#define B_ATR ((B_BGND<<4) | B_FGND)
#define ERR_ATR ((ERR_BGND<<4) | ERR_FGND)
#define NORM_ATR ((NORM_BGND<<4) | NORM_FGND)

/*******************************************************/

BBSC_D line_ch [] = {
	0xCD,		// HORIZ_LI
	0xBA,		// VERT_LI
	0xC9,		// UL_CORN
	0xBB,		// UR_CORN
	0xC8,		// LL_CORN
	0xBC		// LR_CORN
};

#define HORIZ_LI	line_ch[0]
#define VERT_LI	line_ch[1]
#define TOP_CORN_INDEX	2
#define BOTT_CORN_INDEX	4

#define SAVE 1
#define RESTORE 2
#define VGA_STAT 7		// video hardware + BIOS data areas + DACs
/*******************************************************/

BBSC_D cont_msg [] =        " ESC or ENTER - continue  ";
BBSC_D quit_msg [] =        " Q - Quit DOS at all      ";
BBSC_D restart_msg [] =     " R - Restart DOS (reboot) ";
BBSC_D suspend_msg [] =     " S - Suspend DOS (sleep)  ";
BBSC_D flush_prn_msg [] =   " P - flush Printer buffer ";
BBSC_D timer_msg [] =       " T <0-1> - set timer mode ";
BBSC_D sound_msg [] =       " M - Music (sound) on/off ";

byte _far * (BSC_D msgs) [] = {
	cont_msg,
	quit_msg,
	restart_msg,
	suspend_msg,
	flush_prn_msg,
	timer_msg,
	sound_msg,
	MY_NULL
};

BBSC_D pal_reg_buf [0x11];

/*******************************************************/

#include "msgfunc.def"

/*******************************************************/

void _interrupt _far _cdecl Messages ( word _es, word _ds, word _di, word _si,
												word _bp, word _sp, word _bx, word _dx,
												word _cx, word _ax, word _ip, word _cs,
												word flags )
{
	byte start_col, start_row;
	int j, k, len;
	int num_of_msgs;
	int cols;			// must be got from monitor
	byte page = 0;		// must be got from monitor
	byte mode;

	// _ax - action: 0 -- output menu only
	//               1 -- print error message also
	//                     (common_in_SNG->error_txt -- text)
	//               60 -- return VGA state buffer size (BX)
	//               6E -- save VGA state (_es:_bx -- buffer)
	//               6F -- restore VGA state (_es:_bx -- buffer)

	if (_ax == 0x6E) {		// save state
		SaveRestVGAState (SAVE, _bx, _es);
		outp (MONITOR_SERV, 0xF0);
	}
	if (_ax == 0x6F) {		// restore state
		SaveRestVGAState (RESTORE, _bx, _es);
		outp (MONITOR_SERV, 0xF0);
	}
	if (_ax == 0x60) {		// info about save state
		InfoVGAState ();		// returns itself
	}

	SetDSCS ();

	mode = *((byte _far *)SEG_OFF(0x40,0x49));
/*	if (SetVMode (mode | 0x80))		// non-0 if failed to set mode
		SetMode (3);
*/

//	cols = (_dx > 60) ? 80 : 40;
	cols = *((word _far *)SEG_OFF(0x40,0x4A));
	page = *((byte _far *)SEG_OFF(0x40,0x62));
	GetPaletteRegs (pal_reg_buf);

	if (_ax == 1) {
		len = StrLen (common_in_SNG.error_txt);
		k = (len / (cols-2)) + 1;		// number of lines for message
		if (k > 1) {
			for ( j=0 ; j < ((len+k-1)/k)*k - len ; ++j )
				common_in_SNG.error_txt[len+j] = ' ';
			common_in_SNG.error_txt[len+j] = '\0';
		}
		len = StrLen (common_in_SNG.error_txt) / k;
		start_col = (cols - len) / 2;
		start_row = 5;
		WriteBorder (TOP_CORN_INDEX, start_row, start_col, page, len);
		for ( j=0 ; j<k ; ++j ) {
			SetCursor (++start_row, start_col, page);
			WriteChar (VERT_LI, B_ATR, page, 1);
			WriteString (start_row, start_col+1, page, ERR_ATR, len,
							common_in_SNG.error_txt + j*len);
			WriteChar (VERT_LI, B_ATR, page, 1);
		}
		WriteBorder (BOTT_CORN_INDEX, ++start_row, start_col, page, len);
	}
	len = StrLen (msgs[0]);
	num_of_msgs = 0;
	while ( msgs[num_of_msgs] != MY_NULL )
		++num_of_msgs;
	start_col = (cols - len) / 2;
	start_row = (_ax == 1) ?  start_row+3 : 8;

	WriteBorder (TOP_CORN_INDEX, start_row, start_col, page, len);
	for ( j=0 ; j<num_of_msgs ; ++j ) {
		SetCursor (++start_row, start_col, page);
		WriteChar (VERT_LI, B_ATR, page, 1);
		WriteString (start_row, start_col+1, page, NORM_ATR, len, msgs[j]);
		WriteChar (VERT_LI, B_ATR, page, 1);
	}
	WriteBorder (BOTT_CORN_INDEX, ++start_row, start_col, page, len);
	HideCursor ();

	outp (MONITOR_SERV, 0x6D);
}

void SetDSCS ()
{
	_asm {
		push	cs
		pop	ds
	}
}

int StrLen (char _far * str)
{
	int j = 0;

	while ( str[j] != 0 )
		++j;
	return j;
}

void WriteBorder (int ch_index, byte row, byte column, byte page, int len)
{
	SetCursor (row, column, page);
	WriteChar (line_ch[ch_index], B_ATR, page, 1);
	SetCursor (row, ++column, page);
	WriteChar (HORIZ_LI, B_ATR, page, len);
	column += len;
	SetCursor (row, column, page);
	WriteChar (line_ch[ch_index+1], B_ATR, page, 1);
}

void WriteString (byte row, byte col, byte page, byte t_atr,
						int len, char _far * msg)
{
	_asm {
		mov	bl,t_atr
		mov	bh,page
		mov	dh,row
		mov	dl,col
		mov	cx,len
		mov	ax,0x1301
		push	bp
		les	bp,msg
		int	0x10
		pop	bp
	}
}

void WriteChar (byte c, byte attrib, byte page, int cnt)
{
	_asm {
		mov	bh,page
		mov	bl,attrib
		mov	cx,cnt
		mov	al,c
		mov	ah,0x09
		int	0x10
	}
}

void SetCursor (byte row, byte column, byte page)
{
	_asm {
		mov	dh,row
		mov	dl,column
		mov	bh,page
		mov	ah,0x02
		int	0x10
	}
}

void HideCursor (void)
{
	_asm {
		mov	cx,0x2000
		mov	ah,1
		int	0x10
	}
}

WoW ReadChar (byte page)
{
	_asm {
		mov	bh,page
		mov	ah,0x08
		int	0x10
	}
}

int SetVMode (byte mo)			// returns 0 if success
{
	_asm {
		mov	ah,0
		mov	al,mo
		int	0x10
		push	0x40
		pop	es
		mov	al,es:(0x49)
		mov	ah,mo
		and	ax,0x7F7F
		xor	al,ah
		cbw
	}
}

void SetPaletteRegs (byte far * reg_buf)
{
	_asm {
		les	dx,reg_buf
		mov	ax,0x1002
		int	0x10
	}
}

void GetPaletteRegs (byte far * reg_buf)
{
	_asm {
		les	dx,reg_buf
		mov	ax,0x1008
		int	0x10
	}
}

void InfoVGAState (void)
{
	_asm {
		mov	ax,0x1C00
		mov	cx,VGA_STAT		// video hardware + BIOS data areas + DACs
		int	0x10
		cmp	al,0x1C
		jz		info_ret
		mov	bx,0
	info_ret:
		mov	dx,MONITOR_SERV
		mov	al,0xF0
		out	dx,al			// return to monitor, BX = number of 64-byte blocks
	}
}

void _far InfoVGAInit (void)
{
	_asm {
		mov	ax,0x1C00
		mov	cx,VGA_STAT		// video hardware + BIOS data areas + DACs
		int	0x10
		cmp	al,0x1C
		jz		info_ret
		mov	bx,0
	info_ret:
		mov	dx,MONITOR_SERV
		mov	al,0xF1
		out	dx,al			// return to monitor, BX = number of 64-byte blocks
	}
}

void SaveRestVGAState (byte save_rest, word _bx, word _es)
{
	_asm {
		mov	ah,0x1C
		mov	al,save_rest
		mov	es,_es
		mov	bx,_bx
		mov	cx,VGA_STAT		// video hardware + BIOS data areas + DACs
		int	0x10
	}
}

/*******************************************************/
/*
 * "Old" INT 10 handler,
 *   pointed by INT 42 after VGA BIOS initializes.
 * May be used by accident, or by VGA BIOS in some rare cases, or
 *   by crazy programs referencing address F000:F065 directly.
 * It will proceed remote terminal i/o (in future).
 */

void _interrupt _far _cdecl int10 ( word _es, word _ds, word _di, word _si,
												word _bp, word _sp, word _bx, word _dx,
												word _cx, word _ax, word _ip, word _cs,
												word flags )
{
	common_in_SNG.int_10_parm = (word _far *) (& _ax);
	outp (MONITOR_SERV, 0x10);
}

/*******************************************************/

void PrintChar (byte c)
{
	_asm {
		mov	al,c
		mov	dx,0
		int	0x17
	}
}

void _interrupt _far _cdecl int5 ( word _es, word _ds, word _di, word _si,
												word _bp, word _sp, word _bx, word _dx,
												word _cx, word _ax, word _ip, word _cs,
												word flags )
{
	int i, j;
	word save_curs_pos;
	WoW c;

	LoadDS ();		// DS = 0x40
	if (prtsc_status)
		return ;
	prtsc_status = 1;
	_enable ();
	
	save_curs_pos = cursor_posn[active_page];

	PrintChar ('\r');
	PrintChar ('\n');
	for ( i=0 ; i<=ega_rows ; i++ ) {
		for ( j=0 ; j<crt_cols ; j++ ) {
			SetCursor (i, j, active_page);
			c = ReadChar (active_page);
			PrintChar (c.lo);
		}
		PrintChar ('\r');
		PrintChar ('\n');
	}
	cursor_posn[active_page] = save_curs_pos;
	prtsc_status = 0;
	return ;

prt_err:
	prtsc_status = 0xFF;
	return ;

}


