/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: dos_disksubr.c,v 1.3 1994/01/05 20:12:06 karels Exp $
 */

#include "param.h"
#include "reboot.h"
#include "i386/include/bootblock.h"
#include "i386/isa/rtc.h"

/*
 * Functions specific to interactions with DOS
 * on PC/AT-style machines.
 */

struct biosgeom *biosgeom[2];
#ifdef BSDGEOM
struct bsdgeom *bsdgeom;
#endif

/*
 * Check BIOS-reported disk parameters
 * and CMOS value for presence of disks.
 */
setbiosgeom(bgp)
	struct biosgeom *bgp;
{
	int types;

	types = rtcin(RTC_DISKTYPE);
	if ((types & 0xF0) == 0)
		bgp[0].ncylinders = 0;
	else
		biosgeom[0] = &bgp[0];
	if ((types & 0xF) == 0)
		bgp[1].ncylinders = 0;
	else
		biosgeom[1] = &bgp[1];
}

#ifdef BSDGEOM
setbsdgeom(bgp)
	struct bsdgeom *bgp;
{

	bsdgeom = bgp;
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
 * Check block 0 from a disk for a DOS partition table, and return the DOS
 * partition entry for the BSD partition as determined by rootpart (above).
 * Returns 0, storing the partition entry as indicated by mpp,
 * or an error if no correct partition table is found.
 * Used to locate the BSD disklabel by the disk drivers.
 * A pointer to the partition entry is returned so that the driver
 * can use either the starting block number or the cyl/track/sector info
 * (the geometry may not be known yet).
 */
struct mbpart *
getbsdpartition(buf)
	char *buf;
{
	struct masterboot *mbp;
	struct mbpart *mp;
	int ia, error;

	mbp = (struct masterboot *) buf;
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
		mp = mbparts(mbp) + ia;
		if (mbpssec(mp) == 15 && mbpstrk(mp) == 0 &&
		    mbpscyl(mp) == 0) {
			mp->ssec = 1;	/* clobbers high cyl bits */
			mp->start = 0;
		}
		return (mp);
	}
	return (NULL);
}
