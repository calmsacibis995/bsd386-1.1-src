/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 *
 *	BSDI $Id: if_c_hdlcvar.h,v 1.1 1993/02/21 00:04:31 karels Exp $
 */

/*
 * Software state of cisco SLARP protocol
 */
struct slarp_sc {
	long    sls_myseq;      /* own sequence number (host byteorder) */
	long    sls_rmtseq;     /* peer's sequence number (net byteorder) */
	long    sls_loopctr;    /* counter of suspicious keepalive packets */
};
