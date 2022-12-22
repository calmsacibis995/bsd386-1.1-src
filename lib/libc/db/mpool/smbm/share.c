/******************************************************************************

VERSION $Id: share.c,v 1.2 1993/02/05 21:53:39 polk Exp $
PACKAGE: 	User Level Shared Memory Manager

DESCRIPTION: 	
	This package provides shared memory to user level processes
	It provides both BSD (mmap/munmap) and systemV (shmget/shmop)
	interfaces.

ROUTINES: 
    External
	share_init
	attach_region
	detach_region

    Internal

******************************************************************************/
/* INCLUDES */
#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
/*#include <limits.h>*/
#include <errno.h>
#include "list.h"
#include "txn_sys.h"

#ifdef SYSTEM_FIVE
#define	SHARED_MODE	(SHM_R|SHM_W|SM_RND)

/* Use System Five shared memory */

#include <sys/ipc.h>
#include <sys/shm.h>

extern void
share_init()
{
	return;
}

extern ADDR_T
attach_region ( region_name, region_num, size, refp )
char	*region_name;		/* File system name associated with segment */
long	region_num;		/* Numerical identifier of segment */
int	size;			/* Size of the shared segment */
int	*refp;			/* OUTPUT: ref count of segment */
{
    int		segsize;
    int		sh_id;
    long	addr;
    struct	shmid_ds	ipcbuf;

    /* Get an ID for the region_name region */
    segsize = getpagesize();
    segsize = (size + (segsize - 1)) & ~(segsize-1);
    if ( (sh_id = shmget (region_num, segsize, IPC_CREAT|SHARED_MODE) ) < 0 ) {
	error_log3 ( "SVattach_region:  Unable to get region %s (%d).  Errno: %d\n",
			region_name, region_num, errno );
	return(NULL);
    }

    /* Now attach the region */
    if ( (addr = ((long)shmat ( sh_id, 0, SHARED_MODE ))) == -1 ) {
	error_log3 ( "SVattach_region:  Unable to attach region %s (%d).  Errno: %d\n",
			region_name, region_num, errno );
	return(NULL);
    }
    if ( shmctl (sh_id, IPC_STAT, &ipcbuf ) < 0 ) {
	error_log3 ( "SVattach_region:  Unable to stat region %s (%d).  Errno: %d\n",
		    region_name, region_num, errno );
	*refp = -1;
    } else {
	*refp = (int)ipcbuf.sm_count;
    }
    return((ADDR_T)addr);

}
/*
*/
extern	int
detach_region ( addr, id, len, refp )
int	addr;
int	id;
int	len;
int	*refp;
{
    int		remove = 0;
    int		segsize;
    int		sh_id;
    struct	shmid_ds	ipcbuf;

    *refp = 0;
    segsize = getpagesize();
    segsize = (len + (segsize - 1)) & ~(segsize-1);
    if ( (sh_id = shmget (id, segsize, IPC_CREAT|SHARED_MODE) ) < 0 ) {
	error_log3 ( "SVdetach_region:  Unable to get region %d.  Errno: %d\n",
			id, errno );
    } else {
	if ( shmctl (sh_id, IPC_STAT, &ipcbuf ) < 0 ) {
	    error_log2 ( "SVdetach_region:  Unable to stat region %d.  Errno: %d\n",
			id, errno );
	} else if ( ipcbuf.sm_count <= 1 ) {
		remove = 1;
	}
	*refp = ( ipcbuf.sm_count ? ipcbuf.sm_count - 1 : 0 );
    }

    if ( shmdt((char *)addr) != 0 ) {
	error_log2 ( "SVdetach_region:  Unable to detach region %X.  Errno: %d\n",
			addr, errno );
    }
    if ( remove ) {
	if ( shmctl (sh_id, IPC_RMID, &ipcbuf ) < 0 ) {
	    error_log3 ( "SVdetach_region:  Unable to remove region %d.  Errno: %d\n",
			id, errno );
	}
    }
    return(0);
}

#else
#include <sys/stat.h>
#include <sys/mman.h>
/* Assume BSD semantics */
#define	SHARED_MODE	0600
/* This is defined somewhere, but I can't find it */
#define MAX_REGIONS	8


typedef struct _addr_fd {
	int	addr;
	int	fd;
	char	*name;
} ADDR_FD;
static ADDR_FD addr_fd_array[MAX_REGIONS];

extern void
share_init()
{
    int	i;

    for ( i = 0; i < MAX_REGIONS; i++ ) {
	addr_fd_array[i].addr = -1;
    }
    return;
}
/*
    Attach a region of shared memory

    Returns valid address on Success and NULL on failure.  On NULL
    errno should be set.
*/

extern ADDR_T
attach_region ( region_name, region_num, size, refp )
char	*region_name;		/* File system name associated with segment */
int	region_num;
int	size;			/* Size of the shared segment */
int	*refp;			/* OUTPUT: Reference count */
{
    int	addr;
    int	fd;
    int i;
    int init;
    int map_size;
    int pagesize;
    int	prot;
    int	real_size;
    int	*ref_countp;
    struct stat sbuf;

    init = 0;
    if ( stat(region_name, &sbuf) ) {
	if ( errno == ENOENT ) {
		errno = 0;
		init = 1;
	}
    }
    if (!(fd = open ( region_name, O_CREAT|O_RDWR,  SHARED_MODE ))) {
	return ( (ADDR_T) NULL );
    }

/* Might be able to unlink fd */

    real_size = (size + sizeof(int) + (sizeof(int)-1)) & ~(sizeof(int)-1);
    prot = PROT_READ | PROT_WRITE;

    pagesize = getpagesize();
    map_size = (real_size + (pagesize - 1)) & ~(pagesize-1);
    if ( !(addr = (int)sbrk(0)) ) {
	error_log1 ( "BSDattach_region: Sbrk failed with %d\n", errno );
	return ( NULL );
    }
    addr = (addr + (pagesize - 1)) & ~(pagesize-1); 
#ifdef SPRITE
    if ( (addr = mmap(addr, map_size, prot, MAP_SHARED, fd, 0)) < 0 ) { 
#else
    if ( mmap ( addr, map_size, prot, MAP_SHARED, fd, 0  ) ) { 
#endif
	error_log2 ( "BSDattach_region: Map of region %s failed with %d\n", 
		      region_name, errno );
	close (fd);
	return ( (ADDR_T) NULL );
    }
    for (i=0; i< MAX_REGIONS; i++) {
	if ( addr_fd_array[i].addr == -1 ) {
		addr_fd_array[i].addr = (int)addr;
		addr_fd_array[i].fd = (int)fd;
		addr_fd_array[i].name = region_name;
		break;
	}
    }
    if ( i == MAX_REGIONS ) {
	error_log0 ( "BSDattach_region: No more room in addr_fd array\n" );
    }

    ref_countp = (int *)((((unsigned) addr) + size + (sizeof(int)-1)) & 
				~(sizeof(int)-1));
    if ( init ) {
	*ref_countp = 0;
    }
    *ref_countp = *ref_countp + 1;
    *refp = *ref_countp;
    return ( (ADDR_T) addr );
}

/*
    Returns 0 on success
    and -1 on failure setting errno according to munmap conditions
*/
extern	int
detach_region ( addr, id, len, refp )
int	addr;
int	id;
int	len;
int	*refp;
{
    int	i;
    int	map_size;
    int	pagesize;
    int	real_size;
    int	*ref_countp;

    ref_countp = (int *)((((unsigned)addr) + len + (sizeof(int)-1)) & 
				~(sizeof(int)-1));
    *ref_countp = *ref_countp-1;
    *refp = *ref_countp;
    pagesize = getpagesize();
    real_size = (len + sizeof(int) + (sizeof(int)-1)) & ~(sizeof(int)-1);
    map_size = (real_size + (pagesize - 1)) & ~(pagesize-1);
    if ( munmap ( addr, map_size ) ) {
	error_log2 ( "BSDdetach_region:  Unable to detach region %X.  Errno: %d\n",
			addr, errno );
    }
    for ( i=0; i<MAX_REGIONS; i++ ) {
	if ( addr_fd_array[i].addr == (int)addr ) {
	    addr_fd_array[i].addr = -1;
	    if (!*refp) {
		unlink(addr_fd_array[i].name);
	    }
	    close(addr_fd_array[i].fd);
	}
    }
    return(0);
}

#endif
