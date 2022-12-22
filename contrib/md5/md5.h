/* @(#)md5.h	6.1 1/10/94 22:52:43 */
/*
 * md5 - RSA Data Security, Inc. MD5 Message-Digest Algorithm
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
 * See md5drvr.c for version and modification history.
 */

/*
 ***********************************************************************
 ** Copyright (C) 1990, RSA Data Security, Inc. All rights reserved.  **
 **								      **
 ** License to copy and use this software is granted provided that    **
 ** it is identified as the "RSA Data Security, Inc. MD5 Message-     **
 ** Digest Algorithm" in all material mentioning or referencing this  **
 ** software or this function.					      **
 **								      **
 ** License is also granted to make and use derivative works	      **
 ** provided that such works are identified as "derived from the RSA  **
 ** Data Security, Inc. MD5 Message-Digest Algorithm" in all          **
 ** material mentioning or referencing the derived work.	      **
 **								      **
 ** RSA Data Security, Inc. makes no representations concerning       **
 ** either the merchantability of this software or the suitability    **
 ** of this software for any particular purpose.  It is provided "as  **
 ** is" without express or implied warranty of any kind.              **
 **								      **
 ** These notices must be retained in any copies of any part of this  **
 ** documentation and/or software.				      **
 ***********************************************************************
 */

#if !defined(MD5_H)
#define MD5_H

#include <sys/types.h>
#include <sys/stat.h>

extern void *malloc();	/* storage allocator */

/*
 * determine if we are checked in
 */
#define MD5_H_WHAT "@(#)"		/* will be @(#) or @(#) */

/*
 * Useful defines/typedefs
 */
typedef unsigned char BYTE;
typedef unsigned long ULONG;
typedef unsigned int UINT;

/*
 * The MD5 block size and message digest sizes, in bytes
 */
/* MD5_BLOCKSIZE must be a power of 2 */
#define MD5_BLOCKSIZE	64
/* READSIZE must be a multiple of MD5_BLOCKSIZE */
#define READSIZE (512*MD5_BLOCKSIZE)

/* 
 * Data structure for MD5 (Message-Digest) computation 
 */
typedef struct {
    ULONG countLo, countHi;	/* number of _bits_ handled mod 2^64 */
    ULONG datalen;	/* length of data in inp.inp_BYTE */
    ULONG buf[4];	/* scratch buffer */
    union {
	BYTE inp_BYTE[64];	/* BYTE input buffer */
	ULONG inp_ULONG[16];	/* ULONG input buffer */
    } inp;
    BYTE digest[16];	/* actual digest after MD5Final call */
} MD5_CTX;
#define in_byte inp.inp_BYTE
#define in_ulong inp.inp_ULONG

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

/* md5.c */
void MD5Init P((MD5_CTX*));
void MD5Update P((MD5_CTX*, BYTE*, UINT));
void MD5fullUpdate P((MD5_CTX*, BYTE*, UINT));
void MD5Final P((MD5_CTX*));
char *MD5_what;

/* md5dual.c */
void dualMain P((int, char**, BYTE*, UINT, char*));
void dualTest P((void));
char *MD5dual_what;

/* md5drvr.c */
void MD5Print P((MD5_CTX*));
ULONG zero[];
char *program;
int i_flag;
int q_flag;
int dot_zero;

#endif /* MD5_H */
