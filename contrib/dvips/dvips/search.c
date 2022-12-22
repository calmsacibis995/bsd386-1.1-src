/*
 *   The search routine takes a directory list, separated by PATHSEP, and
 *   tries to open a file.  Null directory components indicate current
 *   directory. if the file SUBDIR exists and the file is a font file,
 *   it checks for the file in a subdirectory named the same as the font name.
 *   Returns the open file descriptor if ok, else NULL.
 */
#include "structures.h" /* The copyright notice in that file is included too! */
#include "pathsrch.h"
/*
 *
 *   We hope MAXPATHLEN is enough -- only rudimentary checking is done!
 */

#ifdef DEBUG
extern integer debug_flag;
#endif  /* DEBUG */
extern char *mfmode ;
extern int actualdpi ;

/* Set this when we successfully find something.  */
string realnameoffile ;

FILE *
search(path, file, mode)
        char **path, *file, *mode ;
{
   FILE *f;
   string name = find_path_filename (file, path);
   
   if (name == NULL)
     f = NULL;
   else
     {
       f = fopen (name, mode);
       realnameoffile = name;
     }
   
   return f;
}               /* end search */

FILE *
pksearch(path, file, mode, n, dpi, vdpi)
        char **path, *file, *mode ;
	char *n ;
	halfword dpi, vdpi ;
{
   return search (path, file, mode);
}               /* end search */

/* do we report file openings? */

#ifdef DEBUG
#  ifdef fopen
#    undef fopen
#  endif
#  ifdef VMCMS  /* IBM: VM/CMS */
#    define fopen cmsfopen
#  endif /* IBM: VM/CMS */
FILE *my_real_fopen(n, t)
register char *n, *t ;
{
   FILE *tf ;
   if (dd(D_FILES)) {
      fprintf(stderr, "<%s(%s)> ", n, t) ;
      tf = fopen(n, t) ;
      if (tf == 0)
         fprintf(stderr, "failed\n") ;
      else
         fprintf(stderr, "succeeded\n") ;
   } else
      tf = fopen(n, t) ;
   return tf ;
}
#endif
