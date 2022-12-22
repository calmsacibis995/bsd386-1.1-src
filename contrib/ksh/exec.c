/*
 * execute command tree
 */

#ifndef lint
static char *RCSid = "$Id: exec.c,v 1.2 1993/12/21 01:41:50 polk Exp $";
#endif

#include "stdh.h"
#include <errno.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "sh.h"
#include "edit.h"

static int      comexec     ARGS((struct op *t, char **vp, char **ap, int flags));
#ifdef	SHARPBANG
static void     scriptexec  ARGS((struct op *tp, char **ap));
#endif
static void     iosetup     ARGS((struct ioword *iop));
static int      herein      ARGS((char *hname, int sub));
static void     echo        ARGS((char **vp, char **ap));
static	char 	*do_selectargs ARGS((char **ap, char *));
static	int	selread	    ARGS((void));


/*
 * handle systems that don't have F_SETFD
 */
#ifndef F_SETFD
# ifndef MAXFD
#   define  MAXFD 64
# endif
/*
 * a bit field would be smaller, but this
 * will work
 */
static char clexec_tab[MAXFD+1];

/* this is so that main() can clear it */
void
init_clexec()
{
  (void) memset(clexec_tab, 0, sizeof(clexec_tab)-1);
}
#endif

/*
 * we now use this function always.
 */
int
fd_clexec(fd)
  int fd;
{
#ifndef F_SETFD
  static int once = 0;

  if (once++ == 0)
    init_clexec();

  if (fd < sizeof(clexec_tab))
  {
    clexec_tab[fd] = 1;
    return 0;
  }
  return -1;
#else
  return fcntl(fd, F_SETFD, 1);
#endif
}


/*
 * execute command tree
 */
int
execute(t, flags)
	register struct op *t;
	volatile int flags;	/* if XEXEC don't fork */
{
	int i;
	int volatile rv = 0;
	int pv[2];
	register char **ap;
	char *s, *cp;
	struct ioword **iowp;

	if (t == NULL)
		return 0;

	if ((flags&XFORK) && !(flags&XEXEC) && t->type != TPIPE)
		return exchild(t, flags); /* run in sub-process */

	newenv(E_EXEC);
	if (trap)
		runtraps();
 
	if (t->ioact != NULL || t->type == TPIPE) {
		e.savefd = (short*) alloc(sizeofN(short, NUFILE), ATEMP);
		for (i = 0; i < NUFILE; i++)
			e.savefd[i] = 0; /* not redirected */
		/* mark fd 0/1 in-use if pipeline */
		if (flags&XPIPEI)
			e.savefd[0] = -1;
		if (flags&XPIPEO)
			e.savefd[1] = -1;
	}

	/* do redirection, to be restored in quitenv() */
	if (t->ioact != NULL)
		for (iowp = t->ioact; *iowp != NULL; iowp++)
			iosetup(*iowp);

	switch(t->type) {
	  case TCOM:
		e.type = E_TCOM;
		rv = comexec(t, eval(t->vars, DOTILDE),
			     eval(t->args, DOBLANK|DOGLOB|DOTILDE), flags);
		break;

	  case TPAREN:
		exstat = rv = execute(t->left, flags|XFORK);
		break;

	  case TPIPE:
		flags |= XFORK;
		flags &= ~XEXEC;
		e.savefd[0] = savefd(0);
		e.savefd[1] = savefd(1);
		flags |= XPIPEO;
		(void) dup2(e.savefd[0], 0); /* stdin of first */
		while (t->type == TPIPE) {
			openpipe(pv);
			(void) dup2(pv[1], 1);	/* stdout of curr */
			(void) fd_clexec(pv[0]);
			(void) fd_clexec(pv[1]);
			exchild(t->left, flags);
			(void) dup2(pv[0], 0);	/* stdin of next */
			closepipe(pv);
			flags |= XPIPEI;
			t = t->right;
		}
		flags &= ~ XPIPEO;
#if 0
		/* 
		 * this patch broke more things than it fixed
		 */
		flags |= XEXEC;
#endif
		(void) dup2(e.savefd[1], 1); /* stdout of last */
		(void) fd_clexec(e.savefd[0]);
		(void) fd_clexec(e.savefd[1]); 
		execute(t, flags);
		(void) dup2(e.savefd[0], 0); /* close pipe in */
		flags &= ~XEXEC;
		if (!(flags&XBGND))
			exstat = rv = waitlast();
		break;

	  case TLIST:
		while (t->type == TLIST) {
			execute(t->left, flags & XXWHL);
			t = t->right;
		}
		rv = execute(t, flags & XXWHL);
		break;

	  case TASYNC:
		rv = execute(t->left, flags|XBGND|XFORK);
		break;

	  case TOR:
	  case TAND:
		rv = execute(t->left, flags & XXWHL);
		if (t->right != NULL && (rv == 0) == (t->type == TAND))
			rv = execute(t->right, flags & XXWHL);
		break;

	  case TFOR:
		e.type = E_LOOP;
		ap = (t->vars != NULL) ?
			eval(t->vars, DOBLANK|DOGLOB|DOTILDE) : e.loc->argv + 1;
		while ((i = setjmp(e.jbuf)))
			if (i == LBREAK)
				goto Break1;
		while (*ap != NULL) {
			setstr(global(t->str), *ap++);
			rv = execute(t->left, flags & XXWHL);
		}
	  Break1:
		break;

	  case TSELECT:
		e.type = E_LOOP;
		ap = (t->vars != NULL) ?
			eval(t->vars, DOBLANK|DOGLOB|DOTILDE) : e.loc->argv + 1;
		while ((i = setjmp(e.jbuf)))
			if (i == LBREAK)
				goto Break1;
		signal(SIGINT, trapsig); /* needs change to trapsig */
		cp = NULL;
		for (;;) {
			if ((cp = do_selectargs(ap, cp)) == (char *)1)
				break;
			setstr(global(t->str), cp);
			rv = execute(t->left, flags & XXWHL);
		}
		break;
		
	  case TWHILE:
	  case TUNTIL:
		e.type = E_LOOP;
		while ((i = setjmp(e.jbuf)))
			if (i == LBREAK)
				goto Break2;
		while ((execute(t->left, flags & XXWHL) == 0) == (t->type == TWHILE))
			rv = execute(t->right, XXWHL);
	  Break2:
		break;

	  case TIF:
	  case TELIF:
		if (t->right == NULL)
			break;	/* should be error */
		rv = execute(t->left, flags & XXWHL) == 0 ?
			execute(t->right->left, flags & XXWHL) :
			execute(t->right->right, flags & XXWHL);
		break;

	  case TCASE:
		cp = evalstr(t->str, 0);
		for (t = t->left; t != NULL && t->type == TPAT; t = t->right)
		    for (ap = t->vars; *ap; ap++)
			if ((s = evalstr(*ap, DOPAT)) && gmatch(cp, s))
				goto Found;
		break;
	  Found:
		rv = execute(t->left, flags & XXWHL);
		break;

	  case TBRACE:
		rv = execute(t->left, flags & XXWHL);
		break;

	  case TFUNCT:
		rv = define(t->str, t->left);
		break;

	  case TTIME:
		rv = timex(t, flags);
		break;

	  case TEXEC:		/* an eval'd TCOM */
		s = t->args[0];
		ap = makenv();
#ifndef F_SETFD
		for (i = 0; i < sizeof(clexec_tab); i++)
		  if (clexec_tab[i])
		  {
		    close(i);
		    clexec_tab[i] = 0;
		  }
#endif
		execve(t->str, t->args, ap);
		if (errno == ENOEXEC) {
			char *shell;
#ifdef	SHARPBANG
			scriptexec(t, ap);
#else
			shell = strval(global("EXECSHELL"));
			if (shell && *shell) {
				if ((shell = search(shell,path,1)) == NULL)
					shell = SHELL;
			} else {
				shell = SHELL;
			}
			*t->args-- = t->str;
			*t->args = shell;
			execve(t->args[0], t->args, ap);
			errorf("No shell\n");
#endif	/* SHARPBANG */
		}
		errorf("%s: %s\n", s, strerror(errno));
	}

	quitenv();		/* restores IO */
	if (e.interactive) {	/* flush stdout, shlout */
		fflush(shf[1]);
		fflush(shf[2]);
	}
	if ((flags&XEXEC) && !(flags&XPIPEI))  
		exit(rv);	/* exit child */
	return rv;
}

/*
 * execute simple command
 */

static int
comexec(t, vp, ap, flags)
	struct op *t;
	register char **ap, **vp;
	int flags;
{
	int i;
	int rv = 0;
	register char *cp;
	register char **lastp;
	register struct tbl *tp = NULL;
	register struct block *l;
	static struct op texec = {TEXEC};
	extern int c_exec(), c_builtin();

	if (flag[FXTRACE])
		echo(vp, ap);

	/* snag the last argument for $_ */
	if ((lastp = ap) && *lastp) {
		while (*++lastp)
			;
		setstr(typeset("_",LOCAL,0),*--lastp);
	}	

	/* create new variable/function block */
	l = (struct block*) alloc(sizeof(struct block), ATEMP);
	l->next = e.loc; e.loc = l;
	newblock();

 Doexec:
	if ((cp = *ap) == NULL)
		cp = ":";
	tp = findcom(cp, flag[FHASHALL]);

	switch (tp->type) {
	  case CSHELL:			/* shell built-in */
		while (tp->val.f == c_builtin) {
			if ((cp = *++ap) == NULL)
				break;
			tp = tsearch(&builtins, cp, hash(cp));
			if (tp == NULL)
				errorf("%s: not builtin\n", cp);
		}
		if (tp->val.f == c_exec) {
			if (*++ap == NULL) {
				e.savefd = NULL; /* don't restore redirection */
				break;
			}
			flags |= XEXEC;
			goto Doexec;
		}
		if ((tp->flag&TRACE))
			e.loc = l->next; /* no local block */
		i = (tp->flag&TRACE) ? 0 : LOCAL;
		while (*vp != NULL)
			(void) typeset(*vp++, i, 0);
		rv = (*tp->val.f)(ap);
		break;

	case CFUNC:			/* function call */
		if (!(tp->flag&ISSET))
			errorf("%s: undefined function\n", cp);
		l->argv = ap;
		for (i = 0; *ap++ != NULL; i++)
			;
		l->argc = i - 1;
		resetopts();
		while (*vp != NULL)
			(void) typeset(*vp++, LOCAL, 0);
		e.type = E_FUNC;
		if (setjmp(e.jbuf))
			rv = exstat; /* return # */
		else
			rv = execute(tp->val.t, flags & XXWHL);
		break;

	case CEXEC:		/* executable command */
		if (!(tp->flag&ISSET)) {
			/*
			 * mlj addition:
			 *
			 * If you specify a full path to a file
			 * (or type the name of a file in .) which
			 * doesn't have execute priv's, it used to
			 * just say "not found".  Kind of annoying,
			 * particularly if you just wrote a script
			 * but forgot to say chmod 755 script.
			 *
			 * This should probably be done in eaccess(),
			 * but it works here (at least I haven't noticed
			 * changing errno here breaking something
			 * else).
			 *
			 * So, we assume that if the file exists, it
			 * doesn't have execute privs; else, it really
			 * is not found.
			 */
			if (access(cp, 0) < 0)
			    shellf("%s: not found\n", cp);
			else
			    shellf("%s: cannot execute\n", cp);
			rv = 1;
			break;
		}

		/* set $_ to program's full path */
		setstr(typeset("_", LOCAL|EXPORT, 0), tp->val.s);
		while (*vp != NULL)
			(void) typeset(*vp++, LOCAL|EXPORT, 0);

		if ((flags&XEXEC)) {
			j_exit();
			if (flag[FMONITOR] || !(flags&XBGND))
			{
#ifdef USE_SIGACT
			  sigaction(SIGINT, &Sigact_dfl, NULL);
			  sigaction(SIGQUIT, &Sigact_dfl, NULL);
#else
			  signal(SIGINT, SIG_DFL);
			  signal(SIGQUIT, SIG_DFL);
#endif
			}
		}

		/* to fork we set up a TEXEC node and call execute */
		texec.left = t;	/* for tprint */
		texec.str = tp->val.s;
		texec.args = ap;
		rv = exchild(&texec, flags);
		break;
	}
	if (rv != 0 && flag[FERREXIT])
		leave(rv);
	return (exstat = rv);
}

#ifdef	SHARPBANG
static void
scriptexec(tp, ap)
	register struct op *tp;
	register char **ap;
{
	char line[LINE];
	register char *cp;
	register int fd, n;
	char *shell;

	shell = strval(global("EXECSHELL"));
	if (shell && *shell) {
		if ((shell = search(shell,path,1)) == NULL)
			shell = SHELL;
	} else {
		shell = SHELL;
	}

	*tp->args-- = tp->str;
	line[0] = '\0';
	if ((fd = open(tp->str,0)) >= 0) {
		if ((n = read(fd, line, LINE - 1)) > 0)
			line[n] = '\0';
		(void) close(fd);
	}
	if (line[0] == '#' && line[1] == '!') {
		cp = &line[2];
		while (*cp && (*cp == ' ' || *cp == '\t'))
			cp++;
		if (*cp && *cp != '\n') {
			*tp->args = cp;
			while (*cp && *cp != '\n' && *cp != ' ' && *cp != '\t')
				cp++;
			if (*cp && *cp != '\n') {
				*cp++ = '\0';
				while (*cp && (*cp == ' ' || *cp == '\t'))
					cp++;
				if (*cp && *cp != '\n') {
					tp->args--;
					tp->args[0] = tp->args[1];
					tp->args[1] = cp;
					while (*cp && *cp != '\n' &&
					       *cp != ' ' && *cp != '\t')
						cp++;
				}
			}
			*cp = '\0';
		} else
			*tp->args = shell;
	} else
		*tp->args = shell;

	(void) execve(tp->args[0], tp->args, ap);
	errorf( "No shell\n" );
}
#endif	/* SHARPBANG */

int
shcomexec(wp)
	register char **wp;
{
	register struct tbl *tp;

	tp = tsearch(&builtins, *wp, hash(*wp));
	if (tp == NULL)
		errorf("%s: shcomexec botch\n", *wp);
	return (*tp->val.f)(wp);
}

/*
 * define function
 */
int
define(name, t)
	char	*name;
	struct op *t;
{
	register struct block *l;
	register struct tbl *tp;

	for (l = e.loc; l != NULL; l = l->next) {
		lastarea = &l->area;
		tp = tsearch(&l->funs, name, hash(name));
		if (tp != NULL && (tp->flag&DEFINED))
			break;
		if (l->next == NULL) {
			tp = tenter(&l->funs, name, hash(name));
			tp->flag = DEFINED|FUNCT;
			tp->type = CFUNC;
		}
	}

	if ((tp->flag&ALLOC))
		tfree(tp->val.t, lastarea);
	tp->flag &= ~(ISSET|ALLOC);

	if (t == NULL) {		/* undefine */
		tdelete(tp);
		return 0;
	}

	tp->val.t = tcopy(t, lastarea);
	tp->flag |= (ISSET|ALLOC);

	return 0;
}

/*
 * add builtin
 */
builtin(name, func)
	char *name;
	int (*func)();
{
	register struct tbl *tp;
	int flag = DEFINED;

	if (*name == '=') {		/* sets keyword variables */
		name++;
		flag |= TRACE;	/* command does variable assignment */
	}

	tp = tenter(&builtins, name, hash(name));
	tp->flag |= flag;
	tp->type = CSHELL;
	tp->val.f = func;
}

/*
 * find command
 * either function, hashed command, or built-in (in that order)
 */
struct tbl *
findcom(name, insert)
	char	*name;
	int	insert;			/* insert if not found */
{
	register struct block *l = e.loc;
	unsigned int h = hash(name);
	register struct	tbl *tp = NULL;
	static struct tbl temp;

	if (strchr(name, '/') != NULL) {
		tp = &temp;
		tp->type = CEXEC;
		tp->flag = 0;	/* make ~ISSET */
		goto Search;
	}
	for (l = e.loc; l != NULL; l = l->next) {
		tp = tsearch(&l->funs, name, h);
		if (tp != NULL && (tp->flag&DEFINED))
			break;
	}
	if (tp == NULL) {
		tp = tsearch(&commands, name, h);
		if (tp != NULL &&
		    (tp->flag&ALLOC) &&
		    eaccess(tp->val.s,1) != 0) {
			if (tp->flag&ALLOC)
				afree(tp->val.s, commands.areap);
			tp->type = CEXEC;
			tp->flag = DEFINED;
		}
	}
	if (tp == NULL)
		tp = tsearch(&builtins, name, h);
	if (tp == NULL) {
		tp = tenter(&commands, name, h);
		tp->type = CEXEC;
		tp->flag = DEFINED;
	}
  Search:
	if (tp->type == CEXEC && !(tp->flag&ISSET)) {
		if (!insert) {
			tp = &temp;
			tp->type = CEXEC;
			tp->flag = 0;	/* make ~ISSET */
		}
		name = search(name, path, 1);
		if (name != NULL) {
			tp->val.s = strsave(name,
					    (tp == &temp) ? ATEMP : APERM);
			tp->flag |= ISSET|ALLOC;
		}
	}
	return tp;
}

/*
 * flush executable commands with relative paths
 */
flushcom(all)
	int all;		/* just relative or all */
{
	register struct tbl *tp;

	for (twalk(&commands); (tp = tnext()) != NULL; )
		if ((tp->flag&ISSET) && (all || tp->val.s[0] != '/')) {
			if ((tp->flag&ALLOC))
				afree(tp->val.s, commands.areap);
			tp->flag = DEFINED; /* make ~ISSET */
		}
}

/*
 * search for command with PATH
 */
char *
search(name, path, mode)
	char *name, *path;
	int mode;		/* 0: readable; 1: executable */
{
	register int i;
	register char *sp, *tp;
	struct stat buf;

	if (strchr(name, '/'))
		return (eaccess(name, mode) == 0) ? name : NULL;

	sp = path;
	while (sp != NULL) {
		tp = line;
		for (; *sp != '\0'; tp++)
			if ((*tp = *sp++) == ':') {
				--sp;
				break;
			}
		if (tp != line)
			*tp++ = '/';
		for (i = 0; (*tp++ = name[i++]) != '\0';)
			;
		i = eaccess(line, mode);
		if (i == 0 && (mode != 1 || (stat(line,&buf) == 0 &&
		    (buf.st_mode & S_IFMT) == S_IFREG)))
			return line;
		/* what should we do about EACCES? */
		if (*sp++ == '\0')
			sp = NULL;
	}
	return NULL;
}

/*
 * set up redirection, saving old fd's in e.savefd
 */
static void
iosetup(iop)
	register struct ioword *iop;
{
	register int u = -1;
	char *cp = iop->name;
	extern long lseek();

	if (iop->unit == 0 || iop->unit == 1 || iop->unit == 2)
		e.interactive = 0;
#if 0
	if (e.savefd[iop->unit] != 0)
		errorf("file descriptor %d already redirected\n", iop->unit);
#endif
	e.savefd[iop->unit] = savefd(iop->unit);

	if ((iop->flag&IOTYPE) != IOHERE)
		cp = evalonestr(cp, DOTILDE|DOGLOB);

	switch (iop->flag&IOTYPE) {
	  case IOREAD:
		u = open(cp, 0);
		break;

	  case IOCAT:
		if ((u = open(cp, 1)) >= 0) {
			(void) lseek(u, (long)0, 2);
			break;
		}
		/* FALLTHROUGH */
	  case IOWRITE:
		u = creat(cp, 0666);
		break;

	  case IORDWR:
		u = open(cp, 2);
		break;

	  case IOHERE:
		u = herein(cp, iop->flag&IOEVAL);
		/* cp may have wrong name */
		break;

	  case IODUP:
		if (*cp == '-')
			close(u = iop->unit);
		else
		if (digit(*cp))
			u = *cp - '0';
		else
			errorf("%s: illegal >& argument\n", cp);
		break;
	}
	if (u < 0)
		errorf("%s: cannot %s\n", cp,
		       (iop->flag&IOTYPE) == IOWRITE ? "create" : "open");
	if (u != iop->unit) {
		(void) dup2(u, iop->unit);
		if (iop->flag != IODUP)
			close(u);
	}

	fopenshf(iop->unit);
}

/*
 * open here document temp file.
 * if unquoted here, expand here temp file into second temp file.
 */
static int
herein(hname, sub)
	char *hname;
	int sub;
{
	int fd;
	FILE * volatile f = NULL;

	f = fopen(hname, "r");
	if (f == NULL)
		return -1;
	setvbuf(f, (char *)NULL, _IOFBF, BUFSIZ);

	if (sub) {
		char *cp;
		struct source *s;
		struct temp *h;

		newenv(E_ERRH);
		if (setjmp(e.jbuf)) {
			if (f != NULL)
				fclose(f);
			quitenv();
			return -1; /* todo: error()? */
		}

		/* set up yylex input from here file */
		s = pushs(SFILE);
		s->u.file = f;
		source = s;
		if (yylex(ONEWORD) != LWORD)
			errorf("exec:herein error\n");
		cp = evalstr(yylval.cp, 0);

		/* write expanded input to another temp file */
		h = maketemp(ATEMP);
		h->next = e.temps; e.temps = h;
		if (h == NULL)
			error();
		f = fopen(h->name, "w+");
		if (f == NULL)
			error();
		setvbuf(f, (char *)NULL, _IOFBF, BUFSIZ);
		fputs(cp, f);
		rewind(f);

		quitenv();
	}
	fd = dup(fileno(f));
	fclose(f);
	return fd;
}

static void
echo(vp, ap)
	register char **vp, **ap;
{
	shellf("+");
	while (*vp != NULL)
		shellf(" %s", *vp++);
	while (*ap != NULL)
		shellf(" %s", *ap++);
	shellf("\n");
}

/*
 *	ksh special - the select command processing section
 *	print the args in column form - assuming that we can
 */
#define	COLARGS		20

static char *
do_selectargs(ap, secondtime)
	register char **ap;
	char	*secondtime;
{
	char *rv;

	register int i, c;
	static char *replybase = NULL;
	static int replymax;
	static int repct;
	static int argct;
	
	/*
	 * deal with REPLY variable
	 */
	if (replybase == NULL) {
		replybase = alloc(64, APERM);
		replymax = 64;
	}

	if (!secondtime)
		argct = pr_menu(ap, 0);
	
	/*
	 * and now ask for an answer
	 */
retry:
	shellf("%s", strval(global("PS3")));
	fflush(shlout);
	repct = 0;
	i = 0;
	rv = NULL;
	while ((c = selread()) != EOF) {
		if (c == -2) {
			shellf("Read error\n");
			rv = (char*)1;
			break;
		}
		if (repct+1 >= replymax)
		{	replymax += 64;
			replybase = aresize(replybase, replymax, APERM);
		}
		if (i >= 0 && c >= '0' && c <= '9') {
			replybase[repct++] = c;
			if (i >= 0)
				i = i*10 + (c - '0');
		}
		else
		if (c == '\n') {
			if (repct == 0) {
				pr_menu(ap, 1);
				goto retry;
			}
				
			if (i >= 1 && i <= argct)
				rv = ap[i-1];
			else	rv = "";
			break;
		} else
			i = -1,	replybase[repct++] = c;
	}
	if (rv == NULL) {
		shellf("\n");
		rv = (char *)1;
	}
	replybase[repct] = '\0';
	setstr(global("REPLY"), replybase);
	return rv;
}

/*
 *	print a select style menu
 */
int
pr_menu(ap, usestored)
	register char **ap;
	int usestored;
{
	register char **pp;
	register i, j;
	register int ix;
	static int argct;
	static int nwidth;
	static int dwidth;
	static int ncols;
	static int nrows;

	if (usestored == 0) {
		/*
		 * get dimensions of the list
		 */
		for (argct = 0, nwidth = 0, pp = ap; *pp; argct++, pp++) {
			i = strlen(*pp);
			nwidth = (i > nwidth) ? i : nwidth;
		}
		/*
		 * we will print an index of the form
		 *	%d)
		 * in front of each entry
		 * get the max width of this
		 */
		for (i = argct, dwidth = 1; i >= 10; i /= 10)
			dwidth++;

		if (argct < COLARGS)
			ncols = 1, nrows = argct;
		else {
			ncols = x_cols/(nwidth+dwidth+3);
			nrows = argct/ncols;
			if (argct%ncols) nrows++;
			if (ncols > nrows)
			i = nrows, nrows = ncols, ncols = 1;
		}

	}
	/*
	 * display the menu
	 */
	for (i = 0; i < nrows; i++) {
		for (j = 0; j < ncols; j++) {
			ix = j*nrows + i;
			if (ix < argct)
				shellf("%*d) %-*.*s ", dwidth, ix+1, nwidth, nwidth, ap[ix]);
			}
		shellf("\n");
	}
	return argct;
}

static int
selread()
{	char c;
	register int	rv;
	
	switch (read(0, &c, 1)) {
	   case 1:
		rv = c&0xff;
		break;
	   case 0:
		rv = EOF;
		break;
	   case -1:
		rv = -2;
		break;
	}
	return rv;
}
