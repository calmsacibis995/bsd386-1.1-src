/* general.h -- defines that everybody likes to use. */

#if !defined (_GENERAL_H)
#define _GENERAL_H

#if !defined (NULL)
#  if defined (__STDC__)
#    define NULL ((void *) 0)
#  else
#    define NULL 0x0
#  endif /* !__STDC__ */
#endif /* !NULL */

#if defined (HAVE_STRING_H)
#  include <string.h>
#else
#  include <strings.h>
#endif /* !HAVE_STRING_H */

#define pointer_to_int(x) (int)((long)(x))

#if !defined (savestring)
   extern char *xmalloc ();
#  if !defined (strcpy)
   extern char *strcpy ();
#  endif
#  define savestring(x) (char *)strcpy (xmalloc (1 + strlen (x)), (x))
#endif

#ifndef whitespace
#define whitespace(c) (((c) == ' ') || ((c) == '\t'))
#endif

#ifndef digit
#define digit(c)  ((c) >= '0' && (c) <= '9')
#endif

#ifndef isletter
#define isletter(c) (((c) >= 'A' && (c) <= 'Z') || ((c) >= 'a' && (c) <= 'z'))
#endif

#ifndef digit_value
#define digit_value(c) ((c) - '0')
#endif

#if !defined (maybe_free)
#define maybe_free(x) do { if (x) free (x); } while (0)
#endif /* !maybe_free */

#if !defined (__STDC__) && !defined (strchr)
extern char *strchr (), *strrchr ();
#endif /* !strchr */

#ifndef member
#  if defined (alpha) && defined (__GNUC__)
     extern char *strchr ();
#  endif
#  define member(c, s) ((c) ? ((char *)strchr ((s), (c)) != (char *)NULL) : 0)
#endif

/* All structs which contain a `next' field should have that field
   as the first field in the struct.  This means that functions
   can be written to handle the general case for linked lists. */
typedef struct g_list {
  struct g_list *next;
} GENERIC_LIST;

/* Here is a generic structure for associating character strings
   with integers.  It is used in the parser for shell tokenization. */
typedef struct {
  char *word;
  int token;
} STRING_INT_ALIST;

/* A macro to avoid making an uneccessary function call. */
#define REVERSE_LIST(list, type) \
  ((list && list->next) ? (type)reverse_list (list) : (type)(list))

/* String comparisons that possibly save a function call each. */
#define STREQ(a, b) ((a)[0] == (b)[0] && strcmp(a, b) == 0)
#define STREQN(a, b, n) ((a)[0] == (b)[0] && strncmp(a, b, n) == 0)

/* Function pointers can be declared as (Function *)foo. */
#if !defined (__FUNCTION_DEF)
#  define __FUNCTION_DEF
typedef int Function ();
typedef void VFunction ();
typedef char *CPFunction ();
typedef char **CPPFunction ();
#endif /* _FUNCTION_DEF */

#define NOW	((time_t) time ((time_t *) 0))

/* Some defines for calling file status functions. */
#define FS_EXISTS	  0x1
#define FS_EXECABLE	  0x2
#define FS_EXEC_PREFERRED 0x4
#define FS_EXEC_ONLY	  0x8

/* Posix and USG systems do not guarantee to restart a read () that is
   interrupted by a signal. */
#if defined (USG) || defined (_POSIX_VERSION)
#  define NO_READ_RESTART_ON_SIGNAL
#endif /* USG || _POSIX_VERSION */

/* Here is a definition for set_signal_handler () which simply expands to
   a call to signal () for non-Posix systems.  The code for set_signal_handler
   in the Posix case resides in general.c. */

#if defined (VOID_SIGHANDLER)
#  define sighandler void
#else
#  define sighandler int
#endif /* !VOID_SIGHANDLER */

typedef sighandler SigHandler ();

#if !defined (_POSIX_VERSION)
#  define set_signal_handler(sig, handler) (SigHandler *)signal (sig, handler)
#else
extern SigHandler *set_signal_handler ();
#endif /* _POSIX_VERSION */

/* Declarations for non-int functions defined in general.c */
extern GENERIC_LIST *reverse_list (), *delete_element (), *list_append ();
extern char *xmalloc (), *xrealloc ();
extern void vfree ();
extern char *itos ();
extern void unset_nodelay_mode ();
extern void map_over_list ();
extern void map_over_words ();
extern void free_array ();
extern char **copy_array ();
extern void strip_leading ();
extern void strip_trailing ();
extern char *canonicalize_pathname ();
extern char *make_absolute ();
extern char *base_pathname ();
extern char *full_pathname ();
extern char *strindex ();
extern void set_lines_and_columns ();
extern void tilde_initialize ();
extern char *getwd ();
extern char *polite_directory_format ();

#if !defined (strerror)
extern char *strerror ();
#endif

#endif	/* _GENERAL_H */
