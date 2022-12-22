/*
 * INT 13 emulation.
 */

/*
 * Alexander Kolbasov.                   Started Wed Mar 11 1992.
 */

#if !defined(lint)
static char rcsid[] = "$Header: /bsdi/MASTER/BSDI_OS/usr.bin/rundos/kernel/int13.c,v 1.2 1992/09/02 23:57:11 polk Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#if defined(BSD)
#  include <sys/disklabel.h>
#endif

#include "v86types.h"
#include "int13.h"
#include "int13_l.h"

#include <sys/stat.h>

static void put_debug(int level, const char *fmt, ...);

/*
 * Function init_disk_service() opens all pseudo-disks.
 * It is called by the BIOS signal during BIOS start-up.
 */

void init_disk_service(void)
{
  static int already_done = 0;  /* This function should be called only once */
  
  if (! already_done)
    {
      already_done++;

      /* Initialize global variables */
      connection_area = (connection_block_t)MK_32_ADDR(*CONNECT_ADDR);

      fd_parm_table =
	(fd_parameter_table_t *)
	  MK_32_ADDR(connection_area->addr_ADDR_fl_PARAMS);

      if ((disk_array[0].d_device = search_device(DEV_FLOP, 1)) != NULL)
	init_disk(&disk_array[0]);
      if ((disk_array[1].d_device = search_device(DEV_FLOP, 2)) != NULL)
	init_disk(&disk_array[1]);
      if ((disk_array[2].d_device = search_device(DEV_HARD, 1)) != NULL)
	init_disk(&disk_array[2]);
      if ((disk_array[3].d_device = search_device(DEV_HARD, 2)) != NULL)
	init_disk(&disk_array[3]);
    }
}

/*
 * Function init_disk() prepares particular pseudo-disk.
 * It actually opens regular files and fills internal disk structures.
 * For special files (floppies) it only fills structures and doesn't open
 * anything. The open() will be issued later by reset_controller() function.
 */

static void init_disk(dinfo_t dsk)
{
  const char *name = dsk->d_name;
  struct stat statinfo;

  dsk->d_type = NO_DRIVE;  /* I'll define the type later */

  /* First check, whether file is regular or special */
 
  if ((stat(name, &statinfo)) < 0)
    put_debug(1, "Can't stat file %s, errno = %d\n", name, errno);
  else if (! S_ISREG(statinfo.st_mode) && ! S_ISCHR(statinfo.st_mode))
    put_debug(1, "\tFile %s has invalid type.\n", name);
  else if (access(name, R_OK) != 0)
    put_debug(1, "\tCan't read file %s\n", name);
  else  /* Here file exist and has read access */
    {
      dsk->is_regular = S_ISREG(statinfo.st_mode);

      /* Check whether file is writable */

      if (access(name, W_OK) == 0)
	dsk->d_mode = O_RDWR;
      else dsk->d_mode = O_RDONLY;

      /* Open file only in case it is regular */

      if (dsk->is_regular && open_with_lock(dsk) < 0)
	put_debug(1, "\tCannot open disk %s, mode = %d, errno = %d\n", 
		  name, dsk->d_mode, errno);
      else if (dsk->d_device->dev_type == DEV_HARD)
	fill_hard_disk_parameters(dsk);
      else if (dsk->d_device->dev_type == DEV_FLOP)
	fill_floppy_disk_parameters(dsk);
      else put_debug(1, "\tInvalid device id %d\n", dsk->d_device->dev_type);
    }
}

static void fill_hard_disk_parameters(dinfo_t dsk)
{
  const int devid = dsk->d_device->dev_id;  /* CMOS HD type */

  if (devid < 1 || devid > MAX_HDISK_ID)
    put_debug(1, "\tBad hard disk type - %d\n", devid);
  else
    {
      hd_parameter_table_t dt =
       	&(((hd_parameter_table_t)
	   MK_32_ADDR(connection_area->_hdisk_table)) [devid - 1]);
       
      /* cmos_disk_type_t dt = &cmos_disk_table[devid - 1]; */
      
      number_of_hards++;
      dsk->d_ncyl   = dt->hd_ncyl - 1; /* ??? */
      dsk->d_nsect  = dt->hd_nsect;
      dsk->d_nhead  = number_of_heads(dt->hd_nhead);
      dsk->d_type   = DRIVE_HD;
      dsk->d_szsect = SECTOR_SIZE;
      dsk->d_nsect_cyl = dsk->d_nsect * dsk->d_nhead;
      dsk->d_size = dsk->d_nsect_cyl * dsk->d_ncyl * dsk->d_szsect;
      put_debug(9, "\tdisk type %d: %d cyl, %d heads, %d sect\n",
		devid, dsk->d_ncyl, dsk->d_nhead, dsk->d_nsect);
    }
}
	  
static void fill_floppy_disk_parameters(dinfo_t dsk)
{
  int devtype = dsk->d_device->dev_id / 360;  /* Index in floppy table */
  floppy_type_t ft;

  number_of_floppies++;
  if (devtype >= MAX_FDISK_ID || devtype < 0)
    devtype = 0;
  ft = & floppy_parameter_table [devtype];
  
  dsk->d_ncyl   = ft->fd_ncyl;
  dsk->d_nsect  = ft->fd_nsect;
  dsk->d_nhead  = ft->fd_nhead;
  dsk->d_type   = ft->fd_id;
  dsk->d_szsect = SECTOR_SIZE;
  dsk->d_nsect_cyl = dsk->d_nsect * dsk->d_nhead;
  dsk->d_size = dsk->d_nsect_cyl * dsk->d_ncyl * dsk->d_szsect;
  put_debug(9, "\tfloppy disk: %d cyl, %d heads, %d sect\n",
	    dsk->d_ncyl, dsk->d_nhead, dsk->d_nsect);
}

static int open_with_lock(dinfo_t dsk)
{
  int rc = 1;
  const char *name = dsk->d_name;

  put_debug(2,"\tCall13: open_with_lock(%s)\n", name);
  if ((rc = dsk->d_file = open(name, dsk->d_mode)) < 0)
      put_debug(2, "\tCan't open file %s\n", name);

/* MS-DOS doesn't have fcntl() and VPIX already locked everything */

#if !defined(__MSDOS__) && !defined(VPIX) && defined(USE_FLOCK)
  else if (dsk->d_mode == O_RDWR)  /* We don't need to lock RONLY files */
    {
      struct flock lck;

      /* Write-lock whole file */

      lck.l_type = F_WRLCK; lck.l_whence = SEEK_SET;
      lck.l_start = lck.l_len = 0;
      if (fcntl(dsk->d_file, F_SETLK, &lck) < 0)
	{
	  put_debug(3,"\tFile %s already locked\n", name);
	  dsk->d_mode = O_RDONLY;
	}
      else put_debug(3,"\tFile %s locked successfully\n", name);
    }
#endif        /* __MSDOS__ && VPIX && USE_FLOCK */
  return rc;
}

/* 
 * General INT13 wrapper. It calls v86_int13 with correct address of
 * register images
 */

int v86_do_int13(void)
{
  return v86_int13((V86_REGPACK_t)MK_32_ADDR(connection_area->addr_link_fi13));
}

/*
 * This is the main function that performs INT 13H work
 */

int v86_int13(V86_REGPACK_t regs)
{
  int rc;

  put_debug(1,"Call13: v86_int13(AX = %x, BX = %x, CX = %x, DX = %x, ES = %x)\n",
	    get_AX(regs), get_BX(regs), get_CX(regs), get_DX(regs),
	    get_ES(regs));
  
  clear_CF(get_v86_flags(regs));

  switch(get_subfunction(regs))
    {
    case INT13_RESET_CONTROLLER:                        /* 00 */
      rc = reset_controller(regs);
      break;
    case INT13_GET_STATUS:                              /* 01 */
      rc = int13_get_status(regs);
      break;
    case INT13_READ_SECTORS:                            /* 02 */
      rc = do_sectors_IO(regs, DO_READ);
      break;
    case INT13_WRITE_SECTORS:                           /* 03 */
      rc = do_sectors_IO(regs, DO_WRITE);
      break;
    case INT13_VERIFY_SECTORS:                          /* 04 */
      rc = int13_dummy(regs, "Verify Sectors");
      break;
    case INT13_GET_DRIVE_PARMS:                         /* 08 */
      rc = get_drive_parms(regs);
      break;
    case INT13_SEEK_CYLINDER:                           /* 0C */
      rc = int13_dummy(regs, "Seek Cylinder");
      break;
    case INT13_ALTERNATE_RESET:                         /* 0D */
      rc = int13_dummy(regs, "Alternate Disk Reset");
      break;
    case INT13_TEST_DRIVE_READY:                        /* 10 */
      rc = int13_dummy(regs, "Test Drive Ready");
      break;
    case INT13_RECALIBRATE:                             /* 11 */
      rc = int13_dummy(regs, "Recalibrate");
      break;
    case INT13_CONTRLR_RAM_DIAG:                        /* 12 */
      rc = int13_dummy(regs, "Controller RAM Diagnostics");
      break;
    case INT13_DRIVE_DIAG:                              /* 13 */
      rc = int13_dummy(regs, "Drive Diagnostics");
      break;
    case INT13_CONTRLR_DIAG:                            /* 14 */
      rc = int13_dummy(regs, "Controller Diagnostics");
      break;
    case INT13_GET_DISK_TYPE:                           /* 15 */
      rc = int13_get_type(regs);
      break;
    case INT13_DISK_CHANGE_STAT:                        /* 16 */
      rc = get_change_status(regs);
      break;
    case INT13_SET_MEDIA_FORMAT:                        /* 18 */
      rc = set_media_for_format(regs);
      break;
    case INT13_PARK_HEADS:                              /* 19 */
      rc = int13_dummy(regs, "Park Heads");
      break;
    default:
      rc = unimplemented(regs);                         /* OOPS! */
      break;
    }
  put_debug(1,"\tReturn: v86_int13() = %d, CF = %d\n", rc,
	    get_CF(get_v86_flags(regs)));
  return rc;
}

/*
 * Function unimplemented() is called for all currently unimplemented
 * int 13H subfunction. There should be no calls for this function
 * during emulation.
 */

static int unimplemented(V86_REGPACK_t regs)
{
  put_debug(1,"Call13:!!unimplemented function %#x called\n",
	    get_subfunction(regs));
  put_debug(1,"\tReturn: unimp(AX = %x, BX = %x, CX = %x, DX = %x, ES = %x)\n",
	    get_AX(regs), get_BX(regs), get_CX(regs), get_DX(regs),
	    get_ES(regs));
  return int13_error(regs, INT13_ERR_BAD_COMMAND);
}  

/*
 * Function int13_ok() clears Carry flag and AH.
 *
 * Modifies:  regs->r_flags.fl_cf is set to 0
 *            regs->r_ax.br.high is set to 0
 */

static int int13_ok(V86_REGPACK_t regs)
{
  clear_CF(get_v86_flags(regs));
  regs->r_ax.br.high = INT13_ERR_NONE;
  put_debug(2,"\tint13_ok(AX = %x, BX = %x, CX = %x, DX = %x, ES = %x,\
 DI = %x)\n",
	    get_AX(regs), get_BX(regs), get_CX(regs), get_DX(regs),
	    get_ES(regs), get_DI(regs));
  return (INT13_ERR_NONE);
}  

/*
 * Function int13_io_ok() clears error status in BIOS area,
 * clears Carry flag and AH.
 *
 * Modifies: regs->r_flags.fl_cf is set to 0
 *           regs->r_ax.br.high is set to 0
 */

static int int13_io_ok(V86_REGPACK_t regs, int disk_no)
{
  if (disk_no < INT13_HARD) *BIOS_FLOP_OP_STAT_ADDR = INT13_ERR_NONE;
  else *BIOS_HARD_OP_STAT_ADDR = INT13_ERR_NONE;
  return int13_ok(regs);
}  

/*
 * Function int13_error() is called to set AH and CF when error
 * encountered.
 *
 * Modifies: regs->r_flags.fl_cf is set to 1;
 *           regs->r_ax.br.high (AH) is set to err_code.
 */

static int int13_error(V86_REGPACK_t regs, int err_code)
{
  put_debug(4,"\tCall13: int13_error(err_code = %x)\n", err_code);

  /* On errors set carry flag to 1 and remember the status of operation */
  raise_CF(get_v86_flags(regs));
  return (regs->r_ax.br.high = err_code);
}  

/*
 * Function int13_io_error() sets err_code in BIOS error codes area and
 * than call int13_error.
 *
 * Modifies: regs->r_flags.fl_cf is set to 1;
 *           regs->r_ax.br.high (AH) is set to err_code.
 *           Error code is set in BIOS area depending on disk_no.
 *
 */

static int int13_io_error(V86_REGPACK_t regs, int disk_no, int err_code)
{
  put_debug(5,"\tCall13: int13_io_error(err = %x, disk = %x)\n", 
	    err_code, disk_no);
  if (disk_no < INT13_HARD)
    *BIOS_FLOP_OP_STAT_ADDR = err_code;
  else *BIOS_HARD_OP_STAT_ADDR = err_code;
  return int13_error(regs, err_code);
}  

static int int13_dummy(V86_REGPACK_t regs, const char *name)
{
  put_debug(4,"\tCall13: int13_dummy(%s)\n", name);
  return int13_ok(regs);
}

/*-------------------------------------------------------------------
 * RESET CONTROLLER: (SubFn 0)
 *
 * Input: DL = drive (if bit 7 is set both hard disks
 *                      and floppy disks reset)
 *
 * According to interrupts list it returns nothing.
 * Here I clear CF;
 *-------------------------------------------------------------------
 */

static int reset_controller(V86_REGPACK_t regs)
{
  int rc;
  const int dno = get_disk_number(regs);
  const dinfo_t dsk = &disk_array[get_dno(dno)];
  
  put_debug(2,"\tCall13: reset_controller(%d)\n", dno);
  if (! valid_dno(dno) || dsk->is_regular)
    rc = int13_ok(regs);
  else 
    {
      if (disk_is_opened(dsk->d_file))
	{
	  (void)close(dsk->d_file);
	  dsk->d_file = -1;
	}
      dsk->media_changed = NO_M_CHANGE;
      if (open_with_lock(dsk) < 0 ||
	  read(dsk->d_file, first_sector_cache.cache_buf, SECTOR_SIZE) !=
	     SECTOR_SIZE ||
	  fill_dsk_parameters(dsk) < 0)
	rc = int13_error(regs, INT13_ERR_NOT_READY);
      else
	{
	  put_debug(7, "\tDisk reopened\n");
	  first_sector_cache.drive_no = dno;
	  first_sector_cache.cache_is_active = CACHE_ACTIVE;
	  rc = int13_ok(regs);
	}
    }
  return rc;
}    
 
/*-------------------------------------------------------------------------
 * GET_DISK_STATUS (SubFunction 01h)
 *
 * Output: AH = status of last disk operation.
 *-------------------------------------------------------------------------
 */

static int int13_get_status(V86_REGPACK_t regs)
{
  int rc;
  const int dno = get_disk_number(regs);
  caddr_t err_addr = dno < INT13_HARD ? BIOS_FLOP_OP_STAT_ADDR:
                                        BIOS_HARD_OP_STAT_ADDR;

  put_debug(4,"\tCall13: int13_get_status(disk = %x)\n", dno);

  if (! valid_dno(dno))
    rc = int13_error(regs, INT13_ERR_BAD_COMMAND);
  else if (*err_addr == INT13_ERR_NONE)
    rc = int13_ok(regs);
  else rc = int13_error(regs, *err_addr);

  *err_addr = INT13_ERR_NONE;           /* Clear error status */
  return rc;
}


/*-------------------------------------------------------------------
 * READ/WRITE_SECTORS: (SubFns 02h, 03h)
 *
 * Input: DL = drive number (0 = drive A...; 80H = hard disk 0...)
 *        DH = read/write head number
 *        CH = low 8 bits of cylinder number (0-n)
 *        CL = high 2 bits of cyl. number and 6-bit sector number
 *        AL = sector count (no more than 1 cyl. worth of sectors)
 *        ES:BX = buffer address
 *
 * Output: Carry Flag = 1 if error occured
 *         AH = disk error code
 *         AL = number of sectors read
 *         ES:BX buffer contains data read from disk.
 *-------------------------------------------------------------------
 */
       
static int do_sectors_IO(V86_REGPACK_t regs, int read_or_write)
{
  int rc = INT13_ERR_NONE;
  const int dno = get_disk_number(regs);
  const dinfo_t dsk = &disk_array[get_dno(dno)];
  const int count  = get_AL(regs);

  put_debug(3,"\tCall13: do_sectors_IO(%s)\n", read_or_write ? "Write" : "Read");

  /* Clear AL. It will be set again when success in do_io_at() */
  regs->r_ax.br.low = 0;   

  if (! valid_dno(dno) || dsk->d_type == NO_DRIVE)
    rc = int13_io_error(regs, dno, INT13_ERR_BAD_COMMAND);
  else if (read_or_write == DO_WRITE && dsk->d_mode == O_RDONLY)
    rc = int13_io_error(regs, dno, INT13_ERR_WRITE_PROTECT);
  else if (! disk_is_opened(dsk->d_file))
    rc = int13_io_error(regs, dno, INT13_ERR_NOT_READY);
  else          /* Everything is ok */
    {
      const int head   = get_DH(regs);
      const int sector = get_sector(get_CX(regs));
      const int cyl    = get_cylinder(get_CX(regs));
      
      put_debug(3,
		"\tdisk = %d, head = %d, cyl = %d, sector = %d, count = %d\n",
		   dno,       head,      cyl,      sector,      count);


      /* Check parameters for conformity with disk type */
      if (! correct_disk_parameters(dsk, head, cyl, sector))
	rc = int13_io_error(regs, dno, INT13_ERR_SEEK);
      else
	{
	  /* `where' is the absolute sector number */
	  long where = cyl  * dsk->d_nsect_cyl + head * dsk->d_nsect + sector;

	  /* address is ES:BX */
	  const caddr_t address = MK_FP(get_ES(regs), get_BX(regs));

	  if ((rc = do_io_at(regs, dsk, where, address, count, read_or_write))
	      == INT13_ERR_NONE)
	    rc = int13_io_ok(regs, dno);
	  else  rc = int13_io_error(regs, dno, rc);
	}
    }
  put_debug(3,"\tReturn: do_sectors_IO() = %d\n", rc);
  return rc;
}


static int do_io_at(V86_REGPACK_t regs, const dinfo_t dsk, long where,
		    caddr_t address, int count, int read_or_write)
{
  int rc = INT13_ERR_NONE;
  const int size_sect = dsk->d_szsect;
  const int fd = dsk->d_file;
  size_t how_many = count * size_sect;  /* Number of bytes to I/O */
  int n = 0;                            /* Number of bytes actually I/O'ed */

  put_debug(5,"\tCall13: do_io_at(%s)\n", read_or_write ? "Write":"Read");
  put_debug(5,"\twhere = %ld, address = %x, count = %d fd = %d\n",
	         where,       address,      count,     fd);

  /*
   * Sectors are started from 1 so (where - 1) address the beginning of
   * the file.
   */

  where--;

  if (where + how_many > dsk->d_size) /* Attempt to i/o too many sectors */
    how_many = dsk->d_size - where;

  if (dsk->media_changed == M_IS_CHANGED)
    rc = INT13_ERR_CLL_ACTIVE;
  else if ((lseek(fd, where * size_sect, SEEK_SET)) == -1)
    rc = INT13_ERR_SEEK;
  else if (read_or_write == DO_READ)
    {
      if (first_sector_cache.cache_is_active == CACHE_ACTIVE &&
	  where == 1 && count == 1 &&
	  first_sector_cache.drive_no == get_disk_number(regs))
	{
	  put_debug(6, "\tCopying first sector from cache\n");
	  (void)memcpy(address, first_sector_cache.cache_buf, SECTOR_SIZE);
	  n = 1;  /* Pretend to read one sector */
	}
      else if ((n = read(fd, address, how_many)) == how_many)
	rc = INT13_ERR_NONE;
      else if (! dsk->is_regular /* || errno != 0 */ )
	rc = INT13_ERR_SECTOR_ID_BAD;
      else
	{
	  /*
	   * When we try to read past the end of the actual file and
	   * this is regular file we pretend to read all zeroes.
	   */
	  (void)bzero(address + n, how_many - n);
	  put_debug(6,"\tRead %d sectors. Fill with zeroes %d sectors\n",
		    n, how_many - n);
	  n = how_many;
	  rc = INT13_ERR_NONE;
	}
    }
  else if ((n = write(fd, address, how_many)) == how_many)
    rc = INT13_ERR_NONE;
  else rc = INT13_ERR_SECTOR_ID_BAD;

  first_sector_cache.cache_is_active = CACHE_DIRTY;  /* Flush cache */
  n /= SECTOR_SIZE;               /* Number of sectors read/written */
  regs->r_ax.br.low = n < 0 ? 0 : n;

  put_debug(5,"\tReturn: do_io_at() = %#x, AL = %d\n", rc, get_AL(regs));
  return rc;
}

/*ARGSUSED to make lint happy */

static int fill_dsk_parameters(dinfo_t dsk)
{
  struct disklabel dl;
  int rc = 1;

  put_debug(8, "\tCall13: fill_disk_parameters()\n"); 

  /* Get media type */
  if (ioctl(dsk->d_file, DIOCGDINFO, &dl) < 0)
    rc = -1;
  else
    {
      dsk->d_ncyl      = dl.d_ncylinders;
      dsk->d_nsect     = dl.d_nsectors;
      dsk->d_nhead     = dl.d_ntracks;
      dsk->d_szsect    = dl.d_secsize;
      dsk->d_nsect_cyl = dsk->d_nsect * dsk->d_nhead;
      dsk->d_size      = dsk->d_nsect_cyl * dsk->d_ncyl * dsk->d_szsect;

#if defined(DEBUG) && (DEBUG == 9)
      {
	int devtype = dsk->d_size / (360 * 1024);  /* Index in floppy table */
	if (devtype >= MAX_FDISK_ID || devtype < 0)
	  put_debug(9,"\tInvalid devtype %d\n", devtype);
	else
	  {
	    floppy_type_t ft = & floppy_parameter_table [devtype];
	    if (dsk->d_ncyl  != ft->fd_ncyl ||
		dsk->d_nsect != ft->fd_nsect ||
		dsk->d_nhead != ft->fd_nhead)
	      put_debug(9, "\tInvalid device parameters\n");
	  }
      }
#endif
    }
  put_debug(8, "\tReturn: fill_disk_parameters() = %d\n", rc);
  return rc;
}


/*-------------------------------------------------------------------------
 * GET_DRIVE_PARAMETERS: (SubFunction 08h)
 *
 * Input:  DL = drive
 *       
 * Output: Carry Flag = 1 on error
 *         AH = status code
 *         BL = drive type 
 *         DL = nymber of consecutive ascknowledging drives
 *         DH = maximum value for heads
 *         CL = maximum value for sectors (and max cyl high bits)
 *         CH = maximum value for cylinders (lower 8 bits)
 *           ES:DI -> drive parameter table (Only for floppy disks)
 *-------------------------------------------------------------------------
 */

static int get_drive_parms(V86_REGPACK_t regs)
{
  int rc = INT13_ERR_NONE;
  const int dno = get_disk_number(regs);
  const dinfo_t dsk = &disk_array[get_dno(dno)];

  put_debug(3,"\tCall13: int13_get_drive_parms(disk = %x)\n", dno);
  
  if (! valid_dno(dno) || dsk->d_type == NO_DRIVE)
    rc = INT13_ERR_BAD_COMMAND;
  else
    {
      const int maxcyl = dsk->d_ncyl - 1;
      /*
       * CX structure for get parameters:  
       *
       * 111111
       * 5432109876543210
       * ccccccccCcSsssss
       */
      regs->r_dx.br.low  = dno < INT13_HARD ? number_of_floppies:
                                                number_of_hards; 
      regs->r_dx.br.high = dsk->d_nhead - 1;
      regs->r_cx.br.high = maxcyl & 0xFF;
      regs->r_cx.br.low  = ((maxcyl & 0x300) >> 2) | (dsk->d_nsect & 0x3F);

      /* Set drive type */
      if (dno < INT13_HARD)
	{
	  fd_parameter_table_t fdt_addr =
	    *(fd_parm_table + ((int)dsk->d_type - 1));

	  regs->r_es.wr.x = GET_SEG(fdt_addr);
	  regs->r_di.wr.x = GET_OFS(fdt_addr);
	  regs->r_bx.br.low = (int)dsk->d_type;  /* Set CMOS disk type */
	}
    }

  put_debug(3,"\tReturn: int13_get_drive_parms() = %d\n", rc);
  return rc == INT13_ERR_NONE ? int13_io_ok(regs, dno) : 
                                int13_io_error(regs, dno, rc);
}


/*-------------------------------------------------------------------------
 * GET_DISK_TYPE (SubFunction 015h)
 *
 * Input:  DL = drive
 * Output: Carry Flag set on error
 *
 *       AH = drive code:         - Not status code!
 *
 *          INT13_15_NOT_PRESENT:  Drive DL not present
 *          INT13_15_FD_NO_CLL:    Diskette; CLL not present
 *          INT13_15_FD_CLL:       Diskette; CLL available
 *	    INT13_15_HD:           Hard disk
 *            CX:DX = number of 512-byte sectors
 *-------------------------------------------------------------------------
 */

static int int13_get_type(V86_REGPACK_t regs)
{
  const int dno = get_disk_number(regs);
  const dinfo_t dsk = &disk_array[get_dno(dno)];

  put_debug(3,"\tCall13: int13_get_type(disk = %x)\n", dno);

  clear_CF(get_v86_flags(regs));

  if (! valid_dno(dno) || dsk->d_type == NO_DRIVE)
    raise_CF(get_v86_flags(regs));
  else if (dno < INT13_HARD)
    regs->r_ax.br.high = INT13_15_FD_CLL;
  else                                        /* This is hard disk */ 
    {
      long number_of_sectors = dsk->d_nsect_cyl * dsk->d_ncyl;
      
      regs->r_ax.br.high = INT13_15_HD;
      regs->r_cx.wr.x = (u_WORD)(number_of_sectors >> V86_WORD_SIZE);
      regs->r_dx.wr.x = (u_WORD)number_of_sectors;
    }
  put_debug(3,"\tReturn:int13_get_type(AX = %X, CX = %X, DX = %X, CF = %d)\n",
	    get_AX(regs), get_CX(regs), get_DX(regs), regs->r_flags.fl_cf);
  return INT13_ERR_NONE;  /* No int13_ok() !! */
}

/*-------------------------------------------------------------------------
 * GET_DISK_TYPE (SubFunction 016h)
 *
 * Input:  DL = drive to check
 * Output: AH = disk change status - Not status code!
 *          00h = no disk change
 *          06h = disk changed
 *
 *  CF never change its value here!
 *-------------------------------------------------------------------------
 * Now I return disk changed for floppies.
 */

static int get_change_status(V86_REGPACK_t regs)
{
  int rc = INT13_ERR_CLL_ACTIVE;
  const int dno = get_disk_number(regs);
  const dinfo_t dsk = &disk_array[get_dno(dno)];

  put_debug(2,"\tCall13: get_change_status(disk = %d)\n", dno);

  if ((dno >= INT13_HARD) || ! valid_dno(dno) ||
        ! disk_is_opened(dsk->d_file) || dsk->is_regular)
    rc = INT13_ERR_NONE;

  dsk->media_changed = NO_M_CHANGE;

  return rc == INT13_ERR_NONE ? int13_io_ok(regs, dno) :
                       	        int13_io_error(regs, dno, rc);
}

/*-------------------------------------------------------------------------
 * SET MEDIA TYPE FOR FORMAT (SubFunction 018h)
 *
 * Input:  DL = drive number
 *         CH = lower 8 bits of number of tracks
 *         CL = sectors per track (bits 0-5)
 *                top 2 bits of number of tracks (bits 6,7)
 * Output: AH = 00h requested command supported
 *              01h function not available
 *              0Ch not supported or drive type unknown
 *              80h there is no disk in the drive.
 *         ES:DI -> 11 byte parameter table.
 *
 *  CF never change its value here!
 *-------------------------------------------------------------------------
 * Now I return 'function not available'.
 */

static int set_media_for_format(V86_REGPACK_t regs)
{
  regs->r_ax.br.high = 0x01;
  return INT13_ERR_NONE;
}

/* Debugging interface */

#if defined(DEBUG)
extern FILE *logfile;
#include <stdarg.h>
#endif

static void put_debug(int level, const char *fmt, ...)
{
#if defined(DEBUG)
  va_list args;
  va_start(args, fmt);
  if (logfile != NULL && level <= DEBUG)
    {
      (void)vfprintf(logfile, fmt, args);
      (void)fflush(logfile);
    }
  va_end(args);
#endif
}

/* That's all, folks! */
