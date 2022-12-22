/* bashbug.c -- Make a bug template for the user to fill in. */

#include <stdio.h>
#include <pwd.h>
#include "../version.h"

int my_version_major = 1;
int my_version_minor = 0;

main (argc, argv)
     int argc;
     char **argv;
{
  char *user, *host, *getuser (), *gethost ();

  user = getuser ();
  host = gethost ();

  printf ("To: bash-maintainers@ai.mit.edu\n");
  printf ("Reply-To: %s@%s\n", user, host);
  printf ("Subject: Bash-%s bug-report\n", DISTVERSION);
  printf ("Bug-Synopsis: (Please fill this in with a synopsis of the bug)\n");
  printf ("BashBug-Version: %d.%d\n", my_version_major, my_version_minor);
  printf ("Bash-OS-Info: Bash-%s, made for %s running %s.\n",
	  DISTVERSION, SYSTEM_NAME, OS_NAME);

  printf ("--text follows this line--\n\n");

  printf ("Please fill in the fields below as completely as possible.  Any\n");
  printf ("additional info you wish to provide will be happily accepted!\n\n");
  printf ("I need a description of the bug:\n\n\n");
  printf ("A recipe for recreating the bug reliably:\n\n\n");
  printf ("A fix for the bug if you have one!:\n\n\n");
  printf ("Your email-address (if the one in this message is incorrect):\n\n");

  printf ("Thank you very much for reporting this bug.  We will try to fix\n");
  printf ("it as quickly as possible.\n\n");
}

/* Attempt to get this user's loginname. */
char *
getuser ()
{
  struct passwd *entry;
  int uid;

  uid = (int) getuid ();

  entry = getpwuid (uid);
  if (entry)
    return (entry->pw_name);
  else
    return ("UNKNOWN");
}

static char hostname[100];

/* Attempt to get this hostname. */
char *
gethost ()
{
  hostname[0] = '\0';
  gethostname (hostname, sizeof (hostname));
  return (hostname);
}

#if defined (HPUX) || defined (sgi)
#  if !defined (HAVE_GETHOSTNAME)
#    define HAVE_GETHOSTNAME
#  endif /* !HAVE_GETHOSTNAME */
#endif /* HPUX || sgi */

#if defined (USG) && !defined (HAVE_GETHOSTNAME)

#include <sys/utsname.h>

int
gethostname (name, namelen)
     char *name;
     int namelen;
{
  int i;
  struct utsname ut;

  --namelen;

  uname (&ut);
  i = strlen (ut.nodename) + 1;
  strncpy (name, ut.nodename, i < namelen ? i : namelen);
  name[namelen] = '\0';
  return (0);
}
#endif /* USG && !HAVE_GETHOSTNAME */
