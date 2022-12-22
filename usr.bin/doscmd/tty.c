/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	Krystal $Id: tty.c,v 1.2 1994/01/15 21:20:09 polk Exp $ */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <termios.h>
#include <limits.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include "doscmd.h"
#include "video.h"
#include "font.h"

static struct termios save = { 0 };
static struct termios raw = { 0 };
static int flags = 0;
static int mode = -1;
#define	vmem		((u_short *)0xB8000)
static int blink = 1;
int flipdelete = 1;		/* Flip meaning of delete and backspace */

static Display *dpy = 0;
static Window win;
static XFontStruct *font;
static unsigned long black;
static unsigned long white;
static unsigned long pixels[16];
static char *color_names[16] = {
    "black",
    "medium blue",
    "olive drab",
    "turquoise3",
    "red1",
    "maroon",
    "goldenrod4",
    "gray80",
    "gray50",
    "sky blue",
    "spring green",
    "PaleTurquoise2",
    "pink",
    "violet red",
    "yellow1",
    "white",
};

static int FH;
static int FW;
static GC gc;
static GC cgc;
static int xfd;
static int vattr = 0x0700;

static int width = 80;
static int height = 25;

typedef struct TextLine {
	u_short	*data;
	u_char	max_length;	/* Not used, but here for future use */
	u_char	changed:1;
} TextLine;
static TextLine *lines;

/*
 * 0040:0050 is a list of 16 bytes, 2 for each page, which contains
 * the row and column of each page
 */
#define	row BIOSDATA[0x50]
#define	col BIOSDATA[0x51]

u_char	*VREG;

inline SetVREGCur()
{
	int cp = row * width + col;
	VREG[MVC_CurHigh] = cp >> 8;
	VREG[MVC_CurLow] = cp & 0xff;
}

/*
 * 0040:0060 contains the start and end of the cursor
 */
#define	curs_end BIOSDATA[0x60]
#define	curs_start BIOSDATA[0x61]

#define	video_pate	BISODATA[0x62]

#define	IBMFONT	"ibmpc"

char *xfont = 0;

void video_async_event();
void video_event();
void tty_cooked();
void video_update();
unsigned char video_inb(int);
void video_outb(int, unsigned char);


#define	PEEKSZ	16

#define	K_NEXT		*(u_short *)0x41a
#define	K_FREE		*(u_short *)0x41c
#define	K_BUFSTARTP	*(u_short *)0x480
#define	K_BUFENDP	*(u_short *)0x482
#define	K_BUFSTART	((u_short *)(0x400 | *(u_short *)0x480))
#define	K_BUFEND	((u_short *)(0x400 | *(u_short *)0x482))

#define	K1_STATUS	BIOSDATA[0x17]
#define	K1_RSHIFT	0x01
#define	K1_LSHIFT	0x02
#define	K1_SHIFT	0x03
#define	K1_CTRL		0x04
#define	K1_ALT		0x08
#define	K1_SLOCK	0x10
#define	K1_NLOCK	0x20
#define	K1_CLOCK	0x40
#define	K1_INSERT	0x80

#define	K2_STATUS	BIOSDATA[0x18]
#define	K2_LCTRL	0x01
#define	K2_LALT		0x02
#define	K2_SYSREQ	0x04
#define	K2_PAUSE	0x08
#define	K2_SLOCK	0x10
#define	K2_NLOCK	0x20
#define	K2_CLOCK	0x40
#define	K2_INSERT	0x80

#define	K3_STATUS	BIOSDATA[0x96]
#define	K3_E1		0x01		/* Last code read was e1 */
#define	K3_E2		0x02		/* Last code read was e2 */
#define	K3_RCTRL	0x04
#define	K3_RALT		0x08
#define	K3_ENHANCED	0x10
#define	K3_FORCENLOCK	0x20
#define	K3_TWOBYTE	0x40		/* last code was first of 2 */
#define	K3_READID	0x80		/* read ID in progress */

#define	K4_STATUS	BIOSDATA[0x97]
#define	K4_SLOCK_LED	0x01
#define	K4_NLOCK_LED	0x02
#define	K4_CLOCK_LED	0x04
#define	K4_ACK		0x10		/* ACK recieved from keyboard */
#define	K4_RESEND	0x20		/* RESEND recieved from keyboard */
#define	K4_LED		0x40		/* LED update in progress */
#define	K4_ERROR	0x80

struct VideoSavePointerTable {
	u_short	video_parameter_tabel[2];
	u_short	parameter_dynamic_save_area[2];		/* Not used */
	u_short	alphanumeric_character_set_override[2];	/* Not used */
	u_short	graphics_character_set_override[2];	/* Not used */
	u_short	secondary_save_pointer_table[2];	/* Not used */
	u_short	mbz[4];
};

struct SecondaryVideoSavePointerTable {
	u_short	length;
	u_short	display_combination_code_table[2];
	u_short	alphanumeric_character_set_override[2];	/* Not used */
	u_short	user_palette_profile_table[2];		/* Not used */
	u_short	mbz[6];
};

int KbdEmpty();
void KbdWrite(u_short code);
void KbdRepl(u_short code);
u_short KbdRead();
u_short KbdPeek();

int redirect0;
int redirect1;
int redirect2;

static void
Failure()
{
        fprintf(stderr, "X Connection shutdown\n");
	exit(1);
}

struct VideoSavePointerTable *vsp;
struct SecondaryVideoSavePointerTable *svsp;

#include "vparams.h"

video_bios_init()
{
	u_char *p;
	/*
	 * Assure video bios is zeroed out
	 */
	memset(vsp, 0, sizeof(struct VideoSavePointerTable));

	/*
	 * Put the Video Save Pointer table @ C000:0000
	 * Put the Secondary Video Save Pointer table @ C000:0020
	 * Put the Display Combination code table @ C000:0040
	 * Put the Video Parameter table @ C000:1000 - C000:2FFF
	 * Put the default Font @ C000:3000 - C000:3FFF
	 */
	
	*(u_long *)&BIOSDATA[0xA8] = 0xC0000000; /* 0040:00A8 points to us */

	vsp = (struct VideoSavePointerTable *)0xC0000L;
	svsp = (struct SecondaryVideoSavePointerTable *)0xC0020L;

	vsp->video_parameter_tabel[0] = 0x1000;
	vsp->video_parameter_tabel[1] = 0xC000;

	vsp->secondary_save_pointer_table[0] = 0x0020;
	vsp->secondary_save_pointer_table[1] = 0xC000;

	svsp->display_combination_code_table[0] = 0x0040;
	svsp->display_combination_code_table[1] = 0xC000;

	p = (u_char *)0xC0040;
	*p++ = 2;		/* Only support 2 combinations currently */
	*p++ = 1;		/* Version # */
	*p++ = 8;		/* We wont use more than type 8 */
	*p++ = 0;		/* Resereved */
	*p++ = 0; *p++ = 0;	/* No Display No Display */
	*p++ = 0; *p++ = 8;	/* No Display VGA Color */

	memcpy((void *)0xC1000, videoparams, sizeof(videoparams));

	ivec[0x1d] = 0xC0001000L;	/* Video Parameter Table */
	ivec[0x1e] = 0xC0003000L;	
	ivec[0x42] = ivec[0x10];	/* Copy of video interrupt */
        memcpy((void *)0xC3000L, ascii_font, sizeof(ascii_font));

                     BIOSDATA[0x49] = 3;        /* Video Mode */
        *(u_short *)&BIOSDATA[0x4a] = 80;       /* Columns on Screen */
        *(u_short *)&BIOSDATA[0x4c] = 0;        /* Page */
        *(u_short *)&BIOSDATA[0x4e] = 0;        /* Offset into video memory */
        *(u_short *)&BIOSDATA[0x63] = 0x03d4;   /* controller base reg */
                     BIOSDATA[0x84] = 24;       /* Rows on screen */
        *(u_short *)&BIOSDATA[0x85] = 16;       /* font height */
                     BIOSDATA[0x87] = 0;        /* video ram etc. */
                     BIOSDATA[0x88] = 0xf9;     /* video switches */
                     BIOSDATA[0x89] = 0x11;     /* video stats */
                     BIOSDATA[0x8a] = 1;        /* Index into DCC table */
		     BIOSDATA[0x96] = 0x10;	
}

void
video_init()
{
    XSizeHints sh;
    XGCValues gcv;
    XColor ccd, rgb;
    unsigned long mask;
    int level;
    int i, j;

    /*
     * Define all known I/O port handlers
     */
    define_input_port_handler(CGA_Status, video_inb);
    define_input_port_handler(0x03c2, video_inb);
    define_input_port_handler(0x03cc, video_inb);
    define_input_port_handler(CVC_Data, video_inb);
    define_output_port_handler(CGA_Control, video_outb);
    define_output_port_handler(0x03c2, video_outb);
    define_output_port_handler(0x03cc, video_outb);
    define_output_port_handler(CVC_Address, video_outb);
    define_output_port_handler(CVC_Data, video_outb);

    redirect0 = isatty(0) == 0 || !xmode ;
    redirect1 = isatty(1) == 0 || !xmode ;
    redirect2 = isatty(2) == 0 || !xmode ;

    K_BUFSTARTP = 0x1e;	/* Start of keyboard buffer */
    K_BUFENDP = 0x3e;	/* End of keyboard buffer */
    K_NEXT = K_FREE = 0x00;

    /*
     * Initialize video memory with black background, white foreground
     */
    for (i = 0; i < height * width; ++i)
	vmem[i] = vattr;

    if (!xmode)
	return;

    if (!(lines = (TextLine *)malloc(sizeof(TextLine) * height))) {
	fprintf(stderr, "Could not allocate data structure for text lines\n");
	exit(1);
    }
    for (i = 0; i < height; ++i) {
	lines[i].max_length = width;
	if (!(lines[i].data = (u_short *)malloc(width * sizeof(u_short)))) {
	    fprintf(stderr,
		    "Could not allocate data structure for text lines\n");
	    exit(1);
	}
	lines[i].changed = 1;
    }


    dpy = XOpenDisplay(NULL);

    if (dpy == NULL) {
	    fprintf(stderr, "Could not open display ``%s''\n",
		    XDisplayName(NULL));
	    exit(1);
    }
    xfd = ConnectionNumber(dpy);
    xfd = squirrel_fd(xfd);
    ConnectionNumber(dpy) = xfd;

    _RegisterIO(xfd, video_async_event, xfd, Failure);
    _BlockIO();

    pixels[0] = BlackPixel(dpy, DefaultScreen(dpy));
    pixels[15] = WhitePixel(dpy, DefaultScreen(dpy));
    for (i = 0; i < 15; ++i) {
	    if (XAllocNamedColor(dpy,
				 DefaultColormap(dpy, DefaultScreen(dpy)),
				 color_names[i], &ccd, &rgb)) {
		    pixels[i] = ccd.pixel;
	    } else if (i < 7)
		    pixels[i] = pixels[0];
	    else
		    pixels[i] = pixels[15];
    }

    if (!xfont)
	xfont = IBMFONT;

    font = XLoadQueryFont(dpy, xfont);

    if (font == NULL)
	font = XLoadQueryFont(dpy, IBMFONT);

    if (font == NULL) {
	    fprintf(stderr, "Could not open font ``%s''\n", xfont);
	    exit(1);
    }

    FW = font->max_bounds.width;
    FH = font->max_bounds.ascent + font->max_bounds.descent;

    curs_start = FH - 2;
    curs_end = FH - 1;

    sh.width = FW * width + 4;
    sh.height = FH * height + 4;

    sh.width += 4;
    sh.height += 4;

    sh.min_width = sh.max_width = sh.width;
    sh.min_height = sh.max_height = sh.height;

    sh.flags = USSize | PMinSize | PMaxSize | PSize;

    win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0,
			      sh.width, sh.height, 2, black, black);
    if (win == NULL) {
	    fprintf(stderr, "Could not create window\n");
	    exit(1);
    }

    gcv.foreground = white;
    gcv.background = black;
    gcv.font = font->fid;

    mask = GCForeground | GCBackground | GCFont;

    gc = XCreateGC(dpy, win, mask, &gcv);

    gcv.foreground = 1;
    gcv.background = 0;
    gcv.function = GXxor;
    cgc = XCreateGC(dpy, win, GCForeground|GCBackground|GCFunction, &gcv);

    XSetNormalHints(dpy, win, &sh);
    XSelectInput(dpy, win, KeyReleaseMask | KeyPressMask |
			   ExposureMask | ButtonPressMask);
    

    XMapWindow(dpy, win);
    XFlush(dpy);

    {
	struct sigaction sa;
	struct itimerval itv;

	itv.it_interval.tv_sec = 0;
	itv.it_interval.tv_usec = (1000*1000*10L)/182;/* 18.2 times a second */
	itv.it_value.tv_sec = 0;
	itv.it_value.tv_usec =    (1000*1000*10L)/182;

	sa.sa_handler = video_update;
	sa.sa_mask = sigmask(SIGIO) | sigmask(SIGALRM);
	sa.sa_flags = SA_ONSTACK;
	sigaction(SIGALRM, &sa, NULL);	/**/
	setitimer(ITIMER_REAL, &itv, 0);
    }
    _UnblockIO();
}

video_reinit()
{
    if (xmode) {
	struct sigaction sa;
	struct itimerval itv;

	itv.it_interval.tv_sec = 0;
	itv.it_interval.tv_usec = (1000*1000*10L)/182;/* 18.2 times a second */
	itv.it_value.tv_sec = 0;
	itv.it_value.tv_usec =    (1000*1000*10L)/182;

	sa.sa_handler = video_update;
	sa.sa_mask = sigmask(SIGIO) | sigmask(SIGALRM);
	sa.sa_flags = SA_ONSTACK;
	sigaction(SIGALRM, &sa, NULL);	/**/
	setitimer(ITIMER_REAL, &itv, 0);

        _RegisterIO(xfd, video_async_event, xfd, Failure);
    }
}

void
video_setborder(int color)
{
	_BlockIO();
	XSetWindowBackground(dpy, win, pixels[color & 0xf]);
	_UnblockIO();
}
void
video_blink(int mode)
{
	blink = mode;
}

static int show = 1;

setgc(u_short attr)
{
	XGCValues v;
	if (blink && !show && (attr & 0x8000))
		v.foreground = pixels[(attr >> 12) & 0x07];
	else
		v.foreground = pixels[(attr >> 8) & 0x0f];

	v.background = pixels[(attr >> 12) & (blink ? 0x07 : 0x0f)];
#if 0
    	if (v.foreground == v.background) {
		v.foreground = pixels[15];
		v.background = pixels[0];
	}
#endif
	XChangeGC(dpy, gc, GCForeground|GCBackground, &v);
}

void
video_update(struct sigframe sf, struct trapframe tf)
{
    	static int or = -1;
    	static int oc = -1;

	static int icnt = 4;
    	extern int int_state;

	static char buf[256];
	int r, c;
	int sc;
	int attr = vmem[0] & 0xff00;
	XGCValues v;

	if (--icnt == 0) {
		icnt = 4;
	    show ^= 1;

	    wakeup_poll();	/* Wake up anyone waiting on kbd poll */

    	    setgc(attr);

	    for (r = 0; r < height; ++r) {
		    int sc = 0;

		    if (!lines[r].changed) {
		    	if ((r == or || r == row) && (or != row || oc != col))
			    lines[r].changed = 1;
			else {
			    for (c = 0; c < width; ++c) {
			    	if (lines[r].data[c] != vmem[r * width + c]) {
				    lines[r].changed = 1;
				    break;
				}
				if (blink && lines[r].data[c] & 0x8000) {
				    lines[r].changed = 1;
				    break;
    	    	    	    	}
			    }
			}
		    }

		    if (!lines[r].changed)
		    	continue;

		    lines[r].changed = 0;
		    memcpy(lines[r].data,
			   &vmem[r * width], sizeof(u_short) * width);

		    for (c = 0; c < width; ++c) {
			    int cv = vmem[r * width + c];
			    if ((cv & 0xff00) != attr) {
				if (sc < c)
				    XDrawImageString(dpy, win, gc,
						     2 + sc * FW,
						     2 + (r + 1) * FH,
						     buf + sc, c - sc);
				sc = c;
				attr = cv  & 0xff00;
				setgc(attr);
			    }
			    buf[c] = (cv & 0xff) ? cv & 0xff : ' ';
		    }
		    if (sc < c) {
			    XDrawImageString(dpy, win, gc,
					     2 + sc * FW,
					     2 + (r + 1) * FH,
					     buf + sc, c - sc);
		    }
	    }
	    or = row;
    	    oc = col;

    	    if (curs_start >= 0 && curs_start <= curs_end && curs_end <= FH &&
	        show && row >= 0 && row < height && col >= 0 && col < width) {
		    attr = vmem[row * width + col] & 0xff00;
		    v.foreground = pixels[(attr >> 8) & 0x0f] ^
				   pixels[(attr >> 12) & (blink ? 0x07 : 0x0f)];
		    if (v.foreground) {
			    v.function = GXxor;
		    } else {
			    v.foreground = pixels[7];
			    v.function = GXcopy;
		    }
		    XChangeGC(dpy, cgc, GCForeground | GCFunction, &v);
		    XFillRectangle(dpy, win, cgc,
				   2 + col * FW,
				   2 + row * FH + curs_start,
				   FW, curs_end + 1 - curs_start);
	    }
	    
	    XFlush(dpy);
	}

    	*(u_long *)&BIOSDATA[0x6c] += 1;    /* Timer ticks since midnight... */
    	while (*(u_long *)&BIOSDATA[0x6c] >= 24*60*6*182) {
	    *(u_long *)&BIOSDATA[0x6c] -= 24*60*6*182;
	    BIOSDATA[0x70]++;		    /* BIOSDATA[0x70] # times past mn */
	}

	if ((sf.sf_sc.sc_ps & PSL_VM) && (tf.tf_eflags & PSL_VM)) {
	    if (int_state & PSL_I) {
		if ((ivec[8] >> 16) != 0xF000) {
		    debug (D_TRAPS|0x08, "int%x:%x [%04x:%04x]\n",
				    8, tf.tf_ax >> 8,
				    ivec[8] >> 16, ivec[8] & 0xffff);

		    PUSH(tf.tf_eflags, &tf);
		    PUSH(tf.tf_cs, &tf);
		    PUSH(tf.tf_ip, &tf);
		    tf.tf_cs = ivec[8] >> 16;
		    tf.tf_ip = ivec[8] & 0xffff;
		} else if ((ivec[0x1c] >> 16) != 0xF000) {
		    debug (D_TRAPS|0x1c, "int%x:%x [%04x:%04x]\n",
				    0x1c, tf.tf_ax >> 8,
				    ivec[0x1c] >> 16, ivec[0x1c] & 0xffff);
		    PUSH(tf.tf_eflags, &tf);
		    PUSH(tf.tf_cs, &tf);
		    PUSH(tf.tf_ip, &tf);
		    tf.tf_cs = ivec[0x1c] >> 16;
		    tf.tf_ip = ivec[0x1c] & 0xffff;
		}
	    }
	    switch_vm86 (sf, tf);
	}
}

void
video_async_event(int fd)
{
	for (;;) {
                int x;
                fd_set fdset;
                XEvent ev;  
                static struct timeval tv = { 0 };
 
                /*
                 * Handle any events just sitting around...
                 */
                XFlush(dpy);
                while (QLength(dpy) > 0) {
                        XNextEvent(dpy, &ev);
                        video_event(&ev);
                }

                FD_ZERO(&fdset);
                FD_SET(fd, &fdset);

                x = select(FD_SETSIZE, &fdset, 0, 0, &tv);

                switch (x) {  
                case -1:
                        /*
                         * Errno might be wrong, so we just select again.
                         * This could cause a problem is something really
                         * was wrong with select....
                         */
                        perror("select");
                        return;
                case 0:
                        return;
                default:
                        if (FD_ISSET(fd, &fdset)) {
                                do {
                                        XNextEvent(dpy, &ev);
                                        video_event(&ev);
                                } while (QLength(dpy));
                        }
                        break;
                }
        }
}


static u_short Ascii2Scan[] = {
 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
 0x000e, 0x000f, 0xffff, 0xffff, 0xffff, 0x001c, 0xffff, 0xffff,
 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff,
 0xffff, 0xffff, 0xffff, 0x0001, 0xffff, 0xffff, 0xffff, 0xffff,
 0x0039, 0x0102, 0x0128, 0x0104, 0x0105, 0x0106, 0x0108, 0x0028,
 0x010a, 0x010b, 0x0109, 0x010d, 0x0033, 0x000c, 0x0034, 0x0035,
 0x000b, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007, 0x0008,
 0x0009, 0x000a, 0x0127, 0x0027, 0x0133, 0x000d, 0x0134, 0x0135,
 0x0103, 0x011e, 0x0130, 0x012e, 0x0120, 0x0112, 0x0121, 0x0122,
 0x0123, 0x0117, 0x0124, 0x0125, 0x0126, 0x0132, 0x0131, 0x0118,
 0x0119, 0x0110, 0x0113, 0x011f, 0x0114, 0x0116, 0x012f, 0x0111,
 0x012d, 0x0115, 0x012c, 0x001a, 0x002b, 0x001b, 0x0107, 0x010c,
 0x0029, 0x001e, 0x0030, 0x002e, 0x0020, 0x0012, 0x0021, 0x0022,
 0x0023, 0x0017, 0x0024, 0x0025, 0x0026, 0x0032, 0x0031, 0x0018,
 0x0019, 0x0010, 0x0013, 0x001f, 0x0014, 0x0016, 0x002f, 0x0011,
 0x002d, 0x0015, 0x002c, 0x011a, 0x012b, 0x011b, 0x0129, 0xffff,
};

struct {
    u_short	base;
    u_short	shift;
    u_short	ctrl;
    u_short	alt;
} ScanCodes[] = {
    {	0xffff, 0xffff, 0xffff, 0xffff }, /* key 0 */
    {	0x011b, 0x011b, 0x011b, 0xffff }, /* key  1 - Escape key */
    {	0x0231, 0x0221, 0xffff, 0x7800 }, /* key  2 - '1' */
    {	0x0332, 0x0340, 0x0300, 0x7900 }, /* key  3 - '2' - special handling */
    {	0x0433, 0x0423, 0xffff, 0x7a00 }, /* key  4 - '3' */
    {	0x0534, 0x0524, 0xffff, 0x7b00 }, /* key  5 - '4' */
    {	0x0635, 0x0625, 0xffff, 0x7c00 }, /* key  6 - '5' */
    {	0x0736, 0x075e, 0x071e, 0x7d00 }, /* key  7 - '6' */
    {	0x0837, 0x0826, 0xffff, 0x7e00 }, /* key  8 - '7' */
    {	0x0938, 0x092a, 0xffff, 0x7f00 }, /* key  9 - '8' */
    {	0x0a39, 0x0a28, 0xffff, 0x8000 }, /* key 10 - '9' */
    {	0x0b30, 0x0b29, 0xffff, 0x8100 }, /* key 11 - '0' */
    {	0x0c2d, 0x0c5f, 0x0c1f, 0x8200 }, /* key 12 - '-' */
    {	0x0d3d, 0x0d2b, 0xffff, 0x8300 }, /* key 13 - '=' */
    {	0x0e08, 0x0e08, 0x0e7f, 0xffff }, /* key 14 - backspace */
    {	0x0f09, 0xffff, 0xffff, 0xffff }, /* key 15 - tab */
    {	0x1071, 0x1051, 0x1011, 0x1000 }, /* key 16 - 'Q' */
    {	0x1177, 0x1157, 0x1117, 0x1100 }, /* key 17 - 'W' */
    {	0x1265, 0x1245, 0x1205, 0x1200 }, /* key 18 - 'E' */
    {	0x1372, 0x1352, 0x1312, 0x1300 }, /* key 19 - 'R' */
    {	0x1474, 0x1454, 0x1414, 0x1400 }, /* key 20 - 'T' */
    {	0x1579, 0x1559, 0x1519, 0x1500 }, /* key 21 - 'Y' */
    {	0x1675, 0x1655, 0x1615, 0x1600 }, /* key 22 - 'U' */
    {	0x1769, 0x1749, 0x1709, 0x1700 }, /* key 23 - 'I' */
    {	0x186f, 0x184f, 0x180f, 0x1800 }, /* key 24 - 'O' */
    {	0x1970, 0x1950, 0x1910, 0x1900 }, /* key 25 - 'P' */
    {	0x1a5b, 0x1a7b, 0x1a1b, 0xffff }, /* key 26 - '[' */
    {	0x1b5d, 0x1b7d, 0x1b1d, 0xffff }, /* key 27 - ']' */
    {	0x1c0d, 0x1c0d, 0x1c0a, 0xffff }, /* key 28 - CR */
    {	0xffff, 0xffff, 0xffff, 0xffff }, /* key 29 - control shift */
    {	0x1e61, 0x1e41, 0x1e01, 0x1e00 }, /* key 30 - 'A' */
    {	0x1f73, 0x1f53, 0x1f13, 0x1f00 }, /* key 31 - 'S' */
    {	0x2064, 0x2044, 0x2004, 0x2000 }, /* key 32 - 'D' */
    {	0x2166, 0x2146, 0x2106, 0x2100 }, /* key 33 - 'F' */
    {	0x2267, 0x2247, 0x2207, 0x2200 }, /* key 34 - 'G' */
    {	0x2368, 0x2348, 0x2308, 0x2300 }, /* key 35 - 'H' */
    {	0x246a, 0x244a, 0x240a, 0x2400 }, /* key 36 - 'J' */
    {	0x256b, 0x254b, 0x250b, 0x2500 }, /* key 37 - 'K' */
    {	0x266c, 0x264c, 0x260c, 0x2600 }, /* key 38 - 'L' */
    {	0x273b, 0x273a, 0xffff, 0xffff }, /* key 39 - ';' */
    {	0x2827, 0x2822, 0xffff, 0xffff }, /* key 40 - ''' */
    {	0x2960, 0x297e, 0xffff, 0xffff }, /* key 41 - '`' */
    {	0xffff, 0xffff, 0xffff, 0xffff }, /* key 42 - left shift */
    {	0x2b5c, 0x2b7c, 0x2b1c, 0xffff }, /* key 43 - '' */
    {	0x2c7a, 0x2c5a, 0x2c1a, 0x2c00 }, /* key 44 - 'Z' */
    {	0x2d78, 0x2d58, 0x2d18, 0x2d00 }, /* key 45 - 'X' */
    {	0x2e63, 0x2e43, 0x2e03, 0x2e00 }, /* key 46 - 'C' */
    {	0x2f76, 0x2f56, 0x2f16, 0x2f00 }, /* key 47 - 'V' */
    {	0x3062, 0x3042, 0x3002, 0x3000 }, /* key 48 - 'B' */
    {	0x316e, 0x314e, 0x310e, 0x3100 }, /* key 49 - 'N' */
    {	0x326d, 0x324d, 0x320d, 0x3200 }, /* key 50 - 'M' */
    {	0x332c, 0x333c, 0xffff, 0xffff }, /* key 51 - ',' */
    {	0x342e, 0x343e, 0xffff, 0xffff }, /* key 52 - '.' */
    {	0x352f, 0x353f, 0xffff, 0xffff }, /* key 53 - '/' */
    {	0xffff, 0xffff, 0xffff, 0xffff }, /* key 54 - right shift - */
    {	0x372a, 0xffff, 0x3772, 0xffff }, /* key 55 - prt-scr - */
    {	0xffff, 0xffff, 0xffff, 0xffff }, /* key 56 - Alt - */
    {	0x3920, 0x3920, 0x3920, 0x3920 }, /* key 57 - space bar */
    {	0xffff, 0xffff, 0xffff, 0xffff }, /* key 58 - caps-lock -  */
    {	0x3b00, 0x5400, 0x5e00, 0x6800 }, /* key 59 - F1 */
    {	0x3c00, 0x5500, 0x5f00, 0x6900 }, /* key 60 - F2 */
    {	0x3d00, 0x5600, 0x6000, 0x6a00 }, /* key 61 - F3 */
    {	0x3e00, 0x5700, 0x6100, 0x6b00 }, /* key 62 - F4 */
    {	0x3f00, 0x5800, 0x6200, 0x6c00 }, /* key 63 - F5 */
    {	0x4000, 0x5900, 0x6300, 0x6d00 }, /* key 64 - F6 */
    {	0x4100, 0x5a00, 0x6400, 0x6e00 }, /* key 65 - F7 */
    {	0x4200, 0x5b00, 0x6500, 0x6f00 }, /* key 66 - F8 */
    {	0x4300, 0x5c00, 0x6600, 0x7000 }, /* key 67 - F9 */
    {	0x4400, 0x5d00, 0x6700, 0x7100 }, /* key 68 - F10 */
    {	0xffff, 0xffff, 0xffff, 0xffff }, /* key 69 - num-lock - */
    {	0xffff, 0xffff, 0xffff, 0xffff }, /* key 70 - scroll-lock -  */
    {	0x4700, 0x4737, 0x7700, 0xffff }, /* key 71 - home */
    {	0x4800, 0x4838, 0xffff, 0xffff }, /* key 72 - cursor up */
    {	0x4900, 0x4939, 0x8400, 0xffff }, /* key 73 - page up */
    {	0x2d00, 0x4a2d, 0xffff, 0xffff }, /* key 74 - minus sign */
    {	0x4b00, 0x4b34, 0x7300, 0xffff }, /* key 75 - cursor left */
    {	0xffff, 0x4c35, 0xffff, 0xffff }, /* key 76 - center key */
    {	0x4d00, 0x4d36, 0x7400, 0xffff }, /* key 77 - cursor right */
    {	0x2b00, 0x4e2b, 0xffff, 0xffff }, /* key 78 - plus sign */
    {	0x4f00, 0x4f31, 0x7500, 0xffff }, /* key 79 - end */
    {	0x5000, 0x5032, 0xffff, 0xffff }, /* key 80 - cursor down */
    {	0x5100, 0x5133, 0x7600, 0xffff }, /* key 81 - page down */
    {	0x5200, 0x5230, 0xffff, 0xffff }, /* key 82 - insert */
    {	0x5300, 0x532e, 0xffff, 0xffff }, /* key 83 - delete */
    {	0xffff, 0xffff, 0xffff, 0xffff }, /* key 84 - sys key */
    {	0xffff, 0xffff, 0xffff, 0xffff }, /* key 85 */
    {	0xffff, 0xffff, 0xffff, 0xffff }, /* key 86 */
    {	0x8500, 0x5787, 0x8900, 0x8b00 }, /* key 87 - F11 */
    {	0x8600, 0x5888, 0x8a00, 0x8c00 }, /* key 88 - F12 */
};

void
video_event(XEvent *ev)
{
	switch (ev->type) {
	case ButtonPress:
		if ((K1_STATUS & (K1_ALT|K1_CTRL)) == (K1_ALT|K1_CTRL))
		    exit(0);
		break;
        case NoExpose:
                break;
        case GraphicsExpose:
        case Expose: {
		int r;
		for (r = 0; r < height; ++r)
		    lines[r].changed = 1;
		break;
	    }
	case KeyRelease: {
		static char buf[128];
		KeySym ks;
		int n;

    	    	if (!(ev->xkey.state & ShiftMask)) {
		    K1_STATUS &= ~K1_LSHIFT;
		    K1_STATUS &= ~K1_RSHIFT;
		}
    	    	if (!(ev->xkey.state & ControlMask)) {
			K1_STATUS &= ~K1_CTRL;
			K2_STATUS &= ~K2_LCTRL;
			K3_STATUS &= ~K3_RCTRL;
		}
    	    	if (!(ev->xkey.state & Mod1Mask)) {
                        K1_STATUS &= ~K1_ALT;
                        K2_STATUS &= ~K2_LALT;
                        K3_STATUS &= ~K3_RALT;
		}
    	    	if (!(ev->xkey.state & LockMask)) {
                        K2_STATUS &= ~K2_CLOCK;
		}

		XLookupString((XKeyEvent *)ev, buf, sizeof(buf), &ks, 0);
		switch (ks) {
		case XK_Shift_L:
			K1_STATUS &= ~K1_LSHIFT;
			break;
		case XK_Shift_R:
			K1_STATUS &= ~K1_RSHIFT;
			break;
		case XK_Control_L:
			K1_STATUS &= ~K1_CTRL;
			K2_STATUS &= ~K2_LCTRL;
			break;
		case XK_Control_R:
			K1_STATUS &= ~K1_CTRL;
			K3_STATUS &= ~K3_RCTRL;
			break;
		case XK_Alt_L:
			K1_STATUS &= ~K1_ALT;
			K2_STATUS &= ~K2_LALT;
			break;
		case XK_Alt_R:
			K1_STATUS &= ~K1_ALT;
			K3_STATUS &= ~K3_RALT;
			break;
		case XK_Scroll_Lock:
			K2_STATUS &= ~K2_SLOCK;
			break;
		case XK_Num_Lock:
			K2_STATUS &= ~K2_NLOCK;
			break;
		case XK_Caps_Lock:
			K2_STATUS &= ~K2_CLOCK;
			break;
		case XK_Insert:
			K2_STATUS &= ~K2_INSERT;
			break;
		}
		break;
	    }
	case KeyPress: {
		static char buf[128];
		KeySym ks;
		int n;
    	    	int nlock = 0;
		u_short scan = 0xffff;

    	    	if (!(ev->xkey.state & ShiftMask)) {
		    K1_STATUS &= ~K1_LSHIFT;
		    K1_STATUS &= ~K1_RSHIFT;
		}
    	    	if (!(ev->xkey.state & ControlMask)) {
			K1_STATUS &= ~K1_CTRL;
			K2_STATUS &= ~K2_LCTRL;
			K3_STATUS &= ~K3_RCTRL;
		}
    	    	if (!(ev->xkey.state & Mod1Mask)) {
                        K1_STATUS &= ~K1_ALT;
                        K2_STATUS &= ~K2_LALT;
                        K3_STATUS &= ~K3_RALT;
		}
    	    	if (!(ev->xkey.state & LockMask)) {
                        K2_STATUS &= ~K2_CLOCK;
		}

		n = XLookupString((XKeyEvent *)ev, buf, sizeof(buf), &ks, 0);

		switch (ks) {
		case XK_Shift_L:
			K1_STATUS |= K1_LSHIFT;
			break;
		case XK_Shift_R:
			K1_STATUS |= K1_RSHIFT;
			break;
		case XK_Control_L:
			K1_STATUS |= K1_CTRL;
			K2_STATUS |= K2_LCTRL;
			break;
		case XK_Control_R:
			K1_STATUS |= K1_CTRL;
			K3_STATUS |= K3_RCTRL;
			break;
		case XK_Alt_L:
			K1_STATUS |= K1_ALT;
			K2_STATUS |= K2_LALT;
			break;
		case XK_Alt_R:
			K1_STATUS |= K1_ALT;
			K3_STATUS |= K3_RALT;
			break;
		case XK_Scroll_Lock:
			K1_STATUS ^= K1_SLOCK;
			K2_STATUS |= K2_SLOCK;
			break;
		case XK_Num_Lock:
			K1_STATUS ^= K1_NLOCK;
			K2_STATUS |= K2_NLOCK;
			break;
		case XK_Caps_Lock:
			K1_STATUS ^= K1_CLOCK;
			K2_STATUS |= K2_CLOCK;
			break;
		case XK_Insert:
			K1_STATUS ^= K1_INSERT;
			K2_STATUS |= K2_INSERT;
			scan = 82;
			goto docode;

		case XK_Escape:
			scan = 1;
			goto docode;

		case XK_Tab:
			scan = 15;
			goto docode;
			
    	    	case XK_Return:
		case XK_KP_Enter:
			scan = 28;
		    	goto docode;

    	    	case XK_Print:
			scan = 55;
			goto docode;

		case XK_F1:
		case XK_F2:
		case XK_F3:
		case XK_F4:
		case XK_F5:
		case XK_F6:
		case XK_F7:
		case XK_F8:
		case XK_F9:
		case XK_F10:
			scan = ks - XK_F1 + 59;
			goto docode;

    	    	case XK_KP_7:
			nlock = 1;
		case XK_Home:
			scan = 71;
			goto docode;
    	    	case XK_KP_8:
			nlock = 1;
		case XK_Up:
			scan = 72;
			goto docode;
    	    	case XK_KP_9:
			nlock = 1;
		case XK_Prior:
			scan = 73;
			goto docode;
    	    	case XK_KP_Subtract:
			scan = 74;
			goto docode;
    	    	case XK_KP_4:
			nlock = 1;
		case XK_Left:
			scan = 75;
			goto docode;
    	    	case XK_KP_5:
			nlock = 1;
		case XK_Begin:
			scan = 76;
			goto docode;
    	    	case XK_KP_6:
			nlock = 1;
		case XK_Right:
			scan = 77;
			goto docode;
    	    	case XK_KP_Add:
			scan = 78;
			goto docode;
    	    	case XK_KP_1:
			nlock = 1;
		case XK_End:
			scan = 79;
			goto docode;
    	    	case XK_KP_2:
			nlock = 1;
		case XK_Down:
			scan = 80;
			goto docode;
    	    	case XK_KP_3:
			nlock = 1;
		case XK_Next:
			scan = 81;
			goto docode;
    	    	case XK_KP_0:
			nlock = 1;
    	    	/* case XK_Insert: This is above */
			scan = 82;
			goto docode;

    	    	case XK_KP_Decimal:
			nlock = 1;
			scan = 83;
			goto docode;

    	    	case XK_Delete:
			scan = flipdelete ? 14 : 83;
			goto docode;

		case XK_BackSpace:
			scan = flipdelete ? 83 : 14;
			goto docode;

    	    	case XK_F11:
			scan = 87;
			goto docode;
    	    	case XK_F12:
			scan = 88;
			goto docode;


		case XK_KP_Divide:
			scan = Ascii2Scan['/'];
			goto docode;

		case XK_KP_Multiply:
			scan = Ascii2Scan['*'];
			goto docode;

		default:
    	    	    	if ((K1_STATUS&(K1_ALT|K1_CTRL)) == (K1_ALT|K1_CTRL)) {
				if (ks == 'T' || ks == 't') {
				    tmode ^= 1;
				    if (!tmode)
					    resettrace();
				    break;
				}
			}
			if (ks < ' ' || ks > '~')
				break;
			scan = Ascii2Scan[ks]; 
    	    	docode:
			if (nlock)
			    scan |= 0x100;

    	    	    	if ((scan & ~0x100) > 88) {
			    scan = 0xffff;
			    break;
    	    	    	}

    	    	    	if ((K1_STATUS & K1_SHIFT) || (scan & 0x100)) {
			    scan = ScanCodes[scan & 0xff].shift;
			} else if (K1_STATUS & K1_CTRL) {
			    scan = ScanCodes[scan & 0xff].ctrl;
			} else if (K1_STATUS & K1_ALT) {
			    scan = ScanCodes[scan & 0xff].alt;
			}  else
			    scan = ScanCodes[scan & 0xff].base;

			break;
		}
		if (scan != 0xffff) {
			KbdWrite(scan);
		}
		break;
	    }
	default:
		break;
	}
}

#define	R03D4	BIOSDATA[0x65]
static u_char	R03BA = 0;
static u_char	R03DA = 0;

unsigned char
video_inb(int port)
{
	switch(port) {
	case CGA_Status:
		R03DA += 1;	/* Just cylce throught the values */
		return(R03DA &= 0x0f);
	case 0x03c2:	/* Misc */
	case 0x03cc:	/* Misc */
		return(0xc3);
	case CVC_Data:
		if (R03D4 < 0x10) {
			return(VREG[R03D4]);
		}
		break;
	}
}

void
video_outb(int port, unsigned char value)
{
	int cp;

	switch(port) {
	case 0x03cc:
	case 0x03c2:
		if ((value & 0x1) == 0)	/* Trying to request monochrome */
			break;
		return;
	case CGA_Control:
		if (value & 0x22)	/* Trying to select graphics */
			break;
		return;
	case CVC_Address:
		R03D4 = value & 0x1f;
		return;
	case CVC_Data:
		if (R03D4 > 0x0f)
			break;
		VREG[R03D4] = value;
		switch (R03D4) {
		case MVC_CurHigh:
			cp = row * width + col;
			cp &= 0xff;
			cp |= (value << 8) & 0xff00;
			row = cp / width;
			col = cp % width;
			break;
		case MVC_CurLow:
			cp = row * width + col;
			cp &= 0xff00;
			cp |= value & 0xff;
			row = cp / width;
			col = cp % width;
			break;
		}
		return;
	}
}

void
tty_move(int r, int c)
{
	row = r;
	col = c;
	SetVREGCur();
}

void
tty_report(int *r, int *c)
{
	*r = row;
	*c = col;
}

void
tty_flush()
{
	K_NEXT = K_FREE = 0;
}

void
tty_index()
{
	int i;

	if (row < 0 || row > height - 1)
		row = 0;
	else if (++row >= height) {
		row = height - 1;
		memcpy(vmem, &vmem[width], 2 * width * (height - 1));
		for (i = 0; i < width; ++i)
		    vmem[(height - 1) * width + i] = vattr | ' ';
	}
	SetVREGCur();
}

void
tty_write(int c, int attr)
{
    	if (attr == TTYF_REDIRECT) {
		if (redirect1) {
		    char tc = c;
		    write(1, &c, 1);
		    return;
		}
		attr = -1;
	}
	c &= 0xff;
	switch (c) {
	case 0x07:
		if (xmode) {
			XBell(dpy, 0);
		} else
			write(1, "\007", 1);
		break;
	case 0x08:
		if (row < 0 || row > (height - 1) || col < 0 || col > width)
			break;
		if (col > 0)
			--col;
		vmem[row * width + col] &= 0xff00;
		break;
	case '\t':
		if (row < 0 || row > (height - 1))
			row = 0;
		col = (col + 8) & ~0x07;
		if (col > width) {
			col = 0;
			tty_index();
		}
		break;
	case '\r':
		col = 0;
		break;
	case '\n':
		tty_index();
		break;
	default:
		if (col >= width) {
			col = 0;
			tty_index();
		}
		if (row < 0 || row > (height - 1))
			row = 0;
		if (attr >= 0)
			vmem[row * width + col] = attr & 0xff00;
		else
			vmem[row * width + col] &= 0xff00;
		vmem[row * width + col++] |= c;
		break;
	}
	SetVREGCur();
}

void
tty_rwrite(int c, int attr)
{
	c &= 0xff;

	if (col >= width) {
		col = 0;
		tty_index();
	}
	if (row < 0 || row > (height - 1))
		row = 0;
	if (attr >= 0)
		vmem[row * width + col] = attr & 0xff00;
	else
		vmem[row * width + col] &= 0xff00;
	vmem[row * width + col++] |= c;
	SetVREGCur();
}

void
tty_pause()
{
	sigset_t set;

	sigprocmask(0, 0, &set);
	sigdelset(&set, SIGIO);
	sigdelset(&set, SIGALRM);
	sigsuspend(&set);
}

static int nextchar = 0;

int
tty_read(struct trapframe *tf, int flag)
{
    int r;

    if (r = nextchar) {
	nextchar = 0;
	return(r & 0xff);
    }

    if ((flag & TTYF_REDIRECT) && redirect0) {
	char c;
    	if (read(0, &c, 1) != 1)
	    return(-1);
	if (c == '\n')
	    c = '\r';
	return(c);
    }

    if (KbdEmpty()) {
	if (flag & TTYF_BLOCK) {
	    while (KbdEmpty())
		tty_pause();
	} else {
	    return(-1);
	}
    }

    r = KbdRead();
    if ((r & 0xff) == 0)
	nextchar = r >> 8;
    r &= 0xff;
    if (flag & TTYF_CTRL) {
	if (r == 3) {
	    /*
	     * XXX - Not quite sure where we should return, maybe not
	     *       all the way to the user, but...
	     */
	    if ((ivec[0x23] >> 16) && (ivec[0x23]>>16) != 0xF000) {
    	    	trap(tf, 0x23);
	    	tf->tf_ip -= 2;
		return(-2);
	    }
    	}
    }
    if (flag & TTYF_ECHO) {
	if ((flag & TTYF_ECHONL) && (r == '\n' || r == '\r')) { 
	    tty_write('\r', -1);
	    tty_write('\n', -1);
    	} else
	    tty_write(r, -1);
    }
    return(r & 0xff);
}

int
tty_peek(struct trapframe *tf, int flag)
{
	int c;

    	if (c == nextchar)
	    return(nextchar & 0xff);

	if (KbdEmpty()) {
		if (flag & TTYF_POLL) {
			sleep_poll();
			if (KbdEmpty())
				return(0);
		} else if (flag & TTYF_BLOCK) {
			while (KbdEmpty())
				tty_pause();
		} else
			return(0);
	}
	c = KbdPeek();
    	if ((c & 0xff) == 3) {
	    /*
	     * XXX - Not quite sure where we should return, maybe not
	     *       all the way to the user, but...
	     */
	    if ((ivec[0x23] >> 16) && (ivec[0x23]>>16) != 0xF000) {
    	    	trap(tf, 0x23);
	    	tf->tf_ip -= 2;
		return(-2);
	    }
    	}
	return(0xff);
}

int
tty_state()
{
	return(K1_STATUS);
}

tty_estate()
{
    int state = 0;
    if (K2_STATUS & K2_SYSREQ)
    	state |= 0x80;
    if (K2_STATUS & K2_CLOCK)
    	state |= 0x40;
    if (K2_STATUS & K2_NLOCK)
    	state |= 0x20;
    if (K2_STATUS & K2_SLOCK)
    	state |= 0x10;
    if (K3_STATUS & K3_RALT)
    	state |= 0x08;
    if (K3_STATUS & K3_RCTRL)
    	state |= 0x04;
    if (K2_STATUS & K2_LALT)
    	state |= 0x02;
    if (K2_STATUS & K2_LCTRL)
    	state |= 0x01;
    return(state);
}

inline int
inrange(int a, int n, int x)
{
	return(a < n ? n : a > x ? x : a);
}

void
tty_scroll(int sr, int sc, int er, int ec, int n, int attr)
{
	int i, j;

	sr = inrange(sr, 0, height);
	er = inrange(er, 0, height);
	sc = inrange(sc, 0, width);
	ec = inrange(ec, 0, width);
	if (sr >= er || sc >= ec)
		return;
	++er;
	++ec;

	attr &= 0xff00;
	attr |= ' ';

    	if (n == 0 || n >= er - sr) {
		for (j = sr; j < er; ++j)
			for (i = sc; i < ec; ++i)
				vmem[j * width + i] = attr;
	} else {
		for (j = sr; j < er - n; ++j) {
			memcpy(&vmem[j * width + sc],
			       &vmem[(j + n) * width + sc],
			       sizeof(vmem[0]) * ec - sc);
		}
		for (j = er - n; j < er; ++j)
			for (i = sc; i < ec; ++i)
				vmem[j * width + i] = attr;
	}
}

void
tty_rscroll(int sr, int sc, int er, int ec, int n, int attr)
{
	int i, j;

	sr = inrange(sr, 0, height);
	er = inrange(er, 0, height);
	sc = inrange(sc, 0, width);
	ec = inrange(ec, 0, width);
	if (sr >= er || sc >= ec)
		return;
	++er;
	++ec;

	attr &= 0xff00;
	attr |= ' ';

    	if (n == 0 || n >= er - sr) {
		for (j = sr; j < er; ++j)
			for (i = sc; i < ec; ++i)
				vmem[j * width + i] = attr;
	} else {
		for (j = er - 1; j > sr + n; --j) {
			memcpy(&vmem[(j + n) * width + sc],
			       &vmem[j * width + sc],
			       sizeof(vmem[0]) * ec - sc);
		}
		for (j = sr; j < sr + n; ++j)
			for (i = sc; i < ec; ++i)
				vmem[j * width + i] = attr;
	}
}

int
tty_char(int r, int c)
{
	if (r == -1)
		r = row;
	if (c == -1)
		c = col;
	r = inrange(r, 0, height);
	c = inrange(c, 0, width);
	return(vmem[r * width + c]);
}

int
KbdEmpty()
{
	return(K_NEXT == K_FREE);
}

void
KbdWrite(u_short code)
{
	int klen = K_BUFEND - K_BUFSTART;
	int kf = (K_FREE + 1) % klen;
	if (kf == K_NEXT) {
		XBell(dpy, 0);
		return;
	}
	K_BUFSTART[K_FREE] = code;
	K_FREE = kf;
}

void
KbdRepl(u_short code)
{
	K_BUFSTART[K_NEXT] = code;
}

u_short
KbdRead()
{
	int klen = K_BUFEND - K_BUFSTART;
	int kf = K_NEXT;
	K_NEXT = (K_NEXT + 1) % klen;
	return(K_BUFSTART[kf]);
}

u_short
KbdPeek()
{
	return(K_BUFSTART[K_NEXT]);
}
