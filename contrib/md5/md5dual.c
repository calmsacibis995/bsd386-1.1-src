/* @(#)md5dual.c	6.2 1/11/94 00:06:33 */
/*
 * md5dual - md5 dual digest code
 *
 * Split our data into even and odd byte index streams, digest them both
 * and output the digests, space separated on a line with a 0x prefix.
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
 * See md5drvr.c for version and modification history.
 */

char *MD5dual_what="@(#)";	/* #(@) if checked in */

#include <stdio.h>
#include <string.h>
#include "md5.h"

/* static declarations */
static void dualData P((BYTE*, UINT, BYTE*, UINT, MD5_CTX*, MD5_CTX*));
static void dualStream P((BYTE*, UINT, FILE*, MD5_CTX*, MD5_CTX*));
static void dualFile P((BYTE*, UINT, char*, MD5_CTX*, MD5_CTX*));
static void dualOutput P((char*, int, MD5_CTX*, MD5_CTX*));

/* dual test suite strings */
#define ENTRY(str) {(BYTE *)str, sizeof(str)-1}
struct dual_test {
    BYTE *data;		/* data or NULL to test */
    int len;		/* length of data */
} dual_test_data[] = {
    {NULL, 0},
    ENTRY(""),
    ENTRY("a"),
    ENTRY("aa"),
    ENTRY("aabbccddeeffgghhiijjkkllmmnnooppqqrrssttuuvvwwxxyyzz"),
    ENTRY("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"),
    ENTRY("chongo <Ich bin, du bist, aber ein Yit ist nicht!!! :-)> /\\../\\"),
    ENTRY("123456789 123456789 123456789 123456789 123456789 123456789 1234"),
    ENTRY("a123456789 123456789 123456789 123456789 123456789 123456789 1234")
};
#define MAX_DUAL_TEST_DATA (sizeof(dual_test_data)/sizeof(dual_test_data[0]))

/* dual test filenames */
char *dual_test_file[] = {
    "file1",
    "file2",
    "md5.data",
    "/dev/null"
};
#define MAX_DUAL_TEST_FILE (sizeof(dual_test_file)/sizeof(dual_test_file[0]))


/*
 * dualData - divide data into alternating bytes and digest both halves
 */
static void
dualData(pre_str, pre_len, inString, in_len, even, odd)
    BYTE *pre_str;		/* string prefix or NULL */
    UINT pre_len;		/* length of pre_str */
    BYTE *inString;		/* string to digest */
    UINT in_len;		/* length of inString */
    MD5_CTX *even;		/* even byte digest */
    MD5_CTX *odd;		/* odd byte digest */
{
    int len;			/* total length of pre_str and inString */
    BYTE *even_buf;		/* even byte array */
    BYTE *odd_buf;		/* odd byte array */
    int indx;			/* byte stream index */
    BYTE *p;
    int i;

    /*
     * determine lengths
     */
    len = (pre_str == NULL) ? 0 : pre_len;
    len += (inString == NULL) ? 0 : in_len;
    /* no strings, quick return */
    if (len == 0) {
        return;
    }
    /* only 1 byte, process now and return */
    if (len == 1) {
        if (pre_str) {
            MD5Update(even, (BYTE *)pre_str, 1);
        } else {
            MD5Update(odd, (BYTE *)inString, 1);
        }
        return;
    }

    /*
     * malloc both string halves
     */
    odd_buf = (BYTE *)malloc(len/2);
    if (odd_buf == NULL) {
	fprintf(stderr, "%s: bad malloc #1\n", program);
	exit(6);
    }
    even_buf = (BYTE *)malloc((len+1)/2);
    if (even_buf == NULL) {
	fprintf(stderr, "%s: bad malloc #2\n", program);
	exit(7);
    }

    /*
     * divide the pre-string
     */
    indx = 0;
    if (pre_str != NULL) {
	for (p=pre_str, i=0; i < pre_len; ++indx, ++i, ++p) {
	    if (indx & 0x1) {
	        odd_buf[indx>>1] = *p;
	    } else {
	        even_buf[indx>>1] = *p;
	    }
	}
    }

    /*
     * divide the string
     */
    if (inString != NULL) {
	for (p=inString, i=0; i < in_len; ++indx, ++i, ++p) {
	    if (indx & 0x1) {
	        odd_buf[indx>>1] = *p;
	    } else {
	        even_buf[indx>>1] = *p;
	    }
	}
    }

    /*
     * update both halves
     */
    MD5Update(even, (BYTE *)even_buf, (len+1)/2);
    MD5Update(odd, (BYTE *)odd_buf, len/2);

    /*
     * cleanup
     */
    free(even_buf);
    free(odd_buf);
}


/*
 * dualStream - divide a Stream into alternating bytes and digest both halves
 */
static void
dualStream(pre_str, pre_len, stream, even, odd)
    BYTE *pre_str;		/* data prefix or NULL */
    UINT pre_len;		/* length of pre_str */
    FILE *stream;		/* the stream to process */
    MD5_CTX *even;		/* even byte digest */
    MD5_CTX *odd;		/* odd byte digest */
{
    ULONG data[2*READSIZE/sizeof(ULONG)];	/* our read buffer */
    ULONG even_buf[READSIZE/sizeof(ULONG)];	/* even half */
    ULONG odd_buf[READSIZE/sizeof(ULONG)];	/* off half */
    int bytes;				/* bytes last read */
    int epartial;			/* 1 => even partial block */
    int opartial;			/* 1 => odd partial block */
    int elen;				/* length of even block */
    int olen;				/* length of odd block */
    BYTE *p;
    int i;

    /*
     * pre-process prefix if needed
     */
    if (pre_str != NULL) {
	dualData(pre_str, pre_len, NULL, 0, even, odd);
    }

    /*
     * determine if either half has a partial block
     */
    if (even->datalen > 0) {
	epartial = 1;
    } else {
	epartial = 0;
    }
    if (odd->datalen > 0) {
	opartial = 1;
    } else {
	opartial = 0;
    }

    /*
     * process the contents of the file
     */
    while ((bytes = fread((char *)data, 1, READSIZE*2, stream)) > 0) {

        /*
         * split bytes into two halves
         */
	for (i=0, olen=0, p=(BYTE *)data; i < bytes; i+=2, ++olen, p+=2) {
	    ((BYTE *)even_buf)[olen] = *p;
	    ((BYTE *)odd_buf)[olen] = *(p+1);
	}
	if (bytes & 0x1) {
	    ((BYTE *)even_buf)[olen] = p[bytes-1];
	    elen = olen+1;
	} else {
	    elen = olen;
	}

	/*
	 * digest even bytes
	 */
	if (epartial) {
	    if (even->datalen == 0 && (elen&(MD5_BLOCKSIZE-1)) == 0) {
		MD5fullUpdate(even, (BYTE *)even_buf, elen);
		epartial = 0;
	    } else {
		MD5Update(even, (BYTE *)even_buf, elen);
	    }
	} else if ((elen&(MD5_BLOCKSIZE-1)) == 0) {
	    MD5fullUpdate(even, (BYTE *)even_buf, elen);
	} else {
	    MD5Update(even, (BYTE *)even_buf, elen);
	    epartial = 1;
	}

	/*
	 * digest odd bytes
	 */
	if (opartial) {
	    if (odd->datalen == 0 && (olen&(MD5_BLOCKSIZE-1)) == 0) {
		MD5fullUpdate(odd, (BYTE *)odd_buf, olen);
		opartial = 0;
	    } else {
		MD5Update(odd, (BYTE *)odd_buf, olen);
	    }
	} else if ((olen&(MD5_BLOCKSIZE-1)) == 0) {
	    MD5fullUpdate(odd, (BYTE *)odd_buf, olen);
	} else {
	    MD5Update(odd, (BYTE *)odd_buf, olen);
	    opartial = 1;
	}
    }
}


/*
 * dualFile - divide a file into alternating bytes and digest both halves
 */
static void
dualFile(pre_str, pre_len, filename, even, odd)
    BYTE *pre_str;		/* string prefix or NULL */
    UINT pre_len;		/* length of pre_str */
    char *filename;		/* the filename to process */
    MD5_CTX *even;		/* even byte digest */
    MD5_CTX *odd;		/* odd byte digest */
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
	    dualData(NULL, 0, (BYTE *)filename, strlen(filename), even, odd);
	}
    } else {
	if (i_flag) {
	    dualData(pre_str, pre_len, (BYTE *)filename, strlen(filename),
		     even, odd);
	} else {
	    dualData(pre_str, pre_len, NULL, 0, even, odd);
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
        dualData((BYTE *)&hashbuf, sizeof(hashbuf), (BYTE *)&hashlbuf,
        	 sizeof(hashlbuf), even, odd);

	/*
	 * pad both halves with zeros to process file data faster
	 */
	if (even->datalen > 0) {
	    MD5Update(even, (BYTE *)zero, MD5_BLOCKSIZE - even->datalen);
	}
	if (odd->datalen > 0) {
	    MD5Update(odd, (BYTE *)zero, MD5_BLOCKSIZE - odd->datalen);
	}
    }

    /*
     * process the data stream
     */
    dualStream(NULL, 0, inFile, even, odd);
    fclose(inFile);
}


/*
 * dualOutput - output the dual digests
 */
static void
dualOutput(str, quot, even, odd)
    char *str;		/* print string after digest, NULL => none */
    int quot;		/* 1 => surround str with a double quotes */
    MD5_CTX *even;	/* even byte digest */
    MD5_CTX *odd;	/* odd byte digest */
{
    /*
     * finalize both sets
     */
    MD5Final(even);
    MD5Final(odd);

    /*
     * print the 320 bit hex value
     */
    MD5Print(even);
    putchar(' ');
    MD5Print(odd);
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
 * dualTest - MD5 dual test suite
 */
void
dualTest()
{
    struct dual_test *t;	/* current dual test */
    struct dual_test *p;	/* current dual pre-string test */
    struct stat buf;		/* stat of a test file */
    MD5_CTX even_dig;		/* even byte digest */
    MD5_CTX odd_dig;		/* odd byte digest */
    char **f;			/* current file being tested */
    int i;
    int j;

    /*
     * find all of the test files
     */
    for (i=0, f=dual_test_file; i < MAX_DUAL_TEST_FILE; ++i, ++f) {
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
    for (i=0, t=dual_test_data; i < MAX_DUAL_TEST_DATA; ++i, ++t) {
	for (j=0, p=dual_test_data; j < MAX_DUAL_TEST_DATA; ++j, ++p) {
	    printf("pre:%d data:%d\n", i, j);
	    MD5Init(&even_dig);
	    MD5Init(&odd_dig);
	    dualData(p->data, p->len, t->data, t->len, &even_dig, &odd_dig);
	    dualOutput(NULL, 0, &even_dig, &odd_dig);
	}
    }

    /*
     * try the files with all test strings as prefixes
     */
    for (i=0, p=dual_test_data; i < MAX_DUAL_TEST_DATA; ++i, ++p) {
	for (j=0, f=dual_test_file; j < MAX_DUAL_TEST_FILE; ++j, ++f) {
	    printf("pre:%d file:%s\n", i, *f);
	    MD5Init(&even_dig);
	    MD5Init(&odd_dig);
	    dualFile(p->data, p->len, *f, &even_dig, &odd_dig);
	    dualOutput(NULL, 0, &even_dig, &odd_dig);
	}
    }
    exit(0);
}


/*
 * dualMain - main driver of MD5 dual routines
 */
void
dualMain(argc, argv, pre_str, pre_len, data_str)
    int argc;			/* arg count left after getopt */
    char **argv;		/* args left after getopt */
    BYTE *pre_str;		/* pre-process this data first */
    UINT pre_len;		/* length of pre_str */
    char *data_str;		/* data is this string, not a file */
{
    extern int optind;		/* option index */
    MD5_CTX even_dig;		/* even byte digest */
    MD5_CTX odd_dig;		/* odd byte digest */
    char *p;

    /*
     * case: initialize both halves
     */
    MD5Init(&even_dig);
    MD5Init(&odd_dig);

    /*
     * digest a string
     */
    if (data_str != NULL) {
	dualData(pre_str, pre_len, (BYTE *)data_str, strlen(data_str),
	    &even_dig, &odd_dig);
	dualOutput(data_str, 1, &even_dig, &odd_dig);

    /*
     * case: digest stdin
     */
    } else if (optind == argc) {
	dualStream(pre_str, pre_len, stdin, &even_dig, &odd_dig);
	dualOutput(NULL, 0, &even_dig, &odd_dig);

    /*
     * case: digest files
     */
    } else {
	if (i_flag) {
	    dot_zero = 1;
	}
	for (; optind < argc; optind++) {
	    dualFile(pre_str, pre_len, argv[optind], &even_dig, &odd_dig);
	    dualOutput(argv[optind], 0, &even_dig, &odd_dig);
	}
    }
}
