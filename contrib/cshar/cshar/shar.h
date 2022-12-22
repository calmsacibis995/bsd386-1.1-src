/*
**  Header file for shar and friends.
**  If you have to edit this file, then I messed something up, please
**  let me know what.
**
**  $Header: /bsdi/MASTER/BSDI_OS/contrib/cshar/cshar/shar.h,v 1.1.1.1 1993/02/23 18:00:00 polk Exp $
*/

#include "config.h"

#ifdef	ANSI_HDRS
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <io.h>
#include <time.h>
#else
#include <stdio.h>
#ifdef	VMS
#include <types.h>
#else
#include <sys/types.h>
#endif	/* VMS */
#include <ctype.h>
#include <setjmp.h>
#endif	/* ANSI_HDRS */

#ifdef	IN_SYS_DIR
#include <sys/dir.h>
#endif	/* IN_SYS_DIR */
#ifdef	IN_DIR
#include <dir.h>
#endif	/* IN_DIR */
#ifdef	IN_DIRECT
#include <direct.h>
#endif	/* IN_DIRECT */
#ifdef	IN_SYS_NDIR
#include <sys/ndir.h>
#endif	/* IN_SYS_NDIR */
#ifdef	IN_NDIR
#include "ndir.h"
#endif	/* IN_NDIR */
#ifdef	IN_DIRENT
#include <dirent.h>
#endif	/* IN_DIRENT */


/*
**  Handy shorthands.
*/
#define TRUE		1
#define FALSE		0
#define WIDTH		72
#define F_DIR		36		/* Something is a directory	*/
#define F_FILE		65		/* Something is a regular file	*/
#define S_IGNORE	76		/* Ignore this signal		*/
#define S_RESET		90		/* Reset signal to default	*/

/* These are used by the archive parser. */
#define LINE_SIZE	200		/* Length of physical input line*/
#define MAX_VARS	 20		/* Number of shell vars allowed	*/
#define MAX_WORDS	 30		/* Make words in command lnes	*/
#define VAR_NAME_SIZE	 30		/* Length of a variable's name	*/
#define VAR_VALUE_SIZE	128		/* Length of a variable's value	*/


/*
**  Lint placation.
*/
#ifdef	lint
#undef RCSID
#undef putchar
#endif	/* lint */
#define Fprintf		(void)fprintf
#define Printf		(void)printf


/*
**  Memory hacking.
*/
#define NEW(T, count)	((T *)getmem(sizeof (T), (unsigned int)(count)))
#define ALLOC(n)	getmem(1, (unsigned int)(n))
#define COPY(s)		strcpy(NEW(char, strlen((s)) + 1), (s))


/*
**  Macros.
*/
#define BADCHAR(c)	(!isascii((c)) || (iscntrl((c)) && !isspace((c))))
#define HDRCHAR(c)	((c) == '-' || (c) == '_' || (c) == '.')
#define EQ(a, b)	(strcmp((a), (b)) == 0)
#define EQn(a, b, n)	(strncmp((a), (b), (n)) == 0)
#define PREFIX(a, b)	(EQn((a), (b), sizeof b - 1))
#define WHITE(c)	((c) == ' ' || (c) == '\t')


/*
**  Linked in later.
*/
extern int	 errno;
extern int	 optind;
extern char	*optarg;

/* From your C run-time library. */
extern FILE	*popen();
extern time_t	 time();
extern long	 atol();
extern char	*IDX();
extern char	*RDX();
extern char	*ctime();
extern char	*gets();
extern char	*mktemp();
extern char	*strcat();
extern char	*strcpy();
extern char	*strncpy();
extern char   	*getenv();
#ifdef	PTR_SPRINTF
extern char	*sprintf();
#endif	/* PTR_SPRINTF */

/* From our local library. */
extern align_t	 getmem();
extern off_t	 Fsize();
extern char	*Copy();
extern char	*Cwd();
extern char	*Ermsg();
extern char	*Host();
extern char	*User();

/* Exported by the archive parser. */
extern jmp_buf	 jEnv;
extern FILE	*Input;			/* Current input stream		*/
extern char	*File;			/* Input filename		*/
extern int	 Interactive;		/* isatty(fileno(stdin))?	*/
extern void	 SetVar();		/* Shell variable assignment	*/
extern void	 SynErr();		/* Fatal syntax error		*/
