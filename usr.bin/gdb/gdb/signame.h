/* Convert between signal names and numbers.
   Copyright 1990, 1992 Free Software Foundation, Inc.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#if !defined (SIGNAME_H)
#define SIGNAME_H 1

/* Names for signals from 0 to NSIG-1.  */
extern char *sys_siglist[];

/* Return the abbreviation (e.g. ABRT, FPE, etc.) for signal NUMBER.
   Do not return this as a const char *.  The caller might want to
   assign it to a char *.  */

extern char *
sig_abbrev PARAMS ((int));

/* Return the signal number for an ABBREV, or -1 if there is no
   signal by that name.  */

extern int
sig_number PARAMS ((const char *));

#ifndef PSIGNAL_IN_SIGNAL_H
/* Print to standard error the name of SIGNAL, preceded by MESSAGE and
   a colon, and followed by a newline.  */

extern void
psignal PARAMS ((unsigned, const char *));
#endif

#endif	/* !defined (SIGNAME_H) */
