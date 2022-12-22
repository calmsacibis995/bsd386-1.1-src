/*
**  Return current working directory.  Something for everyone.
*/
/* LINTLIBRARY */
#include "shar.h"
#ifdef	RCSID
static char RCS[] =
	"$Header: /bsdi/MASTER/BSDI_OS/contrib/cshar/cshar/lcwd.c,v 1.1.1.1 1993/02/23 18:00:00 polk Exp $";
#endif	/* RCSID */


#ifdef	PWDGETENV
/* ARGSUSED */
char *
Cwd(p, i)
    char	*p;
    int		 i;
{
    char	*q;

    return((q = getenv(PWDGETENV)) ? strcpy(p, q) : NULL);
}
#endif	/* PWDGETENV */


#ifdef	GETWD
/* ARGSUSED1 */
char *
Cwd(p, size)
    char	*p;
    int		 size;
{
    extern char	*getwd();

    return(getwd(p) ? p : NULL);
}
#endif	/* GETWD */


#ifdef	GETCWD
char *
Cwd(p, size)
    char	*p;
    int		 size;
{
    extern char	*getcwd();

    return(getcwd(p, size) ? p : NULL);
}
#endif	/* GETCWD */


#ifdef	PWDPOPEN
char *
Cwd(p, size)
    char	*p;
    int		 size;
{
    extern FILE	*popen();
    FILE	*F;
    int		 i;

    /* This string could be "exec /bin/pwd" if you want... */
    if (F = popen("pwd", "r")) {
	if (fgets(p, size, F) && p[i = strlen(p) - 1] == '\n') {
	    p[i] = '\0';
	    (void)fclose(F);
	    return(p);
	}
	(void)fclose(F);
    }
    return(NULL);
}
#endif	/* PWDPOPEN */
