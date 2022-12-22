#include <math.h>

/* NB: should call srand48 but this should be good enough... */

long
random(void)
{
    return lrand48();
}
