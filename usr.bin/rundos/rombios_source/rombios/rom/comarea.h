#ifndef BYTE_WORD_DEF
typedef unsigned char byte;
typedef unsigned int word;
typedef unsigned long dword;
typedef unsigned int uint;
typedef unsigned long ulong;
#define BYTE_WORD_DEF
#endif

struct COMMON_IN_SNG {

	dword	cli_flag;
	dword	pending_flag;

	dword	addr_link_fi13;		//	dd	err_code

	word skipped_ticks_lo;		// dword	require_tick;
	word skipped_ticks_hi;		// dword	require_tick;
	byte _far * addr_for_KB;
/***************** from monitor  ***/
	dword	num_printers;
	dword	num_COMs;
	dword	remote_terminal;
/***************** from monitor  ***/
	byte _far * menu_stack;
	void _far * menu_fun;			//	dd	_MESSAGES

	byte _far * aimseg0;				// dd	_beg_im_p0
	byte _far * aimseg40;			// dd	_beg_seg40
	byte _far * adr_bufLPTx[3];	// dd	lpt1buf,lpt2buf,lpt3buf
	word _far * int_10_parm;		// dd	0
	byte _far * reservv [8];		// dd 8 dup(0) -- reserved now
/***************** initial values  ***/
	dword	cs_ip;
	dword	ss_sp;
	dword	pg_0;
/***************** initial values  ***/
	dword	hdisk_table;
	dword	addr_ADDR_fl_PARAMS;

	byte _far * error_txt;
	dword begin_offs;
};

extern struct COMMON_IN_SNG _far common_in_SNG;



