#ifndef lint
static char rcsid[] = "$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/fax2ps/faxdecode.c,v 1.1.1.1 1994/01/14 23:09:41 torek Exp $";
#endif

/*
 * Copyright (c) 1991, 1992, 1993 by Sam Leffler.
 * All rights reserved.
 *
 * This file is provided for unrestricted use provided that this
 * legend is included on all tape media and as a part of the
 * software program in whole or part.  Users may copy, modify or
 * distribute this file at will.
 */
#include "defs.h"
#include "t4.h"
#include "g3states.h"

Fax3DecodeState fax;

#if USE_PROTOTYPES
static	int decode1DRow(TIFF*, u_char*, int);
static	int decode2DRow(TIFF*, u_char*, int);
static	void fillspan(char*, int, int);
static	void emitcode(TIFF*, int, int, int);
static	int findspan(u_char**, int, int, u_char*);
static	int finddiff(u_char*, int, int);
#else
static	int decode1DRow();
static	int decode2DRow();
static	void fillspan();
static	void emitcode();
static	int findspan();
static	int finddiff();
#endif

static void
DECLARE4(emitcode, TIFF*, tif, int, dx, int, x, int, count)
{
    CodeEntry* thisCode;

    switch (fax.pass) {
    case 1:    /* count potential code & pair use */
	thisCode = enterCode(x-dx, count);
	thisCode->c.count++;
	if (dopairs) {
	    if (fax.lastCode)
		enterPair(fax.lastCode, thisCode)->c.count++;
	    fax.lastCode = thisCode;
	}
	break;
    case 2:    /* rescan w/ potential codes */
	thisCode = enterCode(x-dx, count);
	if (fax.lastCode) {
	    CodePairEntry* pair = findPair(fax.lastCode, thisCode);
	    if (pair) {
		pair->c.count++;
		fax.lastCode = 0;
	    } else {
		fax.lastCode->c.count++;
		fax.lastCode = thisCode;
	    }
	} else
	    fax.lastCode = thisCode;
	break;
    case 3:    /* generate encoded output */
	thisCode = enterCode(x-dx, count);
	if (dopairs) {
	    if (fax.lastCode) {
		if (!printPair(tif, fax.lastCode, thisCode)) {
		    printCode(tif, fax.lastCode);
		    fax.lastCode = thisCode;
		} else
		    fax.lastCode = 0;
	    } else
		fax.lastCode = thisCode;
	} else
	    printCode(tif, thisCode);
	break;
    }
}

static u_char bitMask[8] =
    { 0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
#define	isBitSet(sp)    ((sp)->b.data & bitMask[(sp)->b.bit])

#define	is2DEncoding(tif)	(fax.is2d)
#define	fetchByte(tif, sp)	(fax.cc--, *fax.bp++)

#define	BITCASE(b)			\
    case b:				\
    code <<= 1;				\
    if (data & (1<<(7-b))) code |= 1;	\
    len++;				\
    if (code > 0) { bit = b+1; break; }

/*
 * Skip over input until an EOL code is found.  The
 * value of len is passed as 0 except during error
 * recovery when decoding 2D data.  Note also that
 * we don't use the optimized state tables to locate
 * an EOL because we can't assume much of anything
 * about our state (e.g. bit position).
 */
static void
DECLARE2(skiptoeol, TIFF*, tif, int, len)
{
    Fax3DecodeState *sp = &fax;
    register int bit = sp->b.bit;
    register int data = sp->b.data;
    int code = 0;

    /*
     * Our handling of ``bit'' is painful because
     * the rest of the code does not maintain it as
     * exactly the bit offset in the current data
     * byte (bit == 0 means refill the data byte).
     * Thus we have to be careful on entry and
     * exit to insure that we maintain a value that's
     * understandable elsewhere in the decoding logic.
     */
    if (bit == 0)            /* force refill */
        bit = 8;
    for (;;) {
        switch (bit) {
again:  BITCASE(0);
        BITCASE(1);
        BITCASE(2);
        BITCASE(3);
        BITCASE(4);
        BITCASE(5);
        BITCASE(6);
        BITCASE(7);
        default:
            if (fax.cc <= 0)
                return;
            data = fetchByte(tif, sp);
            goto again;
        }
        if (len >= 12 && code == EOL)
            break;
        code = len = 0;
    }
    sp->b.bit = bit > 7 ? 0 : bit;    /* force refill */
    sp->b.data = data;
}

/*
 * Return the next bit in the input stream.  This is
 * used to extract 2D tag values and the color tag
 * at the end of a terminating uncompressed data code.
 */
static int
DECLARE1(nextbit, TIFF*, tif)
{
    Fax3DecodeState *sp = &fax;
    int bit;

    if (sp->b.bit == 0 && fax.cc > 0)
        sp->b.data = fetchByte(tif, sp);
    bit = isBitSet(sp);
    if (++(sp->b.bit) > 7)
        sp->b.bit = 0;
    return (bit);
}

static void
DECLARE3(bset, unsigned char*, cp, int, n, int, v)
{
    while (n-- > 0)
        *cp++ = v;
}

/*
 * Setup state for decoding a strip.
 */
int
DECLARE1(FaxPreDecode, TIFF*, tif)
{
    Fax3DecodeState *sp = &fax;

    sp->b.bit = 0;            /* force initial read */
    sp->b.data = 0;
    sp->b.tag = G3_1D;
    if (sp->b.refline)
        bset(sp->b.refline, sp->b.rowbytes, sp->b.white ? 0xff : 0x00);
    /*
     * If image has EOL codes, they precede each line
     * of data.  We skip over the first one here so that
     * when we decode rows, we can use an EOL to signal
     * that less than the expected number of pixels are
     * present for the scanline.
     */
    if ((fax.options & FAX3_NOEOL) == 0) {
        skiptoeol(tif, 0);
        if (is2DEncoding(tif))
            /* tag should always be 1D! */
            sp->b.tag = nextbit(tif) ? G3_1D : G3_2D;
    }
    return (1);
}

/*
 * Fill a span with ones.
 */
static void
DECLARE3(fillspan, register char*, cp, register int, x, register int, count)
{
    static unsigned char masks[] =
        { 0, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff };

    if (count <= 0)
        return;
    cp += x>>3;
    if (x &= 7) {            /* align to byte boundary */
        if (count < 8 - x) {
            *cp++ |= masks[count] >> x;
            return;
        }
        *cp++ |= 0xff >> x;
        count -= 8 - x;
    }
    while (count >= 8) {
        *cp++ = 0xff;
        count -= 8;
    }
    *cp |= masks[count];
}

/*
 * Decode the requested amount of data.
 */
int
DECLARE2(Fax3DecodeRow, TIFF*, tif, int, npels)
{
    Fax3DecodeState *sp = &fax;

    fax.lastCode = 0;
    while (npels > 0) {
	/* decoding only sets non-zero bits */
	memset(sp->scanline, 0, sp->b.rowbytes);
        if (sp->b.tag == G3_1D) {
            if (!decode1DRow(tif, sp->scanline, sp->b.rowpixels) && fax.cc <= 0)
		break;
        } else {
            if (!decode2DRow(tif, sp->scanline, sp->b.rowpixels) && fax.cc <= 0)
		break;
        }
        if (is2DEncoding(tif)) {
            /*
             * Fetch the tag bit that indicates
             * whether the next row is 1d or 2d
             * encoded.  If 2d-encoded, then setup
             * the reference line from the decoded
             * scanline just completed.
             */
            sp->b.tag = nextbit(tif) ? G3_1D : G3_2D;
            if (sp->b.tag == G3_2D)
                memcpy(sp->b.refline, sp->scanline, sp->b.rowbytes);
        }
        npels -= sp->b.rowpixels;
	fax.row++;
    }
    switch (sp->pass) {
    case 2:
	if (fax.lastCode)
	    fax.lastCode->c.count++;
	break;
    case 3:
	if (fax.lastCode)
	    printCode(tif, fax.lastCode);
	break;
    }
    return (npels == 0);
}

int
DECLARE2(Fax4DecodeRow, TIFF*, tif, int, npels)
{
    Fax3DecodeState *sp = &fax;

    fax.lastCode = 0;
    while (npels > 0) {
	/* decoding only sets non-zero bits */
	memset(sp->scanline, 0, sp->b.rowbytes);
	if (!decode2DRow(tif, sp->scanline, sp->b.rowpixels))
	    return (0);
	memcpy(sp->b.refline, sp->scanline, sp->b.rowbytes);
	fax.row++;
        npels -= sp->b.rowpixels;
    }
    switch (sp->pass) {
    case 2:
	if (fax.lastCode)
	    fax.lastCode->c.count++;
	break;
    case 3:
	if (fax.lastCode)
	    printCode(tif, fax.lastCode);
	break;
    }
    return (1);
}

/*
 * Decode a run of white.
 */
static int
DECLARE1(decode_white_run, TIFF*, tif)
{
    Fax3DecodeState *sp = &fax;
    short state = sp->b.bit;
    short action;
    int runlen = 0;

    for (;;) {
        if (sp->b.bit == 0) {
    nextbyte:
            if (fax.cc <= 0)
                return (G3CODE_EOF);
            sp->b.data = fetchByte(tif, sp);
        }
        action = TIFFFax1DAction[state][sp->b.data];
        state = TIFFFax1DNextState[state][sp->b.data];
        if (action == ACT_INCOMP)
            goto nextbyte;
        if (action == ACT_INVALID)
            return (G3CODE_INVALID);
        if (action == ACT_EOL)
            return (G3CODE_EOL);
        sp->b.bit = state;
        action = RUNLENGTH(action - ACT_WRUNT);
        runlen += action;
        if (action < 64)
            return (runlen);
    }
    /*NOTREACHED*/
}

/*
 * Decode a run of black.
 */
static int
DECLARE1(decode_black_run, TIFF*, tif)
{
    Fax3DecodeState *sp = &fax;
    short state = sp->b.bit + 8;
    short action;
    int runlen = 0;

    for (;;) {
        if (sp->b.bit == 0) {
    nextbyte:
            if (fax.cc <= 0)
                return (G3CODE_EOF);
            sp->b.data = fetchByte(tif, sp);
        }
        action = TIFFFax1DAction[state][sp->b.data];
        state = TIFFFax1DNextState[state][sp->b.data];
        if (action == ACT_INCOMP)
            goto nextbyte;
        if (action == ACT_INVALID)
            return (G3CODE_INVALID);
        if (action == ACT_EOL)
            return (G3CODE_EOL);
        sp->b.bit = state;
        action = RUNLENGTH(action - ACT_BRUNT);
        runlen += action;
        if (action < 64)
            return (runlen);
        state += 8;
    }
    /*NOTREACHED*/
}

/*
 * Process one row of 1d Huffman-encoded data.
 */
static int
DECLARE3(decode1DRow, TIFF*, tif, u_char*, buf, int, npels)
{
    Fax3DecodeState *sp = &fax;
    int x = 0;
    int dx = 0;
    int runlen;
    short action;
    short color = sp->b.white;
    static char module[] = "Fax3Decode1D";

    for (;;) {
        if (color == sp->b.white)
            runlen = decode_white_run(tif);
        else
            runlen = decode_black_run(tif);
        switch (runlen) {
        case G3CODE_EOF:
            TIFFError(module,
                "%s: Premature EOF at scanline %d (x %d)",
                TIFFFileName(tif), fax.row, x);
            return (0);
        case G3CODE_INVALID:    /* invalid code */
            /*
             * An invalid code was encountered.
             * Flush the remainder of the line
             * and allow the caller to decide whether
             * or not to continue.  Note that this
             * only works if we have a G3 image
             * with EOL markers.
             */
            TIFFError(TIFFFileName(tif),
               "%s: Bad code word at scanline %d (x %d)",
               module, fax.row, x);
            goto done;
        case G3CODE_EOL:    /* premature end-of-line code */
            TIFFWarning(TIFFFileName(tif),
                "%s: Premature EOL at scanline %d (x %d)",
                module, fax.row, x);
            return (1);    /* try to resynchronize... */
        }
        if (x+runlen > npels)
            runlen = npels-x;
        if (runlen > 0) {
            if (color)
                fillspan((char *)buf, x, runlen);
	    if (color != sp->b.white) {
		emitcode(tif, dx, x, runlen);
		dx = x+runlen;
	    }
            x += runlen;
            if (x >= npels)
                break;
        }
        color = !color;
    }
done:
    /*
     * Cleanup at the end of the row.  This convoluted
     * logic is merely so that we can reuse the code with
     * two other related compression algorithms (2 & 32771).
     *
     * Note also that our handling of word alignment assumes
     * that the buffer is at least word aligned.  This is
     * the case for most all versions of malloc (typically
     * the buffer is returned longword aligned).
     */
    if ((fax.options & FAX3_NOEOL) == 0)
        skiptoeol(tif, 0);
    if (fax.options & FAX3_BYTEALIGN)
        sp->b.bit = 0;
    if ((fax.options & FAX3_WORDALIGN) && ((long)fax.bp & 1))
        (void) fetchByte(tif, sp);
    return (x == npels);
}

/*
 * Group 3 2d Decoding support.
 */

/*
 * Return the next uncompressed mode code word.
 */
static int
DECLARE1(decode_uncomp_code, TIFF*, tif)
{
    Fax3DecodeState *sp = &fax;
    short code;

    do {
        if (sp->b.bit == 0 || sp->b.bit > 7) {
            if (fax.cc <= 0)
                return (UNCOMP_EOF);
            sp->b.data = fetchByte(tif, sp);
        }
        code = TIFFFaxUncompAction[sp->b.bit][sp->b.data];
        sp->b.bit = TIFFFaxUncompNextState[sp->b.bit][sp->b.data];
    } while (code == ACT_INCOMP);
    return (code);
}

/*
 * Process one row of 2d encoded data.
 */
static int
DECLARE3(decode2DRow, TIFF*, tif, u_char*, buf, int, npels)
{
#define	PIXEL(buf,ix)    ((((buf)[(ix)>>3]) >> (7-((ix)&7))) & 1)
    Fax3DecodeState *sp = &fax;
    int a0 = 0;
    int b1 = 0;
    int b2 = 0;
    int dx = 0;
    int run1, run2;        /* for horizontal mode */
    short mode;
    short color = sp->b.white;
    static char module[] = "Fax3Decode2D";

    do {
        if (sp->b.bit == 0 || sp->b.bit > 7) {
            if (fax.cc <= 0) {
                TIFFError(module,
                "%s: Premature EOF at scanline %d",
                    TIFFFileName(tif), fax.row);
                return (0);
            }
            sp->b.data = fetchByte(tif, sp);
        }
        mode = TIFFFax2DMode[sp->b.bit][sp->b.data];
        sp->b.bit = TIFFFax2DNextState[sp->b.bit][sp->b.data];
        switch (mode) {
        case MODE_NULL:
            break;
        case MODE_PASS:
            if (a0 || PIXEL(sp->b.refline, 0) == color) {
                b1 = finddiff(sp->b.refline, a0, npels);
                if (color == PIXEL(sp->b.refline, b1))
                    b1 = finddiff(sp->b.refline, b1, npels);
            } else
                b1 = 0;
            b2 = finddiff(sp->b.refline, b1, npels);
            if (color)
                fillspan((char *)buf, a0, b2 - a0);
	    if (color != sp->b.white) {
		emitcode(tif, dx, a0, b2 - a0);
		dx = b2;
	    }
            a0 += b2 - a0;
            break;
        case MODE_HORIZ:
            if (color == sp->b.white) {
                run1 = decode_white_run(tif);
                run2 = decode_black_run(tif);
            } else {
                run1 = decode_black_run(tif);
                run2 = decode_white_run(tif);
            }
	    /*
	     * Do the appropriate fill.  Note that we exit
	     * this logic with the same color that we enter
	     * with since we do 2 fills.  This explains the
	     * somewhat obscure logic below.
	     */
	    if (run1 < 0)
		goto bad;
	    if (a0 + run1 > npels)
		run1 = npels - a0;
	    if (color)
		fillspan((char *)buf, a0, run1);
	    if (color != sp->b.white) {
		emitcode(tif, dx, a0, run1);
		dx = a0 + run1;
	    }
	    a0 += run1;
	    if (run2 < 0)
		goto bad;
	    if (a0 + run2 > npels)
		run2 = npels - a0;
	    if (!color)
		fillspan((char *)buf, a0, run2);
	    if (!color != sp->b.white) {
		emitcode(tif, dx, a0, run2);
		dx = a0 + run2;
	    }
	    a0 += run2;
            break;
        case MODE_VERT_V0:
        case MODE_VERT_VR1:
        case MODE_VERT_VR2:
        case MODE_VERT_VR3:
        case MODE_VERT_VL1:
        case MODE_VERT_VL2:
        case MODE_VERT_VL3:
            /*
             * Calculate b1 as the "first changing element
             * on the reference line to right of a0 and of
             * opposite color to a0".  In addition, "the
             * first starting picture element a0 of each
             * coding line is imaginarily set at a position
             * just before the first picture element, and
             * is regarded as a white element".  For us,
             * the condition (a0 == 0 && color == sp->b.white)
             * describes this initial condition. 
             */
            if (!(a0 == 0 &&
        color == sp->b.white && PIXEL(sp->b.refline, 0) != sp->b.white)) {
                b1 = finddiff(sp->b.refline, a0, npels);
                if (color == PIXEL(sp->b.refline, b1))
                    b1 = finddiff(sp->b.refline, b1, npels);
            } else
                b1 = 0;
            b1 += mode - MODE_VERT_V0;
            if (color)
                fillspan((char *)buf, a0, b1 - a0);
	    if (color != sp->b.white) {
		emitcode(tif, dx, a0, b1 - a0);
		dx = b1;
	    }
            color = !color;
            a0 += b1 - a0;
            break;
	case MODE_UNCOMP:
            /*
             * Uncompressed mode: select from the
             * special set of code words.
             */
            do {
                mode = decode_uncomp_code(tif);
                switch (mode) {
                case UNCOMP_RUN1:
                case UNCOMP_RUN2:
                case UNCOMP_RUN3:
                case UNCOMP_RUN4:
                case UNCOMP_RUN5:
                    run1 = mode - UNCOMP_RUN0;
                    fillspan((char *)buf, a0+run1-1, 1);
                    a0 += run1;
		    if (color != sp->b.white) {
			emitcode(tif, dx, a0-1, 1);
			dx = a0;
		    }
                    break;
                case UNCOMP_RUN6:
                    a0 += 5;
                    break;
                case UNCOMP_TRUN0:
                case UNCOMP_TRUN1:
                case UNCOMP_TRUN2:
                case UNCOMP_TRUN3:
                case UNCOMP_TRUN4:
                    run1 = mode - UNCOMP_TRUN0;
                    a0 += run1;
                    color = nextbit(tif) ? !sp->b.white : sp->b.white;
                    break;
                case UNCOMP_INVALID:
                    TIFFError(module,
                "%s: Bad uncompressed code word at scanline %d",
                        TIFFFileName(tif), fax.row);
                    goto bad;
                case UNCOMP_EOF:
                    TIFFError(module,
                        "%s: Premature EOF at scanline %d",
                        TIFFFileName(tif), fax.row);
                    return (0);
                }
            } while (mode < UNCOMP_EXIT);
            break;
	case MODE_ERROR_1:
            if ((fax.options & FAX3_NOEOL) == 0) {
                TIFFWarning(TIFFFileName(tif),
                    "%s: Premature EOL at scanline %d (x %d)",
                    module, fax.row, a0);
                skiptoeol(tif, 7);    /* seen 7 0's already */
                return (1);        /* try to synchronize */
            }
            /* fall thru... */
	case MODE_ERROR:
            TIFFError(TIFFFileName(tif),
                "%s: Bad 2D code word at scanline %d",
                module, fax.row);
            goto bad;
	default:
            TIFFError(TIFFFileName(tif),
                "%s: Panic, bad decoding state at scanline %d",
                module, fax.row);
            return (0);
        }
    } while (a0 < npels);
bad:
    /*
     * Cleanup at the end of row.  We check for
     * EOL separately so that this code can be
     * reused by the Group 4 decoding routine.
     */
    if ((fax.options & FAX3_NOEOL) == 0)
        skiptoeol(tif, 0);
    return (a0 >= npels);
#undef	PIXEL
}

static u_char zeroruns[256] = {
    8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,    /* 0x00 - 0x0f */
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,    /* 0x10 - 0x1f */
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,    /* 0x20 - 0x2f */
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,    /* 0x30 - 0x3f */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,    /* 0x40 - 0x4f */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,    /* 0x50 - 0x5f */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,    /* 0x60 - 0x6f */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,    /* 0x70 - 0x7f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    /* 0x80 - 0x8f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    /* 0x90 - 0x9f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    /* 0xa0 - 0xaf */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    /* 0xb0 - 0xbf */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    /* 0xc0 - 0xcf */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    /* 0xd0 - 0xdf */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    /* 0xe0 - 0xef */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    /* 0xf0 - 0xff */
};
static u_char oneruns[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    /* 0x00 - 0x0f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    /* 0x10 - 0x1f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    /* 0x20 - 0x2f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    /* 0x30 - 0x3f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    /* 0x40 - 0x4f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    /* 0x50 - 0x5f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    /* 0x60 - 0x6f */
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,    /* 0x70 - 0x7f */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,    /* 0x80 - 0x8f */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,    /* 0x90 - 0x9f */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,    /* 0xa0 - 0xaf */
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,    /* 0xb0 - 0xbf */
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,    /* 0xc0 - 0xcf */
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,    /* 0xd0 - 0xdf */
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,    /* 0xe0 - 0xef */
    4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 7, 8,    /* 0xf0 - 0xff */
};

/*
 * Bit handling utilities.
 */

/*
 * Find a span of ones or zeros using the supplied
 * table.  The byte-aligned start of the bit string
 * is supplied along with the start+end bit indices.
 * The table gives the number of consecutive ones or
 * zeros starting from the msb and is indexed by byte
 * value.
 */
static int
DECLARE4(findspan, u_char**, bpp, int, bs, int, be, u_char*, tab)
{
    u_char *bp = *bpp;
    int bits = be - bs;
    int n, span;

    /*
     * Check partial byte on lhs.
     */
    if (bits > 0 && (n = (bs & 7))) {
	span = tab[(*bp << n) & 0xff];
	if (span > 8-n)        /* table value too generous */
	    span = 8-n;
	if (n+span < 8)        /* doesn't extend to edge of byte */
	    goto done;
	bits -= span;
	bp++;
    } else
	span = 0;
    /*
     * Scan full bytes for all 1's or all 0's.
     */
    while (bits >= 8) {
	n = tab[*bp];
	span += n;
	bits -= n;
	if (n < 8)        /* end of run */
	    goto done;
	bp++;
    }
    /*
     * Check partial byte on rhs.
     */
    if (bits > 0) {
	n = tab[*bp];
	span += (n > bits ? bits : n);
    }
done:
    *bpp = bp;
    return (span);
}

/*
 * Return the offset of the next bit in the range
 * [bs..be] that is different from bs.  The end,
 * be, is returned if no such bit exists.
 */
static int
DECLARE3(finddiff, u_char*, cp, int, bs, int, be)
{
    cp += bs >> 3;            /* adjust byte offset */
    return (bs + findspan(&cp, bs, be,
	(*cp & (0x80 >> (bs&7))) ? oneruns : zeroruns));
}
