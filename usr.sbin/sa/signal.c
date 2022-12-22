/*
 * $Header: /bsdi/MASTER/BSDI_OS/usr.sbin/sa/signal.c,v 1.1.1.1 1992/09/25 19:11:41 trent Exp $
 * $Log: signal.c,v $
 * Revision 1.1.1.1  1992/09/25  19:11:41  trent
 * Initial import of sa from andy@terasil.terasil.com (Andrew H. Marrinson)
 *
 * Revision 1.2  1992/05/12  18:02:35  andy
 * Changed RCS ids.
 *
 */
#include <signal.h>
#include "sa.h"


void
trap_signals (int *signals, void (*func) (void))

{
  int i;

  for (i = 0; signals[i] != 0; ++i)
    {
      struct sigaction new_action;
      struct sigaction old_action;

      new_action.sa_handler = SIG_IGN;
      sigfillset (&new_action.sa_mask);
      new_action.sa_flags = SA_RESTART|SA_NOCLDSTOP;
      (void) sigaction (signals[i], &new_action, &old_action);
      if (old_action.sa_handler != SIG_IGN)
	{
	  new_action.sa_handler = terminate;
	  (void) sigaction (signals[i], &new_action, NULL);
	}
    }
}
