/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*	Krystal $Id: cwd.h,v 1.2 1994/01/31 00:50:44 polk Exp $ */

#include <sys/stat.h>

static inline u_char *
ustrcpy(u_char *s1, u_char *s2)
{
    return((u_char *)strcpy((char *)s1, (char *)s2));
}

static inline u_char *
ustrcat(u_char *s1, u_char *s2)
{
    return((u_char *)strcat((char *)s1, (char *)s2));
}

static inline u_char *
ustrncpy(u_char *s1, u_char *s2, unsigned n)
{
    return((u_char *)strncpy((char *)s1, (char *)s2, n));
}

static inline int
ustrcmp(u_char *s1, u_char *s2)
{
    return(strcmp((char *)s1, (char *)s2));
}

static inline int
ustrncmp(u_char *s1, u_char *s2, unsigned n)
{
    return(strncmp((char *)s1, (char *)s2, n));
}

static inline int
ustrlen(u_char *s)
{
    return(strlen((char *)s));
}

static inline u_char *
ustrrchr(u_char *s, u_char c)
{
    return((u_char *)strrchr((char *)s, c));
}

static inline u_char *
ustrdup(u_char *s)
{
    return((u_char *)strdup((char *)s));
}

static inline int
ustat(u_char *s, struct stat *sb)
{
    return(stat((char *)s, sb));
}

static inline int
uaccess(u_char *s, int mode)
{
    return(access((char *)s, mode));
}

void   init_path(int drive, u_char *base, u_char *where);
void   dos_makereadonly(int drive);
int    dos_readonly(int drive);
u_char *dos_getcwd(int drive);
u_char *dos_getpath(int drive);
int    dos_makepath(int drive, u_char *where, u_char *newpath);
int    dos_setcwd(int drive, u_char *where);
int    dos_to_real_path(int drive, u_char *dospath, u_char *realpath);
void   real_to_dos(u_char *real, u_char *dos);
void   dos_to_real(u_char *dos, u_char *real);
u_char **get_entries(u_char *path);
int    get_space(int drive, fsstat_t *fs);
int    find_first(int drive, u_char *path, int attr,
		  dosdir_t *dir, find_block_t *dta);
int    find_next(dosdir_t *dir, find_block_t *dta);
