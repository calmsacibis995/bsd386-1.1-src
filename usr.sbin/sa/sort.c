/*
 * $Header: /bsdi/MASTER/BSDI_OS/usr.sbin/sa/sort.c,v 1.1.1.1 1992/09/25 19:11:41 trent Exp $
 * $Log: sort.c,v $
 * Revision 1.1.1.1  1992/09/25  19:11:41  trent
 * Initial import of sa from andy@terasil.terasil.com (Andrew H. Marrinson)
 *
 * Revision 1.2  1992/05/12  18:02:35  andy
 * Changed RCS ids.
 *
 */
#include <stdlib.h>
#include <sysexits.h>
#include "sa.h"


#define SORT_GROW 64
static const void **sort_table = NULL;
static size_t sort_table_count = 0;
static size_t sort_table_size = 0;


void
add_to_sort_table (const void *entry)

{
  if (sort_table_count == sort_table_size)
    {
      sort_table_size += SORT_GROW;
      sort_table = realloc (sort_table, sort_table_size * sizeof (entry));
      if (sort_table == NULL)
	fatal_error (NULL, "Out of memory.", EX_SOFTWARE);
    }
  sort_table[sort_table_count++] = entry;
}

void
iterate_sort_table (int (*compare) (const void *, const void *),
		    int (*func) (const void *))

{
  int i;

  qsort (sort_table, sort_table_count, sizeof (void *), compare);
  for (i = 0; i < sort_table_count; ++i)
    if (! func (sort_table[i]))
      break;
}
