/*
 * Expansion - quoting, separation, substitution, globbing
 */

#ifndef lint
static char *RCSid = "$Id: eval.c,v 1.2 1993/12/21 01:40:35 polk Exp $";
#endif

#include "stdh.h"
#include <errno.h>
#include <setjmp.h>
#include <unistd.h>
#include <dirent.h>
#include <pwd.h>
#include "sh.h"
#include "expand.h"

/*
 * string expansion
 *
 * first pass: quoting, IFS separation, ${} and $() substitution.
 * second pass: filename expansion (*?[]~).
 */

/* expansion generator state */
typedef struct Expand {
	/* int  type; */	/* see expand() */
	char   *str;		/* string */
	union {
		char  **strv;	/* string[] */
		FILE   *file;	/* file */
	} u;			/* source */
	short	split;		/* split "$@"*/
} Expand;

#define	XBASE	0		/* scanning original */
#define	XSUB	1		/* expanding ${} string */
#define	XARGSEP	2		/* ifs0 between "$@" */
#define	XARG	3		/* expanding $*, $@ */
#define	XCOM	4		/* expanding $() */

static void     expand      ARGS((char *cp, XPtrV *wp, int f));
static int      varsub      ARGS((Expand *xp, char *sp, int stype));
static int      comsub      ARGS((Expand *xp, char *cp));
static char *   trimsub     ARGS((char *str, char *pat, int how));
static void     glob        ARGS((char *cp, XPtrV *wp));
static void     globit      ARGS((char *ds, char *dp, char *sp, XPtrV *wp, int check));
static char *   debunk      ARGS((char *cp));
static char *   tilde       ARGS((char *acp));
static char *   homedir     ARGS((char *name));
#ifdef ALTERNATIONS
static	int	alt_expand  ARGS((char *, XPtrV *, int));
static	int	alt_count   ARGS((char *));
static	int	alt_scan    ARGS((char **, char **, char, 	int));
#endif

int	ifs0 = ' ';		/* todo: first char of $IFS */

/* compile and expand word */
char *
substitute(cp, f)
	char const *cp;
	int f;
{
	struct source *s, *sold;

	sold = source;
	s = pushs(SWSTR);
	s->str = (char *) cp;
	source = s;
	if (yylex(ONEWORD) != LWORD)
		errorf("eval:substitute error\n");
	source = sold;
	return evalstr(yylval.cp, f);
}

/*
 * expand arg-list
 */
char **
eval(ap, f)
	register char **ap;
{
	XPtrV w;

	if (*ap == NULL)
		return ap;
	XPinit(w, 32);
	XPput(w, NULL);		/* space for shell name */
#ifdef	SHARPBANG
	XPput(w, NULL);		/* and space for one arg */
#endif
	while (*ap != NULL)
		expand(*ap++, &w, f);
	XPput(w, NULL);
#ifdef	SHARPBANG
	return (char **) XPclose(w) + 2;
#else
	return (char **) XPclose(w) + 1;
#endif
}

/*
 * expand string
 */
char *
evalstr(cp, f)
	register char *cp;
	int f;
{
	XPtrV w;

	XPinit(w, 1);
	expand(cp, &w, f);
	cp = (XPsize(w) == 0) ? "" : (char*) *XPptrv(w);
	XPfree(w);
	return cp;
}

/*
 * expand string - return only one component
 * used from iosetup to expand redirection files
 */
char *
evalonestr(cp, f)
	register char *cp;
	int f;
{
	XPtrV w;

	XPinit(w, 1);
	expand(cp, &w, f);
	switch (XPsize(w)) {
	case 0:
		cp = "";
		break;
	case 1:
		cp = (char*) *XPptrv(w);
		break;
	default:
		cp = evalstr(cp, f&~DOGLOB);
		break;
	}
	XPfree(w);
	return cp;
}

/* for nested substitution: ${var:=$var2} */
typedef struct SubType {
	short	type;		/* [=+-?%#] action after expanded word */
	short	base;		/* begin position of expanded word */
	char   *name;		/* name for ${var=word} */
} SubType;

static void
expand(cp, wp, f)
	char *cp;		/* input word */
	register XPtrV *wp;	/* output words */
	int f;			/* DO* flags */
{
	register int c;
	register int type = XBASE; /* expansion type */
	register int quote = 0;	/* quoted */
	int quotestack[11];	/* Keep this bigger than the subtype stack */
	register int *qst = quotestack + 11;	/* This too, of course */
	XString ds;		/* destination string */
	register char *dp, *sp;	/* dest., source */
	int fdo, word, combase;	/* second pass flags; have word */
	Expand x;		/* expansion variables */
	SubType subtype [10];	/* substitution type stack */
	register SubType *st = subtype + 10;
	int newlines;		/* For trailing newlines in COMSUB */
	int trimming = 0;	/* flag if expanding ${var#pat} or ${var%pat} */

	if (cp == NULL)
		errorf("eval:expand(NULL)\n");
	if (flag[FNOGLOB])
		f &= ~ DOGLOB;

#ifdef ALTERNATIONS
#define NOALT	BIT(8)		/* internal to this file */
				/* prevent endless recursion */
	/* look for '{' in the input word */
	if (((f & NOALT) == 0) && (f & DOGLOB) &&
	    (dp = strchr(cp, '{')) != NULL &&
	    (dp[-1] == CHAR) &&
    	    !(dp[1] == CHAR && dp[2] == '}')) {
		if (alt_expand(cp, wp, f))
			return;
	}
	f &= ~NOALT;
#endif

	Xinit(ds, dp, 128);	/* init dest. string */
	type = XBASE;
	sp = cp;
	fdo = 0;
	word = !(f&DOBLANK);

	while (1) {
		Xcheck(ds, dp);

		switch (type) {
		  case XBASE:	/* original prefixed string */
			c = *sp++;
			switch (c) {
			  case EOS:
				c = 0;
				break;
			  case CHAR:
				c = *sp++;
				break;
			  case QCHAR:
				quote |= 2; /* temporary quote */
				c = *sp++;
				break;
			  case OQUOTE:
				word = quote = 1;
				continue;
			  case CQUOTE:
				quote = 0;
				continue;
			  case COMSUB:
				type = comsub(&x, sp);
				sp = strchr(sp, 0) + 1;
				combase = Xsavepos(ds, dp);
				newlines = 0;
				continue;
			  case OSUBST: /* ${var{:}[=+-?]word} */
				cp = sp; 		/* variable */
				sp = strchr(sp, 0) + 1;	/* skip variable */
				c = (*sp == CSUBST) ? 0 : *sp++;
				if ((c&0x7F) == '#' || (c&0x7F) == '%') {
					if (flag[FNOUNSET] &&
					    strval(global(cp)) == null)
						errorf("%s: unset variable\n", cp);
					trimming++;
					type = XBASE;
					*--qst = quote;
					quote = 0;
				} else
					type = varsub(&x, cp, c);
				if (type == XBASE) {	/* expand? */
					if (st == subtype)
						errorf("ridiculous ${} nesting\n");
					--st;
					st->type = c;
					st->base = Xsavepos(ds, dp);
					st->name = cp;
				} else
					sp = wdscan(sp, CSUBST); /* skip word */
				continue;
			  case CSUBST: /* only get here if expanding word */
				*dp = 0;
				if (f&DOGLOB)
					f &= ~DOPAT;
				switch (st->type&0x7F) {
				  case '#':
				  case '%':
					*dp = 0;
					dp = Xrestpos(ds, dp, st->base);
					quote = *qst++;
					x.str = trimsub(strval(global(st->name)),
						dp, st->type);
					type = XSUB;
					trimming--;
					continue;
				  case '=':
#if 0
					if ((x.u.vp->flag&RDONLY))
						errorf("cannot set readonly %s\n", cp);
#endif
					setstr(global(st->name), Xrestpos(ds, dp, st->base));
					break;
				  case '?':
					if (dp == Xrestpos(ds, dp, st->base))
						errorf("missing value for %s\n", cp);
					else
						errorf("%s\n", Xrestpos(ds, dp, st->base));
				}
				st++;
				type = XBASE;
				continue;
			}
			break;

		  case XSUB:
			if ((c = *x.str++) == 0) {
				type = XBASE;
				continue;
			}
			break;

		  case XARGSEP:
			type = XARG;
			quote = 1;
		  case XARG:
			if ((c = *x.str++) == 0) {
				if ((x.str = *x.u.strv++) == NULL) {
					type = XBASE;
					continue;
				} else if (quote && x.split) {
					/* terminate word for "$@" */
					type = XARGSEP;
					quote = 0;
				}
				c = ifs0;
			}
			break;

		  case XCOM:
			if (newlines) {		/* Spit out saved nl's */
				c = '\n';
				--newlines;
			} else {
				while ((c = getc(x.u.file)) == '\n')
					newlines++;	/* Save newlines */
				if (newlines && c != EOF) {
					ungetc(c, x.u.file);
					c = '\n';
					--newlines;
				}
			}
			if (c == EOF) {
				cp = Xrestpos(ds, sp, combase);
				newlines = 0;
				fclose(x.u.file);
				if (x.split)
					waitlast();
				type = XBASE;
				continue;
			}
			break;
		}

		/* check for end of word or IFS separation */
		if (c == 0 || (!quote && (f&DOBLANK) && ctype(c, C_IFS))) {
			if (word) {
				*dp++ = 0;
				cp = Xclose(ds, dp);
				if (fdo&DOTILDE)
					cp = tilde(cp);
				if (fdo&DOGLOB)
					glob(cp, wp);
				else
					{XPput(*wp, cp);}
				fdo = word = 0;
				if (c != 0)
					Xinit(ds, dp, 128);
			} else
				; /* ignore IFS */
			if (c == 0)
				return;
		} else {
			/* mark any special second pass chars */
			if (!quote)
				switch (c) {
				  case '*':
				  case '?':
				  case '[':
					if (f&(DOPAT|DOGLOB) || trimming) {
						fdo |= (f&DOGLOB);
						*dp++ = MAGIC;
					}
					break;
				  case NOT:
					if ((f&(DOPAT|DOGLOB) || trimming) &&
					    dp[-1] == '[' && dp[-2] == MAGIC) {
						*dp++ = MAGIC;
					}
					break;
				  case '~':
					if (((f&DOTILDE) &&
					     dp == Xstring(ds, dp)) ||
					    (!(f&DOBLANK) && 
					    (dp[-1] == '=' || dp[-1] == ':'))) {
						fdo |= DOTILDE;
						*dp++ = MAGIC;
					}
					break;
				}
			else
				quote &= ~2; /* undo temporary */

			word = 1;
			*dp++ = c; /* save output char */
		}
	}
}

/*
 * Prepare to generate the string returned by ${} substitution.
 */
static int
varsub(xp, sp, stype)
	register Expand *xp;
	register char *sp;
	int stype;
{
	register int c;
	int type;

	/* ${#var}, string length or argc */
	if (sp[0] == '#' && (c = sp[1]) != 0) {
		c = (c == '*' || c == '@') ? e.loc->argc :
			strlen(strval(global(sp+1)));
		xp->str = strsave(ulton((unsigned long)c, 10), ATEMP);
		return XSUB;
	}

	c = sp[0];
	if (c == '*' || c == '@') {
		if (e.loc->argc == 0) {
			xp->str = null;
			type = XSUB;
		} else {
			xp->u.strv = e.loc->argv + 1;
			xp->str = *xp->u.strv++;
			xp->split = c == '@'; /* $@ */
			type = XARG;
		}
	} else {
		if ((xp->str = strval(global(sp))) == NULL)
		  xp->str = null;
		type = XSUB;
	}

	c = stype&0x7F;
	/* test the compiler's code generator */
	if (c == '%' || c == '#' ||
	    (((stype&0x80) ? *xp->str=='\0' : xp->str==null) ? /* undef? */
	     c == '=' || c == '-' || c == '?' : c == '+'))
		type = XBASE;	/* expand word instead of variable value */
	if (type != XBASE && flag[FNOUNSET] && xp->str == null && c != '+')
		errorf("%s: unset variable\n", sp);
	return type;
}

/*
 * Run the command in $(...) and read its output.
 */
static int
comsub(xp, cp)
	register Expand *xp;
	char *cp;
{
	Source *s;
	register struct op *t;
	FILE *fi;

	s = pushs(SSTRING);
	s->str = cp;
	t = compile(s);

	if (t != NULL && t->type == TCOM && /* $(<file) */
	    *t->args == NULL && *t->vars == NULL && t->ioact != NULL) {
		register struct ioword *io = *t->ioact;

		if ((io->flag&IOTYPE) != IOREAD)
			errorf("funny $() command\n");
		fi = fopen(evalstr(io->name, DOTILDE), "r");
		if (fi != NULL)
			fileno(fi) = savefd(fileno(fi));
		xp->split = 0;	/* no waitlast() */
	} else {
		int ofd1, pv[2];
		openpipe(pv);
		fi = fdopen(pv[0], "r");
		ofd1 = savefd(1);
		dup2(pv[1], 1);
		close(pv[1]);
		(void) fd_clexec(pv[0]);
		(void) fd_clexec(ofd1);
		execute(t, XFORK|XXCOM|XPIPEO);
		dup2(ofd1, 1);
		close(ofd1);
		xp->split = 1;	/* waitlast() */
	}	

	if (fi == NULL)
		errorf("cannot open $() input\n");
	setvbuf(fi, (char *)NULL, _IOFBF, BUFSIZ);
	xp->u.file = fi;
	return XCOM;
}

/*
 * perform #pattern and %pattern substitution in ${}
 */

static char *
trimsub(str, pat, how)
	register char *str;
	char *pat;
	int how;
{
	register char *end = strchr(str, 0);
	register char *p, c, *match;

	switch (how&0xff) {	/* UCHAR_MAX maybe? */
	case '#':		/* shortest at begin */
		for (p = str; p <= end; p++) {
			c = *p; *p = '\0';
			if (gmatch(str, pat)) {
				*p = c;
				return p;
			}
			*p = c;
		}
		break;
	case '#'|0x80:		/* longest match at begin */
		for (p = end; p >= str; p--) {
			c = *p; *p = '\0';
			if (gmatch(str, pat)) {
				*p = c;
				return p;
			}
			*p = c;
		}
		break;
	case '%':		/* shortest match at end */
		for (p = end; p >= str; p--) {
			if (gmatch(p, pat)) {
				c = *p; *p = '\0';
				match = strsave( str, APERM );	/* APERM? */
				*p = c;
				return match;
			}
		}
		break;
	case '%'|0x80:	/* longest match at end */
		for (p = str; p <= end; p++) {
			if (gmatch(p, pat)) {
				c = *p; *p = '\0';
				match = strsave( str, ATEMP );	/* APERM? */
				*p = c;
				return match;
			}
		}
		break;
	}

	return str;		/* no match, return string */
}

#ifdef ALTERNATIONS
/*	(pc@hillside.co.uk)
 *	I have decided to `fudge' alternations by picking up
 *	the compiled command tree and working with it recursively
 *	to generate the set of arguments
 *	This has the advantage of making a single discrete change
 *	to the code
 *
 *	This routine calls itself recursively
 *	a)	scan forward looking for { building the output string
 *		if none found then call expand - and exit
 *	b)	When { found, scan forward finding the end }
 *	c)	add first alternate to output string
 *	d)	scan for the end of the string copying into output
 *	e)	call routine with new string
 *	Major complication is quoting
 */
static int
alt_expand(cp, wp, f)
	char *cp;		/* input word */
	register XPtrV *wp;	/* output words */
	int f;			/* DO* flags */
{
	char *srcp = cp;
	char *left;		/* destination string of left hand side */
	char *leftend;		/* end of left hand side */
	char *alt;		/* start of alterate section */
	char *altend;		/* end of alternate section */
	char *ap;		/* working pointer */
	char *right;		/* right hand side */
	char *rp;		/* used to copy right-hand side */
	int  maxlen;		/* max string length */

	leftend = left = alloc((maxlen = alt_count(cp)), ATEMP);
	
	if (alt_scan(&srcp, &leftend, '{', 0) == 0) {
		expand(cp, wp, f&NOALT);
		afree(left, ATEMP);
		return;
	}

	/*
	 *	we have a alternation section
	 */
	alt = altend = alloc(maxlen, ATEMP);

	srcp += 2;
	if (alt_scan(&srcp, &altend, '}', 1) == 0) {
		afree(left, ATEMP);
		afree(alt, ATEMP);
		errorf("Missing }.\n");
	}
	*altend++ = CHAR;
	*altend++ = ',';
	*altend = EOS;
	/*
	 *	finally we may have a right-hand side
	 */
	right = srcp + 2;

	/*
	 *	glue the bits together making a new string
	 */
	for (srcp = alt; *srcp != EOS;) {

		ap = leftend;

		if (alt_scan(&srcp, &ap, ',', -1) == 0) {
			afree(left, ATEMP);
			afree(alt, ATEMP);
			errorf("Missing comma.\n");
		}
		
		srcp += 2;

		rp = right;
		(void) alt_scan(&rp, &ap, EOS, 0);

		alt_expand(left, wp, f);
	}
	afree(left, ATEMP);
	afree(alt, ATEMP);
}

/*
 * see how much space we need to hold this tree
 */
static int
alt_count(cp)
	register char *cp;
{
	register int sum = 0;
	register char *sp;

	while (*cp != EOS) {
		switch(*cp) {
		  case CHAR:
		  case QCHAR:
			sum += 2;
			cp += 2;
			break;
		  case OQUOTE:
		  case CQUOTE:
		  case CSUBST:
			sum++;
			cp++;
			break;
		  case COMSUB:
		  case OSUBST:
			sp = cp;
			cp = strchr(sp, 0) + 1;
			sum += cp - sp;
			break;
		}
	}
	return ++sum;
}

#ifdef __STDC__
static int
alt_scan(
	char **cpp,		/* address of source pointer */
	char **dpp,		/* address of destination pointer */
	char endc,		/* last character we are looking for */
	int bal)
#else
static int
alt_scan(cpp, dpp, endc, bal)
	char **cpp;		/* address of source pointer */
	char **dpp;		/* address of destination pointer */
	char endc;		/* last character we are looking for */
	int bal;
#endif
{
	register char *cp, *dp;
	int quote = 0;
	int balance = 0;
	int usebalance = 0;

	if (bal)
	{	usebalance = 1;
		balance = (bal < 1) ? 0 : 1;
	}

	cp = *cpp;
	dp = *dpp;

	while (*cp != EOS) {
		switch (*cp) {
		  case CHAR:
			if (quote == 1) {
				if (cp[1] == ']')
					quote = 0;
			}
			else
			if (quote == 0) {
				if (cp[1] == '[')
					quote = 1;
				else {
					if (usebalance) {
						if (cp[1] == '{')
							balance++;
						if (cp[1] == '}')
							balance--;
						}
					if (cp[1] == endc && balance == 0) {
						*dp = EOS;
						*dpp = dp;
						*cpp = cp;
						return 1;
						}
				}
			}
		  case QCHAR:
			*dp++ = *cp++;
		  case CSUBST:
		  copy:
			*dp++ = *cp++;
			break;
			
		  case OQUOTE:
			quote = 1;
			goto copy;

		  case CQUOTE:
			quote = 0;
			goto copy;

		  case COMSUB:
		  case OSUBST:
			while (*dp++ = *cp++);
			break;
		}
	}
	*dp = EOS;
	*cpp = cp;
	*dpp = dp;
	return 0;
}
#endif	/* ALTERNATIONS */

/*
 * glob
 * Name derived from V6's /etc/glob, the program that expanded filenames.
 */

static	char   *debunk();

static void 
glob(cp, wp)
	char *cp;
	register XPtrV *wp;
{
	char path [PATH];
	register char *sp = cp;
	int oldsize;

	oldsize = XPsize(*wp);
	globit(path, path, sp, wp, 0);

	if (XPsize(*wp) == oldsize)
		{XPput(*wp, debunk(cp));}
	else
		qsortp(XPptrv(*wp) + oldsize, (size_t)(XPsize(*wp) - oldsize), xstrcmp);
}

static void
globit(ds, dp, sp, wp, check)
	char *ds;		/* dest path */
	char *dp;		/* dest end */
	char *sp;		/* source path */
	register XPtrV *wp;	/* output list */
	int check;		/* check dest existence */
{
	register char *np;	/* next source component */
	register char *tsp, *tdp;

	if (sp == NULL) {	/* end of source path */
		if (check && eaccess(ds, 0) < 0)
			return;
		XPput(*wp, strsave(ds, ATEMP));
		return;
	}

	if (dp > ds)
		*dp++ = '/';
	while (*sp == '/')
		*dp++ = *sp++;
	np = strchr(sp, '/');
	if (np != NULL)
		*np++ = 0;

	*dp = 0;
	if (strchr(sp, MAGIC) == NULL) { /* contains no pattern? */
		tdp = dp; tsp = sp;
		while ((*tdp++ = *tsp++) != 0)
			;
		--tdp;
		globit(ds, tdp, np, wp, check);
	} else {
		DIR *dirp;
		struct dirent *d;

		/* ToDo:
		 * should not attemp to open() special files: /dev/ttyd0/*
		 * opendir should do this check, but Doug Gwyn's does not.
		 */
		dirp = opendir((*ds == 0) ? "." : ds);
		if (dirp == NULL)
			goto Nodir;
		while ((d = readdir(dirp)) != NULL) {
			tsp = d->d_name;
			if (tsp[0] == '.' &&
			    (tsp[1] == 0 || (tsp[1] == '.' && tsp[2] == 0)))
				continue; /* always ignore . and .. */
			if ((*tsp == '.' && *sp != '.') || !gmatch(tsp, sp))
				continue;

			tdp = dp;
			while ((*tdp++ = *tsp++) != 0)
				;
			--tdp;
			globit(ds, tdp, np, wp, np != NULL);
		}
		closedir(dirp);
	  Nodir:;
	}

	if (np != NULL)
		*--np = '/';
}

/* remove MAGIC from string */
static char *
debunk(cp)
	char *cp;
{
	register char *dp, *sp;

	for (dp = sp = cp; *sp != 0; sp++)
		if (*sp != MAGIC)
			*dp++ = *sp;
	*dp = 0;
	return cp;
}

/*
 * tilde expansion
 *
 * based on a version by Arnold Robbins
 */

static char *homedir();

static char *
tilde(acp)
	char *acp;
{
	register int c;
	char path [PATH+1];
	register char *cp = acp, *wp = path, *dp;
	char userid [16+1];

  Again:
	while (1) {
		if ((c = *cp++) == 0) {
			*wp = 0;
			afree((void*)acp, ATEMP);
			return strsave(path, ATEMP);
		} else if (c == MAGIC && *cp == '~')
			break;
		else
			*wp++ = c;
	}

	dp = NULL;	/* no output substitution */
	if (cp[1] == 0 || cp[1] == '/' || cp[1] == ':') /* ~ or ~/ */
		dp = strval(global("HOME")), cp += 1;
	else if (cp[1] == '+' && (cp[2] == '/' || cp[2] == ':' || cp[2] == 0))
		dp = strval(global("PWD")), cp += 2;
	else if (cp[1] == '-' && (cp[2] == '/' || cp[2] == ':' || cp[2] == 0))
		dp = strval(global("OLDPWD")), cp += 2;
	else if (letter(cp[1])) {
		char *save = cp;
		for (dp = userid, cp++; letnum(*cp) && dp < userid+16; )
			*dp++ = *cp++;
		*dp = 0;
		dp = homedir(userid);
		if (dp == NULL)
			cp = save;
	}
	/* substitute */
	if (dp != NULL)
		while (*dp != 0)
			*wp++ = *dp++;
	goto Again;
}

/*
 * map userid to user's home directory.
 * todo: implement a cache with the "homedirs" table.
 * note that 4.3's getpw adds more than 6K to the shell,
 * and the YP version probably adds much more.
 * we might consider our own version of getpwnam() to keep the size down.
 */

static char *
homedir(name)
	char *name;
{
	register struct tbl *ap;
	register struct passwd *pw;
	extern struct passwd *getpwnam();

	ap = tsearch(&homedirs, name, hash(name));
	if ((ap != NULL && (ap->flag&ISSET)))
		return ap->val.s;
	pw = getpwnam(name);
	if (pw == NULL)
		return NULL;
	return pw->pw_dir;
}
