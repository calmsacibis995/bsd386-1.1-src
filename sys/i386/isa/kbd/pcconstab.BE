/*
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	$Id: pcconstab.BE,v 1.2 1994/01/28 04:52:13 karels Exp $
 */

/*
 * Copyright (c) 1992 Lode Vande Sande, YaBBS,
 * Belgium. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by YaBBS, Belgium.
 * 4. Neither the name of the BBS nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY YABBS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 *	Belgian Keyboard mapping tables
 */

struct key pcconstab[] = {
/* 00 */ KNULL,
/* 01 */ KCONST(033),
/* 02 */ KCHAR('&',	'1',	'1',	'1',	'|',	000,	000,	000),
/* 03 */ KCHAR(130,	'2',	'2',	'2',	'@',	000,	000,	171),
/* 04 */ KCHAR('"',	'3',	176,	177,	'#',	178,	219,	219),
/* 05 */ KCHAR('\'',	'4',	'4',	'4',	000,	000,	000,	172),
/* 06 */ KCHAR('(',	'5',	'5',	'5',	000,	000,	000,	000),
/* 07 */ KCHAR(21,	'6',	'6',	'6',	'^',	232,	232,	232),
/* 08 */ KCHAR(138,	'7',	'7',	'7',	000,	000,	000,	236),
/* 09 */ KCHAR('!',	'8',	'8',	'8',	173,	173,	173,	173),
/* 0a */ KCHAR(135,	'9',	'9',	'9',	'{',	000,	000,	000),
/* 0b */ KCHAR(133,	'0',	'0',	'0',	'}',	000,	000,	000),
/* 0c */ KCHAR(')',	248,	166,	167,	166,	167,	166,	167),
/* 0d */ KCHAR('-',	'_',	000,	000,	000,	000,	000,	246),
/* 0e */ KMCHAR(010,	010,	034,	034),
/* 0f */ KMCHAR('\t',	'\t',	'\177',	'\177'),
/* 10 */ KALPHA('a',	'A',	001,	160,	131,	133,	142,	142),
/* 11 */ KALPHA('z',	'Z',	032,	32,	0,	0,	0,	0),
/* 12 */ KALPHA('e',	'E',	005,	130,	136,	138,	137,	144),
/* 13 */ KALPHA('r',	'R',	022,	0,	0,	0,	0,	0),
/* 14 */ KALPHA('t',	'T',	024,	0,	0,	0,	0,	0),
/* 15 */ KALPHA('y',	'Y',	031,	0,	152,	0,	0,	157),
/* 16 */ KALPHA('u',	'U',	025,	163,	150,	151,	129,	154),
/* 17 */ KALPHA('i',	'I',	011,	161,	140,	141,	139,	0),
/* 18 */ KALPHA('o',	'O',	017,	162,	147,	149,	148,	153),
/* 19 */ KALPHA('p',	'P',	020,	158,	227,	158,	227,	158),
/* 1a */ KALPHA('^',	':',	033,	033,	'[',	000,	000,	000),
/* 1b */ KALPHA('$',	'*',	035,	035,	']',	015,	015,	015),
/* 1c */ KMCHAR('\r',	'\r',	'\r',	'\r'),
/* 1d */ KCTL,
/* 1e */ KALPHA('q',	'Q',	021,	021,	'@',	'@',	021,	021),
/* 1f */ KALPHA('s',	'S',	023,	023,	228,	228,	228,	228),
/* 20 */ KALPHA('d',	'D',	004,	004,	235,	235,	235,	235),
/* 21 */ KALPHA('f',	'F',	006,	006,	232,	159,	232,	159),
/* 22 */ KALPHA('g',	'G',	007,	007,	231,	231,	231,	231),
/* 23 */ KMALPHA('h',	'H',	010,	010),
/* 24 */ KALPHA('j',	'J',	012,	012,	244,	245,	244,	245),
/* 25 */ KMALPHA('k',	'K',	013,	013),
/* 26 */ KMALPHA('l',	'L',	014,	014),
/* 27 */ KMALPHA('m',	'M',	015,	015),
/* 28 */ KCHAR(151,	'%',	145,	145,	39,	145,	145,	145),
/* 29 */ KCHAR(253,	252,	0,	0,	0,	0,	0,	0),
/* 2a */ KSHF,
/* 2b */ KCHAR(230,	156,	0,	0,	96,	0,	0,	0),
/* 2c */ KALPHA('w',	'W',	027,	027,	017,	016,	030,	031),
/* 2d */ KALPHA('x',	'X',	030,	030,	024,	025,	026,	000),
/* 2e */ KALPHA('c',	'C',	003,	135,	0,	155,	0,	128),
/* 2f */ KALPHA('v',	'V',	026,	026,	239,	251,	239,	251),
/* 30 */ KALPHA('b',	'B',	002,	002,	225,	225,	225,	225),
/* 31 */ KALPHA('n',	'N',	016,	016,	264,	165,	264,	165),
/* 32 */ KCHAR(',',	'?',	015,	015, 	168,	168,	168,	168),
/* 33 */ KCHAR(';',	'.',	254,	220,	223,	248,	249,	250),
/* 34 */ KMCHAR(':',	'/',	0,	0),
/* 35 */ KCHAR('=',	'+',	240,	241,	'~',	247,	0,	241),
/* 36 */ KRSHF,
/* 37 */ KNUMPAD('*', '\033', '[', '*'),
/* 38 */ KALT,
/* 39 */ KMCHAR(' ',	' ',	000,	000),
#ifndef CAPSLOCK_CONTROL
/* 3a */ KCAPSLOCK,
#else
/* 3a */ KCTL,
#endif
/* 3b */ KSFUNC('M', 'Y', 'k', 'w', 'M', 'Y', 'k', 'w'),
/* 3c */ KSFUNC('N', 'Z', 'l', 'x', 'N', 'Z', 'l', 'x'),
/* 3d */ KSFUNC('O', 'a', 'm', 'y', 'O', 'a', 'm', 'y'),
/* 3e */ KSFUNC('P', 'b', 'n', 'z', 'P', 'b', 'n', 'z'),
/* 3f */ KSFUNC('Q', 'c', 'o', '@', 'Q', 'c', 'o', '@'),
/* 40 */ KSFUNC('R', 'd', 'p', '[', 'R', 'd', 'p', '['),
/* 41 */ KSFUNC('S', 'e', 'q', '\\', 'S', 'e', 'q', '\\'),
/* 42 */ KSFUNC('T', 'f', 'r', ']', 'T', 'f', 'r', ']'),
/* 43 */ KSFUNC('U', 'g', 's', '^', 'U', 'g', 's', '^'),
/* 44 */ KSFUNC('V', 'h', 't', '_', 'V', 'h', 't', '_'),
/* 45 */ KNUM,
/* 46 */ KSCROLL,
/* 47 */ KNUMPAD('7', '\033', '[', 'H' ),
/* 48 */ KNUMPAD('8', '\033', '[', 'A'),
/* 49 */ KNUMPAD('9', '\033', '[', 'I'),
/* 4a */ KNUMPAD('-', '\033', '[', '-'),
/* 4b */ KNUMPAD('4', '\033', '[', 'D'),
/* 4c */ KNUMPAD('5', '\033', '[', 'E'),
/* 4d */ KNUMPAD('6', '\033', '[', 'C'),
/* 4e */ KNUMPAD('+', '\033', '[', '+'),
/* 4f */ KNUMPAD('1', '\033', '[', 'F'),
/* 50 */ KNUMPAD('2', '\033', '[', 'B'),
/* 51 */ KNUMPAD('3', '\033', '[', 'G'),
/* 52 */ KNUMPAD('0', '\033', '[', 'L'),
/* 53 */ KNUMPAD('.', '\033', '[', '.'),
/* 54 */ KCHAR('1',	'.',	0,	0,	0,	0,	0,	250),
/* 55 */ KCHAR('2',	'.',	0,	0,	0,	0,	0,	250),
/* 56 */ KCHAR('<',	'>',	243,	242,	'\\',	174,	175,	0),
/* 57 */ KSFUNC('W', 'i', 'u', '`', 'W', 'i', 'u', '`'),
/* 58 */ KSFUNC('X', 'j', 'v', '{', 'X', 'j', 'v', '{'),
/* 59 */ KNULL,
/* what are the rest of these, compaq extensions? ??? */
#define STP 0	/* ??? */
/* 5a */ KCHAR(0, 	0,	033,	033,	0,	0,	033,	033),
/* 5b */ KCHAR(0,	0,	'7',	'7',	0,	0,	'7',	'7'),
/* 5c */ KCHAR(0,	0,	'4',	'4',	0,	0,	'4',	'4'),
/* 5d */ KCHAR(0,	0,	'1',	'1',	0,	0,	'1',	'1'),
/* 5e */ KNULL,
/* 5f */ KCHAR(0,	0,	NUM,	NUM,	0,	0,	NUM,	NUM),
/* 60 */ KFUNC("\033[A"),
/* 61 */ KFUNC("\033[D"),
/* 62 */ KFUNC("\033[B"),
/* 63 */ KFUNC("\033[C"),
/* 64 */ KFUNC("\033[H"),
/* 65 */ KFUNC("\033[I"),
/* 66 */ KFUNC("\033[F"),
/* 67 */ KFUNC("\033[G"),
/* 68 */ KFUNC("\033[L"),
/* 69 */ KFUNC("\033[."),
/* 6a */ KCHAR(0,	0,	'*',	'*',	0,	0,	'*',	'*'),
/* 6b */ KCHAR(0,	0,	'-',	'-',	0,	0,	'-',	'-'),
/* 6c */ KCHAR(0,	0,	'+',	'+',	0,	0,	'+',	'+'),
};

int pcconstabsize = sizeof(pcconstab) / sizeof(pcconstab[0]);

int pcextstart = 0x1c;
int pcextend = 0x54;

struct key pcexttab[] = {
/* 1c */ KCHAR('\r',	'\r',	'\r',	'\r',	'\r',	'\r',	'\r',	'\r'),
/* 1d */ KRCTL,
/* 1e */ KNULL,
/* 1f */ KNULL,
/* 20 */ KNULL,
/* 21 */ KNULL,
/* 22 */ KNULL,
/* 23 */ KNULL,
/* 24 */ KNULL,
/* 25 */ KNULL,
/* 26 */ KNULL,
/* 27 */ KNULL,
/* 28 */ KNULL,
/* 29 */ KNULL,
/* 2a */ KNULL,		/* simulated left shift */
/* 2b */ KNULL,
/* 2c */ KNULL,
/* 2d */ KNULL,
/* 2e */ KNULL,
/* 2f */ KNULL,
/* 30 */ KNULL,
/* 31 */ KNULL,
/* 32 */ KNULL,
/* 33 */ KNULL,
/* 34 */ KNULL,
/* 35 */ KCHAR('/',	'/',	'/',	'/',	'/',	'/',	'/',	'/'),
/* 36 */ KNULL,		/* simulated right shift */
/* 37 */ KNULL,		/* print scrn */
/* 38 */ KRALT,
/* 39 */ KNULL,
/* 3a */ KNULL,
/* 3b */ KNULL,
/* 3c */ KNULL,
/* 3d */ KNULL,
/* 3e */ KNULL,
/* 3f */ KNULL,
/* 40 */ KNULL,
/* 41 */ KNULL,
/* 42 */ KNULL,
/* 43 */ KNULL,
/* 44 */ KNULL,
/* 45 */ KNUM,
/* 46 */ KNULL,			/* break (control-pause) */
/* 47 */ KFUNC("\033[H"),	/* edit-pad home */
/* 48 */ KFUNC("\033[A"),	/* cursor-pad up arrow */
/* 49 */ KFUNC("\033[I"),	/* edit-pad page up */
/* 4a */ KNULL,
/* 4b */ KFUNC("\033[D"),	/* cursor-pad left arrow */
/* 4c */ KNULL,
/* 4d */ KFUNC("\033[C"),	/* cursor-pad right arrow */
/* 4e */ KNULL,
/* 4f */ KFUNC("\033[F"),	/* edit-pad end */
/* 50 */ KFUNC("\033[B"),	/* cursor-pad down arrow */
/* 51 */ KFUNC("\033[G"),	/* edit-pad page down */
/* 52 */ KFUNC("\033[L"),	/* edit-pad insert */
/* 53 */ KCONST(0177),		/* edit-pad delete (or "\033[."?) */
};

/* Alternate key mapping: unused */
struct key pcaltconstab[] = {
/* 00 */ KNULL,
};

int pcaltconstabsize = sizeof(pcaltconstab) / sizeof(pcaltconstab[0]);

/*
 * Accented key table: unused.
 */
struct key accenttab[] = {
	0
};

int maxaccent = 0;
