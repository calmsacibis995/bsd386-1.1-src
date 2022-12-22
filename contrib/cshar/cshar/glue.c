/*
**  Subroutine to call the shell archive parser.  This is "glue"
**  between unshar and the parser proper.
*/
#include "shar.h"
#ifdef	RCSID
static char RCS[] =
	"$Header: /bsdi/MASTER/BSDI_OS/contrib/cshar/cshar/glue.c,v 1.1.1.1 1993/02/23 18:00:00 polk Exp $";
#endif	/* RCSID */


#ifdef	USE_MY_SHELL
/*
**  Cleanup routine after BinSh is done
*/
void
BSclean()
{
    (void)fclose(Input);
    (void)unlink(File);
}


/*
**  Copy the input to a temporary file, then call the shell parser.
*/
BinSh(Name, Stream, Pushback)
    char		*Name;
    REGISTER FILE	*Stream;
    char		*Pushback;
{
    REGISTER FILE	*F;
    char		 buff[BUFSIZ];
    char		*vec[MAX_WORDS];

    Interactive = Name == NULL;
#ifdef	MSDOS
    File = "shell.XXX";
    onexit(BSclean);
#else
    File = mktemp("/tmp/shellXXXXXX");
#endif	/* MSDOS */

    F = fopen(File, "w");
    (void)fputs(Pushback, F);
    while (fgets(buff, sizeof buff, Stream))
	(void)fputs(buff, F);
    (void)fclose(Stream);

    if ((Input = fopen(TEMP, "r")) == NULL)
	Fprintf(stderr, "Can't open %s, %s!?\n", TEMP, Ermsg(errno));
    else
	while (GetLine(TRUE)) {
#ifdef	MSDOS
	    if (setjmp(jEnv))
		break;
#endif	/* MSDOS */
	    if (Argify(vec) && Exec(vec) == -FALSE)
		    break;
	}

    BSclean();
}
#endif	/* USE_MY_SHELL */
