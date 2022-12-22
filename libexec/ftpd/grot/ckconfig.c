#define  HOST_ACCESS  1

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "pathnames.h"

main()
{
  struct stat  sbuf;
  char        *sp;
  char         buf[1024];

  /* _PATH_FTPUSERS   */
  fprintf(stdout, "Checking _PATH_FTPUSERS :: %s\n", _PATH_FTPUSERS);
  if ( (stat(_PATH_FTPUSERS, &sbuf)) < 0 )
    printf("I can't find it... look in doc/examples for an example.\n");
  else
    printf("ok.\n");

  /* _PATH_FTPACCESS  */
  fprintf(stdout, "\nChecking _PATH_FTPACCESS :: %s\n", _PATH_FTPACCESS);
  if ( (stat(_PATH_FTPACCESS, &sbuf)) < 0 )
    printf("I can't find it... look in doc/examples for an example.\n");
  else
    printf("ok.\n");

  /* _PATH_PIDNAMES   */
  fprintf(stdout, "\nChecking _PATH_PIDNAMES :: %s\n", _PATH_PIDNAMES);
  strcpy(buf, _PATH_PIDNAMES);
  sp = (char *)strrchr(buf, '/');
  *sp = '\0';
  if ( (stat(buf, &sbuf)) < 0 ) {
    printf("I can't find it...\n");
    printf("You need to make this directory [%s] in order for\n",buf);
    printf("the limit and user count functions to work.\n");
  } else
    printf("ok.\n");

  /* _PATH_CVT        */
  fprintf(stdout, "\nChecking _PATH_CVT :: %s\n", _PATH_CVT);
  if ( (stat(_PATH_CVT, &sbuf)) < 0 )
    printf("I can't find it... look in doc/examples for an example.\n");
  else
    printf("ok.\n");

  /* _PATH_XFERLOG    */
  fprintf(stdout, "\nChecking _PATH_XFERLOG :: %s\n", _PATH_XFERLOG);
  if ( (stat(_PATH_XFERLOG, &sbuf)) < 0 ) {
    printf("I can't find it... \n");
    printf("Don't worry, it will be created automatically by the\n");
    printf("server if you do transfer logging.\n");
  } else
    printf("ok.\n");

  /* _PATH_PRIVATE    */
  fprintf(stdout, "\nChecking _PATH_PRIVATE :: %s\n", _PATH_PRIVATE);
  if ( (stat(_PATH_PRIVATE, &sbuf)) < 0 ) {
    printf("I can't find it... look in doc/examples for an example.\n");
    printf("You only need this if you want SITE GROUP and SITE GPASS\n");
    printf("functionality. If you do, you will need to edit the example.\n");
  } else
    printf("ok.\n");

  /* _PATH_FTPHOSTS   */
  fprintf(stdout, "\nChecking _PATH_FTPHOSTS :: %s\n", _PATH_FTPHOSTS);
  if ( (stat(_PATH_FTPHOSTS, &sbuf)) < 0 ) {
    printf("I can't find it... look in doc/examples for an example.\n");
    printf("You only need this if you are using the HOST ACCESS features\n");
    printf("of the server.\n");
  } else
    printf("ok.\n");
}
