#include "ac2.h"

/* we need to make as many passes through the tty_list as we have midnights. */

#define SECONDS_IN_DAY 86400
#define FINISHED_NODE -1

int
make_time_list(int this_midnight)
{
  tty_list current = &t_list;
  int time_in;
  int t_time;

  t_time = 0;
  time_in = this_midnight;

  /* run through current until either
          1) we get a current->time_out that is less than time_in
	  2) we get to the end of the list
   */

  while (current->next != NULL)
    {
      if (current->name != NULL && (strcmp(current->name, "\0") != 0)) 
	{
	  if (current->time_in != FINISHED_NODE && current->time_out > this_midnight)
	    {
	      if (time_in < current->time_in)
		{
		  t_time = current->time_out - current->time_in;
		  /* at this point, we are done with this entry, and should delete
		     the node */
		  current->time_in = FINISHED_NODE;
		}
	      else
		{
		  t_time = current->time_out - time_in;
		  current->time_out = this_midnight;
		}
	      do_time_list(current->name, this_midnight, t_time);
	    }	/* only do the work if the node is not finished */
	}
      current = current->next;
    }

  if (current->next == NULL)
    {
      if (current->time_in != FINISHED_NODE && current->time_out >= this_midnight)
	{
	  /* do the last entry */
	  if (time_in < current->time_in)
	    {
	      t_time = current->time_out - current->time_in;
	      current->time_in = FINISHED_NODE;
	    }
	  else
	    {
	      t_time = current->time_out - time_in;
	      current->time_out = time_in;
	    }
	  do_time_list(current->name, this_midnight, t_time);
	}		/* if the person logged out today, do the work */
      else if (current->time_in == FINISHED_NODE)
	{
	  return TRUE;
	}
    }		/* do the very first entry */

  if (current->time_in != FINISHED_NODE && current->time_in < time_in)
    {
      make_time_list(this_midnight - SECONDS_IN_DAY);
    }
  
  return TRUE;
}
