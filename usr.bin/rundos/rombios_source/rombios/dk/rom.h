/*******************************************************/

#ifndef BYTE_WORD_DEF
typedef unsigned char byte;
typedef unsigned int word;
typedef unsigned long dword;
typedef unsigned int uint;
typedef unsigned long ulong;
#define BYTE_WORD_DEF
#endif

typedef struct WoW {
	byte lo;
	byte hi;
	} WoW;

typedef union Wo {
	word x;
	WoW wo;
	} Wo;

#define l wo.lo
#define h wo.hi

typedef Wo * bptr;

#define FP_SEG(fp) ((word)(*((word _far *)&(fp) + 1)))
#define FP_OFF(fp) ((word)(*((word _far *)&(fp))))

/*******************************************************/

/* if no optimization only !!! */

/*
int _far _cdecl inp(unsigned);
unsigned _far _cdecl inpw(unsigned);
int _far _cdecl outp(unsigned, int);
unsigned _far _cdecl outpw(unsigned, unsigned);
void _cdecl _disable(void);
void _cdecl _enable(void);
*/

#pragma intrinsic(inp, inpw, outp, outpw, _disable, _enable)

/*******************************************************/

#define WBSSS (Wo _based (_segname("_STACK")) *)	// cast
#define WBSSS_D Wo _based (_segname("_STACK")) *	// declaration
#define BBSCS (byte _based (_segname("_CODE")) *)	// cast
#define BBSC_D byte _based (_segname("_CODE"))		// declaration
#define BSC_D _based (_segname("_CODE"))		// declaration

#define BY (byte)
#define BVS (void _based (void) *)
#define SG (_segment)
#define SEG_OFF(sg,off) ((SG (sg)) :> (BVS (off)))
/*******************************************************/

#define ZF_FLAG_MASK 0x40

/*******************************************************/

#define MONITOR_SERV 0x8000

/*******************************************************/

typedef struct SDATA {

	/* miscellaneous data 1 */

	word _rs232_base [4];
	word _printer_base [4];
	word _equip_flag;
	byte _mfg_tst;
	word _memory_size;
	byte _mfg_err[2];

	/* keyboard data */

	byte _kb_flag;
	byte _kb_flag_1;
	byte _alt_input;
	Wo * _buffer_head;
	Wo * _buffer_tail;
	Wo _kb_buffer [0x10];

	/* diskette data */

	byte _seek_status;
	byte _motor_status;
	byte _motor_count;
	byte _diskette_status;
	byte _nec_status [7];

	/* video data */

	byte _crt_mode;
	word _crt_cols;
	word _crt_len;
	word _crt_start;
	word _cursor_posn[8];
	word _cursor_mode;
	byte _active_page;
	word _addr_6845;
	byte _crt_mode_set;
	byte _crt_palette;

	/* miscellaneous data 2 */

	void ( _far * _io_rom_adr ) ();
	byte _intr_flag;

	/* timer data */

	word _timer_low;
	word _timer_high;
	byte _timer_ofl;

	/* miscellaneous data 3 */

	byte _bios_break;
	word _reset_flag;

	word _fixed_disk_data [2];

	byte _print_tim_out [4];
	byte _rs232_tim_out [4];

	/* additional keyboard data */

	Wo * _buffer_start;
	Wo * _buffer_end;

	/* EGA work area */

	byte _ega_rows;
	word _ega_points;

	byte _ega_info0;
	byte _ega_info1;
	byte _ega_info2;
	byte _ega_info3;

	/* additional media data */

	byte _lastrate;
	byte _hf_status;
	byte _hf_error;
	byte _hf_int_flag;
	byte _hf_cntrl;
	byte _dsk_state [4];
	byte _dsk_trk [2];

	/* additional keyboard flags */

	byte _kb_flag_3;
	byte _kb_flag_2;

	/* real time clock data area */

	byte _far * _user_rtc_flag;
	dword _rtc_cnt;
	byte _rtc_wait_flag;

	/* area for network adapter */

	byte _net_var [7];

	/* EGA misc. data pointer */

	byte _far * _far * _ega_save_ptr;

	/* reserved area */

	byte _reserv2 [0x3A];

	byte _kb_ec1849;

	byte _reserv3 [0x09];

	/* offset 0xF0 */

	byte _reserv4[0x10];

	/* print screen area */

	byte _prtsc_status;

	} SDATA;

/*******************************************************/

	/* miscellaneous data 1 */

#define rs232_base d._rs232_base
#define printer_base d._printer_base
#define equip_flag d._equip_flag
#define mfg_tst d._mfg_tst
#define memory_size d._memory_size
#define mfg_err d._mfg_err

	/* keyboard data */

#define kb_flag d._kb_flag
#define kb_flag_1 d._kb_flag_1
#define alt_input d._alt_input
#define buffer_head d._buffer_head
#define buffer_tail d._buffer_tail
#define kb_buffer d._kb_buffer

	/* diskette data */

#define seek_status d._seek_status
#define motor_status d._motor_status
#define motor_count d._motor_count
#define diskette_status d._diskette_status
#define nec_status d._nec_status

	/* video data */

#define crt_mode d._crt_mode
#define crt_cols d._crt_cols
#define crt_len d._crt_len
#define crt_start d._crt_start
#define cursor_posn d._cursor_posn
#define cursor_mode d._cursor_mode
#define active_page d._active_page
#define addr_6845 d._addr_6845
#define crt_mode_set d._crt_mode_set
#define crt_palette d._crt_palette

	/* miscellaneous data 2 */

#define io_rom_adr d._io_rom_adr
#define intr_flag d._intr_flag

	/* timer data */

#define timer_low d._timer_low
#define timer_high d._timer_high
#define timer_ofl d._timer_ofl

	/* miscellaneous data 3 */

#define bios_break d._bios_break
#define reset_flag d._reset_flag

#define fixed_disk_data d._fixed_disk_data

#define print_tim_out d._print_tim_out
#define rs232_tim_out d._rs232_tim_out

	/* additional keyboard data */

#define buffer_start d._buffer_start
#define buffer_end d._buffer_end

	/* EGA work area */

#define ega_rows d._ega_rows
#define ega_points d._ega_points

#define ega_info0 d._ega_info0
#define ega_info1 d._ega_info1
#define ega_info2 d._ega_info2
#define ega_info3 d._ega_info3

	/* additional media data */

#define lastrate d._lastrate
#define hf_status d._hf_status
#define hf_error d._hf_error
#define hf_int_flag d._hf_int_flag
#define hf_cntrl d._hf_cntrl
#define dsk_state d._dsk_state
#define dsk_trk d._dsk_trk

	/* additional keyboard flags */

#define kb_flag_3 d._kb_flag_3
#define kb_flag_2 d._kb_flag_2

	/* real time clock data area */

#define user_rtc_flag d._user_rtc_flag
#define rtc_cnt d._rtc_cnt
#define rtc_wait_flag d._rtc_wait_flag

	/* area for network adapter */

#define net_var d._net_var

	/* EGA misc. data pointer */

#define ega_save_ptr d._ega_save_ptr

	/* reserved area */

#define reserv2 d._reserv2

#define kb_ec1849 d._kb_ec1849

#define reserv3 d._reserv3

	/* print screen area */

#define prtsc_status d._prtsc_status

/*******************************************************/

extern SDATA d;

/*******************************************************/

