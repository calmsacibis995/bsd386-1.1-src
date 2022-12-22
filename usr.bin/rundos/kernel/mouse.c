#include <stdio.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include "kernel.h"
#include "queue.h"

#define BUFSIZE 1024

extern struct connect_area *connect_area;

/* line control register */

#define LC_DIV_ACC 0x80   /* divisor latch access bit */
#define LC_BRK_CTRL 0x40  /* se break control */
#define LC_S_PAR 0x20     /* stick parity */
#define LC_EVEN_P 0x10    /* even parity select */
#define LC_PAR_E 0x08     /* parity enable */
#define LC_STOP_B 0x04    /* number of stop bits (0 - 1 bit) */
#define LC_W_LEN 0x03     /* unsigned short length (00 - 5, 01 - 6 etc.) */

/* line status register */

#define LS_NOP 0x80       /* not used */
#define LS_X_SHFT_E 0x40  /* 0=data transfer, 1=transmitter idle */
#define LS_X_HOLD_E 0x20  /* 0=ready, 1=transferring character */
#define LS_BREAK 0x10     /* break received */
#define LS_FRM_ERR 0x08   /* framing error */
#define LS_PAR_ERR 0x04   /* parity error */
#define LS_OVRN_ERR 0x02  /* overrun error */
#define LS_RCV_DATA_RD 0x01  /* data received */

/* interrupt enable register */

#define IE_NOP 0xF0         /* not used */
#define IE_MODEM_STAT 0x08  /* modem status int. */
#define IE_LINE_STAT 0x04   /* receiver-line status int. */
#define IE_TRANS_HLD 0x02   /* transmitter holding register empty int. */
#define IE_RCV_DATA 0x01    /* received data available int. */

/* interrupt identification register */

#define II_NOP 0xF8
#define II_INT_ID 0x06      /* mask: bits see below */
#define II_PEND_INT 0x01    /* 1=no interrupt pending */

/* bit masks for II_INT_ID */

#define II_LINE_STAT 0x06
#define II_RCV_DATA 0x04
#define II_TRANS_HLD 0x02
#define II_MODEM_STAT 0x00

#define N_OF_COM_REGS 7

#define COM_PORT_BASE	0x3f8
#define COM_IRQ		4

static struct {
  int			file;
  int			mtype;
  struct queue		*com_queue;
  unsigned char		div_latch[2];
  unsigned char		int_enable;
  unsigned char		int_id;
  unsigned char		line_ctrl;
  unsigned char		modem_ctrl;
  unsigned char		line_stat;
  unsigned char		modem_stat;
} com_data = { -1 };

#define DIV_LATCH_LOW	0
#define DIV_LATCH_HIGH	1

static inline void com_set_line(void)
{
  static unsigned short cflag[4] = {
    (CS7                  ),   /* MicroSoft */
    (CS8                  ),   /* MouseSystems */
    (CS8 | PARENB | PARODD),   /* MMSeries */
    (CS8                  ),   /* Logitech */
  };
  struct termios  tty;
  int mode = 0;		/* read|write */

  tty.c_iflag = IGNBRK | IGNPAR;
  tty.c_oflag = 0;           
  tty.c_lflag = 0;
  tty.c_cc[VTIME] = 0; 
  tty.c_cc[VMIN] = 1;
  tty.c_cflag = cflag[com_data.mtype]|CREAD|CLOCAL|HUPCL;
  cfsetispeed(&tty, B1200);
  cfsetospeed(&tty, B1200);
  tcsetattr(com_data.file, 0, &tty);

  fcntl(com_data.file, F_SETFL, O_NDELAY);
  ioctl(com_data.file, TIOCFLUSH, &mode);
}

static unsigned char com_port_in(int port)
{
  unsigned char rs;

  port -= COM_PORT_BASE;

  switch (port) {
  case 0:       /* 3F8 - (receive buffer) or (divisor latch LO) */
    if (com_data.line_ctrl & LC_DIV_ACC)
      rs = com_data.div_latch[DIV_LATCH_LOW];
    else {
      if (queue_not_empty(com_data.com_queue)) {
	rs = get_char_q(com_data.com_queue);
	if (queue_not_empty(com_data.com_queue) &&
	    (com_data.int_enable & IE_RCV_DATA) != 0) {
	  set_irq_request(COM_IRQ);
	}
      } else
	rs = 0x4d;		/* SUPER PARANOID !!! */
    }
    break;
  case 1:       /* 3F9 - (interrupt enable) or (divisor latch HI) */
    if (com_data.line_ctrl & LC_DIV_ACC)
      rs = com_data.div_latch[DIV_LATCH_HIGH];
    else
      rs = com_data.int_enable;
  case 2:       /* 3FA - interrupt identification register */
    rs = com_data.int_id;
    break;
  case 3:       /* 3FB - line control register */
    rs = com_data.line_ctrl;
    break;
  case 4:       /* 3FC - modem control register */
    rs = com_data.modem_ctrl;
    break;
  case 5:       /* 3FD - line status register */
    rs = LS_X_SHFT_E|LS_X_HOLD_E|LS_RCV_DATA_RD;
    break;
  case 6:       /* 3FE - modem status register */
    rs = com_data.modem_stat;
    break;
  }
#ifdef DEBUG
  dprintf("mouse: input from port 0x%x val 0x%x\n", COM_PORT_BASE + port, rs);
#endif
  return rs;
}

static void com_port_out(int port, unsigned char val)
{
#ifdef DEBUG
  dprintf("mouse: output to port 0x%x val 0x%x\n", port, val);
#endif
  port -= COM_PORT_BASE;

  switch (port) {
  case 0:       /* 3F8 - (transmit buffer) or (divisor latch LO) */
    if (com_data.line_ctrl & LC_DIV_ACC)
      com_data.div_latch[DIV_LATCH_LOW] = val;
    else {
      /* output to line - do nothing */
    }
    break;
  case 1:       /* 3F9 - (interrupt enable) or (divisor latch HI) */
    if (com_data.line_ctrl & LC_DIV_ACC)
      com_data.div_latch[DIV_LATCH_HIGH] = val;
    else {
      com_data.int_enable = val;
      if ((val & IE_RCV_DATA) == 0)
	reset_irq_request(COM_IRQ);
      else if (queue_not_empty(com_data.com_queue))
	set_irq_request(COM_IRQ);
    }
    break;
  case 2:       /* 3FA - interrupt identification register */
    com_data.int_id = val;
    break;
  case 3:       /* 3FB - line control register */
    com_data.line_ctrl = val;
    break;
  case 4:       /* 3FC - modem control register */
    com_data.modem_ctrl = val;
    break;
  case 5:       /* 3FD - line status register */
    com_data.line_stat = val;
    break;
  case 6:       /* 3FE - modem status register */
    com_data.modem_stat = val;
    break;
  }
}

static inline void generate_event(int buttons,
				  unsigned char dx, unsigned char dy)
{
  put_char_q(com_data.com_queue,
	     0x40|((buttons & 0x4) << 3)|((buttons & 0x1) << 4)|
	     (dx >> 6) | ((dy >> 4) & 0xc));
  put_char_q(com_data.com_queue, dx & 0x3f);
  put_char_q(com_data.com_queue, dy & 0x3f);
  if (com_data.int_enable & IE_RCV_DATA)
    set_irq_request(COM_IRQ);
}

static void process_mouse_events(unsigned char *rBuf, int nBytes)
{
  int i, buttons, dx, dy;
  static int pBufP = 0;
  static unsigned char pBuf[8];

  static unsigned char proto[4][5] = {
    /*  hd_mask hd_id   dp_mask dp_id   nobytes */
    { 	0x40,	0x40,	0x40,	0x00,	3 	},  /* MicroSoft */
    {	0xf8,	0x80,	0x00,	0x00,	5	},  /* MouseSystems */
    {	0xe0,	0x80,	0x80,	0x00,	3	},  /* MMSeries */
    {	0xf8,	0x80,	0x00,	0x00,	5	},  /* Logitech */
  };
  
  for ( i=0; i < nBytes; i++) {
    /*
     * Hack for resyncing: We check here for a package that is:
     *  a) illegal (detected by wrong data-package header)
     *  b) invalid (0x80 == -128 and that might be wrong for MouseSystems)
     *  c) bad header-package
     *
     * NOTE: b) is a voilation of the MouseSystems-Protocol, since values of
     *       -128 are allowed, but since they are very seldom we can easily
     *       use them as package-header with no button pressed.
     */
    if (pBufP != 0 && 
	((rBuf[i] & proto[com_data.mtype][2]) != proto[com_data.mtype][3]
	 || rBuf[i] == 0x80)) 
      pBufP = 0;          /* skip package */

    if (pBufP == 0 &&
	(rBuf[i] & proto[com_data.mtype][0]) != proto[com_data.mtype][1])
      continue;            /* skip package */

    pBuf[pBufP++] = rBuf[i];
    if (pBufP != proto[com_data.mtype][4]) continue;

    /*
     * assembly full package
     */
    switch(com_data.mtype) {
      
    case MicroSoft:
      buttons = ((int)(pBuf[0] & 0x20) >> 3)
	| ((int)(pBuf[0] & 0x10) >> 4);
      dx = (char)(((pBuf[0] & 0x03) << 6) | (pBuf[1] & 0x3F));
      dy = (char)(((pBuf[0] & 0x0C) << 4) | (pBuf[2] & 0x3F));
      break;
      
    case MMSeries:
      buttons = pBuf[0] & 0x07;
      dx = (pBuf[0] & 0x10) ?   pBuf[1] : - pBuf[1];
      dy = (pBuf[0] & 0x08) ? - pBuf[2] :   pBuf[2];
      break;
      
    case MouseSystems:
    case Logitech:
      buttons = (~pBuf[0]) & 0x07;
      dx =   (char)pBuf[1] + (char)pBuf[3];
      dy =   -((char)pBuf[2] + (char)pBuf[4]);
      break;
    }
    generate_event(buttons, dx, dy);
    pBufP = 0;
  }
}

static void do_mouse_input(void)
{
  unsigned char buffer[BUFSIZE];
  int i, nbytes;

  do {
    nbytes = read(com_data.file, buffer, BUFSIZE);
    if (nbytes > 0) {
      process_mouse_events(buffer, nbytes);
#ifdef DEBUG
      dprintf("mouse: read %d bytes: ", nbytes);
      for (i = 0; i < nbytes; i++)
	dprintf("%.2x",buffer[i]);
      dprintf("\n");
#endif
    }
  } while (nbytes == BUFSIZE);
}

void init_mouse(void)
{
  int reg_num;
  struct device *dp;
  struct stat stat_buf;

  if ((dp = search_device(DEV_MOUSE, 0)) != NULL) {
    stat(dp->dev_real_name, &stat_buf);
    if ((stat_buf.st_mode & S_IFCHR) != 0 &&
	(com_data.file = open(dp->dev_real_name, O_RDWR|O_NONBLOCK, 0666)) != -1) {
      com_data.mtype = dp->dev_id;
      com_set_line();
#ifdef DEBUG
      dprintf("mouse: attached\n");
#endif
      for (reg_num = 0; reg_num < N_OF_COM_REGS; reg_num++) {
	define_in_port(COM_PORT_BASE + reg_num, com_port_in);
	define_out_port(COM_PORT_BASE + reg_num, com_port_out);
      }
      com_data.com_queue = create_queue(COM_IRQ);
    }
    connect_area->num_of_COMs = 1;
    add_input_handler(com_data.file, do_mouse_input);
  } else
    connect_area->num_of_COMs = 0;
}
