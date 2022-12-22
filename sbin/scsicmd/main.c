/*
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

static const char rcsid[] =
    "$Id: main.c,v 1.4 1993/03/08 07:13:25 polk Exp $";

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/disklabel.h>
#ifdef __i386__
#include <machine/bootblock.h>
#endif
#include <scsi/scsi_ioctl.h>
#include <ufs/fs.h>	/* XXX */
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "scsicmd.h"
#include "bomb.h"

static struct disklabel *compute_disklabel __P((void));
#ifdef __i386__
extern void disklabel_display __P((char *specname, FILE *, struct disklabel *, struct mbpart *));
#else
extern void disklabel_display __P((char *specname, FILE *, struct disklabel *));
#endif
static void print_disklabel __P((struct disklabel *));
static void write_disklabel __P((struct disklabel *));
static void warntape __P((char *));

/*
 * Options:
 *	-P p=v,...	set command parameters
 *	-c command	apply this command (SCSI command by default)
 *	-d		print values in decimal rather than hex
 *	-f dev		use this device to access SCSI
 *	-h host-adapter	apply host adapter command rather than SCSI command
 *	-l		label a disk
 *	-p p,...	execute command and print results
 *	-t target/lun	use this target/lun pair (default is the open device)
 *	-v p,...	verify the parameters but don't execute the command
 *	-x		print values in hex rather than decimal
 * special print parameters: all, none, pages, [buffer name]
 * if -p and -v are missing, -p none is assumed
 * options imply operations that are executed sequentially
 * options may be repeated to repeat an effect
 *
 * Example:
 *	scsicmd -f /dev/rst0 -c inquiry -P evpd,pc=0x80 -p psn
 *
 * The four options do the following:
 *	(1) opens /dev/rst0
 *	(2) starts building an INQUIRY command
 *	(3) sets two INQUIRY parameters
 *	(4) executes the command and prints a returned parameter
 *
 * Remarks:
 *	This code could be a hell of a lot faster, but it won't get used
 *	much and it's easier to read without lots of speed tweaks.
 */

char *scsiname;
int scsi = -1;
char *hostname;
unsigned target = 0;
unsigned lun = 0;
int activity;
int xflag;

ht *current_ht = httab;
cmd *current_cmd;

static void
usage()
{
	static const char usage_format[] =
"usage:\n\
\t%s -f scsi-device -l\n\
\t%s -a asc/ascq\n\
\t%s -f scsi-device -c command\n\
\t\t[-P parm=value,...] [-p parm,...] [-v parm,...] ...\n\
\tuse `-c all' for a list of commands, `-v all' for a list of parameters";

	sbomb(usage_format, program_name, program_name, program_name);
}

int
main(argc, argv)
	int argc;
	char **argv;
{
	char *cp;
	int c;
	unsigned asc, ascq;

	program_name = argv[0];

	if (argc == 1)
		usage();

	/*
	 * XXX
	 * Ideally the default SCSI device should be /dev/scsi...
	 * Also ideally, we should be able to retrieve the target/lun
	 * values for a given device using an ioctl().
	 * Currently we can't set the target value and there are no
	 * host adapter ioctl()s, so -h and -t aren't useful.
	 * The target defaults to the device specified by -f.
	 */

	if (scsiname = getenv("SCSI")) {
		/* XXX skip warntape() */
		if ((scsi = open(scsiname, O_RDWR|O_NONBLOCK)) == -1)
			bomb("can't open %s", scsiname);
	}

	/*
	 * We make a quick pre-scan to verify some of the parameters.
	 * We open the first device here in case the user was lax
	 * about putting -f before the first print command.
	 */
	while ((c = getopt(argc, argv, "LP:a:c:df:h:lp:t:v:x")) != EOF)
		switch (c) {
		case 'L':
			/* write a disk label out */
			sbomb("-L not yet supported");
			break;
		case 'c':
			if (search_command(optarg) == 0)
				sbomb("-c %s: unknown command", optarg);
			break;
		case 'f':
			if (scsi < 0) {
				scsiname = optarg;
				warntape(scsiname);
				if ((scsi = open(scsiname, O_RDWR|O_NONBLOCK))
				    == -1)
					bomb("can't open %s", scsiname);
			} else {
				if (access(optarg, R_OK|W_OK) == -1)
					bomb("can't access %s", optarg);
				warntape(optarg);
			}
			break;
		case 'h':
			/* specify host adapter number */
			sbomb("-h not yet supported");
			break;
		case 't':
			/* specify target/lun */
			sbomb("-t not yet supported");
			break;
		case 'P':
		case 'a':
		case 'd':
		case 'l':
		case 'p':
		case 'v':
		case 'x':
			break;
		default:
			usage();
		}

	/*
	 * Now for the syntax pass.
	 * Parse argument values and execute commands.
	 */
	optind = 1;
	while ((c = getopt(argc, argv, "P:a:c:f:h:lp:t:v:")) != EOF)
		switch (c) {

		case 'L':
			if (scsi < 0)
				bomb("no SCSI device provided");
			write_disklabel(compute_disklabel());
			break;

		case 'P':
			if (current_cmd == 0)
				sbomb("parameter list without command");
			set_parameters(optarg);
			break;

		case 'a':
			if (cp = strchr(optarg, '/')) {
				*cp++ = '\0';
				asc = strtol(optarg, (char **)NULL, 0);
				ascq = strtol(cp, (char **)NULL, 0);
			} else {
				asc = strtol(optarg, (char **)NULL, 0);
				ascq = 0;
			}
			puts(sprintasc(asc, ascq));
			break;

		case 'c':
			start_command(optarg);
			break;

		case 'd':
			xflag = 0;
			break;

		case 'f':
			if (strcmp(scsiname, optarg) == 0)
				break;
			scsiname = optarg;
			if ((scsi = open(scsiname, O_RDWR|O_NONBLOCK)) == -1)
				bomb("can't open %s", scsiname);
			break;

		case 'h':
			/* XXX switch command table */
			break;

		case 'l':
			if (scsi < 0)
				bomb("no SCSI device provided");
			print_disklabel(compute_disklabel());
			break;

		case 'p':
			if (scsi < 0)
				bomb("no SCSI device provided");
			if (current_cmd == 0)
				sbomb("parameter list without command");
			execute_command();
			print_parameters(optarg);
			break;

		case 't':
			if (cp = strchr(optarg, '/')) {
				*cp++ = '\0';
				target = atoi(optarg);
				lun = atoi(cp);
			} else {
				target = atoi(optarg);
				lun = 0;
			}
			if (target > 7)
				sbomb("-t: illegal target %u", target);
			if (lun > 7)
				sbomb("-t: illegal LUN %u", lun);
			break;

		case 'v':
			if (current_cmd == 0)
				sbomb("parameter list without command");
			print_parameters(optarg);
			break;

		case 'x':
			xflag = 1;
			break;

		default:
			/* shouldn't happen */
			usage();
		}

	if (!activity && scsi >= 0 && current_cmd)
		execute_command();

	return 0;
}

/*
 * Warn users about the ugly habits of tape drives.
 * This can probably go away when we create /dev/scsi.
 */
static void
warntape(s)
	char *s;
{
	static int seentape;
	unsigned n;
	char line[256];

	if (strncmp(s, "/dev/rst", 8) == 0)
		n = atoi(&s[8]) & 3;
	else if (strncmp(s, "/dev/nrst", 9) == 0)
		n = atoi(&s[9]) & 3;
	else
		return;

	if (seentape & 1 << n)
		return;
	seentape |= 1 << n;

	fprintf(stderr, "%s is a tape drive\n", s);
	fprintf(stderr, "You should load a scratch tape into the drive.\n");
	fprintf(stderr, "Hit return when ready (or interrupt to abort): ");
	fgets(line, sizeof line, stdin);
}

cmd *
search_command(command)
	char *command;
{
	cmd *cmdp = current_ht->ht_cmd;

	if (strcmp(command, "all") == 0)
		return cmdp;
	for (; cmdp->cmd_name; ++cmdp)
		if (strcmp(cmdp->cmd_name, command) == 0)
			return cmdp;
	return 0;
}

void
start_command(command)
	char *command;
{
	cmd *cmdp;
	pm *pmp;

	if (strcmp(command, "all") == 0) {
		printf("%s commands:\n", current_ht->ht_name);
		for (cmdp = current_ht->ht_cmd; cmdp->cmd_name; ++cmdp)
			printf("\t%s (%s)\n", cmdp->cmd_desc, cmdp->cmd_name);
		++activity;
		return;
	}

	current_cmd = search_command(command);

	if ((current_cmd->cmd_flag & CMD_INIT) == 0) {
		/* XXX any string initializations? */
		for (pmp = current_cmd->cmd_pm; pmp->pm_name; ++pmp)
			if (pmp->pm_value)
				store_pm(pmp);
		current_cmd->cmd_flag |= CMD_INIT;
	}
}

/*
 * Store a value into a buffer (e.g. a SCSI cdb, a MODE SELECT page)
 * before executing a command.
 * Both string parameters (blank padded on the right) and
 * numeric parameters are supported.
 * The peculiar semantics of SCSI bit-field contiguity are obeyed:
 * numbers are stored in big-endian byte order and little-endian bit order,
 * and are assumed to completely fill every byte after the first.
 * Bit offsets apply only to high-order bits in the first byte.
 */
void
store_pm(pmp)
	pm *pmp;
{
	bf *bftab = current_cmd->cmd_bf;
	bf *bfp = &bftab[pmp->pm_bufno];
	unsigned len = pmp->pm_len;
	unsigned m, n;
	u_long u, v;
	u_char *b, *s;

	/* handle aliased buffers */
	while (bfp->bf_buf == 0)
		bfp = &bftab[bfp->bf_overlap];

	b = &bfp->bf_buf[pmp->pm_byte];

	if (s = (u_char *)pmp->pm_string) {

		if (pmp->pm_bit > 0 || len % NBBY > 0) {
		    scomplain("non-byte-aligned string in parameter `%s'",
			    pmp->pm_name);
			++b;
			len -= len % NBBY;
		}

		/* copy and pad with blanks */
		for (; *s && len > 0; len -= NBBY)
			*b++ = *s++;
		for (; len > 0; len -= NBBY)
			*b++ = ' ';
		return;
	}

	/* copy unaligned high bits first, then copy by bytes */
	u = pmp->pm_value;
	if ((n = len % NBBY) > 0) {
		len -= n;
		m = (1 << n) - 1 << pmp->pm_bit;
		v = (u >> len) << pmp->pm_bit & m;
		*b &= ~m;
		*b++ |= v;
	}
	while (len > 0) {
		len -= NBBY;
		*b++ = u >> len;
	}

	/* handle any code translation */
	if (pmp->pm_code)
		(*current_cmd->cmd_code)(pmp);
}

/*
 * Parse a parameter list, extract parameter values and call store_pm()
 * to update the buffers.
 */
void
set_parameters(parm_list)
	char *parm_list;
{
	cmd *cmdp = current_cmd;
	pm *pmp;
	char *p, *v, *np, *e;

	parm_list = strdup(parm_list);
	for (p = parm_list; p; p = np) {
		/*
		 * Prep the parameter name by
		 * stripping initial and trailing space.
		 */
		if (np = strchr(p, ','))
			*np++ = '\0';
		if (v = strchr(p, '='))
			*v++ = '\0';
		while (isspace(*p))
			++p;
		if ((e = p + strlen(p)) == p) {
			scomplain("missing parameter name in parameter list");
			continue;
		}
		while (isspace(*--e))
			;
		*++e = '\0';

		/*
		 * Search for parameter name in parameter table.
		 */
		for (pmp = cmdp->cmd_pm; pmp->pm_name; ++pmp)
			if (strcmp(pmp->pm_name, p) == 0)
				break;
		if (pmp->pm_name == 0)
			continue;

		/*
		 * Parse the value and assign it to the parameter.
		 */
		if (pmp->pm_string)
			if (v && *v)
				pmp->pm_string = strdup(v);
			else
				pmp->pm_string = "";
		else
			if (v) {
				e = v;
				pmp->pm_value = strtoul(v, &e, 0);
				if (e == v) {
    scomplain("unparsable value `%s' for parameter `%s', using 0 instead",
					    v, p);
					pmp->pm_value = 0;
				}
			} else
				/*
				 * Leaving out the '=value' is
				 * a shorthand for setting a bit.
				 */
				pmp->pm_value = 1;
		store_pm(pmp);
	}
	free(parm_list);
}

void
execute_command()
{
	static int on = 1, off;

	/* XXX this SDIOCSFORMAT stuff sucks */

	if (ioctl(scsi, SDIOCSFORMAT, &on) == -1)
		bomb("can't enable SCSI command mode on %s", scsiname);

	(*current_cmd->cmd_ex)();

	if (ioctl(scsi, SDIOCSFORMAT, &off) == -1)
		bomb("can't disable SCSI command mode on %s", scsiname);
}

/*
 * Pull data values out of the buffers after a command executes.
 * Return 1 if the value exists at the given offset, 0 otherwise.
 */
int
extract_pm(pmp)
	pm *pmp;
{
	bf *bftab = current_cmd->cmd_bf;
	bf *bfp = &bftab[pmp->pm_bufno];
	unsigned len = pmp->pm_len;
	unsigned n;
	u_long u;
	u_char *b, *e;

	/* handle aliased buffers */
	while (bfp->bf_len == 0)
		bfp = &bftab[bfp->bf_overlap];

	if (pmp->pm_byte >= bfp->bf_len)
		return 0;
	if ((bfp->bf_len - pmp->pm_byte) * NBBY < len)
		len -= (len / NBBY - (bfp->bf_len - pmp->pm_byte)) * NBBY;
	b = &bfp->bf_buf[pmp->pm_byte];

	if (pmp->pm_string) {
		len /= NBBY;

		/* strip blanks from the end of strings */
		for (e = &b[len - 1]; e >= b && *e == ' '; --e)
			;
		len = e + 1 - b;
		if (len == 0) {
			pmp->pm_string = "";
			return 1;
		}
		pmp->pm_string = malloc(len + 1);
		bcopy(b, pmp->pm_string, len);
		pmp->pm_string[len] = '\0';
		return 1;
	}

	/* copy unaligned high bits first, then copy by bytes */
	u = 0;
	if ((n = len % NBBY) > 0) {
		len -= n;
		u = (*b >> pmp->pm_bit & (1 << n) - 1) << len;
	}
	while (len > 0) {
		len -= NBBY;
		u |= *b++ << len;
	}
	pmp->pm_value = u;

	return 1;
}

void
display_pm(pmp)
	pm *pmp;
{
	bf *bftab = current_cmd->cmd_bf;
	bf *bfp = &bftab[pmp->pm_bufno];
	unsigned len;
	const int width = 72;

	/* handle aliased buffers */
	while (bfp->bf_len == 0)
		bfp = &bftab[bfp->bf_overlap];

	if (pmp->pm_byte >= bfp->bf_len)
		return;
	len = strlen(pmp->pm_desc) + strlen(pmp->pm_name) + 5;

	if (pmp->pm_code)
		(*current_cmd->cmd_code)(pmp);

	printf("%s (%s):", pmp->pm_desc, pmp->pm_name);
	if (pmp->pm_string)
		printf("%*s\n", width - len, pmp->pm_string);
	else if (pmp->pm_code && *pmp->pm_code)
		printf("%*s\n", width - len, pmp->pm_code);
	else if (xflag)
		printf("%#*x\n", width - len, pmp->pm_value);
	else
		printf("%*d\n", width - len, pmp->pm_value);
}

pm *
extract_bf(parm, display)
	char *parm;
	int display;
{
	cmd *cmdp = current_cmd;
	bf *bftab = cmdp->cmd_bf;
	pm *pmp;
	int bufno = -1;
	int found = 0;
	int i;

	/* look the name up as a buffer */
	for (i = 0; bftab[i].bf_name; ++i)
		if (strcmp(bftab[i].bf_name, parm) == 0) {
			bufno = i;
			break;
		}

	/* scan for matching parameters */
	for (pmp = cmdp->cmd_pm; pmp->pm_name; ++pmp) {
		if (bufno >= 0) {
			if (bufno != pmp->pm_bufno)
				continue;
		} else if (strcmp(pmp->pm_name, parm))
			continue;
		if (!extract_pm(pmp))
			break;
		found = 1;
		if (display)
			display_pm(pmp);
		if (bufno < 0)
			break;
	}

	return found ? pmp : 0;
}

void
print_parameters(parm_list)
	char *parm_list;
{
	bf *bfp;
	char *e, *p, *np;

	++activity;

	if (strcmp(parm_list, "none") == 0)
		/* side effects only */
		return;

	if (strcmp(parm_list, "all") == 0) {
		for (bfp = current_cmd->cmd_bf; bfp->bf_name; ++bfp) {
			printf("\n%s (%s):\n\n", bfp->bf_desc, bfp->bf_name);
			extract_bf(bfp->bf_name, 1);
		}
		return;
	}

	if (strcmp(parm_list, "pages") == 0) {
		printf("pages for %s (%s):\n", current_cmd->cmd_desc,
		    current_cmd->cmd_name);
		for (bfp = current_cmd->cmd_bf; bfp->bf_name; ++bfp)
			printf("\t%s (%s)\n", bfp->bf_desc, bfp->bf_name);
		return;
	}

	for (p = parm_list; p; p = np) {

		/*
		 * Prep the parameter name by
		 * stripping initial and trailing space.
		 */
		if (np = strchr(p, ','))
			*np++ = '\0';
		while (isspace(*p))
			++p;
		if ((e = p + strlen(p)) == p) {
			scomplain("missing parameter name in parameter list");
			continue;
		}
		while (isspace(*--e))
			;
		*++e = '\0';

		/*
		 * Print the value.
		 */
		if (extract_bf(p, 1) == 0)
			scomplain("unknown parameter `%s'", p);
	}
}

static char *
prompt(s)
	char *s;
{
	static char line[256];

	fprintf(stderr, "%s: ", s);
	fgets(line, sizeof line, stdin);
	return line;
}

static u_long
getpm(parm)
	char *parm;
{
	pm *pmp = extract_bf(parm, 0);

	if (pmp == 0)
		return -1;
	return pmp->pm_value;
}

static struct disklabel *
compute_disklabel()
{
	pm *pmp;
	char *typename;
	int ansi, rdf, uzone;
	int tpz, aspz, atpz, atplu, spt, i;
	u_long addr, raddr, xaddr;
	u_long ncyls, totalcyls, spc, optimum, bestfit, notches;
	static struct disklabel dl;
	char parmlist[128];
	u_long *zone;
	int bytracks = 0, nt;

	++activity;

	bzero(&dl, sizeof dl);
	dl.d_magic = dl.d_magic2 = DISKMAGIC;
	dl.d_type = DTYPE_SCSI;

	/*
	 * Step one: get the disk type and name.
	 * This information can be obtained from an INQUIRY.
	 */
	start_command("in");
	execute_command();
	if (getpm("pq") != 0)
		sbomb("%s: device not present\n", scsiname);
	if (getpm("pdt") != 0)
		/* XXX needs adjustment after implementing target select */
		sbomb("%s: not a disk\n", scsiname);
	ansi = getpm("ansi");
	rdf = getpm("rdf");
	if (ansi != 1 && ansi != 2)
		sbomb("%s: not SCSI-1 or SCSI-2 conformant (%d)",
		    scsiname, ansi);
	if (ansi == 1 && rdf == 0)
		fprintf(stderr,
		    "(This SCSI-1 drive does not have CCS support;\n"
		    "be prepared for lots of manual intervention.)\n");
	if ((pmp = extract_bf("vi", 0)) == 0 || !isgraph(*pmp->pm_string))
		typename = strdup(prompt("drive vendor and type"));
	else {
		typename = malloc(32);
		strcpy(typename, pmp->pm_string);
		if ((pmp = extract_bf("pi", 0)) && isgraph(*pmp->pm_string))
			strcat(strcat(typename, " "), pmp->pm_string);
		if ((pmp = extract_bf("prl", 0)) && isgraph(*pmp->pm_string))
			strcat(strcat(typename, " "), pmp->pm_string);
	}
	strncpy(dl.d_typename, typename, sizeof dl.d_typename);
	if (getpm("rmb") == 1)
		dl.d_flags |= D_REMOVABLE;

	/*
	 * Step two: get the basic geometry of the disk.
	 * We try to extract as much information from MODE SENSE
	 * as we can; in step three, we try to prove it wrong :-).
	 */

	/* fetch rigid disk drive geometry page */
	start_command("msen");
	set_parameters("pcode=0x04");
	execute_command();
	if (getpm("mpcode") != 0x04)	/* mode sense failed */
		current_cmd->cmd_bf[1].bf_len = 0;
	if ((dl.d_ncylinders = getpm("noc")) == -1)
		dl.d_ncylinders = atol(prompt("number of cylinders"));
	if ((dl.d_ntracks = getpm("noh")) == -1)
		dl.d_ntracks = atol(prompt("number of heads"));
	if ((dl.d_rpm = getpm("mrr")) == (u_short)-1 || dl.d_rpm == 0)
		dl.d_rpm = 3600;	/* XXX */

	/* fetch disk format device page */
	set_parameters("pcode=0x03");
	execute_command();
	if (getpm("mpcode") != 0x03)	/* mode sense failed */
		current_cmd->cmd_bf[1].bf_len = 0;
	tpz = getpm("tpz");
	if ((aspz = getpm("aspz")) < 0)
		aspz = 0;
	if ((atpz = getpm("atpz")) < 0)
		atpz = 0;
	/* XXX we assume that a zone covers one track, cylinder or unit */
	uzone = 0;	/* true if zone covers a unit */
	if (tpz == 1)
		/* zone size is one track */
		dl.d_sparespertrack = aspz;
	else if (tpz == dl.d_ntracks)
		/* zone size is one cylinder */
		/* XXX we assume we never see alternate tracks per cylinder */
		dl.d_sparespercyl = aspz;
	else if (tpz == dl.d_ntracks * dl.d_ncylinders)
		uzone = 1;
	else if (tpz == -1)
		dl.d_sparespercyl = atol(prompt("spare sectors per cylinder"));
	if ((dl.d_nsectors = getpm("spt")) == -1)
		dl.d_nsectors = 0;
	else
		dl.d_nsectors -= dl.d_sparespertrack;
	if ((atplu = getpm("atplu")) < 0)
		atplu = 0;
	dl.d_acylinders = (atplu + (uzone ? atpz : 0)) / dl.d_ntracks;
	if (dl.d_acylinders > 0)
		dl.d_ncylinders -= dl.d_acylinders;

	if ((dl.d_interleave = getpm("il")) == (u_short)-1 ||
	    dl.d_interleave == 0)
		dl.d_interleave = 1;
	if ((dl.d_trackskew = getpm("tsf")) == (u_short)-1)
		dl.d_trackskew = 0;
	if ((dl.d_cylskew = getpm("csf")) == (u_short)-1)
		dl.d_cylskew = 0;

	/* read capacity, get count of usable sectors */
	start_command("rc");
	execute_command();
	dl.d_secperunit = getpm("rlba") + 1;
	dl.d_secsize = getpm("blib");

	/*
	 * Step three: find out how badly we were lied to.
	 * Experience shows that we can't trust
	 * some of the information we were given,
	 * starting with the sector and cylinder counts.
	 * We scan the disk looking for zones and counting
	 * cylinders, then compare with what MODE SENSE said.
	 */

	fprintf(stderr, "Scanning for logical cylinder sizes...\n"); 

	/* One zone per cylinder but add slop for bad cylinder estimates */
	zone = (u_long *) malloc(dl.d_ncylinders * 2 * sizeof zone[0]);
	if (zone == 0)
		bomb("can't allocate space for zone table");

	/* are we getting track sizes or cylinder sizes? */
	set_parameters("pmi");
	execute_command();
	addr = getpm("rlba") + 1;
	if (addr <= 1) {
		if (dl.d_nsectors <= 0)
			dl.d_nsectors =
			    atol(prompt("number of sectors per track"));
		dl.d_secpercyl = dl.d_nsectors * dl.d_ntracks -
		    dl.d_sparespercyl;
		goto trouble;
	}
	if (addr * dl.d_ntracks * dl.d_ncylinders < dl.d_secperunit * 2)
		bytracks = 1;

	addr = 0;
	raddr = 0;
	spt = -1;
	spc = -1;
	ncyls = 0;
	notches = 0;
	do {
		sprintf(parmlist, "pmi,lba=%d", raddr);
		set_parameters(parmlist);
		execute_command();
		raddr = getpm("rlba") + 1;
		if (bytracks) {
			if (raddr == 1) {
				/* stupid Quantum drive firmware bug */
				raddr = getpm("lba") + 1;
				continue;
			}
			/*
			 * Check for a zone change.
			 * Alas, we sometimes see a track broken into pieces
			 * (e.g. global bad sectors on a Quantum drive).
			 * We check d_ntracks tracks and choose the biggest,
			 * which may not be different from what we're using.
			 */
			if (raddr - addr != spt) {
				bestfit = raddr - addr;
				xaddr = raddr;
				for (nt = 1; nt < dl.d_ntracks; ++nt) {
					sprintf(parmlist, "pmi,lba=%d", raddr);
					set_parameters(parmlist);
					execute_command();
					raddr = getpm("rlba") + 1;
					if (raddr == 1) {
						/* Quantum fun & games */
						--nt;
						raddr = getpm("lba") + 1;
						continue;
					}
					if (raddr - xaddr > bestfit)
						bestfit = raddr - xaddr;
					xaddr = raddr;
				}
				spt = bestfit;
			}
			/*
			 * Skip to the end of the cylinder -- it's much faster.
			 * We assume that d_sparespercyl is valid...
			 */
			raddr = addr + spt * dl.d_ntracks - dl.d_sparespercyl;
		}
		if (raddr - addr != spc) {
			if (ncyls > 0) {
				fprintf(stderr,
			    "Zone %d: %d cylinders, %d sectors per cylinder\n",
				    notches, ncyls, spc);
				zone[notches - 1] = spc;
			}
			++notches;
			ncyls = 0;
		}
		spc = raddr - addr;
		addr = raddr;
		++ncyls;
		++totalcyls;
	} while (raddr < dl.d_secperunit);

	fprintf(stderr, "Zone %d: %d cylinders, %d sectors per cylinder\n",
		    notches, ncyls, spc);
	zone[notches - 1] = spc;

	/*
	 * Were we lied to about the number of cylinders?
	 * It's surprising how often this seems to happen.
	 */
	if (totalcyls != dl.d_ncylinders) {
		fprintf(stderr,
		    "Total number of logical cylinders found in scan (%d)\n"
		    "differs from reported drive parameters (%d+%d);\n",
		    totalcyls, dl.d_ncylinders, dl.d_acylinders);
		if (totalcyls < dl.d_ncylinders)
			dl.d_acylinders += dl.d_ncylinders - totalcyls;
		else
			dl.d_acylinders = 0;
		dl.d_ncylinders = totalcyls;
		fprintf(stderr, "assuming %d+%d cylinders for disk label.\n",
		    dl.d_ncylinders, dl.d_acylinders);
	}

	/*
	 * Pick a value for sectors per cylinder that
	 * matches some existing zone and would account
	 * for all the sectors if it was used throughout.
	 * When there's more than one such value,
	 * we use the smallest.
	 */
	bestfit = dl.d_secperunit;
	for (i = 0; i < notches; ++i)
		if (zone[i] * dl.d_ncylinders >= dl.d_secperunit &&
		    zone[i] < bestfit)
			bestfit = zone[i];
	if (bestfit == dl.d_secperunit)
		sbomb("can't find zone with reasonable cylinder size!?");
	dl.d_secpercyl = bestfit;

	/*
	 * Try to guess a reasonable value for sectors
	 * per track and spare sectors per cylinder.
	 * We assume that there are fewer spare sectors
	 * per cylinder than there are heads (otherwise
	 * we'd have spare sectors per track).
	 */
	spt = (dl.d_secpercyl + dl.d_ntracks - 1) / dl.d_ntracks;
	if (notches == 1 && spt != dl.d_nsectors) {
		fprintf(stderr,
		    "Total number of logical sectors per track in scan (%d)\n"
		    "differs from reported drive parameters (%d+%d);\n",
		    spt, dl.d_nsectors, dl.d_sparespertrack);
		dl.d_nsectors = spt;    
		dl.d_sparespertrack = spt - dl.d_nsectors;
		fprintf(stderr,
		    "assuming %d+%d sectors per track for disk label.\n",
		    dl.d_nsectors, dl.d_sparespertrack);    
	} else if (notches > 1) {
		dl.d_nsectors = spt;
		dl.d_flags |= D_ZONE;
		fprintf(stderr,
		    "This is a zone recorded disk with several track sizes;\n"
		    "assuming %d sectors per cylinder and\n"
		    "%d sectors per track for disk label.\n",
		    dl.d_secpercyl, dl.d_nsectors);
	}
	spc = dl.d_nsectors * dl.d_ntracks - dl.d_secpercyl;
	if (spc != dl.d_sparespercyl) {
		fprintf(stderr,
		    "Total number of spare sectors per cylinder in scan (%d)\n"
		    "differs from reported drive parameters (%d);\n",
		    dl.d_sparespercyl, spc);
		dl.d_sparespercyl = spc;
		fprintf(stderr, "assuming %d spares for disk label.\n",
		    dl.d_sparespercyl);
	}

	/*
	 * Step four: try to come up with reasonable default partitions.
	 * We use ~8 MB for the root, ~32 MB for swap and provide an 'h'
	 * partition that runs from the end of swap to the end of the disk.
	 */
trouble:
	spc = dl.d_secpercyl; 
	dl.d_npartitions = 8;

	/* 'a' partition: ~8 MB */
	dl.d_partitions[0].p_offset = 0;
	dl.d_partitions[0].p_size =
	    ((8 * 1024 * 1024 / dl.d_secsize + spc - 1) / spc) * spc;
	dl.d_partitions[0].p_fsize = 1024;
	dl.d_partitions[0].p_fstype = FS_BSDFFS;
	dl.d_partitions[0].p_frag = 8;
	dl.d_partitions[0].p_cpg = 16;    
	dl.d_bbsize = BBSIZE;
	dl.d_sbsize = SBSIZE;

	/* 'b' partition: ~32 MB */
	dl.d_partitions[1].p_offset = dl.d_partitions[0].p_size;
	dl.d_partitions[1].p_size =
	    ((32 * 1024 * 1024 / dl.d_secsize + spc - 1) / spc) * spc;
	dl.d_partitions[1].p_fstype = FS_SWAP;

	/* 'c' partition: whole disk */    
	dl.d_partitions[2].p_offset = 0;
	dl.d_partitions[2].p_size = dl.d_secperunit;
	dl.d_partitions[2].p_fstype = FS_UNUSED;

	/* 'h' partition: leftovers */
	dl.d_partitions[7].p_offset = dl.d_partitions[1].p_offset +
	    dl.d_partitions[1].p_size;
	dl.d_partitions[7].p_size = dl.d_secperunit -
	    dl.d_partitions[7].p_offset;
	dl.d_partitions[7].p_fsize = 1024;
	dl.d_partitions[7].p_fstype = FS_BSDFFS;
	dl.d_partitions[7].p_frag = 8;
	
#if 0
	/*
	 * Step five: compute the checksum.
	 * We call a routine from disklabel(8) to do the work.
	 */
	dl.d_cksum = dkcksum(&dl); 
#endif

	return &dl;
}

static void
write_disklabel(lp)
	struct disklabel *lp;
{
/*
	be certain that you're labelling a disk (!) not a tape or something
	if an old label exists,
		remind the user to save the old label
		prompt them to ask if they really want to do this
*/
	sbomb("write_disklabel not yet implemented");
}

static void
print_disklabel(lp)
	struct disklabel *lp;
{
#ifdef __i386__
	disklabel_display(scsiname, stdout, lp, NULL);
#else
	disklabel_display(scsiname, stdout, lp);
#endif
}
