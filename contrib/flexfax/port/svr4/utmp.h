#ifndef _PORT_SVR4_UTMP_H
#define _PORT_SVR4_UTMP_H

#ifdef __cplusplus
extern "C" {

#ifndef sun
/* workaround SVR4.0.3 header braindamage */
struct exit_status {
        short e_termination;    /* Process termination status */
        short e_exit;           /* Process exit status */
};
#endif /* !sun */

#endif /* __cplusplus */

#ifdef __GNUC__
#include_next <utmp.h>
#include_next <utmpx.h>
#else
#include "/usr/include/utmp.h"
#include "/usr/include/utmpx.h"
#endif

#ifdef __cplusplus
}
#endif

#define utmp          utmpx
#undef  ut_time
#define ut_time       ut_xtime

#define getutent      getutxent
#define getutid       getutxid
#define getutline     getutxline
#define pututline     pututxline
#define setutent      setutxent
#define endutent      endutxent

#endif /* _PORT_SVR4_UTMP_H */
