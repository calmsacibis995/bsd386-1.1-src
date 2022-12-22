/*
 * $Header: /bsdi/MASTER/BSDI_OS/usr.sbin/sa/user.c,v 1.1.1.1 1992/09/25 19:11:42 trent Exp $
 * $Log: user.c,v $
 * Revision 1.1.1.1  1992/09/25  19:11:42  trent
 * Initial import of sa from andy@terasil.terasil.com (Andrew H. Marrinson)
 *
 * Revision 1.2  1992/05/12  18:02:35  andy
 * Changed RCS ids.
 *
 */
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include "sa.h"


int
dump_user_command (const struct acct *record)

{
  static long last_user_id = -1;
  static char user[40];

  if (record->ac_uid != last_user_id)
    {
      struct passwd *pw = getpwuid (record->ac_uid);

      if (pw)
	{
	  strncpy (user, pw->pw_name, sizeof (user));
	  user[sizeof (user) - 1] = 0;
	}
      else
	sprintf (user, "%d", record->ac_uid);
    }
  printf ("%-10.10s %s\n", record->ac_comm, user);
  return TRUE;
}
