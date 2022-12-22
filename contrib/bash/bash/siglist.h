/* siglist.h -- encapsulate various definitions for sys_siglist */
#if !defined (_SIGLIST_H_)
#define _SIGLIST_H_

#if defined (Solaris) || defined (USGr4_2) || defined (drs6000)
#  if !defined (sys_siglist)
#    define sys_siglist _sys_siglist
#  endif /* !sys_siglist */
#endif /* Solaris || USGr4_2 */

#if !defined (Solaris) && !defined (Linux) && !defined (__BSD_4_4__)
extern char *sys_siglist[];
#endif /* !Solaris */

#if !defined (strsignal)
#  define strsignal(sig) (char *)sys_siglist[sig]
#endif /* !strsignal */

#endif /* _SIGLIST_H */
