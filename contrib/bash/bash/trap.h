/* trap.h -- data structures used in the trap mechanism. */

#if !defined (__TRAP_H__)
#define __TRAP_H__

#if !defined (SIG_DFL)
#include <sys/types.h>
#include <signal.h>
#endif /* SIG_DFL */

#if !defined (NSIG)
#define NSIG 64
#endif /* !NSIG */

#define NO_SIG -1
#define DEFAULT_SIG SIG_DFL
#define IGNORE_SIG  SIG_IGN

#define signal_object_p(x) (decode_signal (x) != NO_SIG)

extern char *trap_list[NSIG];
extern char *signal_name ();
extern int decode_signal ();

#endif /* __TRAP_H__ */
