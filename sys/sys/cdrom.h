/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *      BSDI $Id: cdrom.h,v 1.1 1992/09/23 17:22:16 karels Exp $
 */

#define FRAMES_PER_SECOND 75
#define FRAMES_PER_MINUTE (FRAMES_PER_SECOND * 60)

struct track_info {
	int	initialized;
	int	track_num;
	int	control;
	int	start_frame;
	int	nframes;
};

struct cdinfo {
	int	fd;
	struct	cdfuncs *funcs;
	int	ntracks;
	struct	track_info *tracks;
	int	total_frames;
};

enum cdstate {
	cdstate_unknown,
	cdstate_stopped,
	cdstate_playing,
	cdstate_paused,
};

struct cdstatus {
	enum	cdstate state;
	int	control;
	int	track_num;
	int	index_num;
	int	rel_frame;
	int	abs_frame;
};

#include <sys/cdefs.h>

__BEGIN_DECLS
struct cdinfo *cdopen __P((char *fname));
int cdclose __P((struct cdinfo *));
int cdstop __P((struct cdinfo *));
int cdplay __P((struct cdinfo *, int, int));
int cdstatus __P((struct cdinfo *, struct cdstatus *));
int cdeject __P((struct cdinfo *));
int cdvolume __P((struct cdinfo *, int));

void frame_to_msf __P((int, int *, int *, int *));
int msf_to_frame __P((int, int, int));

int cd_suf __P((char *, int, char *, int, char *, int));
__END_DECLS
