/*	BSDI $Id: elvprsv.c,v 1.3 1993/03/23 05:20:20 polk Exp $	*/

/* elvprsv.c */

/* Author:
 *	Steve Kirkendall
 *	14407 SW Teal Blvd. #C
 *	Beaverton, OR 97005
 *	kirkenda@cs.pdx.edu
 */


/* This file contains the portable sources for the "elvprsv" program.
 * "Elvprsv" is run by Elvis when Elvis is about to die.  It is also
 * run when the computer boots up.  It is not intended to be run directly
 * by the user, ever.
 *
 * Basically, this program does the following four things:
 *    - It extracts the text from the temporary file, and places the text in
 *	a file in the /usr/preserve directory.
 *    - It adds a line to the /usr/preserve/Index file, describing the file
 *	that it just preserved.
 *    - It removes the temporary file.
 *    -	It sends mail to the owner of the file, saying that the file was
 *	preserved, and how it can be recovered.
 *
 * The /usr/preserve/Index file is a log file that contains one line for each
 * file that has ever been preserved.  Each line of this file describes one
 * preserved file.  The first word on the line is the name of the file that
 * contains the preserved text.  The second word is the full pathname of the
 * file that was being edited; for anonymous buffers, this is the directory
 * name plus "/foo".
 *
 * If elvprsv's first argument (after the command name) starts with a hyphen,
 * then the characters after the hyphen are used as a description of when
 * the editor went away.  This is optional.
 *
 * The remaining arguments are all the names of temporary files that are
 * to be preserved.  For example, on a UNIX system, the /etc/rc file might
 * invoke it this way:
 *
 *	elvprsv "-the system went down" /tmp/elv_*.*
 *
 * This file contains only the portable parts of the preserve program.
 * It must #include a system-specific file.  The system-specific file is
 * expected to define the following functions:
 *
 *	char *ownername(char *filename)	- returns name of person who owns file
 *
 *	void mail(char *user, char *name, char *when)
 *					- tell user that file was preserved
 */

#include <stdio.h>
#include "config.h"
#include "vi.h"
#include "ctype.h"

void preserve P_((char *, char *));
void main P_((int, char **));

#if AMIGA
BLK tmpblk;
# include "amiwild.c"
# include "amiprsv.c"
#endif

#if OSK
# undef sprintf
#endif

#if ANY_UNIX || OSK
# include "prsvunix.c"
#endif

#if MSDOS || TOS
# include "prsvdos.c"
# define WILDCARD_NO_MAIN
# include "wildcard.c"
#endif


BLK	buf;
BLK	hdr;
BLK	name;
int	rewrite_now;	/* boolean: should we send text directly to orig file? */
unsigned userid;



/* This function preserves a single file, and announces its success/failure
 * via an e-mail message.
 */
void preserve(tname, when)
	char	*tname;		/* name of a temp file to be preserved */
	char	*when;		/* description of when the editor died */
{
	int	infd;		/* fd used for reading from the temp file */
	FILE	*outfp;		/* fp used for writing to the recovery file */
	FILE	*index;		/* fp used for appending to index file */
	char	outname[100];	/* the name of the recovery file */
	char	*user;		/* name of the owner of the temp file */
#if AMIGA
	char	*prsvdir;
#endif
#if ANY_UNIX
	struct	stat sbuf;
#endif
	int	i;

#if ANY_UNIX
	/* beware of booby-traps when we're being run from /etc/rc */
#ifdef S_IFLNK
	if (lstat(tname, &sbuf) == -1)
#else
	if (stat(tname, &sbuf) == -1)
#endif
		return;
	if ((sbuf.st_mode & S_IFMT) != S_IFREG)
		return;
	seteuid(sbuf.st_uid);
#endif

	/* open the temp file */
	infd = open(tname, O_RDONLY|O_BINARY);

#if ANY_UNIX
	/* restore privileges so that we can copy the file */
	/* XXX depends on saved uid feature */
	seteuid(0);
#endif

	if (infd < 0)
	{
		/* if we can't open the file, then we should assume that
		 * the filename contains wildcard characters that weren't
		 * expanded... and also assume that they weren't expanded
		 * because there are no files that need to be preserved.
		 * THEREFORE... we should silently ignore it.
		 * (Or loudly ignore it if the user was using -R)
		 */
		if (rewrite_now)
		{
			perror(tname);
		}
		return;
	}

#if ANY_UNIX
	/* make sure we got the right file */
	if (fstat(infd, &sbuf) == -1 ||
	    (sbuf.st_mode & S_IFMT) != S_IFREG ||
	    (userid && sbuf.st_uid != userid)) {
		close(infd);
		return;
	}
#endif

	/* read the header and name from the file */
	if (read(infd, hdr.c, BLKSIZE) != BLKSIZE
	 || read(infd, name.c, BLKSIZE) != BLKSIZE)
	{
		/* something wrong with the file - sorry */
		fprintf(stderr, "%s: truncated header blocks\n", tname);
		close(infd);
		return;
	}

	/* If the filename block contains an empty string, then Elvis was
	 * only keeping the temp file around because it contained some text
	 * that was needed for a named cut buffer.  The user doesn't care
	 * about that kind of temp file, so we should silently delete it.
	 */
	if (name.c[0] == '\0' && name.c[1] == '\177')
	{
		close(infd);
#if ANY_UNIX
		seteuid(sbuf.st_uid);
#endif
		unlink(tname);
#if ANY_UNIX
		seteuid(0);
#endif
		return;
	}

	if (rewrite_now)
	{
		/* we don't need to open the index file */
		index = (FILE *)0;

		/* make sure we can read every block! */
		for (i = 1; i < MAXBLKS && hdr.n[i]; i++)
		{
			lseek(infd, (long)hdr.n[i] * (long)BLKSIZE, 0);
			if (read(infd, buf.c, BLKSIZE) != BLKSIZE)
			{
				/* messed up header */
				fprintf(stderr, "%s: unrecoverable -- header trashed\n", name.c);
				close(infd);
				return;
			}
		}

#if ANY_UNIX
		seteuid(sbuf.st_uid);
#endif
		/* open the user's file for writing */
		outfp = fopen(name.c, "w");
#if ANY_UNIX
		seteuid(0);
#endif
		if (!outfp)
		{
			perror(name.c);
			close(infd);
			return;
		}
	}
	else
	{
		/* open/create the index file */
		if ((i = open(PRSVINDEX, O_WRONLY|O_APPEND|O_CREAT|O_EXLOCK,
		    0600)) == -1) {
			perror(PRSVINDEX);
			exit(2);
		}
		index = fdopen(i, "a");
		if (!index)
		{
			perror(PRSVINDEX);
			exit(2);
		}

		/* should be at the end of the file already, but MAKE SURE */
		fseek(index, 0L, 2);

		/* create the recovery file in the PRESVDIR directory */
#if AMIGA
		prsvdir = &PRSVDIR[strlen(PRSVDIR) - 1];
		if (*prsvdir == '/' || *prsvdir == ':')
		{
			sprintf(outname, "%sp%ld", PRSVDIR, ftell(index));
		}
		else
#endif
		sprintf(outname, "%s%cp%ld", PRSVDIR, SLASH, ftell(index));
		outfp = fopen(outname, "w");
		if (!outfp)
		{
			perror(outname);
			close(infd);
			fclose(index);
			return;
		}
#if ANY_UNIX
		/* we had to be root to create the file; fix it here */
		fchown(fileno(outfp), sbuf.st_uid, sbuf.st_gid);
#endif
	}

	/* write the text of the file out to the recovery file */
	for (i = 1; i < MAXBLKS && hdr.n[i]; i++)
	{
		lseek(infd, (long)hdr.n[i] * (long)BLKSIZE, 0);
		if (read(infd, buf.c, BLKSIZE) != BLKSIZE)
		{
			/* messed up header */
			fprintf(stderr, "%s: unrecoverable -- header trashed\n", name.c);
			fclose(outfp);
			close(infd);
			if (index)
			{
				fclose(index);
			}
			unlink(outname);
			return;
		}
		fputs(buf.c, outfp);
	}

	/* add a line to the index file */
	if (index)
	{
		fprintf(index, "%s %s\n", outname, name.c);
	}

	/* close everything */
	close(infd);
	fclose(outfp);
	if (index)
	{
		fclose(index);
	}

	/* Are we doing this due to something more frightening than just
	 * a ":preserve" command?
	 */
	if (*when)
	{
#if ANY_UNIX
		seteuid(sbuf.st_uid);
#endif
		/* send a mail message */
		mail(ownername(tname), name.c, when);

		/* remove the temp file -- the editor has died already */
		unlink(tname);
#if ANY_UNIX
		seteuid(0);
#endif
	}
}

void main(argc, argv)
	int	argc;
	char	**argv;
{
	int	i;
	char	*when = "the editor went away";

#if MSDOS || TOS
	/* expand any wildcards in the command line */
	_ct_init("");
	argv = wildexpand(&argc, argv);
#endif

	userid = getuid();
# if ANY_UNIX
	umask(077);
# endif

	/* do we have a "when" argument? */
	i = 1;
	if (argc >= i + 1 && !strcmp(argv[i], "-R"))
	{
		rewrite_now = 1;
		when = "";
		i++;
#if ANY_UNIX
		setuid(geteuid());
#endif
	}
#if OSK
	else
	{
		setuid(0);
	}
#endif
	if (argc >= i + 1 && argv[i][0] == '-')
	{
		when = argv[i] + 1;
		i++;
	}

	/* preserve everything we're supposed to */
	while (i < argc)
	{
		preserve(argv[i], when);
		i++;
	}
}
