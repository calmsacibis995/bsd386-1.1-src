extern "C" {
#include <sys/types.h>
#include_next <sys/signal.h>
int sigpause (int);
int sigsetmask (int);
int sigblock (int);
void (*sigset(int, void (*) (int)))(int);
int sighold(int);
int sigrelse(int);
int sigignore(int);
#undef SV_INTERRUPT
}
