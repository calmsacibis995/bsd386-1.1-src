#include "ac2.h"

void
do_name_list(char *name, int a_time)
{
  name_list current = &n_list;
  
  int counter;		/* should be TRUE if match is found */
  
  counter = FALSE;
  
  while (current->next != NULL 
	 && current->name != NULL 
	 && (strncmp(name, current->name, 9)) != 0) 
    {
      current = current->next;
    }
  
  if (current->next != NULL 
      || (current->name != NULL 
	  && ( !strcmp(name, current->name)))) 
    {
      current->time += a_time;
    } else {
      make_name_node(&n_list, name, a_time);
    }
}    



void
make_name_node(name_list head_list, char *name, int a_time)
{
  name_list temp;
  name_list new;
  
  temp = head_list->next;
  
  new = (name_list) malloc(sizeof(struct namelist));
/*  if (new->name != NULL)
    free(new->name);*/
  new->name = mstrdup(name);
  new->time = a_time;
  
  head_list->next = new;
  head_list->next->next = temp;
}



void
reset_name_list()
{

    /* set everything in name_list to NULL */
    n_list.next = NULL;
    n_list.name = mstrdup("\0");
    n_list.time = 0;
}


