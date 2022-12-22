#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include "kernel.h"
#include "paths.h"
#ifdef DEBUG
#include <stdarg.h>
#endif

struct device devices[] = {
  { DEV_FLOP, 1, "flop1", { 0 }, 0, },
  { DEV_FLOP, 2, "flop2", { 0 }, 0, },
  { DEV_FLOP, 3, "flop3", { 0 }, 0, },
  { DEV_FLOP, 4, "flop4", { 0 }, 0, },
  { DEV_HARD, 1, "hard1", { 0 }, 0, },
  { DEV_HARD, 2, "hard2", { 0 }, 0, },
  { DEV_LPT,  1, "lpt1",  { 0 }, 0, },
  { DEV_LPT,  2, "lpt2",  { 0 }, 0, },
  { DEV_LPT,  3, "lpt3",  { 0 }, 0, },
  { DEV_LPT,  4, "lpt4",  { 0 }, 0, },
  { DEV_MOUSE,  0, "mouse",  { 0 }, 0, },
};

int ndevices = sizeof(devices) / sizeof(devices[0]);

#define BUFSIZE 1024

FILE *logfile = NULL;

struct device *search_device(enum devtype dtype, int dnum)
{
  struct device *dp;
  int i;

  for (i = 0, dp = devices; i < ndevices; i++, dp++)
    if (dp->dev_type == dtype && dp->dev_num == dnum)
      break;
  if (i == ndevices || dp->dev_real_name[0] == 0)
    dp = NULL;
  return(dp);
}

static void syntax_error(int line)
{
  fprintf(stderr, "Syntax error in config file (line %d)\n", line);
  exit(1);
}

void read_config_file(struct trapframe *tf)
{
  FILE *fp;
  char *home;
  char buffer[BUFSIZE];
  char name[BUFSIZE];
  int i, addr, size, val, line = 0;
  extern char *getenv();
  
  if (access(CONFIG_FILE_NAME, R_OK) == 0)	/* config in current dir? */
      (void) strcpy(buffer, CONFIG_FILE_NAME);
  else
  {
      if ((home = getenv("HOME")) != NULL)	/* config in home dir? */ 
      (void) sprintf(buffer, "%s/%s", home, CONFIG_FILE_NAME);
      if (access(buffer, R_OK) < 0)
          if (access(SYS_CONFIG_NAME, R_OK) == 0)	/* sys config? */
              (void) strcpy (buffer, SYS_CONFIG_NAME);
	  else
	  {
	      fprintf(stderr, 
	      	"Can't find any config files\n\t(e.g., ./%s, $HOME/%s, %s)\n",
	      	CONFIG_FILE_NAME, CONFIG_FILE_NAME, SYS_CONFIG_NAME);
	      exit(1);
	  }
  }
      
  if ((fp = fopen(buffer, "r")) == NULL) {
    fprintf(stderr, "can't open configuration file %s\n", buffer);
    exit(1);
  }
  while (fgets(buffer, BUFSIZE, fp)) {
    line++;
    if (strlen(buffer) <= 1 || buffer[0] == '#')
      continue;
    if (strncmp(buffer, "logfile", 7) == 0) {
      if (sscanf(buffer, "%*s %s", name) != 1)
	syntax_error(line);
      logfile = fopen(name, "w");
    } else if (strncmp(buffer, "nokernemul", 10) == 0) {
      extern int kernel_emulator;
#ifdef DEBUG
      dprintf("kernel emulator turn off\n");
#endif
      kernel_emulator = 0;
#ifdef TRACE
    } else if (strncmp(buffer, "trace", 5) == 0) {
      if (sscanf(buffer, "%*s %c", &val) != 1)
	syntax_error(line);
#ifdef DEBUG
      dprintf("trace terminal /dev/ttyp%c\n", val);
#endif
      init_trace(val);
#endif
    } else if (strncmp(buffer, "assign", 6) == 0) {
      struct device *dp;

      if (sscanf(buffer, "%*s %s", name) != 1)
	syntax_error(line);
      for (i = 0, dp = devices; i < ndevices; i++, dp++)
	if (strcmp(dp->dev_name, name) == 0)
	  break;
      if (i == ndevices)
	syntax_error(line);
      if (dp->dev_type == DEV_FLOP || dp->dev_type == DEV_HARD) {
	if (sscanf(buffer, "%*s %*s %s %d",
		   dp->dev_real_name, &dp->dev_id) != 2)
	  syntax_error(line);
	if (dp->dev_type == DEV_FLOP &&
	    (dp->dev_id != 360 && dp->dev_id != 720 &&
	     dp->dev_id != 1200 && dp->dev_id != 1440))
	  syntax_error(line);
#ifdef DEBUG
	dprintf("assign dev %s to %s id %d\n",
		dp->dev_name, dp->dev_real_name, dp->dev_id);
#endif
      } else if (dp->dev_type == DEV_MOUSE) {
	if (sscanf(buffer, "%*s %*s %s %s",
		   dp->dev_real_name, name) != 2)
	  syntax_error(line);
	
	if (strcasecmp(name, "MicroSoft") == 0)
	  dp->dev_id = MicroSoft;
	else if (strcasecmp(name, "MouseSystems") == 0)
	  dp->dev_id = MouseSystems;
	else if (strcasecmp(name, "MMSeries") == 0)
	  dp->dev_id = MMSeries;
	else if (strcasecmp(name, "Logitech") == 0)
	  dp->dev_id = Logitech;
	else
	  syntax_error(line);
#ifdef DEBUG
	dprintf("assign mouse to %s (id %d)\n",
		  dp->dev_real_name, dp->dev_id);
#endif
      } else {
	if (sscanf(buffer, "%*s %*s %s", dp->dev_real_name) != 1)
	    syntax_error(line);
#ifdef DEBUG
	dprintf("assign dev %s to %s\n",
		  dp->dev_name, dp->dev_real_name);
#endif
      }
    } else
      syntax_error(line);
  }
}

#ifdef DEBUG
void dprintf(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  if (logfile) {
    (void)vfprintf(logfile, fmt, args);
/*    (void)fflush(logfile); */
  }
  va_end(args);
}
#endif
