/*	BSDI	$Id: pstat.c,v 1.1.1.1 1994/01/13 23:13:31 polk Exp $	*/
/*
 * Copyright (c) 1994 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*-
 * Copyright (c) 1980, 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
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
static char copyright[] =
"@(#) Copyright (c) 1980, 1991, 1993\n\
	The Regents of the University of California.  All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)pstat.c	8.5 (Berkeley) 1/4/94";
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/vnode.h>
#include <sys/map.h>
#include <sys/ucred.h>
#include <sys/kinfo.h>
#define KERNEL
#include <sys/file.h>
#include <ufs/quota.h>
#include <ufs/inode.h>
#define NFS
#include <sys/mount.h>
#undef NFS
#undef KERNEL
#include <sys/stat.h>
#include <nfs/nfsnode.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <vm/swap_pager.h>

#define ISO9660
#undef  VTOI
#include <iso9660/iso9660.h>
#include <iso9660/iso9660_node.h>


#include <err.h>
#include <kvm.h>
#include <limits.h>
#include <nlist.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pathnames.h"

char *devname();
char *user_from_uid();
char *getbsize();
 


struct nlist nl[] = {
#define VM_SWAPMAP	0
	{ "_swapmap" },	/* list of free swap areas */
#define VM_NSWAPMAP	1
	{ "_nswapmap" },/* size of the swap map */
#define VM_SWDEVT	2
	{ "_swdevt" },	/* list of swap devices and sizes */
#define VM_SWAPSTATS	3
	{ "_swapstats" },/* swap info */
#define VM_DMMAX	4
	{ "_dmmax" },	/* maximum size of a swap block */
#define V_ROOTFS	5
	{ "_rootfs" },
#define V_NUMV		6
	{ "_numvnodes" },
#define	FNL_NFILE	7
	{"_nfiles"},
#define FNL_MAXFILE	8
	{"_maxfiles"},
#define NLMANDATORY FNL_MAXFILE	/* names up to here are mandatory */
#define VM_NISWAP	NLMANDATORY + 1
	{ "_niswap" },
#define VM_NISWDEV	NLMANDATORY + 2
	{ "_niswdev" },
#define	SCONS		NLMANDATORY + 3
	{ "_cons" },
#define	SPTY		NLMANDATORY + 4
	{ "_pt_tty" },
#define	SNPTY		NLMANDATORY + 5
	{ "_npty" },

#ifdef hp300
#define	SDCA	(SNPTY+1)
	{ "_dca_tty" },
#define	SNDCA	(SNPTY+2)
	{ "_ndca" },
#define	SDCM	(SNPTY+3)
	{ "_dcm_tty" },
#define	SNDCM	(SNPTY+4)
	{ "_ndcm" },
#define	SDCL	(SNPTY+5)
	{ "_dcl_tty" },
#define	SNDCL	(SNPTY+6)
	{ "_ndcl" },
#define	SITE	(SNPTY+7)
	{ "_ite_tty" },
#define	SNITE	(SNPTY+8)
	{ "_nite" },
#endif

#ifdef mips
#define SDC	(SNPTY+1)
	{ "_dc_tty" },
#define SNDC	(SNPTY+2)
	{ "_dc_cnt" },
#endif

	{ "" }
};

int	usenumflag;
int	totalflag;
char	*nlistf	= NULL;
char	*memf	= NULL;
kvm_t	*kd;

#define	SVAR(var) __STRING(var)	/* to force expansion */
#define	KGET(idx, var)							\
	KGET1(idx, &var, sizeof(var), SVAR(var))
#define	KGET1(idx, p, s, msg)						\
	KGET2(nl[idx].n_value, p, s, msg)
#define	KGET2(addr, p, s, msg)						\
	if (kvm_read(kd, (u_long)(addr), p, s) != s)			\
		warnx("cannot read %s: %s", msg, kvm_geterr(kd))
#define	KGETRET(addr, p, s, msg)					\
	if (kvm_read(kd, (u_long)(addr), p, s) != s) {			\
		warnx("cannot read %s: %s", msg, kvm_geterr(kd));	\
		return (0);						\
	}

void	filemode __P((void));
int	getfiles __P((char **, int *));
struct mount *
	getmnt __P((struct mount *));
struct e_vnode *
	kinfo_vnodes __P((int *));
struct e_vnode *
	loadvnodes __P((int *));
void	mount_print __P((struct mount *));
void	nfs_header __P((void));
int	nfs_print __P((struct vnode *));
void	swapinfo __P((void));
void	ttymode __P((void));
void	ttyprt __P((struct tty *, int));
void	ttytype __P((struct tty *, char *, int, int));
void	ufs_header __P((void));
int	ufs_print __P((struct vnode *));
void	usage __P((void));
void	vnode_header __P((void));
void	vnode_print __P((struct vnode *, struct vnode *));
void	vnodemode __P((void));
void	iso_header __P((void));
int	iso_print __P((struct vnode *));


char *pname;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	int ch, i, quit, ret;
	int fileflag, swapflag, ttyflag, vnodeflag;
	char buf[_POSIX2_LINE_MAX];

	pname = argv[0];

	fileflag = swapflag = ttyflag = vnodeflag = 0;
	while ((ch = getopt(argc, argv, "TM:N:finstv")) != EOF)
		switch (ch) {
		case 'f':
			fileflag = 1;
			break;
		case 'M':
			memf = optarg;
			break;
		case 'N':
			nlistf = optarg;
			break;
		case 'n':
			usenumflag = 1;
			break;
		case 's':
			swapflag = 1;
			break;
		case 'T':
			totalflag = 1;
			break;
		case 't':
			ttyflag = 1;
			break;
		case 'v':
		case 'i':		/* Backward compatibility. */
			vnodeflag = 1;
			break;
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	/*
	 * Discard setgid privileges if not the running kernel so that bad
	 * guys can't print interesting stuff from kernel memory.
	 */
	if (nlistf != NULL || memf != NULL)
		(void)setgid(getgid());

	if ((kd = kvm_openfiles(nlistf, memf, NULL, O_RDONLY, buf)) == 0)
		errx(1, "kvm_openfiles: %s", buf);
	if ((ret = kvm_nlist(kd, nl)) != 0) {
		if (ret == -1)
			errx(1, "kvm_nlist: %s", kvm_geterr(kd));
		for (i = quit = 0; i <= NLMANDATORY; i++)
			if (!nl[i].n_value) {
				quit = 1;
				warnx("undefined symbol: %s\n", nl[i].n_name);
			}
		if (quit)
			exit(1);
	}
	if (!(fileflag | vnodeflag | ttyflag | swapflag | totalflag))
		usage();
	if (fileflag || totalflag)
		filemode();
	if (vnodeflag || totalflag)
		vnodemode();
	if (ttyflag)
		ttymode();
	if (swapflag || totalflag)
		swapinfo();
	exit (0);
}

struct e_vnode {
	struct vnode *avnode;
	struct vnode vnode;
};

void
vnodemode()
{
	register struct e_vnode *e_vnodebase, *endvnode, *evp;
	register struct vnode *vp;
	register struct mount *maddr, *mp;
	int numvnodes;

	e_vnodebase = loadvnodes(&numvnodes);
	if (totalflag) {
		(void)printf("%7d vnodes\n", numvnodes);
		return;
	}
	endvnode = e_vnodebase + numvnodes;
	(void)printf("%d active vnodes\n", numvnodes);


#define ST	mp->mnt_stat
	maddr = NULL;
	for (evp = e_vnodebase; evp < endvnode; evp++) {
		vp = &evp->vnode;
		if (vp->v_mount != maddr) {
			/*
			 * New filesystem
			 */
			if ((mp = getmnt(vp->v_mount)) == NULL)
				continue;
			maddr = vp->v_mount;
			mount_print(mp);
			vnode_header();
			switch(ST.f_type) {
			case MOUNT_UFS:
			case MOUNT_MFS:
				ufs_header();
				break;
			case MOUNT_NFS:
				nfs_header();
				break;
			case MOUNT_ISO9660:
				iso_header();
				break;
			case MOUNT_NONE:
			case MOUNT_MSDOS:
			default:
				break;
			}
			(void)printf("\n");
		}
		vnode_print(evp->avnode, vp);
		switch(ST.f_type) {
		case MOUNT_UFS:
		case MOUNT_MFS:
			ufs_print(vp);
			break;
		case MOUNT_NFS:
			nfs_print(vp);
			break;
		case MOUNT_ISO9660:
			iso_print(vp);
			break;
		case MOUNT_NONE:
		case MOUNT_MSDOS:
		default:
			break;
		}
		(void)printf("\n");
	}
	free(e_vnodebase);
}

void
vnode_header()
{
	(void)printf("ADDR     TYP VFLAG  USE HOLD");
}

void
vnode_print(avnode, vp)
	struct vnode *avnode;
	struct vnode *vp;
{
	char *type, flags[16]; 
	char *fp = flags;
	register int flag;

	/*
	 * set type
	 */
	switch(vp->v_type) {
	case VNON:
		type = "non"; break;
	case VREG:
		type = "reg"; break;
	case VDIR:
		type = "dir"; break;
	case VBLK:
		type = "blk"; break;
	case VCHR:
		type = "chr"; break;
	case VLNK:
		type = "lnk"; break;
	case VSOCK:
		type = "soc"; break;
	case VFIFO:
		type = "fif"; break;
	case VBAD:
		type = "bad"; break;
	default: 
		type = "unk"; break;
	}
	/*
	 * gather flags
	 */
	flag = vp->v_flag;
	if (flag & VROOT)
		*fp++ = 'R';
	if (flag & VTEXT)
		*fp++ = 'T';
	if (flag & VSYSTEM)
		*fp++ = 'S';
	if (flag & VXLOCK)
		*fp++ = 'L';
	if (flag & VXWANT)
		*fp++ = 'W';
	if (flag & VBWAIT)
		*fp++ = 'B';
	if (flag & VALIASED)
		*fp++ = 'A';
	if (flag == 0)
		*fp++ = '-';
	*fp = '\0';
	(void)printf("%8x %s %5s %4d %4d",
	    avnode, type, flags, vp->v_usecount, vp->v_holdcnt);
}

static void
v_mode(mode_t mode, int type)
{
	char buf[16];           /* at least 11 plus NULL */

	switch (type) {
	case VREG:
		mode |= S_IFREG;
		break;
	case VDIR:
		mode |= S_IFDIR;
		break;
	case VBLK:
		mode |= S_IFBLK;
		break;
	case VCHR:
		mode |= S_IFCHR;
		break;
	case VLNK:
		mode |= S_IFLNK;
		break;
	case VSOCK:
		mode |= S_IFSOCK;
		break;
	case VFIFO:
		mode |= S_IFIFO;
		break;
	}
	strmode(mode, buf);
	printf(" %10s", buf);
}

static void
v_uid(uid_t uid)
{
	char *s = user_from_uid(uid, 1);

/*
 * SPC uses 12-character usernames. Others can shorten this to the usual 8.
 */
	if (s)
		printf("%12.12s ", s);
	else
		printf("%12d ", uid);
}

void
ufs_header() 
{
	(void)printf("     FILEID IFLAG       MODE          UID   RDEV|SZ");
}

int
ufs_print(vp) 
	struct vnode *vp;
{
	register int flag;
	struct inode inode, *ip = &inode;
	char flagbuf[16], *flags = flagbuf;
	char *name;
	mode_t type;

	ip = (struct inode *) vp->v_data;
	flag = ip->i_flag;
	if (flag & ILOCKED)
		*flags++ = 'L';
	if (flag & IWANT)
		*flags++ = 'W';
	if (flag & IRENAME)
		*flags++ = 'R';
	if (flag & IUPD)
		*flags++ = 'U';
	if (flag & IACC)
		*flags++ = 'A';
	if (flag & ICHG)
		*flags++ = 'C';
	if (flag & IMOD)
		*flags++ = 'M';
	if (flag & ISHLOCK)
		*flags++ = 'S';
	if (flag & IEXLOCK)
		*flags++ = 'E';
	if (flag & ILWAIT)
		*flags++ = 'Z';
	if (flag == 0)
		*flags++ = '-';
	*flags = '\0';

        (void)printf(" %10d %5s", ip->i_number, flagbuf);
        type = ip->i_mode & S_IFMT;
        /* BSDI enhancement - should work elsewhere */
        v_mode(ip->i_mode, vp->v_type);
        v_uid(ip->i_uid);
        if (S_ISCHR(ip->i_mode) || S_ISBLK(ip->i_mode))
                if (usenumflag || ((name = devname(ip->i_rdev, type)) == NULL)){
                        (void)sprintf(flags = flagbuf, "%d, %d",
                            major(ip->i_rdev), minor(ip->i_rdev));
                        (void)printf(" %8.8s", flags);
                }
                else
                        (void)printf(" %8s", name);
        else
                (void)printf(" %8d", ip->i_size);

	return (0);
}

void
nfs_header() 
{
	(void)printf("     FILEID NFLAG       MODE          UID   RDEV|SZ");

}

int
nfs_print(vp) 
	struct vnode *vp;
{
	struct nfsnode nfsnode, *np = &nfsnode;
	char flagbuf[16], *flags = flagbuf;
	register int flag;
	char *name;
	mode_t type;

	np = (struct nfsnode *) vp->v_data;
	flag = np->n_flag;
	if (flag & NLOCKED)
		*flags++ = 'L';
	if (flag & NWANT)
		*flags++ = 'W';
	if (flag & NMODIFIED)
		*flags++ = 'M';
	if (flag & NWRITEERR)
		*flags++ = 'E';
	if (flag == 0)
		*flags++ = '-';
	*flags = '\0';

#define VT	np->n_vattr
	(void)printf(" %10u %5s", VT.va_fileid, flagbuf);
	type = VT.va_mode & S_IFMT;
	/* BSDI enhancement - should work elsewhere */
	v_mode(VT.va_mode, vp->v_type);
	v_uid(VT.va_uid);
	if (S_ISCHR(VT.va_mode) || S_ISBLK(VT.va_mode))
		if (usenumflag || ((name = devname(VT.va_rdev, type)) == NULL)){
			(void)sprintf(flags = flagbuf, "%d, %d",
			    major(VT.va_rdev), minor(VT.va_rdev));
			printf(" %8.8s", flags);
		}
		else
			(void)printf(" %8s", name);
	else
		(void)printf(" %8d", np->n_size);
	return (0);
}

void
iso_header()
{
	(void)printf("     FILEID XFLAG       MODE          UID   RDEV|SZ");
}

int
iso_print(vp)
	struct vnode *vp;
{
	register int flag;
	struct iso9660_node isonode, *isop = &isonode;
	char flagbuf[16], *flags = flagbuf;
	char *name;
	mode_t type;

	isop = (struct iso9660_node *) vp->v_data;
	flag = isop->i_flag;
	if (flag & NLOCKED)
		*flags++ = 'L';
	if (flag & NWANT)
		*flags++ = 'W';
	if (flag == 0)
		*flags++ = '-';
	*flags = '\0';
	(void)printf(" %10d %5s", isop->fileid, flagbuf);
	type = isop->rr.rr_mode & S_IFMT;
	/* BSDI enhancement - should work elsewhere */
	v_mode(isop->rr.rr_mode, vp->v_type);
	v_uid(isop->rr.rr_uid);
	if (S_ISCHR(isop->rr.rr_mode) || S_ISBLK(isop->rr.rr_mode))
		if (usenumflag || 
		    ((name = devname(isop->rr.rr_rdev, type)) == NULL)) {
			(void)sprintf(flags = flagbuf, "%d, %d",
			    major(isop->rr.rr_rdev), minor(isop->rr.rr_rdev));
			printf(" %8.8s", flags);
		}
		else
			(void)printf(" %8s", name);
	else
		(void)printf(" %8d", isop->size);
	return (0);
}

	
/*
 * Given a pointer to a mount structure in kernel space,
 * read it in and return a usable pointer to it.
 */
struct mount *
getmnt(maddr)
	struct mount *maddr;
{
	static struct mtab {
		struct mtab *next;
		struct mount *maddr;
		struct mount mount;
	} *mhead = NULL;
	register struct mtab *mt;

	for (mt = mhead; mt != NULL; mt = mt->next)
		if (maddr == mt->maddr)
			return (&mt->mount);
	if ((mt = malloc(sizeof(struct mtab))) == NULL)
		err(1, NULL);
	KGETRET(maddr, &mt->mount, sizeof(struct mount), "mount table");
	mt->maddr = maddr;
	mt->next = mhead;
	mhead = mt;
	return (&mt->mount);
}

void
mount_print(mp)
	struct mount *mp;
{
	register int flags;
	char *type;

#define ST	mp->mnt_stat
	(void)printf("*** MOUNT ");
	switch (ST.f_type) {
	case MOUNT_NONE:
		type = "none";
		break;
	case MOUNT_UFS:
		type = "ufs";
		break;
	case MOUNT_NFS:
		type = "nfs";
		break;
	case MOUNT_MFS:
		type = "mfs";
		break;
	case MOUNT_MSDOS:
		type = "msdos";
		break;
	case MOUNT_ISO9660:
		type = "iso9660";
		break;
	default:
		type = "unknown";
		break;
	}
	(void)printf("%s %s on %s", type, ST.f_mntfromname, ST.f_mntonname);
	if (flags = mp->mnt_flag) {
		char *comma = "(";

		putchar(' ');
		/* user visable flags */
		if (flags & MNT_RDONLY) {
			(void)printf("%srdonly", comma);
			flags &= ~MNT_RDONLY;
			comma = ",";
		}
		if (flags & MNT_SYNCHRONOUS) {
			(void)printf("%ssynchronous", comma);
			flags &= ~MNT_SYNCHRONOUS;
			comma = ",";
		}
		if (flags & MNT_NOEXEC) {
			(void)printf("%snoexec", comma);
			flags &= ~MNT_NOEXEC;
			comma = ",";
		}
		if (flags & MNT_NOSUID) {
			(void)printf("%snosuid", comma);
			flags &= ~MNT_NOSUID;
			comma = ",";
		}
		if (flags & MNT_NODEV) {
			(void)printf("%snodev", comma);
			flags &= ~MNT_NODEV;
			comma = ",";
		}
		if (flags & MNT_EXPORTED) {
			(void)printf("%sexport", comma);
			flags &= ~MNT_EXPORTED;
			comma = ",";
		}
		if (flags & MNT_EXRDONLY) {
			(void)printf("%sexrdonly", comma);
			flags &= ~MNT_EXRDONLY;
			comma = ",";
		}
		if (flags & MNT_LOCAL) {
			(void)printf("%slocal", comma);
			flags &= ~MNT_LOCAL;
			comma = ",";
		}
		if (flags & MNT_QUOTA) {
			(void)printf("%squota", comma);
			flags &= ~MNT_QUOTA;
			comma = ",";
		}
		/* filesystem control flags */
		if (flags & MNT_UPDATE) {
			(void)printf("%supdate", comma);
			flags &= ~MNT_UPDATE;
			comma = ",";
		}
		if (flags & MNT_MLOCK) {
			(void)printf("%slock", comma);
			flags &= ~MNT_MLOCK;
			comma = ",";
		}
		if (flags & MNT_MWAIT) {
			(void)printf("%swait", comma);
			flags &= ~MNT_MWAIT;
			comma = ",";
		}
		if (flags & MNT_MPBUSY) {
			(void)printf("%sbusy", comma);
			flags &= ~MNT_MPBUSY;
			comma = ",";
		}
		if (flags & MNT_MPWANT) {
			(void)printf("%swant", comma);
			flags &= ~MNT_MPWANT;
			comma = ",";
		}
		if (flags & MNT_UNMOUNT) {
			(void)printf("%sunmount", comma);
			flags &= ~MNT_UNMOUNT;
			comma = ",";
		}
		if (flags)
			(void)printf("%sunknown_flags:%x", comma, flags);
		(void)printf(")");
	}
	(void)printf("\n");
#undef ST
}

struct e_vnode *
loadvnodes(avnodes)
	int *avnodes;
{
	size_t copysize, expected;
	struct e_vnode *vnodebase;

	if (memf != NULL) {
		/*
		 * do it by hand
		 */
		return (kinfo_vnodes(avnodes));
	}
	expected = copysize = getkerninfo(KINFO_VNODE, NULL, NULL, 0);
	if (copysize < 0)
		err(1, "KINFO_VNODE size");
	if ((vnodebase = malloc(copysize)) == NULL)
		err(1, NULL);
	expected = getkerninfo(KINFO_VNODE, vnodebase, &copysize, 0);
	if (expected != copysize)
		err(1, "KINFO_VNODE"); 
	if (copysize % sizeof(struct e_vnode))
		errx(1, "vnode size mismatch"); 
	*avnodes = copysize / sizeof(struct e_vnode);
	return (vnodebase);
}

/*
 * simulate what a running kernel does in in kinfo_vnode
 */
struct e_vnode *
kinfo_vnodes(avnodes)
	int *avnodes;
{
	struct mount *mp, mount, *rootfs;
	struct vnode *vp, vnode;
	char *vbuf, *evbuf, *bp;
	int num=0, numvnodes;

#define VPTRSZ  sizeof(struct vnode *)
#define VNODESZ sizeof(struct vnode)

	KGET(V_NUMV, numvnodes);
	if ((vbuf = malloc((numvnodes + 20) * (VPTRSZ + VNODESZ))) == NULL)
		err(1, NULL);
	bp = vbuf;
	evbuf = vbuf + (numvnodes + 20) * (VPTRSZ + VNODESZ);
	KGET(V_ROOTFS, rootfs);
	mp = rootfs;
	do {
		KGET2(mp, &mount, sizeof(mount), "mount entry");
		for (vp = mount.mnt_mounth; vp; vp = vnode.v_mountf) {
			KGET2(vp, &vnode, sizeof(vnode), "vnode");
			if ((bp + VPTRSZ + VNODESZ) > evbuf)
				/* XXX - should realloc */
				errx(1, "no more room for vnodes");
			memmove(bp, &vp, VPTRSZ);
			bp += VPTRSZ;
			memmove(bp, &vnode, VNODESZ);
			bp += VNODESZ;
			num++;
		}
	} while ((mp = mount.mnt_next) != rootfs);
	*avnodes = num;
	return ((struct e_vnode *)vbuf);
}
	
char hdr[]=
    "  LINE  RAW CAN OUT  HWT LWT     ADDR    COL STATE  SESS  PGID DISC\n";

int ttyspace = 128;

void
ttymode()
{
	char *s;
	int i, ntds;
	struct ttydevice_tmp *td;
	struct tty *tp;
	int bufsize, expected;
		
	/*      
	 * XXX
	 * Add emulation of KINFO_TTY_... here.
	 */
	if (memf != NULL)
		errx(1, "terminals on dead kernel, not implemented\n");

	/* 
	 * call getkerninfo(KINFO_TTY_NAMES_TMP) to get an array of struct
	 * ttydevice_tmp's
	 */
	expected = bufsize = getkerninfo(KINFO_TTY_NAMES_TMP, NULL, NULL, 0);
	if (bufsize < 0) {
		perror("KINFO_TTY_NAMES_TMP size");
		exit(1);
	}
	td = (struct ttydevice_tmp *) malloc(bufsize);
	if ((expected = getkerninfo(KINFO_TTY_NAMES_TMP, (char *) td,
		    &bufsize, 0)) < 0) {
		perror("KINFO_TTY_NAMES_TMP");
		exit(1);
	}
	if (expected != bufsize) {
		fprintf(stderr, "getkerninfo(KINFO_TTY_NAMES_TMP): wrong size (expectd %d, got %d)\n",
		    bufsize, expected);
		exit(1);
	}
	ntds = bufsize / sizeof(*td);   /* number of buffer entries */
	/*
	 * call getkerninfo(KINFO_TTY_STATS_TMP) to get an array of struct
	 * tty's
	 */
	expected = bufsize = getkerninfo(KINFO_TTY_STATS_TMP, NULL, NULL, 0);
	if (bufsize < 0) {
		perror("KINFO_TTY_STATS_TMP size");
		exit(1);
	}
	tp = (struct tty *) malloc(bufsize);
	if ((expected = getkerninfo(KINFO_TTY_STATS_TMP, (char *) tp,
		    &bufsize, 0)) < 0) {
		perror("KINFO_TTY_STATS_TMP");
		exit(1);
	}
	if (expected != bufsize) {
		fprintf(stderr, "getkerninfo(KINFO_TTY_STATS_TMP): wrong size (expectd %d, got %d)\n",
		    bufsize, expected);
		exit(1);
	}
	for (; ntds > 0; ntds--, td++) {
		(void)printf("%d %s line%s\n", td->tty_count, td->tty_name,
		     td->tty_count > 1 ? "s" : "");
		(void)printf(hdr);
		for (i = 0; i < td->tty_count; tp++, i++)
			ttyprt(tp, i);
	}
}

void
ttytype(tty, name, type, number)
	register struct tty *tty;
	char *name;
	int type, number;
{
	register struct tty *tp;
	int ntty;

	if (tty == NULL)
		return;
	KGET(number, ntty);
	(void)printf("%d %s %s\n", ntty, name, (ntty == 1) ? "line" : "lines");
	if (ntty > ttyspace) {
		ttyspace = ntty;
		if ((tty = realloc(tty, ttyspace * sizeof(*tty))) == 0)
			err(1, NULL);
	}
	KGET1(type, tty, ntty * sizeof(struct tty), "tty structs");
	(void)printf(hdr);
	for (tp = tty; tp < &tty[ntty]; tp++)
		ttyprt(tp, tp - tty);
}

struct {
	int flag;
	char val;
} ttystates[] = {
	{ TS_WOPEN,	'W'},
	{ TS_ISOPEN,	'O'},
	{ TS_CARR_ON,	'C'},
	{ TS_TIMEOUT,	'T'},
	{ TS_FLUSH,	'F'},
	{ TS_BUSY,	'B'},
	{ TS_ASLEEP,	'A'},
	{ TS_XCLUDE,	'X'},
	{ TS_TTSTOP,	'S'},
	{ TS_TBLOCK,	'K'},
	{ TS_ASYNC,	'Y'},
	{ TS_BKSL,	'D'},
	{ TS_ERASE,	'E'},
	{ TS_LNCH,	'L'},
	{ TS_TYPEN,	'P'},
	{ TS_CNTTB,	'N'},
	{ 0,	       '\0'},
};

void
ttyprt(tp, line)
	register struct tty *tp;
	int line;
{
	register int i, j;
	pid_t pgid;
	char *name, state[20];

	if (usenumflag || tp->t_dev == 0 ||
	   (name = devname(tp->t_dev, S_IFCHR)) == NULL)
		(void)printf("%8d ", line); 
	else
		(void)printf("%8s ", name);
	(void)printf("%2d %3d ", tp->t_rawq.c_cc, tp->t_canq.c_cc);
	(void)printf("%3d %4d %3d %8x %6d ", tp->t_outq.c_cc,
		tp->t_hiwat, tp->t_lowat, tp->t_addr, tp->t_col);
	for (i = j = 0; ttystates[i].flag; i++)
		if (tp->t_state&ttystates[i].flag)
			state[j++] = ttystates[i].val;
	if (j == 0)
		state[j++] = '-';
	state[j] = '\0';
	(void)printf("%-4s %6x", state, (u_long)tp->t_session & ~KERNBASE);
	pgid = 0;
	if (tp->t_pgrp != NULL)
		pgid = (pid_t) tp->t_pgrp;
	(void)printf("%6d ", pgid);
	switch (tp->t_line) {
	case TTYDISC:
		(void)printf("term\n");
		break;
	case TABLDISC:
		(void)printf("tablet\n");
		break;
	case SLIPDISC:
		(void)printf("slip\n");
		break;
	default:
		(void)printf("%d\n", tp->t_line);
		break;
	}
}

void
filemode()
{
	register struct file *fp;
	struct file *addr;
	char *buf, flagbuf[16], *fbp;
	int len, maxfile, nfile;
	static char *dtypes[] = { "???", "inode", "socket" };

	KGET(FNL_MAXFILE, maxfile);
	if (totalflag) {
		KGET(FNL_NFILE, nfile);
		(void)printf("%3d/%3d files\n", nfile, maxfile);
		return;
	}
	if (getfiles(&buf, &len) == -1)
		return;
	/*
	 * Getfiles returns in malloc'd memory a pointer to the first file
	 * structure, and then an array of file structs (whose addresses are
	 * derivable from the previous entry).
	 */
	addr = *((struct file **)buf);
	fp = (struct file *)(buf + sizeof(struct file *));
	nfile = (len - sizeof(struct file *)) / sizeof(struct file);
	
	(void)printf("%d/%d open files\n", nfile, maxfile);
	(void)printf("   LOC   TYPE    FLG     CNT  MSG    DATA    OFFSET\n");
	for (; (char *)fp < buf + len; addr = fp->f_filef, fp++) {
		if ((unsigned)fp->f_type > DTYPE_SOCKET)
			continue;
		(void)printf("%x ", addr);
		(void)printf("%-8.8s", dtypes[fp->f_type]);
		fbp = flagbuf;
		if (fp->f_flag & FREAD)
			*fbp++ = 'R';
		if (fp->f_flag & FWRITE)
			*fbp++ = 'W';
		if (fp->f_flag & FAPPEND)
			*fbp++ = 'A';
#ifdef FSHLOCK	/* currently gone */
		if (fp->f_flag & FSHLOCK)
			*fbp++ = 'S';
		if (fp->f_flag & FEXLOCK)
			*fbp++ = 'X';
#endif
		if (fp->f_flag & FASYNC)
			*fbp++ = 'I';
		*fbp = '\0';
		(void)printf("%6s  %3d", flagbuf, fp->f_count);
		(void)printf("  %3d", fp->f_msgcount);
		(void)printf("  %8.1x", fp->f_data);
		if (fp->f_offset < 0)
			(void)printf("  %x\n", fp->f_offset);
		else
			(void)printf("  %d\n", fp->f_offset);
	}
	free(buf);
}

int
getfiles(abuf, alen)
	char **abuf;
	int *alen;
{
	size_t len, expected;
	char *buf;

	/*
	 * XXX
	 * Add emulation of KINFO_FILE here.
	 */
	if (memf != NULL)
		errx(1, "files on dead kernel, not implemented\n");
	expected = len = getkerninfo(KINFO_FILE, NULL, NULL, 0);
	if (expected < 0) {
		warn("KINFO_FILE size");
		return (-1);
	}
	if ((buf = malloc(len)) == NULL)
		err(1, NULL);
	expected = getkerninfo(KINFO_FILE, buf, &len, 0);
	if (expected != len) {
		warn("KINFO_FILE");
		return (-1);
	}
	*abuf = buf;
	*alen = len;
	return (0);
}

void
swapinfo()
{

	struct swapstats swap;
	struct swdevt *sw;
	int i;
	char *header;
	int	hl;
	int	bs;

	KGET(VM_SWAPSTATS, swap);

	header = getbsize(pname, &hl, &bs);
	if (totalflag) {
		(void)printf("%d/%d swap space (%s)\n", 
		    (int)(((quad_t)(swap.swap_total - swap.swap_free)
			* DEV_BSIZE) / bs),
		    (int)(((quad_t)swap.swap_total * DEV_BSIZE) / bs),
		    header);
		return;
	}

	if ((sw = malloc(swap.swap_devs * sizeof(*sw))) == NULL)
		err(1, NULL);

	KGET1(VM_SWDEVT, sw, swap.swap_devs * sizeof(*sw), "swdevt");
	if (usenumflag) {
		(void)printf("Major,Minor          %s\n", header);
		for (i = 0; i < swap.swap_devs; i++) {
			char name[100];
			sprintf(name,"%d,%d",
			    major(sw[i].sw_dev), minor(sw[i].sw_dev));
			printf ("%-20s %d\n", name, 
			    (int)((u_quad_t)sw[i].sw_nblks * DEV_BSIZE / bs) );
		}
	} else {
		(void)printf("Device name          %s\n", header);
		for (i = 0; i < swap.swap_devs; i++) {
			char * name;
			name = devname(sw[i].sw_dev, (mode_t)S_IFBLK);
			printf ("/dev/%-15s %d\n", name, 
			    (int)((u_quad_t)sw[i].sw_nblks * DEV_BSIZE / bs) );
		}
	}
	(void)printf("%d (%s) allocated, %d (%s) available\n", 
	    (int)(((quad_t)(swap.swap_total - swap.swap_free) * DEV_BSIZE)
		/ bs),
	    header,
	    (int)(((quad_t)swap.swap_free * DEV_BSIZE) / bs),
	    header);
}


void
usage()
{
	(void)fprintf(stderr,
	    "usage: pstat -Tfnstv [system] [-M core] [-N system]\n");
	exit(1);
}
