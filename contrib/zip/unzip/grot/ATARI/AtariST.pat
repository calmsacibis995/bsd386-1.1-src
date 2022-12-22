*** mapname.c.orig	Fri Feb  7 22:56:19 1992
--- mapname.c	Fri Feb  7 23:06:32 1992
***************
*** 209,215 ****
--- 209,219 ----
  #ifdef MACOS
                      strcat(cdp, ":");
  #else /* !MACOS */
+ #if	ATARI_ST
+                     strcat(cdp, "\\");
+ #else  /* !ATARI_ST */
                      strcat(cdp, "/");
+ #endif /* ?ATARI_ST */
  #endif /* ?MACOS */
  #endif /* ?VMS */
                  }               /***** FALL THROUGH to ':' case  **** */
*** unzip.c.orig	Fri Feb  7 22:56:20 1992
--- unzip.c	Fri Feb  7 23:17:06 1992
***************
*** 119,124 ****
--- 119,131 ----
  byte *stack;
  #else
  byte suffix_of[HSIZE + 1];      /* also s-f length_nodes (smaller) */
+ #if	ATARI_ST
+ /* now this is the third time I had to fix this...
+  * does NOBODY understand that you C_A_N_N_O_T reuse a byte array
+  * for anything of larger type because of possible alignment problems?
+  */
+ int	HadToAlignStackElseItCrashed;
+ #endif
  byte stack[HSIZE + 1];          /* also s-f distance_nodes (smaller) */
  #endif
  
*** unzip.h.orig	Fri Feb  7 22:56:21 1992
--- unzip.h	Sat Feb  8 00:47:55 1992
***************
*** 27,34 ****
  #  if defined(THINK_C) || defined(MPW) /* for Macs */
  #    include <stddef.h>
  #  else
! #    include <sys/types.h> /* off_t, time_t, dev_t, ... */
! #    include <sys/stat.h>  /* Everybody seems to need this. */
  #  endif
  #endif                   /*   This include file defines
                            *     #define S_IREAD 0x0100  (owner may read)
--- 27,39 ----
  #  if defined(THINK_C) || defined(MPW) /* for Macs */
  #    include <stddef.h>
  #  else
! #    ifdef ATARI_ST
! #      include <stddef.h>
! #      define __STDC__ 1 /* see note below */
! #    else
! #      include <sys/types.h> /* off_t, time_t, dev_t, ... */
! #      include <sys/stat.h>  /* Everybody seems to need this. */
! #    endif
  #  endif
  #endif                   /*   This include file defines
                            *     #define S_IREAD 0x0100  (owner may read)
***************
*** 71,76 ****
--- 76,95 ----
      And now, our MS-DOS and OS/2 corner:
    ---------------------------------------------------------------------------*/
  
+ /*
+  * How comes poor little Atari ST 's playing with these boys of the 'hood ?
+  *
+  * For everybody: TURBO C for the Atari ST also defines __TURBOC__
+  *                You (yes YOU!!) may NOT RELY ON __TURBOC__ to tell
+  *                that this is MSDOS or whatever!
+  */
+ #ifdef ATARI_ST
+ /* KLUDGE KLUDGE KLUDGE KLUDGE KLUDGE KLUDGE KLUDGE KLUDGE KLUDGE KLUDGE    */
+ #undef __TURBOC__
+ #endif
+ 
+ /* FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME FIXME  */
+ /*       vvvvvv    */
  #ifdef __TURBOC__
  #  define DOS_OS2             /* Turbo C under DOS, MSC under DOS or OS2    */
  #  include <sys/timeb.h>      /* for structure ftime                        */
***************
*** 123,132 ****
  #    ifdef MTS
  #      include <sys/file.h>     /* MTS uses this instead of fcntl.h */
  #    else
! #      include <fcntl.h>
  #    endif
  #  endif
  #endif
  /*
   *   fcntl.h (above):   This include file defines
   *                        #define O_BINARY 0x8000  (no cr-lf translation)
--- 142,162 ----
  #    ifdef MTS
  #      include <sys/file.h>     /* MTS uses this instead of fcntl.h */
  #    else
!      /*
!       * FIXME:
!       *
!       * Again, just by not being VMS, V7 or MTS, the little Atari ST
!       * winds up here (and everybody else too). WHOEVER NEEDS this stuff,
!       * DECLARE YOURSELF and DONT rely on this kind of negative logic!
!       */
! #      ifndef ATARI_ST
!          /* KLUDGE KLUDGE KLUDGE KLUDGE KLUDGE KLUDGE KLUDGE KLUDGE KLUDGE    */
! #        include <fcntl.h>
! #      endif
  #    endif
  #  endif
  #endif
+ 
  /*
   *   fcntl.h (above):   This include file defines
   *                        #define O_BINARY 0x8000  (no cr-lf translation)
***************
*** 159,164 ****
--- 189,203 ----
      And finally, some random extra stuff:
    ---------------------------------------------------------------------------*/
  
+ /* FIXME:
+  *
+  * As used by Turbo C (at least for the Atari ST),
+  * __STDC__ means that the compiler has been RESTRICTED to standard ANSI C.
+  *
+  * What we want here is: do we have a compiler which has ANSI C prototypes
+  * and includes.
+  * So better use somthing like: ANSI_C or so...
+  */
  #ifdef __STDC__
  #  include <stdlib.h>      /* standard library prototypes, malloc(), etc. */
  #  include <string.h>      /* defines strcpy, strcmp, memcpy, etc. */
***************
*** 169,174 ****
--- 208,245 ----
  #endif
  
  
+ /* Incidently, for Turbo C on the Atari ST we just order the following items:
+  */
+ #if ATARI_ST
+ #  include <time.h>
+ /* the following includes are really specific for Turbo C 2.0 !!      */
+ #  include <ext.h>      /* this gives us stat()                         */
+ #  include <tos.h>      /* OS specific functions (Fdup)                 */
+ #  define MSDOS         1               /* from here on. */
+ #  define DOS_OS2       1               /* from here on. */
+ #  define __TURBOC__    1               /* from here on. */
+ /*
+  * FIXME:
+  * Although the Atari ST (MC68000) and Turbo C use 16 bit ints,
+  * we have to use NOTINT16, since its an high-endian, and therefore
+  * we cannot read the intel little-endian structs.
+  * For that reason, NOTINT16 is another misnomer.
+  */
+ #  define NOTINT16      1
+ 
+ #  ifndef S_IFMT
+ #  define S_IFMT        (S_IFCHR|S_IFREG|S_IFDIR)
+ #  endif
+ 
+ #  ifndef O_BINARY
+ #  define O_BINARY 0
+ #  endif
+ 
+ /* replace dup by corresponding tos function  */
+ #  define       dup             Fdup
+ #  define       mkdir           Dcreate
+ 
+ #endif
  
  
  
