#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include "kernel.h"
#include "queue.h"

struct ihandler {
  int fd;
  void (*input_function)(void);
  struct ihandler *next;
};

static struct ihandler *ihandlers = NULL;
static fd_set select_mask;
static int select_count;
static int input_busy = 0;

static void compute_select_count(void)
{
  struct ihandler *ih;

  FD_ZERO(&select_mask);
  select_count = 0;

  for (ih = ihandlers; ih != NULL; ih = ih->next) {
    FD_SET(ih->fd, &select_mask);
    if (select_count <= ih->fd)
      select_count = ih->fd + 1;
  }
}

void add_input_handler(int fd, void (*input_function)(void))
{
  struct ihandler *ih;
  void *malloc();

  input_busy++;

  if ((ih = (struct ihandler *)malloc(sizeof(struct ihandler))) == NULL)
    ErrExit (0x07, "Out of memory\n");

  ih->fd = fd;
  ih->input_function = input_function;
  ih->next = ihandlers;
  ihandlers = ih;
  
  compute_select_count();
  input_busy--;
}

void delete_input_handler(int fd)
{
  struct ihandler *ih, *temp_ih;

  input_busy++;
  if (select_count == 0)
    ErrExit (100, "No handlers to delete\n");
  if (ihandlers->fd == fd) {
    temp_ih = ihandlers;
    ihandlers = ihandlers->next;
    free(temp_ih);
  } else {
    for (ih = ihandlers; ih->next != NULL; ih = ih->next) {
      if (ih->next->fd == fd) {
	temp_ih = ih->next;
	ih->next = ih->next->next;
	free(temp_ih);
	break;
      }
    }
  }
  compute_select_count();
  input_busy--;
}

void do_inputs(void)
{
  struct ihandler *ih;
  struct timeval tv;
  fd_set files;

  if (select_count == 0 || input_busy)
    return;

  tv.tv_sec = 0;
  tv.tv_usec = 0;

  files = select_mask;
  select(select_count, &files, NULL, NULL, &tv);

  for (ih = ihandlers; ih != NULL; ih = ih->next)
    if (FD_ISSET(ih->fd, &files))
      ih->input_function();
}
