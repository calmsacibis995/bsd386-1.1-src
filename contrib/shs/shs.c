/*	BSDI	$Id: shs.c,v 1.1.1.1 1994/01/13 22:42:33 polk Exp $	*/
/* @(#)shs.c	6.3 1/11/94 01:06:50 */
/*
 * shs - NIST proposed Secure Hash Standard
 *
 * Written 2 September 1992, Peter C. Gutmann.
 *
 * This file was Modified/Re-written by:
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

#include <string.h>
#include "shs.h"
#ifdef __bsdi__
#include <machine/endian.h>
#else
#include "align.h"
#include "endian.h"
#endif

char *shs_what="@(#)";	/* #(@) if checked in */

/* 
 * The SHS f()-functions.  The f1 and f3 functions can be optimized 
 * to save one boolean operation each - thanks to Rich Schroeppel,
 * rcs@cs.arizona.edu for discovering this.
 *
 * f1: ((x&y) | (~x&z)) == (z ^ (x&(y^z)))
 * f3: ((x&y) | (x&z) | (y&z)) == ((x&y) | (z&(x|y)))
 */
#define f1(x,y,z)       (z ^ (x&(y^z)))		/* Rounds  0-19 */
#define f2(x,y,z)       (x^y^z)			/* Rounds 20-39 */
#define f3(x,y,z)       ((x&y) | (z&(x|y)))	/* Rounds 40-59 */
#define f4(x,y,z)       (x^y^z)			/* Rounds 60-79 */
 
/* The SHS Mysterious Constants */
#define K1      0x5A827999L                                                                     /* Rounds  0-19 */
#define K2      0x6ED9EBA1L                                                                     /* Rounds 20-39 */
#define K3      0x8F1BBCDCL                                                                     /* Rounds 40-59 */
#define K4      0xCA62C1D6L                                                                     /* Rounds 60-79 */
 
/* SHS initial values */
#define h0init  0x67452301L
#define h1init  0xEFCDAB89L
#define h2init  0x98BADCFEL
#define h3init  0x10325476L
#define h4init  0xC3D2E1F0L
 
/* 32-bit rotate left - kludged with shifts */
#define ROT(X,n)  (((X)<<(n)) | ((X)>>(32-(n))))
 
/* 
 * The initial expanding function.  The hash function is defined over an
 * 80-word expanded input array W, where the first 16 are copies of the input
 * data, and the remaining 64 are defined by
 *
 *      W[ i ] = W[ i - 16 ] ^ W[ i - 14 ] ^ W[ i - 8 ] ^ W[ i - 3 ]
 *
 * This implementation generates these values on the fly in a circular
 * buffer - thanks to Colin Plumb, colin@nyx10.cs.du.edu for this
 * optimization.
 */
#define exor(W,i) (W[(i)&15] ^= W[(i)-14&15] ^ W[(i)-8&15] ^ W[(i)-3&15])

/* 
 * The prototype SHS sub-round.  The fundamental sub-round is:
 *
 *      a' = e + ROT(a,5) + f(b,c,d) + k + data;
 *      b' = a;
 *      c' = ROT(b,30);
 *      d' = c;
 *      e' = d;
 *
 * but this is implemented by unrolling the loop 5 times and renaming the
 * variables ( e, a, b, c, d ) = ( a', b', c', d', e' ) each iteration.
 * This code is then replicated 20 times for each of the 4 functions, using
 * the next 20 values from the W[] array each time.
 */
#define subRound(a, b, c, d, e, f, k, data) \
    (e += ROT(a,5) + f(b,c,d) + k + data, b = ROT(b,30))

/* forward declarations */
static void shsTransform P((ULONG*, ULONG*));


/*
 * Initialize the SHS values
 */
void
shsInit(shsInfo)
    SHS_INFO *shsInfo;
{
    /* Set the h-vars to their initial values */
    shsInfo->digest[0] = h0init;
    shsInfo->digest[1] = h1init;
    shsInfo->digest[2] = h2init;
    shsInfo->digest[3] = h3init;
    shsInfo->digest[4] = h4init;

    /* Initialise bit count */
    shsInfo->countLo = shsInfo->countHi = shsInfo->datalen = 0L;
}


/*
 * Perform the SHS transformation.  Note that this code, like MD5, seems to
 * break some optimizing compilers - it may be necessary to split it into
 * sections, eg based on the four subrounds
 */
static void
shsTransform(digest, W)
    ULONG *digest;
    ULONG *W;
{
    ULONG A, B, C, D, E;	/* Local vars */
 
    /* Set up first buffer and local data buffer */
    A = digest[0];
    B = digest[1];
    C = digest[2];
    D = digest[3];
    E = digest[4];
 
    /* Heavy mangling, in 4 sub-rounds of 20 interations each. */
    subRound(A, B, C, D, E, f1, K1, W[ 0]);
    subRound(E, A, B, C, D, f1, K1, W[ 1]);
    subRound(D, E, A, B, C, f1, K1, W[ 2]);
    subRound(C, D, E, A, B, f1, K1, W[ 3]);
    subRound(B, C, D, E, A, f1, K1, W[ 4]);
    subRound(A, B, C, D, E, f1, K1, W[ 5]);
    subRound(E, A, B, C, D, f1, K1, W[ 6]);
    subRound(D, E, A, B, C, f1, K1, W[ 7]);
    subRound(C, D, E, A, B, f1, K1, W[ 8]);
    subRound(B, C, D, E, A, f1, K1, W[ 9]);
    subRound(A, B, C, D, E, f1, K1, W[10]);
    subRound(E, A, B, C, D, f1, K1, W[11]);
    subRound(D, E, A, B, C, f1, K1, W[12]);
    subRound(C, D, E, A, B, f1, K1, W[13]);
    subRound(B, C, D, E, A, f1, K1, W[14]);
    subRound(A, B, C, D, E, f1, K1, W[15]);
    subRound(E, A, B, C, D, f1, K1, exor(W,16));
    subRound(D, E, A, B, C, f1, K1, exor(W,17));
    subRound(C, D, E, A, B, f1, K1, exor(W,18));
    subRound(B, C, D, E, A, f1, K1, exor(W,19));
 
    subRound(A, B, C, D, E, f2, K2, exor(W,20));
    subRound(E, A, B, C, D, f2, K2, exor(W,21));
    subRound(D, E, A, B, C, f2, K2, exor(W,22));
    subRound(C, D, E, A, B, f2, K2, exor(W,23));
    subRound(B, C, D, E, A, f2, K2, exor(W,24));
    subRound(A, B, C, D, E, f2, K2, exor(W,25));
    subRound(E, A, B, C, D, f2, K2, exor(W,26));
    subRound(D, E, A, B, C, f2, K2, exor(W,27));
    subRound(C, D, E, A, B, f2, K2, exor(W,28));
    subRound(B, C, D, E, A, f2, K2, exor(W,29));
    subRound(A, B, C, D, E, f2, K2, exor(W,30));
    subRound(E, A, B, C, D, f2, K2, exor(W,31));
    subRound(D, E, A, B, C, f2, K2, exor(W,32));
    subRound(C, D, E, A, B, f2, K2, exor(W,33));
    subRound(B, C, D, E, A, f2, K2, exor(W,34));
    subRound(A, B, C, D, E, f2, K2, exor(W,35));
    subRound(E, A, B, C, D, f2, K2, exor(W,36));
    subRound(D, E, A, B, C, f2, K2, exor(W,37));
    subRound(C, D, E, A, B, f2, K2, exor(W,38));
    subRound(B, C, D, E, A, f2, K2, exor(W,39));

    subRound(A, B, C, D, E, f3, K3, exor(W,40));
    subRound(E, A, B, C, D, f3, K3, exor(W,41));
    subRound(D, E, A, B, C, f3, K3, exor(W,42));
    subRound(C, D, E, A, B, f3, K3, exor(W,43));
    subRound(B, C, D, E, A, f3, K3, exor(W,44));
    subRound(A, B, C, D, E, f3, K3, exor(W,45));
    subRound(E, A, B, C, D, f3, K3, exor(W,46));
    subRound(D, E, A, B, C, f3, K3, exor(W,47));
    subRound(C, D, E, A, B, f3, K3, exor(W,48));
    subRound(B, C, D, E, A, f3, K3, exor(W,49));
    subRound(A, B, C, D, E, f3, K3, exor(W,50));
    subRound(E, A, B, C, D, f3, K3, exor(W,51));
    subRound(D, E, A, B, C, f3, K3, exor(W,52));
    subRound(C, D, E, A, B, f3, K3, exor(W,53));
    subRound(B, C, D, E, A, f3, K3, exor(W,54));
    subRound(A, B, C, D, E, f3, K3, exor(W,55));
    subRound(E, A, B, C, D, f3, K3, exor(W,56));
    subRound(D, E, A, B, C, f3, K3, exor(W,57));
    subRound(C, D, E, A, B, f3, K3, exor(W,58));
    subRound(B, C, D, E, A, f3, K3, exor(W,59));
 
    subRound(A, B, C, D, E, f4, K4, exor(W,60));
    subRound(E, A, B, C, D, f4, K4, exor(W,61));
    subRound(D, E, A, B, C, f4, K4, exor(W,62));
    subRound(C, D, E, A, B, f4, K4, exor(W,63));
    subRound(B, C, D, E, A, f4, K4, exor(W,64));
    subRound(A, B, C, D, E, f4, K4, exor(W,65));
    subRound(E, A, B, C, D, f4, K4, exor(W,66));
    subRound(D, E, A, B, C, f4, K4, exor(W,67));
    subRound(C, D, E, A, B, f4, K4, exor(W,68));
    subRound(B, C, D, E, A, f4, K4, exor(W,69));
    subRound(A, B, C, D, E, f4, K4, exor(W,70));
    subRound(E, A, B, C, D, f4, K4, exor(W,71));
    subRound(D, E, A, B, C, f4, K4, exor(W,72));
    subRound(C, D, E, A, B, f4, K4, exor(W,73));
    subRound(B, C, D, E, A, f4, K4, exor(W,74));
    subRound(A, B, C, D, E, f4, K4, exor(W,75));
    subRound(E, A, B, C, D, f4, K4, exor(W,76));
    subRound(D, E, A, B, C, f4, K4, exor(W,77));
    subRound(C, D, E, A, B, f4, K4, exor(W,78));
    subRound(B, C, D, E, A, f4, K4, exor(W,79));
 
    /* Build message digest */
    digest[0] += A;
    digest[1] += B;
    digest[2] += C;
    digest[3] += D;
    digest[4] += E;
}


/*
 * Update SHS for a block of data.  This code does not assume that the
 * buffer size is a multiple of SHS_BLOCKSIZE bytes long.  This code
 * handles partial blocks between calls to shsUpdate().
 *
 * Changed to deal with partial buffers.  This in turn fixed the
 * time trial results.		Landon Curt Noll -- chongo@toad.com /\../\
 */
void
shsUpdate(shsInfo, buffer, count)
    SHS_INFO *shsInfo;
    BYTE *buffer;
    UINT count;
{
    ULONG in[SHS_BLOCKSIZE/sizeof(ULONG)];
    ULONG datalen = shsInfo->datalen;
    ULONG value;
    long tmp;
    int cnt;

    /*
     * Update bitcount
     */
    tmp = shsInfo->countLo;
    if ((shsInfo->countLo = tmp + ((ULONG)count<<3)) < tmp ) {
        shsInfo->countHi++;                             
    }
    shsInfo->countHi += count >> 29;

    /*
     * Catch the case of a non-empty data buffer
     */
    if (datalen > 0) {

	/* determine the size we need to copy */
	ULONG cpylen = SHS_BLOCKSIZE - datalen;

	/* case: new data will not fill the buffer */
	if (cpylen > count) {
	    memcpy((char *)(shsInfo->data)+datalen, buffer, count);
	    shsInfo->datalen = datalen+count;
	    return;

	/* case: buffer will be filled */
	} else {
	    memcpy((char *)(shsInfo->data)+datalen, buffer, cpylen);
#if BYTE_ORDER == LITTLE_ENDIAN 
	    /* byte swap data into big endian order */
	    for (cnt=0; cnt < SHS_BLOCKSIZE/sizeof(ULONG); ++cnt) {
		value = (shsInfo->data[cnt] << 16) |
			(shsInfo->data[cnt] >> 16);
		in[cnt] = ((value & 0xFF00FF00L) >> 8) |
			  ((value & 0x00FF00FFL) << 8);
	    }
	    shsTransform(shsInfo->digest, in);
#else
	    shsTransform(shsInfo->digest, shsInfo->data);
#endif
	    buffer += cpylen;
	    count -= cpylen;
	    shsInfo->datalen = 0;
	}
    }

    /*
     * Process data in SHS_BLOCKSIZE chunks
     */
    while (count >= SHS_BLOCKSIZE) {
#if defined(MUST_ALIGN)
	memcpy(in, buffer, SHS_BLOCKSIZE);
#if BYTE_ORDER == LITTLE_ENDIAN 
	/* byte swap data into big endian order */
	for (cnt=0; cnt < SHS_BLOCKSIZE/sizeof(ULONG); ++cnt) {
	    value = (in[cnt] << 16) | (in[cnt] >> 16);
	    in[cnt] = ((value & 0xFF00FF00L) >> 8) |
		      ((value & 0x00FF00FFL) << 8);
	}
#endif
	shsTransform(shsInfo->digest, in);
#else
#if BYTE_ORDER == BIG_ENDIAN
	shsTransform(shsInfo->digest, (ULONG *)buffer);
#else
	/* byte swap data into big endian order */
	for (cnt=0; cnt < SHS_BLOCKSIZE/sizeof(ULONG); ++cnt) {
	    value = (((ULONG *)buffer)[cnt] << 16) | 
	    	    (((ULONG *)buffer)[cnt] >> 16);
	    in[cnt] = ((value & 0xFF00FF00L) >> 8) |
		      ((value & 0x00FF00FFL) << 8);
	}
	shsTransform(shsInfo->digest, in);
#endif
#endif
	buffer += SHS_BLOCKSIZE;
	count -= SHS_BLOCKSIZE;
    }

    /*
     * Handle any remaining bytes of data.
     * This should only happen once on the final lot of data
     */
    if (count > 0) {
	memcpy(shsInfo->data, buffer, count);
    }
    shsInfo->datalen = count;
}


/*
 * Update SHS for a block of data with no partial block.  This function
 * assumes that count is a multiple of SHS_BLOCKSIZE and that no partial
 * blocks is left over from a previous call.
 */
void
shsfullUpdate(shsInfo, buffer, count)
    SHS_INFO *shsInfo;
    BYTE *buffer;
    UINT count;
{
#if defined(MUST_ALIGN) || BYTE_ORDER == LITTLE_ENDIAN
    ULONG in[SHS_BLOCKSIZE/sizeof(ULONG)];
#endif
    ULONG value;
    int cnt;

    /*
     * Update bitcount
     */
    if ((shsInfo->countLo + ((ULONG) count << 3)) < shsInfo->countLo) {
	 /* Carry from low to high bitCount */
	 shsInfo->countHi++;
    }
    shsInfo->countLo += ((ULONG) count << 3);
    shsInfo->countHi += ((ULONG) count >> 29);

    /*
     * Process data in SHS_BLOCKSIZE chunks
     */
    while (count >= SHS_BLOCKSIZE) {
#if defined(MUST_ALIGN)
	memcpy(in, buffer, SHS_BLOCKSIZE);
#if BYTE_ORDER == LITTLE_ENDIAN 
	/* byte swap data into big endian order */
	for (cnt=0; cnt < SHS_BLOCKSIZE/sizeof(ULONG); ++cnt) {
	    value = (in[cnt] << 16) | (in[cnt] >> 16);
	    in[cnt] = ((value & 0xFF00FF00L) >> 8) |
		      ((value & 0x00FF00FFL) << 8);
	}
#endif
	shsTransform(shsInfo->digest, in);
#else
#if BYTE_ORDER == BIG_ENDIAN
	shsTransform(shsInfo->digest, (ULONG *)buffer);
#else
	/* byte swap data into big endian order */
	for (cnt=0; cnt < SHS_BLOCKSIZE/sizeof(ULONG); ++cnt) {
	    value = (((ULONG *)buffer)[cnt] << 16) | 
	    	    (((ULONG *)buffer)[cnt] >> 16);
	    in[cnt] = ((value & 0xFF00FF00L) >> 8) |
		      ((value & 0x00FF00FFL) << 8);
	}
	shsTransform(shsInfo->digest, in);
#endif
#endif
	buffer += SHS_BLOCKSIZE;
	count -= SHS_BLOCKSIZE;
    }
}


void
shsFinal(shsInfo)
    SHS_INFO *shsInfo;
{
    int cnt;
    ULONG value;
    int count = shsInfo->datalen;
    ULONG lowBitcount = shsInfo->countLo;
    ULONG highBitcount = shsInfo->countHi;

    /*
     * Set the first char of padding to 0x80.
     * This is safe since there is always at least one byte free
     */
    ((BYTE *)shsInfo->data)[count++] = 0x80;

    /* Pad out to 56 mod 64 */
    if (count > 56) {
	/* Two lots of padding:  Pad the first block to 64 bytes */
	memset((BYTE *) shsInfo->data + count, 0, 64 - count);
#if BYTE_ORDER == LITTLE_ENDIAN 
	/* byte swap data into big endian order */
	for (cnt=0; cnt < SHS_BLOCKSIZE/sizeof(ULONG); ++cnt) {
	    value = (shsInfo->data[cnt] << 16) |
		    (shsInfo->data[cnt] >> 16);
	    shsInfo->data[cnt] = ((value & 0xFF00FF00L) >> 8) |
				 ((value & 0x00FF00FFL) << 8);
	}
#endif
	shsTransform(shsInfo->digest, shsInfo->data);

	/* Now fill the next block with 56 bytes */
	memset(shsInfo->data, 0, 56);
    } else
	/* Pad block to 56 bytes */
	memset((BYTE *) shsInfo->data + count, 0, 56 - count);
#if BYTE_ORDER == LITTLE_ENDIAN
	/* byte swap data into big endian order */
	for (cnt=0; cnt < SHS_BLOCKSIZE/sizeof(ULONG); ++cnt) {
	    value = (shsInfo->data[cnt] << 16) |
	    	    (shsInfo->data[cnt] >> 16);
	    shsInfo->data[cnt] = ((value & 0xFF00FF00L) >> 8) |
	    			 ((value & 0x00FF00FFL) << 8);
	}
#endif

    /*
     * Append length in bits and transform
     */
    shsInfo->data[14] = highBitcount;
    shsInfo->data[15] = lowBitcount;
    shsTransform(shsInfo->digest, shsInfo->data);
#if BYTE_ORDER != BIG_ENDIAN 
	for (cnt=0; cnt < SHS_DIGESTSIZE/sizeof(ULONG); ++cnt) {
	    value = (shsInfo->data[cnt] << 16) |
	    	    (shsInfo->data[cnt] >> 16);
	    shsInfo->data[cnt] = ((value & 0xFF00FF00L) >> 8) |
	    			 ((value & 0x00FF00FFL) << 8);
	}
#endif
}
