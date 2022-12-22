/*
 * Device tables.  See the Configure file for a complete description.
 */

#include <stdio.h>
#include <string.h>
#include "msdos.h"

#ifdef DELL
struct device xdevices[] = {
	{'A', "/dev/rdsk/f0d9dt", 0L, 12, 0, (int (*) ()) 0, 40, 2, 9},
	{'A', "/dev/rdsk/f0q15dt", 0L, 12, 0, (int (*) ()) 0, 80, 2, 15},
	{'A', "/dev/rdsk/f0d8dt", 0L, 12, 0, (int (*) ()) 0, 40, 2, 8},
	{'B', "/dev/rdsk/f13ht", 0L, 12, 0, (int (*) ()) 0, 80, 2, 18},
	{'B', "/dev/rdsk/f13dt", 0L, 12, 0, (int (*) ()) 0, 80, 2, 9},
	{'C', "/dev/rdsk/dos", 0L, 16, 0, (int (*) ()) 0, 0, 0, 0},
	{'\0', (char *) NULL, 0L, 0, 0, (int (*) ()) 0, 0, 0, 0}
};
#endif /* DELL */

#ifdef ISC
struct device xdevices[] = {
	{'A', "/dev/rdsk/f0d9dt", 0L, 12, 0, (int (*) ()) 0, 40, 2, 9},
	{'A', "/dev/rdsk/f0q15dt", 0L, 12, 0, (int (*) ()) 0, 80, 2, 15},
	{'A', "/dev/rdsk/f0d8dt", 0L, 12, 0, (int (*) ()) 0, 40, 2, 8},
	{'B', "/dev/rdsk/f13ht", 0L, 12, 0, (int (*) ()) 0, 80, 2, 18},
	{'B', "/dev/rdsk/f13dt", 0L, 12, 0, (int (*) ()) 0, 80, 2, 9},
	{'C', "/dev/rdsk/0p1", 0L, 16, 0, (int (*) ()) 0, 0, 0, 0},
	{'D', "/usr/vpix/defaults/C:", 8704L, 12, 0, (int (*) ()) 0, 0, 0, 0},
	{'E', "$HOME/vpix/C:", 8704L, 12, 0, (int (*) ()) 0, 0, 0, 0},
	{'\0', (char *) NULL, 0L, 0, 0, (int (*) ()) 0, 0, 0, 0}
};
#endif /* ISC */

#ifdef SPARC
struct device xdevices[] = {
	{'A', "/dev/rfd0c", 0L, 12, 0, (int (*) ()) 0, 80, 2, 18},
	{'A', "/dev/rfd0c", 0L, 12, 0, (int (*) ()) 0, 80, 2, 9},
	{'\0', (char *) NULL, 0L, 0, 0, (int (*) ()) 0, 0, 0, 0}
};
#endif /* SPARC */
#ifdef RT_ACIS
struct device xdevices[] = {
      {'A', "/dev/rfd0", 0L, 12, 0, (int (*) ()) 0, 80, 2, 15},
      {'A', "/dev/rfd0", 0L, 12, 0, (int (*) ()) 0, 40, 2, 9},
      {'\0', (char *) NULL, 0L, 0, 0, (int (*) ()) 0, 0, 0, 0}
};
#endif /* RT_ACIS */


#ifdef UNIXPC
#include <sys/gdioctl.h>
#include <fcntl.h>

int init_unixpc();

struct device xdevices[] = {
	{'A', "/dev/rfp020", 0L, 12, O_NDELAY, init_unixpc, 40, 2, 9},
	{'C', "/usr/bin/DOS/dvd000", 0L, 12, 0, (int (*) ()) 0, 0, 0, 0},
	{'\0', (char *) NULL, 0L, 0, 0, (int (*) ()) 0, 0, 0, 0}
};

int
init_unixpc(fd, ntracks, nheads, nsect)
int fd, ntracks, nheads, nsect;
{
	struct gdctl gdbuf;

	if (ioctl(fd, GDGETA, &gdbuf) == -1) {
		ioctl(fd, GDDISMNT, &gdbuf);
		return(1);
	}

	gdbuf.params.cyls = ntracks;
	gdbuf.params.heads = nheads;
	gdbuf.params.psectrk = nsect;

	gdbuf.params.pseccyl = gdbuf.params.psectrk * gdbuf.params.heads;
	gdbuf.params.flags = 1;		/* disk type flag */
	gdbuf.params.step = 0;		/* step rate for controller */
	gdbuf.params.sectorsz = 512;	/* sector size */

	if (ioctl(fd, GDSETA, &gdbuf) < 0) {
		ioctl(fd, GDDISMNT, &gdbuf);
		return(1);
	}
	return(0);
}
#endif /* UNIXPC */


#ifdef RT_ACIS
struct device xdevices[] = {
	{'A', "/dev/rfd0", 0L, 12, 0, (int (*) ()) 0, 80, 2, 15},
	{'A', "/dev/rfd0", 0L, 12, 0, (int (*) ()) 0, 40, 2, 9},
	{'\0', (char *) NULL, 0L, 0, 0, (int (*) ()) 0, 0, 0, 0}
};
#endif /* RT_ACIS */

#ifdef SUN386
struct device xdevices[] = {
	{'A', "/dev/rfdl0c", 0L, 12, 0, (int (*) ()) 0, 80, 2, 9},
	{'A', "/dev/rfd0c", 0L, 12, 0, (int (*) ()) 0, 80, 2, 18},
	{'\0', (char *) NULL, 0L, 0, 0, (int (*) ()) 0, 0, 0, 0}
};
#endif /* SUN386 */

#ifdef SPARC_ODD
#include <sys/types.h>
#include <sun/dkio.h>
#include <fcntl.h>

int init_sparc();

struct device xdevices[] = {
	{'A', "/dev/rfd0c", 0L, 12, 0, init_sparc, 80, 2, 0},
	{'\0', (char *) NULL, 0L, 0, 0, (int (*) ()) 0, 0, 0, 0}
};

/*
 * Stuffing back the floppy parameters into the driver allows for gems
 * like 10 sector or single sided floppies from Atari ST systems.
 * 
 * Martin Schulz, Universite de Moncton, N.B., Canada, March 11, 1991.
 */

int
init_sparc(fd, ntracks, nheads, nsect)
int fd, ntracks, nheads, nsect;
{
	struct fdk_char dkbuf;
	struct dk_map   dkmap;

	if (ioctl(fd, FDKIOGCHAR, &dkbuf) != 0) {
		ioctl(fd, FDKEJECT, NULL);
		return(1);
	}

	if (ioctl(fd, DKIOCGPART, &dkmap) != 0) {
		ioctl(fd, FDKEJECT, NULL);
		return(1);
	}

	if (ntracks)
		dkbuf.ncyl = ntracks;
	if (nheads)
		dkbuf.nhead = nheads;
	if (nsect)
		dkbuf.secptrack = nsect;

	if (ntracks && nheads && nsect )
		dkmap.dkl_nblk = ntracks * nheads * nsect;

	if (ioctl(fd, FDKIOSCHAR, &dkbuf) != 0) {
		ioctl(fd, FDKEJECT, NULL);
		return(1);
	}

	if (ioctl(fd, DKIOCSPART, &dkmap) != 0) {
		ioctl(fd, FDKEJECT, NULL);
		return(1);
	}
	return(0);
}
#endif /* SPARC_ODD */
  
#ifdef XENIX
struct device xdevices[] = {
	{'A', "/dev/fd096ds15", 0L, 12, 0, (int (*) ()) 0, 80, 2, 15},
	{'A', "/dev/fd048ds9", 0L, 12, 0, (int (*) ()) 0, 40, 2, 9},
	{'B', "/dev/fd1135ds18", 0L, 12, 0, (int (*) ()) 0, 80, 2, 18},
	{'B', "/dev/fd1135ds9", 0L, 12, 0, (int (*) ()) 0, 80, 2, 9},
	{'C', "/dev/hd0d", 0L, 16, 0, (int (*) ()) 0, 0, 0, 0},
	{'\0', (char *) NULL, 0L, 0, 0, (int (*) ()) 0, 0, 0, 0}
};
#endif /* XENIX */

#ifdef __bsdi__
struct device xdevices[] = {
	{'A', "/dev/rfd0c", 0L, 12, 0, (int (*) ()) 0, 80, 2, 18},
	{'A', "/dev/rfd0_720_3.5", 0L, 12, 0, (int (*) ()) 0, 80, 2, 9},
	{'B', "/dev/rfd1c", 0L, 12, 0, (int (*) ()) 0, 80, 2, 15},
	{'B', "/dev/rfd1_360_5.25", 0L, 12, 0, (int (*) ()) 0, 40, 2, 9},
	{'\0', (char *) NULL, 0L, 0, 0, (int (*) ()) 0, 0, 0, 0}
};
#endif /* __bsdi__ */


/*
 * Dynamically read the configuration information
 */
int
initdevicefile (file, devices, olddevs)
	char *file;
	struct device *devices;
	int olddevs;
{
	char *p;
	char buf[BUFSZ];
	char namebuf[BUFSZ];
	char newdrive;
	int  numdevs;
	FILE *fp;
	int  i;
	
	if ((fp = fopen(file, "r")) == NULL)
		return(olddevs);	/* no new devices */
	    
	numdevs = olddevs;
	while (fgets(buf, BUFSZ, fp) != NULL) {

		/* strip comments, white space, and blank lines */
		if ((p = strchr(buf, '#')) != NULL)
			*p = '\0';
		if ((p = strchr(buf, '\n')) != NULL)
			*p = '\0';
		p = buf;
		while (isspace(*p)) p++;
		if (*p == '\0')
			continue;

		if (numdevs == MAX_DEVS) {
			fprintf (stderr, "initdevices: too many devices (Max: %d) - extra devices ignored\n", MAX_DEVS);
			break;
		}

		/* look to see if we've set up this drive in a previous file */
		newdrive = isupper(*p) ? *p : toupper(*p);
		for (i=0; i<olddevs; i++)
			if (newdrive == devices[i].drive)
				goto skip;

		/* letter name       offset FAT oflag tracks heads sectors */
		/* A      /dev/rfd0c 0      12  0     40     2     9       */

		if (sscanf(p, "%c%s%ld%d%d%d%d%d", &devices[numdevs].drive, 
			   namebuf, &devices[numdevs].offset,
			   &devices[numdevs].fat_bits, &devices[numdevs].mode, 
			   &devices[numdevs].tracks, &devices[numdevs].heads, 
			   &devices[numdevs].sectors) != 8) {
			fprintf (stderr, "initdevices: bad line in configuration file [%s]: %s\n", file, buf);
			continue;
		}
		if (!isupper(devices[numdevs].drive))
			devices[numdevs].drive = toupper(devices[numdevs].drive);
		devices[numdevs].gioctl = (int (*)()) NULL;
		devices[numdevs].name = strdup (namebuf);
		if (devices[numdevs].name == NULL)
		{
		    fprintf (stderr, "initdevices: strdup error!\n");
		    exit (1);
		}
		numdevs++;
skip:	;
	}
	fclose (fp);
	return(numdevs);
}

struct device *
initdevices ()
{
	int numdevs;
	char *home;
	char buf[BUFSZ];
	struct device *devices;
	extern char *getenv();

	devices = (struct device *)malloc((MAX_DEVS+1) * sizeof(struct device));
	numdevs = 0;

	/* load DEV_CONF from the current directory */
	numdevs = initdevicefile(DEV_CONF, devices, numdevs);

	/* load DEV_CONF from the user's $HOME */
	if ((home = getenv("HOME")) != NULL) {
		(void) sprintf(buf, "%s/%s", home, DEV_CONF);
		numdevs = initdevicefile(buf, devices, numdevs);
	}

	/* load the system DEV_CONF */
	numdevs = initdevicefile(SYS_DEV_CONF, devices, numdevs);

	if (numdevs) {
		bzero (&devices[numdevs], sizeof (struct device));
		return (devices);
	}
	else {
		/* fall back on compiled in devices */
		free (devices);
		return (xdevices);
	}
}
