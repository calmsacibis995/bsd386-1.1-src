#include "ac2.h"

void
do_tty_list(char *line, char *name, int a_time)
{
  tty_list current = &t_list;
  int counter;		/* should be TRUE if match is found */
  int t_time;			/* totel time logged in */

  counter = FALSE;
  t_time = 0;
    
#ifndef FTP
  if (!strcmp(name, "ftp")) {
    return;
  }
#endif		/* if not FTP */

#ifndef LOGIN
  if (!strcmp(name, "LOGIN"))
    {
      return;
    }
#endif

  if (!strcmp(line, "~")) 
    {
      update_times(a_time);
      return;
    }

  if (name == NULL || !strcmp(name, "\0")) 
    {
    while (current->next != NULL)
      {
	if (current->line != NULL && strncmp(line, current->line, 8) == 0) 
	  {
	    counter = TRUE;
	    if (current->time_out == 0) 
	      {
		current->time_out = a_time;
	      }
	    break;
	  }		/* check 8 characters of the line, because of ftp */
	current = current->next;
      }			/* set the logout time, upto the second entry */
    if (!counter && strncmp(line, current->line, 8) == 0)
      {
	if (current->time_out == 0)
	  {
	    current->time_out = a_time;
	  }
      }			/* because of fence-posts, do the first entry */
  } else {		/* there is a name */
    while (current->next != NULL)
      {
	if (current->line != NULL && strncmp(line, current->line, 8) == 0) 
	  {
	    counter = TRUE;
	    if (current->time_out == 0)
	      {
		current->time_out = a_time;
	      }
	    break;
	  }
	current = current->next;
      }			/* step to the previous occurance of
			   the tty.  If it has not been logged out,
			   log it out now. */
    if (!counter && strncmp(line, current->line, 8) == 0)
      {
	if (current->time_out == 0)
	  {
	    current->time_out = a_time;
	  }
      }
    make_tty_node(&t_list, line, name, a_time);
  }

}



void
  make_tty_node(tty_list head_list, char *line, char *name, int a_time)
{
  tty_list temp;
  tty_list new;
  
  temp = head_list->next;
  
  new = (tty_list) malloc(sizeof(struct ttylist));
/*  if (new->line != NULL)
    free(new->line);*/
  new->line = mstrdup(line);
/*  if (new->name != NULL)
    free(new->name);*/
  new->name = mstrdup(name);
  new->time_in = a_time;
  new->time_out = 0;
  
  head_list->next = new;
  head_list->next->next = temp;
  
}


