#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

extern struct queue *q_irq[];
struct queue *qlist = NULL;

struct queue *create_queue (int intno)
{
  struct queue *q;

  if ( (q = (struct queue *) malloc (sizeof(struct queue))) == NULL)
    ErrExit (0x12, "Out of memory\n");
  q->head = q->tail = 0;
  q->next = qlist;
  q->intno = intno;
  q->locked = 0;
  qlist = q;
  q_irq [intno] = q;
  return q;
}
