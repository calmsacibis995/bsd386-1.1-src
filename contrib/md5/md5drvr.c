/* @(#)md5drvr.c	6.2 1/11/94 00:54:10 */
/*
 * md5drvr - md5 driver and test code
 *
 * Written by Landon Curt Noll  (chongo@toad.com)
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
 * NOTE: The version information below refers to all md5 code, not
 *	 just this file.  In particular, this file was created by
 *	 Landon Curt Noll.
 *
 * Version 1.1: 17 Feb 1990		RLR
 *	Original code written.
 *
 * Version 1.2: 27 Dec 1990		SRD,AJ,BSK,JT
 *	C reference version.
 *
 * Version 1.3: 27 Apr 1991		RLR
 *	G modified to have y&~z instead of y&z
 *	FF, GG, HH modified to add in last register done
 *	access pattern: round 2 works mod 5, round 3 works mod 3
 *	distinct additive constant for each step
 *	round 4 added, working mod 7
 *
 * Version 1.4: 10 Jul 1991		SRD,AJ,BSK,JT
 *	Constant correction.
 *
 * Version 2.1:	31 Dec 1993		Landon Curt Noll   (chongo@toad.com)
 *     Modified/Re-wrote md5.c
 *
 * Version 2.2: 09 Jan 1994		Landon Curt Noll   (chongo@toad.com)
 *     md5drvr.c and md5dual.c code cloned from shs version 2.5.8 94/01/09
 *     performance tuning
 *
 * Version 2.3: 10 Jan 1994		Landon Curt Noll   (chongo@toad.com)
 *     added MUST_ALIGN for Sparc and other RISC architectures
 *     use must_align.c to automatically determine if MUST_ALIGN is required
 *     performance tuning
 *     increased test to 64 megabytes due to increased performance
 *
 * Version 2.4:	non-existent version
 *
 * Version 2.5:	non-existent version
 *
 * Version 2.6: 10 Jan 1994		Landon Curt Noll   (chongo@toad.com)
 *     Merged the shs and md5 Makefiles to build both in the same directory
 *     Bumped version to 2.6 to match level to shs
 *     Test suite header now says md5 (not MD5)
 *     Minor performance improvements
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "md5.h"

/* size of test in megabytes */
#define TEST_MEG 64

/* number of blocks to process */
#define TEST_BLOCKS (TEST_MEG*1024*1024/READSIZE)

/* maximum size of pre_file that is used */
#define MAX_PRE_FILE READSIZE

/* MD5 test suite strings */
#define ENTRY(str) {(BYTE *)str, sizeof(str)-1}
struct MD5_test {
    BYTE *data;		/* data or NULL to test */
    int len;		/* length of data */
} MD5_test_data[] = {
    ENTRY(""),
    ENTRY("a"),
    ENTRY("abc"),
    ENTRY("message digest"),
    ENTRY("abcdefghijklmnopqrstuvwxyz"),
    ENTRY("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"),
    ENTRY("12345678901234567890123456789012345678901234567890123456789012345678901234567890")
};
#define MAX_MD5_TEST_DATA (sizeof(MD5_test_data)/sizeof(MD5_test_data[0]))

/* MD5 test filenames */
char *MD5_test_file[] = {
    "file1",
    "file2",
};
#define MAX_MD5_TEST_FILE (sizeof(MD5_test_file)/sizeof(MD5_test_file[0]))

/* Prototypes of the static functions */
static void MD5Data P((BYTE*, UINT, BYTE*, UINT, MD5_CTX*));
static void MD5Stream P((BYTE*, UINT, FILE*, MD5_CTX*));
static void MD5File P((BYTE*, UINT, char*, MD5_CTX*));
static void MD5Output P((char*, int, MD5_CTX*));
static int MD5PreFileRead P((char*, BYTE**));
static void MD5TestSuite P((void));
static void MD5Help P((void));
void main P((int, char**));

/* global variables */
char *program;			/* our name */
static int c_flag = 0;		/* 1 => print C style digest with leading 0x */
int i_flag = 0;			/* 1 => process inode & filename */
int q_flag = 0;			/* 1 => print the digest only */
int dot_zero = 0;		/* 1 => print .0 after the digest */
ULONG zero[MD5_BLOCKSIZE/sizeof(ULONG)];	/* block of zeros */


/*
 * MD5Data - digest a string
 */
static void
MD5Data(pre_str, pre_len, inString, in_len, dig)
    BYTE *pre_str;		/* string prefix or NULL */
    UINT pre_len;		/* length of pre_str */
    BYTE *inString;		/* string to digest */
    UINT in_len;		/* length of inString */
    MD5_CTX *dig;		/* current digest */
{
    int len;			/* total length of pre_str and inString */
    int indx;			/* byte stream index */
    BYTE *p;
    int i;

    /*
     * digest the pre-string
     */
    if (pre_str && pre_len > 0) {
	MD5Update(dig, pre_str, pre_len);
    }

    /*
     * digest the string
     */
    if (inString && in_len > 0) {
	MD5Update(dig, inString, in_len);
    }
}


/*
 * MD5Stream - digest a open file stream
 */
static void
MD5Stream(pre_str, pre_len, stream, dig)
    BYTE *pre_str;		/* data prefix or NULL */
    UINT pre_len;		/* length of pre_str */
    FILE *stream;		/* the stream to process */
    MD5_CTX *dig;		/* current digest */
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
	MD5Data(pre_str, pre_len, NULL, 0, dig);
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
	    if (dig->datalen == 0 && (bytes&(MD5_BLOCKSIZE-1)) == 0) {
		MD5fullUpdate(dig, (BYTE *)data, bytes);
		partial = 0;
	    } else {
		MD5Update(dig, (BYTE *)data, bytes);
	    }
	} else if ((bytes&(MD5_BLOCKSIZE-1)) == 0) {
	    MD5fullUpdate(dig, (BYTE *)data, bytes);
	} else {
	    MD5Update(dig, (BYTE *)data, bytes);
	    partial = 1;
	}
    }
}


/*
 * MD5File - digest a file
 */
static void
MD5File(pre_str, pre_len, filename, dig)
    BYTE *pre_str;		/* string prefix or NULL */
    UINT pre_len;		/* length of pre_str */
    char *filename;		/* the filename to process */
    MD5_CTX *dig;		/* current digest */
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
	    MD5Data(NULL, 0, (BYTE *)filename, strlen(filename), dig);
	}
    } else {
	if (i_flag) {
	    MD5Data(pre_str, pre_len, (BYTE *)filename, strlen(filename), dig);
	} else {
	    MD5Data(pre_str, pre_len, NULL, 0, dig);
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
        MD5Data((BYTE *)&hashbuf, sizeof(hashbuf), (BYTE *)&hashlbuf,
        	 sizeof(hashlbuf), dig);

	/*
	 * pad with zeros to process file data faster
	 */
	if (dig->datalen > 0) {
	    MD5Update(dig, (BYTE *)zero, MD5_BLOCKSIZE - dig->datalen);
	}
    }

    /*
     * process the data stream
     */
    MD5Stream(NULL, 0, inFile, dig);
    fclose(inFile);
}


/*
 * MD5Output - output the dual digests
 */
static void
MD5Output(str, quot, dig)
    char *str;		/* print string after digest, NULL => none */
    int quot;		/* 1 => surround str with a double quotes */
    MD5_CTX *dig;	/* current digest */
{
    /*
     * finalize the digest
     */
    MD5Final(dig);

    /*
     * print the digest
     */
    MD5Print(dig);
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
 * MD5Print - print a digest in hex
 *
 * Prints message digest buffer in MD5Info as 40 hexadecimal digits. Order is
 * from low-order byte to high-order byte of digest. Each byte is printed
 * with high-order hexadecimal digit first.
 *
 * If -c, then print a leading "0x".  If -i, then print a trailing ".0".
 */
void
MD5Print(MD5Info)
    MD5_CTX *MD5Info;
{
    int i;

    if (c_flag) {
	fputs("0x", stdout);
    }
    for (i = 0; i < 16; i++) {
	printf ("%02x", MD5Info->digest[i]);
    }
    if (dot_zero) {
	fputs(".0", stdout);
    }
}


/*
 * A time trial routine, to measure the speed of MD5.
 *
 * Measures user time required to digest TEST_MEG megabytes of characters.
 */
static void
MD5TimeTrial()
{
    ULONG data[READSIZE/sizeof(ULONG)];	/* test buffer */
    MD5_CTX MD5Info;			/* hash state */
    struct rusage start;		/* test start time */
    struct rusage stop;			/* test end time */
    double usrsec;			/* duration of test in user seconds */
    unsigned int i;

    /* initialize test data */
    for (i = 0; i < READSIZE; i++)
	((BYTE *)data)[i] = (BYTE)(i & 0xFF);

    /* start timer */
    if (!q_flag) {
	printf("md5 time trial for %d megabytes of test data ...", TEST_MEG);
	fflush(stdout);
    }
    getrusage(RUSAGE_SELF, &start);

    /* digest data in READSIZE byte blocks */
    MD5Init(&MD5Info);
    for (i=0; i < TEST_BLOCKS; ++i) {
	MD5fullUpdate(&MD5Info, (BYTE *)data, READSIZE);
    }
    MD5Final(&MD5Info);

    /* stop timer, get time difference */
    getrusage(RUSAGE_SELF, &stop);
    usrsec = (stop.ru_utime.tv_sec - start.ru_utime.tv_sec) +
    	   (double)(stop.ru_utime.tv_usec - start.ru_utime.tv_usec)/1000000.0;
    if (!q_flag) {
	putchar('\n');
    }
    MD5Print(&MD5Info);
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
MD5TestSuite()
{
    struct MD5_test *t;	/* current MD5 test */
    struct MD5_test *p;	/* current MD5 pre-string test */
    struct stat buf;		/* stat of a test file */
    MD5_CTX digest;		/* test digest */
    char **f;			/* current file being tested */
    int i;

    /*
     * print test header
     */
    puts("md5 test suite results:");

    /*
     * find all of the test files
     */
    for (i=0, f=MD5_test_file; i < MAX_MD5_TEST_FILE; ++i, ++f) {
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
    for (i=0, t=MD5_test_data; i < MAX_MD5_TEST_DATA; ++i, ++t) {
	MD5Init(&digest);
	MD5Data(NULL, 0, t->data, t->len, &digest);
	MD5Output((char *)(t->data), 1, &digest);
    }

    /*
     * try the files with all test strings as prefixes
     */
    for (i=0, f=MD5_test_file; i < MAX_MD5_TEST_FILE; ++i, ++f) {
	MD5Init(&digest);
	MD5File(NULL, 0, *f, &digest);
	MD5Output(*f, 0, &digest);
    }
    exit(0);
}


/*
 * MD5PreFileRead - read and process a prepend file
 *
 * Returns the length of pre_str, and modifies pre_str to
 * point at the malloced prepend data.
 */
static int
MD5PreFileRead(pre_file, buf)
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
 * MD5Help - print MD5 help message and exit
 */
static void
MD5Help()
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
 * main - MD5 main
 */
void
main(argc, argv)
    int argc;			/* arg count */
    char **argv;		/* the args */
{
    MD5_CTX digest;		/* our current digest */
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
	    MD5Help();
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
                program, "6", "2",
	        (strcmp(MD5_what,"@(#)") == 0 &&
	         strcmp("@(#)","@(#)") == 0 &&
	         strcmp(MD5dual_what,"@(#)") == 0 &&
	         strcmp(MD5_H_WHAT,"@(#)") == 0) ? "" : "+",
	        "94/01/11");
	    exit(0);
	case 'x':
	    x_flag = 1;
	    break;
	default:
	    MD5Help();
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
	    MD5TestSuite();
	}
	exit(0);
    }

    /*
     * process -t if needed
     */
    if (t_flag) {
	MD5TimeTrial();
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
	pre_str_len = MD5PreFileRead(pre_file, &pre_str);
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
	    MD5Init(&digest);
	    MD5Data(pre_str, pre_str_len, (BYTE *)data_str, strlen(data_str),
		&digest);
	    MD5Output(data_str, 1, &digest);

	/*
	 * case: digest stdin
	 */
	} else if (optind == argc) {
	    MD5Init(&digest);
	    MD5Stream(pre_str, pre_str_len, stdin, &digest);
	    MD5Output(NULL, 0, &digest);

	/*
	 * case: digest files
	 */
	} else {
	    if (i_flag) {
		dot_zero = 1;
	    }
	    for (; optind < argc; optind++) {
		MD5Init(&digest);
		MD5File(pre_str, pre_str_len, argv[optind], &digest);
		MD5Output(argv[optind], 0, &digest);
	    }
	}
    }

    /* all done */
    exit(0);
}
