extern "C" {
#define popen Fx_popen
#define vfprintf Fx_vfprintf
#define vprintf Fx_vprintf
#define vsprintf Fx_vsprintf

#ifdef _VA_LIST
#define __va_list       va_list
#else
typedef char *          __va_list;
#endif

#include_next <stdio.h>

#undef  popen
#undef  vfprintf
#undef  vprintf
#undef  vsprintf

extern FILE* popen(const char*, const char*);
extern int _filbuf(FILE*);
extern int _flsbuf(unsigned char, FILE*);
extern int vfprintf(FILE *, const char *, __va_list);
extern int vprintf(const char *, __va_list); 
extern int vsprintf(char *, const char *, __va_list);
}
