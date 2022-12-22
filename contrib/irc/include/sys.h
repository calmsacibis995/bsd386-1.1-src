/************************************************************************
 *   IRC - Internet Relay Chat, include/sys.h
 *   Copyright (C) 1990 University of Oulu, Computing Center
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

#ifndef	__sys_include__
#define __sys_include__
#ifdef ISC202
#include <net/errno.h>
#else
#include <sys/errno.h>
#endif

#if !defined(mips) || defined(SGI) || defined(ULTRIX)
# if !defined(NEXT)
#include <unistd.h>
# endif
#endif
#if defined(HPUX) || defined(sun)
#include <stdlib.h>
#endif

#if defined(HPUX) || defined(VMS) || defined(AIX) || defined(SOL20) || \
    defined(ESIX) || defined(DYNIXPTX) || defined(SVR4)
#include <string.h>
#define bcopy(a,b,s)  memcpy(b,a,s)
#define bzero(a,s)    memset(a,0,s)
#define bcmp          memcmp
# ifndef AIX
extern char *strchr(), *strrchr();
extern char *inet_ntoa();
#define index strchr
#define rindex strrchr
# endif
#else 
# if !defined(DYNIXPTX)
#include <strings.h>
#  if !defined(NEXT)
extern	char	*index();
extern	char	*rindex();
#  endif
# endif
#endif
#define	strcasecmp	mycmp
#define	strncasecmp	myncmp

#ifdef AIX
#include <sys/select.h>
#endif
#if defined(HPUX )|| defined(AIX)
#include <time.h>
#ifdef AIX
#include <sys/time.h>
#endif
#else
#include <sys/time.h>
#endif

#ifdef NEXT
#define VOIDSIG int	/* whether signal() returns int of void */
#else
#define VOIDSIG void	/* whether signal() returns int of void */
#endif

extern	VOIDSIG	dummy();

#ifdef	DYNIXPTX
#define	bcopy(a,b,s)	memcpy(b,a,s)
#define	bzero(a,s)	memset(a,0,s)
#define	bcmp		memcmp
#define index strchr
#define rindex strrchr
#define	NO_U_TYPES
#endif

#ifdef	NO_U_TYPES
typedef	unsigned char	u_char;
typedef	unsigned short	u_short;
typedef	unsigned long	u_long;
typedef	unsigned int	u_int;
#endif

#ifdef	USE_VARARGS
#include <varargs.h>
#endif

#endif /* __sys_include__ */
