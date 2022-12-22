/* -*-C-*- strid2.h */
/*-->strid2*/
/**********************************************************************/
/******************************* strid2 *******************************/
/**********************************************************************/

/* toupper() is supposed to work for all letters, but PCC-20 does it
incorrectly if the argument is not already lowercase; this definition
fixes that. */

#define UC(c) (islower(c) ? toupper(c) : c)

int
strid2(string,substring)/* Return index (0,1,...) of substring in string */
char string[];		/* or -1 if not found.  Letter case is IGNORED. */
char substring[];
{
    register int k;	/* loop index */
    register int limit;	/* loop limit */
    register char *s;
    register char *sub;

    limit = (int)strlen(string) - (int)strlen(substring);

    for (k = 0; k <= limit; ++k)/* simple (and slow) linear search */
    {
	s = &string[k];

	for (sub = &substring[0]; (UC(*s) == UC(*sub)) && (*sub); (++s, ++sub))
	    /* NO-OP */ ;

	if (*sub == '\0')	/* then all characters match */
	    return(k);		/* success -- match at index k */
    }

    return(-1);			/* failure */
}
