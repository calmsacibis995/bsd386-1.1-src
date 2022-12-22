/*
 * Copyright (c) 1993 David I. Bell
 * Permission is granted to use, distribute, or modify this source,
 * provided that this copyright notice remains intact.
 *
 * version - determine the version of calc
 */

#include "calc.h"

#define MAJOR_VER	2	/* major version */
#define MINOR_VER	9	/* minor version */
#define PATCH_LEVEL	0	/* patch level */


void
version(stream)
	FILE *stream;	/* stream to write version on */
{
	fprintf(stream,
		"C-style arbitrary precision calculator (version %d.%d.%d)\n", 
		MAJOR_VER, MINOR_VER, PATCH_LEVEL);
}

/* END CODE */
