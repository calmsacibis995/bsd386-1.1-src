/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	Krystal $Id: intff.c,v 1.2 1994/01/15 21:18:46 polk Exp $ */
#include "doscmd.h"
#include "errno.h"

static LOL *lol = 0;
static SDA *sda = 0;

inline
from_dos_attr(int attr)
{
    return((attr & READ_ONLY_FILE) ? 0444 : 0666);
}

void
intff(tf)
struct trapframe *tf;
{
    int drive;
    u_char *path;

    /*
     * ES:DI List of lists
     * DS:SI Swappable Data Area
     */

    if (lol && sda)
	return;
    lol = (LOL *)MAKE_ADDR(tf->tf_es, tf->tf_di);
    sda = (SDA *)MAKE_ADDR(tf->tf_ds, tf->tf_si);

    for (drive = 0; drive < 26; ++drive) {
    	if (path = dos_getpath(drive))
	    install_drive(drive, path);
    }
}

int
install_drive(int drive, u_char *path)
{
    char *dpb;
    CDS *cds;


    if (drive < 0 || drive >= lol->lastdrive) {
	debug(D_REDIR, "Drive %c beyond limit of %c)\n",
			drive + 'A', lol->lastdrive - 1 + 'A');
	return;
    }

    cds = (CDS *)MAKE_ADDR(lol->cds_seg, lol->cds_offset);
    cds += drive;
    dpb = (char *)MAKE_ADDR(*(u_short *)cds->dpb_seg, *(u_short *)cds->dpb_off);

#if 0
    if (cds->flag & (CDS_network | CDS_physical)) {
	debug(D_REDIR, "Drive %c already installed\n", drive + 'A');
	return;
    }
#endif

    debug(D_REDIR, "Installing %c: as %s\n", drive + 'A', path);

    cds->flag |= CDS_network | CDS_physical;
    cds->path[0] = drive + 'A';
    cds->path[1] = ':';
    cds->path[2] = '\\';
    cds->path[3] = '\0';
    *(u_short *)(cds->offset) = 2;	/* offset of \ in current path field */
}

int
int2f_11(struct trapframe *tf)
{               
    u_char filename[1024];
    u_char filenamep[1024];
    u_char *fname;
    CDS *cds;
    struct byteregs *b = (struct byteregs *)&tf->tf_bx;
    u_short action;
    u_short mode;
    u_short attr;
    u_long cnt;
    int drive;
    struct stat sb;
    int fd;
    SFT *sft;
    u_char *p, *e;
    int i;

    reset_poll();

    if (!sda)
	return(0);

    cds = (CDS *)MAKE_ADDR(sda->cds_seg, sda->cds_off);
    sft = (SFT *)MAKE_ADDR(tf->tf_es, tf->tf_di);

    debug(D_REDIR, "INT 2F:11:%02x", b->tf_al);

    switch (b->tf_al) {
    default:
    case 0x24:	/* TURN_OFF_PRINTER */
    case 0x25:	/* PRINTER_MODE			Used in DOS 3.1+ */
    case 0x2d:	/* EXTENDED_ATTRIBUTES	Used in DOS 4.x */
	goto skip;
    case 0x00:	/* INSTALLATION_CHECK */
    case 0x1e:	/* CONTROL_REDIRECT */
    case 0x22:	/* PROCESS_TERMINATED */
    case 0x1d:	/* CLOSE_ALL */
	drive = -1;
    case 0x0c:	/* GET_DISK_SPACE */
	cds = (CDS *)MAKE_ADDR(tf->tf_es, tf->tf_di);
    case 0x01:	/* REMOVE_DIRECTORY */
    case 0x02:	/* REMOVE_DIRECTORY_2 */
    case 0x03:	/* MAKE_DIRECTORY */
    case 0x04:	/* MAKE_DIRECTORY_2 */
    case 0x05:	/* SET_CURRENT_DIRECTORY */
    case 0x11:	/* RENAME_FILE */
    case 0x17:	/* CREATE_TRUNCATE_FILE */
    case 0x2e:	/* MULTIPURPOSE_OPEN	Used in DOS 4.0+ */
    case 0x1c:	/* FIND_NEXT */
    case 0x1f:	/* PRINTER_SETUP */
    case 0x20:	/* FLUSH_ALL_DISK_BUFFERS */
	fname = (u_char *)cds->path;
	goto checkname;
    case 0x23:	/* QUALIFY_FILENAME */
	fname = (u_char *)MAKE_ADDR(tf->tf_ds, tf->tf_si);
	goto checkname;

    case 0x18:	/* CREATE_TRUNCATE_NO_CDS */
    case 0x16:	/* OPEN_EXISTING_FILE */
    case 0x0e:	/* SET_FILE_ATTRIBUTES */
    case 0x0f:	/* GET_FILE_ATTRIBUTES */
    case 0x13:	/* DELETE_FILE */
    case 0x1b:	/* FIND_FIRST */
    case 0x19:	/* FIND_FIRST_NO_CDS */
	fname = sda->filename1;
	goto checkname;
    checkname:
	debug(D_REDIR, " [%s]", fname);
	if (fname[1] != ':') {
	    goto skip;
	}
	drive = fname[0] - 'A';
	break;
    case 0x06:	/* CLOSE_FILE */
    case 0x07:	/* COMMIT_FILE */
    case 0x08:	/* READ_FILE */
    case 0x09:	/* WRITE_FILE */
    case 0x0a:	/* LOCK_FILE_REGION */
    case 0x0b:	/* UNLOCK_FILE_REGION */
    case 0x21:	/* SEEK_FROM_EOF */
	drive = *(u_short *)sft->info & 0x1f;
	break;
    }

    debug(D_REDIR, " drive %c", drive + 'A');
    if (drive < 0 || !dos_getcwd(drive)) {
	goto skip;
    }

    switch (b->tf_al) {
    case 0x00:	/* INSTALLATION_CHECK */
	b->tf_al = 0xff;
	goto okay;
    case 0x01:	/* REMOVE_DIRECTORY */
    case 0x02:	/* REMOVE_DIRECTORY_2 */
	debug(D_REDIR, "rmdir(%s)\n", sda->filename1);
    	if (dos_readonly(drive)) {
	    tf->tf_ax = WRITE_PROT_DISK;
	    goto error;
    	}
    	if (dos_to_real_path(drive, sda->filename1 + 2, filename)) {
    	    debug(D_REDIR, "Failed to map %s on %c\n", sda->filename1 + 2, drive + 'A');
	    return(0);
	}
    	if (rmdir(filename) < 0) {
	    switch (errno) {
	    case EINVAL:
	    case ENOENT:
		tf->tf_ax = PATH_NOT_FOUND;
		break;
	    default:
		tf->tf_ax = ACCESS_DENIED;
		break;
	    }
	    goto error;
	}
	goto okay;
    case 0x03:	/* MAKE_DIRECTORY */
    case 0x04:	/* MAKE_DIRECTORY_2 */
	debug(D_REDIR, "mkdir(%s)\n", sda->filename1);
    	if (dos_readonly(drive)) {
	    tf->tf_ax = WRITE_PROT_DISK;
	    goto error;
    	}
    	if (dos_to_real_path(drive, sda->filename1 + 2, filename)) {
    	    debug(D_REDIR, "Failed to map %s on %c\n", sda->filename1 + 2, drive + 'A');
	    return(0);
	}
    	if (mkdir((char *)filename, 0777) < 0) {
	    switch(errno) {
	    case ENOENT:
		tf->tf_ax = PATH_NOT_FOUND;
		break;
	    default:
		tf->tf_ax = ACCESS_DENIED;
		break;
	    }
	    goto error;
	}
	goto okay;
    case 0x05:	/* SET_CURRENT_DIRECTORY */
	debug(D_REDIR, "chdir(%s)\n", sda->filename1);
	if (tf->tf_ax = dos_setcwd(drive, sda->filename1 + 2))
	    goto error;
	goto okay;
	goto okay;
    case 0x06:	/* CLOSE_FILE */
	debug(D_REDIR, "close(%d)\n", *(u_short *)sft->fd);
	if ((sft->nfiles -= 1) <= 0) {
	    if (close(*(u_short *)sft->fd) < 0) {
		debug(D_REDIR, "%s: %s\n", *(u_short *)sft->fd, strerror(errno));
		tf->tf_ax = HANDLE_INVALID;
		goto error;
	    }
	    debug(D_REDIR, "Close was okay\n");
	}
	else debug(D_REDIR, "No close requested\n");
	goto okay;
    case 0x07:	/* COMMIT_FILE */
	break;
    case 0x08:	/* READ_FILE */
	debug(D_REDIR, "read(%d)\n", *(u_short *)sft->fd);
    	if (lseek(*(u_short *)sft->fd, *(u_long *)sft->offset, 0) < 0) {
	    tf->tf_ax = SEEK_ERROR;
	    goto error;
	}
	i = read(*(u_short *)sft->fd,
		 MAKE_ADDR(sda->dta_seg, sda->dta_off),
		 tf->tf_cx);
	if (i < 0) {
	    tf->tf_ax = READ_FAULT;
	    goto error;
	}
	tf->tf_cx = i;
    	*(u_long *)sft->offset += i;
	goto okay;
    case 0x09:	/* WRITE_FILE */
	debug(D_REDIR, "write(%d)\n", *(u_short *)sft->fd);
    	if (lseek(*(u_short *)sft->fd, *(u_long *)sft->offset, 0) < 0) {
	    tf->tf_ax = SEEK_ERROR;
	    goto error;
	}
	i = write(*(u_short *)sft->fd,
		 MAKE_ADDR(sda->dta_seg, sda->dta_off),
		 tf->tf_cx);
	if (i < 0) {
	    debug(D_REDIR, "%d: %s\n", *(u_short *)sft->fd, strerror(errno));
	    tf->tf_ax = WRITE_FAULT;
	    goto error;
	}
	debug(D_REDIR, "%d: Wrote %d of %d\n", *(u_short *)sft->fd, i, tf->tf_cx);
	tf->tf_cx = i;
    	*(u_long *)sft->offset += i;
    	if (*(u_long *)sft->offset > *(u_long *)sft->size)
		*(u_long *)sft->size = *(u_long *)sft->offset;
	debug(D_REDIR, "Offset is now %d\n", *(u_long *)sft->offset);
	goto okay;
    case 0x0a:	/* LOCK_FILE_REGION */
    case 0x0b:	/* UNLOCK_FILE_REGION */
	break;
    case 0x0c:	/* GET_DISK_SPACE */
      {
    	fsstat_t fs;

    	if (tf->tf_ax = get_space(drive, &fs))
	    goto error;
    	tf->tf_ax = fs.sectors_cluster;
    	tf->tf_bx = fs.total_clusters;
    	tf->tf_cx = fs.bytes_sector;
    	tf->tf_dx = fs.avail_clusters;
    	goto keep_okay;
      }
    case 0x0e:	/* SET_FILE_ATTRIBUTES */
    case 0x0f:	/* GET_FILE_ATTRIBUTES */
	break;
    case 0x11:	/* RENAME_FILE */
	debug(D_REDIR, "rename(%s, %s)\n", sda->filename1, sda->filename1);
	if (dos_readonly(drive)) {
	    tf->tf_ax = WRITE_PROT_DISK;
	    goto error;
	}
    	if (dos_to_real_path(drive, sda->filename1 + 2, filename)) {
    	    debug(D_REDIR, "Failed to map %s\n", sda->filename1);
	    return(0);
	}
    	if (dos_to_real_path(drive, sda->filename2 + 2, filenamep)) {
    	    debug(D_REDIR, "Failed to map %s\n", sda->filename2);
	    return(0);
	}
    	if (rename((char *)filename, (char *)filenamep) < 0) {
	    switch(errno) {
	    case ENOENT:
		tf->tf_ax = PATH_NOT_FOUND;
		break;
	    default:
		tf->tf_ax = ACCESS_DENIED;
		break;
	    }
	    goto error;
	}
	goto okay;
    case 0x13:	/* DELETE_FILE */
	debug(D_REDIR, "delete(%s)\n", sda->filename1);
    	if (dos_readonly(drive)) {
	    tf->tf_ax = WRITE_PROT_DISK;
	    goto error;
    	}
    	if (dos_to_real_path(drive, sda->filename1 + 2, filename)) {
    	    debug(D_REDIR, "Failed to map %s\n", sda->filename1);
	    return(0);
	}
    	if (unlink(filename) < 0) {
    	    tf->tf_ax = (errno == ENOENT ? PATH_NOT_FOUND : ACCESS_DENIED);
	    goto error;
    	}
	goto okay;
    case 0x16:	/* OPEN_EXISTING_FILE */
	debug(D_REDIR, "open(%s)\n", sda->filename1);
    	mode = sda->open_mode & 0x3;
    	attr = *(u_short *)MAKE_ADDR(tf->tf_ss, tf->tf_sp);

    	if (dos_to_real_path(drive, sda->filename1 + 2, filename)) {
    	    debug(D_REDIR, "Failed to map %s on %c\n", sda->filename1 + 2, drive + 'A');
	    return(0);
	}

    	if (ustat(filename, &sb) < 0) 
	    sb.st_ino = 0;
    do_open:

    	if (mode && dos_readonly(drive)) {
	    tf->tf_ax = WRITE_PROT_DISK;
	    goto error;
    	}

    	if (sb.st_ino == 0 || (sb.st_mode & S_IFDIR)) {
	    tf->tf_ax = FILE_NOT_FOUND;
	    goto error;
    	}

    	if ((fd = open((char *)filename, mode)) < 0) {
	    tf->tf_ax = ACCESS_DENIED;
	    goto error;
    	}

    	e = p = sda->filename1 + 2;

	while (*p) {
	    if (*p++ == '\\')
		e = p;
    	}

    	for (i = 0; i < 8; ++i) {
	    if (*e && *e != '.')
		sft->name[i] = *e++;
	    else
		sft->name[i] = ' ';
    	}

	if (*e == '.')
	    ++e;

    	for (i = 0; i < 3; ++i) {
	    if (*e)
		sft->ext[i] = *e++;
	    else
		sft->ext[i] = ' ';
    	}

    	sft->open_mode = sda->open_mode & 0x7f;
    	*(u_long *)sft->ddr_dpb = 0;
    	*(u_long *)sft->size = sb.st_size;
    	*(u_short *)sft->fd = fd;
    	*(u_long *)sft->offset = 0;
    	*(u_short *)sft->dir_sector = 0;
    	sft->dir_entry = 0;
	sft->attribute = attr;
    	*(u_short *)sft->info = drive + 0x8040;
    	encode_dos_file_time(sb.st_mtime, sft->date, sft->time);

	goto okay;
    case 0x17:	/* CREATE_TRUNCATE_FILE */
    case 0x18:	/* CREATE_TRUNCATE_NO_CDS */
	debug(D_REDIR, "create(%s)\n", sda->filename1);
    	attr = *(u_short *)MAKE_ADDR(tf->tf_ss, tf->tf_sp) & 0xff;

    	if (dos_to_real_path(drive, sda->filename1 + 2, filename)) {
    	    debug(D_REDIR, "Failed to map %s on %c\n", sda->filename1 + 2, drive + 'A');
	    return(0);
	}

    	if (ustat(filename, &sb) < 0)
	    sb.st_ino = 0;

    do_creat:
	debug(D_REDIR, "Create %s (%02x)\n", sda->filename1, attr);

    	if (dos_readonly(drive)) {
	    tf->tf_ax = WRITE_PROT_DISK;
	    goto error;
    	}

    	if (sb.st_ino && (sb.st_mode & S_IFREG) == 0) {
	    debug(D_REDIR, "%s: Access Denined\n", sda->filename1);
	    tf->tf_ax = ACCESS_DENIED;
	    goto error;
    	}

    	fd = open((char *)filename, (O_RDWR|O_CREAT|O_TRUNC),
		  from_dos_attr(attr));

    	if (fd < 0) {
    	    perror((char *)sda->filename1);
	    tf->tf_ax = ACCESS_DENIED;
	    goto error;
    	}

    	e = p = sda->filename1 + 2;

	while (*p) {
	    if (*p++ == '\\')
		e = p;
    	}

    	for (i = 0; i < 8; ++i) {
	    if (*e && *e != '.')
		sft->name[i] = *e++;
	    else
		sft->name[i] = ' ';
    	}

	if (*e == '.')
	    ++e;

    	for (i = 0; i < 3; ++i) {
	    if (*e)
		sft->ext[i] = *e++;
	    else
		sft->ext[i] = ' ';
    	}

    	sft->open_mode = 0x03;
    	*(u_long *)sft->ddr_dpb = 0;
    	*(u_long *)sft->size = 0;
    	*(u_short *)sft->fd = fd;
    	*(u_long *)sft->offset = 0;
    	*(u_short *)sft->dir_sector = 0;
    	sft->dir_entry = 0;
	sft->attribute = attr;
    	*(u_short *)sft->info = drive + 0x8040;
    	encode_dos_file_time(sb.st_mtime, sft->date, sft->time);

	goto okay;
    case 0x19:	/* FIND_FIRST_NO_CDS */
    case 0x1b:	/* FIND_FIRST */
    	debug(D_REDIR, "findfirst(%s)\n", sda->filename1);
    	tf->tf_ax = find_first(drive, sda->filename1 + 2, sda->attrmask,
				      (dosdir_t *)sda->foundentry,
				      (find_block_t *)sda->findfirst);
	if (tf->tf_ax)
	    goto error;
	goto okay;
	break;
    case 0x1c:	/* FIND_NEXT */
    	debug(D_REDIR, "findnext()\n");
    	tf->tf_ax = find_next((dosdir_t *)sda->foundentry,
			       (find_block_t *)sda->findfirst);
	if (tf->tf_ax)
	    goto error;
	goto okay;
	break;

    case 0x21:	/* SEEK_FROM_EOF */
	debug(D_REDIR, "seek(%d)\n", *(u_short *)sft->fd);
    	cnt = (tf->tf_cx << 16) | tf->tf_dx;
    	if ((cnt = lseek(*(u_short *)sft->fd, cnt, 2)) < 0) {
	    tf->tf_ax = SEEK_ERROR;
	    goto error;
	}
    	*(u_long *)sft->offset = cnt;
	goto okay;

    case 0x1d:	/* CLOSE_ALL */
    case 0x1e:	/* CONTROL_REDIRECT */
    case 0x1f:	/* PRINTER_SETUP */
    case 0x20:	/* FLUSH_ALL_DISK_BUFFERS */
    case 0x22:	/* PROCESS_TERMINATED */
    case 0x24:	/* TURN_OFF_PRINTER */
	break;
    case 0x23:	/* QUALIFY_FILENAME */
	debug(D_REDIR, "qualify(%s)\n", fname);
	ustrcpy(filename, fname);
    	if (tf->tf_ax = canonicalize((char *)filename))
	    goto error;
	ustrncpy((u_char *)MAKE_ADDR(tf->tf_es, tf->tf_di), filename, 128);
	goto okay;
    case 0x25:	/* PRINTER_MODE			Used in DOS 3.1+ */
    		/* UNDOCUMENTED_FUNCTION_2	Used in DOS 4.0+ */
    	mode = *(u_short *)MAKE_ADDR(tf->tf_ss, tf->tf_sp);

	switch (mode) {
    	case 0x5D07:
		debug(D_REDIR, "Get printer stream state\n");
		break;
    	case 0x5D08:
		debug(D_REDIR, "Set printer stream state to %s\n",
			(tf->tf_dx & 0xff) == 0x00 ? "combined" :
			(tf->tf_dx & 0xff) == 0x01 ? "separate" : "unknwon");
		break;
    	case 0x5D09:
		debug(D_REDIR, "Finish print job\n");
		break;
	default:
		debug(D_REDIR, "Unknown printer request %04x\n", mode);
		break;
	}
	goto okay;
    case 0x2d:	/* EXTENDED_ATTRIBUTES	Used in DOS 4.x */
	break;
    case 0x2e:	/* MULTIPURPOSE_OPEN	Used in DOS 4.0+ */
    	action = *(u_short *)sda->ext_action;
    	mode = *(u_short *)sda->ext_mode & 3;
    	attr = *(u_short *)sda->ext_attr;
	debug(D_REDIR, "mopen(%s)\n", sda->filename1);

    	if (dos_to_real_path(drive, sda->filename1 + 2, filename)) {
    	    debug(D_REDIR, "Failed to map %s on %c\n", sda->filename1 + 2, drive + 'A');
	    return(0);
	}

    	if (ustat(filename, &sb) < 0)
	    sb.st_ino = 0;

    	if ((action & 0x10) == 0 && sb.st_ino == 0) {
	    debug(D_REDIR, "%s: File not found\n", sda->filename1);
	    tf->tf_ax = FILE_NOT_FOUND;
	    goto error;
	}

    	if ((action & 0x0f) == 0 && sb.st_ino) {
	    debug(D_REDIR, "%s: File already exists\n", sda->filename1);
	    tf->tf_ax = FILE_ALREADY_EXISTS;
	    goto error;
    	}

    	if ((action & 0x0f) == 1 && sb.st_ino) {
	    debug(D_REDIR, "%s: Do Open\n", sda->filename1);
	    tf->tf_cx = 1;
	    goto do_open;
    	}

    	if ((action & 0x0f) == 2 && sb.st_ino) {
	    debug(D_REDIR, "%s: Do Create 3\n", sda->filename1);
	    tf->tf_cx = 3;
	    goto do_creat;
    	}

    	if ((action & 0x0f) != 0 && sb.st_ino == 0) {
	    debug(D_REDIR, "%s: Do Create 2\n", sda->filename1);
	    tf->tf_cx = 2;
	    goto do_creat;
    	}

	debug(D_REDIR, "%s: File not found\n", sda->filename1);
    	tf->tf_ax = FILE_NOT_FOUND;
    	goto error;
    }
skip:
    debug(D_REDIR, " skipped\n");
    return(0);
error:
    tf->tf_eflags |= PSL_C;
    debug(D_REDIR, " error\n");
    return(1);
okay:
    tf->tf_ax &= 0xFF00;
keep_okay:
    tf->tf_eflags &= ~PSL_C;
    debug(D_REDIR, "okay\n");
    return(1);
}
