/* -*-C-*- signex.h */
/*-->signex*/
/**********************************************************************/
/******************************* signex *******************************/
/**********************************************************************/

INT32
signex(fp, n)     /* return n byte quantity from file fd */
register FILE *fp;    /* file pointer	 */
register BYTE n;      /* number of bytes */

{
    register BYTE n1; /* number of bytes	  */
    register INT32 number; /* number being constructed */

    number = (INT32)getc(fp);/* get first (high-order) byte */

#if    PCC_20
    if (number > 127) /* sign bit is set, pad top of word with sign bit */
	number |= 0xfffffff00;
		      /* this constant could be written for any */
		      /* 2's-complement machine as -1 & ~0xff, but */
		      /* PCC-20 does not collapse constant expressions */
		      /* at compile time, sigh... */
#endif /* PCC_20 */

#if    (OS_ATARI | OS_UNIX)	/* 4.1BSD C compiler uses logical shift too */
    if (number > 127) /* sign bit is set, pad top of word with sign bit */
	number |= 0xffffff00;
#endif /* OS_UNIX */

    n1 = n--;
    while (n--)
    {
	number <<= 8;
	number |= (INT32)getc(fp);
    }

#if    (OS_ATARI | PCC_20 | OS_UNIX)
#else /* NOT (OS_ATARI | PCC_20 | OS_UNIX) */

    /* NOTE: This code assumes that the right-shift is an arithmetic, rather
    than logical, shift which will propagate the sign bit right.   According
    to Kernighan and Ritchie, this is compiler dependent! */

    number<<=HOST_WORD_SIZE-8*n1;

#if    ARITHRSHIFT
    number>>=HOST_WORD_SIZE-8*n1;  /* sign extend */
#else /* NOT ARITHRSHIFT */
    (void)fatal("signex():  no code implemented for logical right shift");
#endif /* ARITHRSHIFT */

#endif /* (OS_ATARI | PCC_20 | OS_UNIX) */

    return((INT32)number);
}
