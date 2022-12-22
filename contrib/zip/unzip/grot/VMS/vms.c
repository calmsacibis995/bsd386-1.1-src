/*************************************************************************
 *                                                                       *
 * Copyright (C) 1992 Igor Mandrichenko.                                 *
 * Permission is granted to any individual or institution to use, copy,  *
 * or redistribute this software so long as all of the original files    *
 * are included unmodified, that it is not sold for profit, and that     *
 * this copyright notice is retained.                                    *
 *                                                                       *
 *************************************************************************/

/*
 *    vms.c  by Igor Mandrichenko
 *    version 1.2-1
 *
 *    This module contains routines to extract VMS file attributes
 *    from extra field and create file with these attributes.  This
 *    source is mainly based on sources of file_io.c from UNZIP 4.1
 *    by Info-ZIP.  [Info-ZIP note:  very little of this code is from
 *    file_io.c; it has virtually been written from the ground up.
 *    Of the few lines which are from the older code, most are mine
 *    (G. Roelofs) and I make no claims upon them.  On the contrary,
 *    my/our thanks to Igor for his contributions!]
 */

/*
 *      Revision history:
 *      1.0-1   Mandrichenko    16-feb-1992
 *              Recognize -c option
 *      1.0-2   Mandrichenko    17-feb-1992
 *              Do not use ASYnchroneous mode.
 *      1.0-3   Mandrichenko    2-mar-1992
 *              Make code more standard
 *              Use lrec instead of crec -- unzip4.2p does not provide
 *              crec now.
 *      1.1     Mandrichenko    5-mar-1992
 *              Make use of asynchronous output.
 *              Be ready to extract RMS blocks of invalid size (because diff
 *              VMS version used to compress).
 *      1.1-1   Mandrichenko    11-mar-1992
 *              Use internal file attributes saved in pInfo to decide
 *              if the file is text.  [GRR:  temporarily disabled, since
 *              no way to override and force binary extraction]
 *      1.1-2   Mandrichenko    13-mar-1992
 *              Do not restore owner/protection info if -X not specified.
 *      1.1-3   Mandrichenko    30-may-1992
 *              Set revision date/time to creation date/time if none specified
 *              Take quiet flag into account.
 *      1.1-4   Cave Newt       14-jun-1992
 *              Check zipfile for variable-length format (unzip and zipinfo).
 *	1.2	Mandrichenko	21-jun-1992
 *		Use deflation/inflation for compression of extra blocks
 *		Free all allocated space
 *	1.2-1	Mandrichenko	23-jun-1992
 *		Interactively select an action when file exists
 */

#ifdef VMS			/*      VMS only !      */

#ifndef SYI$_VERSION
#define SYI$_VERSION 4096	/* VMS 5.4 definition */
#endif

#ifndef VAXC
				/* This definition may be missed */
struct XAB {
    unsigned char xab$b_cod;
    unsigned char xab$b_bln;
    short int xabdef$$_fill_1;
    char *xab$l_nxt;
};

#endif

#include "unzip.h"
#include <ctype.h>
#include <descrip.h>
#include <syidef.h>

#define ERR(s) !((s) & 1)

#define BUFS512 8192*2		/* Must be a multiple of 512 */

/*
*   Local static storage
*/
static struct FAB fileblk;
static struct XABDAT dattim;
static struct XABRDT rdt;
static struct RAB rab;

static struct FAB *outfab = 0;
static struct RAB *outrab = 0;
static struct XABFHC *xabfhc = 0;
static struct XABDAT *xabdat = 0;
static struct XABRDT *xabrdt = 0;
static struct XABPRO *xabpro = 0;
static struct XABKEY *xabkey = 0;
static struct XABALL *xaball = 0;
struct XAB *first_xab = 0L, *last_xab = 0L;

static char query = 0;
static int text_file = 0;

static char locbuf[BUFS512];
static int loccnt = 0;
static char *locptr;

static int WriteBuffer();
static int _flush_blocks();
static int _flush_records();
static byte *extract_block();
static void message();
static int get_vms_version();
static void free_up();
static int replace();

struct bufdsc
{
    struct bufdsc *next;
    byte *buf;
    int bufcnt;
};

static struct bufdsc b1, b2, *curbuf;
static byte buf1[BUFS512], buf2[BUFS512];


int check_format()		/* return non-0 if format is variable-length */
{
    int rtype;
    struct FAB fab;

    fab = cc$rms_fab;
    fab.fab$l_fna = zipfn;
    fab.fab$b_fns = strlen(zipfn);
    sys$open(&fab);
    rtype = fab.fab$b_rfm;
    sys$close(&fab);

    if (rtype == FAB$C_VAR || rtype == FAB$C_VFC)
    {
	fprintf(stderr,
		"\n     Error:  zipfile is in variable-length record format.  Please\n\
     run \"bilf l %s\" to convert the zipfile to stream-LF\n\
     record format.  (Bilf.exe, bilf.c and make_bilf.com are included\n\
     in the VMS UnZip source distribution.)\n\n", zipfn);
	return 2;		/* 2:  error in zipfile */
    }

    return 0;
}


#ifndef ZIPINFO

int create_output_file()
{				/* return non-0 if sys$create failed */
    int ierr, yr, mo, dy, hh, mm, ss;
    char timbuf[24];		/* length = first entry in "stupid" + 1 */
    int attr_given = 0;		/* =1 if VMS attributes are present in
				*     extra_field */

    rab = cc$rms_rab;		/* fill FAB & RAB with default values */
    fileblk = cc$rms_fab;

    text_file =/* pInfo->text || */aflag || cflag;

    if (attr_given = find_vms_attrs())
    {
	text_file = 0;
	if (cflag)
	{
	    printf("Cannot put VMS file %s to stdout.\n",
		   filename);
	    free_up();
	    return 50;
	}
    }

    if (!attr_given)
    {
	outfab = &fileblk;
	outfab->fab$l_xab = 0L;
	if (text_file)
	{
	    outfab->fab$b_rfm = FAB$C_VAR;	/* variable length records */
	    outfab->fab$b_rat = FAB$M_CR;	/* carriage-return carriage ctrl */
	}
	else
	{
	    outfab->fab$b_rfm = FAB$C_STMLF;	/* stream-LF record format */
	    outfab->fab$b_rat = FAB$M_CR;	/* carriage-return carriage ctrl */
	}
    }

    if (!cflag)
	outfab->fab$l_fna = filename;
    else
	outfab->fab$l_fna = "sys$output:";

    outfab->fab$b_fns = strlen(outfab->fab$l_fna);

    if ((!attr_given) || xabdat == 0 || xabrdt == 0)	/* Use date/time info
							 *  from zipfile if
							 *  no attributes given
							 */
    {
	static char *month[] =
	    {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
	     "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

	/*  fixed-length string descriptor: */
	struct dsc$descriptor stupid =
	    {23, DSC$K_DTYPE_T, DSC$K_CLASS_S, timbuf};

	yr = ((lrec.last_mod_file_date >> 9) & 0x7f) + 1980;
	mo = ((lrec.last_mod_file_date >> 5) & 0x0f) - 1;
	dy = (lrec.last_mod_file_date & 0x1f);
	hh = (lrec.last_mod_file_time >> 11) & 0x1f;
	mm = (lrec.last_mod_file_time >> 5) & 0x3f;
	ss = (lrec.last_mod_file_time & 0x1f) * 2;

	dattim = cc$rms_xabdat;	/* fill XABs with default values */
	rdt = cc$rms_xabrdt;
	sprintf(timbuf, "%02d-%3s-%04d %02d:%02d:%02d.00", dy, month[mo], yr,
		hh, mm, ss);
	sys$bintim(&stupid, &dattim.xab$q_cdt);
	memcpy(&rdt.xab$q_rdt, &dattim.xab$q_cdt, sizeof(rdt.xab$q_rdt));

	if ((!attr_given) || xabdat == 0L)
	{
	    dattim.xab$l_nxt = outfab->fab$l_xab;
	    outfab->fab$l_xab = &dattim;
	}
    }

    outfab->fab$w_ifi = 0;	/* Clear IFI. It may be nonzero after ZIP */

    ierr = sys$create(outfab);
    if (ierr == RMS$_FEX)
	ierr = replace();

    if (ierr == 0)		/* Canceled */
	return free_up(), 1;

    if (ERR(ierr))
    {
	char buf[256];

	sprintf(buf, "[ Cannot create output file %s ]\n", filename);
	message(buf, ierr);
	message("", outfab->fab$l_stv);
	free_up();
	return (1);
    }

    if (!text_file && !cflag)	/* Do not reopen text files and stdout
				*  Just open them in right mode         */
    {
	/*
	*       Reopen file for Block I/O with no XABs.
	*/
	if ((ierr = sys$close(outfab)) != RMS$_NORMAL)
	{
#ifdef DEBUG
	    message("[ create_output_file: sys$close failed ]\n", ierr);
	    message("", outfab->fab$l_stv);
#endif
	    fprintf(stderr, "Can't create output file:  %s\n", filename);
	    free_up();
	    return (1);
	}


	outfab->fab$b_fac = FAB$M_BIO | FAB$M_PUT;	/* Get ready for block
							 * output */
	outfab->fab$l_xab = 0L;	/* Unlink all XABs */

	if ((ierr = sys$open(outfab)) != RMS$_NORMAL)
	{
	    char buf[256];

	    sprintf(buf, "[ Cannot open output file %s ]\n", filename);
	    message(buf, ierr);
	    message("", outfab->fab$l_stv);
	    free_up();
	    return (1);
	}
    }

    outrab = &rab;
    rab.rab$l_fab = outfab;
    if (!text_file) rab.rab$l_rop |= RAB$M_BIO;
    if (!text_file) rab.rab$l_rop |= RAB$M_ASY;
    rab.rab$b_rac = RAB$C_SEQ;

    if ((ierr = sys$connect(outrab)) != RMS$_NORMAL)
    {
#ifdef DEBUG
	message("create_output_file: sys$connect failed.\n", ierr);
	message("", outfab->fab$l_stv);
#endif
	fprintf(stderr, "Can't create output file:  %s\n", filename);
	free_up();
	return (1);
    }

    locptr = &locbuf[0];
    loccnt = 0;

    b1.buf = &buf1[0];
    b1.bufcnt = 0;
    b1.next = &b2;
    b2.buf = &buf2[0];
    b2.bufcnt = 0;
    b2.next = &b1;
    curbuf = &b1;

    return (0);
}

static int replace()
{				/*
				*	File exists. Inquire user about futher action.
				*/
    char answ[10];
    struct NAM nam;
    int ierr;

    if (query == 0)
    {
	do
	{
	    fprintf(stderr,
		    "Replace %s : [o]verwrite, new [v]ersion or [c]ancel (O,V,C - all) ? ",
		    filename);
	    fflush(stderr);
	} while (fgets(answ, 9, stderr) == NULL && !isalpha(answ[0])
		 && tolower(answ[0]) != 'o'
		 && tolower(answ[0]) != 'v'
		 && tolower(answ[0]) != 'c');

	if (isupper(answ[0]))
	    query = answ[0] = tolower(answ[0]);
    }
    else
	answ[0] = query;

    switch (answ[0])
    {
	case 'c':
	    ierr = 0;
	    break;
	case 'v':
	    nam = cc$rms_nam;
	    nam.nam$l_rsa = filename;
	    nam.nam$b_rss = FILNAMSIZ - 1;

	    outfab->fab$l_fop |= FAB$M_MXV;
	    outfab->fab$l_nam = &nam;

	    ierr = sys$create(outfab);
	    if (!ERR(ierr))
	    {
		outfab->fab$l_nam = 0L;
		filename[outfab->fab$b_fns = nam.nam$b_rsl] = 0;
	    }
	    break;
	case 'o':
	    outfab->fab$l_fop |= FAB$M_SUP;
	    ierr = sys$create(outfab);
	    break;
    }
    return ierr;
}

/*
*   Extra record format
*   ===================
*   signature       (2 bytes)   = 'I','M'
*   size            (2 bytes)
*   block signature (4 bytes)
*   flags           (2 bytes)
*   uncomprssed size(2 bytes)
*   reserved        (4 bytes)
*   data            ((size-12) bytes)
*   ....
*/

#define BC_MASK 	07	/* 3 bits for compression type */
#define BC_STORED	0	/* Stored */
#define BC_00		1	/* 0byte -> 0bit compression */
#define BC_DEFL		2	/* Deflated */

struct extra_block
{
    UWORD sig;			/* Extra field block header structure */
    UWORD size;
    ULONG bid;
    UWORD flags;
    UWORD length;
    ULONG reserved;
    byte body[1];
};

/*
 *   Extra field signature and block signatures
 */

#define SIGNATURE "IM"
#define FABL    (cc$rms_fab.fab$b_bln)
#define RABL    (cc$rms_rab.rab$b_bln)
#define XALLL   (cc$rms_xaball.xab$b_bln)
#define XDATL   (cc$rms_xabdat.xab$b_bln)
#define XFHCL   (cc$rms_xabfhc.xab$b_bln)
#define XKEYL   (cc$rms_xabkey.xab$b_bln)
#define XPROL   (cc$rms_xabpro.xab$b_bln)
#define XRDTL   (cc$rms_xabrdt.xab$b_bln)
#define XSUML   (cc$rms_xabsum.xab$b_bln)
#define EXTBSL  4		/* Block signature length   */
#define RESL    8		/* Rserved 8 bytes  */
#define EXTHL   (4+EXTBSL)
#define FABSIG  "VFAB"
#define XALLSIG "VALL"
#define XFHCSIG "VFHC"
#define XDATSIG "VDAT"
#define XRDTSIG "VRDT"
#define XPROSIG "VPRO"
#define XKEYSIG "VKEY"
#define XNAMSIG "VNAM"
#define VERSIG  "VMSV"



#define W(p)    (*(unsigned short*)(p))
#define L(p)    (*(unsigned long*)(p))
#define EQL_L(a,b)      ( L(a) == L(b) )
#define EQL_W(a,b)      ( W(a) == W(b) )

/****************************************************************
 * Function find_vms_attrs scans ZIP entry extra field if any   *
 * and looks for VMS attribute records. Returns 0 if either no  *
 * attributes found or no fab given.                            *
 ****************************************************************/
int find_vms_attrs()
{
    byte *scan = extra_field;
    struct extra_block *blk;
    int len;

    outfab = xabfhc = xabdat = xabrdt = xabpro = first_xab = last_xab = 0L;

    if (scan == NULL)
	return 0;
    len = lrec.extra_field_length;

#define LINK(p) {	/* Link xaballs and xabkeys into chain */	\
                if( first_xab == 0L )                   \
                        first_xab = p;                  \
                if( last_xab != 0L )                    \
                        last_xab -> xab$l_nxt = p;      \
                last_xab = p;                           \
                p -> xab$l_nxt = 0;                     \
        }
    /* End of macro LINK */

    while (len > 0)
    {
	blk = (struct block *) scan;
	if (EQL_W(&blk->sig, SIGNATURE))
	{
	    byte *block_id;

	    block_id = &blk->bid;
	    if (EQL_L(block_id, FABSIG))
	    {
		outfab = (struct FAB *) extract_block(blk, 0,
						      &cc$rms_fab, FABL);
	    }
	    else if (EQL_L(block_id, XALLSIG))
	    {
		xaball = (struct XABALL *) extract_block(blk, 0,
							 &cc$rms_xaball, XALLL);
		LINK(xaball);
	    }
	    else if (EQL_L(block_id, XKEYSIG))
	    {
		xabkey = (struct XABKEY *) extract_block(blk, 0,
							 &cc$rms_xabkey, XKEYL);
		LINK(xabkey);
	    }
	    else if (EQL_L(block_id, XFHCSIG))
	    {
		xabfhc = (struct XABFHC *) extract_block(blk, 0,
							 &cc$rms_xabfhc, XFHCL);
	    }
	    else if (EQL_L(block_id, XDATSIG))
	    {
		xabdat = (struct XABDAT *) extract_block(blk, 0,
							 &cc$rms_xabdat, XDATL);
	    }
	    else if (EQL_L(block_id, XRDTSIG))
	    {
		xabrdt = (struct XABRDT *) extract_block(blk, 0,
							 &cc$rms_xabrdt, XRDTL);
	    }
	    else if (EQL_L(block_id, XPROSIG))
	    {
		xabpro = (struct XABPRO *) extract_block(blk, 0,
							 &cc$rms_xabpro, XPROL);
	    }
	    else if (EQL_L(block_id, VERSIG))
	    {
		char verbuf[80];
		int verlen = 0;
		byte *vers;
		char *m;

		get_vms_version(verbuf, 80);
		vers = extract_block(blk, &verlen, 0, 0);
		if ((m = strrchr(vers, '-')) != NULL)
		    *m = 0;	/* Cut out release number */
		if (strcmp(verbuf, vers) && quietflg == 0)
		{
		    printf("[ Warning: VMS version mismatch.");

		    printf("   This version %s --", verbuf);
		    strncpy(verbuf, vers, verlen);
		    verbuf[verlen] = 0;
		    printf(" version made by %s ]\n", verbuf);
		}
		free(vers);
	    }
	    else if (quietflg == 0)
		fprintf(stderr, "[ Warning: Unknown block signature %s ]\n",
			block_id);
	}
	len -= blk->size + 4;
	scan += blk->size + 4;
    }


    if (outfab != 0)
    {				/* Do not link XABPRO,XABRDT now. Leave them for sys$close() */

	outfab->fab$l_xab = 0L;
	if (xabfhc != 0L)
	{
	    xabfhc->xab$l_nxt = outfab->fab$l_xab;
	    outfab->fab$l_xab = xabfhc;
	}
	if (xabdat != 0L)
	{
	    xabdat->xab$l_nxt = outfab->fab$l_xab;
	    outfab->fab$l_xab = xabdat;
	}
	if (first_xab != 0L)	/* Link xaball,xabkey subchain */
	{
	    last_xab->xab$l_nxt = outfab->fab$l_xab;
	    outfab->fab$l_xab = first_xab;
	}
	return 1;
    }
    else
	return 0;
}

static void free_up()
{				/*
				*	Free up all allocated xabs
				*/
    if (xabdat != 0L) free(xabdat);
    if (xabpro != 0L) free(xabpro);
    if (xabrdt != 0L) free(xabrdt);
    if (xabfhc != 0L) free(xabfhc);
    while (first_xab != 0L)
    {
	struct XAB *x;

	x = first_xab->xab$l_nxt;
	free(first_xab);
	first_xab = x;
    }
    if (outfab != 0L && outfab != &fileblk)
	free(outfab);
}

static int get_vms_version(verbuf, len)
    char *verbuf;
int len;
{
    int i = SYI$_VERSION;
    int verlen = 0;
    struct dsc$descriptor version;
    char *m;

    version.dsc$a_pointer = verbuf;
    version.dsc$w_length = len - 1;
    version.dsc$b_dtype = DSC$K_DTYPE_B;
    version.dsc$b_class = DSC$K_CLASS_S;

    if (ERR(lib$getsyi(&i, 0, &version, &verlen, 0, 0)) || verlen == 0)
	return 0;

    /* Cut out trailing spaces "V5.4-3   " -> "V5.4-3" */
    for (m = verbuf + verlen, i = verlen - 1; i > 0 && verbuf[i] == ' '; --i)
	--m;
    *m = 0;

    /* Cut out release number "V5.4-3" -> "V5.4" */
    if ((m = strrchr(verbuf, '-')) != NULL)
	*m = 0;
    return strlen(verbuf) + 1;	/* Transmit ending 0 too */
}

/******************************
 *   Function extract_block   *
 ******************************/
/*
 * Extracts block from p. If resulting length is less then needed, fill
 * extra space with corresponding bytes from 'init'.
 * Currently understands 3 formats of block compression:
 * - Simple storing
 * - Compression of zero bytes to zero bits
 * - Deflation. See memextract() from extract.c
 */
static byte *extract_block(p, retlen, init, needlen)
    struct extra_block *p;
int *retlen;
byte *init;
int needlen;
{
    byte *block;		/* Pointer to block allocated */
    int cmptype;
    int usiz, csiz, max;

    cmptype = p->flags & BC_MASK;
    csiz = p->size - EXTBSL - RESL;
    usiz = (cmptype == BC_STORED ? csiz : p->length);

    if (needlen == 0)
	needlen = usiz;

    if (retlen)
	*retlen = usiz;

#ifndef MAX
#define MAX(a,b)	( (a) > (b) ? (a) : (b) )
#endif

    if ((block = (byte *) malloc(MAX(needlen, usiz))) == NULL)
	return NULL;

    if (init && (usiz < needlen))
	memcpy(block, init, needlen);

    switch (cmptype)
    {
	case BC_STORED:	/* The simplest case */
	    memcpy(block, &(p->body[0]), usiz);
	    break;
	case BC_00:
	    decompress_bits(block, usiz, &(p->body[0]));
	    break;
	case BC_DEFL:
	    memextract(block, usiz, &(p->body[0]), csiz);
	    break;
	default:
	    free(block);
	    block = NULL;
    }
    return block;
}

/*
 *  Simple uncompression routine. The compression uses bit stream.
 *  Compression scheme:
 *
 *  if(byte!=0)
 *      putbit(1),putbyte(byte)
 *  else
 *      putbit(0)
 */
static void decompress_bits(outptr, needlen, bitptr)
    byte *bitptr;

/* Pointer into compressed data */
byte *outptr;			/* Pointer into output block */
int needlen;			/* Size of uncompressed block */
{
    ULONG bitbuf = 0;
    int bitcnt = 0;

#define _FILL   if(bitcnt+8 <= 32)                      \
                {       bitbuf |= (*bitptr++) << bitcnt;\
                        bitcnt += 8;                    \
                }

    while (needlen--)
    {
	if (bitcnt <= 0)
	    _FILL;

	if (bitbuf & 1)
	{
	    bitbuf >>= 1;
	    if ((bitcnt -= 1) < 8)
		_FILL;
	    *outptr++ = (byte) bitbuf;
	    bitcnt -= 8;
	    bitbuf >>= 8;
	}
	else
	{
	    *outptr++ = 0;
	    bitcnt -= 1;
	    bitbuf >>= 1;
	}
    }
}

/***************************/
/*  Function FlushOutput() */
/***************************/

int FlushOutput()
{
    if (mem_mode)
    {				/* Hope, mem_mode stays constant during
				 * extraction */
	int rc = FlushMemory();	/* For mem_extract() */

	outpos += outcnt;
	outcnt = 0;
	outptr = outbuf;
	return rc;
    }

    /* return PK-type error code */
    /* flush contents of output buffer */
    if (tflag)
    {				/* Do not output. Update CRC only */
	UpdateCRC(outbuf, outcnt);
	outpos += outcnt;
	outcnt = 0;
	outptr = outbuf;
	return 0;
    }
    else
	return text_file ? _flush_records(0) : _flush_blocks(0);
}

static int _flush_blocks(final_flag)	/* Asynchronous version */
    int final_flag;

/* 1 if this is the final flushout */
{
    int round;
    int rest;
    int off = 0;
    int out_count = outcnt;
    int status;

    while (out_count > 0)
    {
	if (curbuf->bufcnt < BUFS512)
	{
	    int ncpy;

	    ncpy = out_count > (BUFS512 - curbuf->bufcnt) ?
			    BUFS512 - curbuf->bufcnt :
			    out_count;
	    memcpy(curbuf->buf + curbuf->bufcnt, outbuf + off, ncpy);
	    out_count -= ncpy;
	    curbuf->bufcnt += ncpy;
	    off += ncpy;
	}
	if (curbuf->bufcnt == BUFS512)
	{
	    status = WriteBuffer(curbuf->buf, curbuf->bufcnt);
	    if (status)
		return status;
	    curbuf = curbuf->next;
	    curbuf->bufcnt = 0;
	}
    }

    UpdateCRC(outbuf, outcnt);
    outpos += outcnt;
    outcnt = 0;
    outptr = outbuf;

    return (final_flag && (curbuf->bufcnt > 0)) ?
	WriteBuffer(curbuf->buf, curbuf->bufcnt) :
	0;
    /* 0:  no error */
}

#define RECORD_END(c) ((c) == CR || (c) == LF || (c) == CTRLZ)

static int _flush_records(final_flag)
    int final_flag;

/* 1 if this is the final flushout */
{
    int rest;
    int end = 0, start = 0;
    int off = 0;

    if (outcnt == 0 && loccnt == 0)
	return 0;		/* Nothing to do ... */

    if (loccnt)
    {
	for (end = 0; end < outcnt && !RECORD_END(outbuf[end]);)
	    ++end;

	if (end >= outcnt && !final_flag)
	{
	    if (WriteRecord(locbuf, loccnt))
		return (50);
	    fprintf(stderr, "[ Warning: Record too long (%d) ]\n",
		    outcnt + loccnt);
	    memcpy(locbuf, outbuf, outcnt);
	    locptr = &locbuf[loccnt = outcnt];
	}
	else
	{
	    memcpy(locptr, outbuf, end);
	    if (WriteRecord(locbuf, loccnt + end))
		return (50);
	    loccnt = 0;
	    locptr = &locbuf;
	}
	start = end + 1;

	if (start < outcnt && outbuf[end] == CR && outbuf[start] == LF)
	    ++start;
    }

    do
    {
	while (start < outcnt && outbuf[start] == CR)	/* Skip CR's at the
							*  beginning of rec. */
	    ++start;
	/* Find record end */
	for (end = start; end < outcnt && !RECORD_END(outbuf[end]);)
	    ++end;

	if (end < outcnt)
	{			/* Record end found, write the record */
	    if (WriteRecord(outbuf + start, end - start))
		return (50);
	    /* Shift to the begining of the next record */
	    start = end + 1;
	}

	if (start < outcnt && outbuf[end] == CR && outbuf[start] == LF)
	    ++start;

    } while (start < outcnt && end < outcnt);

    rest = outcnt - start;

    if (rest > 0)
	if (final_flag)
	{
	    /* This is a final flush. Put out all remaining in
	    *  the buffer                               */
	    if (loccnt && WriteRecord(locbuf, loccnt))
		return (50);
	}
	else
	{
	    memcpy(locptr, outbuf + start, rest);
	    locptr += rest;
	    loccnt += rest;
	}
    UpdateCRC(outbuf, outcnt);
    outpos += outcnt;
    outcnt = 0;
    outptr = outbuf;
    return (0);			/* 0:  no error */
}

/***************************/
/*  Function WriteBuffer() */
/***************************/

static int WriteBuffer(buf, len)/* return 0 if successful, 1 if not */
    unsigned char *buf;
int len;
{
    int status;

    status = sys$wait(outrab);
#ifdef DEBUG
    if (ERR(status))
    {
	message("[ WriteBuffer: sys$wait failed ]\n", status);
	message("", outrab->rab$l_stv);
    }
#endif
    outrab->rab$w_rsz = len;
    outrab->rab$l_rbf = buf;

    if (ERR(status = sys$write(outrab)))
    {
	message("[ WriteBuffer: sys$write failed ]\n", status);
	message("", outrab->rab$l_stv);
	return 50;
    }
    return (0);
}

/***************************/
/*  Function WriteRecord() */
/***************************/

static int WriteRecord(rec, len)/* return 0 if successful, 1 if not */
    unsigned char *rec;
int len;
{
    int status;

    sys$wait(outrab);
#ifdef DEBUG
    if (ERR(status))
    {
	message("[ WriteRecord: sys$wait faled ]\n", status);
	message("", outrab->rab$l_stv);
    }
#endif
    outrab->rab$w_rsz = len;
    outrab->rab$l_rbf = rec;

    if (ERR(status = sys$put(outrab)))
    {
	message("[ WriteRecord: sys$put failed ]\n", status);
	message("", outrab->rab$l_stv);
	return 50;
    }
    return (0);
}

/********************************/
/*  Function CloseOutputFile()  */
/********************************/

int CloseOutputFile()
{
    int status;

    if (text_file) _flush_records(1);
    else
	_flush_blocks(1);
    /* Link XABRDT,XABDAT and optionaly XABPRO */
    if (xabrdt != 0L)
    {
	xabrdt->xab$l_nxt = 0L;
	outfab->fab$l_xab = xabrdt;
    }
    else
    {
	rdt.xab$l_nxt = 0L;
	outfab->fab$l_xab = &rdt;
    }
    if (xabdat != 0L)
    {
	xabdat->xab$l_nxt = outfab->fab$l_xab;
	outfab->fab$l_xab = xabdat;
    }
    if (secinf && xabpro != 0L)
    {
	xabpro->xab$l_nxt = outfab->fab$l_xab;
	outfab->fab$l_xab = xabpro;
    }

    sys$wait(outrab);

    status = sys$close(outfab);
#ifdef DEBUG
    if (ERR(status))
    {
	message("\r[ Warning: cannot set owner/protection/time attributes ]\n", status);
	message("", outfab->fab$l_stv);
    }
#endif
    free_up();
}

#ifdef DEBUG
dump_rms_block(p)
    unsigned char *p;
{
    unsigned char bid, len;
    int err;
    char *type;
    char buf[132];
    int i;

    err = 0;
    bid = p[0];
    len = p[1];
    switch (bid)
    {
	case FAB$C_BID:
	    type = "FAB";
	    break;
	case XAB$C_ALL:
	    type = "xabALL";
	    break;
	case XAB$C_KEY:
	    type = "xabKEY";
	    break;
	case XAB$C_DAT:
	    type = "xabDAT";
	    break;
	case XAB$C_RDT:
	    type = "xabRDT";
	    break;
	case XAB$C_FHC:
	    type = "xabFHC";
	    break;
	case XAB$C_PRO:
	    type = "xabPRO";
	    break;
	default:
	    type = "Unknown";
	    err = 1;
	    break;
    }
    printf("Block @%08X of type %s (%d).", p, type, bid);
    if (err)
    {
	printf("\n");
	return;
    }
    printf(" Size = %d\n", len);
    printf(" Offset - Hex - Dec\n");
    for (i = 0; i < len; i += 8)
    {
	int j;

	printf("%3d - ", i);
	for (j = 0; j < 8; j++)
	    if (i + j < len)
		printf("%02X ", p[i + j]);
	    else
		printf("   ");
	printf(" - ");
	for (j = 0; j < 8; j++)
	    if (i + j < len)
		printf("%03d ", p[i + j]);
	    else
		printf("    ");
	printf("\n");
    }
}

#endif				/* DEBUG */

static void message(string, status)
    int status;
char *string;
{
    char msgbuf[256];

    $DESCRIPTOR(msgd, msgbuf);
    int msglen = 0;

    if (ERR(lib$sys_getmsg(&status, &msglen, &msgd, 0, 0)))
	fprintf(stderr, "%s[ VMS status = %d ]\n", string, status);
    else
    {
	msgbuf[msglen] = 0;
	fprintf(stderr, "%s[ %s ]\n", string, msgbuf);
    }
}


#endif				/* !ZIPINFO */
#endif				/* VMS */
