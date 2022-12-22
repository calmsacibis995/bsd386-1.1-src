/* input.h -- Structures and unions used for reading input. */

#if !defined (_INPUT_H)
#define _INPUT_H

/* Function pointers can be declared as (Function *)foo. */
#if !defined (__FUNCTION_DEF)
#  define __FUNCTION_DEF
typedef int Function ();
typedef void VFunction ();
typedef char *CPFunction ();
typedef char **CPPFunction ();
#endif /* _FUNCTION_DEF */

/* Some stream `types'. */
#define st_stream 1
#define st_string 2

#if defined (BUFFERED_INPUT)
#define st_bstream 3

/* Possible values for b_flag. */
#define B_EOF		0x1
#define B_ERROR		0x2
#define B_UNBUFF	0x4

/* A buffered stream.  Like a FILE *, but with our own buffering and
   synchronization.  Look in input.c for the implementation. */
typedef struct BSTREAM
{
  int	b_fd;
  char	*b_buffer;		/* The buffer that holds characters read. */
  int	b_size;			/* How big the buffer is. */
  int	b_used;			/* How much of the buffer we're using, */
  int	b_flag;			/* Flag values. */
  int	b_inputp;		/* The input pointer, index into b_buffer. */
} BUFFERED_STREAM;

extern BUFFERED_STREAM **buffers;

extern BUFFERED_STREAM *fd_to_buffered_stream ();

extern int default_buffered_input;

#endif /* BUFFERED_INPUT */

typedef union {
  FILE *file;
  char *string;
#if defined (BUFFERED_INPUT)
  int buffered_fd;
#endif
} INPUT_STREAM;

typedef struct {
  int type;
  char *name;
  INPUT_STREAM location;
  Function *getter;
  Function *ungetter;
} BASH_INPUT;

extern BASH_INPUT bash_input;

#endif /* _INPUT_H */
