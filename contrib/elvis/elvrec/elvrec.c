/*	BSDI $Id: elvrec.c,v 1.2 1993/03/23 05:20:22 polk Exp $	*/

/* elvrec.c */

/* This file contains the file recovery program */

/* Author:
 *	Steve Kirkendall
 *	14407 SW Teal Blvd. #C
 *	Beaverton, OR 97005
 *	kirkenda@cs.pdx.edu
 */


#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "config.h"
#include "vi.h"

void recover P_((char *, char *));
void main P_((int, char **));


void recover(basename, outname)
	char	*basename;	/* the name of the file to recover */
	char	*outname;	/* the name of the file to write to */
{
	char	pathname[500];	/* full pathname of the file to recover */
	char	line[600];	/* a line from the /usr/preserve/Index file */
	int	ch;		/* a character from the text being recovered */
	FILE	*from;		/* the /usr/preserve file, or /usr/preserve/Index */
	FILE	*to;		/* the user's text file */
	char	*ptr;
	int	f;
#if OSK
	int		uid;
#endif
#if ANY_UNIX
	struct	stat sbuf;
#endif

	/* convert basename to a full pathname */
	if (basename)
	{
#ifndef CRUNCH
# if MSDOS || TOS
		if (!basename[0] || basename[1] != ':')
# else
		if (basename[0] != SLASH)
# endif
		{
			ptr = getcwd(pathname, sizeof pathname);
			if (ptr != pathname)
			{
				strcpy(pathname, ptr);
			}
			ptr = pathname + strlen(pathname);
			*ptr++ = SLASH;
			strcpy(ptr, basename);
		}
		else
#endif
		{
			strcpy(pathname, basename);
		}
	}

#if OSK
	uid = getuid();
	if(setuid(0))
		exit(_errmsg(errno, "Can't set uid\n"));
#endif
	/* scan the /usr/preserve/Index file, for the *oldest* unrecovered
	 * version of this file.
	 */
	from = fopen(PRSVINDEX, "r");
	while (from && fgets(line, sizeof line, from))
	{
		/* strip off the newline from the end of the string */
		line[strlen(line) - 1] = '\0';

		/* parse the line into a "preserve" name and a "text" name */
		for (ptr = line; *ptr != ' '; ptr++)
		{
		}
		*ptr++ = '\0';

		/* If the "preserve" file is missing, then ignore this line
		 * because it describes a file that has already been recovered.
		 */
#ifdef R_OK
		/* don't read a file that belongs to someone else */
		if (access(line, R_OK) < 0)
#else
		if (access(line, 0) < 0)
#endif
		{
			continue;
		}

		/* are we looking for a specific file? */
		if (basename)
		{
			/* quit if we found it */
			if (!strcmp(ptr, pathname))
			{
				break;
			}
		}
		else
		{
			/* list this file as "available for recovery" */
			puts(ptr);
		}
	}

	/* file not found? */
	if (!basename || !from || feof(from))
	{
		if (from != NULL) fclose(from);
		if (basename)
		{
			fprintf(stderr, "%s: no recovered file has that exact name\n", pathname);
		}
		return;
	}
	if (from != NULL) fclose(from);

	/* copy the recovered text back into the user's file... */

	/* open the /usr/preserve file for reading */
	from = fopen(line, "r");
	if (!from)
	{
		perror(line);
		exit(2);
	}

#if ANY_UNIX
	/* paranoia */
	if (fstat(fileno(from), &sbuf) == -1) {
		fclose(from);
		return;
	}
	if (sbuf.st_uid != getuid()) {
		fprintf(stderr, "%s: not owner\n", line);
		fclose(from);
		return;
	}
	if (access(outname, W_OK) == 0) {
		printf("overwrite %s?  ", outname);
		if (getchar() != 'y') {
			fclose(from);
			return;
		}
	}


#if 0
	/* Be careful about user-id.  We want to be running under the user's
	 * real id when we open/create the user's text file... but we want
	 * to be superuser when we delete the /usr/preserve file.  For UNIX,
	 * we accomplish this by deleting the /usr/preserve file *now*,
	 * when it is open but before we've read it.  Then we revert to the
	 * user's real id.
	 */
	unlink(line);
	setuid(getuid());
#else
	/* XXX use the saved ID feature to swap uids */
	seteuid(sbuf.st_uid);
#endif
#endif
#if OSK
	setuid(uid);
#endif

	/* open the user's file for writing */
	to = fopen(outname, "w");
	if (!to)
	{
		perror(ptr);
		exit(2);
	}

	/* copy the text */
	while ((ch = getc(from)) != EOF)
	{
		putc(ch, to);
	}

#if !ANY_UNIX
#if OSK
	fclose(from);
	setuid(0);
#endif
	/* delete the /usr/preserve file */
	unlink(line);
#if OSK
	setuid(uid);
#endif

#else /* ANY_UNIX */
	fclose(from);
	fclose(to);
	seteuid(0);
	unlink(line);
#endif
}

void
main(argc, argv)
	int	argc;
	char	**argv;
{
	/* check arguments */
	if (argc > 3)
	{
		fprintf(stderr, "usage: %s [preserved_file [recovery_file]]\n", argv[0]);
		exit(2);
	}

	/* recover the requested file, or list recoverable files */
	if (argc == 3)
	{
		/* recover the file, but write it to a different filename */
		recover (argv[1], argv[2]);
	}
	else if (argc == 2)
	{
		/* recover the file */
		recover(argv[1], argv[1]);
	}
	else
	{
		/* list the recoverable files */
		recover((char *)0, (char *)0);
	}

	/* success! */
	exit(0);
}
