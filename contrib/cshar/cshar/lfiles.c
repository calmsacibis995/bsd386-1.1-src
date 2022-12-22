/*
**  File related routines.
*/
/* LINTLIBRARY */
#include "shar.h"
#include <sys/stat.h>
#ifdef	RCSID
static char RCS[] =
	"$Header: /bsdi/MASTER/BSDI_OS/contrib/cshar/cshar/lfiles.c,v 1.1.1.1 1993/02/23 18:00:01 polk Exp $";
#endif	/* RCSID */


/* Mask of executable bits. */
#define	EXE_MASK	(S_IEXEC | (S_IEXEC >> 3) | (S_IEXEC >> 6))

/* Stat buffer for last file. */
static struct stat	 Sb;


/*
**  Stat the file if it's not the one we did last time.
*/
int
GetStat(p)
    char		*p;
{
    static char		 Name[BUFSIZ];

    if (*p == Name[0] && EQ(p, Name))
	return(TRUE);
    return(stat(strcpy(Name, p), &Sb) < 0 ? FALSE : TRUE);
}


/*
**  Return the file type -- directory or regular file.
*/
int
Ftype(p)
    char	*p;
{
    return(GetStat(p) && ((Sb.st_mode & S_IFMT) == S_IFDIR) ? F_DIR : F_FILE);
}


/*
**  Return the file size.
*/
off_t
Fsize(p)
    char	*p;
{
    return(GetStat(p) ? Sb.st_size : 0);
}


/*
**  Is a file executable?
*/
int
Fexecute(p)
    char	*p;
{
    return(GetStat(p) && (Sb.st_mode & EXE_MASK) ? TRUE : FALSE);
}
