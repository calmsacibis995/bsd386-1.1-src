/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/Class2Params.c++,v 1.1.1.1 1994/01/14 23:09:46 torek Exp $
/*
 * Copyright (c) 1990, 1991, 1992, 1993 Sam Leffler
 * Copyright (c) 1991, 1992, 1993 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */
#include "Class2Params.h"
#include "t.30.h"

/*
 * Tables for printing some Class 2 capabilities.
 */
const char* Class2Params::vresNames[2] = {
    "3.85 line/mm",
    "7.7 line/mm",
};
const char* Class2Params::bitRateNames[8] = {
    "2400 bit/s",		// BR_2400
    "4800 bit/s",		// BR_4800
    "7200 bit/s",		// BR_7200
    "9600 bit/s",		// BR_9600
    "12000 bit/s",		// BR_12000
    "14400 bit/s",		// BR_14400
    "0 bit/s",			// 6 ???
    "0 bit/s",			// 7 ???
};
const char* Class2Params::dataFormatNames[4] = {
    "1-D MR",			// DF_1DMR
    "2-D MR",			// DF_2DMR
    "2-D Uncompressed Mode",	// DF_2DUNCOMP
    "2-D MMR"			// DF_2DMMR
};
const char* Class2Params::pageWidthNames[8] = {
    "page width 1728 pixels in 215 mm",
    "page width 2048 pixels in 255 mm",
    "page width 2432 pixels in 303 mm",
    "page width 1216 pixels in 151 mm",
    "page width 864 pixels in 107 mm",
    "undefined page width (wd=5)",
    "undefined page width (wd=6)",
    "undefined page width (wd=7)",
};
const char* Class2Params::pageLengthNames[4] = {
    "A4 page length (297 mm)",
    "B4 page length (364 mm)",
    "unlimited page length ",
    "invalid page length (ln=3)",
};
const char* Class2Params::scanlineTimeNames[8] = {
    "0 ms/scanline",
    "5 ms/scanline",
    "10 ms, 5 ms/scanline",
    "10 ms/scanline",
    "20 ms, 10 ms/scanline",
    "20 ms/scanline",
    "40 ms, 20 ms/scanline",
    "40 ms/scanline",
};

Class2Params::Class2Params()
{
}

int
Class2Params::operator==(const Class2Params& other) const
{
    return vr == other.vr
	&& br == other.br
	&& wd == other.wd
	&& ln == other.ln
	&& df == other.df
	&& ec == other.ec
	&& bf == other.bf
	&& st == other.st;
}

int
Class2Params::operator!=(const Class2Params& other) const
{
    return !(*this == other);
}

static char*
addParam(char* cp, u_int v)
{
    if (v != (u_int)-1) {
	sprintf(cp, ",%u", v);
	while (*cp != '\0') cp++;
    } else {
	*cp++ = ',';
	*cp = '\0';
    }
    return (cp);
}

fxStr
Class2Params::cmd() const
{
    char buf[1024];
    char* cp = buf;

    if (vr != -1) {
	sprintf(cp, "%u", vr);
	while (*cp != '\0') cp++;
    }
    cp = addParam(cp, br);
    cp = addParam(cp, wd);
    cp = addParam(cp, ln);
    cp = addParam(cp, df);
    cp = addParam(cp, ec);
    cp = addParam(cp, bf);
    cp = addParam(cp, st);
    return fxStr(buf);
}

fxBool
Class2Params::is2D() const
{
    return (DF_2DMR <= df && df <= DF_2DMRUNCOMP);
}

/*
 * Tables to convert from Class 2
 * subparameter codes to a T.30 DIS.
 */
u_int Class2Params::vrDISTab[2] = {
    0,				// VR_NORMAL
    DIS_7MMVRES			// VR_FINE
};
u_int Class2Params::dfDISTab[4] = {
    0,				// 1-D MR
    DIS_2DENCODE,		// + 2-D MR
    DIS_2DENCODE,		// + Uncompressed data
    DIS_2DENCODE,		// + 2-D MMR
};
u_int Class2Params::brDISTab[8] = {
    DISSIGRATE_V27FB<<10,		// BR_2400
    DISSIGRATE_V27<<10,			// BR_4800
    (DISSIGRATE_V27|DISSIGRATE_V29)<<10,// BR_7200
    (DISSIGRATE_V27|DISSIGRATE_V29)<<10,// BR_9600
    0xD,				// BR_12000 (v.27,v.29,v.17,v.33)
    0xD,				// BR_14400 (v.27,v.29,v.17,v.33)
    (DISSIGRATE_V27|DISSIGRATE_V29)<<10,// 6 ?
    (DISSIGRATE_V27|DISSIGRATE_V29)<<10,// 7 ?
};
u_int Class2Params::wdDISTab[8] = {
    DISWIDTH_1728<<6,		// WD_1728
    DISWIDTH_2048<<6,		// WD_2048
    DISWIDTH_2432<<6,		// WD_2432
    DISWIDTH_1728<<6,		// WD_1216 XXX
    DISWIDTH_1728<<6,		// WD_864 XXX
    DISWIDTH_1728<<6,		// 5
    DISWIDTH_1728<<6,		// 6
    DISWIDTH_1728<<6,		// 7
};
u_int Class2Params::lnDISTab[3] = {
    DISLENGTH_A4<<4,		// LN_A4
    DISLENGTH_A4B4<<4,		// LN_B4
    DISLENGTH_UNLIMITED<<4	// LN_INF
};
u_int Class2Params::stDISTab[8] = {
    DISMINSCAN_0MS<<1,		// ST_0MS
    DISMINSCAN_5MS<<1,		// ST_5MS
    DISMINSCAN_10MS2<<1,	// ST_10MS2
    DISMINSCAN_10MS<<1,		// ST_10MS
    DISMINSCAN_20MS2<<1,	// ST_20MS2
    DISMINSCAN_20MS<<1,		// ST_20MS
    DISMINSCAN_40MS2<<1,	// ST_40MS2
    DISMINSCAN_40MS<<1,		// ST_40MS
};

/*
 * Convert a Class 2 bit rate code to a T.30
 * DCS code.  Beware that certain entries map
 * speeds and protocols (e.g. v.17 vs. v.33).
 */
u_int Class2Params::brDCSTab[8] = {
    DCSSIGRATE_2400V27,		// BR_2400
    DCSSIGRATE_4800V27,		// BR_4800
    DCSSIGRATE_7200V29,		// BR_7200
    DCSSIGRATE_9600V29,		// BR_9600
    DCSSIGRATE_12000V17,	// BR_12000
    DCSSIGRATE_14400V17,	// BR_14400
    DCSSIGRATE_9600V29,		// 6 ?
    DCSSIGRATE_9600V29,		// 7 ?
};
u_int Class2Params::stDCSTab[8] = {
    DISMINSCAN_0MS<<1,		// ST_0MS
    DISMINSCAN_5MS<<1,		// ST_5MS
    DISMINSCAN_10MS<<1,		// ST_10MS2
    DISMINSCAN_10MS<<1,		// ST_10MS
    DISMINSCAN_20MS<<1,		// ST_20MS2
    DISMINSCAN_20MS<<1,		// ST_20MS
    DISMINSCAN_40MS<<1,		// ST_40MS2
    DISMINSCAN_40MS<<1,		// ST_40MS
};

/*
 * Tables for mapping a T.30 DIS to Class 2
 * subparameter code values.
 */
u_int Class2Params::DISdfTab[2] = {
    DF_1DMR,			// !DIS_2DENCODE
    DF_2DMR			// DIS_2DENCODE
};
u_int Class2Params::DISvrTab[2] = {
    VR_NORMAL,			// !DIS_7MMVRES
    VR_FINE			// DIS_7MMVRES
};
/*
 * Beware that this table returns the ``best speed''
 * based on the signalling capabilities of the DIS.
 */
u_int Class2Params::DISbrTab[16] = {
    BR_4800,			// 0x0/V27FB
    BR_14400,			// 0x1/V17
    BR_9600,			// 0x2/undefined
    BR_14400,			// 0x3/V17+undefined
    BR_4800,			// 0x4/V27
    BR_14400,			// 0x5/V17+V27
    BR_4800,			// 0x6/V27+undefined
    BR_14400,			// 0x7/V17+V27+undefined
    BR_9600,			// 0x8/V29
    BR_14400,			// 0x9/V17+V29
    BR_9600,			// 0xA/V29+undefined
    BR_14400,			// 0xB/V17+V29+undefined
    BR_9600,			// 0xC/V29+V27
    BR_14400,			// 0xD/V17+V29+V29
    BR_9600,			// 0xE/V29+V27+undefined
    BR_14400,			// 0xF/V17+V29+V29+undefined
};
u_int Class2Params::DISwdTab[4] = {
    WD_1728,			// DISWIDTH_1728
    WD_2432,			// DISWIDTH_2432
    WD_2048,			// DISWIDTH_2048
    WD_2432			// invalid, but treated as 2432
};
u_int Class2Params::DISlnTab[4] = {
    LN_A4,			// DISLENGTH_A4
    LN_INF,			// DISLENGTH_UNLIMITED
    LN_B4,			// DISLENGTH_B4
    LN_A4			// undefined
};
u_int Class2Params::DISstTab[8] = {
    ST_20MS,			// DISMINSCAN_20MS
    ST_40MS,			// DISMINSCAN_40MS
    ST_10MS,			// DISMINSCAN_10MS
    ST_10MS2,			// DISMINSCAN_10MS2
    ST_5MS,			// DISMINSCAN_5MS
    ST_40MS2,			// DISMINSCAN_40MS2
    ST_20MS2,			// DISMINSCAN_20MS2
    ST_0MS			// DISMINSCAN_0MS
};

/*
 * Convert a T.30 DIS to a Class 2 parameter block.
 */
void
Class2Params::setFromDIS(u_int dis, u_int xinfo)
{
    vr = DISvrTab[(dis & DIS_7MMVRES) >> 9];
    /*
     * Beware that some modems (e.g. the Supra) indicate they
     * support the V.17 bit rates, but not the normal V.27+V.29
     * signalling rates.  The DISbrTab is NOT setup to mark the
     * V.27 and V.29 if V.17 is set.  Instead we let the upper
     * layers select appropriate signalling rate knowing that
     * we'll fall back to something that the modem will support.
     */
    br = DISbrTab[(dis & DIS_SIGRATE) >> 10];
    wd = DISwdTab[(dis & DIS_PAGEWIDTH) >> 6];
    ln = DISlnTab[(dis & DIS_PAGELENGTH) >> 4];
    if (xinfo & DIS_G4COMP)
	df = DF_2DMMR;
    else if (xinfo & DIS_2DUNCOMP)
	df = DF_2DMRUNCOMP;
    else
	df = DISdfTab[(dis & DIS_2DENCODE) >> 8];
    ec = (xinfo & DIS_ECMODE) ? EC_ENABLE : EC_DISABLE;
    bf = BF_DISABLE;			// XXX from xinfo
    st = DISstTab[(dis & DIS_MINSCAN) >> 1];
}

u_int Class2Params::DCSbrTab[16] = {
    BR_2400,			// 0x0/2400 V27
    BR_14400,			// 0x1/14400 V17
    BR_14400,			// 0x2/14400 V33
    0,				// 0x3/undefined 
    BR_4800,			// 0x4/4800 V27
    BR_12000,			// 0x5/12000 V17
    BR_12000,			// 0x6/12000 V33
    0,				// 0x7/undefined 
    BR_9600,			// 0x8/9600 V29
    BR_9600,			// 0x9/9600 V17
    BR_9600,			// 0xA/9600 V33
    BR_7200,			// 0xB/7200 V17
    BR_7200,			// 0xC/7200 V29
    0,				// 0xD/undefined 
    BR_7200,			// 0xE/7200 V33
    0,				// 0xF/undefined 
};

/*
 * Convert a T.30 DCS to a Class 2 parameter block.
 */
void
Class2Params::setFromDCS(u_int dcs, u_int xinfo)
{
    setFromDIS(dcs, xinfo);
    br = DCSbrTab[(dcs & DCS_SIGRATE) >> 10];	// override DIS setup
}

/*
 * Return a T.30 DCS frame that reflects the parameters.
 */
u_int
Class2Params::getDCS() const
{
    u_int dcs = DCS_T4RCVR
	    | vrDISTab[vr&1]
	    | brDCSTab[br&7]
	    | wdDISTab[wd&7]
	    | lnDISTab[ln&3]
	    | dfDISTab[df&3]
	    | stDCSTab[st&7]
	    ;
    dcs = (dcs | DCS_XTNDFIELD) << 8;
    /*
     * NB: G4 is easy if we can negotiate it, but not a good
     * idea unless the modem supports error correction.
     */
    if (df >= DF_2DMRUNCOMP)
	dcs |= DCS_2DUNCOMP;
    return (dcs);
}

/*
 * Return the number of bytes that can be
 * transferred at the selected signalling
 * rate in <ms> milliseconds.
 */
u_int
Class2Params::transferSize(u_int ms) const
{
    static const u_int brRates[8] = {
	2400/8,		// BR_2400
	4800/8,		// BR_4800
	7200/8,		// BR_7200
	9600/8,		// BR_9600
	12000/8,	// BR_12000
	14400/8,	// BR_14400
	14400/8,	// 6? XXX
	14400/8,	// 7? XXX
    };
    return (brRates[br & 7] * ms) / 1000;
}

/*
 * Return the minimum number of bytes in a
 * scanline as determined by the signalling
 * rate, vertical resolution, and min-scanline
 * time parameters.
 */
u_int
Class2Params::minScanlineSize() const
{
    static const u_int stTimes[8] =
	{ 0, 5, 10, 10, 20, 20, 40, 40 };
    u_int ms = stTimes[st&7];
    if ((st & 1) == 0 && vr == VR_FINE)
	ms /= 2;
    return transferSize(ms);
}
