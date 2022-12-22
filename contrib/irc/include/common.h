/************************************************************************
 *   IRC - Internet Relay Chat, include/common.h
 *   Copyright (C) 1990 Armin Gruner
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 1, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef	__common_include__
#define __common_include__

#ifndef PROTO
#if __STDC__
#	define PROTO(x)	x
#else
#	define PROTO(x)	()
#endif
#endif

#ifndef NULL
#define NULL 0
#endif

#ifdef TRUE
#undef TRUE
#endif

#ifdef FALSE
#undef FALSE
#endif

#define FALSE (0)
#define TRUE  (!FALSE)

#if defined(mips) || defined(pyr) || defined(apollo) || (defined(sequent) &&\
    !defined(DYNIXPTX)) || defined(__convex__) ||\
    (defined(BSD) && !defined(sun) && !defined(ultrix) && !defined(__osf__))
char	*malloc(), *calloc();
void	free();
#else
# if defined(NEXT)
#include <sys/malloc.h>
# else
#include <malloc.h>
# endif
#endif

extern	int	matches PROTO((char *, char *));
extern	int	mycmp PROTO((char *, char *));
extern	int	myncmp PROTO((char *, char *, int));
#ifdef NEED_STRTOK
extern	char	*strtok PROTO((char *, char *));
#endif
#ifdef NEED_STRTOKEN
extern	char	*strtoken PROTO((char **, char *, char *));
#endif
#ifdef NEED_INET_ADDR
extern unsigned long inet_addr PROTO((char *));
#endif

#if defined(NEED_INET_NTOA) || defined(NEED_INET_NETOF)
#include <netinet/in.h>
#endif

#ifdef NEED_INET_NTOA
extern char *inet_ntoa PROTO((struct in_addr));
#endif

#ifdef NEED_INET_NETOF
extern int inet_netof PROTO((struct in_addr));
#endif

extern char *myctime PROTO((time_t));
extern char *strtoken PROTO((char **, char *, char *));

#if defined(ULTRIX) || defined(SGI) || defined(sequent) || defined(HPUX) || \
    defined(OSF)
#include <sys/param.h>
#endif

#ifndef MAX
#define MAX(a, b)	((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a, b)	((a) < (b) ? (a) : (b))
#endif

#if !defined(DEBUGMODE) || defined(CLIENT_COMPILE)
#define MyFree(x)       if ((x) != NULL) free(x)
#else
#define	free(x)		MyFree(x)
#endif
#define DupString(x,y) do{x=MyMalloc(strlen(y)+1);(void)strcpy(x,y);}while(0)

extern unsigned char tolowertab[];

#undef tolower
#define tolower(c) (tolowertab[(u_char)(c)])

extern unsigned char touppertab[];

#undef toupper
#define toupper(c) (touppertab[(u_char)(c)])

#undef isalpha
#undef isdigit
#undef isalnum
#undef isprint
#undef isascii
#undef isgraph
#undef ispunct
#undef islower
#undef isupper
#undef isspace

extern unsigned char char_atribs[];

#define PRINT 1
#define CNTRL 2
#define ALPHA 4
#define PUNCT 8
#define DIGIT 16
#define SPACE 32

#define isalpha(c) (char_atribs[(u_char)(c)]&ALPHA)
#define isspace(c) (char_atribs[(u_char)(c)]&SPACE)
#define islower(c) ((char_atribs[(u_char)(c)]&ALPHA) && ((u_char)(c) > 0x5f))
#define isupper(c) ((char_atribs[(u_char)(c)]&ALPHA) && ((u_char)(c) < 0x60))
#define isdigit(c) (char_atribs[(u_char)(c)]&DIGIT)
#define isalnum(c) (char_atribs[(u_char)(c)]&(DIGIT|ALPHA))
#define isprint(c) (char_atribs[(u_char)(c)]&PRINT)
#define isascii(c) ((u_char)(c) >= 0 && (u_char)(c) <= 0x7f)
#define isgraph(c) ((char_atribs[(u_char)(c)]&PRINT) && ((u_char)(c) != 0x32))
#define ispunct(c) (!(char_atribs[(u_char)(c)]&(CNTRL|ALPHA|DIGIT)))

extern char *MyMalloc();
extern void flush_connections();
extern struct SLink *find_user_link(/* struct SLink *, struct Client * */);

#endif /* __common_include__ */
