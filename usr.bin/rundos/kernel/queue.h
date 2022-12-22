#define QSIZE 1024

struct queue {
  unsigned char buffer[QSIZE];
  int head;
  int tail;
  int intno;
  int locked;
  struct queue *next;
};

static inline int check_queue_overflow (struct queue *q)
{
  if ( q->head == q->tail ) {
    if ( q->tail-- == 0 )
      q->tail = QSIZE-1;
    return 1;
  } else
    return 0;
}

static inline void put_char_q (struct queue *q, unsigned char ch)
{
/* check of buffer overflow !!! (it is but not here for kbd_queue) */
  q->buffer[q->tail++] = ch;
  if (q->tail == QSIZE)
    q->tail = 0;
}

static inline void put_char_to_head (struct queue *q, unsigned char ch)
{
/* check of buffer overflow !!! (it is but not here for kbd_queue) */
  if ( q->head-- == 0 )
    q->head = QSIZE-1;
  q->buffer[q->head] = ch;
}

static inline int queue_not_empty (struct queue *q)
{
  return (q->head != q->tail);
}

static inline int queue_empty (struct queue *q)
{
  return (q->head == q->tail);
}

static inline unsigned char get_char_q (struct queue *q)
{
  unsigned char ch = q->buffer[q->head++];

  if (q->head == QSIZE)
    q->head = 0;
  return (ch);
}

static inline unsigned char head_char (struct queue *q)
{
  return (q->buffer[q->head]);
}

static inline int queue_is_not_locked (struct queue *q)
{
  return (q->locked == 0);
}

static inline int queue_is_locked (struct queue *q)
{
  return (q->locked != 0);
}

static inline void lock_queue (struct queue *q)
{
  q->locked = 1;
}

static inline void unlock_queue (struct queue *q)
{
  q->locked = 0;
}

static inline void clear_queue (struct queue *q)
{
  q->head = q->tail = 0;
}

static inline int buffer_contents (struct queue *q)
{
  int num;
  
  num = q->head - q->tail;
  return (num < 0) ? QSIZE - num : num;
}

struct queue *create_queue (int intno);
