/******************************************************************************

VERSION $Header: /bsdi/MASTER/BSDI_OS/lib/libc/db/mpool/smbm/buf.h,v 1.2 1993/02/05 21:53:38 polk Exp $
PACKAGE: User Level Transaction Manager

DESCRIPTION:  Buffer Manager include file

******************************************************************************/
#define	BUF_REGION_NAME		"buf.shared"
#define	BUF_SPIN_NAME		"buf.spin"
#define	BUF_REGION_NUM		0x052392
#define	NUM_BUFS		4000
#define	BUFSIZE	4096
#define	BUFSHIFT		12
#define	NUMTABLE_ENTRIES	512

#define	BUFFERS_SIZE		(BUFSIZE * NUM_BUFS)
#define	BUFHDR_SIZE		(sizeof(BUFHDR_T)*NUM_BUFS)
#define	BUFTABLE_SIZE		(sizeof(u_int)*NUMTABLE_ENTRIES)

#define	BUF_HASH(f,p)		((f + p)  & (NUMTABLE_ENTRIES-1))

/*
	The file table consists of an array of indices into the
	string table, followed by a single integer pointing to
	the next available free space in the table followed by
	the string table
*/
#define	NUM_FILE_ENTRIES	16
#define	FILE_NAME_LEN		256
#define	FILE_TABLE_SIZE		\
	(NUM_FILE_ENTRIES*(sizeof(FINFO_T)+FILE_NAME_LEN)+sizeof(int))
/*
	The region contains (in order):
		The Buffers
		The Buffer Headers
		The Hash Table
		The Free List Pointer
		The Semaphore Key which protects the segment
		The File table
		The File string space
*/
#define	BUF_REGION_SIZE	\
	(BUFFERS_SIZE+BUFHDR_SIZE+BUFTABLE_SIZE+2*sizeof(int)+FILE_TABLE_SIZE)

#define	ADDR_OK(A) \
	(!(A & (BUFSIZE-1)) && (((BUF_T *)A) >= buf_table) && (((BUFHDR_T *)A) < bufhdr_table))

typedef	char	BUF_T[BUFSIZE];

typedef struct _bufhdr {
    OBJ_T	id;		/* Identifies object in buffer */
    int		wait_proc;	/* Process' index sleeping on this event */
    LINKS	lru;		/* Used to link in lru list */
    LINKS	hash;		/* Hash Chain */
    u_long	flags;		/* Flag Array */
#define	BUF_DIRTY	0x1
#define	BUF_VALID	0x2
#define	BUF_PINNED	0x4
#define	BUF_IO_ERROR	0x8
#define	BUF_IO_IN_PROGRESS	0x10
#define	BUF_NEWPAGE	0x20
    unsigned	refcount;	/* Number of pins */
} BUFHDR_T;

/* Shared memory file information */

typedef struct _finfo {
    int	npages;		/* Number of pages in the file (-1) if unitialized */
    int	offset;		/* Offset of string information */
} FINFO_T;

#define MAKE_MRU(BP)	{				\
	if ( *buf_lru == (BP-bufhdr_table) ) {		\
	    *buf_lru = BP->lru.next;			\
	} else if ( !((BP-bufhdr_table) == MRU ) ) {	\
	    int	__mru;					\
	    __mru = MRU;				\
	    LISTP_REMOVE(bufhdr_table, lru, BP)		\
	    LISTPE_INSERT(bufhdr_table, lru, BP, __mru);\
	}						\
}

#define INIT_BUF(BP) {					\
	BP->wait_proc = -1;				\
	LISTP_INIT(bufhdr_table, hash, BP);		\
	BP->flags = 0;					\
	BP->refcount = 0;				\
	MAKE_MRU(BP);					\
}



/*
	Externally visible stuff
*/

/* get_page call flags */
#define BF_DIRTY	BUF_DIRTY
#define BF_EMPTY	0x20
#define BF_PIN		BUF_PINNED
