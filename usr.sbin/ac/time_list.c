#include "ac2.h"

void
do_time_list(char *name, int this_midnight, int a_time)
{
  time_list current = &ti_list;
  int condition;

  condition = 0;

  while (current->next != NULL)
    {
      if (current->name != NULL && strncmp(name, current->name, 9) == 0
	  && current->midnight == this_midnight)
	{
	  condition = 1;
	  break;
	}
      else if (current->midnight != this_midnight)
	{
	  condition = 2;
	  break;
	}
      current = current->next;
    }		/* search through until we reach either the first entry in the list */

  /* right now we have one of the following conditions:
         0) we are on the first entry of the list
	 1) we are on tomorrow's midnight
	 2) current->name exists and is a match
   */

/*  if (current->midnight == this_midnight)
    {
      if (current->next != NULL
	  || ( current->name != NULL
	      && strncmp(name, current->name, 9) != 0))
	{
	  current->total += a_time;
	}
    }
  else
    make_time_node(&ti_list, name, this_midnight, a_time);*/
  switch (condition)
    {
    case 0:
      if (current->name == NULL
	  || strncmp(name, current->name, 9) != 0)
	{
	  make_time_node(&ti_list, name, this_midnight, a_time);
	  break;
	}
    case 1:
      current->total += a_time;
      break;
    case 2:
      make_time_node(&ti_list, name, this_midnight, a_time);
      break;
    default:
      break;
    }
}



void
make_time_node(time_list head_list, char *name, int this_midnight, int a_time)
{
  time_list temp;
  time_list new;

  temp = head_list->next;
  
  new = (time_list) malloc(sizeof(struct timelist));
/*  if (new->name != NULL)
    free(new->name);*/
  new->name = mstrdup(name);
  new->total = a_time;
  new->midnight = this_midnight;
  
  head_list->next = new;
  head_list->next->next = temp;
}


  
