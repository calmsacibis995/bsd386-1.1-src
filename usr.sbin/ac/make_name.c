#include "ac2.h"
  
void
make_name_list()
{
  tty_list current = &t_list;
  int t_time;

  while(current->next != NULL)
    {
      t_time = current->time_out - current->time_in;
      do_name_list(current->name, t_time);
      current = current->next;
    }

  t_time = current->time_out - current->time_in;
  do_name_list(current->name, t_time);
}
