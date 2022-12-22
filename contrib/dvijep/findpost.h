/* -*-C-*- findpost.h */
/*-->findpost*/
/**********************************************************************/
/****************************** findpost ******************************/
/**********************************************************************/

void
findpost()

{
    register long	postambleptr;
    register BYTE	i;
    register UNSIGN16 the_char;		/* loop index */

    (void) FSEEK (dvifp, 0L, 2);	/* goto end of file */

    /* VAX VMS binary files are stored with NUL padding to the next
    512 byte multiple.  We therefore search backwards to the last
    non-NULL byte to find the real end-of-file, then move back from
    that.  Even if we are not on a VAX VMS system, the DVI file might
    have passed through one on its way to the current host, so we
    ignore trailing NULs on all machines. */

    while (FSEEK(dvifp,-1L,1) == 0)
    {
        the_char = (UNSIGN16)fgetc(dvifp);
	if (the_char)
	  break;		/* exit leaving pointer PAST last non-NUL */
	UNGETC((char)the_char,dvifp);
    }

    postambleptr = FTELL(dvifp) - 4;

#if    OS_VAXVMS
    /* There is a problem with FSEEK() that I cannot fix just now.  A
    request to position to end-of-file cannot be made to work correctly,
    so FSEEK() positions in front of the last byte, instead of past it.
    We therefore modify postambleptr accordingly to account for this. */

    postambleptr++;
#endif

    (void) FSEEK (dvifp, postambleptr, 0);
    while (TRUE)
    {
	(void) FSEEK (dvifp, --(postambleptr), 0);
	if (((i = (BYTE)nosignex(dvifp,(BYTE)1)) != 223) &&
	    (i != DVIFORMAT))
	    (void)fatal("findpost():  Bad end of DVI file");
	if (i == DVIFORMAT)
	    break;
    }
    (void) FSEEK (dvifp, postambleptr - 4, 0);
    (void) FSEEK (dvifp, (long)nosignex(dvifp,(BYTE)4), 0);
}


