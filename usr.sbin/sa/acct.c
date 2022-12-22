/* 
 * $Header: /bsdi/MASTER/BSDI_OS/usr.sbin/sa/acct.c,v 1.6 1993/03/04 00:58:46 sanders Exp $
 * $Log: acct.c,v $
 * Revision 1.6  1993/03/04  00:58:46  sanders
 * Ok, this time for sure.  The correct acct.c file.
 *
 * Revision 1.5  1993/03/03  21:33:13  sanders
 * back to standard
 *
 * Revision 1.4  1993/03/03  19:44:04  sanders
 * I forgot to fix the undecl var
 *
 * Revision 1.3  1993/03/03  19:40:20  sanders
 * fixes to copy accounting file and not zero
 *
 * Revision 1.2  1993/02/28  18:04:25  sanders
 * fixed memory usage bug
 *
 * Revision 1.1.1.1  1992/09/25  19:11:41  trent
 * Initial import of sa from andy@terasil.terasil.com (Andrew H. Marrinson)
 *
 * Revision 1.3  1992/12/28  20:04:03  andy
 * No longer zeroes accounting file when creating summaries.
 *
 * Revision 1.2  1992/05/12  18:02:35  andy
 * Changed RCS ids.
 *
 */
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <unistd.h>
#include "sa.h"


static void
iterator (const char *file, size_t size,
	  bool_t must_exist, int (*func) (const void *))

{
  FILE *f = fopen (file, "r");

  if (f != NULL)
    {
      void *record = alloca (size);	/* assume success? XXX */

      for ( ; ; )
	{
	  if (fread (record, size, 1, f) == 1)
	    {
	      if (func (record))
		continue;
	      fclose (f);
	      return;
	    }
	  if (feof (f))
	    {
	      fclose (f);
	      return;
	    }
	  fatal_error (file, "Read error.", EX_IOERR);
	}
    }
  if (errno == ENOENT && ! must_exist)
    return;
  fatal_error (file, NULL, EX_NOINPUT);
}

void
iterate_accounting_file (const char *file, int (*func) (const struct acct *))

{
  iterator (file, sizeof (struct acct), TRUE, (int (*) (const void *)) func);
}

void
iterate_user_summary (int (*func) (const struct acctusr *))

{
  iterator (user_summary_path, sizeof (struct acctusr),
	    FALSE, (int (*) (const void *)) func);
}

void
iterate_command_summary (int (*func) (const struct acctcmd *))

{
  iterator (command_summary_path, sizeof (struct acctcmd),
	    FALSE, (int (*) (const void *)) func);
}

comp_t
comp_add (comp_t left, comp_t right)

{
  int ml = left & 0x1fff;
  int mr = right & 0x1fff;
  int el = left >> 13;
  int er = right >> 13;

  if (el < er)
    {
      do
	{
	  ml >>= 3;
	  ++el;
	}
      while (el < er);
    }
  else if (er < el)
    {
      do
	{
	  mr >>= 3;
	  ++er;
	}
      while (er < el);
    }
  ml += mr;
  if (ml & ~ 0x1fff)
    {
      if (el == 7)
	return 0xffff;
      ml >>= 3;
      ++el;
    }
  return (ml & 0x1fff) | (el << 13);
}

#ifndef __GNUC__
double
comp_conv (comp_t c)

{
  return ldexp (c & 0x1fff, c >> 13);
}
#endif

#ifdef notyet
void
remerge_accounting_files (void)

{
}
#endif
