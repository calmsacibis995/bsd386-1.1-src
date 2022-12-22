/*-
 * Copyright (c) 1991, 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: vga.c,v 1.11 1993/12/19 04:46:10 karels Exp $
 */

/*
 * Graphics display driver for vga/386.
 */

#include "param.h"
#include "proc.h"
#include "ioctl.h"
#include "file.h"
#include "malloc.h"

#include "machine/cpu.h"

#include "vm/vm.h"
#include "vm/vm_kern.h"
#include "vm/vm_page.h"
#include "vm/vm_pager.h"

#include "specdev.h"
#include "vnode.h"
#include "mman.h"
#include "device.h"

#include "isa.h"
#include "isavar.h"
#include "vgaioctl.h"
#include "vgavar.h"

int	vgaprobe();
void	vgaattach();
void	vgasave();
void	vgarestore();

struct cfdriver vgacd =
	{ NULL, "vga", vgaprobe, vgaattach, sizeof(struct vga_softc) };

/*
 * Normal init routine called by configure() code
 */
/* ARGSUSED */
vgaprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;

	/* XXX it'd be nice if we really checked to see if a card exists */
	ia->ia_irq = 0;		/* device doesn't interrupt */
	if (ia->ia_maddr && ia->ia_msize && ia->ia_iobase) {
		ia->ia_iosize = VGA_NPORT;
		return (1);
	}
	return (0);
}

/* ARGSUSED */
void
vgaattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	register struct vga_softc *vgap = (struct vga_softc *) self;
	register struct isa_attach_args *ia = (struct isa_attach_args *) aux;

	printf("\n");
	vgap->vga_mem_addr = ia->ia_maddr;
	vgap->vga_mem_size = ia->ia_msize;
	vgap->vga_io_addr = ia->ia_iobase;
}

vgaopen(dev, flags)
	dev_t dev;
{
	int unit = VGAUNIT(dev);
	register struct vga_softc *vgap;

	/* Validate unit number */
	if (unit >= vgacd.cd_ndevs || (vgap = vgacd.cd_devs[unit]) == NULL)
		return (ENXIO);
	if ((vgap->vga_flags & VGA_OPEN) == VGA_OPEN)
		return (EBUSY);
	/*
	 * First open.
	 */
	vgap->vga_flags |= VGA_OPEN;

	vgasave(vgap);
	return (0);
}

/*ARGSUSED*/
vgaclose(dev, flags)
	dev_t dev;
{
	struct vga_softc *vgap = vgacd.cd_devs[VGAUNIT(dev)];

	vgap->vga_flags = VGA_DEAD;
	vgarestore(vgap);
	return (0);
}

/*ARGSUSED*/
vgaioctl(dev, cmd, data, flag, p)
	dev_t dev;
	caddr_t data;
	struct proc *p;
{
	register struct vga_softc *vgap = vgacd.cd_devs[VGAUNIT(dev)];
	int error;

	error = 0;
	switch (cmd) {

	case VGAIOCMAP:
		error = vgammap(dev, (caddr_t *)data, p);
		break;

	case VGAIOCUNMAP:
		error = vgaunmmap(dev, *(caddr_t *)data, p);
		break;

	default:
		error = EINVAL;
		break;

	}
	return (error);
}

/*ARGSUSED*/
vgaselect(dev, rw)
	dev_t dev;
{
	if (rw == FREAD)
		return (0);
	return (1);
}

/*ARGSUSED*/
vgamap(dev, off, prot)
	dev_t dev;
	int off;
{
	struct vga_softc *vgap = vgacd.cd_devs[VGAUNIT(dev)];
	u_int paddr;

	/*
	 * Should limit mapping to VGA memory size, possibly adding
	 * the text memory.  For now, limit to IO memory space.
	 */
#if 0
	if (off + NBPG > vgap->vga_mem_size)
		return (-1);
#else
	if (off + NBPG > IOM_END)
		return (-1);
#endif
	return (((u_int)vgap->vga_mem_addr + off) >> PGSHIFT);
}


vgammap(dev, addrp, p)
	dev_t dev;
	caddr_t *addrp;
	struct proc *p;
{
	struct vga_softc *vgap = vgacd.cd_devs[VGAUNIT(dev)];
	int len, error;
	struct vnode vn;
	struct specinfo si;
	int flags;

	len = vgap->vga_mem_size;
	flags = MAP_FILE|MAP_SHARED;
	if (*addrp)
		flags |= MAP_FIXED;
	else
		return (1);
	vn.v_type = VCHR;
	vn.v_specinfo = &si;
	vn.v_rdev = dev;
	error = vm_mmap(&p->p_vmspace->vm_map, (vm_offset_t *)addrp,
	    (vm_size_t)len, VM_PROT_ALL, VM_PROT_ALL, flags, (caddr_t)&vn, 0);
	return (error);
}

vgaunmmap(dev, addr, p)
	dev_t dev;
	caddr_t addr;
	struct proc *p;
{
	struct vga_softc *vgap = vgacd.cd_devs[VGAUNIT(dev)];
	vm_size_t size;
	int rv;

	if (addr == 0)
		return (EINVAL);
	size = round_page(0x10000);
	rv = vm_deallocate(p->p_vmspace->vm_map, (vm_offset_t)addr, size);
	return (rv == KERN_SUCCESS ? 0 : EINVAL);
}

extern u_short *Crtat;
extern int pcscreenmem;

void 
vgasave(vgap)
	struct vga_softc *vgap;
{
	int  i;

	/*			 
	 * Get colorlookuptable 
	 */
	outb(VGA_COLMASK, 0xFF);
	outb(VGA_RCOLMODE, 0x00);
	for (i = 0; i < VGA_NCLUTENT; i++) 
		vgap->clut[i] = inb(VGA_CLUT); 

	/*
	 * get current screen text
	 */
	vgap->vga_screen = malloc(pcscreenmem, M_DEVBUF, M_WAITOK);
	bcopy(Crtat, vgap->vga_screen, pcscreenmem);

	vgap->mode = VGA_GRAPHICS;
}



void
vgarestore(vgap)
	struct vga_softc *vgap;
{
	int  i;

	if (vgap->mode != VGA_GRAPHICS)
		return;

	/* 
	 * Restore screen text
	 */
	bcopy(vgap->vga_screen, Crtat, pcscreenmem);
	free(vgap->vga_screen, M_DEVBUF);
	vgap->vga_screen = NULL;

	/* 
	 * Restore color lookup table
	 */
	outb(VGA_COLMASK, 0xFF);
	outb(VGA_WCOLMODE, 0x00);
	for (i = 0; i < VGA_NCLUTENT; i++) 
		outb(VGA_CLUT, vgap->clut[i]);

	vgap->mode = VGA_TEXT;

	return;
}
