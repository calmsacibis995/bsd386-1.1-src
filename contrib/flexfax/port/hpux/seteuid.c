#include <unistd.h>

int
seteuid(uid_t uid)
{
    return (setresuid(-1, uid, -1));
}
