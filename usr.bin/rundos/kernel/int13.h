/*
 * INT 13 emulation.
 */

/*
 * Alexander Kolbasov.                   Started Wed Mar 11 1992.
 *
 * $Header: /bsdi/MASTER/BSDI_OS/usr.bin/rundos/kernel/int13.h,v 1.2 1992/09/02 23:57:53 polk Exp $
 */

#pragma once
#ifndef __INT13_H_
#define __INT13_H_

#include <sys/types.h>

#include "v86types.h"


#define INT13_FUNCTION 0x13
 
/* INT 13 SubFunctions. */
 
#define INT13_RESET_CONTROLLER   0
#define INT13_GET_STATUS         1
#define INT13_READ_SECTORS       2
#define INT13_WRITE_SECTORS      3
#define INT13_VERIFY_SECTORS     4
#define INT13_FORMAT_TRACK       5
#define INT13_GET_DRIVE_PARMS    8
#define INT13_INIT_PARM_TABLES   9
#define INT13_READ_LONG          0x0A
#define INT13_WRITE_LONG         0x0B
#define INT13_SEEK_CYLINDER      0x0C
#define INT13_ALTERNATE_RESET    0x0D
#define INT13_TEST_DRIVE_READY   0x10
#define INT13_RECALIBRATE        0x11
#define INT13_CONTRLR_RAM_DIAG   0x12
#define INT13_DRIVE_DIAG         0x13
#define INT13_CONTRLR_DIAG       0x14
#define INT13_GET_DISK_TYPE      0x15
#define INT13_DISK_CHANGE_STAT   0x16
#define INT13_SET_DISK_TYPE      0x17
#define INT13_SET_MEDIA_FORMAT   0x18
#define INT13_PARK_HEADS         0x19


/*
 * ERROR CODES.
 *
 * Floppy and hard disks have different error code addresses.
 *
 * Hard disk D: possibly has its own error status address.
 * --------------------------------------------------------
 */

#define BIOS_FLOP_OP_STAT_ADDR ((caddr_t) 0x0441)
#define BIOS_HARD_OP_STAT_ADDR ((caddr_t) 0x0474)

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

#define EMUL_UNIMPLEMENTED 1    /* Unimplemented function */


#define INT13_HARD     0x80     /* DL < 80 - floppy, DL >= 80 - hard disks */

/*
 * These are disk parameter table addresses.
 * Disk 0 uses INT 41H, disk 1 uses INT 46H 
 * Diskette base table is at INT 1eH
 */

#define INT13_FD_0_PARAMETER_TABLE_ADDR ((caddr_t)(0x1E * 4))
#define INT13_HD_0_PARAMETER_TABLE_ADDR ((caddr_t)(0x41 * 4))
#define INT13_HD_1_PARAMETER_TABLE_ADDR ((caddr_t)(0x46 * 4))

/* 
 * For Get Disk Type.
 * CLL means Change Line Logic.
 */

#define INT13_15_NOT_PRESENT 0
#define INT13_15_FD_NO_CLL   1
#define INT13_15_FD_CLL      2
#define INT13_15_HD          3

/* For Read Change */

#define INT13_16_NO_CHANGE           0
#define INT13_16_CHANGE_LINE_ACTIVE  6

/* Floppy drive types */

typedef enum {
  NO_DRIVE   = 0,
  DRIVE_360  = 1,
  DRIVE_1200 = 2,
  DRIVE_720  = 3,
  DRIVE_1440 = 4,
  DRIVE_2800 = 5,
  DRIVE_HD   = 100             /* Some large and meaningless value */
  } drive_type_t;

/* Media types */

typedef enum {
  NO_MEDIA   = 0x00,
  MEDIA_360  = 0x10,
  MEDIA_1200 = 0x20,
  MEDIA_720  = 0x30,
  MEDIA_1440 = 0x40,
  MEDIA_2800 = 0x50
  } media_type_t;

/* Diskette types for Set Diskette Type */

#define DISKETTE_NOT_USED      0
#define DISKETTE_360_IN_360    1
#define DISKETTE_360_IN_1200   2
#define DISKETTE_1200_IN_1200  3
#define DISKETTE_720_IN_720    4
#define DISKETTE_720_IN_1440   5
#define DISKETTE_1440_IN_1440  6

#define INT13_FD_USE_DMA 1

/* Sector Sizes */

#define INT13_FD_SECT_128  0
#define INT13_FD_SECT_256  1
#define INT13_FD_SECT_512  2
#define INT13_FD_SECT_1024 3

#pragma pack(1) /* For SYSV cc. Doesn't work with gcc! */
typedef struct fd_parameter_table
{
  struct
    {
      BYTE  step_rate_time : 4,     /* Step rate time - bits 0-3   */
            head_unl_time  : 4;     /* Head unload time - bits 4-7 */
    }  fd_srt_timing;

  struct
    {
      BYTE  dma_usage     : 1,     /* 1 = Use DMA                  */
            head_ld_time  : 7;     /* Head Load Time - bits 2-7    */
    }  fd_dma_head_timing;
  
  BYTE fd_motor_wait;              /* 55-ms increment before turning off */
                                   /*   disk motor                       */
  BYTE fd_sect_sz;                 /* Sector size in bytes        */
                                   /*    0x00 = 128  Bytes/Sector */
                                   /*    0x01 = 256  Bytes/Sector */
                                   /*    0x01 = 512  Bytes/Sector */
                                   /*    0x01 = 1024 Bytes/Sector */
  BYTE fd_eot;                     /* Last sector on a track */
  BYTE fd_rw_gaplen;               /* Gap length for read/write operations */
  BYTE fd_dtl;                     /* Data trasnfer length - max transfer  */
                                   /*   when length not set                */
  BYTE fd_fmt_gaplen;              /* Gap length for format operation      */
  BYTE fd_fmt_fill;                /* Fill character for format            */
  BYTE fd_hd_settle;               /* Head settle time (in milliseconds)   */
  BYTE fd_mt_start;                /* Motor startup time (in 1/8th-seconds */

} *fd_parameter_table_t;
#pragma pack() /* For SYSV cc. Doesn't work with gcc! */

/*
 * This is the actual structure of hd_parameter_table:
 *
 * typedef struct hd_parameter_table
 * {
 *   WORD hd_ncyl;               Maximum number of cylinders 
 *   BYTE hd_nhead;              Maximum number of heads     
 *   WORD hd_wcurr;              Starting reduced-write current cylinder 
 *   WORD hd_wprecmp;            Starting write-precompensation cylinder 
 *   BYTE hd_eccburst;           Maximum ECC data burst length  
 *   BYTE hd_opts;               Drive step options:           
 *                                  bit 7 - disable retries     
 *                                  bit 6 - disable ECC retries 
 *                                  bits 2-0 - drive option     
 * 
 *   BYTE hd_std_timeout;         Standard timeout value       
 *   BYTE hd_fmt_timeout;         Timeout value for format drive
 *   BYTE hd_chk_timeout;         Timeout value for check drive
 *   WORD hd_reserved;
 *   BYTE hd_nsect;               Sectors per track
 *   BYTE hd_reserved1;
 * }
 *
 * To solve the problem with alignment in gcc I redefine it as follows:
 */

typedef struct hd_parameter_table
{
  WORD hd_ncyl;               /* Maximum number of cylinders */

  WORD hd_nhead;              /* Maximum number of heads + wcurr */
  /* And some small hack. */
# define number_of_heads(nhead) ((nhead) & 0xFF)

  WORD hd_wcurr_precomp;      /* Starting reduced-write curr. cylinder */
  WORD hd_wprecmp_eccburst;   /* Starting write-precompensation */
			      /* cylinder */

  /* No alignment problems after this point */

  BYTE hd_opts;               /* Drive step options:           */
                              /*   bit 7 - disable retries     */
                              /*   bit 6 - disable ECC retries */
                              /*   bits 2-0 - drive option     */

  BYTE hd_std_timeout;         /* Standard timeout value         */
  BYTE hd_fmt_timeout;         /* Timeout value for format drive */
  BYTE hd_chk_timeout;         /* Timeout value for check drive  */
  WORD hd_reserved;
  BYTE hd_nsect;               /* Sectors per track */
  BYTE hd_reserved1;
} *hd_parameter_table_t;

/* Global interface functions exported by int3.c */
 
  extern int v86_do_int13(void);         /* Call int13 emulation. */
  extern void init_disk_service(void);   /* Init. all internal structures */
  extern int close_int13(void);          /* Close everything worth closing */

#endif  /* __INT13_H_ */
