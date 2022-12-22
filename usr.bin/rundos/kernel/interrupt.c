#include <stdio.h>
#include <sys/types.h>
#include <machine/psl.h>
#include "kernel.h"
#include "queue.h"

typedef unsigned char byte;
typedef unsigned short word;

extern struct connect_area *connect_area;
extern struct queue *qlist;
extern int tick;

int supercli = 0;

#define MASTER	0
#define SLAVE	1

#define SLAVE_L	4

#define SINGLE  2
#define OPERATE 0x18
#define ICW1    0x10
#define OCW2    0
#define OCW3    8
#define IC4     1

#define STATUS_AND_POLL 7
#define RIS         3
#define RR          2
#define POLL        4
#define IRQ_WAIT    0x80
#define SPEC_MASK   0x60
#define SMM         0x20
#define AEOI        2

#define ROTATE      0x80
#define SL          0x40
#define EOI         0x20

#define IRQNUM      7

#define NO_BIT      ((word) 16)

static struct i8259 {
  enum { READY, SW1, SW2, SW3 } state;
  unsigned short mask;  /* ocw1 */
  int base;  /* icw2 */
  word irr;
  word isr;
  word prior;
  byte ocw3;
  byte icw1;
  byte icw3;
  byte icw4;
  byte ocw2;
} i8259[2] = 
{
  { READY, 0xffff, 0x8, 0, 0, 0, RR},
  { READY, 0xfffe, 0x70, 0, 0, 0, RR}
};

struct queue * q_irq [16];



int only_one_irq (int intnum)

{
  int ret_val = 0;

  if (intnum == 0)
    ret_val = timer_ready ();
  if (intnum == 8)
    ret_val = fast_timer_ready ();
/*
  else
    if (q_irq [intnum] != 0 &&
	(ret_val = buffer_contents (q_irq [intnum])) != 0)
      ret_val--;
*/

  return ret_val;
}

/* NO INLINE!!! */
static word last_bit (word r, word high)
{
  word res;
  byte zero_f;

  r = (((r << 8) | r) >> high) << high;
  asm ("bsfw %0,%%ax" : : "g" (r) : "ax" ); /* search bit */
  asm ("movw %%ax,%0" : "=g" (res));
  asm ("setzb %0" : "=g" (zero_f));
  return zero_f ? NO_BIT : (res % 8);
}


inline void set_irq_request (intnum)
{
  struct i8259 *cp = &i8259[intnum / 8];
  unsigned short mask = 1 << (intnum % 8);

#ifdef DEBUG
  dprintf("SET IRR = %2X\n",intnum);
#endif

  cp->irr |= mask;
  
}

inline void reset_irq_request (intnum)
{
  struct i8259 *cp = &i8259[intnum / 8];
  unsigned short mask = 1 << (intnum % 8);

  cp->irr &= ~mask;
  
}


int irq_request ()
{
  int intnum, i;

  for (i = 0; i < 2; i++) {  /* for Master and Slave */
    if ((intnum = (int) last_bit (i8259[i].irr, i8259[i].prior)) !=
	NO_BIT)
      return intnum + i * 8;
  }
  return NO_BIT;  /* NO irq */
}



int interrupt_enabled(int intnum)
{
  struct i8259 *cp = &i8259[intnum / 8];
  unsigned short mask = 1 << (intnum % 8);

  if (supercli)
    return 0;

  if ((cp->mask & mask) == 0 && (cp->ocw3 & POLL) == 0 &&
      ((cp->ocw3 & SMM) || (intnum % 8) < last_bit (cp->isr,cp->prior))) {
    if ((intnum / 8) == 0 || ((i8259 [0].mask | i8259 [0].isr) & SLAVE_L) == 0)
      if (INT_STATE == PSL_I)
        return 1;
    connect_area->need_interrupt = 1;
  }
  return 0;
}

void hardware_interrupt(int intnum, struct trapframe *tf)
{
  int n = intnum / 8;
  struct i8259 *cp = &i8259[n];


#ifdef DEBUG
  if (intnum == 1 && (cp->isr & 2)) {
    dprintf("KKK ocw3 %2X last %4x\n",
	    cp->ocw3, last_bit (cp->isr,cp->prior));
	  }
#endif

  if (n != MASTER || (cp->icw4 & AEOI) == 0)
    cp->isr |= 1 << (intnum % 8);
  if (only_one_irq (intnum) == 0)
    reset_irq_request (intnum);

  intnum += cp->base;
  if (n == SLAVE) {
    intnum -= 8;
    i8259 [0].isr |= 4;
  }
#ifdef DEBUG
  dprintf("hardware int %2.2X at %4.4X:%4.4X (%4.4X:%4.4X)\n",
	  intnum, tf->tf_cs, tf->tf_ip, tf->tf_ss, tf->tf_sp);
#endif
  PUSH((tf->tf_eflags & (~PSL_I)) | INT_STATE | PSL_IOPL, tf);
  PUSH(tf->tf_cs, tf);
  PUSH(tf->tf_ip, tf);
  tf->tf_cs = GETWORD(intnum * 4 + 2);
  tf->tf_ip = GETWORD(intnum * 4);
  INT_STATE = 0;
}

void check_interrupts(struct trapframe *tf)
{
  int intnum;

  if ( (intnum = irq_request ()) != NO_BIT  &&  interrupt_enabled (intnum) )
    hardware_interrupt (intnum, tf);
}

static void int_outb(int port, unsigned char data)
{
  word isr_mask, inum;
  struct i8259 *cp = &i8259[port >> 7];

#ifdef DEBUG
#ifdef DEB_INT
  dprintf("8259: output byte %2.2X to port %2.2X\n", data, port);
#endif
#endif
  port &= ~0x80;
  switch(port) {
  case 0x20:
    switch (data & OPERATE) {
      case ICW1: 
        cp->icw1 = data;
        cp->state = SW1;
        cp->ocw3 = RR;
        cp->prior = 0;
#ifdef DEBUG
#ifdef DEB_INT
	dprintf("start of chip programming\n");
#endif
#endif
	
	break;
      case OCW2:
	if (data & EOI) {
	  if (data & SL)
	    cp->isr &= ~(1 << (data & IRQNUM));
	  else {
	    if ((inum = last_bit (cp->isr, cp->prior)) != NO_BIT) {
	      cp->isr &= ~(1 << inum);
	      if (data & ROTATE)
		cp->prior = (inum + 1) % 8;
	    }
	    break;
	  }
	}
	if ((data & ROTATE) && (data & SL))
	    cp->prior = ((data & IRQNUM) + 1) % 8;

	break;
      case OCW3:
	if (data & RR)
          cp->ocw3 = (cp->ocw3 & ~RIS) | (data & RIS); 
        cp->ocw3 = (cp->ocw3 & ~POLL) | (data & POLL);
        if (data & SPEC_MASK)
          cp->ocw3 = (cp->ocw3 & ~SPEC_MASK) | (data & SPEC_MASK); 
        break;
      }

    break;
  case 0x21:
    switch (cp->state) {
    case SW1:
      cp->base = data;
#ifdef DEBUG
#ifdef DEB_INT
      dprintf("base = 0x%x\n", cp->base);
#endif
#endif
      if (cp->icw1 & SINGLE)
	goto skip_sw2;
      else
	cp->state = SW2;
      
      break;
    case SW2:
      cp->icw3 = data;
     skip_sw2:
      if (cp->icw1 & IC4)
	cp->state = SW3;
      else
	cp->state = READY;
      break;
    case SW3:
      cp->icw4 = data;
      cp->state = READY;
#ifdef DEBUG
#ifdef DEB_INT
      dprintf("end of chip programming\n");
#endif
#endif
      break;
    case READY:
      cp->mask = data;
    }
    break;
  }
}

static unsigned char int_inb(int port)
{
  struct i8259 *cp = &i8259[port >> 7];
  unsigned char ret_val = 0;

  port &= ~0x80;
#ifdef DEBUG
#ifdef DEB_INT
  dprintf("8259: input byte from port %2.2X\n", port);
#endif
#endif
  switch(port) {
  case 0x20:
    ret_val = 0;
    if (cp->ocw3 & POLL) {
      cp->ocw3 &= ~POLL;
      if ((ret_val = last_bit (cp->irr,cp->prior)) != NO_BIT) {
#ifdef DEBUG
	dprintf("POLL: in %2X irr = %2x\n", ret_val, cp->irr);
#endif
        cp->irr &= ~(1 << ret_val);
        cp->isr |= 1 << ret_val;
#ifdef DEBUG
	dprintf("POLL1: isr %2X irr = %2x\n", cp->isr, cp->irr);
#endif
        ret_val |= IRQ_WAIT;
      }
    }
    else
      if (cp->ocw3 & RIS == RIS)
        ret_val = cp->isr;
      else
        ret_val = cp->irr;
    break;
  case 0x21:
    ret_val = cp->mask;
    break;
  }
  return(ret_val);
}

void init_interrupts(void)
{
  define_in_port(0x20, int_inb);
  define_in_port(0x21, int_inb);
  define_out_port(0x20, int_outb);
  define_out_port(0x21, int_outb);
  define_in_port(0xa0, int_inb);
  define_in_port(0xa1, int_inb);
  define_out_port(0xa0, int_outb);
  define_out_port(0xa1, int_outb);
}
