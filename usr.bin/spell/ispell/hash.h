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

typedef unsigned short str_index;
typedef unsigned short hash_index;
typedef unsigned char comp_char;

struct hash_table_entry
  {
    hash_index next;
    union
      {
	struct short_word
	  {
	    comp_char data[6];
	  } s;
	struct long_word
	  {
	    unsigned char short_flag;
	    unsigned char len;
	    comp_char data[2];
	    str_index sindex;
	  } l;
      } u;
  };


/* EMPTY or END close collision chains */
#define HASH_EMPTY        ((hash_index)0xffff)
#define HASH_END          ((hash_index)0xfffe)
#define HASH_SPECIAL	  ((hash_index)0xfff0)


struct hash_header
  {
    unsigned long magic;
    unsigned long version;
    unsigned long lexletters;
    unsigned long lexsize;
    unsigned long hashsize;
    unsigned long stblsize;
    unsigned char used[32];	/* 256 bits for which characters used */
  };

/* 'HASH'compressed using an early table (!) */
#define HMAGIC ('I' + ('S'<<8) + ((long)'P'<<16) + ((long)'L'<<24))

#define VERSION 3

#define V_FLAG      1
#define N_FLAG      2
#define X_FLAG      4
#define H_FLAG      8
#define Y_FLAG 0x0010
#define G_FLAG 0x0020
#define J_FLAG 0x0040
#define D_FLAG 0x0080
#define T_FLAG 0x0100
#define R_FLAG 0x0200
#define Z_FLAG 0x0400
#define S_FLAG 0x0800
#define P_FLAG 0x1000
#define M_FLAG 0x2000

#define flag_char(f) ("vnxhygjdtrzspm??"[f])

extern hash_index hashsize;






#define VOWEL 1
#define SXZH 2
#define Y 4
#define LEXLETTER 8

#define isvowel(c) ((ctbl+1)[(unsigned char)(c)] & VOWEL)
#define issxzh(c)  ((ctbl+1)[(unsigned char)(c)] & SXZH)
#define issxzhy(c) ((ctbl+1)[(unsigned char)(c)] & (SXZH | Y))
#define islexletter(c) ((ctbl+1)[(unsigned char)(c)] & LEXLETTER)

extern unsigned char ctbl[];

#ifdef __STDC__

str_index copy_out_string (comp_char *, int);
int hash_emptyp (int);
hash_index hash_find_end (int);
void hash_store (int, struct hash_table_entry *);
void hash_retrieve (int, struct hash_table_entry *);
void hash_set_next (int, int);
hash_index hash_lookup (comp_char *, int);
int hash_init (char *);
unsigned short get_flags (int);
char *htetostr (struct hash_table_entry *, int);
unsigned int hash (char *);

#else

extern str_index copy_out_string ();
extern int hash_emptyp ();
extern hash_index hash_find_end ();
extern void hash_store ();
extern void hash_retrieve ();
extern void hash_set_next ();
extern hash_index hash_lookup ();
extern int hash_init ();
extern unsigned short get_flags ();
extern char *htetostr ();
extern unsigned int hash ();

#endif
