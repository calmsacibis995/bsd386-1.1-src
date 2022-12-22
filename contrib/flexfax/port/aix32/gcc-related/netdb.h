extern "C" {
#include_next <netdb.h>
struct hostent * gethostbyname (const char *);
struct servent * getservbyname (const char*, const char*);
}
