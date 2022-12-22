/*-
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: sysinfo.s,v 1.3 1993/01/13 17:01:17 karels Exp $
 */

/*
 * Strings to be copied out with sysinfo.
 * We do this in assembler to force the strings to be consecutive
 * and allow lengths to be calculated by subtracting addresses.
 */
#include "assym.s"
#include "vers.h"

#define MODELLEN	32

	.globl	_sysinfostrings, _machine, _ostype, _osrelease
	.globl	_sccs, _version, _sys_model, _sys_modelbufsize
	.globl	_hostname, _hostnamelen
	.globl	_sysinfostringlen
	
	.data
_sysinfostrings:

_machine:	.asciz	"i386"		/* cpu "architecture" */
_ostype:	.asciz	"BSD/386"	/* system type */
_osrelease:	.asciz	RELEASE		/* system release */
_sccs:		.ascii	"@(#)"		/* prefix to version for "what" */
_version:	.asciz	VERSION

_sys_model:	.space	MODELLEN + 1

_hostname:	.space	MAXHOSTNAMELEN + 1

_hostnamelen: 	.long	0
_sys_modelbufsize:	.long	_hostname-_sys_model

/* length of all of the above strings except hostname */
_sysinfostringlen: .long	_hostname-_sysinfostrings
