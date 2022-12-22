#ifdef THINK_C
#include <Memory.h>
#include <Resources.h>
#include <stdio.h>
#include <stdlib.h>
#include "flexdef.h"

/**
 **		Load a static table from the current resource file
 **/

short int *load_table (int id)
{
	Handle h;
	
	if ((h = GetResource ('YYst', id)) == NULL) {
		fputs ("error loading static table resource", stderr);
		putc ('\n', stderr);
		exit (1);
		}
	MoveHHi (h);
	HLock (h);
	return (short int *) *h;
	}
#endif

