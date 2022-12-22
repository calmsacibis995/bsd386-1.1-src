#ifndef _H_UNISTD

extern "C" {
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

/* avoid clash with (identical, but differing in whitespace) definitions in stdio.h (some macro-preprocessors are whitespace sensitive) */
#ifdef SEEK_SET
#undef SEEK_SET
#endif
#ifdef SEEK_CUR
#undef SEEK_CUR
#endif
#ifdef SEEK_END
#undef SEEK_END
#endif

// Override these definitions in /usr/include/unistd.h
#define chdir  Fx_chdir
#define execve Fx_execve
#define execvp Fx_execvp
#define getcwd Fx_getcwd
#define read   Fx_read
#define write  Fx_write

// Avoid clashes with C++ operator new
#define new    Fx_new

#include_next <unistd.h>

#undef chdir
#undef execve
#undef execvp
#undef getcwd
#undef read   
#undef write

#undef new
#ifdef NULL
#undef NULL
#define NULL 0
#endif

int chdir(const char*);
int execve(const char*, const char*[], const char*[]);
int execvp(const char*, const char*[]);
char *getcwd(const char *, int);
int read(int, void*, unsigned);
int write(int, const void*, unsigned);

// Add these functions - somehow they are missing from /usr/include/unistd.h
int acct(const char*);
int brk(char*);
int chroot(const char*);
int closepl();
char *getwd(char *);
int ioctl(int, int ...);
int lockf(int, int, off_t);
int mknod(const char*, int, int);
int mount(const char*, const char*, int);
int nice(int);
int openpl();
int plock(int);
void profil(const char*, int, int, int);
int ptrace(int, int, int, int);
int rmount(const char *,const char *, const char *, int);
char *sbrk(int);
int setpgrp();
int stime(const long*);
void sync();
void sys3b(int, int ...);
int umount(const char*);
int vfork(void); 

int system(const char*);
int kill(pid_t, int);

int accessx(const char *, int, int);
int faccessx(int, int, int);

}
#endif
