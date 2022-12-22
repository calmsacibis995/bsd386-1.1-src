#ifndef lint
static char rcsid[] = "$Id: printer.c,v 1.2 1992/09/03 00:02:04 polk Exp $";
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include "kernel.h"

#ifndef BYTE
typedef unsigned char byte;
typedef unsigned short word;
#define BYS (byte *)
#endif
#define BYTE

#define MAKE_DD_ADDR(dd_adr)  BYS((dd_adr & 0xFFFF) + ((dd_adr >> 12) & 0xFFFF0))

/*************************************************************/

extern struct connect_area *connect_area;
extern struct device devices[];
extern int ndevices;

/*************************************************************/

#define PARALL_PORT_1 0x378
#define PARALL_PORT_2 0x278
#define PARALL_PORT_3 0x3BC

#define N_OF_LPT 3
#define SPOOL_BUF_SIZE 0x8000

/*************************************************************/

static int paral_port_bases [N_OF_LPT] = {
  PARALL_PORT_1,
  PARALL_PORT_2,
  PARALL_PORT_3
  };

struct SPOOLBUF {    /* the same structure as in V86 ! */
  word max_i;
  word cur_i;
  word b_lock;
  char bu[SPOOL_BUF_SIZE];
};

struct PARAL_data {
  char *name;
  int file;
  int ass_flag;
  struct SPOOLBUF *spoolbuf;
  byte data;
  byte status;
  byte ctrl;
};

/* assign flag: 0 - not assigned
 *              1 - assigned to a plane file
 *              2 - assigned to a special file (device)
 *              3 - assigned as spool print
 */

static struct PARAL_data lpt_data [N_OF_LPT];

static char sys_string[200] = { 0 };

/*******************************************************/
/* Note ! Real lines polarity on printer may differ from described below */

/* port 0x378 -- printer data latch (read/write) */
/*   read: fetch last byte sent */

/* port 0x379 -- printer status (read only)
 *  ...__ means inverted line
 *  (x/y-z) - pins defined by IBM / real pins on EPSON-1050 (in-out)
 */

#define P_ERROR   0x08	/* 0 -- printer signals an error (15/32) */
#define P_SLCT    0x10	/* 1 -- printer is selected (13/?) */
#define P_PAPER   0x20	/* 1 -- out of paper (12/12-30) */
#define P_ACK     0x40	/* 0 -- ready for next char (12 mks pulse) (10/10-28)*/
#define P_BUSY    0x80	/* 0 -- busy or offline or error (11/11-29) */

#define P_STAT_USE 0xF8

/* port 0x37A -- printer controls (read/write)
 *  ...__ means inverted line
 *  (x/y-z) - pins defined by IBM / real pins on EPSON-1050 (in-out)
 */

#define P_STROBE__   0x01   /* 1 -- send strobe ( >0.5 mks pulse) (1/1-19) */
#define P_AUTO_LF__  0x02   /* 1 -- causes LF after CR (14/14) */
#define P_INIT       0x04   /* 0 -- resets the printer (>50 mks pulse)(16/31)*/
#define P_SLCT_IN__  0x08   /* 1 -- selects the printer (17/36) */
#define P_IRQ_ENABLE 0x10   /* 1 -- enables IRQ when -ACK goes false */

#define P_CTRL_USE 0x1F

#define N_OF_PARAL_REGS 3

/*************************************************************/
void PrinterService (byte cod, word prt_n);
/*************************************************************/

static void PrintOut (int port, byte val)
{
   int k;
   struct SPOOLBUF *b_a;

#ifdef DEBUG
#ifdef DEB_PRNT
   dprintf ("PRINTER: out to port %2.2X, val = %2.2X\n", port, val);
#endif
#endif

   for ( k=0 ; k<N_OF_LPT ; ++k )
     if ( (port - paral_port_bases[k]) < N_OF_PARAL_REGS ) {
       port -= paral_port_bases[k];
       break;
     }

   switch (port) {
   case 0:             /* 378 - printer data */
     lpt_data[k].data = val;
     break;

   case 2:             /* 37A - printer control */
     if ( (val & P_INIT) == 0 ) {     /* initialize */
       PrinterService (1, k);   /* initialize printer */
     }
     else {
       if ( val & P_SLCT_IN__ ) {
	 lpt_data[k].status |= P_SLCT;
	 lpt_data[k].ctrl |= P_SLCT_IN__;
       }
       else {
	 lpt_data[k].status &= ~P_SLCT;
	 lpt_data[k].ctrl &= ~P_SLCT_IN__;
       }
       if ( (val & P_STROBE__)  &&  (lpt_data[k].status & P_BUSY) ) {
	 /* send data if not busy */
	 b_a = (struct SPOOLBUF *)MAKE_DD_ADDR(connect_area->addr_buf_lpt[k]);
	 /* syncronous call -- no need of locking ! */
	 b_a->bu[b_a->cur_i++] = lpt_data[k].data;
	 if ( b_a->cur_i == b_a->max_i  ||  lpt_data[k].data == 0x0A ) {
	   PrinterService (0, k);   /* transfer buffer */
	   b_a->cur_i = 0;
	 }
	 lpt_data[k].status &= ~P_BUSY; /* busy when send STROBE */
       }
       if (! (val & P_STROBE__)) {
	 lpt_data[k].status |= P_BUSY; /* NOT busy when remove INIT & STROBE */
       }
       /* auto LF -- ignored !!! */
       /* IRQ -- not implemented !!! (as on most real adapters !) */
     }
     break;
   }
}

static byte PrintIn (int port)
{
   byte rs;
   int k;

   for ( k=0 ; k<N_OF_LPT ; ++k )
     if ( (port - paral_port_bases[k]) < N_OF_PARAL_REGS ) {
       port -= paral_port_bases[k];
       break;
     }

   switch (port) {
   case 0:             /* 378 - printer data */
     rs = lpt_data[k].data;
     break;
   case 1:             /* 379 - printer status */
     rs = lpt_data[k].status;
     break;
   case 2:             /* 37A - printer control */
     rs = lpt_data[k].ctrl;
     break;
   }
#ifdef DEBUG
#ifdef DEB_PRNT
   dprintf ("PRINTER: in from port %2.2X, val = %2.2X\n", port, rs);
#endif
#endif

   return rs;
}

/*************************************************************/
static int ttailn = 0;

static void GenSpoolName (char *str)
{
  int k;

  strcpy (str, "/tmp/rundos.print.");
  k = strlen (str);
  sprintf (str + k, "%4.4i", ttailn++);
  str[k+4] = 0;
}

void init_printer (void)
{
   int j, k;
   struct device *dp;
   struct stat stat_buf;

   for ( k=0 ; k<N_OF_LPT ; ++k ) {
     if ( (dp = search_device (DEV_LPT, k+1)) == NULL )
       lpt_data[k].ass_flag = 0;
     else {
       lpt_data[k].name = dp->dev_real_name;
       if (strcmp (lpt_data[k].name, "spool") == 0) {
	 lpt_data[k].ass_flag = 3;
	 /* must create temporary file !!! */
	 GenSpoolName (lpt_data[k].name);
	 if ( (lpt_data[k].file =
	       open (lpt_data[k].name, O_WRONLY|O_TRUNC|O_CREAT, 0666)) == -1 )
	   ErrExit (0x17,"Can't open %s assigned to LPTx",lpt_data[k].name);
	 lpt_data[k].spoolbuf =
	   (struct SPOOLBUF *) malloc (sizeof(struct SPOOLBUF));
	 lpt_data[k].spoolbuf->max_i = SPOOL_BUF_SIZE;
	 lpt_data[k].spoolbuf->cur_i = 0;
       }
       else {
	 stat_buf.st_mode = S_IFREG;
	 stat (lpt_data[k].name, &stat_buf);
	 if (stat_buf.st_mode & S_IFREG) {  /* file regular or not found */
	   if ( (lpt_data[k].file =
	      open (lpt_data[k].name, O_WRONLY|O_TRUNC|O_CREAT, 0666)) == -1 )
	     ErrExit (0x17,"Can't open %s assigned to LPTx",lpt_data[k].name);
	   lpt_data[k].ass_flag = 1;   /* regular file */
	 }
	 else {
	   if ( (lpt_data[k].file =
		 open (lpt_data[k].name, O_WRONLY, 0666)) == -1 )
	     ErrExit (0x17,"Can't open %s assigned to LPTx",lpt_data[k].name);
	   lpt_data[k].ass_flag = 2;   /* special file */
	 }
       }
#ifdef DEBUG
       dprintf ("INIT PRINT: num %.2X, name %s, file %.4X, flag %.2X\n",
		k, lpt_data[k].name, lpt_data[k].file, lpt_data[k].ass_flag);
#endif
     }
   }
   connect_area->num_of_printers = 0;
   for ( k=0 ; k<N_OF_LPT ; ++k ) {
     if (lpt_data[k].ass_flag != 0) {
       for ( j=0 ; j<N_OF_PARAL_REGS ; ++j ) {
	 define_in_port (paral_port_bases[k]+j, PrintIn);
	 define_out_port (paral_port_bases[k]+j, PrintOut);
       }
       lpt_data[k].data = 0;
       /* selected, no errors, not busy */
       lpt_data[k].status = (P_SLCT | P_BUSY);
       /* selected, no IRQ, no auto LF */
       lpt_data[k].ctrl = (P_INIT | P_SLCT_IN__);
       ++connect_area->num_of_printers;
     }
     else
       connect_area->addr_buf_lpt[k] = 0;
   }
}

/*************************************************************/

void WriteSpoolBuf (word prt_n)
{
  int k;

  if ( (k = write (lpt_data[prt_n].file, lpt_data[prt_n].spoolbuf->bu,
		   lpt_data[prt_n].spoolbuf->cur_i))
      != lpt_data[prt_n].spoolbuf->cur_i )
    /* error handling by V86 -- may be bad nesting ! */
    ErrorTo86 (" Write printer spool file -- ERROR ");
  lpt_data[prt_n].spoolbuf->cur_i = 0;

#ifdef DEBUG
#ifdef DEB_PRNT
  dprintf ("PRINTER: write file %.2X, bytes: %.4X (%.4X), errno: %i\n",
	   lpt_data[prt_n].file,k,lpt_data[prt_n].spoolbuf->cur_i,errno);
#endif
#endif
}

void PrinterService (byte cod, word prt_n)
{
  struct SPOOLBUF *b_a;
  int k, sm, save_errno;

  if (prt_n > N_OF_LPT  ||  lpt_data[prt_n].ass_flag == 0)
    return;
  switch (cod) {
  case 0:         /* 0 - transfer buffer */
    b_a = (struct SPOOLBUF *)MAKE_DD_ADDR(connect_area->addr_buf_lpt[prt_n]);
    if ( lpt_data[prt_n].ass_flag == 1  ||  lpt_data[prt_n].ass_flag == 2 ) {
      sm = 0;
      do {
	k = write (lpt_data[prt_n].file, b_a->bu + sm, b_a->cur_i - sm);
	if (k == -1) {
	  if (errno == EINTR)
	    k = 0;
	  else {
	    char err_buf [80];
	    sprintf (err_buf," Write to printer -- ERROR #%d %c",errno,'\0');
	    ErrorTo86 (err_buf);
	    break;
	  }
	}
	sm += k;
      } while ( sm != b_a->cur_i );
#ifdef DEBUG
#ifdef DEB_PRNT
      dprintf ("PRINTER: write file %.2X, bytes: %.4X (%.4X), errno: %i\n",
	       lpt_data[prt_n].file,k,b_a->cur_i,errno);
#endif
#endif
    }
    if (lpt_data[prt_n].ass_flag == 3  &&  b_a->b_lock == 0 ) {
      if (lpt_data[prt_n].spoolbuf->cur_i + b_a->cur_i >
	  lpt_data[prt_n].spoolbuf->max_i)
	WriteSpoolBuf (prt_n);
      memcpy (lpt_data[prt_n].spoolbuf->bu+lpt_data[prt_n].spoolbuf->cur_i,
	      b_a->bu, b_a->cur_i);
      lpt_data[prt_n].spoolbuf->cur_i += b_a->cur_i;
    }
    break;
  case 1:         /* 1 - "initialize" */
    b_a = (struct SPOOLBUF *)MAKE_DD_ADDR(connect_area->addr_buf_lpt[prt_n]);
    b_a->cur_i = 0;
    b_a->b_lock = 0;
    if (lpt_data[prt_n].ass_flag == 3) {   /* if spool print */
      lpt_data[prt_n].spoolbuf->cur_i = 0;
      (void) lseek (lpt_data[prt_n].file, 0L, SEEK_SET);
    }
    break;
  }
}

void PrintBufFlash (void)
{
  int j;
  /* the only place where (or in calls from where)
      syncronization may be needed */

  for ( j=0 ; j<N_OF_LPT ; ++j ) {
    if (lpt_data[j].ass_flag == 3) {
      PrinterService (0, j);    /* at first flush V86 internal buffer */
      WriteSpoolBuf (j);        /* then write monitor's buffer to a file */
      close (lpt_data[j].file);
      strcpy (sys_string, "lpr ");
      strcat (sys_string, lpt_data[j].name);
      strcat (sys_string, " &");
      system (sys_string);      /* and print the file */
      GenSpoolName (lpt_data[j].name);
      if ( (lpt_data[j].file =
	    open (lpt_data[j].name, O_WRONLY|O_TRUNC|O_CREAT, 0666)) == -1 )
	; /* ErrorTo86 ("..."); -- bad nesting !*/
      /* error handling by V86 */
      /* error handling !!! */
    }
  }
}

/*************************************************************/

static byte GameIn (int port)
{
  return 0xF0;
}

static void GameOut (int port, byte val)
{
  return;
}

void init_game_port (void)
{
  define_in_port (0x201, GameIn);
  define_out_port (0x201, GameOut);
}
