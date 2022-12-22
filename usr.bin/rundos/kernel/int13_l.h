/*
 * INT 13 emulation. Internal macros.
 *
 * This file should be included by int13.c only. It is not public interface.
 */

/*
 * Alexander Kolbasov.                   Started Wed Mar 11 1992.
 *
 * $Header: /bsdi/MASTER/BSDI_OS/usr.bin/rundos/kernel/int13_l.h,v 1.2 1992/09/02 23:58:17 polk Exp $
 */

#pragma once
#if !defined(__INT13_L_H_)
#define __INT13_L_H_

#include "int13.h"
#include "kernel.h"

#define DEBUG_FILE_NAME "./int13.dbg"

/* #define CONNECT_ADDR ((connection_block_t)MK_FP(0xF800,0)) */

#define CONNECT_ADDR ((u_DWORD *)(0xff * 4))

#define INT13_MAGIC_PORT           0x8000
#define SERVICE_MAGIC_PORT         0x8004
#define SERVICE_INIT_CONNECTION    0
#define SERVICE_PRINT_REG          1
#define SERVICE_ENABLE_TRACE_PORT  2
#define SERVICE_DISABLE_TRACE_PORT 3

#define SECTOR_SIZE 512

static connection_block_t connection_area = NULL;
static fd_parameter_table_t *fd_parm_table;

/*
 * This is special cache for first sector. It is used when the floppy
 * type is determined. After this procedure the first sector is already
 * read and thus shouldn't be read again.
 */

typedef enum { CACHE_DIRTY, CACHE_ACTIVE } cache_state_t;

struct {
  cache_state_t cache_is_active;
  int  drive_no;                              /* 1 or 0 */
  char cache_buf[SECTOR_SIZE];
} static first_sector_cache;

#define IS_REGULAR 1
#define IS_SPECIAL 0

#define IS_READ_ONLY  1
#define IS_READ_WRITE 0

#define M_IS_CHANGED 1
#define NO_M_CHANGE  0

/* General info about hard disk */

typedef struct device *device_t;

typedef struct d_info
{
  device_t d_device;         /* Pointer to device struct */
#define d_name d_device->dev_real_name
  int   d_file;              /* File for disk emul.  */
  int   d_ncyl;              /* Number of cylinders  */
  int   d_nhead;             /* Number of heads      */
  int   d_nsect;             /* Sectors per track    */
  int   d_szsect;            /* Sector size          */
  int   d_nsect_cyl;         /* Sectors per cylinder */
  long  d_size;              /* Total disk size in bytes */
  drive_type_t d_type;
  int   d_mode;              /* O_RDWR or O_RDONLY   */
  int   is_regular;
  int   media_changed;       /* Meaningful for floppies only */
} *dinfo_t;

static struct d_info disk_array[] = {
  { NULL, -1, 0, 0, 0, 512, 0, 0, NO_DRIVE, O_RDWR, IS_REGULAR, NO_M_CHANGE },
  { NULL, -2, 0, 0, 0, 512, 0, 0, NO_DRIVE, O_RDWR, IS_REGULAR, NO_M_CHANGE },
  { NULL, -3, 0, 0, 0, 512, 0, 0, NO_DRIVE, O_RDWR, IS_REGULAR, NO_M_CHANGE },
  { NULL, -4, 0, 0, 0, 512, 0, 0, NO_DRIVE, O_RDWR, IS_REGULAR, NO_M_CHANGE }
};

#define NUMBER_OF_DISKS (sizeof(disk_array) / sizeof(disk_array[0]))
#define NUMBER_OF_FLOPPIES 2
#define NUMBER_OF_HARDS (NUMBER_OF_DISKS - NUMBER_OF_FLOPPIES)

#define MAX_HDISK_ID 46

typedef struct floppy_type {
  int fd_ncyl;
  int fd_nhead;
  int fd_nsect;
  drive_type_t fd_id;
} *floppy_type_t;
  
static struct floppy_type floppy_parameter_table[] = {
  { 0,  0, 0,  NO_DRIVE   },
  { 40, 2, 9,  DRIVE_360  },
  { 80, 2, 9,  DRIVE_720  },
  { 80, 2, 15, DRIVE_1200 },
  { 80, 2, 18, DRIVE_1440 }
};

#define MAX_FDISK_ID (sizeof(floppy_parameter_table) /\
		      sizeof(floppy_parameter_table[0]))
  
static int number_of_floppies = 0;
static int number_of_hards = 0;

#define DO_READ  0
#define DO_WRITE 1

/*
 * CX structure for disk read:  
 *
 * 111111
 * 5432109876543210
 * ccccccccCcSsssss
 */

#  define get_cylinder(x) \
             (((((u_WORD)(x)) & 0xC0) << 2) | \
              ((((u_WORD)(x)) & 0xFF00) >> 8))  

#define get_sector(x)  (((u_WORD)(x)) & 0x3F)

#define get_subfunction(regs) get_AH(regs)
#define get_disk_number(regs) get_DL(regs)


/*
 * Macro valid_dno checks whether disk number is correct.
 * X000000X is the correct bit representation for disks 0, 1, 0x80, 0x81.
 *  ^^^^^^
 *   0x7E    X means 0 or 1.
 *
 */

#define valid_dno(val) (!((val) & 0x7E))

/*
 * Macro get_dno translates 0->0, 1->1, 0x80->2 0x81->3
 * and obtain correct index in disk_array.
 */
 
#define get_dno(val) \
          ((((u_BYTE)(val) & 0x80) >> 6) | ((u_BYTE)(val) & 1))


#define disk_is_opened(fd) ((fd) >= 0)

/* 
 * correct_disk_parameters() checks different disk parameters for
 * correspondence with disk type.
 */

#define correct_disk_parameters(dsk, head, cyl, sector) \
  ((head) < (dsk)->d_nhead && (cyl) < (dsk)->d_ncyl &&  \
   (sector) <= (dsk)->d_nsect)


/* Functions that are used internally in int13.c */

  static int v86_int13(V86_REGPACK_t regs);
  static int unimplemented(V86_REGPACK_t regs);
  static int do_sectors_IO(V86_REGPACK_t regs, int read_or_write);
  static int int13_error(V86_REGPACK_t regs, int err_code);
  static int int13_io_error(V86_REGPACK_t regs, int disk_no, int err_code);
  static int int13_ok(V86_REGPACK_t regs);
  static int int13_io_ok(V86_REGPACK_t regs, int disk_no);
  static int int13_get_status(V86_REGPACK_t regs);
  static int int13_get_type(V86_REGPACK_t regs);
  static int get_change_status(V86_REGPACK_t regs);
  static int get_drive_parms(V86_REGPACK_t regs);
  static int set_media_for_format(V86_REGPACK_t regs);
  static int reset_controller(V86_REGPACK_t regs);
  static int do_io_at(V86_REGPACK_t regs, const dinfo_t dsk, long where,
		      caddr_t address, int count, int read_or_write);
  static int  int13_dummy(V86_REGPACK_t regs, const char *name);
  static void init_disk(dinfo_t dsk);
  static void fill_hard_disk_parameters(dinfo_t dsk);
  static void fill_floppy_disk_parameters(dinfo_t dsk);
  static int  fill_dsk_parameters(dinfo_t dsk);
  static int  open_with_lock(dinfo_t dsk);

#endif /* __INT13_L_H_ */
