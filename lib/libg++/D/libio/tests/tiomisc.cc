/* Random regression tests etc. */

#include <fstream.h>
#include <stdio.h>
#include <strstream.h>

#define BUF_SIZE 4096

void
test1 ()
{
   fstream f;
   char    buf[BUF_SIZE];

   f.setbuf( buf, BUF_SIZE );
}

void
test2 ( )
{
   char string[BUF_SIZE];
   ostrstream s( string, BUF_SIZE );

   s << "Bla bla bla " << 55 << ' ' << 3.23 << '\0' << endl;
   cout << "Test2: " << string << endl;
}

int main( )
{
  test1 ();
  test2 ();
  return 0;
}
