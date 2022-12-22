/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: dos_disksubr.c,v 1.3 1994/01/05 20:50:51 karels Exp $
 */

#include "param.h"
#include "systm.h"
#include "disklabel.h"
#include "reboot.h"
#include "buf.h"

/*
 * Functions specific to interactions with DOS
 * on PC/AT-style machines.
 */
#include "machine/bootblock.h"

#define b_cylin b_resid		/* XXX */

struct biosgeom biosgeom[2];
#ifdef BSDGEOM
struct bsdgeom bsdgeom;
#endif

/*
 * Check BIOS-reported disk parameters
 * and CMOS value for presence of disks.
 */
setbiosgeom(bgp)
	struct biosgeom *bgp;
{

	biosgeom[0] = bgp[0];
	biosgeom[1] = bgp[1];
}

#ifdef BSDGEOM
setbsdgeom(bgp)
	struct bsdgeom *bgp;
{

	bsdgeom = *bgp;
}
#endif

/*
 * Examine a possible master boot block/DOS partition table
 * to find the location of the BSD root or other primary partition.
 * Find either the (only) BSD active or first BSD file system partition
 * and return index of this partition.
 * Return -1 if there is no partition table, or it is corrupted.
 */
static
rootpart(mbp)
	struct masterboot *mbp;
{
	struct mbpart *mp;
	int i, nact, iact, iboot, ibsd;
	
	if (mbp->signature != MB_SIGNATURE)
		return (-1);
	iact = -1;
	iboot = -1;
	ibsd = -1;

	/*
	 * Scan the partitions looking for the following,
	 * listed in order of preference:
	 *   1.  active BSD partition
	 *   2.  bootable BSD partition (bootany-type)
	 *   3.  any other BSD partition
	 * If there is more than one of the same preference,
	 * other than active, take the first found.  If there is any
	 * inconsistency, this may not be a partition table; return -1.
	 */
	for (nact = i = 0, mp = mbparts(mbp); i < MB_MAXPARTS; i++, mp++) {
		switch (mp->active) {
		case MBA_ACTIVE:
			if (++nact > 1)
				return (-1);
			if (mp->system == MBS_BSDI)
				iact = i;
			break;

		case MBA_NOTACTIVE:
			if (ibsd < 0 && mp->system == MBS_BSDI)
				ibsd = i;
			break;

		default:
			/*
			 * bootany marks bootable partitions by changing
			 * the active indicator to the system type,
			 * and the system value to MBS_BOOTANY.
			 * If there is a "bootable" BSD partition
			 * and no active BSD partition, take the bootable one.
			 */
			if (mp->system == MBS_BOOTANY) {
				if (mp->active == MBS_BSDI && iboot < 0)
					iboot = i;
				break;
			}
			return (-1);
		}
	}
	if (iact >= 0)
		return (iact);
	if (iboot >= 0)
		return (iboot);
	return (ibsd);
}	

/*
 * Check a disk for a DOS partition table, and return the DOS partition
 * entry for the BSD partition as determined by rootpart (above).
 * Returns 0, storing the partition entry as indicated by mpp,
 * or an error if no correct partition table is found.
 * Used to locate the BSD disklabel by the disk drivers.
 * The entire partition entry is returned so that the driver
 * can use either the starting block number or the cyl/track/sector info
 * (the geometry may not be known yet).
 */
getbsdpartition(dev, strat, lp, mpp)
	dev_t dev;
	int (*strat)();
	register struct disklabel *lp;
	struct mbpart *mpp;
{
	register struct buf *bp;
	struct masterboot *mbp;
	struct partition *pp = &lp->d_partitions[0];
	u_long p0size = pp->p_size;
	u_long p0off = pp->p_offset;
	int ia, error;

	if (lp->d_secperunit == 0)
		lp->d_secperunit = 0x1fffffff;
	if (lp->d_npartitions < 1)
		lp->d_npartitions = 1;
	if (lp->d_partitions[0].p_size == 0)
		lp->d_partitions[0].p_size = 0x1fffffff;
	lp->d_partitions[0].p_offset = 0;

	bp = geteblk((int)lp->d_secsize);
	bp->b_dev = dev;
	bp->b_blkno = 0;
	bp->b_bcount = lp->d_secsize;
	bp->b_flags = B_BUSY | B_READ;
	bp->b_cylin = 0;
	(*strat)(bp);
	if ((error = biowait(bp)) == 0) {
		mbp = (struct masterboot *)(bp->b_un.b_addr);
		ia = rootpart(mbp);
		if (ia >= 0) {
			/*
			 * hack: if the bsd partition starts at the beginning
			 * of the disk, it cannot have an offset of 0 (the
			 * partition table lives there, not our boot block).
			 * However, the BSD boot block may be at the end
			 * of the second-stage bootstrap rather than the
			 * beginning.  In this case, the DOS paritition
			 * table is set to use an offset of 15, but the
			 * BSD partition starts at 0.  "Fix" this here.
			 */
			*mpp = *(mbparts(mbp) + ia);
			if (mbpssec(mpp) == 15 && mbpstrk(mpp) == 0 &&
			    mbpscyl(mpp) == 0) {
			    	mpp->ssec = 1;	/* clobbers high cyl bits */
			    	mpp->start = 0;
			}
		} else
			error = EINVAL;
	}
	bp->b_flags = B_INVAL | B_AGE;
	brelse(bp);
	pp->p_size = p0size;
	pp->p_offset = p0off;
	return (error);
}
