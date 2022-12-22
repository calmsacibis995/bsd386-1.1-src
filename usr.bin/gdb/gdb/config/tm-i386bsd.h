/*	BSDI $Id: tm-i386bsd.h,v 1.3 1993/03/08 07:21:28 polk Exp $	*/

#define	START_INFERIOR_TRAPS_EXPECTED	2

#include "tm-i386v.h"

#undef	FRAME_NUM_ARGS
#define	FRAME_NUM_ARGS(val, fi)		(val = -1)

#undef	COFF_NO_LONG_FILE_NAMES
#define	NAMES_HAVE_UNDERSCORE		1
#define	NEED_TEXT_START_END		1

extern int stop_stack_dummy;
extern void
i386_pop_dummy_frame PARAMS ((void));
#undef POP_FRAME
#define	POP_FRAME \
	(stop_stack_dummy ? i386_pop_dummy_frame() : i386_pop_frame())

#define	nounderscore			1	/* for cplus_dem.c */

#ifdef KERNELDEBUG
#define	MEM_DEVICE			2

extern int kernel_debugging;
#undef FRAME_CHAIN_VALID
#define FRAME_CHAIN_VALID(chain, thisframe) \
	(chain != 0 && \
	 kernel_debugging ? inside_kernstack(chain) : \
		(!inside_entry_file(FRAME_SAVED_PC(thisframe))))

extern int kernel_xfer_memory();
#define	KERNEL_XFER_MEMORY		1
#endif
