/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: isavar.h,v 1.6 1993/11/06 00:50:15 karels Exp $
 */

/*
 * ISA driver attach arguments
 */
struct isa_attach_args {
	u_short	ia_iobase;		/* base i/o address */
	u_short	ia_iosize;		/* span of ports used */
	u_short	ia_irq;			/* interrupt request */
	u_short	ia_drq;			/* DMA request */
	caddr_t ia_maddr;		/* physical i/o mem addr */
	u_int	ia_msize;		/* size of i/o memory */
	void	*ia_aux;		/* driver specific */
};

/*
 * per-device ISA variables
 */
struct isadev {
	struct  device *id_dev;		/* back pointer to generic */
	struct	isadev *id_bchain;	/* forward link in bus chain */	
};

/*
 * ISA masterbus 
 */
struct isa_softc {
	struct	device sc_dev;		/* base device */
	struct	isadev *sc_isadev;	/* list of all children */
};

#define		cf_iobase		cf_loc[0]
#define		cf_iosize		cf_loc[1]
#define		cf_maddr		cf_loc[2]
#define		cf_msize		cf_loc[3]
#define		cf_irq			cf_loc[4]
#define		cf_drq			cf_loc[5]

/*
 * Interrupt handler chains.  Interrupt handlers should return 0 for
 * `not I', 1 (`I took care of it'), or -1 (`I guess it was mine, but
 * I wasn't expecting it').  intr_establish() inserts a handler into
 * the list.  The handler is called with its (single) argument.
 */
struct intrhand {
	int	(*ih_fun)();
	void	*ih_arg;
	u_long	ih_count;
	struct	intrhand *ih_next;
} *intrhand[16];

void intr_establish __P((int intr, struct intrhand *, enum devclass));
int isa_portcheck __P((int base, int size));
void isa_portalloc __P((int base, int size));

/*
 * software conventions
 */
typedef enum { BUS_ISA, BUS_EISA, BUS_MCA } isa_type;

extern int atdevbase;		/* kernel virtual address of "hole" */
extern isa_type isa_bustype;	/* type of bus */

/*
 * Given a kernel virtual address for some location
 * in the "hole" I/O space, return a physical address.
 */
#define	ISA_PHYSADDR(v)	((caddr_t) ((int)(v) - atdevbase + IOM_BEGIN))
/*
 * Given a physical address in the "hole",
 * return a kernel virtual address.
 */
#define	ISA_HOLE_VADDR(p)  ((caddr_t) ((int) (p) - IOM_BEGIN + atdevbase))
