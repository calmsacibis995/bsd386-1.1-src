extern "C" {
#include_next <sys/time.h>
int gettimeofday(struct timeval*, struct timezone*);
int settimeofday(struct timeval*, struct timezone*);
int usleep (unsigned int);
struct itimerval;
int setitimer (int, itimerval*, itimerval*);
int getitimer (int, itimerval*);
}
