/*-
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI $Id: iso9660_lookup.c,v 1.4 1993/12/17 04:06:54 karels Exp $
 */

/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
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
 * Started with
 *	ufs_lookup.c	7.33 (Berkeley) 5/19/91
 */

#include "param.h"
#include "namei.h"
#include "buf.h"
#include "file.h"
#include "vnode.h"
#include "mount.h"
#include "malloc.h"
#include "kernel.h"
#include "systm.h"

#include "iso9660.h"
#include "iso9660_node.h"

int
iso9660_diropen(imp, fileid, max_fileid, want_name, want_link, uio, dirpp)
	struct iso9660_mnt *imp;
	iso9660_fileid_t fileid;
	iso9660_fileid_t max_fileid;
	int want_name;
	int want_link;
	struct uio *uio;
	struct iso9660_dir_info **dirpp;
{
	struct iso9660_dir_info *dirp;
	int extra_space;
	char *p;

	extra_space = 0;
	if (want_name)
		extra_space += NAME_MAX + 1;

	if (want_link)
		extra_space += NAME_MAX + 1;

	MALLOC(dirp, struct iso9660_dir_info *, sizeof *dirp + extra_space,
	    M_TEMP, M_WAITOK);
	bzero(dirp, sizeof *dirp);

	p = (char *)dirp + sizeof *dirp;

	if (want_name) {
		dirp->name = p;
		dirp->name[0] = 0;
		p += NAME_MAX + 1;
	}

	if (want_link) {
		dirp->link = p;
		dirp->link[0] = 0;
		p += NAME_MAX + 1;
	}

	dirp->uio = uio;
	dirp->imp = imp;
	dirp->next_fileid = fileid;
	dirp->max_fileid = max_fileid;

	*dirpp = dirp;
	return (0);
}

int
iso9660_dirget(dirp)
	struct iso9660_dir_info *dirp;
{
	struct iso9660_mnt *imp;
	struct buf *bp;
	struct iso9660_directory_record *ep;
	int reclen;
	int lsecnum, lsecoff;
	

	if (dirp->error)
		return (dirp->error);

	dirp->fileid = dirp->next_fileid;

	imp = dirp->imp;
again:
	if (dirp->fileid + sizeof(struct iso9660_directory_record) >
	    dirp->max_fileid)
		return (ENOENT);

	lsecnum = dirp->fileid / ISO9660_LSS;
	lsecoff = dirp->fileid % ISO9660_LSS;

	bp = dirp->bp;

	if (bp == NULL || lsecoff == 0) {
		if (dirp->bp) {
			brelse(dirp->bp);
			dirp->bp = NULL;
		}

		/*
		 * can't depend on errors being reflected in the return
		 * value of bread in gamma2
		 */
		bread(imp->im_devvp, lsecnum * (ISO9660_LSS / DEV_BSIZE),
		      ISO9660_LSS, NOCRED, &bp);
		if (bp->b_flags & B_ERROR) {
			dirp->error = bp->b_error ? bp->b_error : EIO;
			brelse(bp);
			dirp->bp = NULL;
			return (dirp->error);
		}

		dirp->bp = bp;
	}

	ep = (struct iso9660_directory_record *)(bp->b_un.b_addr + lsecoff);

	reclen = iso9660num_711(ep->length);
	if (reclen == 0) {
		/* 
		 * This is a marker that means skip to next block.  It
		 * shouldn't ever happen in the first byte of a block,
		 * but make sure we don't get stuck here if it does.
		 */
		if ((dirp->fileid % ISO9660_LSS) == 0)
			dirp->fileid++;
		dirp->fileid = roundup(dirp->fileid, ISO9660_LSS);
		goto again;
	}

	/* Directory entries do not cross block boundaries. */
	if (lsecoff + reclen > ISO9660_LSS)
		goto bad_entry;

	if (dirp->error = iso9660_dir_parse(dirp, ep, reclen))
		goto bad_entry;

	dirp->next_fileid = dirp->fileid + reclen;

	return (0);

bad_entry:
	dirp->error = ENOENT;

	return (dirp->error);
}

int
iso9660_dir_parse (dirp, ep, reclen)
	struct iso9660_dir_info *dirp;
	struct iso9660_directory_record *ep;
	int reclen;
{
	struct iso9660_mnt *imp;
	int namelen;
	int suoffset, sulimit;
	int susp_len;
	struct susp *susp;
	struct susp_px *susp_px;
	struct susp_pn *susp_pn;
	struct susp_rr *susp_rr;
	struct susp_tf *susp_tf;
	struct susp_nm *susp_nm;
	struct susp_ce *susp_ce;
	struct susp_cl *susp_cl;
	struct susp_pl *susp_pl;
	struct susp_sl *susp_sl;
	struct susp_slc *slc;
	int rr_wanted;
	char *subase;
	int dot, dotdot;
	int ce_block, ce_offset, ce_length;
	int rr_name_offset;
	int block, sec, off;
	struct buf *bp = NULL;
	int error = 0;
	int len;
	int offset;
	int ts;
	int flags;
	char *np;
	char *name;
	char *p, *q;
	int cflags;
	int doing_relocated;
	int cl_block;

	imp = dirp->imp;

	bzero (&dirp->rr, sizeof dirp->rr);
	dirp->link_used = 0;
	dirp->link_separator = 0;

	name = NULL;
	rr_wanted = 0;

	if (dirp->name)
		rr_wanted |= RR_NM;

	name = iso9660_dir_name(ep);
	namelen = iso9660num_711(ep->name_len);

	dot = 0;
	dotdot = 0;
	if (namelen == 1) {
		/* iso9660 uses '\000' for . and '\001' for .. */
		if (name[0] == 0)
			dot = 1;
		else if (name[0] == 1)
			dotdot = 1;
	}

	doing_relocated = 0;

do_relocated:

	subase = (char *)ep;
	suoffset = roundup(ISO9660_DIR_NAME + iso9660num_711(ep->name_len), 2);

	if (dirp->fileid != imp->root_fileid)
		suoffset += imp->im_rock_ridge_skip;

	sulimit = iso9660num_711(ep->length);
	
	rr_wanted |= RR_PX | RR_PN | RR_TF | RR_SL;

	if (imp->im_raw ||
	    (imp->im_checked_for_rock_ridge && imp->im_have_rock_ridge == 0))
		rr_wanted = 0;

	dirp->size = iso9660num_733(ep->size);
	bcopy(ep->date, dirp->iso9660_time, sizeof dirp->iso9660_time);
	dirp->iso9660_flags = iso9660num_711(ep->flags);
	dirp->extent = iso9660num_733(ep->extent)
		+ iso9660num_711(ep->ext_attr_length);

	/*
	 * Rock Ridge does not support hard links, except the special
	 * cases of dot and dotdot.  Therefore, we can use the address
	 * of the directory entry as the fileid.  To handle the special
	 * case, we always use the address of the "." entry for directories.
	 */
	if (dirp->iso9660_flags & ISO9660_DIR_FLAG)
		dirp->translated_fileid = iso9660_lblktosize(imp,
		    (iso9660_fileid_t)dirp->extent);
	else
		dirp->translated_fileid = dirp->fileid;

	cl_block = 0;
	ce_block = 0;
	ce_offset = 0;
	ce_length = 0;
	rr_name_offset = 0;

	while (rr_wanted || dirp->uio) {
		if (suoffset + SUSP_MIN_SIZE > sulimit) {
			/*
			 * Used up current system usage area.  If we
			 * know a valid continuation, then do it now.
			 */
			if (ce_length < SUSP_MIN_SIZE) {
				if (cl_block == 0)
					break;
				ce_block = cl_block;
				ce_offset = 0;
				ce_length = ISO9660_LSS;
			}

			block = ce_block + (ce_offset / imp->im_bsize);
			off = ce_offset & (imp->im_bsize - 1);

			sec = block / (ISO9660_LSS / imp->im_bsize);
			off += (block % (ISO9660_LSS / imp->im_bsize)) 
				* imp->im_bsize;
			
			if (bp) {
				brelse(bp);
				bp = NULL;
			}

			if (error = bread(imp->im_devvp,
			    sec * (ISO9660_LSS / DEV_BSIZE), ISO9660_LSS,
			    NOCRED, &bp))
				goto done;

			/*
			 * Ignore continuation area if it claims to
			 * be bigger than a block.
			 *
			 * XXX This is wrong.  I've since learned that
			 * continuation area may span block boundaries.
			 * I think it can only happen with ridiculously
			 * long symbolic links, so I'll put off fixing
			 * it for a while.
			 */
			if (off + ce_length > ISO9660_LSS) {
				error = ENOENT;
				goto done;
			}

			if (cl_block) {
				ep = (struct iso9660_directory_record *)
					bp->b_un.b_addr;
				reclen = iso9660num_711(ep->length);
				cl_block = 0;
				doing_relocated = 1;
				goto do_relocated;
			}

			subase = bp->b_un.b_addr;
			suoffset = off;
			sulimit = off + ce_length;
			ce_length = 0;
		}
			
		susp = (struct susp *)(subase + suoffset);
		susp_len = iso9660num_711(susp->length);
			
		if (susp_len < SUSP_MIN_SIZE || suoffset + susp_len > sulimit)
			break;

		if (dirp->uio)
			uiomove (susp, susp_len, dirp->uio);

		switch (SUSP_CODE(susp->code[0], susp->code[1])) {
		case SUSP_CODE('R','R'):
			/*
			 * This field provides a bit map of which other
			 * fields are present.
			 */
			if (susp_len < SUSP_RR_LEN)
				break;
			susp_rr = (struct susp_rr *)susp;
			
			dirp->rr.rr_have_rr = 1;
			dirp->rr.rr_rr = iso9660num_711(susp_rr->flags);
			
			/* Stop looking for fields we will never find. */
			rr_wanted &= dirp->rr.rr_rr;
			break;
			
		case SUSP_CODE('P','X'):
			if (susp_len < SUSP_PX_LEN)
				break;
			susp_px = (struct susp_px *)susp;
			
			if ((rr_wanted & RR_PX) == 0)
				break;
			/* Only one PX is allowed. */
			rr_wanted &= ~RR_PX;
			
			dirp->rr.rr_flags |= RR_PX;
			dirp->rr.rr_mode = iso9660num_733(susp_px->mode);
			dirp->rr.rr_nlink = iso9660num_733(susp_px->nlink);
			dirp->rr.rr_uid = iso9660num_733(susp_px->uid);
			dirp->rr.rr_gid = iso9660num_733(susp_px->gid);
			break;
			
		case SUSP_CODE('P','N'):
			if (susp_len < SUSP_PN_LEN)
				break;
			susp_pn = (struct susp_pn *)susp;
			
			if ((rr_wanted & RR_PN) == 0)
				break;
			/* Only one PN is allowed. */
			rr_wanted &= ~RR_PN;
			
			dirp->rr.rr_flags |= RR_PN;
			dirp->rr.rr_dev_high =
			    iso9660num_733(susp_pn->dev_high);
			dirp->rr.rr_dev_low =
			    iso9660num_733(susp_pn->dev_low);

			if (dirp->rr.rr_dev_high)
				dirp->rr.rr_rdev=makedev(dirp->rr.rr_dev_high,
				    dirp->rr.rr_dev_low);
			else
				dirp->rr.rr_rdev = dirp->rr.rr_dev_low;
			break;
			
		case SUSP_CODE('T','F'):
			if (susp_len < SUSP_TF_LEN)
				break;
			susp_tf = (struct susp_tf *)susp;
			
			if ((rr_wanted & RR_TF) == 0)
				break;
			/* 
			 * There may be more than one TF field,
			 * so just because we found this one, we
			 * can't stop looking for more.
			 */
			flags = iso9660num_711(susp_tf->flags);
			len = (flags & RR_LONG_FORM) ?
			    RR_LONG_TIMESTAMP_LEN : RR_SHORT_TIMESTAMP_LEN;
			
			offset = SUSP_TF_LEN;
			for (ts = 0; ts < RR_TIMESTAMPS; ts++) {
				if (offset + len > susp_len)
					break;
				if ((flags & (1 << ts)) == 0)
					continue;
				if (ts >= RR_MODIFY && ts <= RR_ATTRIBUTES) {
					bcopy((char *)susp + offset,
					    dirp->rr.rr_time[TS_INDEX(ts)],
					    len);
					dirp->rr.rr_time_format[TS_INDEX(ts)] =
					    len;
				}
				offset += len;
			}
			break;
			
		case SUSP_CODE('N','M'):
			if (susp_len < SUSP_NM_LEN)
				break;
			susp_nm = (struct susp_nm *)susp;
			
			if ((rr_wanted & RR_NM) == 0)
				break;
			
			dirp->rr.rr_flags |= RR_NM;

			/* spec says ignore NM for . and .. */
			if (dot || dotdot)
				break;
			
			flags = susp_nm->flags[0];
			
			if (flags & RR_NAME_CURRENT) {
				np = ".";
				len = 1;
			} else if (flags & RR_NAME_PARENT) {
				np = "..";
				len = 2;
			} else if (flags & RR_NAME_HOST) {
				if (*hostname)
					np = hostname;
				else
					np = "localhost";
				len = strlen(np);
			} else {
				np = SUSP_NM_NAME(susp_nm);
				len = susp_len - SUSP_NM_LEN;
			}
			
			if (rr_name_offset + len > NAME_MAX) {
				error = ENOENT;
				goto done;
			}

			bcopy(np, dirp->name + rr_name_offset, len);
			
			rr_name_offset += len;
			dirp->name[rr_name_offset] = 0;
			dirp->namelen = rr_name_offset;
			
			if ((flags & RR_NAME_CONTINUE) == 0)
				rr_wanted &= ~RR_NM;
			
			break;
			
		case SUSP_CODE('S','L'):
			if (susp_len < SUSP_SL_LEN)
				break;
			susp_sl = (struct susp_sl *)susp;

			if ((rr_wanted & RR_SL) == 0)
				break;
			
			flags = iso9660num_711(susp_sl->flags);
			if ((flags & RR_NAME_CONTINUE) == 0)
				rr_wanted &= ~RR_SL;
			
			offset = SUSP_SL_LEN;

			while (offset + SUSP_SLC_LEN <= susp_len) {
				if (dirp->link_separator) {
					if (dirp->link_used + 1 >= NAME_MAX)
						goto symlink_too_long;
					if (dirp->link)
					    dirp->link[dirp->link_used] = '/';
					dirp->link_used++;
				}

				slc = (struct susp_slc *)((char *)susp+offset);
				cflags = iso9660num_711(slc->flags);
				len = -1;
				if (cflags & RR_NAME_CURRENT)
					np = ".";
				else if (cflags & RR_NAME_PARENT)
					np = "..";
				else if (cflags & RR_NAME_ROOT) {
					dirp->link_used = 0;
					np = "/";
				} else if (cflags & RR_NAME_VOLROOT)
					np = imp->im_mountp->
						mnt_stat.f_mntonname;
				else if (cflags & RR_NAME_HOST) {
					if (*hostname)
						np = hostname;
					else
						np = "localhost";
				} else {
					np = SLC_NAME(slc);
					len = iso9660num_711(slc->length);
				}

				if (len == -1)
					len = strlen (np);

				if (dirp->link_used + len > NAME_MAX)
					goto symlink_too_long;

				if (dirp->link)
					bcopy(np, dirp->link + dirp->link_used,
					      len);
				dirp->link_used += len;
				if ((cflags & RR_NAME_CONTINUE) == 0 &&
				    (cflags & RR_NAME_ROOT) == 0)
					dirp->link_separator = 1;
				else
					dirp->link_separator = 0;

				offset += SUSP_SLC_LEN +
				    iso9660num_711(slc->length);
			}
					
			if (dirp->link)
				dirp->link[dirp->link_used] = 0;
			break;

		symlink_too_long:
			rr_wanted &= ~RR_SL;
			dirp->link = NULL;
			break;

		case SUSP_CODE('S','T'):
			/* stop looking at this area */
			suoffset = sulimit;
			break;
			
		case SUSP_CODE('C','E'):
			/* Save pointer to continuation area. */
			if (susp_len < SUSP_CE_LEN)
				break;
			susp_ce = (struct susp_ce *)susp;
			
			ce_block = iso9660num_733(susp_ce->block);
			ce_offset = iso9660num_733(susp_ce->offset);
			ce_length = iso9660num_733(susp_ce->ce_length);
			break;

		case SUSP_CODE('C','L'):
			/*
			 * This file represents a directory that has been
			 * relocated.  Only believe the NM field here -
			 * get the rest of the information from the
			 * real "." entry.
			 */

			if (doing_relocated)
				break; /* illegal CL - avoid infinite loop */

			if (susp_len < SUSP_CL_LEN)
				break;
			susp_cl = (struct susp_cl *)susp;

			cl_block = iso9660num_733(susp_cl->loc);
			break;

		case SUSP_CODE('P','L'):
			/*
			 * This code only appears in the .. entry of a 
			 * directory that has been relocated.  The only
			 * special handling needed is to pick up the 
			 * "extent" from this entry, instead of from the
			 * iso9660 entry.
			 */
			if (susp_len < SUSP_PL_LEN)
				break;
			susp_pl = (struct susp_pl *)susp;
			dirp->extent = iso9660num_733(susp_pl->loc);
			dirp->translated_fileid = iso9660_lblktosize(imp,
				(iso9660_fileid_t)dirp->extent);
			break;

		case SUSP_CODE('R','E'):
			/* 
			 * Hidden directory containing relocated children.
			 * No special action needed, though we may want
			 * to hide this entry someday.
			 */
			break;

		}
		
		suoffset += susp_len;
	}

	if (dirp->name && (dirp->rr.rr_flags & RR_NM) == 0) {
		/* Didn't find Rock Ridge name, so use iso9660 name. */

		if (sizeof(struct iso9660_directory_record) + namelen > reclen
		    || namelen > NAME_MAX) {
			error = ENOENT;
			goto done;
		}
		
		if (dirp->name) {
			if (dot) {
				strcpy (dirp->name, ".");
				dirp->namelen = 1;
			} else if (dotdot) {
				strcpy (dirp->name, "..");
				dirp->namelen = 2;
			} else if (imp->im_raw) {
				bcopy(name, dirp->name, namelen);
				dirp->name[namelen] = 0;
				dirp->namelen = namelen;
			} else {
				/* 
				 * truncate at the semicolon
				 * delete trailing dot
				 * translate to lower case
				 */
				for (len = 0, p = name, q = dirp->name;
				    len < namelen && *p;
				    len++, p++, q++) {
					if (*p == ';')
						break;

					if (*p == '.' &&
					    (p[1] == ';' || p[1] == 0))
						break;

					if (*p >= 'A' && *p <= 'Z')
						*q = *p - 'A' + 'a';
					else
						*q = *p;
				}
				*q = 0;
				dirp->namelen = len;
			}
		}
	}
	
done:
	if (bp)
		brelse(bp);
	return (error);
}

void
iso9660_dirclose(dirp)
	struct iso9660_dir_info *dirp;
{

	if (dirp->bp)
		brelse(dirp->bp);

	FREE(dirp, M_TEMP);
}


int iso9660_enable_name_cache = 1;

int
iso9660_lookup(vdp, ndp, p)
	register struct vnode *vdp;
	register struct nameidata *ndp;
	struct proc *p;
{
	struct iso9660_dir_info *dirp;
	struct iso9660_mnt *imp;
	struct iso9660_node *dp;
	struct iso9660_node *pdp;
	struct iso9660_node *tdp;
	int error = 0;
	int lockparent;
	iso9660_fileid_t start_fileid, end_fileid;
	iso9660_fileid_t dir_first_fileid, dir_end_fileid;
	iso9660_fileid_t fileid;

	ndp->ni_dvp = vdp;
	ndp->ni_vp = NULL;
	dp = VTOI(vdp);
	imp = dp->i_mnt;
	lockparent = ndp->ni_nameiop & LOCKPARENT;

	if (iso9660_enable_name_cache == 0)
		goto skip_cache;

	if (vdp->v_type != VDIR)
		return (ENOTDIR);

	if (error = iso9660_access(vdp, VEXEC, ndp->ni_cred, p))
		return (error);

	if (error = cache_lookup(ndp)) {
		int vpid;

		if (error == ENOENT)
			return (error);

		/*
		 * Get the next vnode in the path.
		 * See comment below starting `Step through' in ufs_lookup for
		 * an explaination of the locking protocol.
		 */
		pdp = dp;
		dp = VTOI(ndp->ni_vp);
		vdp = ndp->ni_vp;
		vpid = vdp->v_id;
		if (pdp == dp) {
			VREF(vdp);
			error = 0;
		} else if (ndp->ni_isdotdot) {
			ISO9660_IUNLOCK(pdp);
			error = vget(vdp);
			if (!error && lockparent && *ndp->ni_next == '\0')
				ISO9660_ILOCK(pdp);
		} else {
			error = vget(vdp);
			if (!lockparent || error || *ndp->ni_next != '\0')
				ISO9660_IUNLOCK(pdp);
		}
		/*
		 * Check that the capability number did not change
		 * while we were waiting for the lock.
		 */
		if (!error) {
			if (vpid == vdp->v_id)
				return (0);
			iso9660_iput(dp);
			if (lockparent && pdp != dp && *ndp->ni_next == '\0')
				ISO9660_IUNLOCK(pdp);
		}
		ISO9660_ILOCK(pdp);
		dp = pdp;
		vdp = ITOV(dp);
		ndp->ni_vp = NULL;
	}

skip_cache:
	dir_first_fileid = iso9660_lblktosize (imp,
	    (iso9660_fileid_t) dp->extent);
	dir_end_fileid = dir_first_fileid + dp->size;

	start_fileid = dp->dir_last_fileid;
	end_fileid = dir_end_fileid;

	if (start_fileid < dir_first_fileid || start_fileid >= dir_end_fileid)
		start_fileid = dir_first_fileid;

	if (error=iso9660_diropen(imp, start_fileid, end_fileid, 1, 0, NULL,
	    &dirp))
		return (error);

again:
	while (dirp->next_fileid < end_fileid &&
	    (error = iso9660_dirget(dirp)) == 0) {
		if (dirp->namelen == ndp->ni_namelen &&
		    !bcmp(ndp->ni_ptr, dirp->name, ndp->ni_namelen))
			goto found;
	}

	if (start_fileid != dir_first_fileid) {
		end_fileid = start_fileid;
		start_fileid = dir_first_fileid;
		dirp->next_fileid = dir_first_fileid;
		goto again;
	}

	if (error == 0)
		error = ENOENT;
	goto done;

found:
	if (*ndp->ni_next == '\0')
		dp->dir_last_fileid = dirp->fileid;

	fileid = dirp->translated_fileid;

	if (ndp->ni_isdotdot) {
		ISO9660_IUNLOCK(dp);	/* race to get the inode */

		/*
		 * The ".." entries on some CD-ROM's have the wrong size.
		 * So, if we are getting "..", open the corresponding "."
		 * and get the size and other "inode" parameters from there.
		 */
		iso9660_dirclose(dirp);
		if (error = iso9660_diropen(imp, fileid, ISO9660_MAX_FILEID,
		    0, 0, NULL, &dirp)) {
			ISO9660_ILOCK(dp);
			return (error);
		}
		
		if (error = iso9660_dirget(dirp)) {
			ISO9660_ILOCK(dp);
			goto done;
		}

		if (error = iso9660_iget(dp, fileid, &tdp, dirp)){
			ISO9660_ILOCK(dp);
			goto done;
		}
		if (lockparent && *ndp->ni_next == '\0')
			ISO9660_ILOCK(dp);
		ndp->ni_vp = ITOV(tdp);
	} else if (dp->fileid == fileid) {
		VREF(vdp);	/* we want ourself, ie "." */
		ndp->ni_vp = vdp;
	} else {
		if (error = iso9660_iget(dp, fileid, &tdp, dirp))
			goto done;
		if (!lockparent || *ndp->ni_next != '\0')
			ISO9660_IUNLOCK(dp);
		ndp->ni_vp = ITOV(tdp);
	}

	/*
	 * Insert name into cache if appropriate.
	 */
	if (ndp->ni_makeentry)
		cache_enter(ndp);

done:
	iso9660_dirclose(dirp);
	return (error);
}
