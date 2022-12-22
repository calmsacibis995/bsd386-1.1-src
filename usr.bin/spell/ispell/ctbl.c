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

#include <stdio.h>
#include "ispell.h"
#include "hash.h"

#define L LEXLETTER
#define V VOWEL
#define S SXZH

unsigned char ctbl[257] =
{
  0,
/*	0 - 7			 			*/
  0, 0, 0, 0, 0, 0, 0, 0,
/* 	010 - 017		 			*/
  0, 0, 0, 0, 0, 0, 0, 0,
/* 	020 - 027 					*/
  0, 0, 0, 0, 0, 0, 0, 0,
/* 	030 - 037 					*/
  0, 0, 0, 0, 0, 0, 0, 0,
/* 	   !  "  #  $  %  &  '	<- don't miss that "'" is a letter */
  0, 0, 0, 0, 0, 0, 0, L,
/*	(  )  *  +  ,  -  .  /   			*/
  0, 0, 0, 0, 0, 0, 0, 0,
/*	0  1  2  3  4  5  6  7   			*/
  0, 0, 0, 0, 0, 0, 0, 0,
/*	8  9  :  ;  <  =  >  ?   			*/
  0, 0, 0, 0, 0, 0, 0, 0,
/*	@    A    B    C    D    E    F    G		*/
  0, L | V, L, L, L, L | V, L, L,
/*	H    I    J    K    L    M    N    O   		*/
  L | S, L | V, L, L, L, L, L, L | V,
/*	P    Q    R    S    T    U    V    W   		*/
  L, L, L, L | S, L, L | V, L, L,
/*	X    Y    Z    [    \    ]    ^    _   		*/
  L | S, L | Y, L | S, 0, 0, 0, 0, 0,
/*	`    a    b    c    d    e    f    g   		*/
  0, L | V, L, L, L, L | V, L, L,
/*	h    i    j    k    l    m    n    o   		*/
  L | S, L | V, L, L, L, L, L, L | V,
/*	p    q    r    s    t    u    v    w   		*/
  L, L, L, L | S, L, L | V, L, L,
/*	x    y    z    {    |    }    ~   		*/
  L | S, L | Y, L | S, 0, 0, 0, 0, 0,
};
