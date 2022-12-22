#include "ac2.h"
#include <time.h>

int
find_midnight(time_t a_time)
{
  struct tm *time_struct;
  long this_midnight;

  this_midnight = 0;

  time_struct = (struct tm *) malloc(sizeof( struct tm));
  time_struct = localtime(&a_time);
  
  this_midnight = a_time - ((time_struct->tm_hour * 3600) + 
			    (time_struct->tm_min * 60) + time_struct->tm_sec);

  return ((int) this_midnight);
}
