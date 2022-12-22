#include <unistd.h>

int
setegid(gid_t gid)
{
    return (setresgid(-1, gid, -1));
}
