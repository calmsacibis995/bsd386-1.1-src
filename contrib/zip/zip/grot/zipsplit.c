/*

 Copyright (C) 1990-1992 Mark Adler, Richard B. Wales, Jean-loup Gailly,
 Kai Uwe Rommel and Igor Mandrichenko.
 Permission is granted to any individual or institution to use, copy, or
 redistribute this software so long as all of the original files are included
 unmodified, that it is not sold for profit, and that this copyright notice
 is retained.

*/

/*
 *  zipsplit.c by Mark Adler.
 */

#define UTIL
#include "revision.h"
#include "zip.h"
#include <signal.h>

#define DEFSIZ 36000L   /* Default split size (change in help() too) */
#ifdef MSDOS
#  define NL 2          /* Number of bytes written for a \n */
#else /* !MSDOS */
#  define NL 1          /* Number of bytes written for a \n */
#endif /* ?MSDOS */
#define INDEX "zipsplit.idx"    /* Name of index file */


/* Local functions */
#ifdef PROTO
   local void handler(int);
   local void license(void);
   local void help(void);
   local extent simple(ulg *, extent, ulg, ulg);
   local int descmp(voidp *, voidp *);
   local extent greedy(ulg *, extent, ulg, ulg);
   void main(int, char **);
#endif /* PROTO */


/* Output zip files */
local char template[16];        /* name template for output files */
local int zipsmade = 0;         /* number of zip files made */
local int indexmade = 0;        /* true if index file made */
local char *path = NULL;        /* space for full name */
local char *name;               /* where name goes in path[] */


void err(c, h)
int c;                  /* error code from the ZE_ class */
char *h;                /* message about how it happened */
/* Issue a message for the error, clean up files and memory, and exit. */
{
  if (PERR(c))
    perror("zipsplit error");
  fprintf(stderr, "zipsplit error: %s (%s)\n", errors[c-1], h);
  if (indexmade)
  {
    strcpy(name, INDEX);
    destroy(path);
  }
  for (; zipsmade; zipsmade--)
  {
    sprintf(name, template, zipsmade);
    destroy(path);
  }
  if (path != NULL)
    free((voidp *)path);
  if (zipfile != NULL)
    free((voidp *)zipfile);
#ifdef VMS
  exit(0);
#else /* !VMS */
  exit(c);
#endif /* ?VMS */
}



local void handler(s)
int s;                  /* signal number (ignored) */
/* Upon getting a user interrupt, abort cleanly using err(). */
{
#ifndef MSDOS
  putc('\n', stderr);
#endif /* !MSDOS */
  err(ZE_ABORT, "aborting");
  s++;                                  /* keep some compilers happy */
}


void warn(a, b)
char *a, *b;            /* message strings juxtaposed in output */
/* Print a warning message to stderr and return. */
{
  fprintf(stderr, "zipsplit warning: %s%s\n", a, b);
}


local void license()
/* Print license information to stdout. */
{
  extent i;             /* counter for copyright array */

  for (i = 0; i < sizeof(copyright)/sizeof(char *); i++) {
    printf(copyright[i], "zipsplit");
    putchar('\n');
  }
  for (i = 0; i < sizeof(disclaimer)/sizeof(char *); i++)
    puts(disclaimer[i]);
}


local void help()
/* Print help (along with license info) to stdout. */
{
  extent i;             /* counter for help array */

  /* help array */
  static char *text[] = {
"",
"ZipSplit %d.%d (%s)",
"Usage:  zipsplit [-ti] [-n size] [-b path] zipfile",
"  -t   report how many files it will take, but don't make them",
"  -i   make index (zipsplit.idx) and count its size against first zip file",
"  -n   make zip files no larger than \"size\" (default = 36000)",
"  -b   use \"path\" for the output zip files",
"  -s   do a sequential split even if it takes more zip files",
"  -h   show this help               -L   show software license"
  };

  for (i = 0; i < sizeof(copyright)/sizeof(char *); i++) {
    printf(copyright[i], "zipsplit");
    putchar('\n');
  }
  for (i = 0; i < sizeof(text)/sizeof(char *); i++)
  {
    printf(text[i], REVISION / 10, REVISION % 10, REVDATE);
    putchar('\n');
  }
}


local extent simple(a, n, c, d)
ulg *a;         /* items to put in bins, return value: destination bins */
extent n;       /* number of items */
ulg c;          /* capacity of each bin */
ulg d;          /* amount to deduct from first bin */
/* Return the number of bins of capacity c that are needed to contain the
   integers in a[0..n-1] placed sequentially into the bins.  The value d
   is deducted initially from the first bin (space for index).  The entries
   in a[] are replaced by the destination bins. */
{
  extent k;     /* current bin number */
  ulg t;        /* space used in current bin */

  t = k = 0;
  while (n--)
  {
    if (*a + t > c - (k == 0 ? d : 0))
    {
      k++;
      t = 0;
    }
    t += *a;
    *(ulg huge *)a++ = k;
  }
  return k + 1;
}


local int descmp(a, b)
voidp *a, *b;           /* pointers to pointers to ulg's to compare */
/* Used by qsort() in greedy() to do a descending sort. */
{
  return **(ulg **)a < **(ulg **)b ? 1 : (**(ulg **)a > **(ulg **)b ? -1 : 0);
}


local extent greedy(a, n, c, d)
ulg *a;         /* items to put in bins, return value: destination bins */
extent n;       /* number of items */
ulg c;          /* capacity of each bin */
ulg d;          /* amount to deduct from first bin */
/* Return the number of bins of capacity c that are needed to contain the
   items with sizes a[0..n-1] placed non-sequentially into the bins.  The
   value d is deducted initially from the first bin (space for index).
   The entries in a[] are replaced by the destination bins. */
{
  ulg *b;       /* space left in each bin (malloc'ed for each m) */
  ulg *e;       /* copy of argument a[] (malloc'ed) */
  extent i;     /* steps through items */
  extent j;     /* steps through bins */
  extent k;     /* best bin to put current item in */
  extent m;     /* current number of bins */
  ulg **s;      /* pointers to e[], sorted descending (malloc'ed) */
  ulg t;        /* space left in best bin (index k) */

  /* Algorithm:
     1. Copy a[] to e[] and sort pointers to e[0..n-1] (in s[]), in
        descending order.
     2. Compute total of s[] and set m to the smallest number of bins of
        capacity c that can hold the total.
     3. Allocate m bins.
     4. For each item in s[], starting with the largest, put it in the
        bin with the smallest current capacity greater than or equal to the
        item's size.  If no bin has enough room, increment m and go to step 4.
     5. Else, all items ended up in a bin--return m.
  */

  /* Copy a[] to e[], put pointers to e[] in s[], and sort s[].  Also compute
     the initial number of bins (minus 1). */
  if ((e = (ulg *)malloc(n * sizeof(ulg))) == NULL ||
      (s = (ulg **)malloc(n * sizeof(ulg *))) == NULL)
  {
    if (e != NULL)
      free((voidp *)e);
    err(ZE_MEM, "was trying a smart split");
    return 0;                           /* only to make compiler happy */
  }
  memcpy((char *)e, (char *)a, n * sizeof(ulg));
  for (t = i = 0; i < n; i++)
    t += *(s[i] = e + i);
  m = (extent)((t + c - 1) / c) - 1;    /* pre-decrement for loop */
  qsort((char *)s, n, sizeof(ulg *), descmp);

  /* Stuff bins until successful */
  do {
    /* Increment the number of bins, allocate and initialize bins */
    if ((b = (ulg *)malloc(++m * sizeof(ulg))) == NULL)
    {
      free((voidp *)s);
      free((voidp *)e);
      err(ZE_MEM, "was trying a smart split");
    }
    b[0] = c - d;                       /* leave space in first bin */
    for (j = 1; j < m; j++)
      b[j] = c;

    /* Fill the bins greedily */
    for (i = 0; i < n; i++)
    {
      /* Find smallest bin that will hold item i (size s[i]) */
      t = c + 1;
      for (k = j = 0; j < m; j++)
        if (*s[i] <= b[j] && b[j] < t)
          t = b[k = j];

      /* If no bins big enough for *s[i], try next m */
      if (t == c + 1)
        break;

      /* Diminish that bin and save where it goes */
      b[k] -= *s[i];
      a[(int)((ulg huge *)(s[i]) - (ulg huge *)e)] = k;
    }

    /* Clean up */
    free((voidp *)b);

    /* Do until all items put in a bin */
  } while (i < n);

  /* Done--clean up and return the number of bins needed */
  free((voidp *)s);
  free((voidp *)e);
  return m;
}


void main(argc, argv)
int argc;               /* number of tokens in command line */
char **argv;            /* command line tokens */
/* Split a zip file into several zip files less than a specified size.  See
   the command help in help() above. */
{
  ulg *a;               /* malloc'ed list of sizes, dest bins */
  extent *b;            /* heads of bin linked lists (malloc'ed) */
  ulg c;                /* bin capacity, start of central directory */
  int d;                /* if true, just report the number of disks */
  FILE *e;              /* input zip file */
  FILE *f;              /* output index and zip files */
  extent g;             /* number of bins from greedy(), entry to write */
  int h;                /* how to split--true means simple split, counter */
  ulg i;                /* size of index file or zero if none */
  extent j;             /* steps through zip entries, bins */
  int k;                /* next argument type */
  ulg *p;               /* malloc'ed list of sizes, dest bins for greedy() */
  char *q;              /* steps through option characters */
  int r;                /* temporary variable, counter */
  extent s;             /* number of bins needed */
  ulg t;                /* total of sizes, end of central directory */
  struct zlist far **w; /* malloc'ed table for zfiles linked list */
  int x;                /* if true, make an index file */
  struct zlist far *z;  /* steps through zfiles linked list */


  /* If no args, show help */
  if (argc == 1)
  {
    help();
    exit(0);
  }

  init_upper();           /* build case map table */

  /* Go through args */
  signal(SIGINT, handler);
  signal(SIGTERM, handler);
  k = h = x = d = 0;
  c = DEFSIZ;
  for (r = 1; r < argc; r++)
    if (*argv[r] == '-')
      if (argv[r][1])
        for (q = argv[r]+1; *q; q++)
          switch(*q)
          {
            case 'b':   /* Specify path for output files */
              if (k)
                err(ZE_PARMS, "options are separate and precede zip file");
              else
                k = 1;          /* Next non-option is path */
              break;
            case 'h':   /* Show help */
              help();  exit(0);
            case 'i':   /* Make an index file */
              x = 1;
              break;
            case 'l': case 'L':  /* Show copyright and disclaimer */
              license();  exit(0);
            case 'n':   /* Specify maximum size of resulting zip files */
              if (k)
                err(ZE_PARMS, "options are separate and precede zip file");
              else
                k = 2;          /* Next non-option is size */
              break;
            case 's':
              h = 1;    /* Only try simple */
              break;
            case 't':   /* Just report number of disks */
              d = 1;
              break;
            default:
              err(ZE_PARMS, "unknown option");
          }
      else
        err(ZE_PARMS, "zip file cannot be stdin");
    else
      if (k == 0)
        if (zipfile == NULL)
        {
          if ((zipfile = ziptyp(argv[r])) == NULL)
            err(ZE_MEM, "was processing arguments");
        }
        else
          err(ZE_PARMS, "can only specify one zip file");
      else if (k == 1)
      {
        tempath = argv[r];
        k = 0;
      }
      else              /* k must be 2 */
      {
        if ((c = (ulg)atol(argv[r])) < 100)     /* 100 is smallest zip file */
          err(ZE_PARMS, "invalid size given");
        k = 0;
      }
  if (zipfile == NULL)
    err(ZE_PARMS, "need to specify zip file");


  /* Read zip file */
  if ((r = readzipfile()) != ZE_OK)
    err(r, zipfile);
  if (zfiles == NULL)
    err(ZE_NAME, zipfile);

  /* Make a list of sizes and check against capacity.  Also compute the
     size of the index file. */
  c -= ENDHEAD + 4;                     /* subtract overhead/zipfile */
  if ((a = (ulg *)malloc(zcount * sizeof(ulg))) == NULL ||
      (w = (struct zlist far **)malloc(zcount * sizeof(struct zlist far *))) ==
       NULL)
  {
    if (a != NULL)
      free((voidp *)a);
    err(ZE_MEM, "was computing split");
    return;
  }
  i = t = 0;
  for (j = 0, z = zfiles; j < zcount; j++, z = z->nxt)
  {
    w[j] = z;
    if (x)
      i += z->nam + 6 + NL;
    t += a[j] = 8 + LOCHEAD + CENHEAD +
           2 * (ulg)z->nam + 2 * (ulg)z->ext + z->com + z->siz;
    if (a[j] > c)
    {
      free((voidp *)w);  free((voidp *)a);
      err(ZE_BIG, z->zname);
    }
  }

  /* Decide on split to use, report number of files */
  if (h)
    s = simple(a, zcount, c, i);
  else
  {
    if ((p = (ulg *)malloc(zcount * sizeof(ulg))) == NULL)
    {
      free((voidp *)w);  free((voidp *)a);
      err(ZE_MEM, "was computing split");
    }
    memcpy((char *)p, (char *)a, zcount * sizeof(ulg));
    s = simple(a, zcount, c, i);
    g = greedy(p, zcount, c, i);
    if (s <= g)
      free((voidp *)p);
    else
    {
      free((voidp *)a);
      a = p;
      s = g;
    }
  }
  printf("%d zip files w%s be made (%d%% efficiency)\n",
         s, d ? "ould" : "ill", ((200 * ((t + c - 1)/c)) / s + 1) >> 1);
  if (d)
  {
    free((voidp *)w);  free((voidp *)a);
    free((voidp *)zipfile);
    zipfile = NULL;
    return;
  }

  /* Set up path for output files */
  if ((path = malloc(tempath == NULL ? 13 : strlen(tempath) + 14)) == NULL)
    err(ZE_MEM, "was making output file names");
  if (tempath == NULL)
     name = path;
  else
  {
    strcpy(path, tempath);
    if (path[0] && path[strlen(path) - 1] != '/')
      strcat(path, "/");
    name = path + strlen(path);
  }

  /* Write the index file */
  if (x)
  {
    strcpy(name, INDEX);
    printf("creating %s\n", path);
    indexmade = 1;
    if ((f = fopen(path, "w")) == NULL)
    {
      free((voidp *)w);  free((voidp *)a);
      err(ZE_CREAT, path);
    }
    for (j = 0; j < zcount; j++)
      fprintf(f, "%5ld %s\n", a[j] + 1, w[j]->zname);
    if ((j = ferror(f)) != 0 || fclose(f))
    {
      if (j)
        fclose(f);
      free((voidp *)w);  free((voidp *)a);
      err(ZE_WRITE, path);
    }
  }

  /* Make linked lists of results */
  if ((b = (extent *)malloc(s * sizeof(extent))) == NULL)
  {
    free((voidp *)w);  free((voidp *)a);
    err(ZE_MEM, "was computing split");
  }
  for (j = 0; j < s; j++)
    b[j] = (extent)-1;
  j = zcount;
  while (j--)
  {
    g = (extent)a[j];
    a[j] = b[g];
    b[g] = j;
  }

  /* Make a name template for the zip files that is eight or less characters
     before the .zip, and that will not overwrite the original zip file. */
  for (k = 1, j = s; j >= 10; j /= 10)
    k++;
  if (k > 7)
  {
    free((voidp *)b);  free((voidp *)w);  free((voidp *)a);
    err(ZE_PARMS, "way too many zip files must be made");
  }
#ifdef VMS
  if ((q = strrchr(zipfile, ']')) != NULL)
#else /* !VMS */
  if ((q = strrchr(zipfile, '/')) != NULL)
#endif /* ?VMS */
    q++;
  else
    q = zipfile;
  r = 0;
  while ((g = *q++) != 0 && g != '.' && r < 8 - k)
    template[r++] = (char)g;
  if (r == 0)
    template[r++] = '_';
  else if (g >= '0' && g <= '9')
    template[r - 1] = (char)(template[r - 1] == '_' ? '-' : '_');
  sprintf(template + r, "%%0%dd.zip", k);

  /* Make the zip files from the linked lists of entry numbers */
  if ((e = fopen(zipfile, FOPR)) == NULL)
  {
    free((voidp *)b);  free((voidp *)w);  free((voidp *)a);
    err(ZE_NAME, zipfile);
  }
  free((voidp *)zipfile);
  zipfile = NULL;
  for (j = 0; j < s; j++)
  {
    sprintf(name, template, j + 1);
    printf("creating %s\n", path);
    zipsmade = j + 1;
    if ((f = fopen(path, FOPW)) == NULL)
    {
      free((voidp *)b);  free((voidp *)w);  free((voidp *)a);
      err(ZE_CREAT, path);
    }
    tempzn = 0;
    for (g = b[j]; g != (extent)-1; g = (extent)a[g])
    {
      if (fseek(e, w[g]->off, SEEK_SET))
      {
        free((voidp *)b);  free((voidp *)w);  free((voidp *)a);
        err(ferror(e) ? ZE_READ : ZE_EOF, zipfile);
      }
      if ((r = zipcopy(w[g], e, f)) != ZE_OK)
      {
        free((voidp *)b);  free((voidp *)w);  free((voidp *)a);
        if (r == ZE_TEMP)
          err(ZE_WRITE, path);
        else
          err(r, zipfile);
      }
    }
    if ((c = ftell(f)) == -1L)
    {
      free((voidp *)b);  free((voidp *)w);  free((voidp *)a);
      err(ZE_WRITE, path);
    }
    for (g = b[j], k = 0; g != (extent)-1; g = (extent)a[g], k++)
      if ((r = putcentral(w[g], f)) != ZE_OK)
      {
        free((voidp *)b);  free((voidp *)w);  free((voidp *)a);
        err(ZE_WRITE, path);
      }
    if ((t = ftell(f)) == -1L)
    {
      free((voidp *)b);  free((voidp *)w);  free((voidp *)a);
      err(ZE_WRITE, path);
    }
    if ((r = putend(k, t - c, c, (extent)0, (char *)NULL, f)) != ZE_OK)
    {
      free((voidp *)b);  free((voidp *)w);  free((voidp *)a);
      err(ZE_WRITE, path);
    }
    if (ferror(f) || fclose(f))
    {
      free((voidp *)b);  free((voidp *)w);  free((voidp *)a);
      err(ZE_WRITE, path);
    }
  }
  free((voidp *)b);  free((voidp *)w);  free((voidp *)a);
  fclose(e);

  /* Done! */
  exit(0);
}
