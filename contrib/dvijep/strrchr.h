/* -*-C-*- strrchr.h */
/*-->strrchr*/
/**********************************************************************/
/****************************** strrchr *******************************/
/**********************************************************************/

#if    (KCC_20 | IBM_PC_MICROSOFT | OS_VAXVMS)
#else
char*
strrchr(s,c)	/* return address of last occurrence of c in s[], */
		/* or (char*)NULL if not found.  c may be NUL; */
		/* terminator of s[] is included in search. */
register char *s;
register char c;
{
    register char *t;

    t = (char *)NULL;
    for (;;)		/* loop forever */
    {
	if (*s == c)
	    t = s;	/* remember last match position */
	if (!*s)
	    break;	/* exit when NULL reached */
	++s;		/* advance to next character */
    }
    return (t);
}
#endif
