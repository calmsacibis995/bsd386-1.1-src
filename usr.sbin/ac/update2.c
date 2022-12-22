#include "ac2.h"


    /* every entry in the tty_list which has a time entry needs
       to do_name_list(current->name, (a_time - current->time));
       We need to do this because the machine has just decided
       to shutdown or reboot.
       */

void
update_times(int a_time)
{
  tty_list current = &t_list;
  int t_time;
  
  t_time = 0;
  
  while (current->next != NULL) 
    { 
      if (current->time_out == 0) 
	{
	  current->time_out = a_time;
	}
      current = current->next;
    }
  
  if (current->time_out == 0) 
    {
      current->time_out = a_time;
    }
}


