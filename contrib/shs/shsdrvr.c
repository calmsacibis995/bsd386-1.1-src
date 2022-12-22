/* @(#)shsdrvr.c	6.4 1/11/94 01:06:51 */
/*
 * shsdrvr - shs driver and test code
 *
 * Based on code by Peter C. Gutmann.
 *
 * This file was written by:
 *
 *	 Landon Curt Noll  (chongo@toad.com)	chongo <was here> /\../\
 *
 * This code has been placed in the public domain.  Please do not 
 * copyright this code.
 *
 * LANDON CURT NOLL DISCLAIMS ALL WARRANTIES WITH  REGARD  TO
 * THIS  SOFTWARE,  INCLUDING  ALL IMPLIED WARRANTIES OF MER-
 * CHANTABILITY AND FITNESS.  IN NO EVENT SHALL  LANDON  CURT
 * NOLL  BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM  LOSS  OF
 * USE,  DATA  OR  PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR  IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 ***
 *
 * NOTE: The version information below refers to all shs code, not
 *	 just this file.  In particular, this file was created by
 *	 Landon Curt Noll.
 *
 * Version 1.1: 02 Sep 1992?			original authors
 *     This code is based on code by Peter C. Gutmann.  Much thanks goes
 *     to Peter C. Gutman (pgut1@cs.aukuni.ac.nz) , Shawn A. Clifford
 *     (sac@eng.ufl.edu), Pat Myrto (pat@rwing.uucp) and others who wrote
 *     and/or worked on the original code.
 *
 * Version 2.1:	31 Dec 1993		Landon Curt Noll   (chongo@toad.com)
 *     Reformatted, performance improvements and bug fixes
 *
 * Version 2.2:	02 Jan 1994		Landon Curt Noll   (chongo@toad.com)
 *     fixed -p usage
 *     better error messages
 *     added -c help
 *     added -c 0	(concatenation)
 *     reordered -i stat buffer pre-pending
 *
 * Version 2.3:	03 Jan 1994		Landon Curt Noll   (chongo@toad.com)
 *     added -c 1	(side by side)
 *     added -c 2	(even force to be odd)
 *     added -c x	(shs dual test suite)
 *     changed -c help to be -c h
 *     changed -c operand to type[,opt[,...]]
 *     prefix & string ABI now can take arbitrary binary data
 *     fixed memory leak
 *     fixed even/odd byte split bug
 *     added -P file
 *     added -q
 *     added UNROLL_LOOPS to control shs.c loop unrolling
 *     major performance improvements
 *
 * Version 2.4: 05 Jan 1994		Landon Curt Noll   (chongo@toad.com)
 *     renamed calc mode to dual mode
 *     removed all -c code
 *     added -d		(dual digests, space separated)
 *     rewrote most of the file, string and stream logic using shsdual code
 *
 * Version 2.5: 08 Jan 1994		Landon Curt Noll   (chongo@toad.com)
 *     added (new) -c	(print 0x in front of digests)
 *     removed st_blksize and st_blocks from -i preprocessing data
 *     only print .0 suffix if -i and digesting a file
 *     non-zero edit codes are now unique
 *     changed the dual test suite (shorter, added non alpha numeric chars)
 *     -i requires filenames
 *     fixed @(#) what string code
 *     boolean logic simplication by Rich Schroeppel (rcs@cs.arizona.edu)
 *     on the fly in a circular buffer by Colin Plumb (colin@nyx10.cs.du.edu)
 *
 * Version 2.6: 11 Jan 1994		Landon Curt Noll   (chongo@toad.com)
 *     Merged the shs and md5 Makefiles to build both in the same directory
 *     alignment and byte order performance improvements
 *     eliminate wateful memory copies
 *     shs transform contains no function calls (all inline is possible)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "shs.h"

/* size of test in megabytes */
#define TEST_MEG 16

/* number of blocks to process */
#define TEST_BLOCKS (TEST_MEG*1024*1024/READSIZE)

/* maximum size of pre_file that is used */
#define MAX_PRE_FILE READSIZE

/* shs test suite strings */
#define ENTRY(str) {(BYTE *)str, sizeof(str)-1}
struct shs_test {
    BYTE *data;		/* data or NULL to test */
    int len;		/* length of data */
} shs_test_data[] = {
    ENTRY(""),
    ENTRY("a"),
    ENTRY("abc"),
    ENTRY("message digest"),
    ENTRY("abcdefghijklmnopqrstuvwxyz"),
    ENTRY("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"),
    ENTRY("12345678901234567890123456789012345678901234567890123456789012345678901234567890")
};
#define MAX_SHS_TEST_DATA (sizeof(shs_test_data)/sizeof(shs_test_data[0]))

/* shs test filenames */
char *shs_test_file[] = {
    "file1",
    "file2",
};
#define MAX_SHS_TEST_FILE (sizeof(shs_test_file)/sizeof(shs_test_file[0]))

/* Prototypes of the static functions */
static void shsData P((BYTE*, UINT, BYTE*, UINT, SHS_INFO*));
static void shsStream P((BYTE*, UINT, FILE*, SHS_INFO*));
static void shsFile P((BYTE*, UINT, char*, SHS_INFO*));
static void shsOutput P((char*, int, SHS_INFO*));
static int shsPreFileRead P((char*, BYTE**));
static void shsTestSuite P((void));
static void shsHelp P((void));
void main P((int, char**));

/* global variables */
char *program;			/* our name */
static int c_flag = 0;		/* 1 => print C style digest with leading 0x */
int i_flag = 0;			/* 1 => process inode & filename */
int q_flag = 0;			/* 1 => print the digest only */
int dot_zero = 0;		/* 1 => print .0 after the digest */
ULONG zero[SHS_BLOCKSIZE/sizeof(ULONG)];	/* block of zeros */


/*
 * shsData - digest a string
 */
static void
shsData(pre_str, pre_len, inString, in_len, dig)
    BYTE *pre_str;		/* string prefix or NULL */
    UINT pre_len;		/* length of pre_str */
    BYTE *inString;		/* string to digest */
    UINT in_len;		/* length of inString */
    SHS_INFO *dig;		/* current digest */
{
    int len;			/* total length of pre_str and inString */
    int indx;			/* byte stream index */
    BYTE *p;
    int i;

    /*
     * digest the pre-string
     */
    if (pre_str && pre_len > 0) {
	shsUpdate(dig, pre_str, pre_len);
    }

    /*
     * digest the string
     */
    if (inString && in_len > 0) {
	shsUpdate(dig, inString, in_len);
    }
}


/*
 * shsStream - digest a open file stream
 */
static void
shsStream(pre_str, pre_len, stream, dig)
    BYTE *pre_str;		/* data prefix or NULL */
    UINT pre_len;		/* length of pre_str */
    FILE *stream;		/* the stream to process */
    SHS_INFO *dig;		/* current digest */
{
    ULONG data[READSIZE/sizeof(ULONG)];	/* our read buffer */
    int bytes;				/* bytes last read */
    int partial;			/* 1 => even partial block */
    BYTE *p;
    int i;

    /*
     * pre-process prefix if needed
     */
    if (pre_str != NULL) {
	shsData(pre_str, pre_len, NULL, 0, dig);
    }

    /*
     * determine if the digest has a partial block
     */
    if (dig->datalen > 0) {
	partial = 1;
    } else {
	partial = 0;
    }

    /*
     * process the contents of the file
     */
    while ((bytes = fread((char *)data, 1, READSIZE, stream)) > 0) {

	/*
	 * digest the read
	 */
	if (partial) {
	    if (dig->datalen == 0 && (bytes&(SHS_BLOCKSIZE-1)) == 0) {
		shsfullUpdate(dig, (BYTE *)data, bytes);
		partial = 0;
	    } else {
		shsUpdate(dig, (BYTE *)data, bytes);
	    }
	} else if ((bytes&(SHS_BLOCKSIZE-1)) == 0) {
	    shsfullUpdate(dig, (BYTE *)data, bytes);
	} else {
	    shsUpdate(dig, (BYTE *)data, bytes);
	    partial = 1;
	}
    }
}


/*
 * shsFile - digest a file
 */
static void
shsFile(pre_str, pre_len, filename, dig)
    BYTE *pre_str;		/* string prefix or NULL */
    UINT pre_len;		/* length of pre_str */
    char *filename;		/* the filename to process */
    SHS_INFO *dig;		/* current digest */
{
    FILE *inFile;		/* the open file stream */
    struct stat buf;		/* stat or lstat of file */
    struct hashstat hashbuf;	/* stat data to digest */
    struct hashstat hashlbuf;	/* lstat data to digest */

    /*
     * open the file
     */
    inFile = fopen(filename, "rb");
    if (inFile == NULL) {
	fprintf(stderr, "%s: cannot open %s: ", program, filename);
	perror("");
	return;
    }

    /*
     * pre-process prefix if needed
     */
    if (pre_str == NULL) {
	if (i_flag) {
	    shsData(NULL, 0, (BYTE *)filename, strlen(filename), dig);
	}
    } else {
	if (i_flag) {
	    shsData(pre_str, pre_len, (BYTE *)filename, strlen(filename), dig);
	} else {
	    shsData(pre_str, pre_len, NULL, 0, dig);
	}
    }

    /*
     * digest file stat and lstat
     */
    if (i_flag) {
	if (fstat(fileno(inFile), &buf) < 0) {
	    printf("%s can't be stated.\n", filename);
	    return;
	}
	hashbuf.st_dev = buf.st_dev;
	hashbuf.st_ino = buf.st_ino;
	hashbuf.st_mode = buf.st_mode;
	hashbuf.st_nlink = buf.st_nlink;
	hashbuf.st_uid = buf.st_uid;
	hashbuf.st_gid = buf.st_gid;
	hashbuf.st_size = buf.st_size;
	hashbuf.st_mtime = buf.st_mtime;
	hashbuf.st_ctime = buf.st_ctime;
	if (lstat(filename, &buf) < 0) {
	    printf("%s can't be lstated.\n", filename);
	    return;
	}
	hashlbuf.st_dev = buf.st_dev;
	hashlbuf.st_ino = buf.st_ino;
	hashlbuf.st_mode = buf.st_mode;
	hashlbuf.st_nlink = buf.st_nlink;
	hashlbuf.st_uid = buf.st_uid;
	hashlbuf.st_gid = buf.st_gid;
	hashlbuf.st_size = buf.st_size;
	hashlbuf.st_mtime = buf.st_mtime;
	hashlbuf.st_ctime = buf.st_ctime;
        shsData((BYTE *)&hashbuf, sizeof(hashbuf), (BYTE *)&hashlbuf,
        	 sizeof(hashlbuf), dig);

	/*
	 * pad with zeros to process file data faster
	 */
	if (dig->datalen > 0) {
	    shsUpdate(dig, (BYTE *)zero, SHS_BLOCKSIZE - dig->datalen);
	}
    }

    /*
     * process the data stream
     */
    shsStream(NULL, 0, inFile, dig);
    fclose(inFile);
}


/*
 * shsOutput - output the dual digests
 */
static void
shsOutput(str, quot, dig)
    char *str;		/* print string after digest, NULL => none */
    int quot;		/* 1 => surround str with a double quotes */
    SHS_INFO *dig;	/* current digest */
{
    /*
     * finalize the digest
     */
    shsFinal(dig);

    /*
     * print the digest
     */
    shsPrint(dig);
    if (str && !q_flag) {
	if (quot) {
	    printf(" \"%s\"\n", str);
	} else {
	    printf(" %s\n", str);
	}
    } else {
	putchar('\n');
    }
}


/*
 * shsPrint - print a digest in hex
 *
 * Prints message digest buffer in shsInfo as 40 hexadecimal digits. Order is
 * from low-order byte to high-order byte of digest. Each byte is printed
 * with high-order hexadecimal digit first.
 *
 * If -c, then print a leading "0x".  If -i, then print a trailing ".0".
 */
void
shsPrint(shsInfo)
    SHS_INFO *shsInfo;
{
    if (c_flag) {
	fputs("0x", stdout);
    }
    printf("%08lx%08lx%08lx%08lx%08lx",
	shsInfo->digest[0], shsInfo->digest[1], shsInfo->digest[2],
	shsInfo->digest[3], shsInfo->digest[4]);
    if (dot_zero) {
	fputs(".0", stdout);
    }
}


/*
 * A time trial routine, to measure the speed of SHS.
 *
 * Measures user time required to digest TEST_MEG megabytes of characters.
 */
static void
shsTimeTrial()
{
    ULONG data[READSIZE/sizeof(ULONG)];	/* test buffer */
    SHS_INFO shsInfo;			/* hash state */
    struct rusage start;		/* test start time */
    struct rusage stop;			/* test end time */
    double usrsec;			/* duration of test in user seconds */
    unsigned int i;

    /* initialize test data */
    for (i = 0; i < READSIZE; i++)
	((BYTE *)data)[i] = (BYTE)(i & 0xFF);

    /* start timer */
    if (!q_flag) {
	printf("shs time trial for %d megabytes of test data ...", TEST_MEG);
	fflush(stdout);
    }
    getrusage(RUSAGE_SELF, &start);

    /* digest data in READSIZE byte blocks */
    shsInit(&shsInfo);
    for (i=0; i < TEST_BLOCKS; ++i) {
	shsfullUpdate(&shsInfo, (BYTE *)data, READSIZE);
    }
    shsFinal(&shsInfo);

    /* stop timer, get time difference */
    getrusage(RUSAGE_SELF, &stop);
    usrsec = (stop.ru_utime.tv_sec - start.ru_utime.tv_sec) +
    	   (double)(stop.ru_utime.tv_usec - start.ru_utime.tv_usec)/1000000.0;
    if (!q_flag) {
	putchar('\n');
    }
    shsPrint(&shsInfo);
    if (q_flag) {
	putchar('\n');
    } else {
	printf(" is digest of test data\n");
	printf("user seconds to process test data: %.2f\n", usrsec);
	printf("characters processed per user second: %d\n",
	    (int)((double)TEST_MEG*1024.0*1024.0/usrsec));
    }
}


/*
 * Runs a standard suite of test data.
 */
static void
shsTestSuite()
{
    struct shs_test *t;	/* current shs test */
    struct shs_test *p;	/* current shs pre-string test */
    struct stat buf;		/* stat of a test file */
    SHS_INFO digest;		/* test digest */
    char **f;			/* current file being tested */
    int i;

    /*
     * print test header
     */
    puts("shs test suite results:");

    /*
     * find all of the test files
     */
    for (i=0, f=shs_test_file; i < MAX_SHS_TEST_FILE; ++i, ++f) {
	if (stat(*f, &buf) < 0) {
	    /* no file1 in this directory, cd to the test suite directory */
	    if (chdir(TLIB) < 0) {
		fflush(stdout);
		fprintf(stderr,
		    "%s: cannot find %s or %s/%s\n", program, *f, TLIB, *f);
		return;
	    }
	}
    }

    /*
     * try all combinations of test strings as prefixes and data
     */
    for (i=0, t=shs_test_data; i < MAX_SHS_TEST_DATA; ++i, ++t) {
	shsInit(&digest);
	shsData(NULL, 0, t->data, t->len, &digest);
	shsOutput((char *)(t->data), 1, &digest);
    }

    /*
     * try the files with all test strings as prefixes
     */
    for (i=0, f=shs_test_file; i < MAX_SHS_TEST_FILE; ++i, ++f) {
	shsInit(&digest);
	shsFile(NULL, 0, *f, &digest);
	shsOutput(*f, 0, &digest);
    }
    exit(0);
}


/*
 * shsPreFileRead - read and process a prepend file
 *
 * Returns the length of pre_str, and modifies pre_str to
 * point at the malloced prepend data.
 */
static int
shsPreFileRead(pre_file, buf)
    char *pre_file;		/* form pre_str from file pre_file */
    BYTE **buf;			/* pointer to pre_str pointer */
{
    struct stat statbuf;	/* stat for pre_file */
    int pre_len;		/* length of pre_file to be used */
    int bytes;			/* bytes read from pre_file */
    FILE *pre;			/* pre_file descriptor */

    /* obtain the length that we will use */
    if (stat(pre_file, &statbuf) < 0) {
	fprintf(stderr, "%s: unpable to find prepend file %s\n",
	    program, pre_file);
	exit(1);
    }
    pre_len = statbuf.st_size;
    if (pre_len > MAX_PRE_FILE) {
	/* don't use beyond MAX_PRE_FILE in size */
	pre_len = MAX_PRE_FILE;
    }

    /* malloc our pre string */
    *buf = (BYTE *)malloc(pre_len+1);
    if (*buf == NULL) {
	fprintf(stderr, "%s: malloc #3 failed\n", program);
	exit(2);
    }

    /* open our pre_file */
    pre = fopen(pre_file, "rb");
    if (pre == NULL) {
	fprintf(stderr, "%s: unable to open prepend file %s\n",
	  program, pre_file);
	exit(3);
    }

    /* read our pre_file data */
    bytes = fread((char *)(*buf), 1, pre_len, pre);
    if (bytes != pre_len) {
	fprintf(stderr,
	  "%s: unable to read %d bytes from prepend file %s\n",
	  program, pre_len, pre_file);
	exit(4);
    }

    /* return our length */
    return (pre_len);
}


/*
 * shsHelp - print shs help message and exit
 */
static void
shsHelp()
{
    fprintf(stderr,
      "%s [-cdhiqtx] [-p str] [-P str] [-s str] [file ...]\n",
      program);
    fprintf(stderr,
      "    -c          print C style digests with a leading 0x\n");
    fprintf(stderr,
      "    -d          dual digests of even and odd indexed bytes\n");
    fprintf(stderr,
      "    -h          prints this message\n");
    fprintf(stderr,
      "    -i          process inode and filename as well as file data\n");
    fprintf(stderr,
      "    -p str      prepend str to data before digesting\n");
    fprintf(stderr,
      "    -P str      prepend the file 'str' to data before digesting\n");
    fprintf(stderr,
      "    -q          print only the digest\n");
    fprintf(stderr,
      "    -s str      prints digest and contents of string\n");
    fprintf(stderr,
      "    -t          prints time statistics for %dM chars\n", TEST_MEG);
    fprintf(stderr,
      "    -v          print version\n");
    fprintf(stderr,
      "    -x          execute an extended standard suite of test data\n");
    fprintf(stderr,
      "    file        print digest and name of file\n");
    fprintf(stderr,
      "    (no args)   print digest of stdin\n");
    exit(0);
}


/*
 * main - shs main
 */
void
main(argc, argv)
    int argc;			/* arg count */
    char **argv;		/* the args */
{
    SHS_INFO digest;		/* our current digest */
    BYTE *pre_str = NULL;	/* pre-process this data first */
    char *pre_file = NULL;	/* pre-process this file first */
    char *data_str = NULL;	/* data is this string, not a file */
    UINT pre_str_len;		/* length of pre_str or pre_file */
    int d_flag = 0;		/* 1 => dual digest mode */
    int t_flag = 0;		/* 1 => -t was given */
    int x_flag = 0;		/* 1 => -x was given */
    extern char *optarg;	/* argument to option */
    extern int optind;		/* option index */
    int c;

    /*
     * parse args
     */
    program = argv[0];
    while ((c = getopt(argc, argv, "cdihp:P:qs:tvx")) != -1) {
        switch (c) {
        case 'c':
	    c_flag = 1;
	    break;
        case 'd':
	    d_flag = 1;
	    break;
	case 'h':
	    shsHelp();
	    /*NOTREACHED*/
	    break;
	case 'i':
            i_flag = 1;
            break;
	case 'p':
	    pre_str = (BYTE *)optarg;
	    break;
	case 'q':
	    q_flag = 1;
	    break;
	case 'P':
	    pre_file = optarg;
	    break;
        case 's':
            data_str = optarg;
            break;
	case 't':
	    t_flag = 1;
	    break;
	case 'v':
	    printf("%s: version 2.%s.%s%s %s\n",
	        program, "6", "4",
	        (strcmp(shs_what,"@(#)") == 0 &&
	         strcmp("@(#)","@(#)") == 0 &&
	         strcmp(shsdual_what,"@(#)") == 0 &&
	         strcmp(SHS_H_WHAT,"@(#)") == 0) ? "" : "+",
	        "94/01/11");
	    exit(0);
	case 'x':
	    x_flag = 1;
	    break;
	default:
	    shsHelp();
	    break;
        }
    }
    if (data_str && optind != argc) {
	fprintf(stderr, "%s: -s is not compatible with digesting files\n",
	    program);
	exit(8);
    }
    if (i_flag && optind == argc) {
	fprintf(stderr, "%s: -i works only on filenames\n", program);
	exit(9);
    }

    /*
     * process -x if needed
     */
    if (x_flag) {
        if (d_flag) {
            dualTest();
	} else {
	    shsTestSuite();
	}
	exit(0);
    }

    /*
     * process -t if needed
     */
    if (t_flag) {
	shsTimeTrial();
	exit(0);
    }

    /*
     * process -P or -p if needed
     */
    if (pre_str && pre_file) {
	fprintf(stderr, "%s: -p and -P conflict\n", program);
	exit(5);
    }
    if (pre_file) {
	pre_str_len = shsPreFileRead(pre_file, &pre_str);
    } else if (pre_str) {
        pre_str_len = strlen((char *)pre_str);
    } else {
        pre_str_len = 0;
    }

    /*
     * if -d, perform dual digest processing instead
     */
    if (d_flag) {
	dualMain(argc, argv, pre_str, pre_str_len, data_str);

    /*
     * if no -d, process string, stdin or files
     */
    } else {

	/*
	 * case: digest a string
	 */
	if (data_str != NULL) {
	    shsInit(&digest);
	    shsData(pre_str, pre_str_len, (BYTE *)data_str, strlen(data_str),
		&digest);
	    shsOutput(data_str, 1, &digest);

	/*
	 * case: digest stdin
	 */
	} else if (optind == argc) {
	    shsInit(&digest);
	    shsStream(pre_str, pre_str_len, stdin, &digest);
	    shsOutput(NULL, 0, &digest);

	/*
	 * case: digest files
	 */
	} else {
	    if (i_flag) {
		dot_zero = 1;
	    }
	    for (; optind < argc; optind++) {
		shsInit(&digest);
		shsFile(pre_str, pre_str_len, argv[optind], &digest);
		shsOutput(argv[optind], 0, &digest);
	    }
	}
    }

    /* all done */
    exit(0);
}
