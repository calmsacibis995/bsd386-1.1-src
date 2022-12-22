/* stat.c -- for Lattice 4.01 */

#include <exec/types.h>
#include <exec/exec.h>
#include <libraries/dos.h>
#include <libraries/dosextens.h>
#include <proto/exec.h>
#include <proto/dos.h>

#include <sys/types.h>
#include <sys/stat.h>

/* I can't find the defines for DirEntryType or EntryType... */
#define DOSDIR  (2L)
#define DOSFILE (-3L)   /* actually, < 0 */

#ifndef SUCCESS
#define SUCCESS (-1)
#define FAILURE (0)
#endif

extern int stat(char *file,struct stat *buf);

stat(file,buf)
char *file;
struct stat *buf;
{

        struct FileInfoBlock *inf;
        struct FileLock *lock;
        long ftime;

        if( (lock = (struct FileLock *)Lock(file,SHARED_LOCK))==0 )
                /* file not found */
                return(-1);

        if( !(inf = (struct FileInfoBlock *)AllocMem(
                (long)sizeof(struct FileInfoBlock),MEMF_PUBLIC|MEMF_CLEAR)) )
        {
                UnLock((BPTR)lock);
                return(-1);
        }

        if( Examine((BPTR)lock,inf)==FAILURE )
        {
                FreeMem((char *)inf,(long)sizeof(*inf));
                UnLock((BPTR)lock);
                return(-1);
        }

        /* fill in buf */

        buf->st_dev                =
        buf->st_nlink        =
        buf->st_uid                =
        buf->st_gid                =
        buf->st_rdev        = 0;
        
        buf->st_ino                = inf->fib_DiskKey;
        buf->st_blocks        = inf->fib_NumBlocks;
        buf->st_size        = inf->fib_Size;
        buf->st_blksize        = 512;

        /* now the date.  AmigaDOG has weird datestamps---
         *      ds_Days is the number of days since 1-1-1978;
         *      however, as Unix wants date since 1-1-1970...
         */

        ftime =
                (inf->fib_Date.ds_Days * 86400 )                +
                (inf->fib_Date.ds_Minute * 60 )                 +
                (inf->fib_Date.ds_Tick / TICKS_PER_SECOND )     +
                (86400 * 8 * 365 )                              +
                (86400 * 2 );  /* two leap years, I think */

/*  ftime += timezone;  */

        buf->st_ctime =
        buf->st_atime =
        buf->st_mtime =
        buf->st_mtime = ftime;

        switch( inf->fib_DirEntryType )
        {
        case DOSDIR:
                buf->st_mode = S_IFDIR;
                break;

        case DOSFILE:
                buf->st_mode = S_IFREG;
                break;

        default:
                buf->st_mode = S_IFDIR | S_IFREG;
                /* an impossible combination?? */
        }

        /* lastly, throw in the protection bits */

        if((inf->fib_Protection & FIBF_READ) == 0)
                buf->st_mode |= S_IREAD;

        if((inf->fib_Protection & FIBF_WRITE) == 0)
                buf->st_mode |= S_IWRITE;

        if((inf->fib_Protection & FIBF_EXECUTE) == 0)
                buf->st_mode |= S_IEXECUTE;

        if((inf->fib_Protection & FIBF_DELETE) == 0)
                buf->st_mode |= S_IDELETE;

        if((inf->fib_Protection & (long)FIBF_ARCHIVE))
                buf->st_mode |= S_IARCHIVE;

        if((inf->fib_Protection & (long)FIBF_PURE))
                buf->st_mode |= S_IPURE;

        if((inf->fib_Protection & (long)FIBF_SCRIPT))
                buf->st_mode |= S_ISCRIPT;

        FreeMem((char *)inf, (long)sizeof(*inf));
        UnLock((BPTR)lock);

        return(0);

}
