/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
 
/*	BSDI $Id: installsw.c,v 1.8 1994/01/06 04:30:53 polk Exp $	*/

#include <stdio.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <curses.h>
#include <string.h>

#include "installsw.h"

int numpkgs = 0;
struct package pkgs[MAXPACKAGES];

char *progname;
int media, location;
char *rhost;
char *tape;
int tfileno = 0;
char *root;
char pkgdir[MAXPATHLEN];
char pkgfile[MAXPATHLEN];
char *verbose = "-v";
char *ruser=NULL;
int  use_pax = USE_PAX;

char *remote_root;
void determine_name_mapping ();
char *make_remote_cd_filename (struct package *pkg);
char *build_extract_command (struct package *pkg, char *tarfile, char **rpp);
void tape_seek (int n, int pkgfile);
void cleanup_and_exit (int code);
int remote_command (char *cmd, char *host);
int remote_local_command (char *remote, char *local, char *host);
char *hide_meta(char *name);

usage()
{
	fflush(stdout);
	fprintf(stdout, "Usage: %s [-pqt] [-l remote_user]\n", progname);
	exit(1);
}

/*
 * Prompt the user for data using the given prompt string.  Accept
 * the default if one is specified and/or require the response to
 * match one of the specified choices.
 */
char *
query(prompt, deflt, choices)
	char *prompt, *deflt;
	char **choices;
{
	char **p;
	char pbuf[BUFSIZ];
	static char abuf[BUFSIZ];

	if (deflt == NULL)
		(void) sprintf(pbuf, "\n%s: ", prompt);
	else
		(void) sprintf(pbuf, "\n%s [%s]: ", prompt, deflt);
	for (;;) {
		printf("%s", pbuf);
		fflush(stdout);
		if (fgets(abuf, BUFSIZ, stdin) == NULL)
			z(1, "EOF -- exiting");
		abuf[strlen(abuf) - 1] = '\0';
		if (abuf[0] == '\0' && deflt != NULL) {
			(void) strcpy(abuf, deflt);
			break;
		}
		if (choices == NULL)
			break;
		for (p = choices; *p != NULL; p++)
			if (strcmp(abuf, *p) == 0)
				goto done;
		printf("Invalid response: %s\n", abuf);
		printf("Valid responses are: ");
		for (p = choices; *p != NULL; p++)
			printf("%s ", *p);
		printf("\n");
	}
done:
	return abuf;
}

/*
 * Try to execute a simple command on the remote host.
 */
checkhost(host)
	char *host;
{
	char buf[BUFSIZ];

	(void) sprintf(buf, "> %s", _PATH_DEVNULL);
	printf("\nTrying to contact %s...", host);
	fflush(stdout);
	if (remote_local_command (CMD_TEST, buf, host)) {
		printf("Failed!\n");
		return 0;	/* fail */
	}
	else {
		printf("Succeeded.\n");
		return 1;
	}
}

/*
 * Strip whitespace and #-style comments from
 * a line.
 */
char *
strip(cp)
	char *cp;
{
	char *cp2;
	extern char *index();

	while (isspace(*cp))
		cp++;
	if ((cp2 = index(cp, '#')) != NULL)
		*cp2 = '\0';
	return (cp);
}

/*
 * Strip the next word (destructively) from the passed string an
 * return a pointer to it.  Ignore white space.
 */
char *
nxtword(cpp)
	char **cpp;
{
	char *wordp;

	*cpp = strip(*cpp);
	wordp = *cpp;
	while (**cpp && !isspace(**cpp))
		(*cpp)++;
	if (**cpp) {
		**cpp = '\0';
		(*cpp)++;
	}
	return wordp;
}

/*
 * Fill in a list of package structures by reading a
 * manifest file.  This routine may lose track of storage
 * on mal-formed lines, and on incomplete lines like for the
 * COPYRIGHT file.
 */
getpackages(pkgfile, pkgs, numpkgsp)
	char *pkgfile;
	struct package *pkgs;
	int *numpkgsp;
{
	FILE *fp;
	char *cp, *val;
	char buf[BUFSIZ];
	int linenum = 0;
	struct package *pkg;
	extern char *strdup();

	z((fp = fopen(pkgfile, "r")) == NULL,
	    "couldn't open manifest (%s)", pkgfile);
	while ((cp = fgets(buf, BUFSIZ, fp)) != NULL) {
		linenum++;
		if (buf[0] == '\0')
			continue;
		if (*numpkgsp > MAXPACKAGES) {
			fprintf(stderr, "%s: too many packages at line %d\n",
			    progname, linenum);
			break;
		}
		buf[strlen(buf) - 1] = '\0';	/* strip the newline */
		pkg = &pkgs[*numpkgsp];
		val = nxtword(&cp);
		if (val[0] == '\0')	/* blank and/or comment lines */
			continue;
		if (val[0] == F_NOFILE)	/* file number */
			pkg->file = -1;
		else
			pkg->file = atoi(val);

		val = nxtword(&cp);
		if (val[0] == '\0') {
			fprintf(stderr, "%s: mal-formed line %d in %s\n",
			    progname, linenum, pkgfile);
			continue;
		}
		pkg->name = strdup(val);

		val = nxtword(&cp);
		if (val[0] == '\0')
			continue;
		pkg->size = atoi(val);

		val = nxtword(&cp);
		if (val[0] == '\0' || val[1] != '\0') {
			fprintf(stderr, "%s: mal-formed line %d in %s\n",
			    progname, linenum, pkgfile);
			continue;
		}
		pkg->type = *val;

		val = nxtword(&cp);
		if (val[0] == '\0' || val[1] != '\0') {
			fprintf(stderr, "%s: mal-formed line %d in %s\n",
			    progname, linenum, pkgfile);
			continue;
		}
		pkg->pref = *val;

		val = nxtword(&cp);
		if (val[0] == '\0') {
			fprintf(stderr, "%s: mal-formed line %d in %s\n",
			    progname, linenum, pkgfile);
			continue;
		}
		pkg->root = strdup(val);

		val = nxtword(&cp);
		if (val[0] == '\0') {
			fprintf(stderr, "%s: mal-formed line %d in %s\n",
			    progname, linenum, pkgfile);
			continue;
		}
		pkg->sentinel = strdup(val);

		if (cp[0] == '\0') {
			fprintf(stderr, "%s: mal-formed line %d in %s\n",
			    progname, linenum, pkgfile);
			continue;
		}
		pkg->desc = strdup(strip(cp));
		(*numpkgsp)++;
		pkg->selected = 0;
	}
	fclose(fp);
}

/*
 * Figure out which packages appear to be installed by examining
 * the sentinel files.
 */
fixpackages(pkgs, numpkgs, root)
	struct package *pkgs;
	int numpkgs;
	char *root;
{
	int i;
	struct stat st;
	char buf[BUFSIZ];

	for (i = 0; i < numpkgs; i++) {
		(void) sprintf(buf, "%s%s", root, pkgs[i].sentinel);
		pkgs[i].present = stat(buf, &st) >= 0;
	}
}

/*
 * Clear the selected flags for all packages.
 */
clearselected(pkgs, numpkgs)
	struct package *pkgs;
	int numpkgs;
{
	int i;

	for (i = 0; i < numpkgs; i++)
		pkgs[i].selected = 0;
}

isdir(name)
	char *name;
{
	struct stat st;

	if (stat(name, &st) < 0)
		return 0;
	return S_ISDIR(st.st_mode);
}

/*
 * Install the named package.  If we're installing off a CD, just
 * invoke the commands.  If we're running off a tape, we also
 * have to do positioning and we may have to do remote access.
 */
installpkg(pkg, root)
	struct package *pkg;
	char *root;
{
	int  n, startdir;
	char *cmd;
	char *rempart;
	char *pkgname;
	char pkgroot[MAXPATHLEN];
	char pkgtar[MAXPATHLEN];
	char buf[MAXPATHLEN];

	z((startdir = open(".", O_RDONLY)) < 0,
	    "couldn't open current directory: %s",
	    strerror(errno));
	(void) sprintf(pkgroot, "%s/%s", root, pkg->root);
	if (!isdir(pkgroot))
		z(mkdir(pkgroot, 0777) < 0, "couldn't mkdir %s: %s",
		    pkgroot, strerror(errno));
	z(chdir(pkgroot) < 0, "couldn't chdir to pkg root %s: %s",
	    pkgroot, strerror(errno));

	if (media == M_CDROM && location == L_LOCAL) {
		switch(pkg->type) {
		case T_NORMAL:
			(void) sprintf(pkgtar, "%s/%s.tar", pkgdir, pkg->name);
			cmd = build_extract_command(pkg, pkgtar, NULL);
			break;
		case T_COMPRESSED:
			(void) sprintf(pkgtar, "%s/%s.tar.Z", pkgdir, pkg->name);
			cmd = build_extract_command(pkg, pkgtar, NULL);
			break;
		case T_GZIPPED:
			(void) sprintf(pkgtar, "%s/%s.tar.gz", pkgdir, pkg->name);
			cmd = build_extract_command(pkg, pkgtar, NULL);
			break;
		default:
			z(1, "Unknown type for package %s: %d",
			    pkg->name, pkg->type);
			break;
		}
	}
	else if (media == M_CDROM && location == L_REMOTE) {
		pkgname = make_remote_cd_filename (pkg);
		cmd = build_extract_command(pkg, pkgname, &rempart);
	}
	else {
		if (tfileno != pkg->file) {
			if (tfileno > pkg->file)
				tape_seek(0, 0); /* rewind */
			tape_seek(pkg->file-tfileno, pkg->file);
		}

		/* XXX VERIFY THE TAPE LOCATION */

		cmd = build_extract_command(pkg, tape, &rempart);
	}

	printf("\nInstalling %s:\n", pkg->desc);
	if (location == L_REMOTE) {
		n = remote_local_command(rempart, cmd, rhost);
		sprintf(buf, "%s if=%s 2>%s", _PATH_DD, STATUS, _PATH_DEVNULL);
		system(buf);
		z(n, "failed extracting package %s", pkg->desc);
	}
	else
		z(system(cmd), "failed extracting package %s", pkg->desc);

	if (media == M_TAPE) {
		tfileno++;
		sleep(TAPESLEEP);
	}

	/* XXX: If /var was installed someplace other than /var, attempt to
	   make a symbolic link, but don't worry if we fail. */
	if (strcmp(pkg->name, PKG_VAR) == 0 &&
	    strcmp(pkg->root, DEFROOT_VAR) != 0) {
		(void) sprintf(cmd, "%s%s", root, DEFROOT_VAR);
		/* make it a relative link */
		if (pkg->root[0] == '/')
			(void) symlink(pkg->root+1, cmd);
		else
			(void) symlink(pkg->root, cmd);
	}

	z(fchdir(startdir) < 0,
	    "couldn't fchdir to original working directory: %s",
	    strerror(errno));
	close(startdir);
}

/*
 * Process the package list and decide whether or not
 * we need to do anything.  If so, call installpkg() to
 * install the individual packages.
 */
doinstall(pkgs, numpkgs, root)
	struct package *pkgs;
	int numpkgs;
	char *root;
{
	int i, numselected = 0;
	char *ans, *strdup();
	char buf[MAXPATHLEN];

	for (i = 0; i < numpkgs; i++) {
		if (pkgs[i].selected) {
			if (numselected == 0)
				printf("\nThe following packages will be installed:\n\n");
			numselected++;
			printf("\t%s\n", pkgs[i].desc);
		}
	}
	if (numselected == 0)
		cleanup_and_exit(0);
	ans = query("Install these packages?", "yes", NULL);
	if (ans[0] == 'y' || ans[0] == 'Y') {
		for (i = 0; i < numpkgs; i++)
			if (pkgs[i].selected && strcmp(pkgs[i].name, PKG_VAR) == 0) {
    printf("\nThe `/var' directory is used to contain miscellaneous\n");
    printf("accounting information, mail spool files, error logs, etc.  By\n");
    printf("default, it is loaded as part of the root partition.  Some\n");
    printf("sites like to put the `/var' data on its own partition or to\n");
    printf("move it to a larger filesystem like `/usr' (e.g., /usr/var)\n");
    printf("and make `/var' a symbolic link to the place where the data\n");
    printf("actually resides.\n\n");
    printf("If you want to locate `/var' somewhere other than on the root\n");
    printf("filesystem, specify the name of the directory where you want it\n");
    printf("installed (e.g., /usr/var to install it on /usr/var).  If you\n"); 
    printf("aren't sure, just accept the default -- you can always move it\n");
    printf("in the future if you decide you need to.\n");

				ans = query("Where would you like to install /var?", DEFROOT_VAR, NULL);
				if (strncmp(root, ans, strlen(root)) == 0)
					if (strcmp(root, "/") != 0)
						ans = &ans[strlen(root) + 1];
				if (ans[0] == '\0')
					ans = "/";
				pkgs[i].root = strdup(ans);
			}
		for (i = 0; i < numpkgs; i++)
			if (pkgs[i].selected)
				installpkg(&pkgs[i], root);
		clearselected(pkgs, numpkgs);
		printf("\nInstallation of selected packages complete.\n");
#ifdef NOTDEF
/* For some reason (apparently related to running the pax or tar commands)
   at this point, stdin is hosed such that fgets requires three <RETURN>'s
   to return when this question is asked.  It appears to only happen
   when booted off the floppy.  
*/
		ans = query("Do you wish to install more packages?",
		    		"no", NULL);
		if (ans[0] != 'y' && ans[0] != 'Y')
			cleanup_and_exit(0);
#else
		cleanup_and_exit(0);
#endif
	}
}

/*
 * Install packages from a distribution tape, floppy or cd-rom.  The 
 * user is prompted for necessary information.  There is a curses
 * based user-friendly interface to select packages, and packages
 * are installed once the entire selection process is complete so that
 * no further user intervention is required.
 * Both local and remote devices are supported.
 */
main(argc, argv)
	int argc;
	char *argv[];
{
	char *ans;
	char locpart[BUFSIZ];
	char rempart[BUFSIZ];
	char *media_answers[] = {"cdrom", "tape", "floppy", NULL};
	char *location_answers[] = {"local", "remote", NULL};
	char *p;
	int  ch;
	extern char *strdup();
	extern char *optarg;
 	extern int optind;

/* init: */
	progname = argv[0];	/* set up command name correctly */
	for (p = argv[0]; *p; p++)
		if (*p == '/')
			progname = p + 1;
	setup();
	cleanup();
	z(LINES < 24 || COLS < 80,
	    "You need a window at least 24x80; yours is %dx%d",
	    LINES, COLS);
	while ((ch = getopt(argc, argv, "l:pqt")) != EOF)
		switch (ch) {
		case 'l':
			ruser=optarg;
			break;
		case 'p':
			use_pax = 1;
			break;
		case 'q':
			verbose="";
			break;
		case 't':
			use_pax = 0;
			break;
		case '?':
		default:
			usage();
		}
	argc-=optind;
	argv+=optind;
/* END OF INIT */

	printf("You must have the filesystems you will be installing\n");
	printf("mounted before running installsw.  If you are installing\n");
	printf("from CDROM, you should also have the CDROM mounted.  If\n");
	printf("you are installing from a remote system, the network must\n");
	printf("already be configured, and you should have remote access\n");
	printf("permission from the machine that hosts the installation media.\n");
	ans = query("Do you wish to continue with installsw?", "yes", NULL);
	if (ans[0] != 'y' && ans[0] != 'Y')
		exit(0);
	ans = query("Are you installing from tape, floppy, or CD-ROM?",
	    DEFAULT_MEDIA, media_answers);
	if (strcmp(ans, "cdrom") == 0 || strcmp(ans, "floppy") == 0)
		media = M_CDROM;
	else
		media = M_TAPE;

	ans = query("Is the distribution volume local or remote?",
		    ruser == NULL ? "local" : "remote", location_answers);
	if (strcmp(ans, "local") == 0)
		location = L_LOCAL;
	else {
		location = L_REMOTE;
		for (;;) {
			ans = query("Hostname of remote host?",
				    NULL, NULL);
			if (checkhost(ans))
				break;
		}
		rhost = strdup(ans);
	}

	if (media == M_CDROM) {
		ans = query("What directory is it mounted on?",
			    DEF_MOUNTPT, NULL);
		if (location == L_LOCAL) {
			(void) sprintf(pkgdir, "%s/%s", ans, PACKAGEDIR);
			(void) sprintf(pkgfile, "%s/%s", pkgdir, PACKAGEFILE);
		} else {
			remote_root = strdup (ans);
			determine_name_mapping ();

			(void) strcpy(pkgfile, TMPFILE);
			z(get_remote_file (PACKAGEFILE, pkgfile, 1),
			  	"can't find PACKAGES file");
		}
	}

	if (media == M_TAPE) {
		for (;;) {
			ans = query("Name of the no-rewind tape device (e.g., /dev/nrst0 or /dev/nrwt0)?",
				    NULL, NULL);
			ans = strip(ans);
			if (ans[0] != '\0')
				break;
			printf("Invalid entry -- please specify a device name\n");
		}
		tape = strdup(ans);
		(void) strcpy(pkgfile, TMPFILE);
		tape_seek(0,0);	/* rewind */
		if (location == L_REMOTE)
			p = rempart;
		else
			p = locpart;
		p += sprintf(p, "%s if=%s %s", _PATH_DD, tape, DD_BS);
		if (location == L_REMOTE)
			p = locpart;
		p += sprintf(p, " > %s", pkgfile);
		printf("\nReading manifest...\n");
		fflush(stdout);
		if (location == L_REMOTE)
			z(remote_local_command(rempart, locpart, rhost),
				"Couldn't retrieve manifest");
		else
			z(system(locpart), "Couldn't retrieve manifest");
		printf("Got it!\n");
		tfileno++;
	}
	getpackages(pkgfile, pkgs, &numpkgs);	/* read the manifest */

	ans = query("What is the root of the install tree?",
	    DEF_ROOT, NULL);
	root = strdup(ans);

	for (;;) {
		fixpackages(pkgs, numpkgs, root);
		setup();
		interactive();
		cleanup();
		doinstall(pkgs, numpkgs, root);
	}
}

#define N_DOT 1
#define N_VER 2
#define N_UPPER 4
#define N_RR 8

struct namemap {
	char *desc;
	int bits;
} namemap[] = {
{ "PACKAGES/PACKAGES    PACKAGES/SYS_SOURCE.tar.Z",  N_RR },
{ "packages/packages    packages/sys_sour.z",        0 },
{ "packages/packages.   packages/sys_sour.z",	     N_DOT },
{ "packages/packages;1  packages/sys_sour.z;1",      N_VER },
{ "packages/packages.;1 packages/sys_sour.z;1",      N_DOT | N_VER },
{ "PACKAGES/PACKAGES    PACKAGES/SYS_SOUR.Z",        N_UPPER },
{ "PACKAGES/PACKAGES.   PACKAGES/SYS_SOUR.Z",        N_UPPER | N_DOT },
{ "PACKAGES/PACKAGES;1  PACKAGES/SYS_SOUR.Z;1",      N_UPPER | N_VER },
{ "PACKAGES/PACKAGES.;1 PACKAGES/SYS_SOUR.Z;1",      N_UPPER | N_DOT | N_VER },
{ NULL },
};

int name_mapping_bits;
FILE *mapf;
int debug_remote;

void
determine_name_mapping ()
{
	int i;
	char buf[1000];
	int choice;

	printf ("\n");
	printf ("There are many conventions for how CD-ROM file names\n");
	printf ("are presented to programs.  You must determine the\n");
	printf ("appropriate file names for your particular remote host.\n");
 again:
	printf ("\n");
	printf ("Pick the entry from the following table that describes\n");
	printf ("how the two example files should be named.  The first\n");
	printf ("choice is for regular UFS or Rock Ridge file systems.\n");
	printf ("The rest are variations on ISO-9660.\n");
	printf ("\n");
	
	for (i = 0; namemap[i].desc; i++)
		printf ("%d %s\n", i+1, namemap[i].desc);

	printf ("\n");
	printf ("Enter choice: ");
	fflush (stdout);

	fgets (buf, sizeof buf, stdin);
	if (sscanf (buf, "%d", &choice) != 1
	    || --choice < 0 || choice > sizeof namemap / sizeof namemap[0])
		goto again;

	name_mapping_bits = namemap[choice].bits;

	if (name_mapping_bits & N_RR)
		return;

	printf ("Trying to find ISO-9660 file map ... ");
	if (debug_remote)
		printf ("\n");
	fflush (stdout);

	unlink (TMPMAP);
	if ((get_remote_file (".MAP;1", TMPMAP, debug_remote) != 0
	     && get_remote_file ("FILEMAP.;1", TMPMAP, debug_remote) != 0)
	    || (mapf = fopen (TMPMAP, "r")) == NULL) {
		if (debug_remote == 0) {
			debug_remote = 1;
			printf("not found.\n");
			printf("You probably need to choose different file\n");
			printf("name mapping convention.  This time,\n");
			printf("additional debugging information will be\n");
			printf("printed during the attempt.\n");
		}
		goto again;
	}

	printf ("got it\n");
}

char *
map_name (char *raw_name)
{
	char *p;
	static char name[MAXPATHLEN];
	int found;
	char buf[1000];
	char flags[1000];
	char iname[1000];
	char pname[1000];
	int len;

	sprintf (name, "%s/%s", PACKAGEDIR, raw_name);

	if (media != M_CDROM || location != L_REMOTE
	    || (name_mapping_bits & N_RR) != 0) {
		if ((p = strchr (name, ';')) != NULL)
			*p = 0;
		len = strlen (name);
		if (len > 0 && name[len - 1] == '.')
			name[len - 1] = 0;
		return (name);
	}

	if (mapf) {
		fseek (mapf, 0, 0);
		found = 0;
		while (fgets (buf, sizeof buf, mapf) != NULL) {
			if (sscanf (buf, "%s %s %s", flags, iname, pname) != 3)
				continue;
			if (strcmp (pname, raw_name) == 0) {
				found = 1;
				break;
			}
		}

		if (!found) {
			z (1, "can't find translation for %s\n", raw_name);
			return (raw_name);
		}

		sprintf (name, "%s/%s", PACKAGEDIR, iname);
	}
		
	if ((name_mapping_bits & N_UPPER) == 0) {
		for (p = name; *p; p++)
			if (isupper (*p))
				*p = tolower (*p);
	}

	if ((name_mapping_bits & N_VER) == 0) {
		if ((p = strchr (name, ';')) != NULL)
			*p = 0;
	}

	if ((name_mapping_bits & N_DOT) == 0) {
		len = strlen (name);
		if (len > 0 && name[len - 1] == '.')
			name[len - 1] = 0;
	}

	return (name);
}

int
get_remote_file (char *rname, char *lname, int verbose)
{
	char rempart[BUFSIZ];
	char locpart[BUFSIZ];
	char *p;
	struct stat statb;

	(void) sprintf (rempart, "%s %s/%s", _PATH_CAT, remote_root, 
			hide_meta(map_name(rname)));
	p = locpart;
	p += sprintf (p, "> %s ", lname);
	
	if (verbose == 0)
		p += sprintf (p, "2> %s", _PATH_DEVNULL);

	if (verbose)
		printf ("Attempting to locate `%s/%s' on the remote system\n",
				remote_root, map_name (rname));

	if (remote_local_command(rempart, locpart, rhost) && verbose) {
		fprintf(stderr, "%s: failed to retrieve remote file `%s/%s'\n",
				progname, remote_root, map_name (rname));
		return 1;
	}

	z(stat (lname, &statb), "get_remote_file: stat failed");

	if (statb.st_size == 0) {
		/*
		 * Once we've found the map it is worthwhile to
		 * print some information about files not found.
		 */
		if (mapf)
			fprintf (stderr, 
				"%s: failed to retrieve remote file `%s/%s'\n", 
				progname, remote_root, map_name(rname));
		return 1;
	}

	return 0;
}

char *
make_remote_cd_filename (struct package *pkg)
{
	static char name[MAXPATHLEN];
	char buf[MAXPATHLEN];

	sprintf (buf, "%s.tar", pkg->name);
	if (pkg->type == T_COMPRESSED)
		strcat (buf, ".Z");
	if (pkg->type == T_GZIPPED)
		strcat (buf, ".gz");

	(void) sprintf (name, "%s/%s", remote_root, map_name (buf));

	return hide_meta(name);
}

char *
build_extract_command(struct package *pkg, char *tarfile, char **rpp)
{
	static char locpart[MAXPATHLEN];
	static char rempart[MAXPATHLEN];
	char *p;


	if (location == L_REMOTE)
		p = rempart;
	else {
		p = locpart;
		rempart[0] = '\0';
	}

	p += sprintf (p, "%s %s if=%s ", _PATH_DD, DD_BS, tarfile);

	if (location == L_REMOTE) {
		p = locpart;
		p += sprintf (p, "2>%s ", STATUS);
	}

	if (pkg->type == T_COMPRESSED)
		p += sprintf (p, "| %s", _PATH_UNCOMPRESS);
	if (pkg->type == T_GZIPPED)
		p += sprintf (p, "| %s", _PATH_UNZIP);

	if (use_pax)
		p += sprintf (p, "| %s %s %s", _PATH_PAX, PAX_ARGS, verbose);
	else
		p += sprintf (p, "| %s %s %s", _PATH_TAR, TAR_ARGS, verbose);

#ifdef DEBUG
	(void) fprintf(stderr, "\n*** Build command: rem: `%s' loc: %s\n", 
			rempart, locpart);
#endif

	if (rpp)
		*rpp = rempart;
	return locpart;
}

void
tape_seek(int n, int pkgfile)
{
	char cmd[MAXPATHLEN];
	char *p;

	
	p = cmd;

	p += sprintf(p, "%s -f %s ", _PATH_MT, tape);
	if (n == 0) {
		p += sprintf(p, "rew");
		printf("\nRewinding tape...");
	}
	else {
		p += sprintf(p, "fsf %d", n);
		printf("\nForward spacing tape to file %d (fsf %d)...",
			    pkgfile, n);
	}

	fflush(stdout);
	if (location == L_REMOTE)
		z(remote_command(cmd, rhost), 
			"manipulating tape on %s (cmd: %s)", rhost, cmd);
	else
		z(system(cmd), "error manipulating tape (cmd: %s)", cmd);
	sleep(TAPESLEEP);
	printf("Done.\n");

	if (n)
		tfileno = pkgfile;
	else
		tfileno = 0;
}

void
cleanup_and_exit (int code)
{
	(void)unlink(TMPFILE);
	(void)unlink(TMPMAP);
	if (media == M_TAPE && tfileno != 0)
		tape_seek(0,0);	/* rewind */
	exit(0);
}

/*
 * Execute a remote command and return the status of the 
 * remote command (or the rsh if either fails).
 */
int
remote_command (char *cmd, char *host)
{
	FILE *fp;
	char rcmdbuf[MAXPATHLEN];
	char line[BUFSIZ];
	char *p;
	int  status;

	p = rcmdbuf;
	p += sprintf(p, "%s ", _PATH_RSH);
	if (ruser)
		p += sprintf(p, "-l %s ", ruser);
	p += sprintf(p, "%s ", host);

	p += sprintf(p, "\"/bin/sh -c '%s ; echo REMOTE_STATUS: \\$?'\"", cmd);

#ifdef DEBUG
	fprintf(stderr, "\n*** remote_command: rcmdbuf: %s\n", rcmdbuf);
#endif

	z((fp = popen(rcmdbuf, "r")) == NULL, 
			"popen failed (cmd: %s)", rcmdbuf);
	setlinebuf(fp);

	while (fgets(line, BUFSIZ, fp) != NULL)
		if (strncmp(line, "REMOTE_STATUS:", 14) == 0)
			status = atoi(&line[15]);
		else
			printf(line);

	if (pclose(fp) < 0)
		return -1;

	return (status);
}

/* 
 * Execute a command (with status) where part is executed locally
 * and part is executed remotely
 * e.g., remote: `dd if=/dev/foo bs=10k', local: `> /tmp/foo'
 * results in: 
 *	rsh host 'dd if=/dev/foo bs=10k; echo $? > /tmp/rstatus' > /tmp/foo
 *	rsh host cat /tmp/rstatus
 *	return rstatus
 */
int
remote_local_command (char *remote, char *local, char *host)
{
	FILE *fp;
	char rcmdbuf[MAXPATHLEN];
	char line[BUFSIZ];
	char *p;
	char *savep;
	int  status;
	int  pid;

	pid = getpid();

	p = rcmdbuf;
	p += sprintf(p, "%s ", _PATH_RSH);
	if (ruser)
		p += sprintf(p, "-l %s ", ruser);
	p += sprintf(p, "%s ", host);
	savep = p;

	p += sprintf(p, "\"/bin/sh -c '%s ; echo \\$? > %s.%d'\" %s",
			remote, RSTATUS, pid, local);
#ifdef DEBUG
	fprintf(stderr, "\n*** remote_local_command: rcmdbuf: %s\n", rcmdbuf);
#endif

	if (system(rcmdbuf))
		return -1;

	p = savep;
	p += sprintf(p, "%s %s.%d ; %s -f %s.%d", _PATH_CAT, RSTATUS, pid,
			_PATH_RM, RSTATUS, pid);

#ifdef DEBUG
	fprintf(stderr, "\n*** remote_local_command: get_status: %s\n", rcmdbuf);
#endif

	z((fp = popen(rcmdbuf, "r")) == NULL, 
			"popen failed (cmd: %s)", rcmdbuf);
	setlinebuf(fp);
	if (fgets(line, BUFSIZ, fp) != NULL)
		status = atoi(line);
	else
		status = -1;

	if (pclose(fp) < 0)
		return -1;

	return (status);
}

/*
 * Return a string with the shell meta-characters escaped with
 * backslash.  Currently only does `;' but it's easy to extend.
 */
char *
hide_meta(char *name)
{
	char *p;
	char *q;
	static char buf[MAXPATHLEN];

	for (p = name, q = buf; *q = *p; p++, q++) {
		if (*p == ';') {
			*q++ = '\\';
			*q = *p;
			continue;
		}
	}
	return buf;
}
