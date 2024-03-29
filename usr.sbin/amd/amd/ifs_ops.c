/*
 * Copyright (c) 1989 Jan-Simon Pendry
 * Copyright (c) 1989 Imperial College of Science, Technology & Medicine
 * Copyright (c) 1989 The Regents of the University of California.
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
 *	@(#)ifs_ops.c	5.3 (Berkeley) 5/12/91
 *
 * $Id: ifs_ops.c,v 1.4 1992/01/06 03:34:39 kolstad Exp $
 *
 */

#include "am.h"

#ifdef HAS_IFS

/*
 * Inheritance file system.
 * This implements a filesystem restart.
 *
 * This is a *gross* hack - it knows far too
 * much about the way other parts of the
 * sytem work.  See restart.c too.
 */
static char not_a_filesystem[] = "Attempting to inherit not-a-filesystem";
/*
 * This should never be called.
 */
/*ARGSUSED*/
static char *ifs_match P((am_opts *fo));
static char *ifs_match(fo)
am_opts *fo;
{
	plog(XLOG_FATAL, "ifs_match called!");
	return 0;
}

static int ifs_init P((mntfs *mf));
static int ifs_init(mf)
mntfs *mf;
{
	mntfs *mf_link = (mntfs *) mf->mf_private;
	if (mf_link == 0) {
		plog(XLOG_FATAL, not_a_filesystem);
		return EINVAL;
	}
#ifdef notdef
	/*
	 * Fill in attribute fields
	 */
	mf_link->mf_fattr.type = NFLNK;
	mf_link->mf_fattr.mode = NFSMODE_LNK | 0777;
	mf_link->mf_fattr.nlink = 1;
	mf_link->mf_fattr.size = MAXPATHLEN / 4;
#endif
	if (mf_link->mf_ops->fs_init)
		return (*mf_link->mf_ops->fs_init)(mf_link);
	return 0;
}

static mntfs *ifs_inherit P((mntfs *mf));
static mntfs *ifs_inherit(mf)
mntfs *mf;
{
	/*
	 * Take the linked mount point and
	 * propogate.
	 */
	mntfs *mf_link = (mntfs *) mf->mf_private;
	if (mf_link == 0) {
		plog(XLOG_FATAL, not_a_filesystem);
		return 0;	/*XXX*/
	}

	mf_link->mf_fo = mf->mf_fo;
#ifdef notdef
	mf_link->mf_fattr.fileid = mf->mf_fattr.fileid;
#endif /* notdef */

	/*
	 * Discard the old map.
	 * Don't call am_unmounted since this
	 * node was never really mounted in the
	 * first place.
	 */
	mf->mf_private = 0;
	free_mntfs(mf);
	/*
	 * Free the dangling reference
	 * to the mount link.
	 */
	free_mntfs(mf_link);
	/*
	 * Get a hold of the other entry
	 */
	mf_link->mf_flags &= ~MFF_RESTART;

	/* Say what happened */
	plog(XLOG_INFO, "restarting %s on %s", mf_link->mf_info, mf_link->mf_mount);

	return mf_link;
}

static int ifs_mount P((am_node *mp));
static int ifs_mount(mp)
am_node *mp;
{
	mntfs *newmf = ifs_inherit(mp->am_mnt);
	if (newmf) {
		mp->am_mnt = newmf;
		/*
		 * XXX - must do the am_mounted call here
		 */
		if (newmf->mf_ops->fs_flags & FS_MBACKGROUND)
			am_mounted(mp);

		new_ttl(mp);
		return 0;
	}
	return EINVAL;
}

static int ifs_fmount P((mntfs *mf));
static int ifs_fmount(mf)
mntfs *mf;
{
	am_node *mp = find_mf(mf);
	if (mp)
		return ifs_mount(mp);
	return ifs_inherit(mf) ? 0 : EINVAL;
}

/*ARGSUSED*/
static int ifs_fumount P((mntfs *mf));
static int ifs_fumount(mf)
mntfs *mf;
{
	/*
	 * Always succeed
	 */
	return 0;
}

/*
 * Ops structure
 */
am_ops ifs_ops = {
	"inherit",
	ifs_match,
	ifs_init,
	ifs_mount,
	ifs_fmount,
	auto_fumount,
	ifs_fumount,
	efs_lookuppn,
	efs_readdir,
	0, /* ifs_readlink */
	0, /* ifs_mounted */
	0, /* ifs_umounted */
	find_afs_srvr,
	FS_DISCARD
};

#endif /* HAS_IFS */
