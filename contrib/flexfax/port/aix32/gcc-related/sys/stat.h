extern "C" {
#define fchmod Fx_fchmod
#define fchown Fx_fchown
#include_next <sys/stat.h>
#undef fchmod
#undef fchown
int      fstat(int fildes, struct stat *);
}
