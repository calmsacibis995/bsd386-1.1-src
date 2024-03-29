/*
 * Copyright (c) 1990 Jan-Simon Pendry
 * Copyright (c) 1990 Imperial College of Science, Technology & Medicine
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry at Imperial College, London.
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
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
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
 *
 *	@(#)util.c	5.3 (Berkeley) 5/12/91
 *
 * $Id: util.c,v 1.2 1992/01/04 19:09:03 kolstad Exp $
 *
 */

/*
 * Utils
 */

#include "am.h"
#include <ctype.h>
#include <sys/stat.h>
#include <netdb.h>


char *strnsave(str, len)
Const char *str;
int len;
{
	char *sp = (char *) xmalloc(len+1);

	bcopy(str, sp, len);
	sp[len] = 0;

	return sp;
}

char *strdup(s)
Const char *s;
{
	return strnsave(s, strlen(s));
}

/*
 * Concatenate three strings and store in buffer pointed to
 * by p, making p large enough to hold the strings
 */
char *str3cat(p, s1, s2, s3)
char *p;
char *s1;
char *s2;
char *s3;
{
	int l1 = strlen(s1);
	int l2 = strlen(s2);
	int l3 = strlen(s3);
	p = (char *) xrealloc(p, l1 + l2 + l3 + 1);
	bcopy(s1, p, l1);
	bcopy(s2, p + l1, l2);
	bcopy(s3, p + l1 + l2, l3 + 1);
	return p;
}

char *strealloc(p, s)
char *p;
char *s;
{
	int len = strlen(s) + 1;

	p = (char *) xrealloc((voidp) p, len);

	strcpy(p, s);
#ifdef DEBUG_MEM
	malloc_verify();
#endif /* DEBUG_MEM */
	return p;
}

char **strsplit P((char *s, int ch, int qc));
char **strsplit(s, ch, qc)
char *s;
int ch;
int qc;
{
	char **ivec;
	int ic = 0;
	int done = 0;

	ivec = (char **) xmalloc((ic+1)*sizeof(char *));

	while (!done) {
		char *v;
		/*
		 * skip to split char
		 */
		while (*s && (ch == ' ' ? (isascii(*s) && isspace(*s)) : *s == ch))
				s++;

		/*
		 * End of string?
		 */
		if (!*s)
			break;

		/*
		 * remember start of string
		 */
		v = s;

		/*
		 * skip to split char
		 */
		while (*s && !(ch == ' ' ? (isascii(*s) && isspace(*s)) : *s == ch)) {
			if (*s++ == qc) {
				/*
				 * Skip past string.
				 */
				s++;
				while (*s && *s != qc)
				 	s++;
				if (*s == qc)
					s++;
			}
		}

		if (!*s)
			done = 1;
		*s++ = '\0';

		/*
		 * save string in new ivec slot
		 */
		ivec[ic++] = v;
		ivec = (char **) xrealloc((voidp) ivec, (ic+1)*sizeof(char *));
#ifdef DEBUG
		Debug(D_STR)
			plog(XLOG_DEBUG, "strsplit saved \"%s\"", v);
#endif /* DEBUG */
	}

#ifdef DEBUG
	Debug(D_STR)
		plog(XLOG_DEBUG, "strsplit saved a total of %d strings", ic);
#endif /* DEBUG */

	ivec[ic] = 0;

	return ivec;
}

/*
 * Strip off the trailing part of a domain
 * to produce a short-form domain relative
 * to the local host domain.
 * Note that this has no effect if the domain
 * names do not have the same number of
 * components.  If that restriction proves
 * to be a problem then the loop needs recoding
 * to skip from right to left and do partial
 * matches along the way -- ie more expensive.
 */
static void domain_strip P((char *otherdom, char *localdom));
static void domain_strip(otherdom, localdom)
char *otherdom, *localdom;
{
#ifdef PARTIAL_DOMAINS
        char *p1 = otherdom-1;
	char *p2 = localdom-1;

        do {
                if (p1 = strchr(p1+1, '.'))
                if (p2 = strchr(p2+1, '.'))
                if (strcmp(p1+1, p2+1) == 0) {
                        *p1 = '\0';
                        break;
                }
        } while (p1 && p2);
#else
	char *p1, *p2;

	if ((p1 = strchr(otherdom, '.')) &&
			(p2 = strchr(localdom, '.')) &&
			(strcmp(p1+1, p2+1) == 0))
		*p1 = '\0';
#endif /* PARTIAL_DOMAINS */
}

/*
 * Normalize a host name
 */
void host_normalize P((char **chp));
void host_normalize(chp)
char **chp;
{
	/*
	 * Normalize hosts is used to resolve host name aliases
	 * and replace them with the standard-form name.
	 * Invoked with "-n" command line option.
	 */
	if (normalize_hosts) {
		struct hostent *hp;
		clock_valid = 0;
		hp = gethostbyname(*chp);
		if (hp && hp->h_addrtype == AF_INET) {
#ifdef DEBUG
			dlog("Hostname %s normalized to %s", *chp, hp->h_name);
#endif /* DEBUG */
			*chp = strealloc(*chp, hp->h_name);
		}
	}
	domain_strip(*chp, hostd);
}

/*
 * Make a dotted quad from a 32bit IP address
 * addr is in network byte order.
 * sizeof(buf) needs to be at least 16.
 */
char *inet_dquad P((char *buf, unsigned long addr));
char *inet_dquad(buf, addr)
char *buf;
unsigned long addr;
{
	addr = ntohl(addr);
	sprintf(buf, "%d.%d.%d.%d",
		((addr >> 24) & 0xff),
		((addr >> 16) & 0xff),
		((addr >> 8) & 0xff),
		((addr >> 0) & 0xff));
	return buf;
}

/*
 * Keys are not allowed to contain " ' ! or ; to avoid
 * problems with macro expansions.
 */
static char invalid_keys[] = "\"'!;@ \t\n";
int valid_key P((char *key));
int valid_key(key)
char *key;
{
	while (*key)
		if (strchr(invalid_keys, *key++))
			return FALSE;
	return TRUE;
}

void going_down P((int rc));
void going_down(rc)
int rc;
{
	if (foreground) {
		if (amd_state != Start) {
			if (amd_state != Done)
				return;
			unregister_amq();
		}
	}
	if (foreground) {
		plog(XLOG_INFO, "Finishing with status %d", rc);
	} else {
#ifdef DEBUG
		dlog("background process exiting with status %d", rc);
#endif /* DEBUG */
	}

	exit(rc);
}


int bind_resv_port P((int so, u_short *pp));
int bind_resv_port(so, pp)
int so;
u_short *pp;
{
	struct sockaddr_in sin;
	int rc;
	unsigned short port;

	bzero((voidp) &sin, sizeof(sin));
	sin.sin_family = AF_INET;

	port = IPPORT_RESERVED;

	do {
		--port;
		sin.sin_port = htons(port);
		rc = bind(so, (struct sockaddr *) &sin, sizeof(sin));
	} while (rc < 0 && port > IPPORT_RESERVED/2);

	if (pp && rc == 0)
		*pp = port;
	return rc;
}

void forcibly_timeout_mp P((am_node *mp));
void forcibly_timeout_mp(mp)
am_node *mp;
{
	mntfs *mf = mp->am_mnt;
	/*
	 * Arrange to timeout this node
	 */
	if (mf && ((mp->am_flags & AMF_ROOT) ||
		(mf->mf_flags & (MFF_MOUNTING|MFF_UNMOUNTING)))) {
		if (!(mf->mf_flags & MFF_UNMOUNTING))
			plog(XLOG_WARNING, "ignoring timeout request for active node %s", mp->am_path);
	} else {
		plog(XLOG_INFO, "\"%s\" forcibly timed out", mp->am_path);
		mp->am_flags &= ~AMF_NOTIMEOUT;
		mp->am_ttl = clocktime();
		reschedule_timeout_mp();
	}
}

void mf_mounted P((mntfs *mf));
void mf_mounted(mf)
mntfs *mf;
{
	int quoted;
	int wasmounted = mf->mf_flags & MFF_MOUNTED;

	if (!wasmounted) {
		/*
		 * If this is a freshly mounted
		 * filesystem then update the
		 * mntfs structure...
		 */
		mf->mf_flags |= MFF_MOUNTED;
		mf->mf_error = 0;

		/*
		 * Do mounted callback
		 */
		if (mf->mf_ops->mounted)
			(*mf->mf_ops->mounted)(mf);

		mf->mf_fo = 0;
	}

	/*
	 * Log message
	 */
	quoted = strchr(mf->mf_info, ' ') != 0;
	plog(XLOG_INFO, "%s%s%s %s fstype %s on %s",
		quoted ? "\"" : "",
		mf->mf_info,
		quoted ? "\"" : "",
		wasmounted ? "referenced" : "mounted",
		mf->mf_ops->fs_type, mf->mf_mount);
}

void am_mounted P((am_node *mp));
void am_mounted(mp)
am_node *mp;
{
	mntfs *mf = mp->am_mnt;

	mf_mounted(mf);

	/*
	 * Patch up path for direct mounts
	 */
	if (mp->am_parent && mp->am_parent->am_mnt->mf_ops == &dfs_ops)
		mp->am_path = str3cat(mp->am_path, mp->am_parent->am_path, "/", ".");

	/*
	 * Check whether this mount should be cached permanently
	 */
	if (mf->mf_ops->fs_flags & FS_NOTIMEOUT) {
		mp->am_flags |= AMF_NOTIMEOUT;
	} else if (mf->mf_mount[1] == '\0' && mf->mf_mount[0] == '/') {
		mp->am_flags |= AMF_NOTIMEOUT;
	} else {
		struct mntent mnt;
		if (mf->mf_mopts) {
			mnt.mnt_opts = mf->mf_mopts;
			if (hasmntopt(&mnt, "nounmount"))
				mp->am_flags |= AMF_NOTIMEOUT;
			if ((mp->am_timeo = hasmntval(&mnt, "utimeout")) == 0)
				mp->am_timeo = am_timeo;
		}
	}

	/*
	 * If this node is a symlink then
	 * compute the length of the returned string.
	 */
	if (mp->am_fattr.type == NFLNK)
		mp->am_fattr.size = strlen(mp->am_link ? mp->am_link : mp->am_mnt->mf_mount);

	/*
	 * Record mount time
	 */
	mp->am_fattr.mtime.seconds = mp->am_stats.s_mtime = clocktime();
	new_ttl(mp);
	/*
	 * Update mtime of parent node
	 */
	if (mp->am_parent && mp->am_parent->am_mnt)
		mp->am_parent->am_fattr.mtime.seconds = mp->am_stats.s_mtime;


	/*
	 * Update stats
	 */
	amd_stats.d_mok++;
}

int mount_node P((am_node *mp));
int mount_node(mp)
am_node *mp;
{
	mntfs *mf = mp->am_mnt;
	int error;

	mf->mf_flags |= MFF_MOUNTING;
	error = (*mf->mf_ops->mount_fs)(mp);
	mf = mp->am_mnt;
	if (error >= 0)
		mf->mf_flags &= ~MFF_MOUNTING;
	if (!error && !(mf->mf_ops->fs_flags & FS_MBACKGROUND)) {
		/* ...but see ifs_mount */
		am_mounted(mp);
	}

	return error;
}

void am_unmounted P((am_node *mp));
void am_unmounted(mp)
am_node *mp;
{
	mntfs *mf = mp->am_mnt;

	if (!foreground) /* firewall - should never happen */
		return;

#ifdef DEBUG
	/*dlog("in am_unmounted(), foreground = %d", foreground);*/
#endif /* DEBUG */

	/*
	 * Do unmounted callback
	 */
	if (mf->mf_ops->umounted)
		(*mf->mf_ops->umounted)(mp);

	/*
	 * Update mtime of parent node
	 */
	if (mp->am_parent && mp->am_parent->am_mnt)
		mp->am_parent->am_fattr.mtime.seconds = clocktime();

	free_map(mp);
}

int auto_fmount P((am_node *mp));
int auto_fmount(mp)
am_node *mp;
{
	mntfs *mf = mp->am_mnt;
	return (*mf->mf_ops->fmount_fs)(mf);
}

int auto_fumount P((am_node *mp));
int auto_fumount(mp)
am_node *mp;
{
	mntfs *mf = mp->am_mnt;
	return (*mf->mf_ops->fumount_fs)(mf);
}

/*
 * Fork the automounter
 *
 * TODO: Need a better strategy for handling errors
 */
static int dofork(P_void);
static int dofork()
{
	int pid;
top:
	pid = fork();

	if (pid < 0) {
		sleep(1);
		goto top;
	}

	if (pid == 0) {
		mypid = getpid();
		foreground = 0;
	}

	return pid;
}

int background(P_void);
int background()
{
	int pid = dofork();
	if (pid == 0) {
#ifdef DEBUG
		dlog("backgrounded");
#endif
		foreground = 0;
	}

	return pid;
}

/*
 * Make all the directories in the path.
 */
int mkdirs P((char *path, int mode));
int mkdirs(path, mode)
char *path;
int mode;
{
	/*
	 * take a copy in case path is in readonly store
	 */
	char *p2 = strdup(path);
	char *sp = p2;
	struct stat stb;
	int error_so_far = 0;

	/*
	 * Skip through the string make the directories.
	 * Mostly ignore errors - the result is tested at the end.
	 *
	 * This assumes we are root so that we can do mkdir in a
	 * mode 555 directory...
	 */
	while (sp = strchr(sp+1, '/')) {
		*sp = '\0';
		if (mkdir(p2, mode) < 0) {
			error_so_far = errno;
		} else {
#ifdef DEBUG
			dlog("mkdir(%s)", p2);
#endif
		}
		*sp = '/';
	}

	if (mkdir(p2, mode) < 0) {
		error_so_far = errno;
	} else {
#ifdef DEBUG
		dlog("mkdir(%s)", p2);
#endif
	}

#ifdef SUNOS4_WORKAROUND
	/*
	 * Do a sync - if we do rmdirs() immediately
	 * and then the system crashes it leaves
	 * the filesystem in a state that fsck -p
	 * can't fix.  (Observed more than once on
	 * SunOS 4 ...)
	 *
	 * The problem was caused by a bug somewhere
	 * in the UFS code which has since been fixed
	 * (at least at Berkeley).
	 *
	 * Attempted workaround - XXX.
	 */
	sync();
#endif /* SUNOS4_WORKAROUND */

	free(p2);

	return stat(path, &stb) == 0 &&
		(stb.st_mode & S_IFMT) == S_IFDIR ? 0 : error_so_far;
}

/*
 * Remove as many directories in the path as possible.
 * Give up if the directory doesn't appear to have
 * been created by Amd (not mode dr-x) or an rmdir
 * fails for any reason.
 */
void rmdirs P((char *dir));
void rmdirs(dir)
char *dir;
{
	char *xdp = strdup(dir);
	char *dp;

	do {
		struct stat stb;
		/*
		 * Try to find out whether this was
		 * created by amd.  Do this by checking
		 * for owner write permission.
		 */
		if (stat(xdp, &stb) == 0 && (stb.st_mode & 0200) == 0) {
			if (rmdir(xdp) < 0) {
				if (errno != ENOTEMPTY &&
				    errno != EBUSY &&
				    errno != EEXIST &&
				    errno != EINVAL)
					plog(XLOG_ERROR, "rmdir(%s): %m", xdp);
				break;
			} else {
#ifdef DEBUG
				dlog("rmdir(%s)", xdp);
#endif
			}
		} else {
			break;
		}
		dp = strrchr(xdp, '/');
		if (dp)
			*dp = '\0';
	} while (dp && dp > xdp);
	free(xdp);
}
