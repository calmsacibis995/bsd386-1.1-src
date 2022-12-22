/*
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	$Id: pcconstab.SP,v 1.2 1994/01/28 04:54:15 karels Exp $
 */

/*-
 * Copyright (c) 1992 Dirk Husemann, University of Erlangen-Nurnberg,
 * Germany. All rights reserved.
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
 *	This product includes software developed by the University of
 *	Erlangen-Nuremberg, Germany.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY ``AS IS'' AND
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
 *	SPanish Keyboard mapping tables (102-key keyboard)
 *	from alberto@proa.es
 */

/*
 * Accented key table. 
 * when a key is entered that may be accented (action ACCENTED),
 * its entry gives us an index into the accented key table.
 * Each key has a group of entries, indexed by these values
 * (0 is unaccented).  Thus we have 5 adjacent entries for each key:
 * unaccented, circumflex, trema, acute, grave
 */
#define	M_CIRCUMFLEX	1
#define	M_TREMA		2
#define	M_ACUTE		3
#define	M_GRAVE		4

int maxaccent = M_GRAVE;

struct key accenttab[] = {
#define	ACCENT_A	0
/* 10 */ KMALPHA('a',   'A',    001,    001),	/* unaccented */
/* 10 */ KMALPHA(0203,  'A',    001,    001),	/* circumflex */
/* 10 */ KMALPHA(0204,  142,    001,    001),	/* trema */
/* 10 */ KMALPHA( 160,  'A',    001,    001),	/* acute */
/* 10 */ KMALPHA( 133,  'A',    001,    001),	/* grave */

#define	ACCENT_E	5
/* 12 */ KMALPHA('e',   'E',    005,    005),	/* unaccented */
/* 12 */ KMALPHA(0210,  'E',    005,    005),	/* circumflex */
/* 12 */ KMALPHA(0211,  'E',    005,    005),	/* trema */
/* 12 */ KMALPHA( 130,  144,    005,    005),	/* acute */
/* 12 */ KMALPHA( 138,  'E',    001,    005),	/* grave */

#define	ACCENT_U	10
/* 16 */ KMALPHA('u',   'U',    025,    025),	/* unaccented */
/* 16 */ KMALPHA(0226,  'U',    025,    025),	/* circumflex */
/* 16 */ KMALPHA(0201,  154,    025,    025),	/* trema */
/* 16 */ KMALPHA( 163,  'U',    025,    025),	/* acute */
/* 16 */ KMALPHA( 151,  'U',    025,    025),	/* grave */

#define	ACCENT_I	15
/* 17 */ KMALPHA('i',   'I',    011,    011),	/* unaccented */
/* 17 */ KMALPHA(0214,  'I',    011,    011),	/* circumflex */
/* 17 */ KMALPHA(0213,  'I',    011,    011),	/* trema */
/* 17 */ KMALPHA( 161,  'I',    011,    011),	/* acute */
/* 17 */ KMALPHA( 141,  'I',    011,    011),	/* grave */

#define	ACCENT_O	20
/* 18 */ KMALPHA('o',   'O',    017,    017),	/* unaccented */
/* 18 */ KMALPHA(0223,  'O',    017,    017),	/* circumflex */
/* 18 */ KMALPHA(0224,  153,    017,    017),	/* trema */
/* 18 */ KMALPHA( 162,  'O',    017,    017),	/* acute */
/* 18 */ KMALPHA( 149,  'O',    017,    017),	/* grave */

#define	ACCENT_SPACE	25
/* 39 */ KMCHAR(' ',	' ',	000,	000),		/* unaccented */
/* 39 */ KCONST(0136),		/* circumflex */
/* 39 */ KMCHAR(' ',	' ',	000,	000),		/* no trema */
/* 39 */ KCONST(39),	/* acute */
/* 39 */ KCONST('`'),	/* grave */
};

struct key pcconstab[] = {
/* 00 */ KNULL,
/* 01 */ KCONST(033),
/* 02 */ KCHAR('1',	'!',	'1',	'!',	'|',	'|',	'|',	'|'),
/* 03 */ KCHAR('2',	'"',	000,	000,	'@',	'@',	'@',	'@'),
/* 04 */ KCHAR('3',	250,	'3',	'#',	'#',	'#',	'#',	'#'),
/* 05 */ KMCHAR('4',	'$',	'4',	'$'),
/* 06 */ KMCHAR('5',	'%',	'5',	'%'),
/* 07 */ KCHAR('6',	'&',	036,	036,	170,	170,	170,	170),
/* 08 */ KMCHAR('7',	'/',	'7',	'/'),
/* 09 */ KMCHAR('8',	'(',	'8',	'('),
/* 0a */ KMCHAR('9',	')',	'9',	')'),
/* 0b */ KMCHAR('0',	'=',	'0',	'='),
/* 0c */ KMCHAR('\'',	'?',	037,	037),
/* 0d */ KMCHAR(173,	168,	'+',	'+'),
/* 0e */ KMCHAR(010,	010,	034,	034),
/* 0f */ KMCHAR('\t',	'\t',	'\177',	'\177'),
/* 10 */ KMALPHA('q',	'Q',	021,	021),
/* 11 */ KMALPHA('w',	'W',	027,	027),
/* 12 */ KACCENTED(ACCENT_E),
/* 13 */ KMALPHA('r',	'R',	022,	022),
/* 14 */ KMALPHA('t',	'T',	024,	024),
/* 15 */ KMALPHA('y',	'Y',	031,	031),
/* 16 */ KACCENTED(ACCENT_U),
/* 17 */ KACCENTED(ACCENT_I),
/* 18 */ KACCENTED(ACCENT_O),
/* 19 */ KMALPHA('p',	'P',	020,	020),
/* 1a */ /*KCHAR('`',	'^',	033,	033,	'[',	'[',	'[',	'['),*/
/* 1a */ KACCENT(M_GRAVE, M_CIRCUMFLEX,	033,	033,	'[',	'[',	'[',	'['),
/* 1b */ KCHAR('+',	'*',	035,	035,	']',	']',	']',	']'),
/* 1c */ KMCHAR('\r',	'\r',	'\r',	'\r'),
/* 1d */ KCTL,
/* 1e */ KACCENTED(ACCENT_A),
/* 1f */ KMALPHA('s',	'S',	023,	023),
/* 20 */ KMALPHA('d',	'D',	004,	004),
/* 21 */ KMALPHA('f',	'F',	006,	006),
/* 22 */ KMALPHA('g',	'G',	007,	007),
/* 23 */ KMALPHA('h',	'H',	010,	010),
/* 24 */ KMALPHA('j',	'J',	012,	012),
/* 25 */ KMALPHA('k',	'K',	013,	013),
/* 26 */ KMALPHA('l',	'L',	014,	014),
/* 27 */ KMCHAR(164,	165,	';',	';'),
/* 28 */ /*KCHAR('\'',	'"',	'\'',	'\'',	'{',	'{',	'{',	'{'),*/
/* 28 */ KACCENT(M_ACUTE, M_TREMA,'\'',	'\'',	'{',	'{',	'{',	'{'),
/* 29 */ KCHAR(167,	166,	'`',	'`',	'\\',	'\\',	'\\',	'\\'),
/* 2a */ KSHF,
/* 2b */ KCHAR(135,	128,	034,	034,	'}',	'}',	'}',	'}'),
/* 2c */ KMALPHA('z',	'Z',	032,	032),
/* 2d */ KMALPHA('x',	'X',	030,	030),
/* 2e */ KMALPHA('c',	'C',	003,	003),
/* 2f */ KMALPHA('v',	'V',	026,	026),
/* 30 */ KMALPHA('b',	'B',	002,	002),
/* 31 */ KMALPHA('n',	'N',	016,	016),
/* 32 */ KMALPHA('m',	'M',	015,	015),
/* 33 */ KMCHAR(',',	';',	'<',	'<'),
/* 34 */ KMCHAR('.',	':',	'>',	'>'),
/* 35 */ KMCHAR('-',	'_',	037,	037),
/* 36 */ KRSHF,
/* 37 */ KNUMPAD('*', '\033', '[', '*'),
/* 38 */ KALT,
/* 39 */ KACCENTED(ACCENT_SPACE),
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
/* 54 */ KNULL,
/* 55 */ KNULL,
/* 56 */ KMCHAR('<',	'>',	'~',	'~'),
/* 57 */ KSFUNC('W', 'i', 'u', '`', 'W', 'i', 'u', '`'),
/* 58 */ KSFUNC('X', 'j', 'v', '{', 'X', 'j', 'v', '{'),
/* 59 */ KNULL,
/* what are the rest of these, compaq extensions? ??? */
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
