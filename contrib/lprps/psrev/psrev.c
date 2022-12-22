/* psrev.c */

#ifndef lint
static char rcsid[] = "$Id: psrev.c,v 1.2 1994/01/13 17:53:44 sanders Exp $";
#endif

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#ifndef errno
extern int errno;
#endif

char *malloc();
char *realloc();

char *xmalloc();
char *xrealloc();
char *prog;

void sys_error(message, arg)
char *message, *arg;
{
  extern char *sys_errlist[];
  extern int sys_nerr;
  int en;

  en = errno;
  fprintf(stderr, "%s: ", prog);
  fprintf(stderr, message, arg);
  if (en > 0 && en < sys_nerr)
    fprintf(stderr, ": %s\n", sys_errlist[en]);
  else
    putc('\n', stderr);
  exit(1);
}

typedef struct line_buf {
  char *buf;
  int len;
  int size;
} line_buf;

int get_line(lb, fp)
line_buf *lb;
FILE *fp;
{
  int left;
  char *ptr;
  if (lb->buf == NULL) {
    lb->size = 16;
    lb->buf = xmalloc(16);
  }
  ptr = lb->buf;
  left = lb->size - 1;
  for (;;) {
    int c = getc(fp);
    if (c == EOF) {
      if (ferror(fp))
	sys_error("read error", (char *)0);
      if (ptr == lb->buf)
	return 0;
      lb->len = ptr - lb->buf;
      *ptr++ = '\0';
      return 1;
    }
    if (left <= 0) {
      int n = ptr - lb->buf;
      lb->size *= 2;
      lb->buf = xrealloc(lb->buf, lb->size);
      left = lb->size - n - 1;
      ptr = lb->buf + n;
    }
    *ptr++ = c;
    left -= 1;
    if (c == '\n') {
      lb->len = ptr - lb->buf;
      *ptr++ = '\0';
      return 1;
    }
  }
}

void put_line(lb, fp)
line_buf *lb;
FILE *fp;
{
  if (fwrite(lb->buf, 1, lb->len, fp) != lb->len)
    sys_error("write error", (char *)0);
}

/* is s2 a prefix of s1? */
int strprefix(s1, s2)
char *s1, *s2;
{
  for (; *s1 != '\0' && *s1 == *s2; s1++, s2++)
    ;
  return *s2 == '\0';
}

void copy_and_exit(fp)
FILE *fp;
{
  int c;
  while ((c = getc(fp)) != EOF)
    if (putchar(c) == EOF)
      sys_error("write error", (char *)0);
  exit(0);
}

typedef struct page_list {
  long pos;
  struct page_list *next;
} page_list;

void usage()
{
  fprintf(stderr, "usage: %s [file]\n", prog);
  exit(1);
}

main(argc, argv)
int argc;
char **argv;
{
  int dont_reverse = 0;
  int pageno;
  int pending_line = 0;
  int had_page_order = 0;
  int page_order_atend = 0;
  FILE *tempfp = NULL;
  page_list *pl = 0;
  long trailer_pos = -1;
  long prev_pos;
  int level = 0;
  line_buf lb;
  struct stat sb;

  prog = argv[0];
  lb.buf = 0;
  if (argc > 2)
    usage();
  if (argc == 2) {
    errno = 0;
    if (!freopen(argv[1], "r", stdin))
      sys_error("can't open `%s'", argv[1]);
  }
  if (!get_line(&lb, stdin))
    exit(0);
  put_line(&lb, stdout);
  if (!strprefix(lb.buf, "%!PS-Adobe-"))
    copy_and_exit(stdin);

  /* process the header section */
  while (get_line(&lb, stdin)) {
    int suppress = 0;
    if (!strprefix(lb.buf, "%%")) {
      pending_line = 1;
      break;
    }
    else if (strprefix(lb.buf, "%%PageOrder:")) {
      /* the first %%PageOrder comment is the significant one */
      if (had_page_order) 
	suppress = 1;
      else {
	char *ptr = lb.buf + sizeof("%%PageOrder:") - 1;

	had_page_order = 1;
	while (*ptr == ' ')
	  ptr++;
	if (strprefix(ptr, "(atend)"))
	  page_order_atend = 1;
	else if (strprefix(ptr, "Ascend")) {
	  fputs("%%PageOrder: Descend\n", stdout);
	  suppress = 1;
	}
	else
	  dont_reverse = 1;
      }
    }
    if (strprefix(lb.buf, "%%EndComments"))
      break;
    if (!suppress)
      put_line(&lb, stdout);
  }

  if (!had_page_order)
    fputs("%%PageOrder: Descend\n", stdout);
  
  printf("%%%%EndComments\n");

  if (dont_reverse)
    copy_and_exit(stdin);

  /* process the prologue */
  while (pending_line || get_line(&lb, stdin)) {
    pending_line = 0;
    if (strprefix(lb.buf, "%%BeginDocument:"))
      ++level;
    else if (strprefix(lb.buf, "%%EndDocument")) {
      if (level > 0)
	--level;
    }
    else if (level == 0 && strprefix(lb.buf, "%%Page:")) {
      pending_line = 1;
      break;
    }
    put_line(&lb, stdout);
  }

  /* if we didn't find any %%Page comments, we're done */
  if (!pending_line)
    exit(0);

  /* open a temporary file if necessary */
  if (fstat(fileno(stdin), &sb) < 0)
    sys_error("cannot stat stdin", (char *)0);
  if ((sb.st_mode & S_IFMT) != S_IFREG) {
    tempfp = tmpfile();
    if (!tempfp)
      sys_error("can't open temporary file", (char *)0);
  }

  /* process the body */
  while (pending_line || get_line(&lb, stdin)) {
    pending_line = 0;
    if (strprefix(lb.buf, "%%BeginDocument:"))
      ++level;
    else if (strprefix(lb.buf, "%%EndDocument")) {
      if (level > 0)
	--level;
    }
    else if (level == 0) {
      if (strprefix(lb.buf, "%%Page:")) {
	page_list *tem = (page_list *)xmalloc(sizeof(page_list));
	tem->next = pl;
	tem->pos = tempfp ? ftell(tempfp) : ftell(stdin) - lb.len;
	pl = tem;
      }
      else if (strprefix(lb.buf, "%%Trailer")) {
	pending_line = 1;
	break;
      }
    }
    if (tempfp != NULL)
      put_line(&lb, tempfp);
  }

  /* process the trailer */
  if (pending_line) {
    trailer_pos = tempfp ? ftell(tempfp) : ftell(stdin) - lb.len;
    while (pending_line || get_line(&lb, stdin)) {
      pending_line = 0;
      if (page_order_atend && strprefix(lb.buf, "%%PageOrder:")) {
	char *p = lb.buf + sizeof("%%PageOrder:") - 1;
	while (*p == ' ')
	  p++;
	dont_reverse = !strprefix(p, "Ascend");
      }
      if (tempfp != NULL)
	put_line(&lb, tempfp);
    }
  }

  if (tempfp == NULL)
    tempfp = stdin;

  if (dont_reverse) {
    long first_page_pos;
    if (pl == NULL)
      abort();
    /* find the position of the first page */
    while (pl != NULL) {
      page_list *tem = pl;
      first_page_pos = pl->pos;
      pl = pl->next;
      free((char *)tem);
    }
    if (fseek(tempfp, first_page_pos, 0) < 0)
      sys_error("seek error", (char *)0);
    copy_and_exit(tempfp);
  }

  /* output each page */
  prev_pos = trailer_pos == -1 ? ftell(tempfp) : trailer_pos;
  pageno = 1;
  while (pl != NULL) {
    char *ptr, *label;
    int count = prev_pos - pl->pos;
    if (fseek(tempfp, pl->pos, 0) < 0)
      sys_error("seek error", (char *)0);
    if (!get_line(&lb, tempfp))
      abort();
    if (!strprefix(lb.buf, "%%Page:"))
      abort();
    ptr = lb.buf + 7;
    while (*ptr == ' ')
      ptr++;
    label = ptr;
    while (*ptr != '\0' && !(isascii(*ptr) && isspace(*ptr)))
      ptr++;
    *ptr = '\0';
    if (*label == '\0')
      label = "?";
    printf("%%%%Page: %s %d\n", label, pageno);
    pageno += 1;
    count -= lb.len;
    while (--count >= 0) {
      int c = getc(tempfp);
      if (c == EOF)
	abort();
      if (putc(c, stdout) == EOF)
	sys_error("write error", (char *)0);
    }
    prev_pos = pl->pos;
    pl = pl->next;
  }

  /* output the trailer if there is one */
  if (trailer_pos != -1) {
    if (fseek(tempfp, trailer_pos, 0) < 0)
      sys_error("seek error", (char *)0);
    if (page_order_atend) {
      /* we need to change the %%PageOrder comment */
      while (get_line(&lb, tempfp)) {
	if (page_order_atend && strprefix(lb.buf, "%%PageOrder:"))
	  fputs("%%PageOrder: Descend\n", stdout);
	else
	  put_line(&lb, stdout);
      }
    }
    else
      copy_and_exit(tempfp);
  }

  if (ferror(stdout))
    sys_error("write error", (char *)0);

  exit(0);
}

char *xmalloc(size)
int size;
{
  char *tem;
  if ((tem = malloc(size)) == NULL) {
    fprintf(stderr, "%s: out of memory\n", prog);
    exit(1);
  }
  return tem;
}

char *xrealloc(ptr, size)
char *ptr;
int size;
{
  char *tem;
  if ((tem = realloc(ptr, size)) == NULL) {
    fprintf(stderr, "%s: out of memory\n", prog);
    exit(1);
  }
  return tem;
}
