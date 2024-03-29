/* Copyright (C) 1990, 1993 Free Software Foundation, Inc.

   This file is part of GNU ISPELL.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifdef __STDC__
void alloc_tables (unsigned long, unsigned long);
void store_flags (int, int);
#else
void alloc_tables ();
void store_flags ();
#endif


#ifdef __STDC__

void write_binary (char *);
void write_ascii (char *);
void build (FILE *);
void toomanywords (void);
hash_index find_slot (int);
struct hash_table_entry *make_hash_table_entry (comp_char *, int);

#else

void write_binary ();
void write_ascii ();
void build ();
void toomanywords ();
hash_index find_slot ();
struct hash_table_entry *make_hash_table_entry ();

#endif
