/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
 
/*	Krystal $Id: int21.c,v 1.2 1994/01/14 23:34:49 polk Exp $ */

#include "doscmd.h"
#include <dirent.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#include <glob.h>
#include <errno.h>
#include <ctype.h>

/* Country Info */
struct {
        ushort  ciDateFormat;
        char    ciCurrency[5];
        char    ciThousands[2];
        char    ciDecimal[2];
        char    ciDateSep[2];
        char    ciTimeSep[2];
        char    ciBitField;
        char    ciCurrencyPlaces;
        int     ciCaseMap;
        char    ciDataSep[2];
        char    ciReserved[10];
} countryinfo = {
        0, "$", ",", ".", "/", ":", 0, 2, 0xfff0ffff, "?"
};

/* DOS File Control Block */
struct fcb {
        u_char  fcbDriveID;
        u_char  fcbFileName[8];
        u_char  fcbExtent[3];
        u_short fcbCurBlockNo;
        u_short fcbRecSize;
        u_long  fcbFileSize;
        u_short fcbFileDate;     
        u_short fcbFileTime;
        int     fcbReserved;
        int     fcb_fd;                 /* hide UNIX FD here */
        u_char  fcbCurRecNo; 
        u_char  fcbRandomRecNo[4];
};      
#define FCBMAGIC 0xbad1

void encode_dos_file_time (time_t, u_short *, u_short *);
char *translate_filename ();
char *convert_filename ();
int canonicalize ();
static int getfcb_name(struct fcb*, char*);
static void openfcb(struct fcb*);
static int getfcb_rec(struct fcb*, int);
static int setfcb_rec(struct fcb*, int);

int ctrl_c_flag = 0;
int return_status = 0;
int doserrno = 0;
int thiserrno = 0;
int diskdrive = 2;	/* C: */
int lastdrive = -1;	/* Set by translate_filename */

char *InDOS;

void
int21 (tf)
struct trapframe *tf;
{
	struct byteregs *b = (struct byteregs *)&tf->tf_bx;
	u_short *intadr;
	char *addr;
	int file_handle;
	int nbytes;
	char buf[1000];
	char *p;
	int seg, newsize;
	int n;
	int memseg;
	int mode;
	char *name;
	int fd,fd2;
	int avail;
	char *uname;
	struct timeval tv;
	struct timezone tz;
	struct tm tm;
	struct fcb *fcbp;
    	off_t offset;
    	static nextchar = 0;

	tf->tf_eflags &= ~PSL_C;
	thiserrno = 0;

	switch (b->tf_ah) {
	case 0x00:
		done(tf, 0);
	case 0x01: /* single character input, with echo */
	func01:
	    	if ((n = tty_read(tf, TTYF_BLOCKALL)) < 0)
		    return;
    	    	b->tf_al = n;
		break;

	case 0x02: /* print char */
		tty_write(b->tf_dl, TTYF_REDIRECT);
		break;

	case 0x06: /* Direct console IO */
	func06:
		/* XXX - should be able to read a file */
		if (b->tf_dl == 0xff) {
			n = tty_read(tf, TTYF_ECHO|TTYF_REDIRECT);
    	    	    	if (n < 0) {
			    tf->tf_eflags &= ~PSL_Z;
			} else {
			    b->tf_al &= 0xff;
			    tf->tf_eflags |= PSL_Z;
			}
		} else {
			tty_write(b->tf_dl, TTYF_REDIRECT);
		}
		break;
	case 0x07: /* single character input, no echo, no controls */
	func07:
		b->tf_al = tty_read(tf, TTYF_BLOCK|TTYF_REDIRECT) & 0xff;
		break;
	case 0x08: /* single character input, no echo */
	func08:
		if ((n = tty_read(tf, TTYF_BLOCK|TTYF_CTRL|TTYF_REDIRECT)) < 0)
			return;
		b->tf_al = n;
		break;

	case 0x09: /* print string */
		addr = (char *)MAKE_ADDR (tf->tf_ds, tf->tf_dx);
		p = strchr(addr, '$');
		if (p) {
			nbytes = p - addr;
			while (nbytes-- > 0)
				tty_write(*addr++, TTYF_REDIRECT);
		}
		break;

	case 0x0a: /* buffered input */
	func0a:
		addr = (char *)MAKE_ADDR (tf->tf_ds, tf->tf_dx);
		/*
		 * Leave room for max, len, '\r' and '\0'
		 */
		avail = ((unsigned char *)addr)[0] - 4;
		nbytes = 0;

		while ((n = tty_read(tf, TTYF_BLOCK|TTYF_CTRL|TTYF_REDIRECT)) >= 0) {
		    switch(n) {
		    case '\r':
			tty_write('\r', TTYF_REDIRECT);
			tty_write('\n', TTYF_REDIRECT);
			addr[1] = nbytes;
			addr[nbytes+2] = '\r';
			addr[nbytes+3] = '\0';
			return;
		    case '\n':
			tty_write('\r', TTYF_REDIRECT);
			tty_write('\n', TTYF_REDIRECT);
			break;
		    case '\b':
			if (nbytes > 0) {
			    --nbytes;
			    tty_write('\b', TTYF_REDIRECT);
			    if (addr[nbytes+2] < ' ')
				tty_write('\b', TTYF_REDIRECT);
			}
			break;
    	    	    case '\0':
			break;
		    default:
			if (nbytes >= avail) {
			    tty_write('\007', TTYF_REDIRECT);
			} else {
			    addr[nbytes++ +2] = n;
    	    	    	    if (n != '\t' && n < ' ') {
				tty_write('^', TTYF_REDIRECT);
				tty_write(n + '@', TTYF_REDIRECT);
			    } else
				tty_write(n, TTYF_REDIRECT);
			}
			break;
		    }
		}
		break;
		
	case 0x0b: /* check keyboard status */
    	    {
		extern poll_cnt;

		if (!xmode) {
		    b->tf_al = 255;
		    break;
		}
		n = tty_peek(tf, poll_cnt ? 0 : TTYF_POLL) ? 255 : 0;
		if (n < 0)
		    return;
		b->tf_al = n;
    	    	if (poll_cnt)
		    --poll_cnt;
		break;
    	    }

	case 0x0c: /* flush buffer, read keyboard */
		if (xmode)
		    tty_flush();
		switch (b->tf_al) {
		case 0x01: goto func01;
		case 0x06: goto func06;
		case 0x07: goto func07;
		case 0x08: goto func08;
		case 0x0a: goto func0a;
		}
		b->tf_al = 0;
		break;

        case 0x0d: /* reset drive */
                break;                  /* don't worry, be happy */

	case 0x0e: /* select drive */
		diskdrive = b->tf_dl;
		b->tf_al = ndisks + 2;
		break;

	case 0x0f: /* open file with FCB */
		fcbp = (struct fcb *)MAKE_ADDR(tf->tf_ds, tf->tf_dx);
		n = getfcb_name(fcbp,buf);
		debug(D_FILE_OPS, "open FCB(%s) ", buf);
    	    	uname = translate_filename(buf);
    	    	if (dos_readonly(lastdrive))
		    mode = O_RDONLY;
    	    	else
		    mode = O_RDWR;
		if ((fd = open(uname, mode)) < 0) {
			b->tf_al |= 0xff;
			thiserrno = FILE_NOT_FOUND;
			debug(D_FILE_OPS, "fails on %s with errno %d",
				uname, errno);
		} else {
			fcbp->fcbReserved = FCBMAGIC+n;
			fcbp->fcb_fd = fd;
			openfcb(fcbp);
			b->tf_al = 0;
			debug (D_FILE_OPS, "returns %d", fd);
		} 
		break;

	case 0x10: /* close file with FCB */
		fcbp = (struct fcb *)MAKE_ADDR(tf->tf_ds, tf->tf_dx);
		n = getfcb_name(fcbp,buf);
		/* require the FCB be unmolested */
		if (fcbp->fcbReserved != FCBMAGIC+n)
			fatal("stray FCB close\n");
		if (0 > close(fcbp->fcb_fd)) {
			debug (D_FILE_OPS, "close(%d) failed with errno %d",
				fcbp->fcb_fd, errno);
			thiserrno = HANDLE_INVALID;
			b->tf_al = 0xff;
		} else {
			debug (D_FILE_OPS, "FCB close(%s,%d)",
				buf, fcbp->fcb_fd);
			fcbp->fcb_fd = -1;
			fcbp->fcbReserved = -1;
			b->tf_al = 0;
		}
		break;

	case 0x16: /* create file with FCB */
		fcbp = (struct fcb *)MAKE_ADDR(tf->tf_ds, tf->tf_dx);
		n = getfcb_name(fcbp,buf);
		debug (D_FILE_OPS, "create FCB(%s) ", buf);
		uname = translate_filename(buf); 
    	    	if (dos_readonly(lastdrive)) {
			thiserrno = WRITE_PROT_DISK;
			debug(D_FILE_OPS, "fails due to WP disk");
			break;
		}
		if ((fd = open (uname, O_CREAT|O_TRUNC|O_RDWR, 0666)) < 0) {
			b->tf_al |= 0xff;
			thiserrno = ACCESS_DENIED;
			debug(D_FILE_OPS, "fails on %s with errno %d",
				uname, errno);
		} else {
			fcbp->fcbReserved = FCBMAGIC+n;
			fcbp->fcb_fd = fd;
			openfcb(fcbp);
			b->tf_al = 0;
			debug(D_FILE_OPS, "returns %d", fd);
		}
		break;

	case 0x19: /* get default disk drive */
		b->tf_al = diskdrive;
		break;

	case 0x1a: /* set disk transfer address */
		disk_transfer_addr = (char *)MAKE_ADDR (tf->tf_ds, tf->tf_dx);
		disk_transfer_seg = tf->tf_ds;
		disk_transfer_offset = tf->tf_dx;
		break;

	case 0x25: /* set interrupt vector */
		debug(D_TRAPS2, "%02x -> %04x:%04x", b->tf_al, tf->tf_ds, tf->tf_dx);
		ivec[b->tf_al] = (((unsigned long)(tf->tf_ds))<<16)|tf->tf_dx;
		break;

	case 0x26: /* create psp */
		addr = (char *)MAKE_ADDR(tf->tf_dx, 0);
		memcpy (addr, dosmem, 256);
		/* XXX needs a few changes */
		break;

	case 0x27: /* random block read */
		fcbp = (struct fcb *)MAKE_ADDR(tf->tf_ds, tf->tf_dx);
		nbytes = getfcb_rec(fcbp,tf->tf_cx);
		if (nbytes < 0) {
			b->tf_al = 0xff;
			break;
		}
		n = read(fcbp->fcb_fd, disk_transfer_addr, nbytes);
		if (n < 0) {
			b->tf_al = 0xff;
			break;
		}
		tf->tf_cx = setfcb_rec(fcbp,n);
		if (n < nbytes) {
			nbytes = n % fcbp->fcbRecSize;
			if (0 == nbytes) {
				b->tf_al = 0x01;
			} else {
				bzero(disk_transfer_addr+n,
					fcbp->fcbRecSize - nbytes);
				b->tf_al = 0x03;
			}
		} else {
			b->tf_al = 0;
		}
		break;

	case 0x28: /* random block write */
		fcbp = (struct fcb *)MAKE_ADDR(tf->tf_ds, tf->tf_dx);
		nbytes = getfcb_rec(fcbp,tf->tf_cx);
		if (nbytes < 0) {
			b->tf_al = 0xff;
			break;
		}
		n = write(fcbp->fcb_fd, disk_transfer_addr, nbytes);
		if (n < 0) {
			b->tf_al = 0xff;
			break;
		}
		tf->tf_cx = setfcb_rec(fcbp,n);
		if (n < nbytes) {
			b->tf_al = 0x01;
		} else {
			b->tf_al = 0;
		}
		break;

	case 0x29: /* parse filename */
		debug (D_FILE_OPS,
		       "parse filename: flag=%d, ", b->tf_al);

		b->tf_al = parse_filename(b->tf_al,
					  MAKE_ADDR(tf->tf_ds, tf->tf_si),
					  MAKE_ADDR(tf->tf_es, tf->tf_di),
					  &nbytes);
		debug (D_FILE_OPS, "%d %s, FCB: %d, %s\n",
		       nbytes,
		       MAKE_ADDR(tf->tf_ds, tf->tf_si),
		       *(int *)MAKE_ADDR(tf->tf_es, tf->tf_di),
		       MAKE_ADDR(tf->tf_es, tf->tf_di) + 1);
		       
		tf->tf_si += nbytes;
		break;

	case 0x2a: /* get date */
		gettimeofday (&tv, &tz);
		tm = *localtime (&tv.tv_sec);

		tf->tf_cx = tm.tm_year;
		b->tf_dh = tm.tm_mon + 1;
		b->tf_dl = tm.tm_mday;
		b->tf_al = tm.tm_wday;
		break;
	case 0x2b:
		debug(D_HALF, "INT 21:0x2b cannot set date.");
		b->tf_al = 0;
		break;

	case 0x2c: /* get time */
		gettimeofday (&tv, &tz);
		tm = *localtime (&tv.tv_sec);

		b->tf_ch = tm.tm_hour;
		b->tf_cl = tm.tm_min;
		b->tf_dh = tm.tm_sec;
		b->tf_dl = tv.tv_usec / 10000;
		break;

	case 0x2f: /* get dta address */
		tf->tf_es = disk_transfer_seg;
		tf->tf_bx = disk_transfer_offset;
		break;

	case 0x30: /* get MS-DOS version number */
	    {
		char *cmd = MAKE_ADDR(get_env(), 0);
    	    	short v;

		do {
		    while (*cmd)
			++cmd;
		} while (*++cmd);
		++cmd;
		cmd += *(short *)cmd + 1;
		while (cmd[-1] && cmd[-1] != '\\' && cmd[-1] != ':')
		    --cmd;

    	    	v = getver(cmd);
		b->tf_al = (v / 100) & 0xff;
		b->tf_ah = v % 100;
		break;
	    }

	case 0x33: /* get or set break flag */
		switch (b->tf_al) {
		case 0x00:
			b->tf_dl = ctrl_c_flag;
			break;
		case 0x01:
			ctrl_c_flag = b->tf_dl;
			break;
		case 0x05:
			b->tf_dl = 1;		/* Boot Drive */
			break;
    	    	case 0x06:			/* Dos version */
			b->tf_bh = 3;
			b->tf_bl = 1;
			b->tf_dh = 0;
			b->tf_dl = 1;
			break;
		default:
			unknown_int3(0x21, 0x33, b->tf_al, tf);
			break;
		}
		break;

	case 0x34: /* Get InDOS Pointer */
		tf->tf_es = ((u_long)InDOS) >> 16;
		tf->tf_bx = ((u_long)InDOS) & 0xFFFF;
		break;

	case 0x35: /* get interrupt vector ... fake it */
		tf->tf_bx = ivec[b->tf_al] & 0xffff;
		tf->tf_es = ivec[b->tf_al] >> 16;
		debug(D_TRAPS2, "%02x <- %04x:%04x", b->tf_al, tf->tf_es, tf->tf_bx);
		break;

    	case 0x36: /* get disk space */
		tf->tf_ax = 8; /* number of sectors per cluster */
		tf->tf_bx = 10000; /* number of available clusters */
		tf->tf_cx = 512; /* number of bytes per sector */
		tf->tf_dx = 20000; /* total number of clusters */
	    	break;

	case 0x37:
		switch (b->tf_al) {
		case 0: /* get switch character */
			b->tf_dl = '/';
			break;

		case 1: /* set switch character (normally /) */
			break;
		default:
			unknown_int3(0x21, 0x37, b->tf_al, tf);
			break;
		}
		break;

	case 0x38:
                if (tf->tf_dx == 0xffff) {
                        debug(D_HALF, "warning: set country code ignored");
                        break;
                }
                addr = (char *)MAKE_ADDR(tf->tf_ds, tf->tf_dx);
                memcpy(addr, &countryinfo, sizeof(countryinfo));
                tf->tf_eflags &= ~PSL_C;
                break;
	case 0x39:
		name = (char *)MAKE_ADDR (tf->tf_ds, tf->tf_dx);
		uname = translate_filename(name);
    	    	if (dos_readonly(lastdrive)) {
		    thiserrno = WRITE_PROT_DISK;
		    break;
    	    	}

		if (mkdir(uname, 0777) < 0)
			thiserrno = (errno == ENOTDIR) ? PATH_NOT_FOUND
						       : ACCESS_DENIED;
		break;

	case 0x3b: /* set current directory */
		n = diskdrive;

		name = (char *)MAKE_ADDR (tf->tf_ds, tf->tf_dx);
		if (name[1] == ':') {
		    n = (name[0] | 040) - 'a';
		    name += 2;
		}

		thiserrno = dos_setcwd(n, (u_char *)name);
		break;

	case 0x3c: /* creat */
		name = (char *)MAKE_ADDR (tf->tf_ds, tf->tf_dx);
		debug (D_FILE_OPS, "creat (%s)", name);

		uname = translate_filename (name);
    	    	if (dos_readonly(lastdrive)) {
		    thiserrno = WRITE_PROT_DISK;
		    break;
    	    	}

		if ((fd = open (uname, O_CREAT|O_TRUNC|O_RDWR, 0666)) < 0) {
			thiserrno = ACCESS_DENIED;
			debug (D_FILE_OPS, "creat fails\n");
		} else {
			tf->tf_ax = fd;
			debug (D_FILE_OPS, "creat returns %d\n", fd);
		}
		break;

	case 0x3d: /* open */
		mode = b->tf_al & 3; /* 0=r, 1=w, 2=rw */
		name = (char *)MAKE_ADDR (tf->tf_ds, tf->tf_dx);
		debug (D_FILE_OPS, "open (%s, %d, %s)",
				    name, mode, translate_filename(name));

    	    	if (mode && dos_readonly(lastdrive)) {
		    thiserrno = WRITE_PROT_DISK;
		    break;
		}

		if ((fd = open (translate_filename (name), mode)) < 0) {
			thiserrno = FILE_NOT_FOUND;
			debug (D_FILE_OPS, "open fails\n", fd);
		} else {
			tf->tf_ax = fd;
			debug (D_FILE_OPS, "open returns %d\n", fd);
		}
		break;

	case 0x3e: /* close */
		fd = tf->tf_bx;
		debug (D_FILE_OPS, "close (%d)\n", fd);
		close (fd);
		break;
		
	case 0x3f: /* read */
		file_handle = tf->tf_bx;

		nbytes = tf->tf_cx;
		addr = (char *)MAKE_ADDR(tf->tf_ds, tf->tf_dx);
		debug (D_FILE_OPS, "read (%d, %d)", file_handle, nbytes);
		if (file_handle == 0) {
			if (redirect0) {
			    n = read (file_handle, addr, nbytes);
			} else {
			    n = 0;
			    while (n < nbytes) {
				avail = tty_read(tf, TTYF_BLOCK|TTYF_CTRL|TTYF_ECHONL);
				if (avail < 0)
				    return;
				if ((addr[n++] = avail) == '\r')
				    break;
			    }
			}
				
		} else {
			n = read (file_handle, addr, nbytes);
		}
		if (n < 0) {
			thiserrno = ACCESS_DENIED;
		} else {
			tf->tf_ax = n;
		}
		debug (D_FILE_OPS, " -> %d\n", n);
		break;

	case 0x40: /* write */
		file_handle = tf->tf_bx;
		nbytes = tf->tf_cx;
		addr = (char *)MAKE_ADDR(tf->tf_ds, tf->tf_dx);
		debug (D_FILE_OPS, "write (%d, %d)\n", file_handle, nbytes);
    	    	switch (file_handle) {
		case 0:
			if (redirect0) {
				n = write (file_handle, addr, nbytes);
				break;
			}
			n = nbytes;
			while (nbytes-- > 0)
				tty_write(*addr++, -1);
			break;
		case 1:
			if (redirect1) {
				n = write (file_handle, addr, nbytes);
				break;
			}
			n = nbytes;
			while (nbytes-- > 0)
				tty_write(*addr++, -1);
			break;
		case 2:
			if (redirect2) {
				n = write (file_handle, addr, nbytes);
				break;
			}
			n = nbytes;
			while (nbytes-- > 0)
				tty_write(*addr++, -1);
			break;
		default:
			n = write (file_handle, addr, nbytes);
			break;
		}
		if (n < 0) {
			thiserrno = ACCESS_DENIED;
		} else {
			tf->tf_ax = n;
		}
		break;

	case 0x41: /* delete */
		name = (char *)MAKE_ADDR (tf->tf_ds, tf->tf_dx);
		debug (D_FILE_OPS, "unlink %s\n", name);
    	    	uname = translate_filename(name);
    	    	if (dos_readonly(lastdrive)) {
		    thiserrno = WRITE_PROT_DISK;
		    break;
    	    	}
		if (unlink (uname) < 0) {
			thiserrno = ACCESS_DENIED;
		}
		break;

	case 0x42: /* seek */
		offset = (tf->tf_cx << 16) | tf->tf_dx;

		n = lseek (tf->tf_bx, offset, b->tf_al);
		debug (D_FILE_OPS, "seek(%d, 0x%x, %d) -> 0x%x\n",
			tf->tf_bx, offset, b->tf_al, n);

		if (n == -1) {
			thiserrno = HANDLE_INVALID;
		} else {
			tf->tf_dx = n >> 16;
			tf->tf_ax = n & 0xffff;
		}
		break;
			
	case 0x43: /* get/set attributes */
	    {
		struct stat statb;

		name = (char *)MAKE_ADDR (tf->tf_ds, tf->tf_dx);
		debug (D_FILE_OPS, "get/set attributes: %s, cx=%x, al=%d",
				   name, tf->tf_cx, b->tf_al);

		uname = translate_filename(name);

		if (stat(uname, &statb) < 0) {
		    debug (D_FILE_OPS, "stat failed for %s", uname);
		    thiserrno = PATH_NOT_FOUND;
		} else {
		    switch (b->tf_al) {
		    case 0:
			mode = 0;
			if (dos_readonly(lastdrive) || access(uname, W_OK))
			    mode |= 0x01;
			if (statb.st_mode & S_IFDIR)
			    mode |= 0x10;
			tf->tf_cx = mode;
			break;

		    case 1:
			if (tf->tf_cx & 0x18) {
			    thiserrno = ACCESS_DENIED;
			}
			break;

		    default:
			thiserrno = FUNC_NUM_IVALID;
		    }
		}
		debug (D_FILE_OPS, "\n");
		break;
	    }
	case 0x44: /* ioctl */
		msdos_ioctl (tf);
		break;

	case 0x45: /* dup handle */
		fd = tf->tf_bx;
		debug (D_FILE_OPS, "dup (%d)\n", fd);
		if ((tf->tf_ax = dup (fd)) < 0) {
			thiserrno = (errno == EBADF) ? HANDLE_INVALID
						     : TOO_MANY_OPEN_FILES;
		}
		break;

	case 0x46: /* dup2 handle */
		fd = tf->tf_bx;
		fd2 = tf->tf_cx;
		debug (D_FILE_OPS, "dup2 (%d,%d)\n", fd,fd2);
		if (dup2 (fd,fd2) < 0)
			thiserrno = (errno == EMFILE) ? TOO_MANY_OPEN_FILES
						      : HANDLE_INVALID;
		break;
		

	case 0x47: /* get current directory */
		n = b->tf_dl;
		if (!n--)
		    n  = diskdrive;

		p = (char *)dos_getcwd(n);
		addr = (char *)MAKE_ADDR (tf->tf_ds, tf->tf_si);

		nbytes = strlen (p);
		if (nbytes > 64) nbytes = 64;

		memcpy (addr, p, nbytes);
		addr[nbytes] = 0;
		break;

		
	case 0x48: /* allocate memory */
		memseg = mem_alloc (tf->tf_bx, pspseg, &avail);

		if (memseg == 0L) {
			thiserrno = INSUF_MEM;
			tf->tf_bx = avail;
		} else {
			tf->tf_ax = memseg;
		}
		break;

	case 0x49: /* free memory */
		mem_adjust (tf->tf_es, 0, NULL);
		break;

	case 0x4a: /* modify memory allocation */
		if ((n = mem_adjust (tf->tf_es, tf->tf_bx, &avail)) < 0) {
			if (n == -1) {
				thiserrno = INSUF_MEM;
			} else {
				thiserrno = MEM_BLK_ADDR_IVALID;
			}
		}
		tf->tf_bx = avail;
		break;

	case 0x4b: /* exec */
		name = MAKE_ADDR(tf->tf_ds,tf->tf_dx);
		debug (D_EXEC, "exec: %s\n", name);
		if ((fd = open_prog (name)) < 0) {
		  debug (D_EXEC, "%s: command not found\n", name);
		  thiserrno = FILE_NOT_FOUND;
		  break;
		}

		if (use_fork && fork()) {
		    /* parent */
		    _BlockIO();
		    wait(&return_status);
		    _UnblockIO();

		    return_status >>= 8;
		    return_status &= 0xff;
		    debug (D_EXEC, "exec: return status = %d\n", return_status);
		} else {
		    /* child */
		    addr = (char *)MAKE_ADDR(tf->tf_es, tf->tf_bx);
		    exec_command(fd, cmdname, addr, tf);

		    debug (D_EXEC, "cs:ip = %04x:%04x, ss:sp = %04x:%04x, "
				 "ds = %04x, es = %04x\n",
			init_cs, init_ip, init_ss, init_sp, init_ds, init_es);

		    tf->tf_eflags = 0x20202;
		    tf->tf_cs = init_cs;
		    tf->tf_ip = init_ip-2;
		    tf->tf_ss = init_ss;
		    tf->tf_sp = init_sp;

		    tf->tf_ds = init_ds;
		    tf->tf_es = init_es;

		    tf->tf_ax = 0;
		    tf->tf_bx = 0;
		    tf->tf_cx = 0;
		    tf->tf_dx = 0;
		    tf->tf_si = 0;
		    tf->tf_di = 0;
		    tf->tf_bp = 0;
		}
		close(fd);
		break;

	case 0x4c: /* exit */
		return_status = b->tf_al;
		done (tf, b->tf_al);

	case 0x4d: /* get return code */
		tf->tf_ax = return_status;
		break;

	case 0x4e: /* find first */
		do_find_first (tf);
		break;

	case 0x4f: /* find next */
		do_find_next (tf);
		break;

	case 0x50: /* set psp */
		pspseg = tf->tf_bx;
		break;

	case 0x51: /* get psp (documented in dosref) */
		tf->tf_bx = pspseg;
		break;

    	case 0x52: /* get list of lists */
		tf->tf_eflags |= PSL_C;
    	    	break;

	case 0x56: /* rename file */
		addr = (char *)MAKE_ADDR(tf->tf_ds, tf->tf_dx);
		strcpy(buf, translate_filename(addr));

		addr = (char *)MAKE_ADDR(tf->tf_es, tf->tf_di);
    	    	uname = translate_filename(addr);

    	    	if (dos_readonly(lastdrive)) {
		    thiserrno = WRITE_PROT_DISK;
		    break;
    	    	}

		if (rename(buf, uname)) {
			thiserrno = FILE_NOT_FOUND;
		} else {
			tf->tf_ax = 0;
		}
		break;

	case 0x57: /* get or set file time */
		do_times (tf);
		break;

	case 0x59: /* get error code */
		tf->tf_ax = doserrno;
		switch (doserrno) {
		case 1:
		case 6:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13:
		case 15:
		    b->tf_bh = 7;
		    break;

		case 2:
		case 3:
		case 4:
		case 5:
		    b->tf_bh = 8;
		    break;

		case 7:
		case 8:
		    b->tf_bh = 1;
		    break;
		  
		default:
		    b->tf_bh = 12;
		    break;
		}
		b->tf_bl = 6;
		b->tf_ch = 0;
		break;

	case 0x5a: /* create temporary file */
		addr = (char *)MAKE_ADDR(tf->tf_ds, tf->tf_dx);
		debug (D_FILE_OPS, "tempname(%s) ", addr);
		uname = translate_filename(addr);
    	    	if (dos_readonly(lastdrive)) {
		    thiserrno = WRITE_PROT_DISK;
		    break;
    	    	}
		n = strlen(uname);
		strcpy(&uname[n],"doscmdXXXXX");
		if (0 > mkstemp(uname)
		    || (fd = open(uname,O_CREAT|O_TRUNC|O_RDWR,0666)) < 0) {
			tf->tf_ax = thiserrno = ACCESS_DENIED;
			debug(D_FILE_OPS,"fails for %s with %d\n",uname,errno);
		} else {
			strcat(addr,&uname[n]);
			tf->tf_ax = fd;
			debug (D_FILE_OPS, "returns %d\n", fd);
		}
		break;

	case 0x5b:
		addr = (char *)MAKE_ADDR(tf->tf_ds, tf->tf_dx);
		uname = translate_filename(addr);
    	    	if (dos_readonly(lastdrive)) {
		    thiserrno = WRITE_PROT_DISK;
		    break;
    	    	}
		fd = open(uname,O_CREAT|O_TRUNC|O_RDWR|O_EXCL,0666);
		if (0 > fd) {
			tf->tf_ax = thiserrno = ACCESS_DENIED;
			debug(D_FILE_OPS, "creat(%s) fails on %s with %d\n",
				addr,uname,errno);
		} else {
			tf->tf_ax = fd;
			debug(D_FILE_OPS, "creat(%s) returns %d\n", addr,fd);
		}
		break;
	case 0x5d:
		switch(b->tf_al) {
		case 0x06:
			debug (D_HALF, "Get Swapable Area");
			thiserrno = ACCESS_DENIED;
			break;
		case 0x08: /* Set redirected printer mode */
			debug(D_HALF, "Redirection is %s\n",
					b->tf_dl ? "seperate jobs"
						 : "combined");
			break;
		case 0x09: /* Flush redirected printer output */
			break;
		default:
			unknown_int3(0x21, 0x5d, b->tf_al, tf);
			break;
		}
		break;

    	case 0x5e: /* Network functions */
	    	thiserrno = FUNC_NUM_IVALID;
		break;

	case 0x60: /* canonicalize name */
		addr = (char *)MAKE_ADDR(tf->tf_ds, tf->tf_si);
		if ((thiserrno = canonicalize(addr)) == 0) {
		    addr = (char *)MAKE_ADDR(tf->tf_es, tf->tf_di);
		    strncpy(addr, buf, 128);
		}
		break;

	case 0x62: /* get PSP */
		tf->tf_bx = pspseg;
		break;
		
	case 0x63: /* get lead byte table (something about inter. charset) */
		thiserrno = FUNC_NUM_IVALID;
		break;

    	case 0x65: /* Get extended country information */
		thiserrno = FUNC_NUM_IVALID;
		break;

    	case 0x67: /* Set handle count */
		debug(D_HALF, "Wants %d files\n", tf->tf_bx);
		/* tf->tf_bx;		/* Number of handles */
		break;

    	case 0x87: /* ??? */
		tf->tf_eflags |= PSL_C;
		break;
		
	default:
	unknown:
		unknown_int2(0x21, b->tf_ah, tf);
		break;
	}

	if (thiserrno) {
		tf->tf_ax = doserrno = thiserrno;
		tf->tf_eflags |= PSL_C;
	}
	
}

msdos_ioctl (tf)
struct trapframe *tf;
{
	struct byteregs *b = (struct byteregs *)&tf->tf_bx;
	int fd;
	
	switch (b->tf_al) {
	case 0: /* get device info */
		fd = tf->tf_bx;
		debug (D_FILE_OPS, "ioctl get %d", fd);
		tf->tf_dx = 0;
		switch (fd) {
		case 0:
			tf->tf_dx |= 0x80 | 0x01;
			break;
		case 1:
			tf->tf_dx |= 0x80 | 0x02;
			break;
		case 2:
			tf->tf_dx |= 0x80;
			break;
		default:
			if (isatty (fd))
				tf->tf_dx |= 0x80;
		}
		tf->tf_eflags &= ~PSL_C;
		break;
	case 1: /* set device information */
		fd = tf->tf_bx;
		debug (D_FILE_OPS, "ioctl set device info %d flags %x",
			fd, tf->tf_dx);
		break;

	case 8: /* test for removable block device */
		tf->tf_ax = 1; /* fixed */
		tf->tf_eflags &= ~PSL_C;
		break;
    	case 0x0d:
		tf->tf_eflags |= PSL_C;
		break;
	default:
		unknown_int3(0x21, 0x44, b->tf_al, tf);
		break;
	}
}

static int
getfcb_name(struct fcb *fcbp, char* buf)
{
	char *p;
	int n;
	char lbuf[8+1+3+1];

	if (!buf)
		buf = lbuf;

	bzero(buf,8+1+3+1);
	bcopy(fcbp->fcbFileName,buf,8);
	if (fcbp->fcbExtent[0] != ' ') {
		p = strchr(buf,' ');
		if (!p)
			p = &buf[8];
		*p++ = '.';
		bcopy(fcbp->fcbExtent, p, 3);
	}
	p = strchr(buf,' ');
	if (p)
		*p = '\0';

	for (n = 0; *buf; buf++);
		n += *buf;
	return (n);
}


static void
openfcb(struct fcb *fcbp)
{
	struct stat statb;

	fcbp->fcbDriveID = 3;		/* drive C */
	fcbp->fcbCurBlockNo = 0;
	fcbp->fcbRecSize = 128;
	if (fstat(fcbp->fcb_fd, &statb) < 0) {
		debug(D_FILE_OPS, "open not complete with errno %d", errno);
		return;
	}
	encode_dos_file_time(statb.st_mtime,
				&fcbp->fcbFileDate, &fcbp->fcbFileTime);
	fcbp->fcbFileSize = statb.st_size;
}

static int
getfcb_rec(struct fcb *fcbp, int nrec)
{
	int n;

	n = getfcb_name(fcbp,0);
	/* require the FCB be unmolested */
	if (fcbp->fcbReserved != FCBMAGIC+n)
		fatal("stray FCB\n");
	n = *(u_long*)&fcbp->fcbRandomRecNo;
	if (fcbp->fcbRecSize >= 64)
		n &= 0xffffff;
	fcbp->fcbCurRecNo = n % 128;
	fcbp->fcbCurBlockNo = n / 128;
	if (0 > lseek(fcbp->fcb_fd, n * fcbp->fcbRecSize, SEEK_SET))
		return (-1);
	return (nrec * fcbp->fcbRecSize);
}


static int
setfcb_rec(struct fcb *fcbp, int n)
{
	int recs, total;

	total = *(u_long*)&fcbp->fcbRandomRecNo;
	if (fcbp->fcbRecSize >= 64)
		total &= 0xffffff;
	recs = (n+fcbp->fcbRecSize-1) / fcbp->fcbRecSize;
	total += recs;

	*(u_long*)&fcbp->fcbRandomRecNo = total;
	fcbp->fcbCurRecNo = total % 128;
	fcbp->fcbCurBlockNo = total / 128;
}
char *
translate_filename (dname)
char *dname;
{
	static char uname[1024];

    	lastdrive = -1;

    	if (!strcasecmp(dname, "con"))
	    return("/dev/tty");

    	lastdrive = diskdrive;

	if (isalpha (dname[0]) && dname[1] == ':') {
		lastdrive = (dname[0] | 040) - 'a';
		dname += 2;
	}

    	if (dos_to_real_path(lastdrive, (u_char *)dname, (u_char *)uname))
	    return(dname);
	else
	    return(uname);
}

char *
convert_filename (uname)
char *uname;
{
	static char dname[1000];
	char *p, *q;

	p = uname;

	for (q = dname; *p; p++, q++) {
		if (islower (*p))
			*q = toupper (*p);
		else
			*q = *p;

		if (*q == '/')
			*q = '\\';
	}

	*q = 0;

	return (dname);
}

int
canonicalize(char *name)
{
    char buf[1024];
    char *oname = name;
    int drive = diskdrive;
    int e;

    if (name[0] && name[1] == ':') {
    	if (islower(name[0]))
	    drive = name[0] - 'a';
    	else if (isupper(name[0]))
	    drive = name[0] - 'A';
    	else
	    return(ACCESS_DENIED);
    	name += 2;
    }
    if (e = dos_makepath(drive, (u_char *)name, (u_char *)buf))
	return(e);
    name[0] = drive + 'A';
    name[1] = ':';
    strcpy(oname + 2, buf);
    return(0);
}

int
parse_filename( int flag, char *str, char *fcb, int *nb )
{
  char buf[256];
  char *sp,*p;
  int ret = 0;

  *nb = 0;
  
  strncpy(buf, str, 255);
  buf[255] = 0;
  
  sp = buf;
  if (sp[1] == ':') {
    if (toupper(*sp) != 'C') return -1;
    *fcb = 3;
    sp += 2;
  }
  else {
    if (!(flag & 2))
      *fcb = 0;
  }
  
  while (p = strpbrk(sp, ":.;,=+/\"[]\\<>| \t\r\n")) {
    if (*p == '.') break;
    if (!(flag & 1)) {
      if (!(flag & 4)) fcb[1] = ' ';
      return 0;
    }
    sp = p + 1;
  }
  
  if (p)
    *p++ = 0;
  
  if (! *sp) {
    if (!(flag & 4)) fcb[1] = ' ';
  }
  else {
    int i;
    for (i = 1; i < 9 && *sp; ++i, ++sp) {
      if (*sp == '*') {
	ret = 1;
	for ( ; i < 9; ++i)
	  fcb[i] = '?';
	break;
      }
      else if (islower(*sp))
	fcb[i] = toupper(*sp);
      else {
	if (*sp == '?') ret = 1;
	fcb[i] = *sp;
      }
    }
    for ( ; i < 9; ++i )
      fcb[i] = ' ';
  }

  if (!p || !*p) {
    if (!(flag & 8)) fcb[10] = ' ';
  }
  else {
    int i;
    for (i = 9; i < 12 && *p; ++i, ++p) {
      if (*p == '*') {
	ret = 1;
	for ( ; i < 12; ++i)
	  fcb[i] = '?';
	++p;
	break;
      }
      else if (islower(*p))
	fcb[i] = toupper(*p);
      else {
	if (*p == '?') ret = 1;
	fcb[i] = *p;
      }
    }
    for ( ; i < 12; ++i )
      fcb[i] = ' ';
  }
  
  *nb = sp - buf;
  return ret;
}

void
encode_dos_file_time (t, datep, timep)
time_t t;
u_short *datep;
u_short *timep;
{
	struct tm tm;

	tm = *localtime (&t);
	*timep = (tm.tm_hour << 11)
		| (tm.tm_min << 5)
			| (tm.tm_sec / 2);
	*datep = ((tm.tm_year - 80) << 9)
		| (tm.tm_mon << 5)
			| (tm.tm_mday - 1);
}

#define FIND_MAGIC 0x38234325

struct dos_find {
	/* first 20 bytes are reserved for the os */
	int magic;
	glob_t *glob;
	glob_t *glob1;
	int idx;
	int idx1;
	char pad;

	char attributes;
	char time[2];
	char date[2];
	char size[4];
	char name[13];
};

do_find_first (tf)
struct trapframe *tf;
{
	u_char *pattern;
    	u_char *p, *q;
    	int drive = diskdrive;
	dosdir_t dosdir;
    	find_block_t *dta = (find_block_t *)disk_transfer_addr;

	tf->tf_eflags |= PSL_C;

	pattern = (u_char *)MAKE_ADDR (tf->tf_ds, tf->tf_dx);
    	if (strchr((char *)pattern, ':')) {
	    if (pattern[1] != ':')
		return(2);
    	    if (islower(pattern[0]))
	    	drive = pattern[0] - 'a';
    	    else if (isupper(pattern[0]))
	    	drive = pattern[0] - 'A';
	    else {
		tf->tf_ax = PATH_NOT_FOUND;
		return;
	    }
    	    pattern += 2;
    	}
    	tf->tf_ax = find_first(drive, pattern, tf->tf_cx, &dosdir, dta);
    	if (tf->tf_ax) {
	    return;
    	}
    	*(u_long *)dta->size = dosdir.size;
    	dta->time = dosdir.time;
    	dta->date = dosdir.date;
    	dta->attribute = dosdir.attr;
    	p = dta->name;
    	for (q = dosdir.name; q < dosdir.name + 8 && *q != ' '; ++q)
	    *p++ = *q;
    	*p++ = '.';
    	for (q = dosdir.ext; q < dosdir.ext + 3 && *q != ' '; ++q)
	    *p++ = *q;
	*p = 0;
	tf->tf_eflags &= ~PSL_C;
}

do_find_next (tf)
struct trapframe *tf;
{
    	u_char *p, *q;
	dosdir_t dosdir;
    	find_block_t *dta = (find_block_t *)disk_transfer_addr;

	tf->tf_eflags |= PSL_C;

    	tf->tf_ax = find_next(&dosdir, dta);
    	if (tf->tf_ax)
	    return;
    	*(u_long *)dta->size = dosdir.size;
    	dta->time = dosdir.time;
    	dta->date = dosdir.date;
    	dta->attribute = dosdir.attr;
    	p = dta->name;
    	for (q = dosdir.name; q < dosdir.name + 8 && *q != ' '; ++q)
	    *p++ = *q;
    	*p++ = '.';
    	for (q = dosdir.ext; q < dosdir.ext + 3 && *q != ' '; ++q)
	    *p++ = *q;
	*p = 0;

	tf->tf_eflags &= ~PSL_C;
}

do_times (tf)
struct trapframe *tf;
{
	struct byteregs *b = (struct byteregs *)&tf->tf_bx;
	struct stat statb;
	int fd;

	fd = tf->tf_bx;

	switch (b->tf_al) {
	case 0:
		if (fstat (fd, &statb) < 0) {
			tf->tf_eflags |= PSL_C;
			tf->tf_ax = 6;
		} else {
			encode_dos_file_time (statb.st_mtime,
					      &tf->tf_cx,
					      &tf->tf_dx);
		}
		break;
	case 1:
		/* set time ... ignore since we don't have futime */
		tf->tf_eflags &= ~PSL_C;
		break;
	default:
		tf->tf_eflags |= PSL_C;
		tf->tf_ax = 1;
		break;
	}
}
