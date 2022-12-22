/* @(#)shs.h	5.2 1/8/94 07:34:38 */
/*
 * shs - NIST proposed Secure Hash Standard
 *
 * Written 2 September 1992, Peter C. Gutmann.
 *
 * This file was Modified by:
 *
 *	 Landon Curt Noll  (chongo@toad.com)	chongo <was here> /\../\
 *
 * This code has been placed in the public domain.  Please do not 
 * copyright this code.
 *
 * LANDON CURT NOLL DISCLAIMS ALL WARRANTIES WITH  REGARD  TO
 * THIS  SOFTWARE,  INCLUDING  ALL IMPLIED WARRANTIES OF MER-
 * CHANTABILITY AND FITNESS.  IN NO EVENT SHALL  LANDON  CURT
 * NOLL  BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM  LOSS  OF
 * USE,  DATA  OR  PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR  IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * See shsdrvr.c for version and modification history.
 */

#if !defined(SHS_H)
#define SHS_H

#include <sys/types.h>
#include <sys/stat.h>

extern void *malloc();	/* storage allocator */

/*
 * determine if we are checked in
 */
#define SHS_H_WHAT "@(#)"	/* @(#) if checked in */

/*
 * Useful defines/typedefs
 */
typedef unsigned char BYTE;
typedef unsigned long ULONG;
typedef unsigned long UINT;

/*
 * The SHS block size and message digest sizes, in bytes
 */
/* SHS_BLOCKSIZE must be a power of 2 */
#define SHS_BLOCKSIZE	64
#define SHS_DIGESTSIZE	20
/* READSIZE must be a multiple of SHS_BLOCKSIZE */
#define READSIZE (512*SHS_BLOCKSIZE)

/*
 * The structure for storing SHS info
 */
typedef struct {
    ULONG digest[5];		/* message digest */
    ULONG countLo, countHi;	/* 64-bit bit count */
    ULONG datalen;		/* length of data in data */
    ULONG data[16];		/* SHS data buffer */
} SHS_INFO;

/*
 * elements of the stat structure that we will process
 */
struct hashstat {
    dev_t st_dev;
    ino_t st_ino;
    mode_t st_mode;
    nlink_t st_nlink;
    uid_t st_uid;
    gid_t st_gid;
    off_t st_size;
    time_t st_mtime;
    time_t st_ctime;
};

/*
 * Turn off prototypes if requested
 */
#if (defined(NOPROTO) && defined(PROTO))
# undef PROTO
#endif

/*
 * Used to remove arguments in function prototypes for non-ANSI C
 */
#ifdef PROTO
# define P(a) a
#else	/* !PROTO */
# define P(a) ()
#endif	/* ?PROTO */

/* shs.c */
void shsInit P((SHS_INFO*));
void shsfullUpdate P((SHS_INFO*, BYTE*, UINT));
void shsUpdate P((SHS_INFO*, BYTE*, UINT));
void shsFinal P((SHS_INFO*));
char *shs_what;
int shs_file;

/* shsdual.c */
void dualMain P((int, char**, BYTE*, UINT, char*));
void dualTest P((void));
char *shsdual_what;

/* shsdrvr.c */
void shsPrint P((SHS_INFO*));
ULONG zero[];
char *program;
int i_flag;
int q_flag;
int dot_zero;

#endif /* SHS_H */
