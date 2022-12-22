/*
**  Stand-alone driver for shell.
*/
#include "shar.h"
#ifdef	RCSID
static char RCS[] =
	"$Header: /bsdi/MASTER/BSDI_OS/contrib/cshar/grot/shell.c,v 1.1.1.1 1993/02/23 18:00:00 polk Exp $";
#endif	/* RCSID */


main(ac, av)
    REGISTER int	 ac;
    REGISTER char	*av[];
{
    char		*vec[MAX_WORDS];
    char		 buff[VAR_VALUE_SIZE];

    if (Interactive = ac == 1) {
	Fprintf(stderr, "Testing shell interpreter...\n");
	Input = stdin;
	File = "stdin";
    }
    else {
	if ((Input = fopen(File = av[1], "r")) == NULL)
	    SynErr("UNREADABLE INPUT");
	/* Build the positional parameters. */
	for (ac = 1; av[ac]; ac++) {
	    (void)sprintf(buff, "%d", ac - 1);
	    SetVar(buff, av[ac]);
	}
    }

    /* Read, parse, and execute. */
    while (GetLine(TRUE))
	if (Argify(vec) && Exec(vec) == -FALSE)
	    break;

    /* That's it. */
    exit(0);
}
