#include <fcntl.h>

int
mkstemp(char* fpat)
{
    extern char *mktemp();

    return (open(mktemp(fpat), O_RDWR|O_CREAT, 0644));
}
