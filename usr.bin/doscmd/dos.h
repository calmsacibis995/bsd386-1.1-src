/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	Krystal $Id: dos.h,v 1.1 1994/01/14 23:26:27 polk Exp $ */
/*
 * DOS Error codes
 */
/* MS-DOS version 2 error codes */
#define FUNC_NUM_IVALID		0x01
#define FILE_NOT_FOUND		0x02
#define PATH_NOT_FOUND		0x03
#define TOO_MANY_OPEN_FILES	0x04
#define ACCESS_DENIED		0x05
#define HANDLE_INVALID		0x06
#define MEM_CB_DEST		0x07
#define INSUF_MEM		0x08
#define MEM_BLK_ADDR_IVALID	0x09
#define ENV_INVALID		0x0a
#define FORMAT_INVALID		0x0b
#define ACCESS_CODE_INVALID	0x0c
#define DATA_INVALID		0x0d
#define UNKNOWN_UNIT		0x0e
#define DISK_DRIVE_INVALID	0x0f
#define ATT_REM_CUR_DIR		0x10
#define NOT_SAME_DEV		0x11
#define NO_MORE_FILES		0x12
/* mappings to critical-error codes */
#define WRITE_PROT_DISK		0x13
#define UNKNOWN_UNIT_CERR	0x14
#define DRIVE_NOT_READY		0x15
#define UNKNOWN_COMMAND		0x16
#define DATA_ERROR_CRC		0x17
#define BAD_REQ_STRUCT_LEN	0x18
#define SEEK_ERROR		0x19
#define UNKNOWN_MEDIA_TYPE	0x1a
#define SECTOR_NOT_FOUND	0x1b
#define PRINTER_OUT_OF_PAPER	0x1c
#define WRITE_FAULT		0x1d
#define READ_FAULT		0x1e
#define GENERAL_FAILURE		0x1f

/* MS-DOS version 3 and later extended error codes */
#define SHARING_VIOLATION	0x20
#define FILE_LOCK_VIOLATION	0x21
#define DISK_CHANGE_INVALID	0x22
#define FCB_UNAVAILABLE		0x23
#define SHARING_BUF_EXCEEDED	0x24

#define NETWORK_NAME_NOT_FOUND	0x35

#define FILE_ALREADY_EXISTS	0x50

#define DUPLICATE_REDIR		0x55

/*
 * dos attribute byte flags
 */
#define REGULAR_FILE    0x00    
#define READ_ONLY_FILE  0x01    
#define HIDDEN_FILE     0x02    
#define SYSTEM_FILE     0x04    
#define VOLUME_LABEL    0x08    
#define DIRECTORY       0x10    
#define ARCHIVE_NEEDED  0x20    

/*
 * Internal structure used for get_space()
 */
typedef struct {
    long	bytes_sector;
    long	sectors_cluster;
    long	total_clusters;
    long	avail_clusters;
} fsstat_t;

/*
 * Several DOS structures used by the file redirector
 */

/*
 * This is really the format of the DTA.  The file redirector will only
 * use the first 21 bytes.
 */
typedef struct {
    u_char      flag;
    u_char      reserved[6];
    u_char      attr;
    u_char      pattern[13];
    u_char      attribute;
    u_short     time;
    u_short     date;
    u_char      size[4];
    u_char      name[13];
} find_block_t;

/*
 * DOS directory entry structure
 */
typedef struct {
    u_char      name[8];
    u_char      ext[3];
    u_char      attr;
    u_char      reserved[10];
    u_short     time;
    u_short     date;
    u_short     start;
    u_long      size;
} dosdir_t;

/*
 * The Current Drive Structure
 */
typedef struct {
	u_char	path[0x43];
	u_char	flag_lo;
	u_char	flag;
	u_char	dpb_off[2];
	u_char	dpb_seg[2];
	u_char	redirector_off[2];
	u_char	redirector_seg[2];
	u_char	paramter_int21[2];
        u_char	offset[2];
	u_char	dummy;
	u_char	ifs_driver[4];
	u_char	dummy2[2];
} CDS;

#define	CDS_network	0x80
#define	CDS_physical	0x40
#define	CDS_joined	0x20
#define	CDS_substed	0x10

/* 
 * The List of Lists (used to get the CDS and a few other numbers)
 */
typedef struct {
	u_char	dummy1[0x16];
	u_short	cds_offset;
	u_short	cds_seg;
	u_char  dummy2[6];
	u_char	numberbdev;
	u_char	lastdrive;
} LOL;

/*
 * The System File Table
 */
typedef struct {
/*00*/	u_short	nfiles;		/* Number file handles referring to this file */
/*02*/	u_short	open_mode;	/* Open mode (bit 15 -> by FCB) */
/*04*/	u_char	attribute;
/*05*/	u_char	info[2];	/* 15 -> remote, 14 ->  dont set date */
/*07*/	u_char	ddr_dpb[4];	/* Device Driver Header/Drive Paramter Block */
/*0b*/	u_char	fd[2];
/*0d*/	u_char	time[2];
/*0f*/	u_char	date[2];
/*11*/	u_char	size[4];
/*15*/	u_char	offset[4];
/*19*/	u_char	rel_cluster[2];
/*1b*/	u_char	abs_cluster[2];
/*1d*/	u_char	dir_sector[2];
/*1f*/	u_char	dir_entry;
/*20*/	u_char	name[8];
/*28*/	u_char	ext[3];
/*2b*/	u_char	sharesft[4];
/*2f*/	u_char	sharenet[2];
/*31*/	u_char	psp[2];
/*33*/	u_char	share_off[2];
/*35*/	u_char	local_end[2];
/*37*/	u_char	ifd_driver[4];
} SFT;

/*
 * Format of PCDOS 4.01 swappable data area
 * (Sorry, but you need a wide screen to make this look nice)
 */
typedef struct {
    u_char	err_crit;	/*   00h    BYTE    critical error flag */
    u_char	InDOS;		/*   01h    BYTE    InDOS flag (count of active INT 21 calls) */
    u_char	err_drive;	/*   02h    BYTE    ??? drive number or FFh */
    u_char	err_locus;	/*   03h    BYTE    locus of last error */
    u_short	err_code;	/*   04h    WORD    extended error code of last error */
    u_char	err_suggest;	/*   06h    BYTE    suggested action for last error */
    u_char	err_class;	/*   07h    BYTE    class of last error */
    u_short	err_di;
    u_short	err_es;		/*   08h    DWORD   ES:DI pointer for last error */
    u_short	dta_off;
    u_short	dta_seg;	/*   0Ch    DWORD   current DTA */
    u_short	psp;		/*   10h    WORD    current PSP */
    u_short	int_23_sp;	/*   12h    WORD    stores SP across an INT 23 */
    u_short	wait_status;	/*   14h    WORD    return code from last process termination (zerod after reading with AH=4Dh) */
    u_char	current_drive;	/*   16h    BYTE    current drive */
    u_char	break_flag;	/*   17h    BYTE    extended break flag */
    u_char	unknown1[2];	/*   18h  2 BYTEs   ??? */
    u_short	int_21_ax;	/*   1Ah    WORD    value of AX on call to INT 21 */
    u_short	net_psp;	/*   1Ch    WORD    PSP segment for sharing/network */
    u_short	net_number;	/*   1Eh    WORD    network machine number for sharing/network (0000h = us) */
    u_short	first_mem;	/*   20h    WORD    first usable memory block found when allocating memory */
    u_short	best_mem;	/*   22h    WORD    best usable memory block found when allocating memory */
    u_short	last_mem;	/*   24h    WORD    last usable memory block found when allocating memory */
    u_char	unknown[10];	/*   26h  2 BYTEs   ??? (don't seem to be referenced) */
    u_char	monthday;	/*   30h    BYTE    day of month */
    u_char	month;		/*   31h    BYTE    month */
    u_short	year;		/*   32h    WORD    year - 1980 */
    u_short	days;		/*   34h    WORD    number of days since 1-1-1980 */
    u_char	weekday;	/*   36h    BYTE    day of week (0 = Sunday) */
    u_char	unknown2[3];	/*   37h    BYTE    ??? */
    u_char	ddr_head[30];	/*   38h 30 BYTEs   device driver request header */
    u_short	ddre_ip;
    u_short	ddre_cs;	/*   58h    DWORD   pointer to device driver entry point (used in calling driver) */
    u_char	ddr_head2[22];	/*   5Ch 22 BYTEs   device driver request header */
    u_char	ddr_head3[30];	/*   72h 30 BYTEs   device driver request header */
    u_char	unknown3[6];	/*   90h  6 BYTEs   ??? */
    u_char	clock_xfer[6];	/*   96h  6 BYTEs   CLOCK$ transfer record (see AH=52h) */
    u_char	unknown4[2];	/*   9Ch  2 BYTEs   ??? */
    u_char	filename1[128];	/*   9Eh 128 BYTEs  buffer for filename */
    u_char	filename2[128];	/*  11Eh 128 BYTEs  buffer for filename */
    u_char	findfirst[21];	/*  19Eh 21 BYTEs   findfirst/findnext search data block (see AH=4Eh) */
    u_char	foundentry[32];	/*  1B3h 32 BYTEs   directory entry for found file */
    u_char	cds[88];	/*  1D3h 88 BYTEs   copy of current directory structure for drive being accessed */
    u_char	fcbname[11];	/*  22Bh 11 BYTEs   ??? FCB-format filename */
    u_char	unknown5;	/*  236h    BYTE    ??? */
    u_char	wildcard[11];	/*  237h 11 BYTEs   wildcard destination specification for rename (FCB format) */
    u_char	unknown6[11];	/*  242h  2 BYTEs   ??? */
    u_char	attrmask;	/*  24Dh    BYTE    attribute mask for directory search??? */
    u_char	open_mode;	/*  24Eh    BYTE    open mode */
    u_char	unknown7[3];	/*  24fh    BYTE    ??? */
    u_char	virtual_dos;	/*  252h    BYTE    flag indicating how DOS function was invoked (00h = direct INT 20/INT 21, FFh = server call AX=5D00h) */
    u_char	unknown8[9];	/*  253h    BYTE    ??? */
    u_char	term_type;	/*  25Ch    BYTE    type of process termination (00h-03h) */
    u_char	unknown9[3];	/*  25Dh    BYTE    ??? */
    u_short	dpb_off;
    u_short	dpb_seg;	/*  260h    DWORD   pointer to Drive Parameter Block for critical error invocation */
    u_short	int21_sf_off;
    u_short	int21_sf_seg;	/*  264h    DWORD   pointer to stack frame containing user registers on INT 21 */
    u_short	store_sp;	/*  268h    WORD    stores SP??? */
    u_short	dosdpb_off;
    u_short	dosdpb_seg;	/*  26Ah    DWORD   pointer to DOS Drive Parameter Block for ??? */
    u_short	disk_buf_seg;	/*  26Eh    WORD    segment of disk buffer */
    u_short	unknown10[4];	/*  270h    WORD    ??? */
    u_char	media_id;	/*  278h    BYTE    Media ID byte returned by AH=1Bh,1Ch */
    u_char	unknown11;	/*  279h    BYTE    ??? (doesn't seem to be referenced) */
    u_short	unknown12[2];	/*  27Ah    DWORD   pointer to ??? */
    u_short	sft_off;
    u_short	sft_seg;	/*  27Eh    DWORD   pointer to current SFT */
    u_short	cds_off;
    u_short	cds_seg;	/*  282h    DWORD   pointer to current directory structure for drive being accessed */
    u_short	fcb_off;
    u_short	fcb_seg;	/*  286h    DWORD   pointer to caller's FCB */
    u_short	unknown13[2];	/*  28Ah    WORD    ??? */
    u_short	jft_off;
    u_short	jft_seg;	/*  28Eh    DWORD   pointer to a JFT entry in process handle table (see AH=26h) */
    u_short	filename1_off;	/*  292h    WORD    offset in DOS CS of first filename argument */
    u_short	filename2_off;	/*  294h    WORD    offset in DOS CS of second filename argument */
    u_short	unknown14[12];	/*  296h    WORD    ??? */
    u_short	file_offset_lo;
    u_short	file_offset_hi;	/*  2AEh    DWORD   offset in file??? */
    u_short	unknown15;	/*  2B2h    WORD    ??? */
    u_short	partial_bytes;	/*  2B4h    WORD    bytes in partial sector */
    u_short	number_sectors;	/*  2B6h    WORD    number of sectors */
    u_short	unknown16[3];	/*  2B8h    WORD    ??? */
    u_short	nbytes_lo;
    u_short	nbytes_hi;	/*  2BEh    DWORD   number of bytes appended to file */
    u_short	qpdb_off;
    u_short	qpdb_seg;	/*  2C2h    DWORD   pointer to ??? disk buffer */
    u_short	asft_off;
    u_short	asft_seg;	/*  2C6h    DWORD   pointer to ??? SFT */
    u_short	int21_bx;	/*  2CAh    WORD    used by INT 21 dispatcher to store caller's BX */
    u_short	int21_ds;	/*  2CCh    WORD    used by INT 21 dispatcher to store caller's DS */
    u_short	temporary;	/*  2CEh    WORD    temporary storage while saving/restoring caller's registers */
    u_short	prevcall_off;
    u_short	prevcall_seg;	/*  2D0h    DWORD   pointer to prev call frame (offset 264h) if INT 21 reentered also switched to for duration of INT 24 */
    u_char	unknown17[9];	/*  2D4h    WORD    ??? */
    u_char	ext_action[2];	/*  2DDh    WORD    multipurpose open action */
    u_char	ext_attr[2];	/*  2DFh    WORD    multipurpose attribute */
    u_char	ext_mode[2];	/*  2E1h    WORD    multipurpose mode */
    u_char	unknown17a[9];
    u_short	lol_ds;		/*  2ECh    WORD    stores DS during call to [List-of-Lists + 37h] */
    u_char	unknown18[5];	/*  2EEh    WORD    ??? */
    u_char	usernameptr[4];	/*  2F3h    DWORD   pointer to user-supplied filename */
    u_char	unknown19[4];	/*  2F7h    DWORD   pointer to ??? */
    u_char	lol_ss[2];	/*  2FBh    WORD    stores SS during call to [List-of-Lists + 37h] */
    u_char	lol_sp[2];	/*  2FDh    WORD    stores SP during call to [List-of-Lists + 37h] */
    u_char	lol_flag;	/*  2FFh    BYTE    flag, nonzero if stack switched in calling [List-of-Lists+37h] */
    u_char	searchdata[21];	/*  300h 21 BYTEs   FindFirst search data for source file(s) of a rename operation (see AH=4Eh) */
    u_char	renameentry[32];/*  315h 32 BYTEs   directory entry for file being renamed */
    u_char	errstack[331];	/*  335h 331 BYTEs  critical error stack */
    u_char	diskstack[384];	/*  480h 384 BYTEs  disk stack (functions greater than 0Ch, INT 25, INT 26) */
    u_char	iostack[384];	/*  600h 384 BYTEs  character I/O stack (functions 01h through 0Ch) */
    u_char	int_21_08_flag;	/*  780h    BYTE    flag affecting AH=08h (see AH=64h) */
    u_char	unknown20[11];	/*  781h    BYTE    ??? looks like a drive number */
} SDA;
