#include <stdio.h>

void main (void)
{
  signed char in;
  unsigned char out;
  
  while (!feof (stdin))
    {
      fread (&in, sizeof (in), 1, stdin);
      out = in + 128;
      fwrite (&out, sizeof (out), 1, stdout);
    }
}
