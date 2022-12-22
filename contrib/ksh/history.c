/*
 * command history
 *
 * only implements in-memory history.
 */

#ifndef lint
static char *RCSid = "$Id: history.c,v 1.2 1993/12/21 01:43:50 polk Exp $";
#endif
/*
 *	This file contains
 *	a)	the original in-memory history  mechanism
 *	b)	a simple file saving history mechanism done by  sjg@zen
 *		define EASY_HISTORY to get this
 *	c)	a more complicated mechanism done by  pc@hillside.co.uk
 *		that more closely follows the real ksh way of doing
 *		things. You need to have the mmap system call for this
 *		to work on your system
 */

#include "config.h"
#include "stdh.h"

#ifdef EASY_HISTORY

#include <errno.h>
#include <setjmp.h>
#include "sh.h"

static FILE *hist_fh = NULL;
static FILE *hist_open ARGS((char *mode));
#ifndef HISTFILE
# define HISTFILE ".pdksh_hist"
#endif

#else
/*	Defines and includes for the complicated case */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <errno.h>
#include <setjmp.h>
#include "sh.h"

/*
 *	variables for handling the data file
 */
static char	*hname;
static int	histfd;
static int	hsize;
static int	hstarted;

static int hist_count_lines ARGS((unsigned char *, int));
static int hist_shrink ARGS((unsigned char *, int));
static unsigned char *hist_skip_back ARGS((unsigned char *,int *,int));
static void histload ARGS((Source *, unsigned char *, int));
static void histinsert ARGS((Source *, int, unsigned char *));
static void writehistfile ARGS((int, char *));
static int sprinkle ARGS((int));

#ifdef MAP_FILE
#define	MAP_FLAGS	MAP_FILE|MAP_PRIVATE
#else
#define	MAP_FLAGS	MAP_PRIVATE
#endif

#endif	/* of EASY_HISTORY */

  
char   *histrpl();
char  **current;
int	curpos;

c_fc(wp)
	register char **wp;
{
	register char *id;
	FILE *f;
	struct temp *tf;
	register char **hp;
	char **hbeg, **hend;
	char *p, *cmd = NULL;
	int lflag = 0, nflag = 0, sflag = 0, rflag = 0, gflag = 0;
	int done = 0;
	void histbackup();

	for (wp++; (id = *wp) != NULL && *id++ == '-' && !done; wp++)
		while (*id && !done) {
			switch (*id++) {
			  case 'l':
				lflag++;
				break;
			  case 'n':
				nflag++;
				break;
			  case 'r':
				rflag++;
				break;
			  case 'g':
				gflag++;
				break;
			  case 'e':
				if (++wp && (p = *wp)) {
					if (p[0] == '-' && !p[1]) {
						sflag++;
					} else {
						cmd = alloc((size_t)(strlen(p)+4),ATEMP);
						strcpy(cmd, p);
						strcat(cmd, " $_");
					}
				} else
					errorf("argument expected\n");
				id = "";
				break;
			  default:
				wp--;
				done++;
				break;
			}
		}

	if (sflag) {
		char *pat = NULL, *rep = NULL;

		hp = histptr - 1;
		while ((id = *wp++) != NULL) {
			/* todo: multiple substitutions */
			if ((p = strchr(id, '=')) != NULL) {
				pat = id;
				rep = p;
				*rep++ = '\0';
			} else
				hp = histget(id);
		}

		if (hp == NULL || hp < history)
			errorf("cannot find history\n");
		if (pat == NULL)
			strcpy(line, *hp);
		else
			histrpl(*hp, pat, rep, gflag);
		histbackup();
#ifdef EASY_HISTORY
		histsave(line); 
#else
		histsave(source->line+1, line, 1);
#endif
		histpush--; 
		line[0] = '\0';
		return 0;
	}

	if (*wp != NULL) {
		hbeg = histget(*wp++); /* first */
		if (*wp != NULL)
			hend = histget(*wp++); /* last */
		else if (lflag)
			hend = histptr;
		else
			hend = hbeg;
	} else {
		if (lflag)
			hbeg = histptr - 16, hend = histptr;
		else
			hbeg = hend = histptr - 1;
		if (hbeg < history)
			hbeg = history;
	}
	if (hbeg == NULL || hend == NULL)
		errorf("can't find history\n");

	if (lflag)
		f = stdout;
	else {
		nflag++;
		tf = maketemp(ATEMP);
		tf->next = e.temps; e.temps = tf;
		f = fopen(tf->name, "w");
		if (f == NULL)
			errorf("cannot create temp file %s", tf->name);
		setvbuf(f, (char *)NULL, _IOFBF, BUFSIZ);
	}

	for (hp = (rflag ? hend : hbeg); rflag ? (hp >= hbeg) : (hp <= hend);
	      rflag ? hp-- : hp++) {
		if (!nflag)
			fprintf(f, "%3d: ", source->line - (int)(histptr-hp));
		fprintf(f, "%s\n", *hp);
	}

	if (lflag)
		return 0;
	else
		fclose(f);

	setstr(local("_"), tf->name);
	if (cmd) {
		command(cmd); /* edit temp file */
		afree(cmd, ATEMP);
	} else
		command("${FCEDIT:-/bin/ed} $_");

	f = fopen(tf->name, "r");
	if (f == NULL)
		errorf("cannot open temp file %s\n", tf->name);
	setvbuf(f, (char *)NULL, _IOFBF, BUFSIZ);
	/* we push the editted lines onto the history list */
	while (fgets(line, sizeof(line), f) != NULL) {
#ifdef EASY_HISTORY
		histsave(line); 
#else
		histsave(source->line, line, 1); 
#endif
		histpush--; 
	}
	line[0] = '\0';
	fclose(f);

	return 0;
}

/******************************/
/* Back up over last histsave */
/******************************/
void
histbackup()
{
	static int last_line = -1;

	if (histptr > history && last_line != source->line) { 
		source->line--;
		afree((void*)*histptr, APERM);
		histptr--;
		last_line = source->line;
	}
}

/*
 * get pointer to history given pattern
 * pattern is a number or string
 */
char **
histget(str)
	char *str;
{
	register char **hp = NULL;

	if (*str == '-')
		hp = histptr + getn(str);
	else
	if (digit(*str))
		hp = histptr + (getn(str) - source->line);
	else 
	if (*str == '?') {	/* unanchored match */
		for (hp = histptr-1; hp >= history; hp--)
			if (strstr(*hp, str+1) != NULL)
				break;
	} else {		/* anchored match */
		for (hp = histptr; hp >= history; hp--)
			if (strncmp(*hp, str, strlen(str)) == 0)
				break;
	}

	return (history <= hp && hp <= histptr) ? hp : NULL;
}

char *
histrpl(s, pat, rep, global)
	char *s;
	char *pat, *rep;
	int global;
{
	char *s1, *p, *last = NULL;
	int len = strlen(pat);

	if (strlen(s) - strlen(pat) + strlen(rep) >= LINE)
		errorf("substitution too long\n");
	line[0] = '\0';
	p = line;
	while (s1 = strstr(s, pat)) {
		strncpy(p, s, s1 - s);		/* first part */
		strcpy(p + (s1 - s), rep);	/* replacement */
		s = s1 + len;
		last = s1;
		p = strchr(p, 0);
		if (!global)
			s = "";
	}
	if (last)
		strcpy(p, last + len);		/* last part */
	else
		errorf("substitution failed\n");
	return line;
}

/*
 * Return the current position.
 */
char **
histpos()
{
	return current;
}

int
histN()
{
	return curpos;
}

int
histnum(n)
{
	int	last = histptr - history;

	if (n < 0 || n >= last) {
		current = histptr;
		curpos = last;
		return last;
	}  else {
		current = &history[n];
		curpos = n;
		return n;
	}
}

/*
 * This will become unecessary if histget is modified to allow
 * searching from positions other than the end, and in either 
 * direction.
 */
char *
findhist(start, fwd, str)
	int	start;
	int	fwd;
	char 	*str;
{
	int	 pos = start;
	char	 *line, *last;

	/* XXX check that we are valid after this */
	if (fwd)
		pos++;
	else
		pos--;
	histnum(pos);
	line = *histpos();
	do {
		last = line;
		if (strstr(line, str) != 0) {
			/* keep position current */
			return (line);
		}
		if (fwd)
			pos++;
		else
			pos--;
		histnum(pos);
		line = *histpos();
	} while (line && *line && line != last && pos>0);

	histnum(start);
	if (pos <= 0)
		return (char*)-1; /* TODO */
	return NULL;
}

#ifdef EASY_HISTORY
/*
 * save command in history
 */
void
histsave(cmd)
	char *cmd;
{
	register char **hp = histptr;
	char *cp;

	if (++hp >= history + HISTORY) { /* remove oldest command */
		afree((void*)*history, APERM);
		for (hp = history; hp < history + HISTORY - 1; hp++)
			hp[0] = hp[1];
	}
	*hp = strsave(cmd, APERM);
	if ((cp = strchr(*hp, '\n')) != NULL)
		*cp = '\0';
	histptr = hp;
}

/*
 * 92-04-25 <sjg@zen>
 * A simple history file implementation.
 * At present we only save the history when we exit.
 * This can cause problems when there are multiple shells are 
 * running under the same user-id.  The last shell to exit gets 
 * to save its history.
 */
void
hist_init(s)
  Source *s;
{
  static int once = 0;
  FILE *fh;
  
  if (once++)
    return;

  if (fh = hist_open("r"))
  {
    while (fgets(line, sizeof(line), fh) != NULL)
    {
      histsave(line); 
      s->line++;
    }
    line[0] = '\0';
    fclose(fh);
#if 0	/* this might be a good idea? */
    hist_fh = hist_open("a");
#endif
  }
  
}

void
init_histvec()
{	;	}

/*
 * save our history.
 * We check that we do not have more than we are allowed.
 * If the history file is read-only we do nothing.
 * Handy for having all shells start with a useful history set.
 */

void
hist_finish()
{
  static int once = 0;
  FILE *fh;
  register int i, mx;
  register char **hp, *mode = "w";
  
  if (once++)
    return;
  if ((mx = atoi(strval(global("HISTSIZE")))) > HISTORY || mx <= 0)
    mx = HISTORY;
  /* check how many we have */
  i = histptr - history;
  if (i >= mx)
  {
    hp = &histptr[-mx];
  }
  else
  {
    hp = history;
  }
  if (fh = hist_open(mode))
  {
    for (i = 0; i < mx && hp[i]; i++)
      fprintf(fh, "%s\n", hp[i]);
    fclose(fh);
  }
}


/*
 * simply grab the nominated history file.
 */
static FILE *
hist_open(mode)
  char *mode;
{
  register char *rcp;
  FILE *fh;
  char name[128];
  
  if ((rcp = strval(global("HISTFILE"))) == NULL || *rcp == '\0')
  {
    (void) sprintf(name, "%s/%s", strval(global("HOME")), HISTFILE);
    rcp = name;
  }
  return fopen(rcp, mode);
}

#else /* EASY_HISTORY */

/*
 *	Routines added by Peter Collinson BSDI(Europe)/Hillside Systems to
 *	a) permit HISTSIZE to control number of lines of history stored
 *	b) maintain a physical history file
 *
 *	It turns out that there is a lot of ghastly hackery here
 */


/*
 * save command in history
 */
void
histsave(lno, cmd, dowrite)
	int lno;
	char *cmd;
	int dowrite;
{
	register char **hp;
	char *cp;

	cmd = strsave(cmd, APERM);
	if ((cp = strchr(cmd, '\n')) != NULL)
		*cp = '\0';

	if (histfd && dowrite)
		writehistfile(lno, cmd);

	hp = histptr;
		
	if (++hp >= history + histsize) { /* remove oldest command */
		afree((void*)*history, APERM);
		for (hp = history; hp < history + histsize - 1; hp++)
			hp[0] = hp[1];
	}
	*hp = cmd;
	histptr = hp;
}

/*
 *	set history
 *	this means reallocating the dataspace
 */
void
sethistsize(n)
	int n;
{
	int	offset;
	
	if (n != histsize) {
		offset = histptr - history;
		history = (char **)aresize(history, n*sizeof(char *), APERM);

		if (n < histsize && offset > histsize)
			offset = histsize;

		histsize = n;
		histptr = history + offset;
	}
}

/*
 *	set history file
 *	This can mean reloading/resetting/starting history file
 *	maintenance
 */
void
sethistfile(name)
	char *name;
{
	/* if not started then nothing to do */
	if (hstarted == 0)
		return;

	/* if the name is the same as the name we have */
	if (hname && strcmp(hname, name) == 0)
		return;

	/*
	 * its a new name - possibly
	 */
	if (histfd) {
		/* yes the file is open */
		(void) close(histfd);
		histfd = 0;
		hsize = 0;
		afree(hname, APERM);
		hname = NULL;
		/* let's reset the history */
		histptr = history - 1;
		source->line = 0;
	}

	hist_init(source);
}

/*
 *	initialise the history vector
 */
void
init_histvec()
{
	if (history == (char **)NULL) {
		history = (char **)alloc(histsize*sizeof (char *), APERM);
		histptr = history-1;
	}
}
	
/*
 *	Write history data to a file nominated by HISTFILE
 *	if HISTFILE is unset then history still happens, but
 *	the data is not written to a file
 *	All copies of ksh looking at the file will maintain the
 *	same history. This is ksh behaviour.
 *
 *	This stuff uses mmap()
 *	if your system ain't got it - then you'll have to undef HISTORYFILE
 */
	
/*
 *	Open a history file
 *	Format is:
 *	Bytes 1, 2: HMAGIC - just to check that we are dealing with
 *		    the correct object
 *	Then follows a number of stored commands
 *	Each command is
 *	<command byte><command number(4 bytes)><bytes><null>
 */
#define HMAGIC1		0xab
#define HMAGIC2		0xcd
#define COMMAND		0xff

void
hist_init(s)
	Source *s;
{
	unsigned char	*base;
	int	lines;
	int	bytes;
	int	fd;
	
	hstarted = 1;
	
	if (flag[FTALKING] == 0)
		return;

	hname = strval(global("HISTFILE"));
	if (hname == NULL)
		return;
	hname = strsave(hname, APERM);

  retry:
	/* we have a file and are interactive */
	if ((fd = open(hname, O_RDWR|O_CREAT|O_APPEND, 0600)) < 0)
		return;

	histfd = fcntl(fd, F_DUPFD, FDBASE);
	close(fd);
	
	(void) fd_clexec(histfd);

	(void) flock(histfd, LOCK_EX);

	hsize = lseek(histfd, 0L, L_XTND);

	if (hsize == 0) {
		/* add magic */
		if (sprinkle(histfd)) {
			hist_finish();
			return;
		}
	}
	else if (hsize > 0) {
		/*
		 * we have some data
		 */
		base = (unsigned char *)mmap(0, hsize, PROT_READ, MAP_FLAGS, histfd, 0);
		/*
		 * check on its validity
		 */
		if ((int)base == -1 || *base != HMAGIC1 || base[1] != HMAGIC2) {
			if ((int)base !=  -1)
				munmap((caddr_t)base, hsize);
			hist_finish();
			unlink(hname);
			goto retry;
		}
		if (hsize > 2) {
			lines = hist_count_lines(base+2, hsize-2);
			if (lines > histsize) {
				/* we need to make the file smaller */
				if (hist_shrink(base, hsize))
					unlink(hname);
				munmap((caddr_t)base, hsize);
				hist_finish();
				goto retry;
			}
		}
		histload(s, base+2, hsize-2);
		munmap((caddr_t)base, hsize);
	}
	(void) flock(histfd, LOCK_UN);
	hsize = lseek(histfd, 0L, L_XTND);
}

typedef enum state {
	shdr,		/* expecting a header */
	sline,		/* looking for a null byte to end the line */
	sn1,		/* bytes 1 to 4 of a line no */
	sn2, sn3, sn4,
} State;

static int
hist_count_lines(base, bytes)
	register unsigned char *base;
	register int bytes;
{
	State state = shdr;
	register lines = 0;
	
	while (bytes--) {
		switch (state)
		{
		case shdr:
			if (*base == COMMAND)
				state = sn1;
			break;
		case sn1:
			state = sn2; break;
		case sn2:
			state = sn3; break;
		case sn3:
			state = sn4; break;
		case sn4:
			state = sline; break;
		case sline:
			if (*base == '\0')
				lines++, state = shdr;
		}
		base++;
	}
	return lines;
}

/*
 *	Shrink the history file to histsize lines
 */
static int
hist_shrink(oldbase, oldbytes)
	unsigned char *oldbase;
	int oldbytes;
{
	int fd;
	char	nfile[1024];
	struct	stat statb;
	unsigned char *nbase = oldbase;
	int nbytes = oldbytes;

	nbase = hist_skip_back(nbase, &nbytes, histsize);
	if (nbase == NULL)
		return 1;
	if (nbase == oldbase)
		return 0;
	
	/*
	 *	create temp file
	 */
	(void) sprintf(nfile, "%s.%d", hname, getpid());
	if ((fd = creat(nfile, 0600)) < 0)
		return 1;

	if (sprinkle(fd)) {
		close(fd);
		unlink(nfile);
		return 1;
	}
	if (write(fd, nbase, nbytes) != nbytes) {
		close(fd);
		unlink(nfile);
		return 1;
	}
	/*
	 *	worry about who owns this file
	 */
	if (fstat(histfd, &statb) >= 0)
		fchown(fd, statb.st_uid, statb.st_gid);
	close(fd);
	
	/*
	 *	rename
	 */
	if (rename(nfile, hname) < 0)
		return 1;
	return 0;
}
	

/*
 *	find a pointer to the data `no' back from the end of the file
 *	return the pointer and the number of bytes left
 */
static unsigned char *
hist_skip_back(base, bytes, no)
	unsigned char *base;
	int *bytes;
	int no;
{
	register int lines = 0;
	register unsigned char *ep;

	

	for (ep = base + *bytes; ep > base; ep--)
	{
		while (*ep != COMMAND) {
			if (--ep == base)
				break;
		}
		if (++lines == no) {
			*bytes = *bytes - ((char *)ep - (char *)base);
			return ep;
		}
	}
	if (ep > base)
		return base;
	return NULL;
}

/*
 *	load the history structure from the stored data
 */
static void
histload(s, base, bytes)
	Source *s;
	register unsigned char *base;
	register int bytes;
{
	State state;
	int	lno;
	unsigned char	*line;
	
	for (state = shdr; bytes-- > 0; base++) {
		switch (state) {
		case shdr:
			if (*base == COMMAND)
				state = sn1;
			break;
		case sn1:
			lno = (((*base)&0xff)<<24);
			state = sn2;
			break;
		case sn2:
			lno |= (((*base)&0xff)<<16);
			state = sn3;
			break;
		case sn3:
			lno |= (((*base)&0xff)<<8);
			state = sn4;
			break;
		case sn4:
			lno |= (*base)&0xff;
			line = base+1;
			state = sline;
			break;
		case sline:
			if (*base == '\0') {
				/* worry about line numbers */
				if (histptr >= history && lno-1 != s->line) {
					/* a replacement ? */
					histinsert(s, lno, line);
				}
				else {
					s->line = lno;
					histsave(lno, (char *)line, 0);
				}
				state = shdr;
			}
		}
	}
}
				
/*
 *	Insert a line into the history at a specified number
 */
static void
histinsert(s, lno, line)
	Source *s;
	int lno;
	unsigned char *line;
{
	register char **hp;
	
	if (lno >= s->line-(histptr-history) && lno <= s->line) {
		hp = &histptr[lno-s->line];
		if (*hp)
			afree((void*)*hp, APERM);
		*hp = strsave((char *)line, APERM);
	}
}

/*
 *	write a command to the end of the history file
 *	This *MAY* seem easy but it's also necessary to check
 *	that the history file has not changed in size.
 *	If it has - then some other shell has written to it
 *	and we should read those commands to update our history
 */
static void
writehistfile(lno, cmd)
	int lno;
	char *cmd;
{
	int	sizenow;
	unsigned char	*base;
	unsigned char	*new;
	int	bytes;
	char	hdr[5];
	
	(void) flock(histfd, LOCK_EX);
	sizenow = lseek(histfd, 0L, L_XTND);
	if (sizenow != hsize) {
		/*
		 *	Things have changed
		 */
		if (sizenow > hsize) {
			/* someone has added some lines */
			bytes = sizenow - hsize;
			base = (unsigned char *)mmap(0, sizenow, PROT_READ, MAP_FLAGS, histfd, 0);
			if ((int)base == -1)
				goto bad;
			new = base + hsize;
			if (*new != COMMAND) {
				munmap((caddr_t)base, sizenow);
				goto bad;
			}
			source->line--;
			histload(source, new, bytes);
			source->line++;
			lno = source->line;
			munmap((caddr_t)base, sizenow);
			hsize = sizenow;
		} else {
			/* it has shrunk */
			/* but to what? */
			/* we'll give up for now */
			goto bad;
		}
	}
	/*
	 *	we can write our bit now
	 */
	hdr[0] = COMMAND;
	hdr[1] = (lno>>24)&0xff;
	hdr[2] = (lno>>16)&0xff;
	hdr[3] = (lno>>8)&0xff;
	hdr[4] = lno&0xff;
	(void) write(histfd, hdr, 5);
	(void) write(histfd, cmd, strlen(cmd)+1);
	hsize = lseek(histfd, 0L, L_XTND);
	(void) flock(histfd, LOCK_UN);
	return;
bad:
	hist_finish();
}

void
hist_finish()
{
	(void) flock(histfd, LOCK_UN);
	(void) close(histfd);
	histfd = 0;
}

/*
 *	add magic to the history file
 */
static int
sprinkle(fd)
	int fd;
{
	static char mag[] = { HMAGIC1, HMAGIC2 };

	return(write(fd, mag, 2) != 2);
}

#endif
