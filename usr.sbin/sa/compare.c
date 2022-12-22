/* 
 * $Header: /bsdi/MASTER/BSDI_OS/usr.sbin/sa/compare.c,v 1.1.1.1 1992/09/25 19:11:41 trent Exp $
 * $Log: compare.c,v $
 * Revision 1.1.1.1  1992/09/25  19:11:41  trent
 * Initial import of sa from andy@terasil.terasil.com (Andrew H. Marrinson)
 *
 * Revision 1.2  1992/05/12  18:02:35  andy
 * Changed RCS ids.
 *
 */
#include "sa.h"


#define SETUP if (! reverse_sort) { const void *t = left; left = right; right = t; }


#define DECLARE(N) static int user_##N (const void *left, const void *right)
#define L(F) ((* (struct acctusr **) left)->F)
#define R(F) ((* (struct acctusr **) right)->F)
#include "compare.i"
#undef DECLARE
#undef L
#undef R


#define DECLARE(N) static int command_##N (const void *left, const void *right)
#define L(F) ((* (struct acctcmd **) left)->F)
#define R(F) ((* (struct acctcmd **) right)->F)
#include "compare.i"
#undef DECLARE
#undef L
#undef R


int (*user_compare_funcs[NSMODE]) (const void *, const void *) = {
  user_total_time, user_average_time, user_average_disk,
  user_total_disk, user_average_memory, user_kcore_secs,
  user_call_count,
};

int (*command_compare_funcs[NSMODE]) (const void *, const void *) = {
  command_total_time, command_average_time, command_average_disk,
  command_total_disk, command_average_memory, command_kcore_secs,
  command_call_count,
};
