#include <iostdio.h>

void
t1 ()
{
  int n = -1;
  sscanf ("abc  ", "abc %n", &n);
  printf ("t1: count=%d\n", n);
}

int
main ()
{
  t1 ();
  return 0;
}
