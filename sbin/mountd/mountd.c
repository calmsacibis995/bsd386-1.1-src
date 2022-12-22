#define dosubdirs
/*-
 * Copyright (c) 1993, 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: mountd.c,v 1.9 1994/02/01 04:47:48 donn Exp $
 */

/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Rick Macklem at The University of Guelph.
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
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1989 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static char sccsid[] = "@(#)mountd.c	5.14 (Berkeley) 2/26/91";
#endif not lint

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <netdb.h>
#include <rpc/rpc.h>
#include <rpc/pmap_clnt.h>
#include <rpc/pmap_prot.h>
#include <nfs/rpcv2.h>
#include <nfs/nfsv2.h>
#include <arpa/inet.h>
#include "pathnames.h"

struct ufid {
	u_short	ufid_len;
	ino_t	ufid_ino;
	long	ufid_gen;
};
/*
 * Structures for keeping the mount list and export list
 */
struct mountlist {
	struct mountlist *ml_next;
	char	ml_host[RPCMNT_NAMELEN+1];
	char	ml_dirp[RPCMNT_PATHLEN+1];
};

struct exportlist {
	struct exportlist *ex_next;
	struct exportlist *ex_prev;
	struct grouplist *ex_groups;
	int	ex_rootuid;
	int	ex_exflags;
	int	ex_options;
	dev_t	ex_dev;
	char	ex_dirp[RPCMNT_PATHLEN+1];
};

#define	EXOP_ALLDIRS	0x0001		/* export all directories of fs */
#define	EXOP_SUBDIRS	0x0002		/* export subdirectories of dir */

struct grouplist {
	struct grouplist *gr_next;
	struct hostent *gr_hp;
};

/* Global defs */

void add_mlist __P((char *, char *));
void del_mlist __P((char *, char *));
void do_opt __P((char *, struct exportlist *, struct exportlist *,
    int *, int *, int *));;
void free_exp __P((struct exportlist *));
void get_exportlist __P((void));
void get_mountlist __P((void));
void mntsrv __P((struct svc_req *, SVCXPRT *));
void nextfield __P((char **, char **));
char *realpath __P((char *, char *));
void send_umntall __P((void));
#ifdef dosubdirs
int subdir __P((char *, char *));
#endif
int umntall_each __P((caddr_t, struct sockaddr_in *));
int xdr_dir __P((XDR *, char *));
int xdr_explist __P((XDR *, caddr_t));
int xdr_fhs __P((XDR *, nfsv2fh_t *));
int xdr_mlist __P((XDR *, caddr_t));

#define	NONROOT	65534

struct exportlist exphead;
struct mountlist *mlhead;
char exname[MAXPATHLEN];
int def_rootuid = NONROOT;
int root_only = 1;
#ifdef DEBUG
int debug = 1;
#else
int debug = 0;
#endif

/*
 * Mountd server for NFS mount protocol as described in:
 * NFS: Network File System Protocol Specification, RFC1094, Appendix A
 * The optional arguments are the exports file name
 * default: _PATH_EXPORTS
 * and "-n" to allow nonroot mount.
 */
int
main(argc, argv)
	int argc;
	char **argv;
{
	SVCXPRT *transp;
	int c;

	while ((c = getopt(argc, argv, "n")) != EOF)
		switch (c) {
		case 'n':
			root_only = 0;
			break;
		default:
			fprintf(stderr, "Usage: mountd [-n] [export_file]\n");
			exit(1);
		};
	argc -= optind;
	argv += optind;
	exphead.ex_next = exphead.ex_prev = (struct exportlist *)0;
	mlhead = (struct mountlist *)0;
	if (argc == 1) {
		strncpy(exname, *argv, MAXPATHLEN-1);
		exname[MAXPATHLEN-1] = '\0';
	} else
		strcpy(exname, _PATH_EXPORTS);
	openlog("mountd:", LOG_PID, LOG_DAEMON);
	get_exportlist();
	get_mountlist();
	if (debug == 0) {
		daemon(0, 0);
		signal(SIGINT, SIG_IGN);
		signal(SIGQUIT, SIG_IGN);
	}
	signal(SIGHUP, get_exportlist);
	signal(SIGTERM, send_umntall);
	{ FILE *pidfile = fopen(_PATH_MOUNTDPID, "w");
	  if (pidfile != NULL) {
		fprintf(pidfile, "%d\n", getpid());
		fclose(pidfile);
	  }
	}
	if ((transp = svcudp_create(RPC_ANYSOCK)) == NULL) {
		syslog(LOG_ERR, "Can't create socket");
		exit(1);
	}
	pmap_unset(RPCPROG_MNT, RPCMNT_VER1);
	if (!svc_register(transp, RPCPROG_MNT, RPCMNT_VER1, mntsrv, IPPROTO_UDP)) {
		syslog(LOG_ERR, "Can't register mount");
		exit(1);
	}
	svc_run();
	syslog(LOG_ERR, "Mountd died");
	exit(1);
}

/*
 * The mount rpc service
 */
void
mntsrv(rqstp, transp)
	register struct svc_req *rqstp;
	register SVCXPRT *transp;
{
	register struct grouplist *grp;
	register u_long **addrp;
	register struct exportlist *ep;
	nfsv2fh_t nfh;
	struct authunix_parms *ucr;
	struct stat stb;
	struct hostent *hp;
	u_long saddr;
	char dirpath[RPCMNT_PATHLEN+1];
	char realname[MAXPATHLEN];
	int bad = ENOENT;
	int omask;
	uid_t uid = NONROOT;

	/* Get authorization */
	switch (rqstp->rq_cred.oa_flavor) {
	case AUTH_UNIX:
		ucr = (struct authunix_parms *)rqstp->rq_clntcred;
		uid = ucr->aup_uid;
		break;
	case AUTH_NULL:
	default:
		break;
	}

	saddr = transp->xp_raddr.sin_addr.s_addr;
	hp = (struct hostent *)0;
	switch (rqstp->rq_proc) {
	case NULLPROC:
		if (!svc_sendreply(transp, xdr_void, (caddr_t)0))
			syslog(LOG_ERR, "Can't send reply");
		return;
	case RPCMNT_MOUNT:
		if (uid != 0 && root_only) {
			svcerr_weakauth(transp);
			return;
		}
		if (!svc_getargs(transp, xdr_dir, dirpath)) {
			svcerr_decode(transp);
			return;
		}

		/* Check to see if it's a valid dirpath */
		if (realpath(dirpath, realname) == NULL ||
		    stat(realname, &stb) < 0) {
			if (!svc_sendreply(transp, xdr_long, (caddr_t)&bad))
				syslog(LOG_ERR, "Can't send reply");
			return;
		}

		/* Check in the exports list */
		omask = sigblock(sigmask(SIGHUP));
		ep = exphead.ex_next;
		while (ep != NULL) {
			if (!strcmp(ep->ex_dirp, realname) ||
			    (stb.st_dev == ep->ex_dev &&
			    ep->ex_options&EXOP_ALLDIRS
#ifdef dosubdirs
			    || (ep->ex_options&EXOP_SUBDIRS &&
				subdir(ep->ex_dirp, realname))
#endif
			    )) {
				grp = ep->ex_groups;
				if (grp == NULL)
					break;

				/* Check for a host match */
				addrp = (u_long **)grp->gr_hp->h_addr_list;
				for (;;) {
					if (**addrp == saddr)
						break;
					if (*++addrp == NULL)
						if (grp = grp->gr_next) {
							addrp = (u_long **)
								grp->gr_hp->h_addr_list;
						} else {
							bad = EACCES;
							if (!svc_sendreply(transp, xdr_long, (caddr_t)&bad))
								syslog(LOG_ERR, "Can't send reply");
							sigsetmask(omask);
							return;
						}
				}
				hp = grp->gr_hp;
				break;
			}
			ep = ep->ex_next;
		}
		sigsetmask(omask);
		if (ep == NULL) {
			bad = EACCES;
			if (!svc_sendreply(transp, xdr_long, (caddr_t)&bad))
				syslog(LOG_ERR, "Can't send reply");
			return;
		}

		/* Get the file handle */
		bzero((caddr_t)&nfh, sizeof(nfh));
		if (getfh(realname, (fhandle_t *)&nfh) < 0) {
			bad = errno;
			if (!svc_sendreply(transp, xdr_long, (caddr_t)&bad))
				syslog(LOG_ERR, "Can't send reply");
			return;
		}
		if (!svc_sendreply(transp, xdr_fhs, (caddr_t)&nfh))
			syslog(LOG_ERR, "Can't send reply");
		if (hp == NULL)
			hp = gethostbyaddr((caddr_t)&saddr, sizeof(saddr), AF_INET);
		if (hp)
			add_mlist(hp->h_name, dirpath);
		return;
	case RPCMNT_DUMP:
		if (!svc_sendreply(transp, xdr_mlist, (caddr_t)0))
			syslog(LOG_ERR, "Can't send reply");
		return;
	case RPCMNT_UMOUNT:
		if (uid != 0 && root_only) {
			svcerr_weakauth(transp);
			return;
		}
		if (!svc_getargs(transp, xdr_dir, dirpath)) {
			svcerr_decode(transp);
			return;
		}
		if (!svc_sendreply(transp, xdr_void, (caddr_t)0))
			syslog(LOG_ERR, "Can't send reply");
		hp = gethostbyaddr((caddr_t)&saddr, sizeof(saddr), AF_INET);
		if (hp)
			del_mlist(hp->h_name, dirpath);
		return;
	case RPCMNT_UMNTALL:
		if (uid != 0 && root_only) {
			svcerr_weakauth(transp);
			return;
		}
		if (!svc_sendreply(transp, xdr_void, (caddr_t)0))
			syslog(LOG_ERR, "Can't send reply");
		hp = gethostbyaddr((caddr_t)&saddr, sizeof(saddr), AF_INET);
		if (hp)
			del_mlist(hp->h_name, (char *)0);
		return;
	case RPCMNT_EXPORT:
		if (!svc_sendreply(transp, xdr_explist, (caddr_t)0))
			syslog(LOG_ERR, "Can't send reply");
		return;
	default:
		svcerr_noproc(transp);
		return;
	}
}

/*
 * Xdr conversion for a dirpath string
 */
int
xdr_dir(xdrsp, dirp)
	XDR *xdrsp;
	char *dirp;
{
	return (xdr_string(xdrsp, &dirp, RPCMNT_PATHLEN));
}

/*
 * Xdr routine to generate fhstatus
 */
int
xdr_fhs(xdrsp, nfh)
	XDR *xdrsp;
	nfsv2fh_t *nfh;
{
	int ok = 0;

	if (!xdr_long(xdrsp, &ok))
		return (0);
	return (xdr_opaque(xdrsp, (caddr_t)nfh, NFSX_FH));
}

int
xdr_mlist(xdrsp, cp)
	XDR *xdrsp;
	caddr_t cp;
{
	register struct mountlist *mlp;
	int true = 1;
	int false = 0;
	char *strp;

	mlp = mlhead;
	while (mlp) {
		if (!xdr_bool(xdrsp, &true))
			return (0);
		strp = &mlp->ml_host[0];
		if (!xdr_string(xdrsp, &strp, RPCMNT_NAMELEN))
			return (0);
		strp = &mlp->ml_dirp[0];
		if (!xdr_string(xdrsp, &strp, RPCMNT_PATHLEN))
			return (0);
		mlp = mlp->ml_next;
	}
	if (!xdr_bool(xdrsp, &false))
		return (0);
	return (1);
}

/*
 * Xdr conversion for export list
 */
int
xdr_explist(xdrsp, cp)
	XDR *xdrsp;
	caddr_t cp;
{
	register struct exportlist *ep;
	register struct grouplist *grp;
	int true = 1;
	int false = 0;
	char *strp;
	int omask;

	omask = sigblock(sigmask(SIGHUP));
	ep = exphead.ex_next;
	while (ep != NULL) {
		if (!xdr_bool(xdrsp, &true))
			goto errout;
		strp = &ep->ex_dirp[0];
		if (!xdr_string(xdrsp, &strp, RPCMNT_PATHLEN))
			goto errout;
		grp = ep->ex_groups;
		while (grp != NULL) {
			if (!xdr_bool(xdrsp, &true))
				goto errout;
			strp = grp->gr_hp->h_name;
			if (!xdr_string(xdrsp, &strp, RPCMNT_NAMELEN))
				goto errout;
			grp = grp->gr_next;
		}
		if (!xdr_bool(xdrsp, &false))
			goto errout;
		ep = ep->ex_next;
	}
	sigsetmask(omask);
	if (!xdr_bool(xdrsp, &false))
		return (0);
	return (1);
errout:
	sigsetmask(omask);
	return (0);
}

/*
 * Get the export list
 */
void
get_exportlist()
{
	register struct hostent *hp, *nhp;
	register char **addrp, **naddrp;
	register int i;
	register struct grouplist *grp;
	register struct exportlist *ep, *ep2;
	struct statfs stfsbuf;
	struct stat sb;
	FILE *inf;
	char *cp, *endcp;
	char savedc;
	int len, dirplen;
	size_t explen;
	int rootuid, exflags, optflags;
	u_long saddr;
	struct exportlist *fep;
	char realname[MAXPATHLEN];

	/*
	 * First, get rid of the old list
	 */
	ep = exphead.ex_next;
	while (ep != NULL) {
		ep2 = ep;
		ep = ep->ex_next;
		free_exp(ep2);
	}

	/*
	 * Read in the exports file and build the list, calling
	 * exportfs() as we go along
	 */
	exphead.ex_next = exphead.ex_prev = (struct exportlist *)0;
	if ((inf = fopen(exname, "r")) == NULL) {
		syslog(LOG_ERR, "Can't open %s", exname);
		exit(2);
	}
	while (cp = fgetln(inf, &explen)) {
		if (*cp == '\n')
			continue;	/* skip blank lines */
		if (cp[explen - 1] != '\n') {
			syslog(LOG_ERR, "export %s: not newline-terminated",
			    cp);
			continue;
		}
		cp[--explen] = '\0';
		if (*cp == '#' || *cp == '\0')
			continue;

		exflags = MNT_EXPORTED;
		rootuid = def_rootuid;
		optflags = 0;
		nextfield(&cp, &endcp);

		/*
		 * Get file system devno and see if an entry for this
		 * file system already exists.
		 */
		savedc = *endcp;
		*endcp = '\0';
		if (lstat(cp, &sb) < 0) {
			syslog(LOG_ERR, "export %s: %m", cp);
			continue;
		}
		if (realpath(cp, realname) == NULL ||
		    strcmp(cp, realname) != 0 ||
		    S_ISLNK(sb.st_mode)) {
			syslog(LOG_ERR,
			    "cannot export %s: path contains symlink", cp);
			continue;
		}
		fep = (struct exportlist *)0;
		ep = exphead.ex_next;
		while (ep) {
			if (ep->ex_dev == sb.st_dev) {
				fep = ep;
				break;
			}
			ep = ep->ex_next;
		}
		*endcp = savedc;

		/*
		 * Create new exports list entry
		 */
		len = endcp-cp;
		if (len <= RPCMNT_PATHLEN && len > 0) {
			ep = (struct exportlist *)malloc(sizeof(*ep));
			if (ep == NULL)
				goto err;
			ep->ex_next = ep->ex_prev = (struct exportlist *)0;
			ep->ex_groups = (struct grouplist *)0;
			bcopy(cp, ep->ex_dirp, len);
			ep->ex_dirp[len] = '\0';
			dirplen = len;
		} else {
			syslog(LOG_ERR, "Bad Exports File entry %s", cp);
			continue;
		}

		cp = endcp;
		nextfield(&cp, &endcp);
		len = endcp-cp;
		while (len > 0) {
			savedc = *endcp;
			*endcp = '\0';
			if (len > RPCMNT_NAMELEN)
				goto more;
			if (*cp == '-') {
				do_opt(cp + 1, fep, ep, &exflags, &rootuid,
				    &optflags);
				goto more;
			}
			if (isdigit(*cp)) {
				saddr = inet_addr(cp);
				if (saddr == -1 ||
				    (hp = gethostbyaddr((caddr_t)&saddr,
				     sizeof(saddr), AF_INET)) == NULL) {
					syslog(LOG_ERR,
					    "Bad Exports File, %s: %s", cp,
					    "Gethostbyaddr failed, ignored");
					goto more;
				}
			} else if ((hp = gethostbyname(cp)) == NULL) {
				syslog(LOG_ERR, "Bad Exports File, %s: %s",
				    cp, "Gethostbyname failed, ignored");
				goto more;
			}
			grp = (struct grouplist *)
				malloc(sizeof(struct grouplist));
			if (grp == NULL)
				goto err;
			nhp = grp->gr_hp = (struct hostent *)
				malloc(sizeof(struct hostent));
			if (nhp == NULL)
				goto err;
			bcopy((caddr_t)hp, (caddr_t)nhp,
				sizeof(struct hostent));
			i = strlen(hp->h_name)+1;
			nhp->h_name = (char *)malloc(i);
			if (nhp->h_name == NULL)
				goto err;
			bcopy(hp->h_name, nhp->h_name, i);
			addrp = hp->h_addr_list;
			i = 1;
			while (*addrp++)
				i++;
			naddrp = nhp->h_addr_list = (char **)
				malloc(i*sizeof(char *));
			if (naddrp == NULL)
				goto err;
			addrp = hp->h_addr_list;
			while (*addrp) {
				*naddrp = (char *)
				    malloc(hp->h_length);
				if (*naddrp == NULL)
				    goto err;
				bcopy(*addrp, *naddrp,
					hp->h_length);
				addrp++;
				naddrp++;
			}
			*naddrp = (char *)0;
			grp->gr_next = ep->ex_groups;
			ep->ex_groups = grp;
		more:
			cp = endcp;
			*cp = savedc;
			nextfield(&cp, &endcp);
			len = endcp - cp;
		}
		if (fep == NULL) {
			cp = (char *)0;
			for (;;) {
				if (statfs(ep->ex_dirp, &stfsbuf) == 0) {
				    switch (stfsbuf.f_type) {
				    case MOUNT_UFS: {
					struct ufs_args args;

					args.fspec = 0;
					args.exflags = exflags;
					args.exroot = rootuid;
					if (mount(MOUNT_UFS, ep->ex_dirp,
				            stfsbuf.f_flags|MNT_UPDATE,
					    &args) == 0)
					    goto exported;
					}
					break;
#ifdef ISO9660
				    case MOUNT_ISO9660: {
					struct iso9660_args args;

					args.fspec = 0;
					args.exflags = exflags;
					args.exroot = rootuid;
					args.raw = 0;	/* unused */
					if (mount(MOUNT_ISO9660, ep->ex_dirp,
				            stfsbuf.f_flags|MNT_UPDATE,
					    &args) == 0)
					    goto exported;
					}
					break;
#endif
				    default:
					syslog(LOG_WARNING,
					     "Can't export %s: %s", ep->ex_dirp,
					     "wrong filesystem type");
					free_exp(ep);
					goto nextline;
				    }
				}

				/* this must not have been root of fs */
				if (optflags & EXOP_ALLDIRS) {
					syslog(LOG_ERR,
	    "%s: -alldirs can be used only at top of filesystem, ignored",
					    realname);
					optflags &= ~EXOP_ALLDIRS;
				}
				if (cp == NULL)
					cp = ep->ex_dirp + dirplen - 1;
				else
					*cp-- = savedc;
				/* back up over the last component */
				while (*cp == '/' && cp > ep->ex_dirp)
					cp--;
				while (cp > ep->ex_dirp && *(cp - 1) != '/')
					cp--;
				if (cp == ep->ex_dirp) {
					if (cp)
						*cp = savedc;
					syslog(LOG_WARNING,
					      "Can't export %s", ep->ex_dirp);
					free_exp(ep);
					goto nextline;
				}
				savedc = *cp;
				*cp = '\0';
			}
	exported:
			if (cp)
				*cp = savedc;
			ep->ex_rootuid = rootuid;
			ep->ex_exflags = exflags;
			ep->ex_options = optflags;
		} else {
			if (optflags & EXOP_ALLDIRS ||
			    fep->ex_options & EXOP_ALLDIRS) {
				syslog(LOG_WARNING,
				  "%s: Can't export alldirs plus other exports",
				     realname);
				free_exp(ep);
				goto nextline;
			}
			ep->ex_rootuid = fep->ex_rootuid;
			ep->ex_exflags = fep->ex_exflags;
			ep->ex_options = optflags;
		}
		ep->ex_dev = sb.st_dev;
		ep->ex_next = exphead.ex_next;
		ep->ex_prev = &exphead;
		if (ep->ex_next != NULL)
			ep->ex_next->ex_prev = ep;
		exphead.ex_next = ep;
nextline:
		;
	}
	fclose(inf);
	return;
err:
	syslog(LOG_ERR, "No more memory: mountd Failed");
	exit(2);
}

/*
 * Parse out the next white space separated field
 */
void
nextfield(cp, endcp)
	char **cp;
	char **endcp;
{
	register char *p;

	p = *cp;
	while (*p == ' ' || *p == '\t')
		p++;
	if (*p == '\n' || *p == '\0') {
		*cp = *endcp = p;
		return;
	}
	*cp = p++;
	while (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\0')
		p++;
	*endcp = p;
}

/*
 * Parse the option string
 */
void
do_opt(cpopt, fep, ep, exflagsp, rootuidp, optflags)
	register char *cpopt;
	struct exportlist *fep, *ep;
	int *exflagsp, *rootuidp, *optflags;
{
	register char *cpoptarg, *cpoptend;

	while (cpopt && *cpopt) {
		if (cpoptend = index(cpopt, ','))
			*cpoptend++ = '\0';
		if (cpoptarg = index(cpopt, '='))
			*cpoptarg++ = '\0';
		if (!strcmp(cpopt, "ro") || !strcmp(cpopt, "o")) {
			if (fep && (fep->ex_exflags & MNT_EXRDONLY) == 0)
				syslog(LOG_WARNING, "ro failed for %s",
				       ep->ex_dirp);
			else
				*exflagsp |= MNT_EXRDONLY;
		} else if (!strcmp(cpopt, "root") || !strcmp(cpopt, "r")) {
			if (cpoptarg && isdigit(*cpoptarg)) {
				*rootuidp = atoi(cpoptarg);
				if (fep && fep->ex_rootuid != *rootuidp)
					syslog(LOG_WARNING,
					       "uid failed for %s",
					       ep->ex_dirp);
			} else
				syslog(LOG_WARNING,
				       "uid failed for %s",
				       ep->ex_dirp);
		} else if (!strcmp(cpopt, "alldirs") || !strcmp(cpopt, "a")) {
			*optflags |= EXOP_ALLDIRS;
#ifdef dosubdirs
		} else if (!strcmp(cpopt, "subdirs")) {
			*optflags |= EXOP_SUBDIRS;
#endif
		} else
			syslog(LOG_WARNING, "unknown opt %s ignored for %s",
			    cpopt, ep->ex_dirp);
		cpopt = cpoptend;
	}
}

#define	STRSIZ	(RPCMNT_NAMELEN+RPCMNT_PATHLEN+50)
/*
 * Routines that maintain the remote mounttab
 */
void
get_mountlist()
{
	register struct mountlist *mlp, **mlpp;
	register char *eos, *dirp;
	int len;
	char str[STRSIZ];
	FILE *mlfile;

	if (((mlfile = fopen(_PATH_RMOUNTLIST, "r")) == NULL) &&
	    ((mlfile = fopen(_PATH_RMOUNTLIST, "w")) == NULL)) {
		syslog(LOG_WARNING, "Can't open %s", _PATH_RMOUNTLIST);
		return;
	}
	mlpp = &mlhead;
	while (fgets(str, STRSIZ, mlfile) != NULL) {
		if ((dirp = index(str, '\t')) == NULL &&
		    (dirp = index(str, ' ')) == NULL)
			continue;
		mlp = (struct mountlist *)malloc(sizeof (*mlp));
		len = dirp-str;
		if (len > RPCMNT_NAMELEN)
			len = RPCMNT_NAMELEN;
		bcopy(str, mlp->ml_host, len);
		mlp->ml_host[len] = '\0';
		while (*dirp == '\t' || *dirp == ' ')
			dirp++;
		if ((eos = index(dirp, '\t')) == NULL &&
		    (eos = index(dirp, ' ')) == NULL &&
		    (eos = index(dirp, '\n')) == NULL)
			len = strlen(dirp);
		else
			len = eos-dirp;
		if (len > RPCMNT_PATHLEN)
			len = RPCMNT_PATHLEN;
		bcopy(dirp, mlp->ml_dirp, len);
		mlp->ml_dirp[len] = '\0';
		mlp->ml_next = (struct mountlist *)0;
		*mlpp = mlp;
		mlpp = &mlp->ml_next;
	}
	fclose(mlfile);
}

void
del_mlist(hostp, dirp)
	register char *hostp, *dirp;
{
	register struct mountlist *mlp, **mlpp;
	FILE *mlfile;
	int fnd = 0;

	mlpp = &mlhead;
	mlp = mlhead;
	while (mlp) {
		if (!strcmp(mlp->ml_host, hostp) &&
		    (!dirp || !strcmp(mlp->ml_dirp, dirp))) {
			fnd = 1;
			*mlpp = mlp->ml_next;
			free((caddr_t)mlp);
		}
		mlpp = &mlp->ml_next;
		mlp = mlp->ml_next;
	}
	if (fnd) {
		if ((mlfile = fopen(_PATH_RMOUNTLIST, "w")) == NULL) {
			syslog(LOG_WARNING, "Can't update %s", _PATH_RMOUNTLIST);
			return;
		}
		mlp = mlhead;
		while (mlp) {
			fprintf(mlfile, "%s %s\n", mlp->ml_host, mlp->ml_dirp);
			mlp = mlp->ml_next;
		}
		fclose(mlfile);
	}
}

void
add_mlist(hostp, dirp)
	register char *hostp, *dirp;
{
	register struct mountlist *mlp, **mlpp;
	FILE *mlfile;

	mlpp = &mlhead;
	mlp = mlhead;
	while (mlp) {
		if (!strcmp(mlp->ml_host, hostp) && !strcmp(mlp->ml_dirp, dirp))
			return;
		mlpp = &mlp->ml_next;
		mlp = mlp->ml_next;
	}
	mlp = (struct mountlist *)malloc(sizeof (*mlp));
	strncpy(mlp->ml_host, hostp, RPCMNT_NAMELEN);
	mlp->ml_host[RPCMNT_NAMELEN] = '\0';
	strncpy(mlp->ml_dirp, dirp, RPCMNT_PATHLEN);
	mlp->ml_dirp[RPCMNT_PATHLEN] = '\0';
	mlp->ml_next = (struct mountlist *)0;
	*mlpp = mlp;
	if ((mlfile = fopen(_PATH_RMOUNTLIST, "a")) == NULL) {
		syslog(LOG_WARNING, "Can't update %s", _PATH_RMOUNTLIST);
		return;
	}
	fprintf(mlfile, "%s %s\n", mlp->ml_host, mlp->ml_dirp);
	fclose(mlfile);
}

/*
 * This function is called via. SIGTERM when the system is going down.
 * It sends a broadcast RPCMNT_UMNTALL.
 */
void
send_umntall()
{
	(void) clnt_broadcast(RPCPROG_MNT, RPCMNT_VER1, RPCMNT_UMNTALL,
		xdr_void, (caddr_t)0, xdr_void, (caddr_t)0, umntall_each);
	exit(0);
}

int
umntall_each(resultsp, raddr)
	caddr_t resultsp;
	struct sockaddr_in *raddr;
{
	return (1);
}

/*
 * Free up an exports list component
 */
void
free_exp(ep)
	register struct exportlist *ep;
{
	register struct grouplist *grp;
	register char **addrp;
	struct grouplist *grp2;

	grp = ep->ex_groups;
	while (grp != NULL) {
		addrp = grp->gr_hp->h_addr_list;
		while (*addrp)
			free(*addrp++);
		free((caddr_t)grp->gr_hp->h_addr_list);
		free(grp->gr_hp->h_name);
		free((caddr_t)grp->gr_hp);
		grp2 = grp;
		grp = grp->gr_next;
		free((caddr_t)grp2);
	}
	free((caddr_t)ep);
}

/*
 * char *realpath(const char *path, char resolved_path[MAXPATHLEN])
 *
 * find the real name of path, by removing all ".", ".."
 * and symlink components.
 *
 * Jan-Simon Pendry, September 1991.
 * (received from Rick Macklem)
 */
char *
realpath(path, resolved)
	char *path;
	char resolved[MAXPATHLEN];
{
	int d = open(".", O_RDONLY);
	int rootd = 0;
	int hops = MAXSYMLINKS;
	char *p, *q;
	struct stat stb;
	char wbuf[MAXPATHLEN];

	strcpy(resolved, path);

	if (d < 0)
		return 0;

loop:;
	q = strrchr(resolved, '/');
	if (q) {
		p = q + 1;
		if (q == resolved)
			q = "/";
		else {
			do
				--q;
			while (q > resolved && *q == '/');
			q[1] = '\0';
			q = resolved;
		}
		if (chdir(q) < 0)
			goto out;
	} else
		p = resolved;

	if (lstat(p, &stb) == 0) {
		if (S_ISLNK(stb.st_mode)) {
			int n;

			if (hops-- <= 0)
				goto out;
			n = readlink(p, resolved, MAXPATHLEN);
			if (n < 0)
				goto out;
			resolved[n] = '\0';
			goto loop;
		}
		if (S_ISDIR(stb.st_mode)) {
			if (chdir(p) < 0)
				goto out;
			p = "";
		}
	}

	strcpy(wbuf, p);
	if (getcwd(resolved, MAXPATHLEN) == 0)
		goto out;
	if (resolved[0] == '/' && resolved[1] == '\0')
		rootd = 1;

	if (*wbuf) {
		if (strlen(resolved) + strlen(wbuf) + rootd + 1 > MAXPATHLEN) {
			errno = ENAMETOOLONG;
			goto out;
		}
		if (rootd == 0)
			strcat(resolved, "/");
		strcat(resolved, wbuf);
	}

	if (fchdir(d) < 0)
		goto out;
	(void) close(d);

	return resolved;

out:;
	(void) close(d);
	return 0;
}

#ifdef dosubdirs
/*
 * Check to see if <leaf> is a subdirectory of <top>.
 * Assumes that the names are canonical (do not contain symlinks).
 */
int
subdir(top, leaf)
	char *top, *leaf;
{
	int toplen = strlen(top);

	if (strlen(leaf) <= toplen || strncmp(top, leaf, toplen) != 0 ||
	    leaf[toplen] != '/')
		return (0);
	return (1);
}
#endif
