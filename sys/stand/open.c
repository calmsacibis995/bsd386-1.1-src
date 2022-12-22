/*
 * Copyright (c) 1991, 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	$Id: open.c,v 1.10 1992/09/29 01:09:27 karels Exp $
 */

#include <sys/param.h>
#include <sys/reboot.h>
#include <string.h>

#include "saio.h"

extern int opendev, bootdev;

int
open(spec, mode)
	char *spec;
	int mode;
{
	register int i, d;
	int len;
	char *params;
	char *filename;
	char *p;
	register struct iob *iobp;
	int nparam = 0;
	int paramval[4];
	extern int ndevs;

	/*
	 * Allocate a file structure.
	 */
	for (i = 0; i < SOPEN_MAX; ++i)
		if ((iob[i].i_flgs & F_ALLOC) == 0)
			break;

	if (i == SOPEN_MAX)
		_stop("open: file table full");
	iobp = &iob[i];
	iobp->i_flgs |= F_ALLOC;

#ifndef SMALL
	/*
	 * Parse the file spec.
	 * First step: get the device name and map it to a device number.
	 */
	if ((params = index(spec, '(')) == 0) {
		/*
		 * The i_dev field must currently remain as 0 for SMALL,
		 * as there is only one entry in a SMALL conf device switch.
		 */
		iobp->i_dev = B_TYPE(bootdev);
#endif
		filename = spec;
		iobp->i_flgs |= F_FILE;
		opendev = bootdev;
		iobp->i_adapt = B_ADAPTOR(bootdev);
		iobp->i_ctlr = B_CONTROLLER(bootdev);
		iobp->i_unit = B_UNIT(bootdev);
		iobp->i_part = B_PARTITION(bootdev);
#ifndef SMALL
	} else {
		len = params - spec;

		for (d = 0; d < ndevs; ++d)
			if (devsw[d].dv_name &&
			    strlen(devsw[d].dv_name) == len &&
			    bcmp(devsw[d].dv_name, spec, len) == 0)
				break;
		if (d == ndevs)
			goto enxio;
		iobp->i_dev = d;

		/*
		 * Look for a filename, if any.
		 */
		if ((filename = index(params, ')')) == 0)
			goto enxio;
		if (*++filename != '\0')
			iobp->i_flgs |= F_FILE;

		/*
		 * Parse the comma-delimited list of numbers in the spec.
		 * It looks like this: [[[[adapter,] controller,] unit,] part]
		 * Missing values are assumed to be 0.
		 */
		for (++params; nparam < 4; params = p + 1) {
			paramval[nparam++] = strtol(params, &p, 0);
			if (*p == ')')
				break;
			if (*p != ',')
				goto enxio;
		}
		if (*p != ')')
			goto enxio;
		d = 0;
		switch (nparam) {
		case 4:
			iobp->i_adapt = paramval[d++];
		case 3:
			iobp->i_ctlr = paramval[d++];
		case 2:
			iobp->i_unit = paramval[d++];
		case 1:
			iobp->i_part = paramval[d++];
		case 0:
			break;
		}
		opendev = MAKEBOOTDEV(iobp->i_dev, iobp->i_adapt, iobp->i_ctlr,
		    iobp->i_unit, iobp->i_part);
	}
#endif

	/*
	 * Call the device-specific open routine.
	 * The driver may read the disk label here.
	 */
	if (errno = devopen(iobp)) {
#ifndef SMALL
		bzero(iobp, sizeof *iobp);
#endif
		return (-1);
	}

	/*
	 * If the spec named a file, try to read its inode.
	 */
	if (iobp->i_flgs & F_FILE) {
		if ((errno = findfs(iobp)) || (errno = namei(iobp, filename))){
#ifndef SMALL
			/* XXX save errno? */
			devclose(iobp);
			bzero(iobp, sizeof *iobp);
#endif
			return (-1);
		}
	}

	errno = 0;
	return (i + 3);

enxio:
#ifndef SMALL
	bzero(iobp, sizeof *iobp);
#endif
	errno = ENXIO;
	return (-1);
}
