/*
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
 
/*	Krystal $Id: int13.c,v 1.2 1994/01/15 21:17:47 polk Exp $ */

#include "doscmd.h"

#include <sys/ioctl.h>

#define	FDCHANGED	_IOR('F', 64, int)

#define INT13_ERR_NONE               0x00
#define INT13_ERR_BAD_COMMAND        0x01
#define INT13_ERR_BAD_ADDRESS_MARK   0x02
#define INT13_ERR_WRITE_PROTECT      0x03
#define INT13_ERR_SECTOR_ID_BAD      0x04
#define INT13_ERR_RESET_FAILURE      0x05
#define INT13_ERR_CLL_ACTIVE         0x06
#define INT13_ERR_ACT_FAILED         0x07
#define INT13_ERR_DMA_OVERRUN        0x08
#define INT13_ERR_DMA_BOUNDARY       0x09
#define INT13_ERR_BAD_TRACK_FLAG     0x0B
#define INT13_ERR_MEDIA_TYP_UNKNOWN  0x0C
#define INT13_ERR_CRC                0x10
#define INT13_ERR_CORRECTED          0x11
#define INT13_ERR_CTRLR_FAILURE      0x20
#define INT13_ERR_SEEK               0x40
#define INT13_ERR_TIME_OUT           0x80
#define INT13_ERR_NOT_READY          0xAA
#define INT13_ERR_UNDEFINED          0xBB
#define INT13_ERR_SENSE_OPERATION    0xFF

typedef struct {
	u_char	bootIndicator;
	u_char	beginHead;
	u_char	beginSector;
	u_char	beginCyl;
	u_char	systemID;
	u_char	endHead;
	u_char	endSector;
	u_char	endCyl;
	u_long	relSector;
	u_long	numSectors;
} PTAB;

struct diskinfo {
	int		type;
	int		sectors;
	int		cylinders;
	int		sides;
	int		secsize;
	int		fd;
	char		*path;
	u_long		location;
    	u_char		*sector0;
	char		*list[4];		/* Up to 4 devices allowed */
	unsigned	multi:2;
    	int		read_only:1;
    	int		removeable:1;
    	int		changed:1;		/* Set if we change format */
};

#define	hd_status	(*(u_char *)0x474)
#define	fd_status	(*(u_char *)0x441)

inline
disize(struct diskinfo *di)
{
    return(di->sectors * di->cylinders * di->sides);
}

inline
cylsize(struct diskinfo *di)
{
    return(di->sectors * di->sides);
}

static u_long ftable = 0xF1000;	/* Floppy table */
static u_long htable = 0xF1020;	/* Hard disk table */

static struct diskinfo diskinfo[26] = { 0 };

static struct diskinfo floppyinfo[] = {
	{    0,  9,   40,  1, 512, -1, 0, 0, }, /* Probably not correct */
	{    1,  9,   40,  2, 512, -1, 0, 0, }, 
	{    2,  9,   80,  2, 512, -1, 0, 0, }, 
	{    3, 15,   80,  2, 512, -1, 0, 0, }, 
	{    4, 18,   80,  2, 512, -1, 0, 0, },
	{    6, 36,   80,  2, 512, -1, 0, 0, },
	{   -1,  0,    0,  0,   0,  0, 0, 0, },
};

extern char *bootfile;

static struct diskinfo *
getdisk(int drive)
{
	struct diskinfo *di;

    	if (drive >= 2 && drive < 0x80) {
		return(0);
	}
    	if (drive >= 0x80) {
	    	drive -= 0x80;
		drive += 2;
    	}

	if (drive > 25 || diskinfo[drive].path == 0) {
		return(0);
	}
	di = &diskinfo[drive];
	if (di->fd < 0) {
	    if (di->removeable) {
		di->read_only = 0;
		if (!(di->path = di->list[di->multi]))
		    di->path = di->list[di->multi = 0];
	    }
	    if ((di->fd = open(di->path, di->read_only ? O_RDONLY
						       : O_RDWR|O_FSYNC)) < 0 &&
		(di->read_only = 1) &&
	        (di->fd = open(di->path, O_RDONLY)) < 0) {
		    return(0);
		}
	    di->fd = squirrel_fd(di->fd);
	}
	return(di);
}

int
disk_fd(int drive)
{
    struct diskinfo *di;

    if (drive > 1)
	drive += 0x80 - 2;
    di = getdisk(drive);
    if (!di)
	return(-1);
    return(di->fd);
}

void
make_readonly(int drive)
{
    if (drive < 0 || drive >= 26)
	return;
    diskinfo[drive].read_only = 1;
}

int
init_hdisk(int drive, int cyl, int head, int tracksize, char *file, char *fake_ptab)
{
    struct diskinfo *di;
    u_long table;

    if (drive < 0) {
	for (drive = 2; drive < 26; ++drive) {
	    if (diskinfo[drive].path == 0)
		break;
	}
    }
    if (drive < 2) {
    	fprintf(stderr, "Only floppies may be assigned to A: or B:\n");
	return(-1);
    }

    if (drive >= 26) {
	fprintf(stderr, "Too many disk drives (only 24 allowed)\n");
	return(-1);
    }

    di = &diskinfo[drive];

    if (di->path) {
	fprintf(stderr, "Drive %c: already assigned to %s\n",
			 drive + 'A', di->path);
	return(-1);
    }
    di->fd = -1;
    di->sectors = tracksize;
    di->cylinders = cyl;
    di->sides = head;
    di->sector0 = 0;

    if (fake_ptab) {
	u_char buf[512];
	int fd;
	PTAB *ptab;
	int clusters;

    	if ((fd = open(fake_ptab, 0)) < 0) {
		perror(fake_ptab);
		return(-1);
	}
    	di->sector0 = malloc(512);
	if (!di->sector0) {
		perror("malloc in init_hdisk");
		exit(1);
	}

	read(fd, di->sector0, 512);
	close(fd);

    	ptab = (PTAB *)(di->sector0 + 0x01BE);
	memset(ptab, 0, sizeof(PTAB) * 4);
	ptab->bootIndicator = 0x80;
	ptab->beginHead = 0;
	ptab->beginSector = 1;	/* this is 1 based */
	ptab->beginCyl = 1;

	ptab->endHead = head - 1;
	ptab->endSector = tracksize;	/* this is 1 based */
	ptab->endCyl = cyl & 0xff;
	ptab->endSector |= (cyl & 0x300) >> 2;

    	ptab->relSector = head * tracksize;
    	ptab->numSectors = head * tracksize * cyl;
	
    	*(u_short *)(di->sector0 + 0x1FE) = 0xAA55;

	fd = open(file, 0);
	if (fd < 0) {
	    perror(file);
	    return(-1);
	}
	memset(buf, 0, 512);
	read(fd, buf, 512);
    	close(fd);
    	if ((clusters = buf[0x0D]) == 0) {
	    if (disize(di) <= 128 * 2048)
	    	clusters = 4;
	    else if (disize(di) <= 256 * 2048)
		clusters = 8;
	    else if (disize(di) <= 8 * 1024 * 2048)
		clusters = 16;
	    else if (disize(di) <= 16 * 1024 * 2048)
		clusters = 32;
	    else
		clusters = 64;
	}
    	if ((disize(di) / clusters) <= 4096) {
	    ptab->systemID = 0x01;
	} else {
	    ptab->systemID = 0x04;
	}

	di->cylinders += 1;	/* Extra cylinder for partition table, etc. */
    }
    di->type = 0xf8;
    di->path = file;
    di->secsize = 512;
    di->path = strdup(file);


    di->location = ((table & 0xf0000) << 12) | (table & 0xffff);

    if (drive == 0) {
	ivec[0x41] = di->location;
    } else if (drive == 1) {
	ivec[0x46] = di->location;
    }

    table = htable + (drive - 2) * 0x10;
    *(u_short *)(table+0x00) = di->cylinders-1;	/* Cylinders */
    *(u_char  *)(table+0x02) = di->sides;	/* Heads */
    *(u_short *)(table+0x03) = 0;		/* 0 */
    *(u_short *)(table+0x05) = 0xffff;		/* write pre-comp */
    *(u_char  *)(table+0x07) = 0;		/* ECC Burst length */
    *(u_char  *)(table+0x08) = 0;		/* Control Byte */
    *(u_char  *)(table+0x09) = 0;		/* standard timeout */
    *(u_char  *)(table+0x0a) = 0;		/* formatting timeout */
    *(u_char  *)(table+0x0b) = 0;		/* timeout for checking drive */
    *(u_short *)(table+0x0c) = di->cylinders-1;	/* landing zone */
    *(u_char  *)(table+0x0e) = di->sectors;	/* sectors/track */
    *(u_char  *)(table+0x0f) = 0;

    if (drive >= ndisks)
	ndisks = drive + 1;
    return(drive);
}

inline
bps(int size)
{
    switch (size) {
    case 128:	return(0);
    case 256:	return(1);
    case 512:	return(2);
    case 1024:	return(3);
    default:
	fprintf(stderr, "Invalid sector size: %d\n", size);
	exit(1);
    }
}

int
init_floppy(int drive, int type, char *file)
{
    struct diskinfo *di = floppyinfo;
    u_long table;
    struct stat sb;

    while (di->type >= 0 && di->type != type && disize(di)/2 != type)
	++di;

    if (!di->type) {
	fprintf(stderr, "Invalid floppy type: %d\n", type);
	return(-1);
    }

    if (drive < 0) {
    	if (diskinfo[0].path == 0) {
	    drive = 0;
    	} else if (diskinfo[1].path == 0) {
	    drive = 1;
	} else {
	    fprintf(stderr, "Too many floppy drives (only 2 allowed)\n");
	    return(-1);
    	}
    }
    if (drive > 1) {
	fprintf(stderr, "Floppies must be either drive A: or B:\n");
	return(-1);
    }

    if (drive >= nfloppies)
	nfloppies = drive + 1;

    if (diskinfo[drive].path == 0) {
	diskinfo[drive] = *di;
    }

    di = &diskinfo[drive];

    if (stat(file, &sb) < 0) {
	fprintf(stderr, "Drive %c: Could not stat %s\n",
			 drive + 'A', file);
	return(-1);
    }

    if (drive < 2 && sb.st_mode & (S_IFCHR|S_IFBLK)) {
	if (di->path && !di->removeable) {
	    fprintf(stderr, "Drive %c: is not removeable and hence can only have one assignment\n", drive + 'A');
	    return(-1);
	}
	di->removeable = 1;
    } else if (di->removeable) {
	fprintf(stderr, "Drive %c: already assigned to %s\n",
			 drive + 'A', di->path);
	return(-1);
    }

    if (di->removeable) {
	if (di->multi == 4) {
	    fprintf(stderr, "Drive %c: already assigned 4 devices\n",
			     drive + 'A');
	    return(-1);
	}
	di->path = di->list[di->multi++] = strdup(file);
    } else {
	if (di->path) {
	    fprintf(stderr, "Drive %c: already assigned to %s\n",
			     drive + 'A', di->path);
	    return(-1);
	}

	di->path = strdup(file);
    }
    di->fd = -1;
    di->location = ((table & 0xf0000) << 12) | (table & 0xffff);
    di->sector0 = 0;


    ivec[0x1e] = ((ftable & 0xf0000) << 12) | (ftable & 0xffff);

    table = ftable + drive * 0x0a;

    *(u_char *)(table+0x00) = 0xdf;	/* First Specify Byte */
    *(u_char *)(table+0x01) = 0x02;	/* Second Specify Byte */
    *(u_char *)(table+0x02) = 0x25;	/* Timer ticks to wait 'til motor OFF */
    *(u_char *)(table+0x03) = bps(di->secsize);	/* Number of bytes/sector */
    *(u_char *)(table+0x04) = di->sectors;	/* Number of sectors/track */
    *(u_char *)(table+0x05) = 0x1b;	/* Gap length, in bytes */
    *(u_char *)(table+0x06) = 0xff;	/* Data length, in bytes */
    *(u_char *)(table+0x07) = 0x6c;	/* Gap length for format */
    *(u_char *)(table+0x09) = 0xf6;	/* Fill byte for formatting */
    *(u_char *)(table+0x09) = 0x0f;	/* Head settle time, in milliseconds */
    *(u_char *)(table+0x0a) = 0x08;	/* Motor startup time, in 1/8 seconds */
    return(drive);
}

static int icnt = 0;

#define	seterror(err)	{ \
    	if (drive & 0x80) \
	    hd_status = err; \
	else \
	    fd_status = err; \
	b->tf_ah = err; \
	tf->tf_eflags |= PSL_C; \
}
	    
static
trynext(struct diskinfo *di)
{
	close(di->fd);
	di->fd = -1;
	di->changed = 1;
	if (di->multi++ >= 4)
	    return(0);
	if (di->list[di->multi] && (di = getdisk(di - diskinfo))) {
	    di->multi = 0;
	    return(1);
	}
	return(0);
}

int
diread(struct diskinfo *di, struct trapframe *tf,
       off_t start, char *addr, int sectors)
{
	struct byteregs *b = (struct byteregs *)&tf->tf_bx;
	off_t res;

	int drive = di - diskinfo;
	di->multi = -1;

	if (drive > 1) {
		drive -= 2;
		drive |= 0x80;
	}

again:
	res = lseek(di->fd, start * di->secsize, 0);

	if (res < 0 && di->removeable && trynext(di))
	    goto again;

	if (res < 0) {
	    seterror(INT13_ERR_SEEK);
	    return(-1);
	}

	res = read(di->fd, addr, sectors * di->secsize);

	if (res < 0 && di->removeable && trynext(di))
	    goto again;

	if (di->removeable) {
	    if (res < 0) {
		seterror(INT13_ERR_NOT_READY);
		return(-1);
	    }
	    return(res / di->secsize);
	}

	/*
	 * reads always work, if if they don't.
	 * Just pretend any byte not read was actually a 0
	 */
	if (res < 0)
		memset(addr, 0, sectors * di->secsize);
	else if (res < sectors * di->secsize)
		memset(addr + res, 0, sectors * di->secsize - res);
	
	return(sectors);
}

int
diwrite(struct diskinfo *di, struct trapframe *tf,
       off_t start, char *addr, int sectors)
{
	struct byteregs *b = (struct byteregs *)&tf->tf_bx;
	off_t res;
	int drive = di - diskinfo;
	di->multi = -1;

	if (drive > 1) {
		drive -= 2;
		drive |= 0x80;
	}

again:
	res = lseek(di->fd, start * di->secsize, 0);

	if (res < 0 && di->removeable && trynext(di))
	    goto again;

	if (res < 0) {
	    seterror(INT13_ERR_SEEK);
	    return(-1);
	}

	res = write(di->fd, addr, sectors * di->secsize);

	if (res < 0 && di->removeable && trynext(di))
	    goto again;

	if (di->removeable) {
	    if (res < 0) {
		seterror(INT13_ERR_NOT_READY);
		return(-1);
	    }
	} else if (res < 0) {
		seterror(INT13_ERR_WRITE_PROTECT);
		return(-1);
	}
	return(res / di->secsize);
}

void
int13 (tf)
struct trapframe *tf;
{
	char *addr;
	struct byteregs *b = (struct byteregs *)&tf->tf_bx;
	int sectors;
	struct diskinfo *di;
	off_t start;
    	int did;

	int cyl;
	int sector;
	int side;
	int drive;

	reset_poll();

        tf->tf_eflags &= ~PSL_C;

	drive = b->tf_dl;


	if (b->tf_ah != 0x01) {
	    if (drive & 0x80) 
		hd_status = 0;
	    else
		fd_status = 0;
	}

	switch (b->tf_ah) {
	case 0x00:	/* Reset */
		break;
	case 0x01:	/* Read disk status */ 
		if (drive & 0x80) 
		    b->tf_ah = hd_status;
		else
		    b->tf_ah = fd_status;
		if (b->tf_ah)
		    tf->tf_eflags |= PSL_C;
		break;
	case 0x02:	/* Read */
		b->tf_ah = 0;
		addr = MAKE_ADDR(tf->tf_es, tf->tf_bx);
		sectors = b->tf_al;
		side = b->tf_dh;
    	    	b->tf_al = 0;	/* Start out with nothing read */

		if (drive & 0x80) {
		    cyl = b->tf_ch | ((b->tf_cl & 0xc0) << 2);
		    sector = (b->tf_cl & 0x3f) - 1;
		} else {
		    sector = b->tf_cl - 1;
		    cyl = b->tf_ch;
    	    	}

		if ((di = getdisk(drive)) == 0) {
			debug(D_DISK, "Bad drive: %d (%d : %d : %d)\n",
				drive, cyl, side, sector);
			seterror(INT13_ERR_BAD_COMMAND);
			break;
		}
		start = cyl * di->sectors * di->sides +
			side * di->sectors +
			sector;

    	    	if (start >= disize(di)) {
			debug(D_DISK, "Read past end of disk\n");
			seterror(INT13_ERR_SEEK);
			break;
		}
    	    	if (sectors + start >= disize(di)) {
		    sectors = disize(di) - start;
		}

    	    	if (di->sector0) {
		    if (start < cylsize(di)) {
			b->tf_al = sectors;
			if (start == 0) {
			    memcpy(addr, di->sector0, di->secsize);
			    addr += di->secsize;
			    --sectors;
			}
			memset(addr, 0, sectors * di->secsize);
			break;
		    } else
			start -= di->sectors * di->sides;
		}
		debug(D_DISK, "%02x: Read %2d sectors from %d to %04x:%04x\n",
			      drive, sectors, start, tf->tf_es, tf->tf_bx);

		if ((did = diread(di, tf, start, addr, sectors)) >= 0)
		    b->tf_al = did;
		break;
	case 0x03:	/* Write */
		b->tf_ah = 0;
		addr = MAKE_ADDR(tf->tf_es, tf->tf_bx);
		sectors = b->tf_al;
		side = b->tf_dh;
    	    	b->tf_al = 0;	/* Start out with nothing written */


		if (drive & 0x80) {
		    cyl = b->tf_ch | ((b->tf_cl & 0xc0) << 2);
		    sector = (b->tf_cl & 0x3f) - 1;
		} else {
		    sector = b->tf_cl - 1;
		    cyl = b->tf_ch;
    	    	}

		if ((di = getdisk(drive)) == 0) {
			debug(D_DISK, "Bad drive: %d (%d : %d : %d)\n",
				drive, cyl, side, sector);
			seterror(INT13_ERR_BAD_COMMAND);
			break;
		}
    	    	if (di->read_only) {
			debug(D_DISK, "%02x: Attempt to write readonly disk\n", drive);
			seterror(INT13_ERR_WRITE_PROTECT);
			break;
    	    	}
		start = cyl * di->sectors * di->sides +
			side * di->sectors +
			sector;

    	    	if (start >= disize(di)) {
			debug(D_DISK, "Read past end of disk\n");
			seterror(INT13_ERR_SEEK);
			break;
		}

    	    	if (sectors + start >= disize(di))
		    sectors = disize(di) - start;

    	    	if (di->sector0) {
		    if (start < di->sectors * di->sides) {
			b->tf_al = sectors;
			break;
		    } else
			start -= di->sectors * di->sides;
		}

		debug(D_DISK, "Write %2d sectors from %d to %04x:%04x\n",
			      sectors, start, tf->tf_es, tf->tf_bx);

		if ((did = diwrite(di, tf, start, addr, sectors)) >= 0)
		    b->tf_al = did;
		break;
    	case 0x04:
		b->tf_ah = 0;
		addr = MAKE_ADDR(tf->tf_es, tf->tf_bx);
		sectors = b->tf_al;
		side = b->tf_dh;

		if (drive & 0x80) {
		    cyl = b->tf_ch | ((b->tf_cl & 0xc0) << 2);
		    sector = (b->tf_cl & 0x3f) - 1;
		} else {
		    sector = b->tf_cl - 1;
		    cyl = b->tf_ch;
    	    	}

		if ((di = getdisk(drive)) == 0) {
			debug(D_DISK, "Bad drive: %d (%d : %d : %d)\n",
				drive, cyl, side, sector);
			seterror(INT13_ERR_BAD_COMMAND);
			break;
		}
		start = cyl * di->sectors * di->sides +
			side * di->sectors +
			sector;

    	    	if (start >= disize(di)) {
			debug(D_DISK, "Read past end of disk\n");
			seterror(INT13_ERR_SEEK);
			break;
		}

    	    	if (sectors + start >= disize(di))
		    sectors = disize(di) - start;

    	    	if (di->sector0) {
		    if (start < di->sectors * di->sides) {
			break;
		    } else
			start -= di->sectors * di->sides;
		}

		debug(D_DISK, "Verify %2d sectors from %d to %04x:%04x\n",
			      sectors, start, tf->tf_es, tf->tf_bx);
		if (lseek(di->fd, start * di->secsize, 0) < 0) {
			debug(D_DISK, "Seek error\n");
			seterror(INT13_ERR_SEEK);
			break;
		}
		while (sectors > 0) {
		    char buf[512];
		    if (read(di->fd, buf, di->secsize) != di->secsize) {
			debug(D_DISK, "Verify error\n");
			seterror(0x04);
			break;
		    }
		    if (memcmp(buf, addr, di->secsize)) {
			debug(D_DISK, "Verify error\n");
			seterror(0x10);
			break;
		    }
		    --sectors;
		    addr += di->secsize;
		}
		break;
	case 0x05:	/* Format track */
		seterror(INT13_ERR_BAD_COMMAND);
		break;
	case 0x08:	/* Status */
		b->tf_ah = 0;

		if ((di = getdisk(drive)) == 0) {
			debug(D_DISK, "Bad drive: %d\n", drive);
			seterror(INT13_ERR_BAD_COMMAND);
			break;
		}
		tf->tf_ax = 0;

		tf->tf_bx = di->type;
		if ((drive & 0x80) == 0) {
		    tf->tf_es = di->location >> 16;
		    tf->tf_di = di->location & 0xffff;
		}

		b->tf_cl = di->sectors | ((di->cylinders >> 2) & 0xc0);
		b->tf_ch = di->cylinders & 0xff;
		b->tf_dl = (drive & 0x80) ? ndisks : nfloppies;
		b->tf_dh = di->sides - 1;
		break;
    	case 0x0c:	/* Move read/write head */
    	case 0x0d:	/* Reset */
		break;
	case 0x15:
		if ((di = getdisk(drive)) == 0) {
			b->tf_ah = 0;
			tf->tf_eflags |= PSL_C;
			break;
		}

		if (drive & 0x80) {
			start = di->sectors * di->cylinders * di->sides;
			tf->tf_cx = (start >> 16) & 0xffff;
			tf->tf_dx = start & 0xffff;
			b->tf_ah = 3;
		} else
			b->tf_ah = 1;	/* Non-changeable disk */
		break;
	case 0x16:	/* Media change */
		b->tf_ah = 0;
		if ((di = getdisk(drive)) && di->changed) {
		    	di->changed = 0;
			b->tf_ah = 6;
		}
		break;
    	case 0x17:	/* Determine floppy disk format */
		seterror(INT13_ERR_BAD_COMMAND);
		break;
    	case 0x18:	/* Determine disk format */
		if ((di = getdisk(drive)) == 0) {
			b->tf_ah = 0;
			tf->tf_eflags |= PSL_C;
			break;
		}
		if (drive & 0x80) {
		    cyl = b->tf_ch | ((b->tf_cl & 0xc0) << 2);
		    sector = (b->tf_cl & 0x3f) - 1;
		} else {
		    sector = b->tf_cl - 1;
		    cyl = b->tf_ch;
    	    	}
		break;
	default:
		unknown_int2(0x13, b->tf_ah, tf);
		break;
	}
}
