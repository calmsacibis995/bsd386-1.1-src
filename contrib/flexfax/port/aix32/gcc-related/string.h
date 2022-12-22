extern "C" {
#include <sys/types.h>
#include <standards.h>
extern char 	*strerror(int errnum);
extern char     *strchr(const char *s, int c);
extern int      strncmp(const char *s1,const char *s2,size_t n);
extern char     *strdup(char *s);
}
