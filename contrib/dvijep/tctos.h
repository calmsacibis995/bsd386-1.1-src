/* -*-C-*- tctos.h */
/*-->tctos*/
/**********************************************************************/
/****************************  tctos  *********************************/
/**********************************************************************/

char *
tctos()	/* return pointer to (static) TeX page counter string */
{	/* (trailing zero counters are not printed) */
    register INT16 k;	/* loop index */
    register INT16 n;	/* number of counters to print */
    static char s[111];	/* 10 32-bit counters n.n.n... */

    for (n = 9; (n > 0) && (tex_counter[n] == 0); --n)
	/* NO-OP */;
    s[0] = '\0';
    for (k = 0; k <= n; ++k)
	(void)sprintf(strrchr(s,'\0'),"%ld%s",
	    tex_counter[k],(k < n) ? "." : "");
    return ((char *)&s[0]);
}
