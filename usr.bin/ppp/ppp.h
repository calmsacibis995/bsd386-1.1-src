/*
 * Includes
 */
#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <setjmp.h>
#include <unistd.h>
#include <sgtty.h>
#include <errno.h>
#include "pathnames.h"

/*
 * File descriptors
 */
extern int FD;          /* line */
extern int Socket;      /* control socket */
extern char *sysname;   /* system name */

char *uucplock;         /* UUCP lock file name */

exterin int debug;

/*
 * The list of system capabilities
 */
extern char *AT;     /* Auto call unit type. */
extern int   BR;     /* Baud rate. */
extern char *CM;     /* Map of special characters (as in pppconfig) */
extern char *CU;     /* Call unit if making a phone call. */
extern int   DI;     /* Dial-in allowed. */
extern int   DO;     /* Automatic dial-out allowed. */
extern int   DU;     /* This is a dial-up line */
extern char *DV;     /* Device(s) to open to establish a connection. */
extern char *E[10];  /* Strings to wait for on nth step of logging in */
		     /* (after sending s0-s9) */
extern char *F[10];  /* Strings to send if no expected str appeared */
extern int   ID;     /* Disconnect on idle (n seconds) */
extern char *IF;     /* Space-separated list of ifconfig parameters. */
extern int   IN;     /* Interface number. */
extern int   MC;     /* Max PPP config retries. */
extern int   MR;     /* PPP MRU. */
extern int   MT;     /* Max PPP terminate retries. */
extern char *PF;     /* Comma-separated list of PPP flags (as in pppconfig) */
extern char *PN;     /* Telephone number(s) for this host. */
extern char *S[10];  /* String to send on nth step of logging in. */
extern int   T[10];  /* Timeout in seconds for e0-e9 */
extern int   TO;     /* PPP retrty timeout (1/10 sec). */

/* Functions */
extern char *hunt();
extern char *printable();
extern char *Connect();

extern char *index(), *rindex();
extern char *tgetstr();

/*
 * Definition of ACU line description
 */
typedef
	struct {
		char	*acu_name;
		int	(*acu_dialer)();
		int	(*acu_disconnect)();
		int	(*acu_abort)();
	}
	acu_t;

#define	equal(a, b)	(strcmp(a,b)==0)/* A nice function to string compare */

#define NOACU	((acu_t *)NULL)
#define NOSTR	((char *)NULL)

int	AC;			/* open file descriptor to dialer (v831 only) */

#define value(x) x
#define number(x) x
#define boolean(x) x

#define VERBOSE debug
#define DIALTIMEOUT 60
#define BAUDRATE BR
