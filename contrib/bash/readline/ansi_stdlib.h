/* ansi_stdlib.h -- An ANSI Standard stdlib.h. */

/* A minimal stdlib.h containing extern declarations for those functions
   that bash uses. */

#if !defined (_STDLIB_H_)
#define	_STDLIB_H_ 1

/* String conversion functions. */
extern int atoi ();
extern long int atol ();

/* Memory allocation functions. */
extern char *malloc ();
extern char *realloc ();
extern void free ();

/* Other miscellaneous functions. */
extern void abort ();
extern void exit ();
extern char *getenv ();
extern void qsort ();

#endif /* _STDLIB_H  */
