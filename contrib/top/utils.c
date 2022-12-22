/*
 *  Top users/processes display for Unix
 *  Version 3
 *
 *  This program may be freely redistributed,
 *  but this entire comment MUST remain intact.
 *
 *  Copyright (c) 1984, 1989, William LeFebvre, Rice University
 *  Copyright (c) 1989, 1990, 1992, William LeFebvre, Northwestern University
 */

/*
 *  This file contains various handy utilities used by top.
 */

#include "top.h"

int atoiwi(str)

char *str;

{
    register int len;

    len = strlen(str);
    if (len != 0)
    {
	if (strncmp(str, "infinity", len) == 0 ||
	    strncmp(str, "all",      len) == 0 ||
	    strncmp(str, "maximum",  len) == 0)
	{
	    return(Infinity);
	}
	else if (str[0] == '-')
	{
	    return(Invalid);
	}
	else
	{
	    return(atoi(str));
	}
    }
    return(0);
}

/*
 *  itoa - convert integer (decimal) to ascii string for positive numbers
 *  	   only (we don't bother with negative numbers since we know we
 *	   don't use them).
 */

static char buffer[16];		/* shared by the next two routines */
				/*
				 * How do we know that 16 will suffice?
				 * Because the biggest number that we will
				 * ever convert will be 2^32-1, which is 10
				 * digits.
				 */

char *itoa(val)

register int val;

{
    register char *ptr;

    ptr = buffer + sizeof(buffer);
    *--ptr = '\0';
    if (val == 0)
    {
	*--ptr = '0';
    }
    else while (val != 0)
    {
	*--ptr = (val % 10) + '0';
	val /= 10;
    }
    return(ptr);
}

/*
 *  itoa7(val) - like itoa, except the number is right justified in a 7
 *	character field.  This code is a duplication of itoa instead of
 *	a front end to a more general routine for efficiency.
 */

char *itoa7(val)

register int val;

{
    register char *ptr;

    ptr = buffer + sizeof(buffer);
    *--ptr = '\0';
    if (val == 0)
    {
	*--ptr = '0';
    }
    else while (val != 0)
    {
	*--ptr = (val % 10) + '0';
	val /= 10;
    }
    while (ptr > buffer + sizeof(buffer) - 7)
    {
	*--ptr = ' ';
    }
    return(ptr);
}

/*
 *  digits(val) - return number of decimal digits in val.  Only works for
 *	positive numbers.  If val <= 0 then digits(val) == 0.
 */

int digits(val)

int val;

{
    register int cnt = 0;

    while (val > 0)
    {
	cnt++;
	val /= 10;
    }
    return(cnt);
}

/*
 *  strecpy(to, from) - copy string "from" into "to" and return a pointer
 *	to the END of the string "to".
 */

char *strecpy(to, from)

register char *to;
register char *from;

{
    while ((*to++ = *from++) != '\0');
    return(--to);
}

/*
 * argparse(line, cntp) - parse arguments in string "line", separating them
 *	put into an argv-like array, and setting *cntp to the number of
 *	arguments encountered.  This is a simple parser that doesn't understand
 *	squat about quotes.
 */

char **argparse(line, cntp)

char *line;
int *cntp;

{
    register char *from;
    register char *to;
    register int cnt;
    register int ch;
    int length;
    int lastch;
    register char **argv;
    char **argarray;
    char *args;

    /* unfortunately, the only real way to do this is to go thru the
       input string twice. */

    /* step thru the string counting the white space sections */
    from = line;
    lastch = cnt = length = 0;
    while ((ch = *from++) != '\0')
    {
	length++;
	if (ch == ' ' && lastch != ' ')
	{
	    cnt++;
	}
	lastch = ch;
    }

    /* add three to the count:  one for the initial "dummy" argument,
       one for the last argument and one for NULL */
    cnt += 3;

    /* allocate a char * array to hold the pointers */
    argarray = (char **)malloc(cnt * sizeof(char *));

    /* allocate another array to hold the strings themselves */
    args = (char *)malloc(length+2);

    /* initialization for main loop */
    from = line;
    to = args;
    argv = argarray;
    lastch = '\0';

    /* create a dummy argument to keep getopt happy */
    *argv++ = to;
    *to++ = '\0';
    cnt = 2;

    /* now build argv while copying characters */
    *argv++ = to;
    while ((ch = *from++) != '\0')
    {
	if (ch != ' ')
	{
	    if (lastch == ' ')
	    {
		*to++ = '\0';
		*argv++ = to;
		cnt++;
	    }
	    *to++ = ch;
	}
	lastch = ch;
    }

    /* set cntp and return the allocated array */
    *cntp = cnt;
    return(argarray);
}

/*
 *  percentages(cnt, out, new, old, diffs) - calculate percentage change
 *	between array "old" and "new", putting the percentages i "out".
 *	"cnt" is size of each array and "diffs" is used for scratch space.
 *	The array "old" is updated on each call.
 *	The routine assumes modulo arithmetic.  This function is especially
 *	useful on BSD mchines for calculating cpu state percentages.
 */

long percentages(cnt, out, new, old, diffs)

int cnt;
int *out;
register long *new;
register long *old;
long *diffs;

{
    register int i;
    register long change;
    register long total_change;
    register long *dp;
    long half_total;

    /* initialization */
    total_change = 0;
    dp = diffs;

    /* calculate changes for each state and the overall change */
    for (i = 0; i < cnt; i++)
    {
	if ((change = *new - *old) < 0)
	{
	    /* this only happens when the counter wraps */
	    change = (int)
		((unsigned long)*new-(unsigned long)*old);
	}
	total_change += (*dp++ = change);
	*old++ = *new++;
    }

    /* calculate percentages based on overall change, rounding up */
    half_total = total_change / 2l;
    for (i = 0; i < cnt; i++)
    {
	*out++ = (int)((*diffs++ * 1000 + half_total) / total_change);
    }

    /* return the total in case the caller wants to use it */
    return(total_change);
}

/*
 * errmsg(errnum) - return an error message string appropriate to the
 *           error number "errnum".  This is a substitute for the System V
 *           function "strerror" with one important difference:  the string
 *           returned by this function does NOT end in a newline!
 *           N.B.:  there appears to be no reliable way to determine if
 *           "strerror" exists at compile time, so I make do by providing
 *           something of similar functionality.
 */

/* externs referenced by errmsg */

extern char *sys_errlist[];
extern int sys_nerr;

char *errmsg(errnum)

int errnum;

{
    if (errnum > 0 && errnum < sys_nerr)
    {
	return(sys_errlist[errnum]);
    }
    return("No error");
}
