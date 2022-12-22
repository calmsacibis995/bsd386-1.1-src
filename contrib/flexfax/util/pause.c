const int MSEC_PER_SEC = 1000;

#include <sys/time.h>
#include <limits.h>

void
pause(int ms)
{
    struct timeval tv;
    tv.tv_sec = ms / MSEC_PER_SEC;
    tv.tv_usec = (ms % MSEC_PER_SEC) * 1000;
    (void) select(0, 0, 0, 0, &tv);
#ifdef notdef
    sginap((ms * CLK_TCK) / 1000);
#endif
}

main(int argc, char* argv[])
{
    struct timeval before, after;

    gettimeofday(&before, 0);
    pause(atoi(argv[1]));
    gettimeofday(&after, 0);
    after.tv_sec -= before.tv_sec;
    after.tv_usec -= before.tv_usec;
    if (after.tv_usec < 0) {
	after.tv_sec--;
	after.tv_usec += 1000000;
    }
    printf("%u.%03u\n", after.tv_sec, after.tv_usec/1000);
}
