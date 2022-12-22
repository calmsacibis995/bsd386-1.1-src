#include <stdio.h>

#define BUFFER_SIZE	0x2000
  
void main (void)
{
  signed char in[BUFFER_SIZE];
  unsigned char out[BUFFER_SIZE];
  int i, got;
  
  while (!feof (stdin))
    {
      got = fread (in, 1, BUFFER_SIZE, stdin);
      for (i = 0; i < got; i++)
	out[i] = in[i] + 128;
      fwrite (out, 1, got, stdout);
    }
}
