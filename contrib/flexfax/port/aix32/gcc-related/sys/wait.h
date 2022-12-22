extern "C" {
#define wait3	Fx_wait3
#include_next <sys/wait.h>
#undef wait3
}
