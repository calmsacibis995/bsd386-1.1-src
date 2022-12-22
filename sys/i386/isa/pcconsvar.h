/*-
 * Copyright (c) 1991, 1992, 1993 Berkeley Software Design, Inc.
 * All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: pcconsvar.h,v 1.7 1993/11/06 01:54:00 karels Exp $
 */

struct	pcconsoftc {
	struct	device cs_dev;	/* base device */
	struct 	isadev cs_id;	/* ISA device */
	struct	intrhand cs_ih;	/* interrupt vectoring */
	struct	ttydevice_tmp cs_ttydev;	/* tty stuff */
	char	cs_flags;
};

#define CSF_RAW		0x01	/* in raw mode */

/*
 * structure for pcconstab key actions and resulting codes
 */
struct key {
  	u_char	action;
	u_char	code[8];
};

/*
 * key actions:
 * Actions for the first group of key types are internal,
 * and ignore the codes.  The actions for the second group
 * are to produce one of the codes in the following array,
 * depending on the state of the modifier keys.
 */
#define	IGNORE		0	/* ignore key input */
#define	SHF		1	/* (left) keyboard shift */
#define	R_SHF		2	/* right keyboard shift */
#define	CTL		3	/* (left) control modifier */
#define	R_CTL		4	/* right control modifier */
#define	ALT		5	/* (left) alternate modifier */
#define	R_ALT		6	/* right alternate modifier */
#define	ALTSHIFT	7	/* toggle alternate input set (locking) */
#define	ACCENT		8	/* pre-shift for accented characters */
#define	CAPSLOCK	9	/* caps lock -- swaps case of letter */
#define	CPS		CAPSLOCK
#define	NUM		10	/* numeric shift  cursors vs. numeric */
#define	DFUNC		11	/* debugging function key */
#define	SCROLL		12	/* scroll lock key */

/* produce code according to shift, control, alt: */
#define	CHAR		13 	/* character code(s) for this key */

/* produce code according to shift, shift lock, control, alt: */
#define	ALPHA		14 	/* character code(s) for this key */

/* produce string, ignoring modifier keys */
#define	FUNC		15	/* function key */

/* produce character or string, depending on num lock key */
#define	NUMPAD		16	/* numeric keypad key */

/* produce code according to accent, then shift, shift lock, control, alt: */
#define	ACCENTED	17 	/* index into accented character table */

/* produce standard function string \E[x, using modifier keys to pick x */
#define	SFUNC		18	/* shiftable function key */

/*
 * The index into the array of codes for ALPHA and CHAR is constructed
 * using the mask of active modifier keys using the following values:
 */
#define	M_SHIFT		0x01
#define	M_CONTROL	0x02
#define	M_ALT		0x04

/*
 * Macros for defining key values for common key classes
 */
#define	KNULL		{ IGNORE, { 0, 0, 0, 0, 0, 0, 0, 0 }}
#define KCTL		{ CTL, 	 { 0, 0, 0, 0, 0, 0, 0, 0 }}
#define KRCTL		{ R_CTL, { 0, 0, 0, 0, 0, 0, 0, 0 }}
#define	KALT		{ ALT,	 { 0, 0, 0, 0, 0, 0, 0, 0 }}
#define	KRALT		{ R_ALT, { 0, 0, 0, 0, 0, 0, 0, 0 }}
#define	KALTSHIFT	{ ALTSHIFT, { 0, 0, 0, 0, 0, 0, 0, 0 }}
#define KSHF		{ SHF,	 { 0, 0, 0, 0, 0, 0, 0, 0 }}
#define KRSHF		{ R_SHF, { 0, 0, 0, 0, 0, 0, 0, 0 }}
#define KCAPSLOCK	{ CAPSLOCK, { 0, 0, 0, 0, 0, 0, 0, 0 }}
#define	KNUM		{ NUM,	  { 0, 0, 0, 0, 0, 0, 0, 0 }}
#define KSCROLL		{ SCROLL, { 0, 0, 0, 0, 0, 0, 0, 0 }}
#define KDFUNC(v)	{ DFUNC,  { v, 0, 0, 0, 0, 0, 0, 0 }}

/* arbitrary character modified by shift/control/alt */
#define	KCHAR(u, s, c, cs, a, as, ac, acs) \
	{ CHAR, { u, s, c, cs, a, as, ac, acs }}

/* arbitrary character modified by shift/control; alt is "meta" */
#define	KMCHAR(u, s, c, cs) \
	{ CHAR, { u, s, c, cs, (u)|0x80, (s)|0x80, (c)|0x80, (cs)|0x80 }}

/* alphabetic character modified by shift/caps lock/control/alt */
#define	KALPHA(u, s, c, cs, a, as, ac, acs) \
	{ ALPHA, { u, s, c, cs, a, as, ac, acs }}

/* alphabetic character modified by shift/caps lock/control; alt is "meta" */
#define	KMALPHA(u, s, c, cs) \
	{ ALPHA, { u, s, c, cs, (u)|0x80, (s)|0x80, (c)|0x80, (cs)|0x80 }}

/* character unmodified by shift/control/alt */
#define	KCONST(v)	{ CHAR, { v, v, v, v, v, v, v, v }}

/* function key, constant string */
#define	KFUNC(string)	{ FUNC, { string }}

/* numeric keypad key, constant string as 3 chars (could be up to 6 plus nul */
#define	KNUMPAD(c, s1, s2, s3)	{ NUMPAD, { c, s1, s2, s3 }}

/*
 * Accents: when a key is entered that may be accented (action ACCENTED),
 * its entry gives us an index into the accented key table.
 * In that table, each key has a group of key entries,
 * indexed by the current accent value (0 is unaccented).
 */

/*
 * accent key modified by shift/control/alt;
 * value is offset from base entry in accenttab if value is <= maxaccent,
 * otherwise value is a normal character.
 */
#define	KACCENT(u, s, c, cs, a, as, ac, acs) \
	{ ACCENT, { u, s, c, cs, a, as, ac, acs }}

/* character that may be accented; gives starting index into accent table */
#define	KACCENTED(i)	{ ACCENTED, { i, 0, 0, 0, 0, 0, 0, 0 }}

/* shiftable function key modified by shift/caps lock/control/alt */
#define	KSFUNC(u, s, c, cs, a, as, ac, acs) \
	{ SFUNC, { u, s, c, cs, a, as, ac, acs }}
