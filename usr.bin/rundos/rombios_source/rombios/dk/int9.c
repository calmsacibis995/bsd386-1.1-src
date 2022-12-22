#include "rom.h"
#include "kb.h"
#include "kbtab.h"
#include "int9func.def"

/*******************************************************/

#ifdef BSD

// mov dx,0FFFFh
// out dx,al

#define jmp_to_reset _asm _emit 0xBA \
							_asm _emit 0xFF \
							_asm _emit 0xFF \
							_asm _emit 0xEE

#define call_sub_E88E _asm _emit 0xBA \
							_asm _emit 0xFF \
							_asm _emit 0xFF \
							_asm _emit 0xEE

#else

// jmp FFFF:0000

#define jmp_to_reset _asm _emit 0xEA \
							_asm _emit 0x00 \
							_asm _emit 0x00 \
							_asm _emit 0xFF \
							_asm _emit 0xFF

#define call_sub_E88E _asm _emit 0xEA \
							_asm _emit 0x00 \
							_asm _emit 0x00 \
							_asm _emit 0xFF \
							_asm _emit 0xFF

#endif

/*******************************************************/

void _interrupt _far _cdecl int9 ()
{
	uint rs;

	LoadDS ();
	_enable ();
	rs = KBhandler ( DisableKeyb_InChar () );
	if ( rs <= 0 )
		kb_flag_3 &= ~(LAST_E0_MASK | LAST_E1_MASK);
	if ( rs <= 1 ) {
		_disable ();
		SendEOI ();
	}
#ifdef BSD
	if ( rs <= 2 ) {
		_disable ();
		EnableKeyboard ();
	}
#else
	if ( rs <= 2 )
		EnableKeyboard ();
	if ( rs <= 3 )
		_disable ();
#endif
	return ;
}

uint KBhandler (byte c)
{
	Wo a;

	a = int15 (0x4F, c);		// int15 returns CF in a.h !!!
	_disable ();
	if ( ! a.h )
		return 0;
	else
		c = a.l;
	if ( c == KB_RESEND ) {
		kb_flag_2 |= LAST_RESEND_MASK;
		return 0;
	}
	if ( c == KB_ACK ) {
		kb_flag_2 |= LAST_ACK_MASK;
		return 0;
	}
	_enable ();
	a = CheckLocksLED ();
	if ( a.h )
		CheckSetLED_1 ();
	if ( c == KB_OVERRUN  ||  c == KB_SELF_TEST_FAIL_2 ) {
		Beep ();
		return 2;
	}
	if ( kb_flag_3 & (RD_ID_MASK | ID_1ST_MASK) ) {
		ReadID (c);
		return 0;
	}
	if ( c == MCOD_E0 ) {
		kb_flag_3 |= (LAST_E0_MASK | ENH_KB_MASK);
		return 1;
	}
	if ( c == MCOD_E1 ) {
		kb_flag_3 |= (LAST_E1_MASK | ENH_KB_MASK);
		return 1;
	}
	if ( c == KB_OVERRUN_2 )
		return 0;
	if ( IsDepressing (c) )
		return Depressing ( PureCod (c) );
	else
		return Pressing (c);
}

/*******************************************************/
void ReadID (c)
byte c;
{
	if ( kb_flag_3 & RD_ID_MASK ) {
		kb_flag_3 &= ~RD_ID_MASK;
		if ( c == KB_ID_1ST )
			kb_flag_3 |= ID_1ST_MASK;
		return ;
	}
	else {
		kb_flag_3 &= ~ID_1ST_MASK;
		if ( c == KB_ID_2ND_2  ||  c == KB_ID_2ND_3 ) {
			if ( c == KB_ID_2ND_3  &&  (kb_flag_3 & FORCE_NUMLC_MASK) ) {
				kb_flag |= NUM_ST_MASK;
				CheckSetLED_1 ();		// ???? may be CheckSetLED_2 ?????
			}
			kb_flag_3 |= ENH_KB_MASK;
		}
		return ;
	}
}

/*******************************************************/

uint Depressing (c)
byte c;
{
	Wo a;
	byte m;

	if ( CheckLastE1 () ) {
		if ( IsSuperShift (c) )
			return 1;
		else
			return 0;
	}
	if ( CheckLastE0 ()  &&  IsPlaneShift (c) )
		return 0;
	if ( (m = IsLock (c))  ||  ((m = INS_PR_MASK)  &&  c == MCOD_INS) ) {
		kb_flag_1 &= ~m;
		return 0;
	}
	if ( c == MCOD_SYS_REQ ) {
		kb_flag_1 &= ~SYS_REQ_MASK;
		return SendSysReq (1);
	}
	if ( m = IsSuperShift (c) ) {
		m = ~m;
		kb_flag &= m;
		if ( IsPlaneShift (c) )
			return 0;
		if ( CheckLastE0 () )
			kb_flag_3 &= m;
		else
			kb_flag_1 &= (m >> 2);
		kb_flag |= ( (kb_flag_1 << 2) | kb_flag_3 ) &
							(ALT_ST_MASK | CTRL_ST_MASK);
		if ( c != MCOD_ALT )
			return 0;
		else {
			if ( alt_input != 0 ) {
				a.l = alt_input;
				a.h = 0;
				alt_input = 0;
				return PutChar (a);
			}
		}
	}
	return 0;
}

/*******************************************************/

uint Pressing (c)
byte c;
{
	byte m;

	if ( CheckLastE1 () ) {
		if ( IsSuperShift (c) )
			return 1;
		if ( c == MCOD_NUM_LOCK  &&  ! (kb_flag_1 & HOLD_STATE_MASK) ) {
			return HoldState ();
		}
		else
			return 0;
	}
	if ( CheckLastE0 ()  &&  IsPlaneShift (c) )
		return 0;
	if ( m = IsSuperShift (c) ) {
		kb_flag |= m;
		if ( IsPlaneShift (c) )
			return 0;
		if ( CheckLastE0 () )
			kb_flag_3 |= m;
		else
			kb_flag_1 |= (m >> 2);
		return 0;
	}
	if ( c > MCOD_MAXIMUM )
		return 0;
	if ( c == MCOD_SYS_REQ ) {
		if ( kb_flag_1 & SYS_REQ_MASK )
			return 0;
		kb_flag_1 |= SYS_REQ_MASK;
		return SendSysReq (0);
	}
	if ( (kb_flag & ALT_ST_MASK)  &&
			! (CheckKBx ()  &&  (kb_flag_1 & SYS_REQ_MASK)) ) {
		if ( kb_flag & CTRL_ST_MASK )
			CtrlAlt (c);
		else {
			if ( ProcLocks (c) )		// if ProcLocks do evrything itself
				return 0;
		}
		return AltState (c);
	}
	if ( kb_flag & CTRL_ST_MASK )
		return CtrlState (c);
	else
		return PlaneState (c);
}

/*******************************************************/
uint AltState (c)
byte c;
{
	Wo a;

	if ( CheckHoldState (c) )		// if CheckHoldState do evrything itself
		return 0;
	a.l = c;
	a = XlatChar (a, d_99E8);
	if ( a.l == 0xFF )
		return 0;
	if ( CheckLastE0 () ) {
		alt_input = 0;
		if ( a.h == MCOD_ENTER )
			a.l = SCAN_GRAY_ENTER;
		else {
			if ( a.h == MCOD_SLASH )
				a.l = SCAN_GRAY_SLASH;
			else {
				a.l = a.h;
				a.l += SCAN_INCREMENT;
			}
		}
	}
	else {
		if ( a.l < ALT_INPUT_LIM ) {
			alt_input = (alt_input * 10) + a.l;
			return 0;
		}
		else {
			alt_input = 0;
			if ( a.h == MCOD_DEL )
				return 0;
			if ( a.h == MCOD_SPACE  ||  a.l == HID_COD_F0 )
				return PutChar (a);
		}
	}
	return PutPureScan (a);
}

/*******************************************************/
uint CtrlState (c)
byte c;
{
	Wo a;

	if ( CheckHoldState (c) )		// if CheckHoldState do evrything itself
		return 0;
	a.l = c;
	a = XlatChar (a, d_9A68);
	if ( a.l == 0xFF )
		return 0;
	if ( a.h == MCOD_NUM_LOCK ) {
		if ( CheckKBx () )
			return 0;
		else
			return HoldState ();
	}
	if ( a.h == MCOD_SCRL_LOCK ) {
		if ( CheckKBx ()  &&  ! CheckLastE0 () )
			return 0;
		CtrlBreak ();
		a.l = a.h = 0;
		return PutChar (a);
	}
	if ( a.h == MCOD_STAR ) {
		if ( ! CheckKBx ()  ||  CheckLastE0 () )
			a.l = CTRL_PRT_SC_COD;
		return PutPureScan (a);
	}
	if ( CheckLastE0 () ) {
		if ( a.h == MCOD_SLASH )
			return PutPureScan (a);
		else {
			if ( a.h == MCOD_ENTER ) {
				a.h = HID_COD_E0;
				return PutChar (a);
			}
			else {
				a.h = a.l;
				a.l = HID_COD_E0;
				return PutChar (a);
			}
		}
	}
	else {
		if ( a.h == MCOD_TAB )
			return PutPureScan (a);
		if ( a.h == MCOD_SPACE )
			return PutChar (a);
		if ( a.h == MCOD_SLASH )
			return 0;
		if ( a.h < MCOD_ALPHA_LIMIT )
			return PutChar (a);
		else
			return PutPureScan (a);
	}
}

/*******************************************************/

uint PlaneState (c)
byte c;
{
	Wo a;
	BBSC_D * b;

	if ( ProcLocks (c) )		// if ProcLocks do evrything itself
		return 0;
	if ( c != MCOD_INS ) {
		if ( CheckHoldState (c) )		// if CheckHoldState do evrything itself
			return 0;
	}
	if ( c == MCOD_STAR  &&
			( (CheckKBx () && CheckLastE0 ()) ||
				(! CheckKBx () && (kb_flag & SHFT_ST_MASK)) ) ) {
		return PrintScreen ();
	}
	a.l = c;
	if ( CheckLastE0 () ) {
		if ( a.l == MCOD_ENTER ) {
			a.l = ASCII_ENTER;
			a.h = HID_COD_E0;
			return PutChar (a);
		}
		else {
			if ( a.l == MCOD_SLASH ) {
				a.l = ASCII_SLASH;
				a.h = HID_COD_E0;
				return PutChar (a);
			}
			else {
				if ( ProcIns (a.l) )		// if ProcIns do evrything itself
					return 0;
				b = d_9B68;
			}
		}
	}
	else {
		if ( kb_flag & SHFT_ST_MASK ) {
			b = d_9AE8;
			if ( kb_flag & NUM_ST_MASK ) {
				if ( ProcIns (a.l) )		// if ProcIns do evrything itself
					return 0;
				if ( a.l > MCOD_LEFT_PART_LIMIT  &&  a.l < MCOD_LEFT_PART_LIMIT_1 )
					b = d_9B68;
			}
		}
		else {
			b = d_9B68;
			if ( kb_flag & NUM_ST_MASK ) {
				if ( a.l > MCOD_LEFT_PART_LIMIT  &&  a.l < MCOD_LEFT_PART_LIMIT_1 )
					b = d_9AE8;
			}
			else {
				if ( ProcIns (a.l) )		// if ProcIns do evrything itself
					return 0;
			}
		}
	}
	a = XlatChar (a, b);
	if ( a.l == 0xFF )
		return 0;
	if ( a.h > SCAN_ALPHA_LIMIT  &&  a.l > ASCII_UNKNWN_LIMIT  &&
			a.l != HID_COD_F0  &&  a.h != SCAN_102_KEY ) {
		a.h = a.l;
		a.l = 0;
		if ( CheckLastE0 () )
			a.l = HID_COD_E0;
	}
	return ProcCapsLock (a);
}

/*******************************************************/

uint PutPureScan (Wo a)
{
	a.h = a.l;
	a.l = 0;
	return PutChar (a);
}

/*******************************************************/

byte ProcLocks (byte c)			// returns ~0 if function do evrything itself
{
	byte m;

	if ( m = IsLock (c) ) {
		if ( ! (kb_flag_1 & m) ) {
			kb_flag_1 |= m;
			kb_flag ^= m;
			CheckSetLED_1 ();
		}
		return ~0;
	}
	return 0;
}

byte CheckHoldState (byte c)		// returns ~0 if function do evrything itself
{
	if ( kb_flag_1 & HOLD_STATE_MASK ) {
		if ( c != MCOD_NUM_LOCK )
			kb_flag_1 &= ~HOLD_STATE_MASK;
		return ~0;
	}
	return 0;
}

Wo XlatChar (Wo a, BBSC_D * table)
{
	a.h = a.l;
	a.l = table [a.l - 1];
	return a;
}

byte ProcIns (byte c)			// returns ~0 if function do evrything itself
{
	if ( c == MCOD_INS )
		if ( kb_flag_1 & INS_PR_MASK )
			return ~0;
		else {
			kb_flag_1 |= INS_PR_MASK;
			kb_flag ^= INS_PR_MASK;
		}
	return 0;
}

byte IsLock (byte c)
{
	if ( c == MCOD_CAPS_LOCK )
		 return CAPS_PR_MASK;
	else
		if ( c == MCOD_NUM_LOCK )
			return NUM_PR_MASK;
		else
			if ( c == MCOD_SCRL_LOCK )
				return SCRL_PR_MASK;
			else
				return 0;
}

byte IsSuperShift (byte c)
{
	if ( c == MCOD_ALT )
		 return ALT_ST_MASK;
	 else
		if ( c == MCOD_CTRL )
			 return CTRL_ST_MASK;
		 else
			 return IsPlaneShift (c);

}

byte IsPlaneShift (byte c)
{
	if ( c == MCOD_LSHFT )
		 return L_SHFT_MASK;
	 else
		if ( c == MCOD_RSHFT )
			 return R_SHFT_MASK;
		 else
			 return 0;
}

void EnableKeyboard (void)
{
#ifdef BSD
	outp ( KB_PORT_CTRL, KB_CTL_ENABLE);
#else
	Wait8042free ();
	outp ( KB_PORT_CTRL, KB_CTL_ENABLE);
	_enable ();
#endif
}

byte DisableKeyb_InChar (void)
{
#ifdef BSD
	outp ( KB_PORT_CTRL, KB_CTL_DISABLE);
	return inp ( KB_PORT_DATA );
#else
	Wait8042free ();
	outp ( KB_PORT_CTRL, KB_CTL_DISABLE);
	Wait8042free ();
	return inp ( KB_PORT_DATA );
#endif
}

#ifndef BSD
void Wait8042free (void)
{
	word cnt1, cnt2;

	_disable();
	cnt1 = cnt2 = ~0;
		// it is to send command to 8042 only when bit 1 of status port is 0 !
	while ( (inp (KB_PORT_STAT) & KB_CTS_IN_BUF_FULL )  &&  --cnt1 != 0 )
		cnt2 += cnt1 - 5;		// instruction to lengthen waiting !!!
}
#endif

void Beep (void)
{
#ifdef BSD
	outp (MONITOR_SERV, 0x40);
	SendEOI ();
#else
#define BEEP_OUT_CNT 0x30
#define BEEP_IN_CNT 0xD0

	int cnt;
	byte p61, p61save;

	SendEOI ();
	p61save = p61 = inp (SPEAKER_PORT);
	p61 &= ~SPEAKER_MASK_2;

	for ( cnt=0 ; cnt < BEEP_OUT_CNT ; ++cnt ) {
		outp (SPEAKER_PORT, p61 | SPEAKER_ON_MASK);
		Delay (BEEP_IN_CNT);
		outp (SPEAKER_PORT, p61 & ~SPEAKER_ON_MASK);
		Delay (BEEP_IN_CNT);
	}
	outp (SPEAKER_PORT, p61save);
#endif
}

byte ProcCapsLock (Wo a)
{
	if ( kb_flag & CAPS_ST_MASK ) {
		if ( kb_flag & SHFT_ST_MASK ) {
			if ( a.l <= 'Z'  &&  a.l >= 'A' )
				a.l += ('a'-'A');
		}
		else {
			if ( a.l <= 'z'  &&  a.l >= 'a' )
				a.l -= ('a'-'A');
		}
	}
	return PutChar (a);
}

						// returns 3 if then to go to IRET (ret 3)
						// returns 2 if then to go to exit (w/o EOI) (ret 2)
byte PutChar (Wo a)
{
	bptr b;

	b = IncBufPtr (buffer_tail);
	if ( b == buffer_head ) {
		Beep ();
		return 2;
	}
	else {
		_disable ();
		* buffer_tail = a;
		buffer_tail = b;
		SendEOI ();
		EnableKeyboard ();
		int15 (0x91, 0x02);
		kb_flag_3 &= ~(LAST_E0_MASK | LAST_E1_MASK);
		return 3;
	}
}

				// returns a.l -- LED state as it is to be
				// returns a.h -- differences between real and logical LED state
Wo CheckLocksLED (void)
{
	Wo a;

	a.l = (kb_flag & ALL_LOCKS_ST_MASK) >> 4;
	a.h = (a.l ^ kb_flag_2) & ALL_LED_MASK;
	return a;
}

void CheckSetLED_1 (void)			// sending EOI !!!
{
	_disable ();
	if ( ! (kb_flag_2 & LED_UPDT_MASK) ) {
		kb_flag_2 |= LED_UPDT_MASK;
		SendEOI ();
		MakeLEDs ();
	}
	_enable ();
}

void MakeLEDs (void)
{
	Wo a;

	SendToKB (KB_CMD_SET_LED);
	a = CheckLocksLED ();
	kb_flag_2 = (kb_flag_2 & ~ALL_LED_MASK) | a.l;
	if ( ! (kb_flag_2 & KB_ERROR_MASK) )
		SendToKB (a.l);			// parameter to make LEDs
	if ( ! (kb_flag_2 & KB_ERROR_MASK) )	// !!! be afraid of optimization !!!
		SendToKB (KB_CMD_ENABLE);
	kb_flag_2 &= ~(KB_ERROR_MASK | LED_UPDT_MASK);
}

void SendToKB (byte c)
{
#ifdef BSD

	outp (KB_PORT_DATA, c);
	kb_flag_2 &= ~KB_ERROR_MASK;

#else
	int retry1 = 3;
	int retry2 = 0x1000;

	do	{
		Wait8042free ();
		outp (KB_PORT_DATA, c);
		kb_flag_2 &= ~(KB_ERROR_MASK | LAST_RESEND_MASK | LAST_ACK_MASK);
		_enable ();
		do	{
			WaitUnknownEvent1 ();
			if ( kb_flag_2 & (LAST_RESEND_MASK | LAST_ACK_MASK) )
				break;
			WaitUnknownEvent1 ();
			if ( kb_flag_2 & (LAST_RESEND_MASK | LAST_ACK_MASK) )
				break;
		} while (--retry2);
		_disable ();
		if ( kb_flag_2 & LAST_ACK_MASK )
			break;
	} while (--retry1);
	if ( ! (kb_flag_2 & LAST_ACK_MASK) )
		kb_flag_2 |= KB_ERROR_MASK;
#endif
}

#ifndef BSD
void WaitUnknownEvent1 (void)
{
	int to_leng = 5;

	while ( ! (inp (PORT_61) & UNKNOWN_MASK_1) )
		to_leng -= 9;		// instruction to lengthen waiting !!!
}
#endif

bptr IncBufPtr (bptr b)
{
	++b;
	if ( b == buffer_end )
		b = buffer_start;
	return b;
}

byte CheckKBx (void)
{
	return (kb_flag_3 & ENH_KB_MASK);
}

byte CheckLastE0 (void)
{
	return (kb_flag_3 & LAST_E0_MASK);
}

byte CheckLastE1 (void)
{
	return (kb_flag_3 & LAST_E1_MASK);
}

/*******************************************************/

void SendEOI (void)
{
#ifdef BSD
	outp (0x20,0x20);
#else
	outp (0x20,0x20);
#endif
}

byte IsDepressing (byte c)
{
	return (c & 0x80);
}

byte PureCod (byte c)
{
	return (c & 0x7F);
}

uint SendSysReq (byte y)
{
	EnableKeyboard ();
	SendEOI ();
	int15 (0x85, y);
	return 3;
}

uint HoldState (void)
{
	kb_flag_1 |= HOLD_STATE_MASK;
	EnableKeyboard ();
	SendEOI ();
#ifdef BSD
	_enable ();
#endif
	EnableVideo ();
	while ( kb_flag_1 & HOLD_STATE_MASK )		// volatile !!!
		;
	return 2;
}

void CtrlBreak (void)
{
	buffer_head = buffer_tail = buffer_start;
	bios_break = 0x80;
	EnableKeyboard ();
	_asm {
		push	bp
		int	0x1B
		pop	bp
	}
	return ;
}

uint PrintScreen (void)
{
	EnableKeyboard ();
	SendEOI ();
	_asm {
		push	bp
		int	0x05
		pop	bp
	}
	kb_flag_3 &= ~(LAST_E0_MASK | LAST_E1_MASK);
	return 2;
}

void EnableVideo (void)
{
	if ( crt_mode != 7 )
		outp (CGA_CTRL_PORT, crt_mode_set);
}


/*******************************************************/

Wo int15 (byte ah_, byte al_)		// must return CF in a.h for a.h == 0x4F
{
	Wo rs;

	_asm {
		mov	ah,ah_
		mov	al,al_
		stc
		push	bp
		int	0x15
		pop	bp
		mov	rs.h,0
		jnc	i15_l10
		mov	rs.h,1
	i15_l10:
		mov	rs.l,al
	}

	return rs;
}

void CtrlAlt (byte c)
{
	if ( c == MCOD_DEL ) {
		reset_flag = 0x1234;
		jmp_to_reset;
	}
	else {
/*
		if ( c == MCOD_PLUS  ||  c == MCOD_MINUS )
			call_sub_E88E;
*/
	}
	return ;
}

void Delay (int cnt)
{
	_asm {
		mov	cx,cnt
	loo:
		loop	loo
	}
	return ;
}

/*******************************************************/

void LoadDS (void)
{
	_asm {
		push	0x40
		pop	ds
	}
}

/*******************************************************/

void _interrupt _far _cdecl int16 ( word _es, word _ds, word _di, word _si,
												word _bp, word _sp, word _bx, word _dx,
												word _cx, word _ax, word _ip, word _cs,
												word flags )
{
	Wo a, b, c;
	int zf, ret_zf;
	bptr b_p;

	LoadDS ();
	_enable ();

	a.x = _ax;
	b.x = _bx;
	c.x = _cx;

	ret_zf = 0;
	switch ( a.h ) {
		case 0:		do {
							a = GetChar ();
						} while ( AnalizeHiddenCodes ( WBSSS(& a)) );
								// AnalizeHiddenCodes returns 1 means "hidden" code
						break;
		case 1:		while (1) {
							if ( ! MakeLEDs_and_GetChar ( WBSSS(& a)) ) {
							// MakeLEDs_and_GetChar returns 0 if there are no chars
								zf = 1;
								break;
							}
							if ( (zf = AnalizeHiddenCodes ( WBSSS (& a))) )
								// AnalizeHiddenCodes returns 1 means "hidden" code
								a = GetChar ();
							else
								break;
						}
						ret_zf = 1;
						break;
		case 2:		a.l = kb_flag;
						break;
		case 3:		if ( b.l <= 0x1F  &&  b.h <= 3  &&  a.l == 5 ) {
							DisableKeyb_InChar ();
							_enable ();
							SendToKB (KB_CMD_SET_RATE);
							SendToKB ( (b.h << 5) | b.l );
							EnableKeyboard ();
							_enable ();
							SendToKB (KB_CMD_ENABLE);
						}
						break;
		case 5:		_disable ();
						if ( (b_p = IncBufPtr (buffer_tail)) == buffer_head ) {
							a.l = 1;
						}
						else {
							* buffer_tail = c;
							buffer_tail = b_p;
							a.l = 0;
						}
						break;
		case 0x10:	a = GetChar ();
						a = RemoveHiddenF0 (a);
						break;
		case 0x11:	if ( MakeLEDs_and_GetChar ( WBSSS (& a)) ) {
							// MakeLEDs_and_GetChar returns 0 if there are no chars
							a = RemoveHiddenF0 (a);
							zf = 0;
						}
						else
							zf = 1;
						ret_zf = 1;
						break;
		case 0x12:	a.l = kb_flag;
						a.h = kb_flag_1 &
							(ALL_LOCKS_ST_MASK | L_ALT_MASK | L_CTRL_MASK);
						a.h |= (kb_flag_1 & SYS_REQ_MASK) << 5;
						a.h |= kb_flag_3 & (R_ALT_MASK | R_CTRL_MASK);
						break;
	}
	_ax = a.x;
	if ( ret_zf ) {
		if ( zf )
			flags |= ZF_FLAG_MASK;
		else
			flags &= ~ZF_FLAG_MASK;
	}
}

/*******************************************************/
void CheckSetLED_2 (void)		// without EOI !!!
{
	_disable ();
	if ( ! (kb_flag_2 & LED_UPDT_MASK) ) {
		kb_flag_2 |= LED_UPDT_MASK;
		MakeLEDs ();
	}
	_enable ();
}

				// returns 0 if there is no char in buffer
int MakeLEDs_and_GetChar ( Wo _based (_segname("_STACK")) * ap )
{
	Wo a;

	a = CheckLocksLED ();
	if ( a.h != 0 )
		CheckSetLED_2 ();
	_disable ();
	* ap = * buffer_head;
	return (buffer_head != buffer_tail);
}

Wo GetChar (void)
{
	Wo a;

	if ( buffer_head == buffer_tail )
		int15 (0x90, 0x02);
	do	{
		_enable ();
	} while ( ! MakeLEDs_and_GetChar (WBSSS (& a)) );
			// MakeLEDs_and_GetChar returns 0 if there are no chars
	buffer_head = IncBufPtr (buffer_head);
	_enable ();
	return a;
}

Wo RemoveHiddenF0 (Wo a)
{
	if ( a.l == HID_COD_F0  &&  a.h != 0x00 )
		a.l = 0;
	return a;
}

				// returns 1 means "hidden" code (for f. 0 & 1)
uint AnalizeHiddenCodes ( Wo _based (_segname("_STACK")) * ap )
{
	if ( ap->h > INT_16_HID_SCAN_LIMIT ) {
		if ( ap->h == HID_COD_E0 ) {
			if ( ap->l == CR  ||  ap->l == LF )
				ap->h = MCOD_ENTER;
			else
				ap->h = MCOD_SLASH;
			return 0;
		}
		else
			return 1;
	}
	else {
		if ( ap->l == HID_COD_F0 ) {
			if ( ap->h == 0x00 )
				return 0;
			else
				return 1;
		}
		if ( ap->l == HID_COD_E0 ) {
			if ( ap->h != 0x00 )
				ap->l = 0;
		}
	}
	return 0;
}

/*******************************************************/

