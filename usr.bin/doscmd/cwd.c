/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	Krystal $Id: cwd.c,v 1.2 1994/01/31 00:50:43 polk Exp $ */
#include <sys/types.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "dos.h"
#include "cwd.h"

typedef struct {
    u_char		*path;
    u_char		*cwd;
    int			len;
    int			maxlen;
    int			read_only:1;
} Path_t;

typedef struct Name_t {
    u_char		*real;
    struct Name_t	*next;
    u_char		name[8];
    u_char		ext[3];
} Name_t;


#define	MAX_DRIVE	26

static Path_t paths[MAX_DRIVE] = { 0, };
static Name_t *names;

/*
 * Initialize the drive to be based at 'base' in the BSDI filesystem
 */
void
init_path(int drive, u_char *base, u_char *dir)
{
    Path_t *d;

    if (drive < 0 || drive >= MAX_DRIVE)
	return;

    d = &paths[drive];

    if (d->path)
	free(d->path);

    if ((d->path = ustrdup(base)) == NULL)
	fatal("strdup in init_path for %c:%s: %s", drive + 'A', base,
	      strerror(errno));

    if (d->maxlen < 2) {
	d->maxlen = 128;
	if ((d->cwd = (u_char *)malloc(d->maxlen)) == NULL)
	    fatal("malloc in init_path for %c:%s: %s", drive + 'A', base,
		  strerror(errno));
    }

    d->cwd[0] = '\\';
    d->cwd[1] = 0;
    d->len = 1;
    if (dir) {
	if (ustrncmp(base, dir, ustrlen(base)) == 0)
		dir += ustrlen(base);
	while (*dir == '/')
	    ++dir;

    	while (*dir) {
	    u_char dosname[14];
	    u_char realname[256];
	    u_char *r = realname;;

	    while ((*r = *dir) && *dir++ != '/') {
		++r;
	    }
    	    *r = 0;
	    while (*dir == '/')
		++dir;

	    real_to_dos(realname, dosname);
	    if (dos_setcwd(drive, dosname)) {
		fprintf(stderr, "Failed to CD to directory %s in %s\n",
				 dosname, d->cwd);
	    }
	}
    }
}

/*
 * Mark this drive as read only
 */
void
dos_makereadonly(int drive)
{
    if (drive < 0 || drive >= MAX_DRIVE)
	return;
    paths[drive].read_only = 1;
}

/*
 * Return read-only status of drive
 */
int
dos_readonly(int drive)
{
    if (drive < 0 || drive >= MAX_DRIVE)
	return(0);
    return(paths[drive].read_only);
}

/*
 * Return DOS's idea of the CWD for drive
 * Return 0 if the drive specified is not mapped (or bad)
 */
u_char *
dos_getcwd(int drive)
{
    if (drive < 0 || drive >= MAX_DRIVE)
	return(0);
    return(paths[drive].cwd);
}

/*
 * Return DOS's idea of the CWD for drive
 * Return 0 if the drive specified is not mapped (or bad)
 */
u_char *
dos_getpath(int drive)
{
    if (drive < 0 || drive >= MAX_DRIVE)
	return(0);
    return(paths[drive].path);
}

/*
 * Fix up a DOS path name.  Strip out all '.' and '..' entries, turn
 * '/' into '\\' and convert all lowercase to uppercase.
 * Returns 0 on success or DOS errno
 */
int
dos_makepath(int drive, u_char *where, u_char *newpath)
{
    u_char **dirs;
    u_char *np;
    Path_t *d;
    u_char tmppath[1024];

    if (drive < 0 || drive >= MAX_DRIVE)
	return(DISK_DRIVE_INVALID);

    d = &paths[drive];

    if (!d->cwd)
	return(DISK_DRIVE_INVALID);

    np = newpath;
    if (*where != '\\' && *where != '/') {
	ustrcpy(tmppath, d->cwd);
	if (d->cwd[1])
	    ustrcat(tmppath, (u_char *)"/");
	ustrcat(tmppath, where);
    } else {
	ustrcpy(tmppath, where);
    }

    dirs = get_entries(tmppath);
    if (**dirs != '/' && **dirs != '\\') {
	fprintf(stderr, "%s:%d: This should not happen!\n", __FILE__, __LINE__);
	return(DISK_DRIVE_INVALID);
    }

    np = newpath;

    while (*dirs) {
	u_char *dir = *dirs++;
	if (*dir == '/' || *dir == '\\') {
	    np = newpath + 1;
	    newpath[0] = '\\';
	} else if (dir[0] == '.' && dir[1] == 0) {
	    ;
	} else if (dir[0] == '.' && dir[1] == '.' && dir[2] == '\0') {
	    while (np[-1] != '/' && np[-1] != '\\')
		--np;
    	    if (np - 1 > newpath)
		--np;
	} else {
    	    if (np[-1] != '\\')
		*np++ = '\\';
	    while (*np = *dir++)
		++np;
    	}
    }
    *np = 0;

    return(0);
}

/*
 * Set DOS's idea of the CWD for drive to be where.
 * Returns DOS errno on failuer.
 */
int
dos_setcwd(int drive, u_char *where)
{
    u_char newpath[1024];
    u_char realpath[1024];
    u_char *np;
    struct stat sb;
    Path_t *d;
    int e;

    if (drive < 0 || drive >= MAX_DRIVE)
	return(DISK_DRIVE_INVALID);

    d = &paths[drive];

    if (!d->cwd)
	return(DISK_DRIVE_INVALID);
    if (e = dos_makepath(drive, where, newpath)) {
	return(e);
    }

    if (e = dos_to_real_path(drive, newpath, realpath)) {
	return(e);
    }

    if (ustat(realpath, &sb) == -1 || !S_ISDIR(sb.st_mode)) {
	return(PATH_NOT_FOUND);
    }
    if (uaccess(realpath, R_OK | X_OK)) {
	return(PATH_NOT_FOUND);
    }

    d->len = ustrlen(newpath);

    if (d->len + 1 > d->maxlen) {
	free(d->cwd);
	d->maxlen = d->len + 1 + 32;

	if ((d->cwd = (u_char *)malloc(d->maxlen)) == NULL)
	    fatal("malloc in dos_setcwd for %c:%s: %s", drive + 'A', newpath,
		  strerror(errno));
    }
    ustrcpy(d->cwd, newpath);
    return(0);
}

/*
 * Given a DOS path dospath and a drive, convert it to a BSDI pathname
 * and store the result in realpath.
 * Return DOS errno on failure.
 */
int
dos_to_real_path(int drive, u_char *dospath, u_char *realpath)
{
    Path_t *d;
    u_char newpath[1024];
    u_char *rp;
    int e;
    u_char **dirs;
    u_char *dir;


    if (drive < 0 || drive >= MAX_DRIVE)
	return(DISK_DRIVE_INVALID);

    d = &paths[drive];

    if (!d->path)
	return(DISK_DRIVE_INVALID);

    if (e = dos_makepath(drive, dospath, newpath)) {
	return(e);
    }

    ustrcpy(realpath, d->path);

    rp = realpath;
    while (*rp)
	++rp;

    dirs = get_entries(newpath);

    /*
     * Skip the leading /
     * There are no . or .. entries to worry about either
     */

    while (dir = *++dirs) {
	*rp++ = '/';
	dos_to_real(dir, rp);
	while (*rp)
	    ++rp;
    }
    return(0);
}

/*
 * Provide a few istype() style functions.
 * isvalid:	True if the character is a valid DOS filename character
 * isdot:	True if '.'
 * isslash:	True if '/' or '\'
 *
 * 0 - invalid
 * 1 - okay
 * 2 - *
 * 3 - dot
 * 4 - slash
 * 5 - colon
 * 6 - ?
 * 7 - lowercase
 */
u_char cattr[256] = {
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,	/* 0x00 */
    0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,	/* 0x10 */
    0, 1, 0, 1, 1, 1, 1, 1,  1, 1, 2, 0, 0, 1, 3, 4,	/* 0x20 */
    1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 5, 0, 0, 0, 0, 6,	/* 0x30 */
    1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,	/* 0x40 */
    1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 0, 4, 0, 1, 1,	/* 0x50 */
    1, 7, 7, 7, 7, 7, 7, 7,  7, 7, 7, 7, 7, 7, 7, 7,	/* 0x60 */
    7, 7, 7, 7, 7, 7, 7, 7,  7, 7, 7, 1, 0, 1, 1, 0,	/* 0x70 */
    1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,	/* 0x80 */
    1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
};

inline
isvalid(unsigned c)
{
    return(cattr[c & 0xff] == 1);
}

inline
isdot(unsigned c)
{
    return (cattr[c & 0xff] == 3);
}

inline
isslash(unsigned c)
{
    return (cattr[c & 0xff] == 4);
}

/*
 * Given a real component, compute the DOS component.
 */
void
real_to_dos(u_char *real, u_char *dos)
{
    int ncnt = 0;
    int ecnt = 0;
    Name_t *n = names;
    Name_t *nn;
    u_char *p;
    u_char *e;
    u_char nm[8], *nmp = nm;
    u_char ex[3], *exp = ex;
    int echar = '0';
    int nchar = '0';

    p = real;

    if (real[0] == '.' && (real[1] == '\0'
		        || (real[1] == '.' && real[2] == '\0'))) {
	ustrcpy(dos, real);
	return;
    }
    while (n) {
	if (!ustrcmp(real, n->real)) {
	    if (n->ext[0])
		sprintf((char *)dos, "%.8s.%.3s", n->name, n->ext);
	    else
		sprintf((char *)dos, "%.8s", n->name);
	    return;
    	}
	n = n->next;
    }

    while (isvalid(*p)) {
	if (ncnt < 8)
	    *nmp++ = *p;
    	++ncnt;
	++p;
    }
    if (isdot(*p)) {
	while (isvalid(*++p)) {
	    if (ecnt < 8)
		*exp++ = *p;
	    ++ecnt;
	}
    }
    if (!*p && ncnt <= 8 && ecnt <= 3) {
	while (nmp > nm && nmp[-1] == ' ')
	    --nmp;
	while (exp > ex && exp[-1] == ' ')
	    --exp;
	if (nmp < nm + 8)
	    *nmp = 0;
	if (exp < ex + 3)
	    *exp = 0;
	n = names;
	while (n) {
	    if (ustrncmp(n->name, nm, 8) == 0 && ustrncmp(n->ext, ex, 3) == 0) {
		break;
	    }
	    n = n->next;
	}
	if (n == 0) {
	    ustrcpy(dos, real);
	    return;
	}
    }

    n = (Name_t *)malloc(sizeof(Name_t));

    if (!n)
	fatal("malloc in real_to_dos: %s\n", strerror(errno));

    n->real = ustrdup(real);

    if (!n->real)
	fatal("strdup in real_to_dos: %s\n", strerror(errno));

    p = real;
    e = ustrrchr(real, '.');
    if (e) {
	u_char *t = e;
	ecnt = 0;
    	while (ecnt < 3 && *++t) { 
	    if (isvalid(*t)) {
	    	n->ext[ecnt] = *t;
    	    } else if (islower(*t)) {
	    	n->ext[ecnt] = toupper(*t);
    	    } else {
		n->ext[ecnt] = (*t |= 0x80);
    	    }
	    ++ecnt;
    	}
	if (ecnt < 3)
	    n->ext[ecnt] = 0;
    } else {
	e = real;
	while (*e)
	    ++e;
	n->ext[ecnt = 0] = 0;
    }
    if (ecnt < 3) {
	int x = ecnt;
    	while (x < 3)
	    n->ext[x++] = 0;
    }
    ncnt = 0;
    while (p < e && ncnt < 8) {
	if (isvalid(*p)) {
	    n->name[ncnt] = *p;
	} else if (islower(*p)) {
	    n->name[ncnt] = toupper(*p);
	} else {
	    n->name[ncnt] = (*p |= 0x80);
	}
	++p;
	++ncnt;
    }
    if (ncnt < 8) {
	int x = ncnt;
    	while (x < 8)
	    n->name[x++] = 0;
    }

    for (;;) {
	nn = names;
	while (nn) {
	    if (ustrncmp(n->name, nn->name, 8) == 0 &&
		ustrncmp(n->ext, nn->ext, 3) == 0) {
		    break;
	    }
	    nn = nn->next;
	}
    	if (!nn)
	    break;
	/*	
	 * Dang, this name was already in the cache.
	 * Let's munge it a little and try again.
	 */
	if (ecnt < 3) {
	    n->ext[ecnt] = echar;
	    if (echar == '9') {
		echar = 'A';
	    } else if (echar == 'Z') {
		++ecnt;
		echar = '0';
	    } else {
		++echar;
	    }
    	} else if (ncnt < 8) {
	    n->name[ncnt] = nchar;
	    if (nchar == '9') {
		nchar = 'A';
	    } else if (nchar == 'Z') {
		++ncnt;
		nchar = '0';
	    } else {
		++nchar;
	    }
    	} else if (n->ext[2] < 'Z')
	    n->ext[2]++;
    	else if (n->ext[1] < 'Z')
	    n->ext[1]++;
    	else if (n->ext[0] < 'Z')
	    n->ext[0]++;
    	else if (n->name[7] < 'Z')
	    n->name[7]++;
    	else if (n->name[6] < 'Z')
	    n->name[6]++;
    	else if (n->name[5] < 'Z')
	    n->name[5]++;
    	else if (n->name[4] < 'Z')
	    n->name[4]++;
    	else if (n->name[3] < 'Z')
	    n->name[3]++;
    	else if (n->name[2] < 'Z')
	    n->name[2]++;
    	else if (n->name[1] < 'Z')
	    n->name[1]++;
    	else if (n->name[0] < 'Z')
	    n->name[0]++;
	else
	    break;
    }

    if (n->ext[0])
	sprintf((char *)dos, "%.8s.%.3s", n->name, n->ext);
    else
	sprintf((char *)dos, "%.8s", n->name);
    n->next = names;
    names = n;
}


/*
 * Given a DOS component, compute the REAL component.
 */
void
dos_to_real(u_char *dos, u_char *real)
{
    int ncnt = 0;
    int ecnt = 0;
    u_char name[8];
    u_char ext[8];
    Name_t *n = names;

    while (ncnt < 8 && (isvalid(*dos) || islower(*dos))) {
	name[ncnt++] = islower(*dos) ? toupper(*dos) : *dos;
	++dos;
    }
    if (ncnt < 8)
	name[ncnt] = 0;

    if (isdot(*dos)) {
	while (ecnt < 3 && (isvalid(*++dos) || islower(*dos))) {
	    ext[ecnt++] = islower(*dos) ? toupper(*dos) : *dos;
    	}
    }
    if (ecnt < 3)
	ext[ecnt] = 0;

    while (n) {
	if (!ustrncmp(name, n->name, 8) && !ustrncmp(ext, n->ext, 3)) {
	    ustrcpy(real, n->real);
	    return;
	}
	n = n->next;
    }

    if (ext[0])
	sprintf((char *)real, "%.8s.%.3s", name, ext);
    else
	sprintf((char *)real, "%.8s", name);

    while (*real) {
	if (isupper(*real))
	    *real = tolower(*real);
    	++real;
    }
}

/*
 * convert a path into an argv[] like vector of components.
 * If the path starts with a '/' or '\' then the first entry
 * will be "/" or "\".  This is the only case in which a "/"
 * or "\" may appear in an entry.
 * Also convert all lowercase to uppercase.
 * The data returned is in a static area, so a second call will
 * erase the data of the first.
 */
u_char **
get_entries(u_char *path)
{
    static u_char *entries[128];	/* Maximum depth... */
    static u_char mypath[1024];
    u_char **e = entries;
    u_char *p = mypath;

    ustrncpy(mypath+1, path, 1022);
    p = mypath+1;
    mypath[1023] = 0;
    if (path[0] == '/' || path[0] == '\\') {
	mypath[0] = path[0];
	*e++ = mypath;
    }
    while (*p && e < entries + 127) {
	while (*p == '/' || *p == '\\')
	    *p++ = 0;

	if (!*p)
	    break;
    	*e++ = p;
	while (*p && *p != '/' && *p != '\\') {
	    if (islower(*p))
	    	*p = tolower(*p);
	    ++p;
    	}
    }
    *e = 0;
    return(entries);
}

/*
 * Return file system statistics for drive.
 * Return the DOS errno on failure.
 */
get_space(int drive, fsstat_t *fs)
{
    Path_t *d;
    struct statfs *buf;
    int nfs;
    int i;
    struct statfs *me = 0;

    if (drive < 0 || drive >= MAX_DRIVE)
	return(DISK_DRIVE_INVALID);

    d = &paths[drive];

    if (!d->path)
	return(DISK_DRIVE_INVALID);

    nfs = getfsstat(0, 0, MNT_WAIT);

    buf = (struct statfs *)malloc(sizeof(struct statfs) * nfs);
    if (buf == NULL) {
	perror("get_space");
	return(DISK_DRIVE_INVALID);
    }
    nfs = getfsstat(buf, sizeof(struct statfs) * nfs, MNT_WAIT);

    for (i = 0; i < nfs; ++i) {
	if (strncmp(buf[i].f_mntonname, (char *)d->path, strlen(buf[i].f_mntonname)))
	    continue;
    	if (me && strlen(me->f_mntonname) > strlen(buf[i].f_mntonname))
	    continue;
    	me = buf + i;
    }
    if (!me) {
	free(buf);
	return(3);
    }
    fs->bytes_sector = me->f_fsize;
    fs->sectors_cluster = me->f_bsize / me->f_fsize;
    fs->total_clusters = me->f_blocks / fs->sectors_cluster;
    while (fs->total_clusters > 0xFFFF) {
	fs->sectors_cluster *= 2;
	fs->total_clusters = me->f_blocks / fs->sectors_cluster;
    }
    fs->avail_clusters = me->f_bavail / fs->sectors_cluster;
    free(buf);
    return(0);
}

DIR *dp = 0;
u_char searchdir[1024];
u_char *searchend;

/*
 * Convert a dos filename into normal form (8.3 format, space padded)
 */
void
to_dos_normal(u_char *p, u_char *expr)
{
    int i;

    if (expr[0] == '.' && (expr[1] == '\0'
		        || (expr[1] == '.' && expr[2] == '\0'))) {
    	sprintf((char *)p, "%-8s.   ", expr);
	return;
    }
    for (i = 0; i < 8; ++i) {
	if (*expr && *expr != '.') {
	    if (islower(*expr))
		p[i] = toupper(*expr);
	    else
		p[i] = *expr;
    	    ++expr;
    	} else
	    p[i] = ' ';
    }
    while (*expr && *expr != '.')
	++expr;
    if (*expr)
	++expr;
    p[8] = '.';

    for (i = 0; i < 3; ++i) {
	if (*expr) {
	    if (islower(*expr))
		p[9+i] = toupper(*expr);
	    else
		p[9+i] = *expr;
	    ++expr;
	} else
	    p[9+i] = ' ';
    }
    p[12] = 0;
}

/*
 * Find the first file on drive which matches the path with the given
 * attributes attr.
 * If found, the result is placed in dir (32 bytes).  dta is used to allow the
 * find_next function to continue. (must be atleast 21 bytes).
 * Returns DOS errno on failure.
 */
find_first(int drive, u_char *path, int attr, dosdir_t *dir, find_block_t *dta)
{
    Path_t *d;
    u_char real_path[1024];
    u_char *expr = path;
    u_char *p;

    if (drive < 0 || drive >= MAX_DRIVE) {
	return(DISK_DRIVE_INVALID);
    }

    d = &paths[drive];
    if (!d->cwd) {
	return(DISK_DRIVE_INVALID);
    }

    if (attr == VOLUME_LABEL)
	return(NO_MORE_FILES);

    while (*expr)
	++expr;

    while (expr > path && !isslash(expr[-1]))
	--expr;

    if (ustrlen(expr) > 12) {
	return(NO_MORE_FILES);
    }

    if (expr > path) {
	u_char slash = expr[-1];
	expr[-1] = 0;
	dos_to_real_path(drive, path, real_path);
	expr[-1] = slash;
    } else {
	dos_to_real_path(drive, d->cwd, real_path);
    }

    if (dp)
	closedir(dp);	/* Wrong, need to deal with nested searches */

    ustrcpy(searchdir, real_path);
    searchend = searchdir;
    while (*searchend)
	++searchend;
    *searchend++ = '/';

    if ((dp = opendir((char *)real_path)) == NULL) {
	return(PATH_NOT_FOUND);
    }
    dp->dd_fd = squirrel_fd(dp->dd_fd);

    to_dos_normal(dta->pattern, expr);

    dta->attr = attr;

    return(find_next(dir, dta));
}

/*
 * Continue on where find_first left off.
 * The results will be placed in dir.
 * dta contains the search informatino from find_first.
 */
int
find_next(dosdir_t *dir, find_block_t *dta)
{
    struct dirent *d;
    struct stat sb;
    u_char name[16];

    if (!dp) {
	return(NO_MORE_FILES);
    }

    while (d = readdir(dp)) {
    	real_to_dos((u_char *)d->d_name, name);
    	if (dos_match(dta->pattern, name) == 0)
	    continue;

    	ustrcpy(searchend, (u_char *)d->d_name);
    	if (ustat(searchdir, &sb) < 0)
	    continue;
    	if (S_ISDIR(sb.st_mode)) {
	    if (!(dta->attr & DIRECTORY))
		continue;
	}
    	to_dos_normal(dir->name, name);
    	dir->ext[0] = dir->ext[1];	/* Shift over the '.' */
    	dir->ext[1] = dir->ext[2];
    	dir->ext[2] = dir->ext[3];
    	dir->attr = (S_ISDIR(sb.st_mode) ? DIRECTORY : 0) |
	    	    (uaccess(searchdir, W_OK) ? 0 : READ_ONLY_FILE);
    	encode_dos_file_time(sb.st_mtime, &dir->date, &dir->time);
    	dir->start = 1;
    	dir->size = sb.st_size;
    	dta->flag = 0x80;
	return(0);
    }
    dta->flag = 0x0;
    closedir(dp);
    dp = 0;
    return(NO_MORE_FILES);
}

/*
 * prefrom hokey DOS pattern matching.  pattern may contain the wild cards
 * '*' and '?' only.  Follow the DOS convention that '?*', '*?' and '**' all
 * are the same as '*'.  Also, allow '?' to match the blank padding in a
 * name (hence, ???? matchs all of "a", "ab", "abc" and "abcd" but not "abcde")
 * Return 1 if a match is found, 0 if not.
 */
int
dos_match(u_char *pattern, u_char *string)
{
    u_char c;
    u_char name[16];
    u_char *q;
    u_char *ep, *es;

    to_dos_normal(name, string);
    string = name;

    /*
     * Check the base part first
     */
    ep = pattern + 8;
    es = string + 8;
    while (*pattern && pattern < ep && *string && string < es) {
	switch (*pattern++) {
    	case '?':
	    q = pattern;
	    while (*q == '?')
		++q;
    	    if (*q != '*') {
		++string;
		break;
    	    }
	    /* FALL THRU */
    	case '*':
    	    while (*pattern == '*' || *pattern == '?')
		++pattern;
    	    if (pattern == ep)
		goto matched;
	    switch (*pattern) {
    	    case ' ':
    	    case '\0':
		goto matched;
    	    default:
	    	while (*string && string < es && *string != *pattern)
		    ++string;
    	    	if (*string != *pattern)
		    goto failed;
	    }
	    break;
    	case ' ':
	default:
	    if (*string++ != pattern[-1])
		goto failed;
	}
    }
    if (pattern < ep && *pattern != ' ')
	goto failed;
    if (string < es && *string != ' ')
	goto failed;

matched:
    pattern = ep + 1;
    string = es + 1;

    while (*pattern && *string) {
	switch (*pattern++) {
    	case '?':
	    q = pattern;
	    while (*q == '?')
		++q;
    	    if (*q != '*') {
		++string;
		break;
    	    }
	    /* FALL THRU */
    	case '*':
    	    while (*pattern == '*' || *pattern == '?')
		++pattern;
	    switch (*pattern) {
    	    case ' ':
    	    case '\0':
		goto okay;
    	    default:
	    	while (*string && *string != *pattern)
		    ++string;
    	    	if (*string != *pattern) {
		    goto failed;
		}
	    }
	    break;
    	case ' ':
	default:
	    if (*string++ != pattern[-1]) {
		goto failed;
	    }
	}
    }
    if (*pattern || *string)
	goto failed;
okay:
    return(1);
failed:
    return(0);
}
