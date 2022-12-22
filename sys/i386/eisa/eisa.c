/*-
 * Copyright (c) 1992, 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: eisa.c,v 1.3 1993/12/23 05:30:30 karels Exp $
 */

#include "param.h"
#include "device.h"

#include "i386/isa/isavar.h"
#include "eisa.h"

static char *eisa_product_id __P((int slot));

/*
 * The slot map starts out with slot 0 allocated,
 * since that's reserved for the CPU and ISA ports.
 */
unsigned short eisa_slot_map = 0x0001;

/* can we use this slot? */
#define	EISA_SLOTCHECK(slot)	((eisa_slot_map & (1 << (slot))) == 0)

void
eisa_slotalloc(slot)
	int slot;
{

	eisa_slot_map |= 1 << slot;
}

/*
 * Look for a matching device by checking slots.
 * If the config file specified a slot with an I/O port base,
 * force a match on that slot only.
 * XXX We may need to break this up for drivers that care about
 * XXX more than just the slot number...
 */
int
eisa_match(cf, ia)
	struct cfdata *cf;
	struct isa_attach_args *ia;
{
	int slot = ia->ia_iobase >> 12;
	char **cf_ids = (char **)cf->cf_driver->cd_aux;
	char *id, **cidp;

	if (slot) {
		if (EISA_SLOTCHECK(slot) == 0 ||
		    (id = eisa_product_id(slot)) == NULL)
			return (0);
		for (cidp = cf_ids; *cidp; ++cidp)
			if (strcmp(*cidp, id) == 0)
				return (slot);
		return (0);
	}

	for (slot = 1; slot < EISA_NUM_PHYS_SLOT; ++slot)
		if (EISA_SLOTCHECK(slot) && (id = eisa_product_id(slot))) {
			for (cidp = cf_ids; *cidp; ++cidp)
				if (strcmp(*cidp, id) == 0)
					return (slot);
		}
	return (0);
}

#ifdef DEBUG
/*
 * Print a list device identifiers for occupied slots.
 */
void
eisa_print_devmap()
{
	int slot;
	char *id;

	printf(" [ ");
	for (slot = 1; slot < EISA_NUM_PHYS_SLOT; ++slot)
		if (id = eisa_product_id(slot))
			printf("(%d)%s ", slot, id);
	printf("]");
}
#endif

/*
 * For a given physical slot number, return a pointer to an ID string
 * if we can, otherwise return NULL.  The string is static.
 * This routine is analogous to the BIOS Read Physical Slot call.
 * XXX The BIOS routine worries about memory refresh; should we?
 * XXX Let's at least check twice, for paranoia's sake.
 */
static char *
eisa_product_id(slot)
	int slot;
{
	static char id[8];
	int p = EISA_PROD_ID_BASE(slot);
	u_long compressed_id;

	outb(p, 0xff);
	outb(p, 0xff);
	if (inb(p) & 0x80 && inb(p) & 0x80)
		return (NULL);

	compressed_id = (u_char)inb(p) |
	    (u_char)inb(p + 1) << 8 |
	    (u_char)inb(p + 2) << 16 |
	    (u_char)inb(p + 3) << 24;

	/* EISA sure picked a funky way of packing characters */
	id[0] = (compressed_id >> 2 & 0x1f) + '@';
	id[1] = (compressed_id << 3 & 0x18 | compressed_id >> 13 & 0x7) + '@';
	id[2] = (compressed_id >> 8 & 0x1f) + '@';
#define	tohex(x)	((x) >= 0xa ? (x) - 0xa + 'A' : (x) + '0')
	id[3] = tohex(compressed_id >> 20 & 0xf);
	id[4] = tohex(compressed_id >> 16 & 0xf);
	id[5] = tohex(compressed_id >> 28 & 0xf);
	id[6] = tohex(compressed_id >> 24 & 0xf);
	id[7] = '\0';

	return (id);
}
