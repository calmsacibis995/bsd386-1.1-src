/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: mcdioctl.h,v 1.1 1992/09/22 19:42:42 karels Exp $
 */

#define MCDIOCSETBUF	_IOW('m', 150, int)
#define MCDIOCCMD	_IOW('m', 151, struct mcd_cmd)

#define MCDMAXARGS 20

struct mcd_cmd {
	char	args[MCDMAXARGS];
	int	nargs;
	char	*results;
	int	nresults;
};
