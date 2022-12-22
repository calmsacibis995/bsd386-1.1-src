#include <stdio.h>
#include "midifile.h"


extern int midifile (void);

mygetc(void) { return(getchar()); }

main(void)
{
	Mf_getc = mygetc;
	midifile ();
	exit(0);
}
