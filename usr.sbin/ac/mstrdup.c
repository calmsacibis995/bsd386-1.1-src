#include "ac2.h"

char *
mstrdup(char *s)
{
  char *new_s;

  if (s != NULL)
    {
      new_s = (char *) malloc(strlen(s) +1);
    } else {
      new_s = NULL;
    }

  if (new_s != NULL)
    strcpy(new_s, s);

  return(new_s);
}


