/******************************************************************************

VERSION $Id: sem.c,v 1.2 1993/02/05 21:53:38 polk Exp $
PACKAGE: 	User Level Semaphore Manager

DESCRIPTION: 	

ROUTINES: 
    External
	create_sem
	get_sem
	release_sem
	destroy_sems
    Internal

******************************************************************************/
/* INCLUDES */
#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <assert.h>
#include "semkeys.h"
#include <sys/ipc.h>
#include <sys/sem.h>

#ifndef SEMMAP
#define SEMMAP  10
#endif
#ifndef SEMMNI
#define SEMMNI  10
#endif
#ifndef SEMMNS
#define SEMMNS  60
#endif
#ifndef SEMMNU
#define SEMMNU  30
#endif
#ifndef SEMMSL
#define SEMMSL  25
#endif
#ifndef SEMOPM
#define SEMOPM  10
#endif
#ifndef SEMUME
#define SEMUME  10
#endif
#ifndef SEMVMX
#define SEMVMX  32767
#endif
#ifndef SEMAEM
#define SEMAEM  16384
#endif

/* Array of semaphore ID's and key values */

static	int	n_sems = 0;
static	key_t	next_key = 1;
static	int	sem_array[SEMMAP];

/*
	< 0 on error
	>= 0 (semaphore ID) on success
*/
extern int
create_sem ( sem_name, sem_num )
char	*sem_name;
long	sem_num;		/* key_t */
{
    int		ndx;
    int		sem_id;

    /* Get an ID for the semaphore */
    if ( (n_sems % SEMMSL) == 0 ) {
	/* Allocate another array of semaphores */

	ndx = n_sems / SEMMSL;
	if ( (sem_array[ndx] = semget(next_key, SEMMSL, IPC_CREAT|0666) ) < 0) {
	    error_log3 ( "SVcreate_sem:  Unable to create semaphore %s (%d).  Errno: %d\n",
			    sem_name, next_key, errno );
	    return(-1);
	}
	next_key++;
    }
    sem_id = n_sems++;

#ifdef SEM_DEBUG
    get_semval ( "create", sem_id );
    printf ( "Semaphore: %s ID: %d\n", sem_name, sem_id );
#endif
    return(sem_id);

}
/*
*/
extern	int
get_sem ( sem_key )
int	sem_key;	/* Returned from Create */
{
    struct	sembuf	sbuf;
    int		ndx;
    int		offset;

#ifdef SEM_DEBUG
    get_semval ( "get", sem_key );
#endif
    ndx = sem_key / SEMMSL;
    offset = sem_key % SEMMSL;
    sbuf.sem_num = offset;
    sbuf.sem_op = -1;
    sbuf.sem_flg = 0;
#ifdef SPRITE
    if ( semop(sem_array[ndx], &sbuf, 1) < 0 ) {
#else
    if ( semop(sem_array[ndx], &sbuf, 1) ) {
#endif
	error_log1 ( "SVget_sem: Error %d\n", errno );
	return(-1);
    }
#ifdef SEM_DEBUG
    get_semval ( "get", sem_key );
#endif
    return(0);
}

extern int
release_sem ( sem_key )
int	sem_key;
{
    struct	sembuf	sbuf;
    int		ndx;
    int		offset;

#ifdef SEM_DEBUG
    get_semval ( "release", sem_key );
#endif
    ndx = sem_key / SEMMSL;
    offset = sem_key % SEMMSL;
    sbuf.sem_num = offset;
    sbuf.sem_op = 1;
    sbuf.sem_flg = 0;
#ifdef SPRITE
    if ( semop(sem_array[ndx], &sbuf, 1) < 0 ) {
#else
    if ( semop(sem_array[ndx], &sbuf, 1) ) {
#endif
	error_log1 ( "SVrelease_sem: Error %d\n", errno );
	return(-1);
    }
#ifdef SEM_DEBUG
    get_semval ( "release", sem_key );
#endif
    return(0);
}

extern int
destroy_sems()
{
    int	i;
    int	nids;

    nids = ( (n_sems-1) / SEMMSL ) + 1;
    for ( i=0; i < nids; i++ ) {
	if ( semctl (sem_array[i], 0, IPC_RMID, 0 ) ) {
	    error_log1 ( "SVdestroy_sem: Unable to remove semaphore %d\n", 
				errno );
	    return;
	}
    }
    next_key = 1;
    n_sems = 0;
    return;
}
#ifdef SEM_DEBUG
get_semval ( op, semid )
char	*op;
int	semid;
{
	int	val;
	int	ndx;
	int	offset;

	ndx = semid/SEMMSL;
	offset = semid % SEMMSL;
	val = semctl ( sem_array[ndx], offset, GETVAL, NULL );
	printf ( "%s\tSEM: %d VAL: %d\n", op, semid, val );
	assert (val<=1);
	return;
}

#endif
