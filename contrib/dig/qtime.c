
/*
** Written with 'dig' version 2.0 from University of Southern
** California Information Sciences Institute (USC-ISI). 9/1/90
*/

#include "qtime.h"

static int qptr = 0;
struct timelist _qtime[HMAXQTIME];
struct timeval hqtime;
u_short hqcount, hxcount;

struct timeval
*findtime(id)
     u_short id;
{
int i;
  for (i=0; i<HMAXQTIME; i++)
    if (_qtime[i].id == id)
      return(&(_qtime[i].time));
  return(NULL);
}


savetime(id,t)
     u_short id;
     struct timeval *t;
{
qptr = ++qptr % HMAXQTIME;
_qtime[qptr].id = id;
_qtime[qptr].time.tv_sec = t->tv_sec;
_qtime[qptr].time.tv_usec = t->tv_usec;
}


struct timeval
*difftv(a,b,tmp)
     struct timeval *a, *b, *tmp;
{
  tmp->tv_sec = a->tv_sec - b->tv_sec;
  if ((tmp->tv_usec = a->tv_usec - b->tv_usec) < 0) {
    tmp->tv_sec--;
    tmp->tv_usec += 1000000;
  }
return(tmp);
}


prnttime(t)
     struct timeval *t;
{
printf("%u msec ",t->tv_sec * 1000 + (t->tv_usec / 1000));
}

