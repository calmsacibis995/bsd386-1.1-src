
/*
** Written with 'dig' version 2.0 from University of Southern
** California Information Sciences Institute (USC-ISI). 9/1/90
*/

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>

#define HMAXQTIME 20

struct timelist {
  u_short id;
  struct timeval time;
};


extern struct timelist _qtime[];
extern struct timeval  hqtime;
extern struct timeval  *difftv();
extern u_short hqcount, hxcount;
