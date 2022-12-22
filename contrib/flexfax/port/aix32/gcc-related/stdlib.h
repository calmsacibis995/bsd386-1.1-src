extern "C" {
#define getopt  Fx_getopt
#include_next <stdlib.h>
#undef  getopt
}
