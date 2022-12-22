/* com.h for doscmd int14.c */

#define	BUFSIZE 1024

/* NS16550A register definitions */

/* interrupt enable register */

#define	IE_NOP		0xF0	/* not used */
#define	IE_MODEM_STAT	0x08	/* modem status int. */
#define	IE_LINE_STAT	0x04	/* receiver-line status int. */
#define	IE_TRANS_HLD	0x02	/* transmitter holding register empty int. */
#define	IE_RCV_DATA	0x01	/* received data available int. */

/* interrupt identification register */

#define	II_FIFOS_EN	0xC0	/* if FIFOs are enabled */
#define	II_NOP		0x38	/* not used */
#define	II_INT_ID	0x06	/* mask: bits see below */
#define	II_PEND_INT	0x01	/* 1=no interrupt pending */

/* bit masks for II_INT_ID */

#define	II_LINE_STAT	0x06
#define	II_RCV_DATA	0x04
#define	II_TRANS_HLD	0x02
#define	II_MODEM_STAT	0x00

/* FIFO control reg */

#define	FC_FIFO_EN	0x01

/* line control register */

#define	LC_DIV_ACC	0x80	/* divisor latch access bit */
#define	LC_BRK_CTRL	0x40	/* set break control */
#define	LC_S_PAR	0x20	/* stick parity */
#define	LC_EVEN_P	0x10	/* even parity select */
#define	LC_PAR_E	0x08	/* parity enable */
#define	LC_STOP_B	0x04	/* number of stop bits (0 - 1 bit) */
#define	LC_W_LEN	0x03	/* unsigned short length (00 - 5, 01 - 6 etc.) */

/* line status register */

#define	LS_NOP		0x80	/* not used */
#define	LS_X_SHFT_E	0x40	/* 0=data transfer, 1=transmitter idle */
#define	LS_X_HOLD_E	0x20	/* 0=ready, 1=transferring character */
#define	LS_BREAK	0x10	/* break received */
#define	LS_FRM_ERR	0x08	/* framing error */
#define	LS_PAR_ERR	0x04	/* parity error */
#define	LS_OVRN_ERR	0x02	/* overrun error */
#define	LS_RCV_DATA_RD	0x01	/* data received */

/* modem status register */

#define	MS_DCD		0x80	/* Data Carrier Detect in */
#define	MS_RI		0x40	/* Ring Indicator in */
#define	MS_DSR		0x20	/* Data Set Ready in */
#define	MS_CTS		0x10	/* Clear To Send in */
#define	MS_DELTA_DCD	0x08	/* Data Carrier Detect changed state */
#define	MS_DELTA_RI	0x04	/* Ring Indicator changed state */
#define	MS_DELTA_DSR	0x02	/* Data Set Ready changed state */
#define	MS_DELTA_CTS	0x01	/* Clear To Send changed state */

/* data structure definitions */

#define	N_OF_COM_REGS	8

struct com_data_struct {
	int		fd;		/* BSD/386 file descriptor */
	char		*path;		/* BSD/386 pathname */
	unsigned char	*addr;		/* ISA I/O address */
	unsigned char	irq;		/* ISA IRQ */
	unsigned char	flags;		/* some general software flags */

	struct queue	*com_queue;	/* XXX DEBUG obsolete MCL? */

	unsigned char	div_latch[2];	/* mirror of 16550 R0':R1' read/write */
	unsigned char	last_char_read;	/* mirror of 16550 R0 read only */
	unsigned char	int_enable;	/* mirror of 16550 R1 read/write */
	unsigned char	int_id;		/* mirror of 16550 R2 read only */
	unsigned char	fifo_ctrl;	/* mirror of 16550 R2 write only */
	unsigned char	line_ctrl;	/* mirror of 16550 R3 read/write */
	unsigned char	modem_ctrl;	/* mirror of 16550 R4 read/write */
	unsigned char	line_stat;	/* mirror of 16550 R5 read/write */
	unsigned char	modem_stat;	/* mirror of 16550 R6 read/write */
	unsigned char	uart_spare;	/* mirror of 16550 R7 read/write */
};

/* DOS definitions -- parameters */

#define	BITRATE_110	0x00
#define	BITRATE_150	0x20
#define	BITRATE_300	0x40
#define	BITRATE_600	0x60
#define	BITRATE_1200	0x80
#define	BITRATE_2400	0xA0
#define	BITRATE_4800	0xC0
#define	BITRATE_9600	0xE0
#define	PARITY_NONE	0x00
#define	PARITY_ODD	0x08
#define	PARITY_EVEN	0x18
#define	STOPBIT_1	0x00
#define	STOPBIT_2	0x04
#define	TXLEN_7BITS	0x02
#define	TXLEN_8BITS	0x03

/* DOS definitions -- return codes */

#define	LS_SW_TIME_OUT	LS_NOP	/* return value used by DOS */

/* miscellaneous definitions */

#define	DIV_LATCH_LOW	0
#define	DIV_LATCH_HIGH	1

#define	DIV_LATCH_LOW_WRITTEN	0x01
#define	DIV_LATCH_HIGH_WRITTEN	0x02
#define	DIV_LATCH_BOTH_WRITTEN	0x03

/* variable declarations */

extern int errno;

/* routine declarations */

void int14(struct trapframe *);
void com_set_line(struct com_data_struct *, unsigned char, unsigned char);
void init_com(int, char *, unsigned char *, unsigned char);
unsigned char com_port_in(unsigned char *);
void com_port_out(unsigned char *, unsigned char);

/* end of file com.h */
