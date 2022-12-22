static char rcsid[] = "@(#)Id: ndbz.c,v 5.11 1993/08/23 02:46:51 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.2 $   $State: Exp $
 *
 *			Copyright (c) 1988-1992 USENET Community Trust
 *			Copyright (c) 1986,1987 Dave Taylor
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein, Elm Coordinator
 *	elm@DSI.COM			dsinc!elm
 *
 *******************************************************************************
 * $Log: ndbz.c,v $
 * Revision 1.2  1994/01/14  00:53:33  cp
 * 2.4.23
 *
 * Revision 5.11  1993/08/23  02:46:51  syd
 * Test ANSI_C, not __STDC__ (which is not set on e.g. AIX).
 * From: decwrl!uunet.UU.NET!fin!chip (Chip Salzenberg)
 *
 * Revision 5.10  1993/08/03  19:28:39  syd
 * Elm tries to replace the system toupper() and tolower() on current
 * BSD systems, which is unnecessary.  Even worse, the replacements
 * collide during linking with routines in isctype.o.  This patch adds
 * a Configure test to determine whether replacements are really needed
 * (BROKE_CTYPE definition).  The <ctype.h> header file is now included
 * globally through hdrs/defs.h and the BROKE_CTYPE patchup is handled
 * there.  Inclusion of <ctype.h> was removed from *all* the individual
 * files, and the toupper() and tolower() routines in lib/opt_utils.c
 * were dropped.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.9  1993/08/03  19:22:01  syd
 * Add proper casts to free() calls to suppress warnings.
 * Properly cast return types.
 * From: chip@chinacat.unicom.com (Chip Rosenthal)
 *
 * Revision 5.8  1993/07/20  02:59:53  syd
 * Support aliases both on 64 bit and 32 bit machines at the same time.
 * From: Dan Mosedale <mosedale@genome.stanford.edu>
 *
 * Revision 5.7  1993/06/09  23:41:34  syd
 * Add frees of db structure on failed dbz_open
 * From: Syd
 *
 * Revision 5.6  1993/02/08  00:18:11  syd
 * fix taghere to be || instead of | and paren to make
 * it catch duplicates again, as per testing.
 * From: Syd and Chip Rosenthal
 *
 * Revision 5.5  1993/02/03  15:26:13  syd
 * protect atol in ifndef __STDC__ as some make it a macro, and its in stdlib.h
 *
 * Revision 5.4  1993/01/27  20:40:10  syd
 * make code match cnews dbz, remove extra | taghere in search,
 * that was causing a compiler error anyway, as improper code
 * From: Syd, via prompt from Denis Lambot
 *
 * Revision 5.3  1992/12/12  01:29:26  syd
 * Fix double inclusion of sys/types.h
 * From: Tom Moore <tmoore@wnas.DaytonOH.NCR.COM>
 *
 * Revision 5.2  1992/10/11  01:46:35  syd
 * change dbm name to dbz to avoid conflicts with partial call
 * ins from shared librarys, and from mixing code with yp code.
 * From: Syd via prompt from Jess Anderson
 *
 * Revision 5.1  1992/10/03  22:41:36  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** 
	multi-database dbm replacement

**/

/*

ndbz.c  V1.0
Syd Weinstein <syd@dsi.com>
Based on dbz.c from the C News distribution
Modified to support multiple DBZ files.

Copyright 1988 Jon Zeeff (zeeff@b-tech.ann-arbor.mi.us)
You can use this code in any manner, as long as you leave my name on it
and don't hold me responsible for any problems with it.

Hacked on by gdb@ninja.UUCP (David Butler); Sun Jun  5 00:27:08 CDT 1988

Various improvments + INCORE by moraes@ai.toronto.edu (Mark Moraes)

Major reworking by Henry Spencer as part of the C News project.

These routines replace dbm as used by the usenet news software
(it's not a full dbm replacement by any means).  It's fast and
simple.  It contains no AT&T code.

In general, dbz's files are 1/20 the size of dbm's.  Lookup performance
is somewhat better, while file creation is spectacularly faster, especially
if the incore facility is used.

*/

#include "headers.h"
#include <errno.h>
#ifndef ANSI_C
extern int errno;
#endif

/*
 * #ifdef index.  "LIA" = "leave it alone unless you know what you're doing".
 *
 * FUNNYSEEKS	SEEK_SET is not 0, get it from <unistd.h>
 * INDEX_SIZE	backward compatibility with old dbz; avoid using this
 * NMEMORY	number of days of memory for use in sizing new table (LIA)
 * INCORE	backward compatibility with old dbz; use dbzincore() instead
 * DEFSIZE	default table size (not as critical as in old dbz)
 * NOTAGS	fseek offsets are strange, do not do tagging (see below)
 * NPAGBUF	size of .pag buffer, in longs (LIA)
 * SHISTBUF	size of ASCII-file buffer, in bytes (LIA)
 * MAXRUN	length of run which shifts to next table (see below) (LIA)
 * OVERFLOW	long-int arithmetic overflow must be avoided, will trap
 * NOBUFFER	do not buffer hash-table i/o, B News locking is defective
 */

#ifdef FUNNYSEEKS
#include <unistd.h>
#else
#define	SEEK_SET	0
#endif
#ifdef OVERFLOW
#include <limits.h>
#endif

static int dbzversion = 3;	/* for validating .dir file format */

/*
 * The dbz database exploits the fact that when news stores a <key,value>
 * tuple, the `value' part is a seek offset into a text file, pointing to
 * a copy of the `key' part.  This avoids the need to store a copy of
 * the key in the dbz files.  However, the text file *must* exist and be
 * consistent with the dbz files, or things will fail.
 *
 * The basic format of the database is a simple hash table containing the
 * values.  A value is stored by indexing into the table using a hash value
 * computed from the key; collisions are resolved by linear probing (just
 * search forward for an empty slot, wrapping around to the beginning of
 * the table if necessary).  Linear probing is a performance disaster when
 * the table starts to get full, so a complication is introduced.  The
 * database is actually one *or more* tables, stored sequentially in the
 * .pag file, and the length of linear-probe sequences is limited.  The
 * search (for an existing item or an empty slot) always starts in the
 * first table, and whenever MAXRUN probes have been done in table N,
 * probing continues in table N+1.  This behaves reasonably well even in
 * cases of massive overflow.  There are some other small complications
 * added, see comments below.
 *
 * The table size is fixed for any particular database, but is determined
 * dynamically when a database is rebuilt.  The strategy is to try to pick
 * the size so the first table will be no more than 2/3 full, that being
 * slightly before the point where performance starts to degrade.  (It is
 * desirable to be a bit conservative because the overflow strategy tends
 * to produce files with holes in them, which is a nuisance.)
 */

/*
 * The following is for backward compatibility.
 */
#ifdef INDEX_SIZE
#define	DEFSIZE	INDEX_SIZE
#endif
#include "ndbz.h"

/*
 * We assume that unused areas of a binary file are zeros, and that the
 * bit pattern of `(of_t)0' is all zeros.  The alternative is rather
 * painful file initialization.  Note that okayvalue(), if OVERFLOW is
 * defined, knows what value of an offset would cause overflow.
 */
#define	VACANT		((of_t)0)
#define	BIAS(o)		((o)+1)		/* make any valid of_t non-VACANT */
#define	UNBIAS(o)	((o)-1)		/* reverse BIAS() effect */

/*
 * In a Unix implementation, or indeed any in which an of_t is a byte
 * count, there are a bunch of high bits free in an of_t.  There is a
 * use for them.  Checking a possible hit by looking it up in the base
 * file is relatively expensive, and the cost can be dramatically reduced
 * by using some of those high bits to tag the value with a few more bits
 * of the key's hash.  This detects most false hits without the overhead of
 * seek+read+strcmp.  We use the top bit to indicate whether the value is
 * tagged or not, and don't tag a value which is using the tag bits itself.
 * We're in trouble if the of_t representation wants to use the top bit.
 * The actual bitmasks and offset come from the configuration stuff,
 * which permits fiddling with them as necessary, and also suppressing
 * them completely (by defining the masks to 0).  We build pre-shifted
 * versions of the masks for efficiency.
 */
#define	HASTAG(o)	((o)&db->dbz_taghere)
#define	TAG(o)		((o)&db->dbz_tagbits)
#define	NOTAG(o)	((o)&~db->dbz_tagboth)
#define	CANTAG(o)	(((o)&db->dbz_tagboth) == 0)
#define	MKTAG(v)	(((v)<<db->dbz_conf.tagshift)&db->dbz_tagbits)

/*
 * A new, from-scratch database, not built as a rebuild of an old one,
 * needs to know table size and tagging.  Normally
 * the user supplies this info, but there have to be defaults.
 */
#ifndef DEFSIZE
#define	DEFSIZE	120011		/* 300007 might be better */
#endif
#ifndef NOTAGS
#define	TAGENB	0x80		/* tag enable is top bit, tag is next 7 */
#define	TAGMASK	0x7f
#define	TAGSHIFT	24
#else
#define	TAGENB	0		/* no tags */
#define	TAGMASK	0
#define	TAGSHIFT	0
#endif

static int getconf();
static int32 getno();
static int putconf();
static void mybytemap();
static of_t bytemap();

/* 
 * For a program that makes many, many references to the database, it
 * is a large performance win to keep the table in core, if it will fit.
 * Note that this does hurt robustness in the event of crashes, and
 * dbz_close() *must* be called to flush the in-core database to disk.
 * The code is prepared to deal with the possibility that there isn't
 * enough memory.  There *is* an assumption that a size_t is big enough
 * to hold the size (in bytes) of one table, so dbz_open() tries to figure
 * out whether this is possible first.
 *
 * The preferred way to ask for an in-core table is to do dbzincore(1)
 * before dbz_open().  The default is not to do it, although -DINCORE
 * overrides this for backward compatibility with old dbz.
 *
 * We keep only the first table in core.  This greatly simplifies the
 * code, and bounds memory demand.  Furthermore, doing this is a large
 * performance win even in the event of massive overflow.
 */
#ifdef INCORE
static int default_incore = 1;
#else
static int default_incore = 0;
#endif

#		ifndef MAXRUN
#		define	MAXRUN	100
#		endif
static void start();
#define	FRESH	((struct searcher *)NULL)
static of_t search();
#define	NOTFOUND	((of_t)-1)
static int okayvalue();
static int set();

/*
 * Arguably the searcher struct for a given routine ought to be local to
 * it, but a dbz_fetch() is very often immediately followed by a dbz_store(), and
 * in some circumstances it is a useful performance win to remember where
 * the dbz_fetch() completed.  So we use a global struct and remember whether
 * it is current.
 */

/* byte-ordering stuff */
#define	MAPIN(o)	((db->dbz_bytesame) ? (o) : bytemap((o), db->dbz_conf.bytemap, db->dbz_mybmap))
#define	MAPOUT(o)	((db->dbz_bytesame) ? (o) : bytemap((o), db->dbz_mybmap, db->dbz_conf.bytemap))

/* externals used */
#if !ANSI_C  /* avoid problems with systems that declare atol as a macro */
extern long atol();
#endif

/* misc. forwards */
static long hash();
static void crcinit();
static int isprime();
static FILE *latebase();

/* file-naming stuff */
static char dir[] = ".dir";
static char pag[] = ".pag";
static char *enstring();

/* central data structures */
static of_t *getcore();
static int putcore();

/*
 - dbz_fresh - set up a new database, no historical info
 */
DBZ *				/* NULL for failure, !NULL for success */
dbz_fresh(name, size, fs, tagmask)
char *name;			/* base name; .dir and .pag must exist */
long size;			/* table size (0 means default) */
int fs;				/* field-separator character in base file */
of_t tagmask;			/* 0 default, 1 no tags */
{
	register char *fn;
	struct dbzconfig c;
	register of_t m;
	register FILE *f;

	if (size != 0 && size < 2) {
		dprint(5, (debugfile, "dbz_fresh: preposterous size (%ld)\n", size));
		return (DBZ *)NULL;
	}

	/* get default configuration */
	if (getconf((FILE *)NULL, (FILE *)NULL, &c) < 0)
		return (DBZ *)NULL; /* "can't happen" */

	/* and mess with it as specified */
	if (size != 0)
		c.tsize = size;
	c.fieldsep = fs;
	switch (tagmask) {
	case 0:			/* default */
		break;
	case 1:			/* no tags */
		c.tagshift = 0;
		c.tagmask = 0;
		c.tagenb = 0;
		break;
	default:
		m = tagmask;
		c.tagshift = 0;
		while (!(m&01)) {
			m >>= 1;
			c.tagshift++;
		}
		c.tagmask = m;
		c.tagenb = (m << 1) & ~m;
		break;
	}

	/* write it out */
	fn = enstring(name, dir);
	if (fn == NULL)
		return (DBZ *)NULL;
	f = fopen(fn, "w");
	free((malloc_t)fn);
	if (f == NULL) {
		dprint(5, (debugfile, "dbz_fresh: unable to write config\n"));
		return (DBZ *)NULL;
	}
	if (putconf(f, &c) < 0) {
		(void) fclose(f);
		return (DBZ *)NULL;
	}
	if (fclose(f) == EOF) {
		dprint(5, (debugfile, "dbz_fresh: fclose failure\n"));
		return (DBZ *)NULL;
	}

	/* create/truncate .pag */
	fn = enstring(name, pag);
	if (fn == NULL)
		return (DBZ *)NULL;
	f = fopen(fn, "w");
	free((malloc_t)fn);
	if (f == NULL) {
		dprint(5, (debugfile, "dbz_fresh: unable to create/truncate .pag file\n"));
		return (DBZ *)NULL;
	} else
		(void) fclose(f);

	/* and punt to dbz_open for the hard work */
	return(dbz_open(name, O_RDWR, 0));
}

/*
 - dbz_size - what's a good table size to hold this many entries?
 */
long
dbzsize(contents)
long contents;			/* 0 means what's the default */
{
	register long n;

	if (contents <= 0) {	/* foulup or default inquiry */
		dprint(5, (debugfile, "dbzsize: preposterous input (%ld)\n", contents));
		return(DEFSIZE);
	}
	n = (contents/2)*3;	/* try to keep table at most 2/3 full */
	if (!(n&01))		/* make it odd */
		n++;
	dprint(5, (debugfile, "dbzsize: tentative size %ld\n", n));
	while (!isprime(n))	/* and look for a prime */
		n += 2;
	dprint(5, (debugfile, "dbzsize: final size %ld\n", n));

	return(n);
}

/*
 - isprime - is a number prime?
 *
 * This is not a terribly efficient approach.
 */
static int			/* predicate */
isprime(x)
register long x;
{
	static int quick[] = { 2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 0 };
	register int *ip;
	register long div;
	register long stop;

	/* hit the first few primes quickly to eliminate easy ones */
	/* this incidentally prevents ridiculously small tables */
	for (ip = quick; (div = *ip) != 0; ip++)
		if (x%div == 0) {
			dprint(5, (debugfile, "isprime: quick result on %ld\n", (long)x));
			return(0);
		}

	/* approximate square root of x */
	for (stop = x; x/stop < stop; stop >>= 1)
		continue;
	stop <<= 1;

	/* try odd numbers up to stop */
	for (div = *--ip; div < stop; div += 2)
		if (x%div == 0)
			return(0);

	return(1);
}

/*
 - dbz_again - set up a new database to be a rebuild of an old one
 */
DBZ *				/* NULL for failure, !NULL for success */
dbz_again(name, oldname)
char *name;			/* base name; .dir and .pag must exist */
char *oldname;			/* base name; all must exist */
{
	register char *fn;
	struct dbzconfig c;
	register int i;
	register long top;
	register FILE *f;
	register int newtable;
	register of_t newsize;

	/* pick up the old configuration */
	fn = enstring(oldname, dir);
	if (fn == NULL)
		return (DBZ *)NULL;
	f = fopen(fn, "r");
	free((malloc_t)fn);
	if (f == NULL) {
		dprint(5, (debugfile, "dbz_again: cannot open old .dir file\n"));
		return (DBZ *)NULL;
	}
	i = getconf(f, (FILE *)NULL, &c);
	(void) fclose(f);
	if (i < 0) {
		dprint(5, (debugfile, "dbz_again: getconf failed\n"));
		return (DBZ *)NULL;
	}

	/* tinker with it */
	top = 0;
	newtable = 0;
	for (i = 0; i < NUSEDS; i++) {
		if (top < c.used[i])
			top = c.used[i];
		if (c.used[i] == 0)
			newtable = 1;	/* hasn't got full usage history yet */
	}
	if (top == 0) {
		dprint(5, (debugfile, "dbz_again: old table has no contents!\n"));
		newtable = 1;
	}
	for (i = NUSEDS-1; i > 0; i--)
		c.used[i] = c.used[i-1];
	c.used[0] = 0;
	newsize = dbzsize(top);
	if (!newtable || newsize > c.tsize)	/* don't shrink new table */
		c.tsize = newsize;

	/* write it out */
	fn = enstring(name, dir);
	if (fn == NULL)
		return (DBZ *)NULL;
	f = fopen(fn, "w");
	free((malloc_t)fn);
	if (f == NULL) {
		dprint(5, (debugfile, "dbz_again: unable to write new .dir\n"));
		return (DBZ *)NULL;
	}
	i = putconf(f, &c);
	(void) fclose(f);
	if (i < 0) {
		dprint(5, (debugfile, "dbz_again: putconf failed\n"));
		return (DBZ *)NULL;
	}

	/* create/truncate .pag */
	fn = enstring(name, pag);
	if (fn == NULL)
		return (DBZ *)NULL;
	f = fopen(fn, "w");
	free((malloc_t)fn);
	if (f == NULL) {
		dprint(5, (debugfile, "dbz_again: unable to create/truncate .pag file\n"));
		return (DBZ *)NULL;
	} else
		(void) fclose(f);

	/* and let dbz_open do the work */
	return(dbz_open(name, O_RDWR, 0));
}

/*
 - dbz_open - open a database, creating it (using defaults) if necessary
 *
 * We try to leave errno set plausibly, to the extent that underlying
 * functions permit this, since many people consult it if dbz_open() fails.
 */
DBZ *				/* NULL for failure, !NULL for success */
dbz_open(name, mode, flags)
char *name;
int mode, flags;
{
	register int i;
	register size_t s;
	register DBZ  *db;
	register char *dirfname;
	register char *pagfname;

	if ((db = (DBZ *) calloc(sizeof(DBZ), 1)) == NULL) {
		dprint(5, (debugfile, "dbz_open: no room for DBZ structure\n"));
		return (DBZ *)NULL;
	}
	/* open the .dir file */
	dirfname = enstring(name, dir);
	if (dirfname == NULL) {
		free((malloc_t)db);
		return (DBZ *)NULL;
	}

	if (mode == O_RDONLY) {
		db->dbz_dirf = fopen(dirfname, "r");
		db->dbz_dirronly = 1;
	} else
		db->dbz_dirf = fopen(dirfname, "r+");
	free((malloc_t)dirfname);

	if (db->dbz_dirf == NULL) {
		dprint(5, (debugfile, "dbz_open: can't open .dir file\n"));
		free((malloc_t)db);
		return (DBZ *)NULL;
	}

	/* open the .pag file */
	pagfname = enstring(name, pag);
	if (pagfname == NULL) {
		(void) fclose(db->dbz_dirf);
		free((malloc_t)db);
		return (DBZ *)NULL;
	}
	if (mode == O_RDONLY) {
		db->dbz_pagf = fopen(pagfname, "rb");
		db->dbz_pagronly = 1;
	} else
		db->dbz_pagf = fopen(pagfname, "r+b");

	if (db->dbz_pagf == NULL) {
		dprint(5, (debugfile, "dbz_open: .pag open failed\n"));
		(void) fclose(db->dbz_dirf);
		free((malloc_t)pagfname);
		free((malloc_t)db);
		return (DBZ *)NULL;
	}
#ifdef NOBUFFER
	/*
	 * B News does not do adequate locking on its database accesses.
	 * Why it doesn't get into trouble using dbm is a mystery.  In any
	 * case, doing unbuffered i/o does not cure the problem, but does
	 * enormously reduce its incidence.
	 */
	(void) setbuf(db->dbz_pagf, (char *)NULL);
#else
#ifdef _IOFBF
	(void) setvbuf(db->dbz_pagf, (char *)db->dbz_pagbuf, _IOFBF, sizeof(db->dbz_pagbuf));
#endif
#endif
	db->dbz_pagpos = -1;
	/* don't free pagfname, need it below */

	/* open the base file */
	db->dbz_basef = fopen(name, "r");
	if (db->dbz_basef == NULL) {
		dprint(5, (debugfile, "dbz_open: basefile open failed\n"));
		db->dbz_basefname = enstring(name, "");
		if (db->dbz_basefname == NULL) {
			(void) fclose(db->dbz_pagf);
			(void) fclose(db->dbz_dirf);
			free((malloc_t)pagfname);
			free((malloc_t)db);
			return (DBZ *)NULL;
		}
	} else
		db->dbz_basefname = NULL;
#ifdef _IOFBF
	if (db->dbz_basef != NULL)
		(void) setvbuf(db->dbz_basef, db->dbz_basebuf, _IOFBF, sizeof(db->dbz_basebuf));
#endif

	/* pick up configuration */
	if (getconf(db->dbz_dirf, db->dbz_pagf, &db->dbz_conf) < 0) {
		dprint(5, (debugfile, "dbz_open: getconf failure\n"));
		(void) fclose(db->dbz_basef);
		(void) fclose(db->dbz_pagf);
		(void) fclose(db->dbz_basef);
		(void) fclose(db->dbz_dirf);
		free((malloc_t)pagfname);
		free((malloc_t)db);
		errno = EDOM;	/* kind of a kludge, but very portable */
		return (DBZ *)NULL;
	}
	db->dbz_tagbits = db->dbz_conf.tagmask << db->dbz_conf.tagshift;
	db->dbz_taghere = db->dbz_conf.tagenb << db->dbz_conf.tagshift;
	db->dbz_tagboth = db->dbz_tagbits | db->dbz_taghere;
	mybytemap(db->dbz_mybmap);
	db->dbz_bytesame = 1;
	for (i = 0; i < SOF; i++)
		if (db->dbz_mybmap[i] != db->dbz_conf.bytemap[i])
			db->dbz_bytesame = 0;

	/* get first table into core, if it looks desirable and feasible */
	s = (size_t)db->dbz_conf.tsize * SOF;
	db->dbz_incore = default_incore;
	if (db->dbz_incore && (of_t)(s/SOF) == db->dbz_conf.tsize) {
		db->dbz_bufpagf = fopen(pagfname, (db->dbz_pagronly) ? "rb" : "r+b");
		if (db->dbz_bufpagf != NULL)
			db->dbz_corepag = getcore(db);
	} else {
		db->dbz_bufpagf = NULL;
		db->dbz_corepag = NULL;
	}
	free((malloc_t)pagfname);

	/* misc. setup */
	crcinit();
	db->dbz_written = 0;
	db->dbz_prevp = FRESH;
	dprint(5, (debugfile, "dbz_open: succeeded\n"));
	return(db);
}

/*
 - enstring - concatenate two strings into a malloced area
 */
static char *			/* NULL if malloc fails */
enstring(s1, s2)
char *s1;
char *s2;
{
	register char *p;

	p = malloc((size_t)strlen(s1) + (size_t)strlen(s2) + 1);
	if (p != NULL) {
		(void) strcpy(p, s1);
		(void) strcat(p, s2);
	} else {
		dprint(5, (debugfile, "enstring(%s, %s) out of memory\n", s1, s2));
	}
	return(p);
}

/*
 - dbz_close - close a database
 */
int
dbz_close(db)
register DBZ *db;
{
	register int ret = 0;

	if (db->dbz_pagf == NULL) {
		dprint(5, (debugfile, "dbz_close: not opened!\n"));
		return(-1);
	}

	if (fclose(db->dbz_pagf) == EOF) {
		dprint(5, (debugfile, "dbz_close: fclose(pagf) failed\n"));
		ret = -1;
	}
	if (dbz_sync(db) < 0)
		ret = -1;
	if (db->dbz_bufpagf != NULL && fclose(db->dbz_bufpagf) == EOF) {
		dprint(5, (debugfile, "dbz_close: fclose(bufpagf) failed\n"));
		ret = -1;
	}
	if (db->dbz_corepag != NULL)
		free((malloc_t)db->dbz_corepag);
	db->dbz_corepag = NULL;
	if (fclose(db->dbz_basef) == EOF) {
		dprint(5, (debugfile, "dbz_close: fclose(basef) failed\n"));
		ret = -1;
	}
	if (db->dbz_basefname != NULL)
		free((malloc_t)db->dbz_basefname);
	db->dbz_basef = NULL;
	db->dbz_pagf = NULL;
	if (fclose(db->dbz_dirf) == EOF) {
		dprint(5, (debugfile, "dbz_close: fclose(dirf) failed\n"));
		ret = -1;
	}

	free((malloc_t) db);

	dprint(5, (debugfile, "dbz_close: %s\n", (ret == 0) ? "succeeded" : "failed"));
	return(ret);
}

/*
 - dbz_sync - push all in-core data out to disk
 */
int
dbz_sync(db)
register DBZ *db;
{
	register int ret = 0;

	if (db->dbz_pagf == NULL) {
		dprint(5, (debugfile, "dbzsync: not opened!\n"));
		return(-1);
	}
	if (!db->dbz_written)
		return(0);

	if (db->dbz_corepag != NULL) {
		if (putcore(db) < 0) {
			dprint(5, (debugfile, "dbzsync: putcore failed\n"));
			ret = -1;
		}
	}
	if (!db->dbz_conf.olddbz)
		if (putconf(db->dbz_dirf, &db->dbz_conf) < 0)
			ret = -1;

	dprint(5, (debugfile, "dbzsync: %s\n", (ret == 0) ? "succeeded" : "failed"));
	return(ret);
}

/*
 - dbzcancel - cancel writing of in-core data
 * Mostly for use from child processes.
 * Note that we don't need to futz around with stdio buffers, because we
 * always fflush them immediately anyway and so they never have stale data.
 */
int
dbz_cancel(db)
register DBZ *db;
{
	if (db->dbz_pagf == NULL) {
		dprint(5, (debugfile, "dbz_cancel: not opened!\n"));
		return(-1);
	}

	db->dbz_written = 0;
	return(0);
}

/*
 - dbz_fetch - get an entry from the database
 *
 * Disgusting fine point, in the name of backward compatibility:  if the
 * last character of "key" is a NUL, that character is (effectively) not
 * part of the comparison against the stored keys.
 */
datum				/* dptr NULL, dsize 0 means failure */
dbz_fetch(db, key)
register DBZ *db;
datum key;
{
	char buffer[DBZMAXKEY + 1];
	static of_t key_ptr;		/* return value points here */
	datum output;
	register size_t keysize;
	register size_t cmplen;
	register char *sepp;

	dprint(5, (debugfile, "dbz_fetch: (%s)\n", key.dptr));
	output.dptr = NULL;
	output.dsize = 0;
	db->dbz_prevp = FRESH;

	/* Key is supposed to be less than DBZMAXKEY */
	keysize = key.dsize;
	if (keysize >= DBZMAXKEY) {
		keysize = DBZMAXKEY;
		dprint(5, (debugfile, "keysize is %d - truncated to %d\n", key.dsize, DBZMAXKEY));
	}

	if (db->dbz_pagf == NULL) {
		dprint(5, (debugfile, "dbz_fetch: database not open!\n"));
		return(output);
	} else if (db->dbz_basef == NULL) {	/* basef didn't exist yet */
		db->dbz_basef = latebase(db);
		if (db->dbz_basef == NULL)
			return(output);
	}

	cmplen = keysize;
	sepp = &db->dbz_conf.fieldsep;
	if (key.dptr[keysize-1] == '\0') {
		cmplen--;
		sepp = &buffer[keysize-1];
	}
	start(db, &key, FRESH);
	while ((key_ptr = search(db)) != NOTFOUND) {
		dprint(5, (debugfile, "got 0x%lx\n", key_ptr));

		/* fetch the key */
		if (fseek(db->dbz_basef, key_ptr, SEEK_SET) != 0) {
			dprint(5, (debugfile, "dbz_fetch: seek failed\n"));
			return(output);
		}
		if (fread(buffer, 1, keysize, db->dbz_basef) != keysize) {
			dprint(5, (debugfile, "dbz_fetch: read failed\n"));
			return(output);
		}

		/* try it */
		buffer[keysize] = '\0';		/* terminated for DEBUG */
		dprint(5, (debugfile, "dbz_fetch: buffer (%s) looking for (%s) size = %d\n", 
						buffer, key.dptr, keysize));
		if (bcmp(key.dptr, buffer, cmplen) == 0 &&
				(*sepp == db->dbz_conf.fieldsep || *sepp == '\0')) {
			/* we found it */
			output.dptr = (char *)&key_ptr;
			output.dsize = SOF;
			dprint(5, (debugfile, "dbz_fetch: successful\n"));
			return(output);
		}
	}

	/* we didn't find it */
	dprint(5, (debugfile, "dbz_fetch: failed\n"));
	db->dbz_prevp = &db->dbz_srch;			/* remember where we stopped */
	return(output);
}

/*
 - latebase - try to open a base file that wasn't there at the start
 */
static FILE *
latebase(db)
register DBZ *db;
{
	register FILE *it;

	if (db->dbz_basefname == NULL) {
		dprint(5, (debugfile, "latebase: name foulup\n"));
		return (FILE *)NULL;
	}
	it = fopen(db->dbz_basefname, "r");
	if (it == NULL) {
		dprint(5, (debugfile, "latebase: still can't open base\n"));
	} else {
		dprint(5, (debugfile, "latebase: late open succeeded\n"));
		free((malloc_t)db->dbz_basefname);
		db->dbz_basefname = NULL;
#ifdef _IOFBF
		(void) setvbuf(it, db->dbz_basebuf, _IOFBF, sizeof(db->dbz_basebuf));
#endif
	}
	return(it);
}

/*
 - dbz_store - add an entry to the database
 */
int				/* 0 success, -1 failure */
dbz_store(db, key, data)
register DBZ *db;
datum key;
datum data;
{
	of_t value;

	if (db->dbz_pagf == NULL) {
		dprint(5, (debugfile, "dbz_store: database not open!\n"));
		return(-1);
	} else if (db->dbz_basef == NULL) {	/* basef didn't exist yet */
		db->dbz_basef = latebase(db);
		if (db->dbz_basef == NULL)
			return(-1);
	}
	if (db->dbz_pagronly) {
		dprint(5, (debugfile, "dbz_store: database open read-only\n"));
		return(-1);
	}
	if (data.dsize != SOF) {
		dprint(5, (debugfile, "dbz_store: value size wrong (%d)\n", data.dsize));
		return(-1);
	}
	if (key.dsize >= DBZMAXKEY) {
		dprint(5, (debugfile, "dbz_store: key size too big (%d)\n", key.dsize));
		return(-1);
	}

	/* copy the value in to ensure alignment */
	(void) bcopy(data.dptr, (char *)&value, SOF);
	dprint(5, (debugfile, "dbz_store: (%s, %ld)\n", key.dptr, (long)value));
	if (!okayvalue(db, value)) {
		dprint(5, (debugfile, "dbz_store: reserved bit or overflow in 0x%lx\n", value));
		return(-1);
	}

	/* find the place, exploiting previous search if possible */
	start(db, &key, db->dbz_prevp);
	while (search(db) != NOTFOUND)
		continue;

	db->dbz_prevp = FRESH;
	db->dbz_conf.used[0]++;
	dprint(5, (debugfile, "dbz_store: used count %ld\n", db->dbz_conf.used[0]));
	db->dbz_written = 1;
	return(set(db, value));
}

/*
 - dbz_incore - control attempts to keep .pag file in core
 */
int				/* old setting */
dbz_incore(value)
int value;
{
	register int old = default_incore;

	default_incore = value;
	return(old);
}

/*
 - getconf - get configuration from .dir file
 */
static int			/* 0 success, -1 failure */
getconf(df, pf, cp)
register FILE *df;		/* NULL means just give me the default */
register FILE *pf;		/* NULL means don't care about .pag */
register struct dbzconfig *cp;
{
	register int c;
	register int i;
	int err = 0;

	c = (df != NULL) ? getc(df) : EOF;
	if (c == EOF) {		/* empty file, no configuration known */
		cp->olddbz = 0;
		if (df != NULL && pf != NULL && getc(pf) != EOF)
			cp->olddbz = 1;
		cp->tsize = DEFSIZE;
		cp->fieldsep = '\t';
		for (i = 0; i < NUSEDS; i++)
			cp->used[i] = 0;
		cp->valuesize = SOF;
		mybytemap(cp->bytemap);
		cp->tagenb = TAGENB;
		cp->tagmask = TAGMASK;
		cp->tagshift = TAGSHIFT;
		dprint(5, (debugfile, "getconf: defaults (%ld, (0x%lx/0x%lx<<%d))\n",
			cp->tsize, cp->tagenb, cp->tagmask, cp->tagshift));
		return(0);
	}
	(void) ungetc(c, df);

	/* first line, the vital stuff */
	if (getc(df) != 'd' || getc(df) != 'b' || getc(df) != 'z')
		err = -1;
	if (getno(df, &err) != dbzversion)
		err = -1;
	cp->tsize = getno(df, &err);
	cp->fieldsep = getno(df, &err);
	while ((c = getc(df)) == ' ')
		continue;
	cp->tagenb = getno(df, &err);
	cp->tagmask = getno(df, &err);
	cp->tagshift = getno(df, &err);
	cp->valuesize = getno(df, &err);
	if (cp->valuesize != SOF) {
		dprint(5, (debugfile, "getconf: wrong of_t size (%d)\n", cp->valuesize));
		err = -1;
		cp->valuesize = SOF;	/* to protect the loops below */
	}
	for (i = 0; i < cp->valuesize; i++)
		cp->bytemap[i] = getno(df, &err);
	if (getc(df) != '\n')
		err = -1;
	dprint(5, (debugfile, "size %ld, sep %d, tags 0x%lx/0x%lx<<%d, ", cp->tsize,
			cp->fieldsep, cp->tagenb, cp->tagmask, cp->tagshift));
	dprint(5, (debugfile, "bytemap (%d)", cp->valuesize));
	for (i = 0; i < cp->valuesize; i++) {
		dprint(5, (debugfile, " %d", cp->bytemap[i]));
	}
	dprint(5, (debugfile, "\n"));

	/* second line, the usages */
	for (i = 0; i < NUSEDS; i++)
		cp->used[i] = getno(df, &err);
	if (getc(df) != '\n')
		err = -1;
	dprint(5, (debugfile, "used %ld %ld %ld...\n", cp->used[0], cp->used[1], cp->used[2]));

	if (err < 0) {
		dprint(5, (debugfile, "getconf error\n"));
		return(-1);
	}
	return(0);
}

/*
 - getno - get an int32
 */
static int32
getno(f, ep)
FILE *f;
int *ep;
{
	register char *p;
#	define	MAXN	50
	char getbuf[MAXN];
	register int c;

	while ((c = getc(f)) == ' ')
		continue;
	if (c == EOF || c == '\n') {
		dprint(5, (debugfile, "getno: missing number\n"));
		*ep = -1;
		return(0);
	}
	p = getbuf;
	*p++ = c;
	while ((c = getc(f)) != EOF && c != '\n' && c != ' ')
		if (p < &getbuf[MAXN-1])
			*p++ = c;
	if (c == EOF) {
		dprint(5, (debugfile, "getno: EOF\n"));
		*ep = -1;
	} else
		(void) ungetc(c, f);
	*p = '\0';

	if (strspn(getbuf, "-1234567890") != strlen(getbuf)) {
		dprint(5, (debugfile, "getno: `%s' non-numeric\n", getbuf));
		*ep = -1;
	}

	return((int32)atol(getbuf));
}

/*
 - putconf - write configuration to .dir file
 */
static int			/* 0 success, -1 failure */
putconf(f, cp)
register FILE *f;
register struct dbzconfig *cp;
{
	register int i;
	register int ret = 0;

	if (fseek(f, (of_t)0, SEEK_SET) != 0) {
		dprint(5, (debugfile, "fseek failure in putconf\n"));
		ret = -1;
	}
	fprintf(f, "dbz %d %ld %d %ld %ld %d %d", dbzversion, cp->tsize,
				cp->fieldsep, cp->tagenb,
				cp->tagmask, cp->tagshift, cp->valuesize);
	for (i = 0; i < cp->valuesize; i++)
		fprintf(f, " %d", cp->bytemap[i]);
	fprintf(f, "\n");
	for (i = 0; i < NUSEDS; i++)
		fprintf(f, "%ld%c", cp->used[i], (i < NUSEDS-1) ? ' ' : '\n');

	(void) fflush(f);
	if (ferror(f))
		ret = -1;

	dprint(5, (debugfile, "putconf status %d\n", ret));
	return(ret);
}

/*
 - getcore - try to set up an in-core copy of .pag file
 */
static of_t *			/* pointer to copy, or NULL */
getcore(db)
register DBZ *db;
{
	register of_t *p;
	register size_t i;
	register size_t nread;
	register char *it;

	it = malloc((size_t)db->dbz_conf.tsize * SOF);
	if (it == NULL) {
		dprint(5, (debugfile, "getcore: malloc failed\n"));
		return (of_t *)NULL;
	}

	nread = fread(it, SOF, (size_t)db->dbz_conf.tsize, db->dbz_bufpagf);
	if (ferror(db->dbz_bufpagf)) {
		dprint(5, (debugfile, "getcore: read failed\n"));
		free((malloc_t)it);
		return (of_t *)NULL;
	}

	p = (of_t *)it + nread;
	i = (size_t)db->dbz_conf.tsize - nread;
	while (i-- > 0)
		*p++ = VACANT;
	return((of_t *)it);
}

/*
 - putcore - try to rewrite an in-core table
 */
static int			/* 0 okay, -1 fail */
putcore(db)
register DBZ *db;
{
	if (fseek(db->dbz_bufpagf, (of_t)0, SEEK_SET) != 0) {
		dprint(5, (debugfile, "fseek failure in putcore\n"));
		return(-1);
	}
	(void) fwrite((char *)db->dbz_corepag, SOF, (size_t)db->dbz_conf.tsize, db->dbz_bufpagf);
	(void) fflush(db->dbz_bufpagf);
	return((ferror(db->dbz_bufpagf)) ? -1 : 0);
}

/*
 - start - set up to start or restart a search
 */
static void
start(db, kp, osp)
register DBZ *db;
register datum *kp;
register struct searcher *osp;		/* may be FRESH, i.e. NULL */
{
	register struct searcher *sp = &db->dbz_srch;
	register long h;

	h = hash(kp->dptr, kp->dsize);
	if (osp != FRESH && osp->hash == h) {
		if (sp != osp)
			*sp = *osp;
		dprint(5, (debugfile, "search restarted\n"));
	} else {
		sp->hash = h;
		sp->tag = MKTAG(h / db->dbz_conf.tsize);
		dprint(5, (debugfile, "tag 0x%lx\n", sp->tag));
		sp->place = h % db->dbz_conf.tsize;
		sp->tabno = 0;
		sp->run = (db->dbz_conf.olddbz) ? db->dbz_conf.tsize : MAXRUN;
		sp->aborted = 0;
	}
	sp->seen = 0;
}

/*
 - search - conduct part of a search
 */
static of_t			/* NOTFOUND if we hit VACANT or error */
search(db)
register DBZ *db;
{
	register struct searcher *sp = &db->dbz_srch;
	register of_t dest;
	register of_t value;
	of_t val;		/* buffer for value (can't fread register) */
	register of_t place;

	if (sp->aborted)
		return(NOTFOUND);

	for (;;) {
		/* determine location to be examined */
		place = sp->place;
		if (sp->seen) {
			/* go to next location */
			if (--sp->run <= 0) {
				sp->tabno++;
				sp->run = MAXRUN;
			}
			place = (place+1)%db->dbz_conf.tsize + sp->tabno*db->dbz_conf.tsize;
			sp->place = place;
		} else
			sp->seen = 1;	/* now looking at current location */
		dprint(5, (debugfile, "search @ %ld\n", place));

		/* get the tagged value */
		if (db->dbz_corepag != NULL && place < db->dbz_conf.tsize) {
			dprint(5, (debugfile, "search: in core\n"));
			value = MAPIN(db->dbz_corepag[place]);
		} else {
			/* seek, if necessary */
			dest = place * SOF;
			if (db->dbz_pagpos != dest) {
				if (fseek(db->dbz_pagf, dest, SEEK_SET) != 0) {
					dprint(5, (debugfile, "search: seek failed\n"));
					db->dbz_pagpos = -1;
					sp->aborted = 1;
					return(NOTFOUND);
				}
				db->dbz_pagpos = dest;
			}

			/* read it */
			if (fread((char *)&val, sizeof(val), 1, db->dbz_pagf) == 1)
				value = MAPIN(val);
			else if (ferror(db->dbz_pagf)) {
				dprint(5, (debugfile, "search: read failed\n"));
				db->dbz_pagpos = -1;
				sp->aborted = 1;
				return(NOTFOUND);
			} else
				value = VACANT;

			/* and finish up */
			db->dbz_pagpos += sizeof(val);
		}

		/* vacant slot is always cause to return */
		if (value == VACANT) {
			dprint(5, (debugfile, "search: empty slot\n"));
			return(NOTFOUND);
		};

		/* check the tag */
		value = UNBIAS(value);
		dprint(5, (debugfile, "got 0x%lx\n", value));
		if (!HASTAG(value)) {
			dprint(5, (debugfile, "tagless\n"));
			return(value);
		} else if ((TAG(value) == sp->tag) || db->dbz_taghere) {
			dprint(5, (debugfile, "match\n"));
			return(NOTAG(value));
		} else {
			dprint(5, (debugfile, "mismatch 0x%lx\n", TAG(value)));
		}
	}
	/* NOTREACHED */
}

/*
 - okayvalue - check that a value can be stored
 */
static int			/* predicate */
okayvalue(db, value)
register DBZ *db;
of_t value;
{
	if (HASTAG(value))
		return(0);
#ifdef OVERFLOW
	if (value == LONG_MAX)	/* BIAS() and UNBIAS() will overflow */
		return(0);
#endif
	return(1);
}

/*
 - set - store a value into a location previously found by search
 */
static int			/* 0 success, -1 failure */
set(db, value)
register DBZ *db;
of_t value;
{
	register struct searcher *sp  = &db->dbz_srch;
	register of_t place = sp->place;
	register of_t v = value;

	if (sp->aborted)
		return(-1);

	if (CANTAG(v) && !db->dbz_conf.olddbz) {
		v |= sp->tag | db->dbz_taghere;
		if (v != UNBIAS(VACANT))	/* BIAS(v) won't look VACANT */
#ifdef OVERFLOW
			if (v != LONG_MAX)	/* and it won't overflow */
#endif
			value = v;
	}
	dprint(5, (debugfile, "tagged value is 0x%lx\n", value));
	value = BIAS(value);
	value = MAPOUT(value);

	/* If we have the index file in memory, use it */
	if (db->dbz_corepag != NULL && place < db->dbz_conf.tsize) {
		db->dbz_corepag[place] = value;
		dprint(5, (debugfile, "set: incore\n"));
		return(0);
	}

	/* seek to spot */
	db->dbz_pagpos = -1;		/* invalidate position memory */
	if (fseek(db->dbz_pagf, place * SOF, SEEK_SET) != 0) {
		dprint(5, (debugfile, "set: seek failed\n"));
		sp->aborted = 1;
		return(-1);
	}

	/* write in data */
	if (fwrite((char *)&value, SOF, 1, db->dbz_pagf) != 1) {
		dprint(5, (debugfile, "set: write failed\n"));
		sp->aborted = 1;
		return(-1);
	}
	/* fflush improves robustness, and buffer re-use is rare anyway */
	if (fflush(db->dbz_pagf) == EOF) {
		dprint(5, (debugfile, "set: fflush failed\n"));
		sp->aborted = 1;
		return(-1);
	}

	dprint(5, (debugfile, "set: succeeded\n"));
	return(0);
}

/*
 - mybytemap - determine this machine's byte map
 *
 * A byte map is an array of ints, sizeof(of_t) of them.  The 0th int
 * is the byte number of the high-order byte in my of_t, and so forth.
 */
static void
mybytemap(map)
int map[];			/* -> int[SOF] */
{
	union {
		of_t o;
		char c[SOF];
	} u;
	register int *mp = &map[SOF];
	register int ntodo;
	register int i;

	u.o = 1;
	for (ntodo = (int)SOF; ntodo > 0; ntodo--) {
		for (i = 0; i < SOF; i++)
			if (u.c[i] != 0)
				break;
		if (i == SOF) {
			/* trouble -- set it to *something* consistent */
			dprint(5, (debugfile, "mybytemap: nonexistent byte %d!!!\n", ntodo));
			for (i = 0; i < SOF; i++)
				map[i] = i;
			return;
		}
		dprint(5, (debugfile, "mybytemap: byte %d\n", i));
		*--mp = i;
		while (u.c[i] != 0)
			u.o <<= 1;
	}
}

/*
 - bytemap - transform an of_t from byte ordering map1 to map2
 */
static of_t			/* transformed result */
bytemap(ino, map1, map2)
of_t ino;
int *map1;
int *map2;
{
	union oc {
		of_t o;
		char c[SOF];
	};
	union oc in;
	union oc out;
	register int i;

	in.o = ino;
	for (i = 0; i < SOF; i++)
		out.c[map2[i]] = in.c[map1[i]];
	return(out.o);
}

/*
 * This is a simplified version of the pathalias hashing function.
 * Thanks to Steve Belovin and Peter Honeyman
 *
 * hash a string into a long int.  31 bit crc (from andrew appel).
 * the crc table is computed at run time by crcinit() -- we could
 * precompute, but it takes 1 clock tick on a 750.
 *
 * This fast table calculation works only if POLY is a prime polynomial
 * in the field of integers modulo 2.  Since the coefficients of a
 * 32-bit polynomial won't fit in a 32-bit word, the high-order bit is
 * implicit.  IT MUST ALSO BE THE CASE that the coefficients of orders
 * 31 down to 25 are zero.  Happily, we have candidates, from
 * E. J.  Watson, "Primitive Polynomials (Mod 2)", Math. Comp. 16 (1962):
 *	x^32 + x^7 + x^5 + x^3 + x^2 + x^1 + x^0
 *	x^31 + x^3 + x^0
 *
 * We reverse the bits to get:
 *	111101010000000000000000000000001 but drop the last 1
 *         f   5   0   0   0   0   0   0
 *	010010000000000000000000000000001 ditto, for 31-bit crc
 *	   4   8   0   0   0   0   0   0
 */

#define POLY 0x48000000L	/* 31-bit polynomial (avoids sign problems) */

static long CrcTable[128];

/*
 - crcinit - initialize tables for hash function
 */
static void
crcinit()
{
	register int i, j;
	register long sum;

	for (i = 0; i < 128; ++i) {
		sum = 0L;
		for (j = 7 - 1; j >= 0; --j)
			if (i & (1 << j))
				sum ^= POLY >> j;
		CrcTable[i] = sum;
	}
	dprint(5, (debugfile, "crcinit: done\n"));
}

/*
 - hash - Honeyman's nice hashing function
 */
static long
hash(name, size)
register char *name;
register int size;
{
	register long sum = 0L;

	while (size--) {
		sum = (sum >> 7) ^ CrcTable[(sum ^ (*name++)) & 0x7f];
	}
	dprint(5, (debugfile, "hash: returns (%ld)\n", sum));
	return(sum);
}

/*
 - dbzdebug - control dbz debugging at run time
 */
int				/* old value */
dbzdebug(value)
int value;
{
#ifdef DBZDEBUG
	register int old = debug;

	debug = value;
	return(old);
#else
	return(-1);
#endif
}
