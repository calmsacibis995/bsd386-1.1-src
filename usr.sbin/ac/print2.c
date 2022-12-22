#include "ac2.h"
  
/* print the name list and the totals */
  
void
print_name(int argc, int name_num, char **names, int p_total)
{
  int t_time;
  int a_counter;
  int n_counter;
  name_list current = &n_list;
  name_list temp = &n_list;
  
  t_time = 0;
  a_counter = argc - 1;
  
  if (argc > name_num) 
    {
      for (n_counter = name_num; n_counter <= a_counter; n_counter++)
	{
	  while (current->next != NULL 
		 && (strncmp(names[n_counter], current->name, 9)) != 0) 
	    {
	      current = current->next;
	    }
	
	  if (!strncmp(names[n_counter], current->name, 9)) 
	    {
	      if (print_names_flag) 
		{
		  printf("\t%-8s%8.2f\n", current->name, 
			 (float) (current->time / (60.0 * 60.0)));
		}
	      t_time += current->time;
	    }
	  current = temp->next;
	}
    }
  else 
    {				/* no names specified */
      while (current->next != NULL) 
	{
	  if (current->name != NULL && (strcmp(current->name, "\0") != 0)) 
	    {
	      if (print_names_flag) 
		{
		  printf("\t%-8s%8.2f\n", current->name, 
			 (float) (current->time / (60.0 * 60.0)));
		}
	      t_time += current->time;
	    }
	  current = current->next;
	}
      
      if (current->name != NULL && (strcmp(current->name, "\0") != 0)) 
	{
	  if (print_names_flag) 
	    {
	      printf("\t%-8s%8.2f\n", current->name, 
		     (float) (current->time / (60.0 * 60.0)));
	    }	
	  t_time += current->time;
	}
    }
  
  if (p_total)
    printf("\t%-8s%8.2f\n", "total", (float) (t_time / (60.0 * 60.0)));
  
}



#define SECONDS_IN_DAY 86400

/* print time list and totals */

void
print_time(int argc, int name_num, char **names)
{
  time_list current = &ti_list;
  int t_time;
  int this_midnight;
  char day[8];
  int a_counter;
  int n_counter;
  
  t_time = 0;
  this_midnight = 0;
  a_counter = argc - 1;

  reset_name_list();

  while (current->next != NULL)
    {
      if (current->name && (strcmp(current->name, "\0") != 0))
	{
	  if (print_names_flag)
	    {
	      do_name_list(current->name, current->total);
	    }
	  
	  if (argc > name_num)
	    {
	      for (n_counter = name_num; n_counter <= a_counter; n_counter++) 
		{
		  if (current->name && (strncmp(names[n_counter], current->name, 9) == 0))
		    {
		      t_time += current->total;
		    }
		}
	    }
	  else
	    {
	      t_time += current->total;
	    }
	  
	  if (current->midnight != current->next->midnight)
	    {
	      
	      if (print_names_flag) 
		{
		  if (argc > name_num) 
		    {
		      print_name(argc, name_num, names, FALSE);
		    }
		  else
		    {
		      print_name(0, 0, (char **) NULL, FALSE);
		    }
		  reset_name_list();
		}
	      
	      day_day(day, (long) (current->midnight + 1));
	      if (t_time)
		{
		  printf("%-8s%-8s%8.2f\n", day, "total", (float) (t_time / 3600.0));
		}
	      t_time = 0;
	    }
	}
      current = current->next;
    }
  
  if (print_names_flag) 
    {
      do_name_list(current->name, current->total);
      if (argc > name_num) {
	print_name(argc, name_num, names, FALSE);
      }
      else
	{
	  print_name(0, 0, (char **) NULL, FALSE);
	}
      reset_name_list();
    }
  if (argc > name_num)
    {
      for (n_counter = name_num; n_counter <= a_counter; n_counter++) 
	{
	  if (current->name && (strncmp(names[n_counter], current->name, 9) == 0))
	    {
	      t_time += current->total;
	    }
	}
    }
  else
    {
      t_time += current->total;
    }
  day_day(day, (time_t) (current->midnight));
  if (t_time)
    {
      printf("%-8s%-8s%8.2f\n", day, "total", (float) (t_time / 3600.0));
    }
}

#include <time.h>

static char *month_name[12] = {
  "Jan",
  "Feb",
  "Mar",
  "Apr",
  "May",
  "June",
  "July",
  "Aug",
  "Sept",
  "Oct",
  "Nov",
  "Dec"
  };


/* determine the month and the day of the month */

int
day_day(char *day, time_t a_time)
{
  struct tm *time_struct;
  
  time_struct = (struct tm *) malloc(sizeof( struct tm));
  time_struct = localtime(&a_time);
  
  sprintf(day, "%-4s %2d", month_name[time_struct->tm_mon], time_struct->tm_mday);
  
  return 0;
}


