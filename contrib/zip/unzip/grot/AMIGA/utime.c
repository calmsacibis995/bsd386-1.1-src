/* utime.c */

#include <string.h>
#include <time.h>
#include <errno.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <libraries/dos.h>
#include <libraries/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>

extern LONG sendpkt(struct MsgPort *,LONG,LONG[],LONG);

extern int _OSERR;

#ifndef SUCCESS
#define SUCCESS (-1L)
#define FAILURE 0L
#endif

int utime(char *file, time_t timep[]);

int utime(file,timep)
char *file;
time_t timep[];
{

    struct DateStamp date;
    struct MsgPort *taskport;
    struct FileLock *dirlock, *lock;
    struct FileInfoBlock *fib;

    LONG argv[4];
    UBYTE *ptr;
    long ret;

/*  timep[1] -= timezone;   */

    date.ds_Days = timep[1] / 86400;
    date.ds_Minute = (timep[1] - (date.ds_Days * 86400))/60;
    date.ds_Tick = ( timep[1] - (date.ds_Days * 86400) -
                                (date.ds_Minute * 60)
                   ) * TICKS_PER_SECOND;
    date.ds_Days -= ((8*365+2));

    if( !(taskport = (struct MsgPort *)DeviceProc(file)) )
    {
        errno = ESRCH;          /* no such process */
        _OSERR = IoErr();
        return(-1);
    }

    if( !(lock = (struct FileLock *)Lock(file,SHARED_LOCK)) )
    {
        errno = ENOENT;         /* no such file */
        _OSERR = IoErr();
        return(-1);
    }

    if( !(fib = (struct FileInfoBlock *)AllocMem(
        (long)sizeof(struct FileInfoBlock),MEMF_PUBLIC|MEMF_CLEAR)) )
    {
        errno = ENOMEM;         /* insufficient memory */
        UnLock((BPTR)lock);
        return(-1);
    }

    if( Examine((BPTR)lock,fib)==FAILURE )
    {
        errno = EOSERR;         /* operating system error */
        _OSERR = IoErr();
        UnLock((BPTR)lock);
        FreeMem((char *)fib,(long)sizeof(*fib));
        return(-1);
    }

    dirlock = (struct FileLock *)ParentDir((BPTR)lock);
    ptr = (UBYTE *)AllocMem(64L,MEMF_PUBLIC);
    strcpy((ptr+1),fib->fib_FileName);
    *ptr = strlen(fib->fib_FileName);
    FreeMem((char *)fib,(long)sizeof(*fib));
    UnLock((BPTR)lock);

    /* now fill in argument array */

    argv[0] = NULL;
    argv[1] = (LONG)dirlock;
    argv[2] = (LONG)&ptr[0] >> 2;
    argv[3] = (LONG)&date;

    errno = ret = sendpkt(taskport,34L,argv,4L);

    FreeMem(ptr,64L);
    UnLock((BPTR)dirlock);

    return(0);

} /* utime() */
/*  sendpkt.c
 *  by A. Finkel, P. Lindsay, C. Sheppner
 *  returns Res1 of the reply packet
 */
/*
#include <exec/types.h>
#include <exec/memory.h>
#include <libraries/dos.h>
#include <libraries/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>
*/

LONG sendpkt(pid,action,args,nargs)
struct MsgPort *pid;            /* process identifier (handler message port) */
LONG action,                    /* packet type (desired action)              */
     *args,                     /* a pointer to argument list                */
     nargs;                     /* number of arguments in list               */
{

    struct MsgPort *replyport;
    struct StandardPacket *packet;
    LONG count, *pargs, res1;

    replyport = (struct MsgPort *)CreatePort(0L,0L);
    if( !replyport ) return(NULL);

    packet = (struct StandardPacket *)AllocMem(
            (long)sizeof(struct StandardPacket),MEMF_PUBLIC|MEMF_CLEAR);
    if( !packet )
    {
        DeletePort(replyport);
        return(NULL);
    }

    packet->sp_Msg.mn_Node.ln_Name  = (char *)&(packet->sp_Pkt);
    packet->sp_Pkt.dp_Link          = &(packet->sp_Msg);
    packet->sp_Pkt.dp_Port          = replyport;
    packet->sp_Pkt.dp_Type          = action;

    /* copy the args into the packet */
    pargs = &(packet->sp_Pkt.dp_Arg1);      /* address of 1st argument */
    for( count=0; count<nargs; count++ )
        pargs[count] = args[count];

    PutMsg(pid,(struct Message *)packet);   /* send packet */

    WaitPort(replyport);
    GetMsg(replyport);

    res1 = packet->sp_Pkt.dp_Res1;

    FreeMem((char *)packet,(long)sizeof(*packet));
    DeletePort(replyport);

    return(res1);

} /* sendpkt() */
