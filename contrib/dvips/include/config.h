/* config.h: master configuration file, included first by all compilable
   source files (not headers).  */

#ifndef CONFIG_H
#define CONFIG_H

/* System dependencies that are figured out by `configure'.  */
#include "c-auto.h"

/* ``Standard'' headers.  */
#include "c-std.h"

/* strchr vs. index, memcpy vs. bcopy, etc.  */
#include "c-memstr.h"

/* Error numbers and errno declaration.  */
#include "c-errno.h"

/* Minima and maxima.  */
#include "c-minmax.h"

/* The arguments for fseek.  */
#include "c-seek.h"

/* How to open files with fopen.  */
#include "c-fopen.h"

/* Macros to discard or keep prototypes.  */
#include "c-proto.h"

/* Some definitions of our own.  */
#include "types.h"
#include "lib.h"

#if defined (DOS) || defined (MSDOS)
#undef DOS
#undef MSDOS
#define DOS
#define MSDOS
#endif

#define READ FOPEN_R_MODE
#define READBIN FOPEN_RBIN_MODE
#define WRITEBIN FOPEN_WBIN_MODE

/* This should be called only after a system call fails.  */
#define FATAL_PERROR(s) do { perror (s); exit (errno); } while (0)

/* A generic fatal error.  */
#define START_FATAL() do { fputs ("fatal: ", stderr)
#define END_FATAL() fputs (".\n", stderr); exit (1); } while (0)

#define FATAL(str) START_FATAL (); fprintf (stderr, "%s", str); END_FATAL ()

#endif /* not CONFIG_H */
