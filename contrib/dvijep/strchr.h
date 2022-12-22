/* -*-C-*- strchr.h */
/*-->strchr*/
/**********************************************************************/
/******************************* strchr *******************************/
/**********************************************************************/

#ifndef KCC_20
#define KCC_20 0
#endif

#ifndef IBM_PC_MICROSOFT
#define IBM_PC_MICROSOFT 0
#endif

#ifndef OS_VAXVMS
#define OS_VAXVMS 0
#endif


#if    (KCC_20 | IBM_PC_MICROSOFT | OS_VAXVMS)
#else
char*
strchr(s,c)	/* return address of c in s[], or (char*)NULL if not found. */
register char *s;/* c may be NUL; terminator of s[] is included in search. */
register char c;
{
    while ((*s) && ((*s) != c))
	++s;

    if ((*s) == c)
	return (s);
    else
	return ((char*)NULL);
}
#endif
