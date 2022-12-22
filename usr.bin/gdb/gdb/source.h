/* Definitions for finding out about GDB source files.
   Copyright (C) 1992  Free Software Foundation, Inc.

This file is part of GDB.

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

#if !defined (SOURCE_H)
#define SOURCE_H

int
get_filename_and_charpos PARAMS ((struct symtab *, char **));

int
source_line_charpos PARAMS ((struct symtab *, int));
     struct symtab *s;

int
source_charpos_line PARAMS ((struct symtab *, int));

#endif	/* !defined(SOURCE_H) */
