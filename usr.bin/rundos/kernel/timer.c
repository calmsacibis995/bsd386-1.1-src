#ifndef lint
static char rcsid[] = "$Id: timer.c,v 1.2 1992/09/03 00:03:12 polk Exp $";
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include "/usr/src/sys/i386/isa/pcconsioctl.h"

#include "kernel.h"

#define BYTE(st) (* (byte *) &(st))
#define WORD(st) (* (short *) &(st))

typedef unsigned char byte;
typedef unsigned short word;

typedef union shorthl {
  unsigned short w;
  struct {
    byte l;
    byte h;
  } b;
} shorthl;

struct reg_c_w {
  byte bcd : 1;
  byte m   : 3;
  byte rw  : 2;
  byte sc  : 2;
} ;


struct timer_us_data {
  shorthl  ol;
  shorthl  cl;
  shorthl  cr;
  struct reg_c_w rs;
  struct reg_c_w rcw;
  byte gate;
  struct {
    byte pari   : 1;
    byte paro   : 1;
    byte rbc    : 1;
    byte lt     : 2;
  } curop;
} prtim[3];

#define RBC_COM     3
#define LATCH_COM   0

#define SPEAKER_PORT 0x61
#define SPEAKER_ON_MASK 0x02
#define SPEAKER_MASK_2 0x01


/*------------------------------------------------------------
  Number of tick in day is (0x10000 * 0x18 + 0xB0) therefore
  tick size in microseconds is :
#define TICK_SIZE 0xD68C
*/

#define TICK_SIZE (((1800 * 1000000) / (0x10000/2 * 3 + 11)) * 3)

/*------------------------------------------------------------*/

#define UNIX_TICK_SIZE ((word) 0x2710)

#define SEC_MS    1000000
#define REQUIRE_TIC_COUNT connect_area->require_tic_count

extern struct timeval glob_clock;
extern struct timezone tz;
extern struct connect_area *connect_area;

byte port_61_val;
short sound_freq = 1000; /* Use in keyboard.c */

static struct timeval dos_clock;
int last_usec;
int last_sec;

static unsigned short timer_counter;
static unsigned short init_timer_counter = 0;

int tick = 0;

static int counter_byte_num = 0;
static word one_thing = 0;
static word clock_from_int8 = 0;
static int add_ticks = 1;
static byte sound_on = 1;
byte tick_mode = 0;

void correct_tick (byte tcm)
{
 if (tcm == 0 || prtim [0].cr.w == 0 || prtim [0].cr.w > UNIX_TICK_SIZE)
    add_ticks = 1;
 else
    add_ticks = UNIX_TICK_SIZE / prtim [0].cr.w;
}

void init_tick (void)
{
  tick = 0;
}


void change_tick_mode (byte tcm)
{
  tick_mode = tcm;
  correct_tick (tcm);
}

void change_sound_mode (void)
{
  sound_on ^= 1;
}


void do_timer_tick(void)
{
  long delta_uclock;
  long delta;

  gettimeofday (&glob_clock, &tz);
  delta_uclock = (glob_clock.tv_sec - dos_clock.tv_sec) * SEC_MS;
  delta_uclock += glob_clock.tv_usec - dos_clock.tv_usec;

/*------ Version with inc DOS counter evry tick ----*/

  if ((delta = delta_uclock / TICK_SIZE) == 0) {
    REQUIRE_TIC_COUNT ++;
    dos_clock.tv_usec += TICK_SIZE;
  }
  else
/*-------------------------------------------------*/
  {
    REQUIRE_TIC_COUNT += delta_uclock / TICK_SIZE;
    dos_clock.tv_sec = glob_clock.tv_sec - 1;
    dos_clock.tv_usec = (glob_clock.tv_usec + SEC_MS) -
      delta_uclock % TICK_SIZE;
  }



  last_sec = glob_clock.tv_sec;
  last_usec = glob_clock.tv_usec;
  prtim [0].cl.w = prtim [0].cr.w;
  prtim [0].cl.w --;
  
#ifdef DEBT
  dprintf ("\n !!! INT8  last = %x:%x \n",last_sec,last_usec);
#endif
  
  
  if (prtim [0].gate)
    set_irq_request (0);
  tick += add_ticks;
  timer_counter = init_timer_counter;
  alarm_check ();
}

int timer_ready(void)
{
/*  int ret_val = tick;*/
  if (tick)
    if (tick_mode == 1)
      tick--;
    else
      tick = 0;
#ifdef DEBUG
  dprintf ("\n !!! TICK = %x \n",tick);
#endif
  return tick;
}


void latch_tic (short n)
{
  long new_usec;
  word mticks;

  if (prtim [n].curop.lt)
    return;             /* Second latch command */
  prtim [n].curop.lt = 2;
  if (n == 0) {
    gettimeofday (&glob_clock, &tz);
    new_usec = (glob_clock.tv_sec - last_sec) * SEC_MS + glob_clock.tv_usec;
    mticks = (word)(new_usec - last_usec) % (word) (prtim [n].cr.w -(word)1);
    if (mticks == clock_from_int8)
      mticks = 0;
    else
      clock_from_int8 = mticks;

    if (one_thing++ == 0x20)
        one_thing = 1;

 /*--------- For future explore ------------------*/
    if (mticks)
      prtim [n].cl.w = prtim [n].cr.w - mticks + 16 * one_thing;
    else
      prtim [n].cl.w -= 16 * one_thing;
 /*------------------------------------------------*/

  }
  else
    prtim [n].cl.w = random ();
  prtim [n].ol = prtim [n].cl;
#ifdef DEBUG
  dprintf("\nLatch  ol = %4x usec = %8x\n",prtim [n].ol.w,
	  glob_clock.tv_usec );
#endif
}


unsigned char timer_inb(int portnum)
{
  byte ret_val = 0;
  short n,i;

#ifdef DEBT
  dprintf("IN port 0x%x \n", portnum);
#endif

  if (portnum == 0x43)
    return  ~0;

  n = portnum & 3;
  if (prtim [n].rs.m == 1 || prtim [n].rs.m == 5) {

#ifdef DEBUG
#ifdef DEBT
    dprintf("!!! IN port 0x%x MODE %x\n", portnum, prtim [n].rs.m);
#endif
#endif

    return ret_val;
  }

  if (prtim [n].curop.rbc) {
#ifdef DEBT
    dprintf("RBC curop %2x \n", BYTE (prtim [n].curop));
#endif
    prtim [n].curop.rbc = 0;
    return BYTE (prtim [n].rs);
  }
  
  if (prtim [n].curop.lt) {
    if (--prtim [n].curop.lt) {
#ifdef DEBT
      dprintf("Latch low %2x \n", BYTE (prtim [n].ol.b.l));
#endif
      return prtim [n].ol.b.l;
    }
    else {
#ifdef DEBT
      dprintf("Latch high %2x \n", BYTE (prtim [n].ol.b.h));
#endif
  return prtim [n].ol.b.h;
  }
}

#ifdef DEBT
  dprintf("Simple Read curop %2x \n", BYTE (prtim [n].curop));
#endif

  if (prtim [n].rs.rw == 3)
    i = prtim [n].curop.pari ++;
  else
    i = prtim [n].rs.rw >> 1;
  
  prtim [n].cl.w = random ();
  ret_val = (i ? prtim [n].cl.b.h : prtim [n].cl.b.l);

#ifdef DEBUG
#ifdef DEBT
  dprintf("timer inb port 0x%x data 0x%x\n", portnum, ret_val);
#endif
#endif
  return(ret_val);
}

void timer_outb(int portnum, unsigned char val)
{

short n;
short i;
int tick_interval, repeat_interval;

#ifdef DEBT
dprintf("timer outb port 0x%x data 0x%x\n", portnum, val);
#endif

  n = portnum & 3;
  if ((prtim [n].rs.m & 2) == 0) {   /* no 2 or 3 mode */
#ifdef DEBUG
    dprintf("!! OUT port 0x%x MODE %x\n", portnum, prtim [n].rs.m);
#endif
    if (n != 0 || prtim [n].rs.m == 1 || prtim [n].rs.m == 5)
       return;
  }
  if (prtim [n].rs.rw == 3)
    i = prtim [n].curop.paro ++;
  else
    i = prtim [n].rs.rw >> 1;
  
  if (i)
    prtim [n].cr.b.h = val;
  else
    prtim [n].cr.b.l = val;


  if (prtim [n].curop.paro == 0) {
    prtim [n].gate = 1;
    prtim [n].cl.w = prtim [n].cr.w - (word) 1;
    switch (n) {
      case 0:
#ifdef DEBT
      dprintf("Alarm (cr) = %4xn", prtim [n].cr.w);
#endif
        correct_tick (tick_mode);
        tick = 0;
        one_thing = 0;

        tick_interval = prtim [n].cr.w ? prtim [n].cr.w : TICK_SIZE; 
        repeat_interval = (prtim [n].rs.m & 2) ? tick_interval : 0;
        ualarm (tick_interval,repeat_interval);
        break;
      case 2:
         sound_freq = prtim[2].cr.w;
         if ((port_61_val & 3) == 3 && sound_on)
           ioctl(0, PCCONIOCSTARTBEEP, &sound_freq);   /* sound on */
         break;
     }
   }
}


void RCW_timer_outb(int portnum, struct reg_c_w val)

{

short n;

#ifdef DEBT
dprintf("timer outb port 0x%x data 0x%x\n", portnum, BYTE (val));
#endif

  if (val.sc == RBC_COM) {
    for (n = 0; n < 3; n++) {
      if ((val.rw & 2) == 0)
        prtim [n].curop.rbc = 1;
      if ((val.rw & 1) == 0)
        latch_tic (n);
    }
    return;
  }
  n = val.sc;
  if (val.rw == LATCH_COM) {
    latch_tic (n);
    return;
  }
                                 /* R/W mode */
  BYTE (prtim [n].curop) = 0;
  prtim [n].gate = 0;
  if (n == 2) {
#ifdef DEBT
    dprintf("Sound OF \n" );
#endif
    ioctl(0, PCCONIOCSTOPBEEP); /* sound off */
  }
  prtim [n].rcw = val;
  val.sc = 0;
  prtim [n].rs = val;
#ifdef DEBT
dprintf("RS = %x1\n",BYTE (prtim [n].rs) );
#endif

  return;

}


byte port_61_in ()

{
         /* here must be speaker gate and data (and much more) handling !!! */
         return port_61_val;
}


void port_61_out (int portnum,byte val)

{
         /* speaker gate and data (and much more) handling !!! */
#ifdef DEBUG
  dprintf("\n OUT 61 = %2x\n",val);
#endif
         if ((port_61_val & 3) != 3) {
	   if ((val & 3) == 3 && prtim [2].gate && sound_on)
	     ioctl(0, PCCONIOCSTARTBEEP, &sound_freq);  /*sound on */
	 }
	 else
	   if ((val & 3) != 3)
	     ioctl(0, PCCONIOCSTOPBEEP); /* sound off */
	 port_61_val = val;

}



void init_timer(void)
{
#ifdef DEBUG
  dprintf("TICK_SIZE = %x data 0x%x\n", TICK_SIZE);
#endif
  ualarm(100000, TICK_SIZE);
  gettimeofday (&dos_clock, &tz);
  define_in_port(0x40, timer_inb);
  define_out_port(0x40, timer_outb);
  define_in_port(0x42, timer_inb);
  define_out_port(0x42, timer_outb);
  define_in_port(0x43, timer_inb);
  define_out_port(0x43, RCW_timer_outb);

  define_out_port(0x61, port_61_out);
  define_in_port(0x61, port_61_in);

   port_61_val = 0x10;

  BYTE (prtim [0].rs) = 0x36;
  BYTE (prtim [2].rs) = 0x36;
  prtim [0].gate = prtim [2].gate = 1;
  prtim [0].cr.w = 0;
  prtim [0].cl.w = ~0;
  prtim [2].cr.w = 2000;
}
