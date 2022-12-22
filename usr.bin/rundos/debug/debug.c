#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>

#ifndef CONFIG_FILE
#define CONFIG_FILE "config.rundos"
#endif

static struct termios tios;
static int fd;

void init_debug(char num)
{
  char tty[64];

  strcpy(tty, "/dev/ptypx");

  tty[strlen(tty) - 1] = num;

  if ((fd = open(tty, O_RDWR)) < 0) {
    fprintf(stderr, "Cannot open debug terminal %s\n", tty);
    exit(1);
  }
  ioctl(fd, TIOCGETA, &tios);
  cfmakeraw(&tios);
  ioctl(fd, TIOCSETA, &tios);
}

void syntax_error(int line)
{
  fprintf(stderr, "Syntax error in config file (line %d)\n", line);
  exit(1);
}

main()
{
  static fd_set selected_files;
  fd_set files;
  char buffer[BUFSIZ];
  int k, line = 0;
  FILE *fp;
  int num = 0;

  if ((fp = fopen(CONFIG_FILE, "r")) == NULL) {
    fprintf(stderr, "can't open configuration file\n");
    exit(1);
  }
  while (fgets(buffer, BUFSIZ, fp)) {
    line++;
    if (strncmp(buffer, "trace", 5) == 0) {
      if (sscanf(buffer, "%*s %c", &num) != 1)
	syntax_error(line);
      fprintf(stderr, "debug terminal = /dev/ptyp%c\n", num);
      init_debug(num);
      break;
    }
  }

  if (!num) {
    fprintf(stderr, "Trace terminal not found in config\n");
    exit(1);
  }

  FD_ZERO(&selected_files);
  FD_SET(0, &selected_files);
  FD_SET(fd, &selected_files);

  while (1) {

    files = selected_files;
    select(fd + 1, &files, NULL, NULL, NULL);
    
    if (FD_ISSET(fd, &files))
      do {
	k = read(fd, buffer, BUFSIZ);
	write(1, buffer, k);
      } while (k == BUFSIZ);
    if (FD_ISSET(0, &files)) 
      do {
	k = read(0, buffer, BUFSIZ);
	write(fd, buffer, k);
      } while (k == BUFSIZ);
  }
}

