extern "C" {
#define localtime Fx_localtime
#include_next <time.h>
#undef localtime
}
