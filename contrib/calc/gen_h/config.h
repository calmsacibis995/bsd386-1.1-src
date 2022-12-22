/*	BSDI	$Id: config.h,v 1.1.1.1 1994/01/03 23:14:04 polk Exp $	*/

/* the default :-separated search path */
#ifndef DEFAULTCALCPATH
#define DEFAULTCALCPATH ".:./lib:~/lib:/usr/contrib/lib/calc"
#endif /* DEFAULTCALCPATH */

/* the default :-separated startup file list */
#ifndef DEFAULTCALCRC
#define DEFAULTCALCRC "/usr/contrib/lib/calc/startup:~/.calcrc"
#endif /* DEFAULTCALCRC */

/* the default key bindings file */
#ifndef DEFAULTCALCBINDINGS
#define DEFAULTCALCBINDINGS "/usr/contrib/lib/calc/bindings"
#endif /* DEFAULTCALCBINDINGS */

/* the location of the help directory */
#ifndef HELPDIR
#define HELPDIR "/usr/contrib/lib/calc/help"
#endif /* HELPDIR */

/* the default pager to use */
#ifndef DEFAULTCALCPAGER
#define DEFAULTCALCPAGER "more"
#endif /* DEFAULTCALCPAGER */
