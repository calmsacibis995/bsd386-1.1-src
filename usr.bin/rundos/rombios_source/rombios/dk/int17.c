#include "rom.h"
#include "comarea.h"

/*******************************************************/

#define HI(x) (*(((byte _far *)(&x))+1))
#define LO(x) (*((byte _far *)(&x)))

#define _ah HI(_ax)
#define _al LO(_ax)

/*******************************************************/
#define MONITOR_SERV 0x8000

#define N_O_LPT 3
/*******************************************************/

typedef struct lptBUF {
	word max_i;
	word cur_i;
	word b_lock;
	byte bu [1018];		// 1018: for instance
} lptBUF;

/*******************************************************/
// Attention ! Real lines polarity on printer may differ from described below

// port 0x378 -- printer data latch (read/write
//   read: fetch last byte sent

// port 0x379 -- printer status (read only)   (...__ means -line)

#define P_ERROR__ 0x08		// 0 -- printer signals an error
#define P_SLCT    0x10		// 1 -- printer is selected
#define P_PAPER   0x20		// 1 -- out of paper
#define P_ACK__   0x40		// 0 -- ready for next character (12 mksec pulse)
#define P_BUSY__  0x80		// 0 -- busy or offline or error

#define P_STAT_MASK 0xF8	// mask off unused bits

// port 0x37A -- printer controls (read/write)   (...__ means -line)

#define P_STROBE__   0x01		// 0 -- send strobe ( >0.5 mksec pulse)
#define P_AUTO_LF    0x02		// 1 -- causes LF after CR
#define P_INIT__     0x04		// 0 -- resets the printer ( >50 mksec pulse)
#define P_SLCT_IN    0x08		// 1 -- selects the printer
#define P_IRQ_ENABLE 0x10		// 1 -- enables IRQ when -ACK goes false

#define P_STD_CTRL  (P_SLCT_IN | P_INIT__)  // standart control value

#define DATA_PORT (printer_base[_dx])
#define STAT_PORT (printer_base[_dx]+1)
#define CTRL_PORT (printer_base[_dx]+2)

#define INIT_DELAY 0x1000       // must be >50 microsec
#define TIME_OUT_CNT 0xFFFF   // number of inner loops to detect time out

/*******************************************************/
// int 17 interface
//
// (ah) = 0 -- print the character in al
//              on return, bit 0 of (ah) == 1 if character
//                         not be printed (time out)
//                         other bits set as on normal status call
// (ah) = 1 -- initialize the printer port
//              returns with (ah) set with printer status
// (ah) = 2 -- read the printer status into (ah)
//
//        bits:  7   6   5   4   3   2   1   0
//               |   |   |   |   |   |   |   |
//               |   |   |   |   |   |   |   |__ 1 = time out
//               |   |   |   |   |   |   |______ not used 
//               |   |   |   |   |   |__________ not used 
//               |   |   |   |   |______________ 1 = i/o error
//               |   |   |   |__________________ 1 = selected
//               |   |   |______________________ 1 = out of paper
//               |   |__________________________ 1 = acknoledge
//               |______________________________ 1 = not busy

/*******************************************************/

extern void LoadDS (void);
extern void Delay (int);
extern Wo int15 (byte, byte);

void MonitorServ (byte cod, int prn_num);

/*******************************************************/

void _interrupt _far _cdecl int17 ( word _es, word _ds, word _di, word _si,
												word _bp, word _sp, word _bx, word _dx,
												word _cx, word _ax, word _ip, word _cs,
												word flags )
{
	lptBUF _far * lb;

	if (_dx > 3) {
		_ah = 0x01;		// return "time out & NOT selected"
		return ;
	}

	switch (_ah) {
		case 0:			// print a char
			lb = common_in_SNG.adr_bufLPTx[_dx];
			lb->b_lock = 1;
			lb->bu[lb->cur_i++] = _al;
			if ( lb->cur_i == lb->max_i  ||  _al == 0x0A ) {
				lb->b_lock = 0;
				MonitorServ (0, _dx);			// transfer buffer
				lb->cur_i = 0;
			}
			else
				lb->b_lock = 0;
			_ah = P_SLCT;			// return "selected & busy"
			break;
		case 1:			// initialize printer
			MonitorServ (1, _dx);			// "initialize" printer
			_ah = P_SLCT;			// return "selected & busy"
			break;
		case 2:			// get a status
			_ah = (P_SLCT | P_BUSY__);			// return "selected & NOT busy"
			break;
	}
}

void MonitorServ (byte cod, int prn_num)
{
	_asm {
		mov	bx,prn_num
		mov	ah,cod
		mov	al,0x17
		mov	dx,MONITOR_SERV
		out	dx,al
	}
}

/*******************************************************/

void _interrupt _far _cdecl int14 ( word _es, word _ds, word _di, word _si,
												word _bp, word _sp, word _bx, word _dx,
												word _cx, word _ax, word _ip, word _cs,
												word flags )
{
	_ah = 0x80;		// time out !!!
}

