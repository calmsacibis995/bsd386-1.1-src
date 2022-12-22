#ifdef DEBUG
#include <stdio.h>
extern FILE *logfile;
static FILE *save_logfile = NULL;
#endif

#include <signal.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include "/usr/src/sys/i386/isa/pcconsioctl.h"
#include "queue.h"
#include "keyboard.h"
#ifdef DEBUG
#include "kernel.h"
extern struct connect_area *connect_area;
#endif

typedef unsigned char byte;
typedef unsigned short word;

static struct queue *kbd_queue;

/*******************************************************/
#define KB_IRQ_N 0x01

/* bit definitions for "what expected" at port 60 */

#define EX_CMD_BYTE 0x01
#define EX_OUT_PORT 0x02

#define KB_EX_LEDS 0x01
#define KB_EX_RATE 0x02

/*******************************************************/

static byte input_buf;
static byte output_buf;
static byte input_port;
static byte output_port;
static byte status_reg;
static byte cmd_reg;

static byte what_expect;
static byte what_to_kb;

static int internal_source;
static int kb_disabled;
static int save_kb_disabled;
static int disable_by_out_port;
static int dis_interf_by_char;

static int prof_time;

/*******************************************************/
static void ToKbdQueue (int scode);
static void KbFillQueue (int scode);
static void SetInternalSource (byte ch);
static void PushKeyboard (void);
void PushKeyboardBySig ();
static void SetProfTimer (void);
void KbTimeExpired (void);
static byte KeybIn (int portnum);
static void KeybOut (int portnum, byte val);
static void KeybCtrlOut (byte val);
static void KeybDataOut (byte val);
static void SendToKB (byte val);
static void MoveGateA20 (int val);
static void ToReset (void);
void init_keyboard (void);

/*******************************************************/
static inline int KbIrqDisabled (void)
{
  return ( ! (cmd_reg & CMD_REG_ENABLE_INT)  ||  disable_by_out_port );
}

static inline int KbInterfaceDisabled (void)
{
  return ( ((cmd_reg & CMD_REG_DISABLE) && !(cmd_reg & CMD_REG_INHIB_OVERR)) ||
	  kb_disabled  ||  dis_interf_by_char );
}

static inline int GarbageChar (unsigned int ch)
{
  return ( ch >= KB_ECHO );  /* I don't know how to throw out other garbage */
}

/*******************************************************/

static void ToKbdQueue (int scode)
{
  sigset_t sgset = sigmask(SIGPROF);

  sigprocmask (SIG_BLOCK, &sgset, NULL);
  if (queue_empty (kbd_queue)) {
    put_char_q (kbd_queue, scode);
    if (! KbInterfaceDisabled ())
      PushKeyboard ();
/*
    else
      SetProfTimer ();
*/
  }
  else {
    put_char_q (kbd_queue, scode);
    if ( check_queue_overflow (kbd_queue) )
      ioctl (0, PCCONIOCBEEP);
  }
  sigprocmask (SIG_UNBLOCK, &sgset, NULL);
}

static void KbFillQueue (int scode)
{
  static int one_ctrl[2], one_alt[2], any_ctrl, any_alt;
  static int last_E0, last_E1;


/******* proceed ctrl + alt + ... -- our private combinations ********/

  if ( any_ctrl  &&  any_alt ) {
    switch (scode) {
#ifdef TRACE
    case MCOD_GREY_MINUS:
      if (console_trace())
	return;
#endif
    case MCOD_DEL:
      ioctl(0, PCCONIOCSTOPBEEP); /* sound off */
      if (! last_E0) {
	clear_queue (kbd_queue);
	if (one_ctrl[1])
	  ToKbdQueue (MCOD_E0);
	ToKbdQueue (MCOD_CTRL|0x80);
	if (one_alt[1])
	  ToKbdQueue (MCOD_E0);
	ToKbdQueue (MCOD_ALT|0x80);
	MenuEnter (0, 0);  /* 0 means no error */
	any_ctrl = one_ctrl[0] = one_ctrl[1] = 0;
	any_alt = one_alt[0] = one_alt[1] = 0;
	last_E0 = last_E1 = 0;
	return;
      }
      else {
	ErrExit (0, "Normal exit\n");
      }
#ifdef DEBUG
    case MCOD_LSHFT:
      vga_save_current_mode();
      break;
    case MCOD_RSHFT:
      vga_restore_current_mode();
      break;
    case MCOD_SLASH:
      if (last_E0) {
	dprintf ("!!!!!! !!!!!! logfile turned off !!!!!! !!!!!!\n");
	save_logfile = logfile;
	logfile = NULL;
      }
      else {
	if (save_logfile != NULL) {
	  logfile = save_logfile;
	  dprintf ("!!!!!! !!!!!! logfile turned on !!!!!! !!!!!!\n");
	}
      }
      break;
    case 0x02:     /* 1/! !!!!! */
    case 0x03:     /* 2/@ !!!!! */
    case 0x04:     /* 3/# !!!!! */
    case 0x05:     /* 4/$ !!!!! */
    case 0x06:     /* 5/% !!!!! */
    case 0x07:     /* 6/^ !!!!! */
    case 0x08:     /* 7/& !!!!! */
    case 0x09:     /* 8/8 !!!!! */
    case 0x0A:     /* 9/( !!!!! */
      dprintf ("!! +++++++ marker %d%d%d%d%d%d%d%d%d marker ++++++ !!\n",
	       scode-1,scode-1,scode-1,scode-1,scode-1,
	       scode-1,scode-1,scode-1,scode-1);
      break;
    case 0x17:      /* letter i/I */
	clear_queue (kbd_queue);
	if (one_ctrl[1])
	  ToKbdQueue (MCOD_E0);
	ToKbdQueue (MCOD_CTRL|0x80);
	if (one_alt[1])
	  ToKbdQueue (MCOD_E0);
	ToKbdQueue (MCOD_ALT|0x80);
	any_ctrl = one_ctrl[0] = one_ctrl[1] = 0;
	any_alt = one_alt[0] = one_alt[1] = 0;
	last_E0 = last_E1 = 0;
      INT_STATE = 0x200;
      return;
#endif
    default:
      break;
    }
  }

/******** put character to keyboard queue ***********/

#ifdef DEBUG
#ifdef DEB_KBD
  dprintf ("KEYBOARD: fill queue, empty = %d, interface = %d, cod=%2.2X\n",
	   queue_empty (kbd_queue), KbInterfaceDisabled (), scode & 0xFF);
#endif
#endif

  if ( ! GarbageChar (scode) )
    ToKbdQueue (scode);

/******* maintain ctrl & alt state ***********/

  if ( scode == MCOD_E0 ) {
    last_E0 = 1;
    return;
  }
  if ( scode == MCOD_E1 ) {
    last_E1 = 1;
    return;
  }
  if ( last_E1 ) {
    last_E1 = 0;
    return;
  }
  switch (scode) {
  case MCOD_CTRL:            /* ctrl */
    one_ctrl[last_E0] = 1;
    break;
  case (MCOD_CTRL | 0x80):   /* cntl up */
    one_ctrl[last_E0] = 0;
    break;
  case MCOD_ALT:             /* alt */
    one_alt[last_E0] = 1;
    break;
  case (MCOD_ALT | 0x80):    /* alt up */
    one_alt[last_E0] = 0;
  default:
    break;
  }
  any_ctrl = one_ctrl[0] | one_ctrl[1];
  any_alt = one_alt[0] | one_alt[1];
  last_E0 = last_E1 = 0;
}

/*******************************************************/

static void SetInternalSource (byte ch)
{
   put_char_to_head (kbd_queue, ch);   /* output_buf = ch; */
   if ( check_queue_overflow (kbd_queue) )
     ioctl (0, PCCONIOCBEEP);
   ++internal_source;
   SetProfTimer ();
}

static void PushKeyboard (void)
{
  if (internal_source)
    --internal_source;
/*
  else
    dis_interf_by_char = 1;
*/
  dis_interf_by_char = 1;

  /* next character goes */
  output_buf = get_char_q (kbd_queue);
  status_reg |= KB_CTS_OUT_BUF_FULL;

  if (! KbIrqDisabled ())
    set_irq_request (KB_IRQ_N);
  prof_time = 0;

#ifdef DEBUG
#ifdef DEB_KBD
  dprintf ("KEYBOARD: push char, out=%2.2X\n", output_buf);
#endif
#endif
}

void PushKeyboardBySig (void)
{
  if (queue_not_empty (kbd_queue)  &&  prof_time == -1  &&
      (! KbInterfaceDisabled ()  ||  internal_source) )
    PushKeyboard ();
}

static void SetProfTimer (void)
{
  struct itimerval value;
  struct itimerval ovalue;

/*
  if ( prof_time == 0 ) {
*/
  if ( 1 ) {
    value.it_interval.tv_sec = 0;
    value.it_interval.tv_usec = 0;
    value.it_value.tv_sec = 0;
    value.it_value.tv_usec = 100;
    prof_time = 1;
    setitimer (ITIMER_PROF, &value, &ovalue);
  }
#ifdef DEBUG
#ifdef DEB_KBD
  dprintf ("KEYBOARD: set prof timer, val=%d\n", prof_time);
#endif
#endif
}

void KbTimeExpired (void)
{
  sigset_t sgset = sigmask(SIGIO);

  sigprocmask (SIG_BLOCK, &sgset, NULL);
  if (queue_not_empty (kbd_queue)) {
    if (! KbInterfaceDisabled ()  ||  internal_source )
      PushKeyboard ();
    else
      prof_time = -1;
  }
  else
    prof_time = 0;
#ifdef DEBUG
#ifdef DEB_KBD
  dprintf ("KEYBOARD: prof timer, val=%d\n", prof_time);
#endif
#endif
  sigprocmask (SIG_UNBLOCK, &sgset, NULL);
}

/*******************************************************/

static byte KeybIn (int portnum)
{
   byte rs;
   sigset_t sgset = sigmask(SIGPROF) | sigmask(SIGIO);

   switch (portnum) {
   case KB_PORT_DATA:
     sigprocmask (SIG_BLOCK, &sgset, NULL);
     rs = output_buf;
     status_reg &= ~KB_CTS_OUT_BUF_FULL;
     reset_irq_request (KB_IRQ_N);
     if (queue_not_empty (kbd_queue)  &&  prof_time == 0)
       SetProfTimer ();
     dis_interf_by_char = 0;
     sigprocmask (SIG_UNBLOCK, &sgset, NULL);
     break;
   case KB_PORT_STAT:
     rs = status_reg;
     break;
   }
   return rs;
}

/*******************************************************/

static void KeybOut (int portnum, byte val)
{
   int kb_was_disabled = KbInterfaceDisabled ();

   switch (portnum) {
      case KB_PORT_DATA:
         KeybDataOut (val);
         break;
      case KB_PORT_CTRL:
         KeybCtrlOut (val);
         break;
   }
   if ( kb_was_disabled  &&  ! KbInterfaceDisabled ()  &&
       queue_not_empty (kbd_queue) )
     SetProfTimer ();

#ifdef DEBUG
#ifdef DEB_KBD
   dprintf ("KEYBOARD: OUT, inerf=%d, time=%d, not emp=%d\n",
	    KbInterfaceDisabled(),prof_time,queue_not_empty(kbd_queue));
#endif
#endif
}

/*******************************************************/

static void KeybCtrlOut (byte val)
{
   status_reg |= KB_CTS_CMD_DATA_FLAG;

   switch ( val ) {
      case KB_CTL_READ_CMD:
         SetInternalSource (cmd_reg);
         break;
      case KB_CTL_WRITE_CMD:
         what_expect |= EX_CMD_BYTE;
         break;
      case KB_CTL_SELF_TEST:
         SetInternalSource (0x55);              /* no errors detected */
         break;
      case KB_CTL_INTERF_TEST:
         SetInternalSource (0x00);              /* no errors detected */
         break;
      case KB_CTL_DIAG_DUMP:
         /* there should be writing of diagnostic dump to internal queue ... */
         break;
      case KB_CTL_DISABLE:
         cmd_reg |= CMD_REG_DISABLE;
         break;
      case KB_CTL_ENABLE:
         cmd_reg &= ~CMD_REG_DISABLE;
         break;
      case KB_CTL_READ_IN_PORT:
         SetInternalSource (input_port);
         break;
      case KB_CTL_READ_OUT_PORT:
         SetInternalSource (output_port);
         break;
      case KB_CTL_WRITE_OUT_PORT:
         what_expect |= EX_OUT_PORT;
         break;
      case KB_CTL_READ_TEST:
         SetInternalSource (0x03);     /* both data and clock lines high */
         break;
      default:
#ifdef DEBUG
	 dprintf("KEYBOARD: UNRECOGNIZED CONTROL BYTE !!! (%.2X)\n", val);
#endif
         break;
   }
   if ( val >= KB_CTL_PULSE_OUTP  &&  (val & OUT_P_RESET) )
      ToReset ();
}

/*******************************************************/

static void KeybDataOut (byte val)
{
   if ( what_expect & EX_CMD_BYTE ) {
         /* CMD_REG_DISABLE, CMD_REG_INHIB_OVERR and CMD_REG_ENABLE_INT
            are covered in KeybOut */
         /* "hold" some bits always = 1 or 0 !!! */
      cmd_reg = (val | CMD_REG_PC_COMPATIB) &
               ~(CMD_REG_RESERV_1 | CMD_REG_PC_MODE | CMD_REG_RESERV_2);
   }
   else {
      if ( what_expect & EX_OUT_PORT ) {
         if ( (val ^ output_port) & OUT_P_GATE_A20 )
            MoveGateA20 (val & OUT_P_GATE_A20);
         if ( ! (val & OUT_P_RESET) )  /* reset line must be high ! */
            ToReset ();
         if ( val & OUT_P_OUT_BUF_FULL )  /* take action but keep bit = 0 ! */
	   disable_by_out_port = 1;
	 else
	   disable_by_out_port = 0;
         /* ignore OUT_P_INP_BUF_EMPT bit */
         /* "hold" some lines always high or low !!! */
         output_port = (val | OUT_P_RESERV_1 | OUT_P_RESERV_2 |
                     OUT_P_KB_DATA | OUT_P_KB_CLOCK) & ~OUT_P_OUT_BUF_FULL;
      }
      else {
         SendToKB (val);
      }
   }
   what_expect = 0;
   status_reg &= ~KB_CTS_CMD_DATA_FLAG;
}

/*******************************************************/

static void SendToKB (byte val)
{
  int no_ack = 0;

  if ( what_to_kb == 0 ) {
    if (val >= KB_CMD_VALID_LIMIT) {
      switch (val) {
      case KB_CMD_SET_LED:
	save_kb_disabled = kb_disabled;
	kb_disabled = 1;
	what_to_kb = KB_EX_LEDS;
	break;
      case KB_CMD_ECHO:
	SetInternalSource (KB_ECHO);
	no_ack = 1;
	break;
      case KB_CMD_READ_ID:
	SetInternalSource (KB_ID_2ND_3);
	SetInternalSource (KB_ID_1ST);
	break;
      case KB_CMD_SET_RATE:
	save_kb_disabled = kb_disabled;
	kb_disabled = 1;
	what_to_kb = KB_EX_RATE;
	break;
      case KB_CMD_ENABLE:
	/*      clear_queue (kbd_queue); */
	kb_disabled = 0;
	break;
      case KB_CMD_DEF_DISABLE:
	clear_queue (kbd_queue);
	kb_disabled = 1;
	break;
      case KB_CMD_SET_DEFAULT:
	clear_queue (kbd_queue);
	break;
      case KB_CMD_RESEND:
	/* previous byte needed !!! */
	no_ack = 1;
	break;
      case KB_CMD_RESET:
	clear_queue (kbd_queue);
	kb_disabled = 0;
	break;
      default:
	break;
      }
    }
    else {
      SetInternalSource (KB_RESEND);
      return;
    }
  }
  else {
    if ( what_to_kb == KB_EX_LEDS ) {
      what_to_kb = 0;
      val &= ALL_LED_MASK;
      ioctl (0, PCCONIOCSETLED, &val);
      kb_disabled = save_kb_disabled;
    }
    if ( what_to_kb == KB_EX_RATE ) {
      what_to_kb = 0;
/* ioctl (0, ..., &val); -- missing ioctl to set typematic rate & delay !!! */
      kb_disabled = save_kb_disabled;
    }
  }
  if ( ! no_ack )
    SetInternalSource (KB_ACK);
}

/*******************************************************/

static void MoveGateA20 (int val)
{
#ifdef DEBUG
  dprintf("KEYBOARD: ATTEMPT TO OPEN/CLOSE GATE A20 !!! \n");
#endif
}

static void ToReset (void)
{
#ifdef DEBUG
  dprintf("KEYBOARD: ATTEMPT TO GO TO RESET !!! \n");
#endif
  ErrorTo86 (" ERROR: attempt to reset microprocessor ");
}
/*******************************************************/

void init_keyboard (void)
{
   input_buf = KB_ACK;
   output_buf = 0;
   input_port = 0xBF;   /* is observed at AT386 (see PC/AT tech. ref.) */
   /* output_port = 0xCD; */
   output_port = OUT_P_KB_DATA | OUT_P_KB_CLOCK | OUT_P_RESERV_1 |
                  OUT_P_RESERV_2 | OUT_P_RESET |
		    OUT_P_GATE_A20; /* gate A20 -- temporarily !!! */
   /* status_reg = 0x1C; */       
   status_reg = KB_CTS_INHIBIT_SW | KB_CTS_CMD_DATA_FLAG | KB_CTS_SYS_FLAG;
   /* cmd_reg = 0x45; */
   cmd_reg = CMD_REG_PC_COMPATIB | CMD_REG_SYS_FLAG | CMD_REG_ENABLE_INT;

   what_expect = 0;     /* expected nothing */
   what_to_kb = 0;      /* nothing to send to keyboard */

   internal_source = 0;
   kb_disabled = 0;
   save_kb_disabled = 0;
   disable_by_out_port = 0;
   dis_interf_by_char = 0;

   prof_time = 0;

   define_in_port (0x60, KeybIn);
   define_in_port (0x64, KeybIn);
   define_out_port (0x60, KeybOut);
   define_out_port (0x64, KeybOut);

   kbd_queue = create_queue (KB_IRQ_N);		/* IRQ 1 */

#ifdef DEBUG
   save_logfile = logfile;
#endif
}

/*******************************************************/

#define KBD_BUFSIZE 1024

void do_keyboard_input(void)
{
  int n, k;
  unsigned char buffer[KBD_BUFSIZE];

  do {
    k = read(0, buffer, KBD_BUFSIZE);
    if (k > 0)
      for(n = 0; n < k; n++)
	KbFillQueue(buffer[n]);
  } while (k == KBD_BUFSIZE);
}
