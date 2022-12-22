/* Breadth-first and depth-first routines for
   searching multiple-inheritance lattice for GNU C++.
   Copyright (C) 1987, 1989, 1992, 1993 Free Software Foundation, Inc.
   Contributed by Michael Tiemann (tiemann@cygnus.com)

This file is part of GNU CC.

GNU CC is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU CC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GNU CC; see the file COPYING.  If not, write to
the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.  */

/* High-level class interface. */

#include "config.h"
#include "tree.h"
#include <stdio.h>
#include "cp-tree.h"
#include "obstack.h"
#include "flags.h"

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free free

void init_search ();
extern struct obstack *current_obstack;

#include "stack.h"

/* Obstack used for remembering decision points of breadth-first.  */
static struct obstack search_obstack;

/* Methods for pushing and popping objects to and from obstacks.  */
struct stack_level *
push_stack_level (obstack, tp, size)
     struct obstack *obstack;
     char *tp;  /* Sony NewsOS 5.0 compiler doesn't like void * here.  */
     int size;
{
  struct stack_level *stack;
  obstack_grow (obstack, tp, size);
  stack = (struct stack_level *) ((char*)obstack_next_free (obstack) - size);
  obstack_finish (obstack);
  stack->obstack = obstack;
  stack->first = (tree *) obstack_base (obstack);
  stack->limit = obstack_room (obstack) / sizeof (tree *);
  return stack;
}

struct stack_level *
pop_stack_level (stack)
     struct stack_level *stack;
{
  struct stack_level *tem = stack;
  struct obstack *obstack = tem->obstack;
  stack = tem->prev;
  obstack_free (obstack, tem);
  return stack;
}

#define search_level stack_level
static struct search_level *search_stack;

static tree lookup_field_1 ();
static int lookup_fnfields_1 ();
static void dfs_walk ();
static int markedp ();
static void dfs_unmark ();
static void dfs_init_vbase_pointers ();

static tree vbase_types;
static tree vbase_decl, vbase_decl_ptr;
static tree vbase_decl_ptr_intermediate;
static tree vbase_init_result;

/* Allocate a level of searching.  */
static struct search_level *
push_search_level (stack, obstack)
     struct stack_level *stack;
     struct obstack *obstack;
{
  struct search_level tem;

  tem.prev = stack;
  return push_stack_level (obstack, (char *)&tem, sizeof (tem));
}

/* Discard a level of search allocation.  */
static struct search_level *
pop_search_level (obstack)
     struct stack_level *obstack;
{
  register struct search_level *stack = pop_stack_level (obstack);

  return stack;
}

/* Search memoization.  */
struct type_level
{
  struct stack_level base;

  /* First object allocated in obstack of entries.  */
  char *entries;

  /* Number of types memoized in this context.  */
  int len;

  /* Type being memoized; save this if we are saving
     memoized contexts.  */
  tree type;
};

/* Obstack used for memoizing member and member function lookup.  */

static struct obstack type_obstack, type_obstack_entries;
static struct type_level *type_stack;
static tree _vptr_name;

/* Make things that look like tree nodes, but allocate them
   on type_obstack_entries.  */
static int my_tree_node_counter;
static tree my_tree_cons (), my_build_string ();

extern int flag_memoize_lookups, flag_save_memoized_contexts;

/* Variables for gathering statistics.  */
static int my_memoized_entry_counter;
static int memoized_fast_finds[2], memoized_adds[2], memoized_fast_rejects[2];
static int memoized_fields_searched[2];
static int n_fields_searched;
static int n_calls_lookup_field, n_calls_lookup_field_1;
static int n_calls_lookup_fnfields, n_calls_lookup_fnfields_1;
static int n_calls_get_base_type;
static int n_outer_fields_searched;
static int n_contexts_saved;

/* Local variables to help save memoization contexts.  */
static tree prev_type_memoized;
static struct type_level *prev_type_stack;

#if NEW_CLASS_SCOPING
/* This list is used by push_class_decls to know what decls need to
   be pushed into class scope.  */
static tree closed_envelopes = NULL_TREE;
#endif

/* Allocate a level of type memoization context.  */
static struct type_level *
push_type_level (stack, obstack)
     struct stack_level *stack;
     struct obstack *obstack;
{
  struct type_level tem;

  tem.base.prev = stack;

  obstack_finish (&type_obstack_entries);
  tem.entries = (char *) obstack_base (&type_obstack_entries);
  tem.len = 0;
  tem.type = NULL_TREE;

  return (struct type_level *)push_stack_level (obstack, (char *)&tem, sizeof (tem));
}

/* Discard a level of type memoization context.  */

static struct type_level *
pop_type_level (stack)
     struct type_level *stack;
{
  obstack_free (&type_obstack_entries, stack->entries);
  return (struct type_level *)pop_stack_level ((struct stack_level *)stack);
}

/* Make something that looks like a TREE_LIST, but
   do it on the type_obstack_entries obstack.  */
static tree
my_tree_cons (purpose, value, chain)
     tree purpose, value, chain;
{
  tree p = (tree)obstack_alloc (&type_obstack_entries, sizeof (struct tree_list));
  ++my_tree_node_counter;
  TREE_TYPE (p) = NULL_TREE;
  ((HOST_WIDE_INT *)p)[3] = 0;
  TREE_SET_CODE (p, TREE_LIST);
  TREE_PURPOSE (p) = purpose;
  TREE_VALUE (p) = value;
  TREE_CHAIN (p) = chain;
  return p;
}

static tree
my_build_string (str)
     char *str;
{
  tree p = (tree)obstack_alloc (&type_obstack_entries, sizeof (struct tree_string));
  ++my_tree_node_counter;
  TREE_TYPE (p) = 0;
  ((int *)p)[3] = 0;
  TREE_SET_CODE (p, STRING_CST);
  TREE_STRING_POINTER (p) = str;
  TREE_STRING_LENGTH (p) = strlen (str);
  return p;
}

/* Memoizing machinery to make searches for multiple inheritance
   reasonably efficient.  */
#define MEMOIZE_HASHSIZE 8
typedef struct memoized_entry
{
  struct memoized_entry *chain;
  int uid;
  tree data_members[MEMOIZE_HASHSIZE];
  tree function_members[MEMOIZE_HASHSIZE];
} *ME;

#define MEMOIZED_CHAIN(ENTRY) (((ME)ENTRY)->chain)
#define MEMOIZED_UID(ENTRY) (((ME)ENTRY)->uid)
#define MEMOIZED_FIELDS(ENTRY,INDEX) (((ME)ENTRY)->data_members[INDEX])
#define MEMOIZED_FNFIELDS(ENTRY,INDEX) (((ME)ENTRY)->function_members[INDEX])
/* The following is probably a lousy hash function.  */
#define MEMOIZED_HASH_FN(NODE) (((long)(NODE)>>4)&(MEMOIZE_HASHSIZE - 1))

static struct memoized_entry *
my_new_memoized_entry (chain)
     struct memoized_entry *chain;
{
  struct memoized_entry *p =
    (struct memoized_entry *)obstack_alloc (&type_obstack_entries,
					    sizeof (struct memoized_entry));
  bzero (p, sizeof (struct memoized_entry));
  MEMOIZED_CHAIN (p) = chain;
  MEMOIZED_UID (p) = ++my_memoized_entry_counter;
  return p;
}

/* Make an entry in the memoized table for type TYPE
   that the entry for NAME is FIELD.  */

tree
make_memoized_table_entry (type, name, function_p)
     tree type, name;
     int function_p;
{
  int index = MEMOIZED_HASH_FN (name);
  tree entry, *prev_entry;

  memoized_adds[function_p] += 1;
  if (CLASSTYPE_MTABLE_ENTRY (type) == 0)
    {
      obstack_ptr_grow (&type_obstack, type);
      obstack_blank (&type_obstack, sizeof (struct memoized_entry *));
      CLASSTYPE_MTABLE_ENTRY (type) = (char *)my_new_memoized_entry ((struct memoized_entry *)0);
      type_stack->len++;
      if (type_stack->len * 2 >= type_stack->base.limit)
	my_friendly_abort (88);
    }
  if (function_p)
    prev_entry = &MEMOIZED_FNFIELDS (CLASSTYPE_MTABLE_ENTRY (type), index);
  else
    prev_entry = &MEMOIZED_FIELDS (CLASSTYPE_MTABLE_ENTRY (type), index);

  entry = my_tree_cons (name, NULL_TREE, *prev_entry);
  *prev_entry = entry;

  /* Don't know the error message to give yet.  */
  TREE_TYPE (entry) = error_mark_node;

  return entry;
}

/* When a new function or class context is entered, we build
   a table of types which have been searched for members.
   The table is an array (obstack) of types.  When a type is
   entered into the obstack, its CLASSTYPE_MTABLE_ENTRY
   field is set to point to a new record, of type struct memoized_entry.

   A non-NULL TREE_TYPE of the entry contains a visibility error message.

   The slots for the data members are arrays of tree nodes.
   These tree nodes are lists, with the TREE_PURPOSE
   of this list the known member name, and the TREE_VALUE
   as the FIELD_DECL for the member.

   For member functions, the TREE_PURPOSE is again the
   name of the member functions for that class,
   and the TREE_VALUE of the list is a pairs
   whose TREE_PURPOSE is a member functions of this name,
   and whose TREE_VALUE is a list of known argument lists this
   member function has been called with.  The TREE_TYPE of the pair,
   if non-NULL, is an error message to print.  */

/* Tell search machinery that we are entering a new context, and
   to update tables appropriately.

   TYPE is the type of the context we are entering, which can
   be NULL_TREE if we are not in a class's scope.

   USE_OLD, if nonzero tries to use previous context.  */
void
push_memoized_context (type, use_old)
     tree type;
     int use_old;
{
  int len;
  tree *tem;

  if (prev_type_stack)
    {
      if (use_old && prev_type_memoized == type)
	{
#ifdef GATHER_STATISTICS
	  n_contexts_saved++;
#endif
	  type_stack = prev_type_stack;
	  prev_type_stack = 0;

	  tem = &type_stack->base.first[0];
	  len = type_stack->len;
	  while (len--)
	    CLASSTYPE_MTABLE_ENTRY (tem[len*2]) = (char *)tem[len*2+1];
	  return;
	}
      /* Otherwise, need to pop old stack here.  */
      type_stack = pop_type_level (prev_type_stack);
      prev_type_memoized = 0;
      prev_type_stack = 0;
    }

  type_stack = push_type_level ((struct stack_level *)type_stack,
				&type_obstack);
  type_stack->type = type;
}

/* Tell search machinery that we have left a context.
   We do not currently save these contexts for later use.
   If we wanted to, we could not use pop_search_level, since
   poping that level allows the data we have collected to
   be clobbered; a stack of obstacks would be needed.  */
void
pop_memoized_context (use_old)
     int use_old;
{
  int len;
  tree *tem = &type_stack->base.first[0];

  if (! flag_save_memoized_contexts)
    use_old = 0;
  else if (use_old)
    {
      len = type_stack->len;
      while (len--)
	tem[len*2+1] = (tree)CLASSTYPE_MTABLE_ENTRY (tem[len*2]);

      prev_type_stack = type_stack;
      prev_type_memoized = type_stack->type;
    }

  if (flag_memoize_lookups)
    {
      len = type_stack->len;
      while (len--)
	CLASSTYPE_MTABLE_ENTRY (tem[len*2])
	  = (char *)MEMOIZED_CHAIN (CLASSTYPE_MTABLE_ENTRY (tem[len*2]));
    }
  if (! use_old)
    type_stack = pop_type_level (type_stack);
  else
    type_stack = (struct type_level *)type_stack->base.prev;
}

/* This is the newer recursive depth first search routine. */
static tree
get_binfo_recursive (binfo, is_private, parent, rval, rval_private_ptr, xtype,
		     friends, protect)
     tree binfo, parent, rval, xtype, friends;
     int *rval_private_ptr, protect, is_private;
{
  tree binfos;
  int i, n_baselinks;

  if (BINFO_TYPE (binfo) == parent)
    {
      if (rval == NULL_TREE)
	{
	  rval = binfo;
	  *rval_private_ptr = is_private;
	}
      else
	{
	  /* I believe it is the case that this error is only an error
	     when used by someone that wants error messages printed.
	     Routines that call this one, that don't set protect want
	     the first one found, even if there are more.  */
	  if (protect)
	    {
	      /* Found two or more possible return values.  */
	      cp_error ("type `%T' is ambiguous base class for type `%T'",
			  parent, xtype);
	      rval = error_mark_node;
	    }
	}
      return rval;
    }

  binfos = BINFO_BASETYPES (binfo);
  n_baselinks = binfos ? TREE_VEC_LENGTH (binfos) : 0;

  /* Process base types.  */
  for (i = 0; i < n_baselinks; i++)
    {
      tree base_binfo = TREE_VEC_ELT (binfos, i);

      if (BINFO_MARKED (base_binfo) == 0)
	{
	  int via_private = is_private || !TREE_VIA_PUBLIC (base_binfo);

	  SET_BINFO_MARKED (base_binfo);

	  if (via_private == 0)
	    ;
	  else if (protect == 0)
	    via_private = 0;
	  else if (protect == 1 && BINFO_TYPE (binfo) == current_class_type)
	    /* The immediate base class of the class we are in
	       does let its public members through.  */
	    via_private = 0;
#ifndef NOJJG
	  else if (protect
		   && friends != NULL_TREE
		   && BINFO_TYPE (binfo) == xtype
		   && value_member (current_class_type, friends))
	    /* Friend types of the most derived type have access
	       to its baseclass pointers.  */
	    via_private = 0;
#endif

	  rval = get_binfo_recursive (base_binfo, via_private, parent, rval,
				      rval_private_ptr, xtype, friends,
				      protect);
	  if (rval == error_mark_node)
	    return rval;
	}
    }

  return rval;
}

/* Return non-zero if PARENT is directly derived from TYPE.  By directly
   we mean it's only one step up the inheritance lattice.  We check this
   by walking horizontally across the types that TYPE directly inherits
   from, to see if PARENT is among them.  This is used by get_binfo and
   by compute_visibility.  */
static int
immediately_derived (parent, type)
     tree parent, type;
{
  if (TYPE_BINFO (type))
    {
      tree binfos = BINFO_BASETYPES (TYPE_BINFO (type));
      int i, n_baselinks = binfos ? TREE_VEC_LENGTH (binfos) : 0;

      for (i = 0; i < n_baselinks; i++)
	{
	  tree base_binfo = TREE_VEC_ELT (binfos, i);

	  if (parent == BINFO_TYPE (base_binfo))
	    return 1;
	}
    }
  return 0;
}

/* Check whether the type given in BINFO is derived from PARENT.  If
   it isn't, return 0.  If it is, but the derivation is MI-ambiguous
   AND protect != 0, emit an error message and return error_mark_node.

   Otherwise, if TYPE is derived from PARENT, return the actual base
   information, unless a one of the protection violations below
   occurs, in which case emit an error message and return error_mark_node.

   The below should be worded better.  It may not be exactly what the code
   does, but there should be a lose correlation.  If you understand the code
   well, please try and make the comments below more readable.

   If PROTECT is 1, then check if access to a public field of PARENT
   would be private.

   If PROTECT is 2, then check if the given type is derived from
   PARENT via private visibility rules.

   If PROTECT is 3, then immediately private baseclass is ok,
   but deeper than that, check if private.  */
tree
get_binfo (parent, binfo, protect)
     register tree parent, binfo;
     int protect;
{
  tree xtype, type;
  tree otype;
  int head = 0, tail = 0;
  int is_private = 0;
  tree rval = NULL_TREE;
  int rval_private = 0;
  tree friends;

#ifdef GATHER_STATISTICS
  n_calls_get_base_type++;
#endif

  if (TREE_CODE (parent) == TREE_VEC)
    parent = BINFO_TYPE (parent);
  /* unions cannot participate in inheritance relationships */
  else if (TREE_CODE (parent) == UNION_TYPE)
    return NULL_TREE;
  else if (TREE_CODE (parent) != RECORD_TYPE)
    my_friendly_abort (89);

  parent = TYPE_MAIN_VARIANT (parent);

  if (TREE_CODE (binfo) == TREE_VEC)
    type = BINFO_TYPE (binfo);
  else if (TREE_CODE (binfo) == RECORD_TYPE)
    {
      type = binfo;
      binfo = TYPE_BINFO (type);
    }
  else my_friendly_abort (90);
  xtype = type;
  friends = current_class_type ? CLASSTYPE_FRIEND_CLASSES (type) : NULL_TREE;

  rval = get_binfo_recursive (binfo, is_private, parent, rval, &rval_private,
			      xtype, friends, protect);

  dfs_walk (binfo, dfs_unmark, markedp);

  if (rval && protect && rval_private)
    {
      if (protect == 3 && immediately_derived (parent, xtype))
	return rval;
      cp_error ("type `%T' is derived from private `%T'", xtype, parent);
      return error_mark_node;
    }

  return rval;
}

/* This is the newer depth first get_base_distance routine.  */
static
get_base_distance_recursive (binfo, depth, is_private, basetype_path, rval,
			     rval_private_ptr, new_binfo_ptr, parent, path_ptr,
			     protect, via_virtual_ptr, via_virtual)
     tree binfo, basetype_path, *new_binfo_ptr, parent, *path_ptr;
     int *rval_private_ptr, depth, is_private, rval, protect, *via_virtual_ptr,
       via_virtual;
{
  tree binfos;
  int i, n_baselinks;

  if (BINFO_TYPE (binfo) == parent || binfo == parent)
    {
      if (rval == -1)
	{
	  rval = depth;
	  *rval_private_ptr = is_private;
	  *new_binfo_ptr = binfo;
	  *via_virtual_ptr = via_virtual;
	}
      else
	{
	  int same_object = tree_int_cst_equal (BINFO_OFFSET (*new_binfo_ptr),
						BINFO_OFFSET (binfo));

	  if (*via_virtual_ptr && via_virtual==0)
	    {
	      *rval_private_ptr = is_private;
	      *new_binfo_ptr = binfo;
	      *via_virtual_ptr = via_virtual;
	    }
	  else if (same_object)
	    {
	      /* Note, this should probably succeed to find, and
		 override the old one if the old one was private and
		 this one isn't.  */
	      return rval;
	    }

	  rval = -2;
	}
      return rval;
    }

  binfos = BINFO_BASETYPES (binfo);
  n_baselinks = binfos ? TREE_VEC_LENGTH (binfos) : 0;
  depth += 1;

  /* Process base types.  */
  for (i = 0; i < n_baselinks; i++)
    {
      tree base_binfo = TREE_VEC_ELT (binfos, i);

      if (BINFO_MARKED (base_binfo) == 0)
	{
	  int via_private = is_private || !TREE_VIA_PUBLIC (base_binfo);
	  int was;

	  /* When searching for a non-virtual, we cannot mark
	     virtually found binfos. */
	  if (!via_virtual)
	    SET_BINFO_MARKED (base_binfo);

	  if (via_private == 0)
	    ;
	  else if (protect == 0)
	    via_private = 0;

#define WATCH_VALUES(rval, via_private) (rval == -1 ? 3 : via_private)

	  was = WATCH_VALUES (rval, *via_virtual_ptr);
	  rval = get_base_distance_recursive (base_binfo, depth, via_private,
					      binfo, rval, rval_private_ptr,
					      new_binfo_ptr, parent, path_ptr,
					      protect, via_virtual_ptr,
					      TREE_VIA_VIRTUAL (base_binfo)|via_virtual);
	  /* watch for updates, only update, if path is good. */
	  if (path_ptr && WATCH_VALUES (rval, *via_virtual_ptr) != was)
	    BINFO_INHERITANCE_CHAIN (base_binfo) = binfo;
	  if (rval == -2 && *via_virtual_ptr == 0)
	    return rval;

#undef WATCH_VALUES

	}
    }

  return rval;
}

/* Return the number of levels between type PARENT and the type given
   in BINFO, following the leftmost path to PARENT not found along a
   virtual path, if there are no real PARENTs (all come from virtual
   base classes), then follow the leftmost path to PARENT.

   Return -1 if TYPE is not derived from PARENT.
   Return -2 if PARENT is an ambiguous base class of TYPE.
   Return -3 if PARENT is private to TYPE, and protect is non-zero.

   If PATH_PTR is non-NULL, then also build the list of types
   from PARENT to TYPE, with TREE_VIA_VIRUAL and TREE_VIA_PUBLIC
   set.

   PARENT can also be a binfo, in which case that exact parent is found
   and no other.  convert_pointer_to_real uses this functionality.

   Code in prepare_fresh_vtable relies upon the path being built even
   when -2 is returned.  */

int
get_base_distance (parent, binfo, protect, path_ptr)
     register tree parent, binfo;
     int protect;
     tree *path_ptr;
{
  int head, tail;
  int is_private = 0;
  int rval;
  int depth = 0;
  int rval_private = 0;
  tree type, basetype_path = NULL_TREE;
  tree friends;
  tree new_binfo = NULL_TREE;
  int via_virtual;

  if (TREE_CODE (parent) != TREE_VEC)
    parent = TYPE_MAIN_VARIANT (parent);

  if (TREE_CODE (binfo) == TREE_VEC)
    type = BINFO_TYPE (binfo);
  else if (TREE_CODE (binfo) == RECORD_TYPE)
    {
      type = binfo;
      binfo = TYPE_BINFO (type);
    }
  else
    my_friendly_abort (92);

  friends = current_class_type ? CLASSTYPE_FRIEND_CLASSES (type) : NULL_TREE;

  if (path_ptr)
    {
      basetype_path = TYPE_BINFO (type);
      BINFO_INHERITANCE_CHAIN (basetype_path) = NULL_TREE;
    }

  if (parent == type || parent == basetype_path)
    {
      /* If the distance is 0, then we don't really need
	 a path pointer, but we shouldn't let garbage go back.  */
      if (path_ptr)
	*path_ptr = basetype_path;
      return 0;
    }

  rval = get_base_distance_recursive (binfo, 0, 0, NULL_TREE, -1,
				      &rval_private, &new_binfo, parent,
				      path_ptr, protect, &via_virtual, 0);

  if (path_ptr)
    BINFO_INHERITANCE_CHAIN (binfo) = NULL_TREE;

  basetype_path = binfo;

  dfs_walk (binfo, dfs_unmark, markedp);

  binfo = new_binfo;

  /* Visibilities don't count if we found an ambiguous basetype.  */
  if (rval == -2)
    rval_private = 0;

  if (rval && protect && rval_private)
    return -3;

  if (path_ptr)
    *path_ptr = binfo;
  return rval;
}

/* Search for a member with name NAME in a multiple inheritance lattice
   specified by TYPE.  If it does not exist, return NULL_TREE.
   If the member is ambiguously referenced, return `error_mark_node'.
   Otherwise, return the FIELD_DECL.  */

/* Do a 1-level search for NAME as a member of TYPE.  The caller
   must figure out whether it has a visible path to this field.
   (Since it is only one level, this is reasonable.)  */
static tree
lookup_field_1 (type, name)
     tree type, name;
{
  register tree field = TYPE_FIELDS (type);

#ifdef GATHER_STATISTICS
  n_calls_lookup_field_1++;
#endif
  while (field)
    {
#ifdef GATHER_STATISTICS
      n_fields_searched++;
#endif
      if (DECL_NAME (field) == NULL_TREE
	  && TREE_CODE (TREE_TYPE (field)) == UNION_TYPE)
	{
	  tree temp = lookup_field_1 (TREE_TYPE (field), name);
	  if (temp)
	    return temp;
	}
      if (DECL_NAME (field) == name)
	{
	  if ((TREE_CODE(field) == VAR_DECL || TREE_CODE(field) == CONST_DECL)
	      && DECL_ASSEMBLER_NAME (field) != NULL)
	    GNU_xref_ref(current_function_decl,
			 IDENTIFIER_POINTER (DECL_ASSEMBLER_NAME (field)));
	  return field;
	}
      field = TREE_CHAIN (field);
    }
  /* Not found.  */
  if (name == _vptr_name)
    {
      /* Give the user what s/he thinks s/he wants.  */
      if (TYPE_VIRTUAL_P (type))
	return CLASSTYPE_VFIELD (type);
    }
  return NULL_TREE;
}

/* Compute the visibility of FIELD.  This is done by computing
   the visibility available to each type in BASETYPES (which comes
   as a list of [via_public/basetype] in reverse order, namely base
   class before derived class).  The first one which defines a
   visibility defines the visibility for the field.  Otherwise, the
   visibility of the field is that which occurs normally.

   Uses global variables CURRENT_CLASS_TYPE and
   CURRENT_FUNCTION_DECL to use friend relationships
   if necessary.

   This will be static when lookup_fnfield comes into this file.  */

#define PUBLIC_RETURN return (DECL_PUBLIC (field) = 1), visibility_public
#define PROTECTED_RETURN return (DECL_PROTECTED (field) = 1), visibility_protected
#define PRIVATE_RETURN return (DECL_PRIVATE (field) = 1), visibility_private

enum visibility_type
compute_visibility (basetype_path, field)
     tree basetype_path, field;
{
  enum visibility_type visibility = visibility_public;
  tree types;
  tree context = DECL_CLASS_CONTEXT (field);
  /* Used once we go past an access gate that makes private really
     deny us access from the context.  */
  int really_private;

  if (context == NULL_TREE)
    context = DECL_CONTEXT (field);

  /* Fields coming from nested anonymous unions have their DECL_CLASS_CONTEXT
     slot set to the union type rather than the record type containing
     the anonymous union.  In this case, DECL_FIELD_CONTEXT is correct.  */
  if (context && TREE_CODE (context) == UNION_TYPE
      && ANON_AGGRNAME_P (TYPE_IDENTIFIER (context)))
    context = DECL_FIELD_CONTEXT (field);

  /* Virtual function tables are never private.  But we should know that
     we are looking for this, and not even try to hide it.  */
  if (DECL_NAME (field) && VFIELD_NAME_P (DECL_NAME (field)) == 1)
    return visibility_public;

  /* Member function manipulating its own members.  */
  if (current_class_type == context
      || (context && current_class_type == TYPE_MAIN_VARIANT (context)))
    PUBLIC_RETURN;

  /* Make these special cases fast.  */
  if (BINFO_TYPE (basetype_path) == current_class_type)
    {
      if (DECL_PUBLIC (field))
	return visibility_public;
      if (DECL_PROTECTED (field))
	return visibility_protected;
      if (DECL_PRIVATE (field))
	return visibility_private;
    }

  /* Member found immediately within object.  */
  if (BINFO_INHERITANCE_CHAIN (basetype_path) == NULL_TREE)
    {
      /* At the object's top level, public members are public.  */
      if (!TREE_PROTECTED (field) && !TREE_PRIVATE (field))
	PUBLIC_RETURN;

      /* Is it a friend function manipulating members that it gets by
	 virtue of its friendship?  Or are we friends with the class
	 that has FIELD?  */
      if ((current_function_decl && is_friend (context, current_function_decl))
	  || (is_friend_type (context, current_class_type)))
	PUBLIC_RETURN;

      if (current_class_type && DECL_VISIBILITY (field) == NULL_TREE)
	{
	  /* If it's private, it's private, you letch.  */
	  if (TREE_PRIVATE (field))
	    PRIVATE_RETURN;

	  /* ARM $11.5.  Member functions of a derived class can access the
	     non-static protected members of a base class only through a
	     pointer to the derived class, a reference to it, or an object
	     of it. Also any subsequently derived classes also have
	     access.  */
	  if (TREE_PROTECTED (field))
	    {
	      if (context == current_class_type
		  || UNIQUELY_DERIVED_FROM_P (context, current_class_type))
		PUBLIC_RETURN;
	      else
		PROTECTED_RETURN;
	    }
	  else
	    /* Will we ever actually reach here?  Doubt it.  */
	    my_friendly_abort (94);
	}
    }

  /* Is it a friend function manipulating members that it gets by
     virtue of its friendship?  */
  if (is_friend (context, current_function_decl))
    PUBLIC_RETURN;

  /* must reverse more than one element */
  basetype_path = reverse_path (basetype_path);
  types = basetype_path;
  really_private = 0;

  while (types)
    {
      tree member;
      tree binfo = types;
      tree type = BINFO_TYPE (binfo);

      member = purpose_member (type, DECL_VISIBILITY (field));
      if (member)
	{
	  visibility = (enum visibility_type)TREE_VALUE (member);
	  if (visibility == visibility_public
	      || is_friend (type, current_function_decl)
	      || (visibility == visibility_protected
		  && current_class_type
		  && UNIQUELY_DERIVED_FROM_P (context, current_class_type)))
	    visibility = visibility_public;
	  goto ret;
	}

      /* Friends inherit the visibility of the class they inherit from.  */
      if (is_friend (type, current_function_decl))
	{
	  if (type == context)
	    {
	      visibility = visibility_public;
	      goto ret;
	    }
	  if (TREE_PROTECTED (field))
	    {
	      visibility = visibility_public;
	      goto ret;
	    }
	  /* else, may be a friend of a deeper base class */
	}

      if (type == context)
	break;

      types = BINFO_INHERITANCE_CHAIN (types);

      /* If the next type was not VIA_PUBLIC, then fields of all
	 remaining class past that one are private.  */
      if (types)
	{
	  if (TREE_VIA_PROTECTED (types))
	    {
	      if (really_private == 0)
		really_private = 1;
	      else
		visibility = visibility_protected;
	    }
	  else if (! TREE_VIA_PUBLIC (types))
	    {
	      if (really_private == 0)
		really_private = 2;
	      else
		visibility = visibility_private;
	    }
	}
    }

  /* No special visibilities apply.  Use normal rules.
     No assignment needed for BASETYPEs here from the nreverse.
     This is because we use it only for information about the
     path to the base.  The code earlier dealt with what
     happens when we are at the base level.  */

  if (visibility == visibility_public)
    {
      reverse_path (basetype_path);
      if (TREE_PRIVATE (field))
	PRIVATE_RETURN;
      if (TREE_PROTECTED (field))
	{
	  /* Used to check if the current class type was derived from
	     the type that contains the field.  This is wrong for
	     multiple inheritance because is gives one class reference
	     to protected members via another classes protected path.
	     I.e., if A; B1 : A; B2 : A;  Then B1 and B2 can access
	     their own members which are protected in A, but not
	     those same members in one another.  */
	  if (current_class_type
	      && UNIQUELY_DERIVED_FROM_P (context, current_class_type))
	    PUBLIC_RETURN;
	  PROTECTED_RETURN;
	}
      PUBLIC_RETURN;
    }

  if (visibility == visibility_protected)
    {
      reverse_path (basetype_path);

      if (TREE_PRIVATE (field))
	PRIVATE_RETURN;
      /* We want to make sure that all non-private members in
	 the current class (as derived) are accessible.  */
      if (current_class_type
	  && UNIQUELY_DERIVED_FROM_P (context, current_class_type))
	PUBLIC_RETURN;
      PROTECTED_RETURN;
    }

  if (visibility == visibility_private && current_class_type != NULL_TREE)
    {
      reverse_path (basetype_path);

      /* If it's private in the base class, it stays private.  */
      if (TREE_PRIVATE (field))
	PRIVATE_RETURN;

      /* ARM $11.2: Specifying a base class private does not affect access
	 to static members of the base class.  */
      if ((TREE_CODE (field) != FUNCTION_DECL
	   && TREE_STATIC (field))
	  || (TREE_CODE (field) == FUNCTION_DECL
	      && DECL_STATIC_FUNCTION_P (field)))
	PUBLIC_RETURN;

      /* If it's public or protected in the base class, it becomes
	 private in the derived class.  If we (the current_class_type)
	 are not that immediately derived type that gets it as a
	 private member, then the member is not visibile.  */
      if (immediately_derived (context, current_class_type))
	PUBLIC_RETURN;

      PRIVATE_RETURN;
    }

 ret:
  reverse_path (basetype_path);

  if (visibility == visibility_public)
    DECL_PUBLIC (field) = 1;
  else if (visibility == visibility_protected)
    DECL_PROTECTED (field) = 1;
  else if (visibility == visibility_private)
    DECL_PRIVATE (field) = 1;
  else my_friendly_abort (96);
  return visibility;
}

/* Routine to see if the sub-object denoted by the binfo PARENT can be
   found as a base class and sub-object of the object denoted by
   BINFO.  This routine relies upon binfos not being shared, except
   for binfos for virtual bases.  */
static int
is_subobject_of_p (parent, binfo)
     tree parent, binfo;
{
  tree binfos = BINFO_BASETYPES (binfo);
  int i, n_baselinks = binfos ? TREE_VEC_LENGTH (binfos) : 0;

  if (parent == binfo)
    return 1;

  /* Process and/or queue base types.  */
  for (i = 0; i < n_baselinks; i++)
    {
      tree base_binfo = TREE_VEC_ELT (binfos, i);
      if (TREE_VIA_VIRTUAL (base_binfo))
	base_binfo = TYPE_BINFO (BINFO_TYPE (base_binfo));
      if (is_subobject_of_p (parent, base_binfo))
	return 1;
    }
  return 0;
}

/* See if a one FIELD_DECL hides another.  This routine is meant to
   correspond to ANSI working paper Sept 17, 1992 10p4.  The two
   binfos given are the binfos corresponding to the particular places
   the FIELD_DECLs are found.  This routine relies upon binfos not
   being shared, except for virtual bases. */
static int
hides (hider_binfo, hidee_binfo)
     tree hider_binfo, hidee_binfo;
{
  /* hider hides hidee, if hider has hidee as a base class and
     the instance of hidee is a sub-object of hider.  The first
     part is always true is the second part is true.

     When hider and hidee are the same (two ways to get to the exact
     same member) we consider either one as hiding the other. */
  return is_subobject_of_p (hidee_binfo, hider_binfo);
}

/* Very similar to lookup_fnfields_1 but it ensures that at least one
   function was declared inside the class given by TYPE.  It really should
   only return functions that match the given TYPE.  */
static int
lookup_fnfields_here (type, name)
     tree type, name;
{
  int index = lookup_fnfields_1 (type, name);
  tree fndecls;

  if (index <= 0)
    return index;
  fndecls = TREE_VEC_ELT (CLASSTYPE_METHOD_VEC (type), index);
  while (fndecls)
    {
      if (TYPE_MAIN_VARIANT (DECL_CLASS_CONTEXT (fndecls))
	  == TYPE_MAIN_VARIANT (type))
	return index;
      fndecls = TREE_CHAIN (fndecls);
    }
  return -1;
}

/* Look for a field named NAME in an inheritance lattice dominated by
   XBASETYPE.  PROTECT is zero if we can avoid computing visibility
   information, otherwise it is 1.  WANT_TYPE is 1 when we should only
   return TYPE_DECLs, if no TYPE_DECL can be found return NULL_TREE.

   It was not clear what should happen if WANT_TYPE is set, and an
   ambiguity is found.  At least one use (lookup_name) to not see
   the error.  */
tree
lookup_field (xbasetype, name, protect, want_type)
     register tree xbasetype, name;
     int protect, want_type;
{
  int head = 0, tail = 0;
  tree rval, rval_binfo = NULL_TREE, rval_binfo_h;
  tree type, basetype_chain, basetype_path;
  enum visibility_type this_v = visibility_default;
  tree entry, binfo, binfo_h;
  enum visibility_type own_visibility = visibility_default;
  int vbase_name_p = VBASE_NAME_P (name);

  /* rval_binfo is the binfo associated with the found member, note,
     this can be set with useful information, even when rval is not
     set, because it must deal with ALL members, not just non-function
     members.  It is used for ambiguity checking and the hidden
     checks.  Whereas rval is only set if a proper (not hidden)
     non-function member is found.  */

  /* rval_binfo_h and binfo_h are binfo values used when we perform the
     hiding checks, as virtual base classes may not be shared.  The strategy
     is we always go into the the binfo hierarchy owned by TYPE_BINFO of
     virtual base classes, as we cross virtual base class lines.  This way
     we know that binfo of a virtual base class will always == itself when
     found along any line.  (mrs)  */

  /* Things for memoization.  */
  char *errstr = 0;

  /* Set this to nonzero if we don't know how to compute
     accurate error messages for visibility.  */
  int index = MEMOIZED_HASH_FN (name);

  /* If we are looking for a constructor in a templated type, use the
     unspecialized name, as that is how we store it.  */
  if (IDENTIFIER_TEMPLATE (name))
    name = constructor_name (name);

  if (TREE_CODE (xbasetype) == TREE_VEC)
    basetype_path = xbasetype, type = BINFO_TYPE (xbasetype);
  else if (IS_AGGR_TYPE_CODE (TREE_CODE (xbasetype)))
    basetype_path = TYPE_BINFO (xbasetype), type = xbasetype;
  else my_friendly_abort (97);

  if (CLASSTYPE_MTABLE_ENTRY (type))
    {
      tree tem = MEMOIZED_FIELDS (CLASSTYPE_MTABLE_ENTRY (type), index);

      while (tem && TREE_PURPOSE (tem) != name)
	{
	  memoized_fields_searched[0]++;
	  tem = TREE_CHAIN (tem);
	}
      if (tem)
	{
	  if (protect && TREE_TYPE (tem))
	    {
	      error (TREE_STRING_POINTER (TREE_TYPE (tem)),
		     IDENTIFIER_POINTER (name),
		     TYPE_NAME_STRING (DECL_FIELD_CONTEXT (TREE_VALUE (tem))));
	      return error_mark_node;
	    }
	  if (TREE_VALUE (tem) == NULL_TREE)
	    memoized_fast_rejects[0] += 1;
	  else
	    memoized_fast_finds[0] += 1;
	  return TREE_VALUE (tem);
	}
    }

#ifdef GATHER_STATISTICS
  n_calls_lookup_field++;
#endif
  if (protect && flag_memoize_lookups && ! global_bindings_p ())
    entry = make_memoized_table_entry (type, name, 0);
  else
    entry = 0;

  rval = lookup_field_1 (type, name);
  if (rval || lookup_fnfields_here (type, name)>=0)
    {
      rval_binfo = basetype_path;
      rval_binfo_h = rval_binfo;
    }

  if (rval && TREE_CODE (rval) != TYPE_DECL && want_type)
    rval = NULL_TREE;

  if (rval)
    {
      if (protect)
	{
	  if (TREE_PRIVATE (rval) | TREE_PROTECTED (rval))
	    this_v = compute_visibility (basetype_path, rval);
	  if (TREE_CODE (rval) == CONST_DECL)
	    {
	      if (this_v == visibility_private)
		errstr = "enum `%s' is a private value of class `%s'";
	      else if (this_v == visibility_protected)
		errstr = "enum `%s' is a protected value of class `%s'";
	    }
	  else
	    {
	      if (this_v == visibility_private)
		errstr = "member `%s' is a private member of class `%s'";
	      else if (this_v == visibility_protected)
		errstr = "member `%s' is a protected member of class `%s'";
	    }
	}

      if (entry)
	{
	  if (errstr)
	    {
	      /* This depends on behavior of lookup_field_1!  */
	      tree error_string = my_build_string (errstr);
	      TREE_TYPE (entry) = error_string;
	    }
	  else
	    {
	      /* Let entry know there is no problem with this access.  */
	      TREE_TYPE (entry) = NULL_TREE;
	    }
	  TREE_VALUE (entry) = rval;
	}

      if (errstr && protect)
	{
	  error (errstr, IDENTIFIER_POINTER (name), TYPE_NAME_STRING (type));
	  return error_mark_node;
	}
      return rval;
    }

  basetype_chain = CLASSTYPE_BINFO_AS_LIST (type);
  TREE_VIA_PUBLIC (basetype_chain) = 1;

  /* The ambiguity check relies upon breadth first searching. */

  search_stack = push_search_level (search_stack, &search_obstack);
  BINFO_VIA_PUBLIC (basetype_path) = 1;
  BINFO_INHERITANCE_CHAIN (basetype_path) = NULL_TREE;
  binfo = basetype_path;
  binfo_h = binfo;

  while (1)
    {
      tree binfos = BINFO_BASETYPES (binfo);
      int i, n_baselinks = binfos ? TREE_VEC_LENGTH (binfos) : 0;
      tree nval;

      /* Process and/or queue base types.  */
      for (i = 0; i < n_baselinks; i++)
	{
	  tree base_binfo = TREE_VEC_ELT (binfos, i);
	  if (BINFO_FIELDS_MARKED (base_binfo) == 0)
	    {
	      tree btypes;

	      SET_BINFO_FIELDS_MARKED (base_binfo);
	      btypes = my_tree_cons (NULL_TREE, base_binfo, basetype_chain);
	      TREE_VIA_PUBLIC (btypes) = TREE_VIA_PUBLIC (base_binfo);
	      TREE_VIA_PROTECTED (btypes) = TREE_VIA_PROTECTED (base_binfo);
	      TREE_VIA_VIRTUAL (btypes) = TREE_VIA_VIRTUAL (base_binfo);
	      if (TREE_VIA_VIRTUAL (base_binfo))
		btypes = tree_cons (NULL_TREE,
				    TYPE_BINFO (BINFO_TYPE (TREE_VEC_ELT (BINFO_BASETYPES (binfo_h), i))),
				    btypes);
	      else
		btypes = tree_cons (NULL_TREE,
				    TREE_VEC_ELT (BINFO_BASETYPES (binfo_h), i),
				    btypes);
	      obstack_ptr_grow (&search_obstack, btypes);
	      tail += 1;
	      if (tail >= search_stack->limit)
		my_friendly_abort (98);
	    }
	}

      /* Process head of queue, if one exists.  */
      if (head >= tail)
	break;

      basetype_chain = search_stack->first[head++];
      binfo_h = TREE_VALUE (basetype_chain);
      basetype_chain = TREE_CHAIN (basetype_chain);
      basetype_path = TREE_VALUE (basetype_chain);
      if (TREE_CHAIN (basetype_chain))
	BINFO_INHERITANCE_CHAIN (basetype_path) = TREE_VALUE (TREE_CHAIN (basetype_chain));
      else
	BINFO_INHERITANCE_CHAIN (basetype_path) = NULL_TREE;

      binfo = basetype_path;
      type = BINFO_TYPE (binfo);

      /* See if we can find NAME in TYPE.  If RVAL is nonzero,
	 and we do find NAME in TYPE, verify that such a second
	 sighting is in fact legal.  */

      nval = lookup_field_1 (type, name);

      if (nval || lookup_fnfields_here (type, name)>=0)
	{
	  if (rval_binfo && hides (rval_binfo_h, binfo_h))
	    {
	      /* This is ok, the member found is in rval_binfo, not
		 here (binfo). */
	    }
	  else if (rval_binfo==NULL_TREE || hides (binfo_h, rval_binfo_h))
	    {
	      /* This is ok, the member found is here (binfo), not in
		 rval_binfo. */
	      if (nval)
		{
		  rval = nval;
		  if (entry || protect)
		    this_v = compute_visibility (basetype_path, rval);
		  /* These may look ambiguous, but they really are not.  */
		  if (vbase_name_p)
		    break;
		}
	      else
		{
		  /* Undo finding it before, as something else hides it. */
		  rval = NULL_TREE;
		}
	      rval_binfo = binfo;
	      rval_binfo_h = binfo_h;
	    }
	  else
	    {
	      /* This is ambiguous. */
	      errstr = "request for member `%s' is ambiguous";
	      protect = 2;
	      break;
	    }
	}
    }
  {
    tree *tp = search_stack->first;
    tree *search_tail = tp + tail;

    if (entry)
      TREE_VALUE (entry) = rval;

    if (want_type && (rval == NULL_TREE || TREE_CODE (rval) != TYPE_DECL))
      {
	rval = NULL_TREE;
	errstr = 0;
      }

    /* If this FIELD_DECL defines its own visibility, deal with that.  */
    if (rval && errstr == 0
	&& ((protect&1) || entry)
	&& DECL_LANG_SPECIFIC (rval)
	&& DECL_VISIBILITY (rval))
      {
	while (tp < search_tail)
	  {
	    /* If is possible for one of the derived types on the
	       path to have defined special visibility for this
	       field.  Look for such declarations and report an
	       error if a conflict is found.  */
	    enum visibility_type new_v;

	    if (this_v != visibility_default)
	      new_v = compute_visibility (TREE_VALUE (TREE_CHAIN (*tp)), rval);
	    if (this_v != visibility_default && new_v != this_v)
	      {
		errstr = "conflicting visibilities to member `%s'";
		this_v = visibility_default;
	      }
	    own_visibility = new_v;
	    CLEAR_BINFO_FIELDS_MARKED (TREE_VALUE (TREE_CHAIN (*tp)));
	    tp += 1;
	  }
      }
    else
      {
	while (tp < search_tail)
	  {
	    CLEAR_BINFO_FIELDS_MARKED (TREE_VALUE (TREE_CHAIN (*tp)));
	    tp += 1;
	  }
      }
  }
  search_stack = pop_search_level (search_stack);

  if (errstr == 0)
    {
      if (own_visibility == visibility_private)
	errstr = "member `%s' declared private";
      else if (own_visibility == visibility_protected)
	errstr = "member `%s' declared protected";
      else if (this_v == visibility_private)
	errstr = TREE_PRIVATE (rval)
	  ? "member `%s' is private"
	    : "member `%s' is from private base class";
      else if (this_v == visibility_protected)
	errstr = TREE_PROTECTED (rval)
	  ? "member `%s' is protected"
	    : "member `%s' is from protected base class";
    }

  if (entry)
    {
      if (errstr)
	{
	  tree error_string = my_build_string (errstr);
	  /* Save error message with entry.  */
	  TREE_TYPE (entry) = error_string;
	}
      else
	{
	  /* Mark entry as having no error string.  */
	  TREE_TYPE (entry) = NULL_TREE;
	}
    }

  if (errstr && protect)
    {
      char *p = IDENTIFIER_POINTER (name), *q = NULL;
      if (IDENTIFIER_OPNAME_P (name))
	{
	  q = operator_name_string (name);
	  p = (char *) xmalloc (9 + strlen (q) + 1);
	  sprintf (p, "operator %s", q);
	}

      error (errstr, p, TYPE_NAME_STRING (type));
      if (q)
	free (p);
      rval = error_mark_node;
    }
  return rval;
}

/* Try to find NAME inside a nested class.  */
tree
lookup_nested_field (name, complain)
     tree name;
     int complain;
{
  register tree t;

  tree id = NULL_TREE;
  if (TREE_CHAIN (current_class_type))
    {
      /* Climb our way up the nested ladder, seeing if we're trying to
	 modify a field in an enclosing class.  If so, we should only
	 be able to modify if it's static.  */
      for (t = TREE_CHAIN (current_class_type);
	   t && DECL_CONTEXT (t);
	   t = TREE_CHAIN (DECL_CONTEXT (t)))
	{
	  if (TREE_CODE (DECL_CONTEXT (t)) != RECORD_TYPE)
	    break;

	  /* N.B.: lookup_field will do the visibility checking for us */
	  id = lookup_field (DECL_CONTEXT (t), name, complain, 0);
	  if (id == error_mark_node)
	    {
	      id = NULL_TREE;
	      continue;
	    }

	  if (id != NULL_TREE)
	    {
	      if (TREE_CODE (id) == FIELD_DECL
		  && ! TREE_STATIC (id)
		  && TREE_TYPE (id) != error_mark_node)
		{
		  if (complain)
		    {
		      /* At parse time, we don't want to give this error, since
			 we won't have enough state to make this kind of
			 decision properly.  But there are times (e.g., with
			 enums in nested classes) when we do need to call
			 this fn at parse time.  So, in those cases, we pass
			 complain as a 0 and just return a NULL_TREE.  */
		      error ("assignment to non-static member `%s' of enclosing class `%s'",
			     lang_printable_name (id),
			     IDENTIFIER_POINTER (TYPE_IDENTIFIER
						 (DECL_CONTEXT (t))));
		      /* Mark this for do_identifier().  It would otherwise
			 claim that the variable was undeclared.  */
		      TREE_TYPE (id) = error_mark_node;
		    }
		  else
		    {
		      id = NULL_TREE;
		      continue;
		    }
		}
	      break;
	    }
	}
    }

  return id;
}

/* TYPE is a class type. Return the index of the fields within
   the method vector with name NAME, or -1 is no such field exists.  */
static int
lookup_fnfields_1 (type, name)
     tree type, name;
{
  register tree method_vec = CLASSTYPE_METHOD_VEC (type);

  if (method_vec != 0)
    {
      register tree *methods = &TREE_VEC_ELT (method_vec, 0);
      register tree *end = TREE_VEC_END (method_vec);

#ifdef GATHER_STATISTICS
      n_calls_lookup_fnfields_1++;
#endif
      if (*methods && name == constructor_name (type))
	return 0;

      while (++methods != end)
	{
#ifdef GATHER_STATISTICS
	  n_outer_fields_searched++;
#endif
	  if (DECL_NAME (*methods) == name)
	    break;
	}
      if (methods != end)
	return methods - &TREE_VEC_ELT (method_vec, 0);
    }

  return -1;
}

/* Starting from BASETYPE, return a TREE_BASELINK-like object
   which gives the following information (in a list):

   TREE_TYPE: list of basetypes needed to get to...
   TREE_VALUE: list of all functions in of given type
   which have name NAME.

   No visibility information is computed by this function,
   other then to adorn the list of basetypes with
   TREE_VIA_PUBLIC.

   If there are two ways to find a name (two members), if COMPLAIN is
   non-zero, then error_mark_node is returned, and an error message is
   printed, otherwise, just an error_mark_node is returned.

   As a special case, is COMPLAIN is -1, we don't complain, and we
   don't return error_mark_node, but rather the complete list of
   virtuals.  This is used by get_virtuals_named_this.  */
tree
lookup_fnfields (basetype_path, name, complain)
     tree basetype_path, name;
     int complain;
{
  int head = 0, tail = 0;
  tree type, rval, rval_binfo = NULL_TREE, rvals = NULL_TREE, rval_binfo_h;
  tree entry, binfo, basetype_chain, binfo_h;
  int find_all = 0;

  /* rval_binfo is the binfo associated with the found member, note,
     this can be set with useful information, even when rval is not
     set, because it must deal with ALL members, not just function
     members.  It is used for ambiguity checking and the hidden
     checks.  Whereas rval is only set if a proper (not hidden)
     function member is found.  */

  /* rval_binfo_h and binfo_h are binfo values used when we perform the
     hiding checks, as virtual base classes may not be shared.  The strategy
     is we always go into the the binfo hierarchy owned by TYPE_BINFO of
     virtual base classes, as we cross virtual base class lines.  This way
     we know that binfo of a virtual base class will always == itself when
     found along any line.  (mrs)  */

  /* For now, don't try this.  */
  int protect = complain;

  /* Things for memoization.  */
  char *errstr = 0;

  /* Set this to nonzero if we don't know how to compute
     accurate error messages for visibility.  */
  int index = MEMOIZED_HASH_FN (name);

  if (complain == -1)
    {
      find_all = 1;
      protect = complain = 0;
    }

  /* If we are looking for a constructor in a templated type, use the
     unspecialized name, as that is how we store it.  */
  if (IDENTIFIER_TEMPLATE (name))
    name = constructor_name (name);

  binfo = basetype_path;
  binfo_h = binfo;
  type = BINFO_TYPE (basetype_path);

  /* The memoization code is in need of maintenance. */
  if (!find_all && CLASSTYPE_MTABLE_ENTRY (type))
    {
      tree tem = MEMOIZED_FNFIELDS (CLASSTYPE_MTABLE_ENTRY (type), index);

      while (tem && TREE_PURPOSE (tem) != name)
	{
	  memoized_fields_searched[1]++;
	  tem = TREE_CHAIN (tem);
	}
      if (tem)
	{
	  if (protect && TREE_TYPE (tem))
	    {
	      error (TREE_STRING_POINTER (TREE_TYPE (tem)),
		     IDENTIFIER_POINTER (name),
		     TYPE_NAME_STRING (DECL_CLASS_CONTEXT (TREE_VALUE (TREE_VALUE (tem)))));
	      return error_mark_node;
	    }
	  if (TREE_VALUE (tem) == NULL_TREE)
	    {
	      memoized_fast_rejects[1] += 1;
	      return NULL_TREE;
	    }
	  else
	    {
	      /* Want to return this, but we must make sure
		 that visibility information is consistent.  */
	      tree baselink = TREE_VALUE (tem);
	      tree memoized_basetypes = TREE_PURPOSE (baselink);
	      tree these_basetypes = basetype_path;
	      while (memoized_basetypes && these_basetypes)
		{
		  memoized_fields_searched[1]++;
		  if (TREE_VALUE (memoized_basetypes) != these_basetypes)
		    break;
		  memoized_basetypes = TREE_CHAIN (memoized_basetypes);
		  these_basetypes = BINFO_INHERITANCE_CHAIN (these_basetypes);
		}
	      /* The following statement is true only when both are NULL.  */
	      if (memoized_basetypes == these_basetypes)
		{
		  memoized_fast_finds[1] += 1;
		  return TREE_VALUE (tem);
		}
	      /* else, we must re-find this field by hand.  */
	      baselink = tree_cons (basetype_path, TREE_VALUE (baselink), TREE_CHAIN (baselink));
	      return baselink;
	    }
	}
    }

#ifdef GATHER_STATISTICS
  n_calls_lookup_fnfields++;
#endif
  if (protect && flag_memoize_lookups && ! global_bindings_p ())
    entry = make_memoized_table_entry (type, name, 1);
  else
    entry = 0;

  index = lookup_fnfields_here (type, name);
  if (index >= 0 || lookup_field_1 (type, name))
    {
      rval_binfo = basetype_path;
      rval_binfo_h = rval_binfo;
    }

  if (index >= 0)
    {
      rval = TREE_VEC_ELT (CLASSTYPE_METHOD_VEC (type), index);
      rvals = my_tree_cons (basetype_path, rval, rvals);
      if (BINFO_BASETYPES (binfo) && CLASSTYPE_BASELINK_VEC (type))
	TREE_TYPE (rvals) = TREE_VEC_ELT (CLASSTYPE_BASELINK_VEC (type), index);

      if (entry)
	{
	  TREE_VALUE (entry) = rvals;
	  TREE_TYPE (entry) = NULL_TREE;
	}

      if (errstr && protect)
	{
	  error (errstr, IDENTIFIER_POINTER (name), TYPE_NAME_STRING (type));
	  return error_mark_node;
	}
      return rvals;
    }
  rval = NULL_TREE;

  basetype_chain = CLASSTYPE_BINFO_AS_LIST (type);
  TREE_VIA_PUBLIC (basetype_chain) = 1;

  /* The ambiguity check relies upon breadth first searching. */

  search_stack = push_search_level (search_stack, &search_obstack);
  BINFO_VIA_PUBLIC (basetype_path) = 1;
  BINFO_INHERITANCE_CHAIN (basetype_path) = NULL_TREE;
  binfo = basetype_path;
  binfo_h = binfo;

  while (1)
    {
      tree binfos = BINFO_BASETYPES (binfo);
      int i, n_baselinks = binfos ? TREE_VEC_LENGTH (binfos) : 0;
      int index;

      /* Process and/or queue base types.  */
      for (i = 0; i < n_baselinks; i++)
	{
	  tree base_binfo = TREE_VEC_ELT (binfos, i);
	  if (BINFO_FIELDS_MARKED (base_binfo) == 0)
	    {
	      tree btypes;

	      SET_BINFO_FIELDS_MARKED (base_binfo);
	      btypes = my_tree_cons (NULL_TREE, base_binfo, basetype_chain);
	      TREE_VIA_PUBLIC (btypes) = TREE_VIA_PUBLIC (base_binfo);
	      TREE_VIA_PROTECTED (btypes) = TREE_VIA_PROTECTED (base_binfo);
	      TREE_VIA_VIRTUAL (btypes) = TREE_VIA_VIRTUAL (base_binfo);
	      if (TREE_VIA_VIRTUAL (base_binfo))
		btypes = tree_cons (NULL_TREE,
				    TYPE_BINFO (BINFO_TYPE (TREE_VEC_ELT (BINFO_BASETYPES (binfo_h), i))),
				    btypes);
	      else
		btypes = tree_cons (NULL_TREE,
				    TREE_VEC_ELT (BINFO_BASETYPES (binfo_h), i),
				    btypes);
	      obstack_ptr_grow (&search_obstack, btypes);
	      tail += 1;
	      if (tail >= search_stack->limit)
		my_friendly_abort (99);
	    }
	}

      /* Process head of queue, if one exists.  */
      if (head >= tail)
	break;

      basetype_chain = search_stack->first[head++];
      binfo_h = TREE_VALUE (basetype_chain);
      basetype_chain = TREE_CHAIN (basetype_chain);
      basetype_path = TREE_VALUE (basetype_chain);
      if (TREE_CHAIN (basetype_chain))
	BINFO_INHERITANCE_CHAIN (basetype_path) = TREE_VALUE (TREE_CHAIN (basetype_chain));
      else
	BINFO_INHERITANCE_CHAIN (basetype_path) = NULL_TREE;

      binfo = basetype_path;
      type = BINFO_TYPE (binfo);

      /* See if we can find NAME in TYPE.  If RVAL is nonzero,
	 and we do find NAME in TYPE, verify that such a second
	 sighting is in fact legal.  */

      index = lookup_fnfields_here (type, name);

      if (index >= 0 || (lookup_field_1 (type, name)!=NULL_TREE && !find_all))
	{
	  if (rval_binfo && !find_all && hides (rval_binfo_h, binfo_h))
	    {
	      /* This is ok, the member found is in rval_binfo, not
		 here (binfo). */
	    }
	  else if (rval_binfo==NULL_TREE || find_all || hides (binfo_h, rval_binfo_h))
	    {
	      /* This is ok, the member found is here (binfo), not in
		 rval_binfo. */
	      if (index >= 0)
		{
		  rval = TREE_VEC_ELT (CLASSTYPE_METHOD_VEC (type), index);
		  /* Note, rvals can only be previously set if find_all is
		     true.  */
		  rvals = my_tree_cons (basetype_path, rval, rvals);
		  if (TYPE_BINFO_BASETYPES (type)
		      && CLASSTYPE_BASELINK_VEC (type))
		    TREE_TYPE (rvals) = TREE_VEC_ELT (CLASSTYPE_BASELINK_VEC (type), index);
		}
	      else
		{
		  /* Undo finding it before, as something else hides it. */
		  rval = NULL_TREE;
		  rvals = NULL_TREE;
		}
	      rval_binfo = binfo;
	      rval_binfo_h = binfo_h;
	    }
	  else
	    {
	      /* This is ambiguous. */
	      errstr = "request for member `%s' is ambiguous";
	      rvals = error_mark_node;
	      break;
	    }
	}
    }
  {
    tree *tp = search_stack->first;
    tree *search_tail = tp + tail;

    while (tp < search_tail)
      {
	CLEAR_BINFO_FIELDS_MARKED (TREE_VALUE (TREE_CHAIN (*tp)));
	tp += 1;
      }
  }
  search_stack = pop_search_level (search_stack);

  if (entry)
    {
      if (errstr)
	{
	  tree error_string = my_build_string (errstr);
	  /* Save error message with entry.  */
	  TREE_TYPE (entry) = error_string;
	}
      else
	{
	  /* Mark entry as having no error string.  */
	  TREE_TYPE (entry) = NULL_TREE;
	  TREE_VALUE (entry) = rvals;
	}
    }

  if (errstr && protect)
    {
      error (errstr, IDENTIFIER_POINTER (name), TYPE_NAME_STRING (type));
      rvals = error_mark_node;
    }

  return rvals;
}

/* BREADTH-FIRST SEARCH ROUTINES.  */

/* Search a multiple inheritance hierarchy by breadth-first search.

   TYPE is an aggregate type, possibly in a multiple-inheritance hierarchy.
   TESTFN is a function, which, if true, means that our condition has been met,
   and its return value should be returned.
   QFN, if non-NULL, is a predicate dictating whether the type should
   even be queued.  */

HOST_WIDE_INT
breadth_first_search (binfo, testfn, qfn)
     tree binfo;
     int (*testfn)();
     int (*qfn)();
{
  int head = 0, tail = 0;
  int rval = 0;

  search_stack = push_search_level (search_stack, &search_obstack);

  while (1)
    {
      tree binfos = BINFO_BASETYPES (binfo);
      int n_baselinks = binfos ? TREE_VEC_LENGTH (binfos) : 0;
      int i;

      /* Process and/or queue base types.  */
      for (i = 0; i < n_baselinks; i++)
	{
	  tree base_binfo = TREE_VEC_ELT (binfos, i);

	  if (BINFO_MARKED (base_binfo) == 0
	      && (qfn == 0 || (*qfn) (binfo, i)))
	    {
	      SET_BINFO_MARKED (base_binfo);
	      obstack_ptr_grow (&search_obstack, binfo);
	      obstack_ptr_grow (&search_obstack, (HOST_WIDE_INT) i);
	      tail += 2;
	      if (tail >= search_stack->limit)
		my_friendly_abort (100);
	    }
	}
      /* Process head of queue, if one exists.  */
      if (head >= tail)
	{
	  rval = 0;
	  break;
	}

      binfo = search_stack->first[head++];
      i = (HOST_WIDE_INT) search_stack->first[head++];
      if (rval = (*testfn) (binfo, i))
	break;
      binfo = BINFO_BASETYPE (binfo, i);
    }
  {
    tree *tp = search_stack->first;
    tree *search_tail = tp + tail;
    while (tp < search_tail)
      {
	tree binfo = *tp++;
	int i = (HOST_WIDE_INT)(*tp++);
	CLEAR_BINFO_MARKED (BINFO_BASETYPE (binfo, i));
      }
  }

  search_stack = pop_search_level (search_stack);
  return rval;
}

/* Functions to use in breadth first searches.  */
typedef tree (*pft)();
typedef int (*pfi)();

int tree_needs_constructor_p (binfo, i)
     tree binfo;
     int i;
{
  tree basetype;
  my_friendly_assert (i != 0, 296);
  basetype = BINFO_TYPE (BINFO_BASETYPE (binfo, i));
  return TYPE_NEEDS_CONSTRUCTOR (basetype);
}

static tree declarator;

static tree
get_virtuals_named_this (binfo)
     tree binfo;
{
  tree fields;

  fields = lookup_fnfields (binfo, declarator, -1);
  /* fields cannot be error_mark_node */

  if (fields == 0)
    return 0;

  /* Get to the function decls, and return the first virtual function
     with this name, if there is one.  */
  while (fields)
    {
      tree fndecl;

      for (fndecl = TREE_VALUE (fields); fndecl; fndecl = DECL_CHAIN (fndecl))
	if (DECL_VINDEX (fndecl))
	  return fields;
      fields = next_baselink (fields);
    }
  return NULL_TREE;
}

static tree get_virtual_destructor (binfo, i)
     tree binfo;
     int i;
{
  tree type = BINFO_TYPE (binfo);
  if (i >= 0)
    type = BINFO_TYPE (TREE_VEC_ELT (BINFO_BASETYPES (binfo), i));
  if (TYPE_HAS_DESTRUCTOR (type)
      && DECL_VINDEX (TREE_VEC_ELT (CLASSTYPE_METHOD_VEC (type), 0)))
    return TREE_VEC_ELT (CLASSTYPE_METHOD_VEC (type), 0);
  return 0;
}

int tree_has_any_destructor_p (binfo, i)
     tree binfo;
     int i;
{
  tree type = BINFO_TYPE (binfo);
  if (i >= 0)
    type = BINFO_TYPE (TREE_VEC_ELT (BINFO_BASETYPES (binfo), i));
  return TYPE_NEEDS_DESTRUCTOR (type);
}

/* Given a class type TYPE, and a function decl FNDECL,
   look for the first function the TYPE's hierarchy which
   FNDECL could match as a virtual function.

   DTORP is nonzero if we are looking for a destructor.  Destructors
   need special treatment because they do not match by name.  */
tree
get_first_matching_virtual (binfo, fndecl, dtorp)
     tree binfo, fndecl;
     int dtorp;
{
  tree tmp = NULL_TREE;

  /* Breadth first search routines start searching basetypes
     of TYPE, so we must perform first ply of search here.  */
  if (dtorp)
    {
      if (tree_has_any_destructor_p (binfo, -1))
	tmp = get_virtual_destructor (binfo, -1);

      if (tmp)
	{
	  if (get_base_distance (DECL_CONTEXT (tmp),
				 DECL_CONTEXT (fndecl), 0, 0) > 0)
	    DECL_CONTEXT (fndecl) = DECL_CONTEXT (tmp);
	  return tmp;
	}

      tmp = (tree) breadth_first_search (binfo,
					 (pfi) get_virtual_destructor,
					 tree_has_any_destructor_p);
      if (tmp)
	{
	  if (get_base_distance (DECL_CONTEXT (tmp),
				 DECL_CONTEXT (fndecl), 0, 0) > 0)
	    DECL_CONTEXT (fndecl) = DECL_CONTEXT (tmp);
	}
      return tmp;
    }
  else
    {
      tree drettype, dtypes, btypes, instptr_type;
      tree basetype = DECL_CLASS_CONTEXT (fndecl);
      tree baselink, best = NULL_TREE;
      tree name = DECL_ASSEMBLER_NAME (fndecl);

      declarator = DECL_NAME (fndecl);
      if (IDENTIFIER_VIRTUAL_P (declarator) == 0)
	return NULL_TREE;

      drettype = TREE_TYPE (TREE_TYPE (fndecl));
      dtypes = TYPE_ARG_TYPES (TREE_TYPE (fndecl));
      if (DECL_STATIC_FUNCTION_P (fndecl))
	instptr_type = NULL_TREE;
      else
	instptr_type = TREE_TYPE (TREE_VALUE (dtypes));

      for (baselink = get_virtuals_named_this (binfo);
	   baselink; baselink = next_baselink (baselink))
	{
	  for (tmp = TREE_VALUE (baselink); tmp; tmp = DECL_CHAIN (tmp))
	    {
	      if (! DECL_VINDEX (tmp))
		continue;

	      btypes = TYPE_ARG_TYPES (TREE_TYPE (tmp));
	      if (instptr_type == NULL_TREE)
		{
		  if (compparms (TREE_CHAIN (btypes), dtypes, 3))
		    /* Caller knows to give error in this case.  */
		    return tmp;
		  return NULL_TREE;
		}

	      if ((TYPE_READONLY (TREE_TYPE (TREE_VALUE (btypes)))
		   == TYPE_READONLY (instptr_type))
		  && compparms (TREE_CHAIN (btypes), TREE_CHAIN (dtypes), 3))
		{
		  if (IDENTIFIER_ERROR_LOCUS (name) == NULL_TREE
		      && ! comptypes (TREE_TYPE (TREE_TYPE (tmp)), drettype, 1))
		    {
		      cp_error ("conflicting return type specified for virtual function `%D'", fndecl);
		      SET_IDENTIFIER_ERROR_LOCUS (name, basetype);
		    }
		  break;
		}
	    }
	  if (tmp)
	    {
	      /* If this is ambiguous, we will warn about it later.  */
	      if (best)
		{
		  if (get_base_distance (DECL_CLASS_CONTEXT (best),
					 DECL_CLASS_CONTEXT (tmp), 0, 0) > 0)
		    best = tmp;
		}
	      else
		best = tmp;
	    }
	}
      if (best == NULL_TREE && warn_overloaded_virtual)
	cp_warning_at ("conflicting specification deriving virtual function `%D'", fndecl);

      if (best)
	{
	  if (get_base_distance (DECL_CONTEXT (best),
				 DECL_CONTEXT (fndecl), 0, 0) > 0)
	    DECL_CONTEXT (fndecl) = DECL_CONTEXT (best);
	}
      return best;
    }
}

/* Return the list of virtual functions which are abstract in type TYPE.
   This information is cached, and so must be built on a
   non-temporary obstack.  */
tree
get_abstract_virtuals (type)
     tree type;
{
  /* For each layer of base class (i.e., the first base class, and each
     virtual base class from that one), modify the virtual function table
     of the derived class to contain the new virtual function.
     A class has as many vfields as it has virtual base classes (total).  */
  tree vfields, vbases, base, tmp;
  tree vfield = CLASSTYPE_VFIELD (type);
  tree fcontext = vfield ? DECL_FCONTEXT (vfield) : NULL_TREE;
  tree abstract_virtuals = CLASSTYPE_ABSTRACT_VIRTUALS (type);

  for (vfields = CLASSTYPE_VFIELDS (type); vfields; vfields = TREE_CHAIN (vfields))
    {
      int normal;

      /* This code is most likely wrong, and probably only works for single
	 inheritance or by accident. */

      /* Find the right base class for this derived class, call it BASE.  */
      base = VF_BASETYPE_VALUE (vfields);
      if (base == type)
	continue;

      /* We call this case NORMAL iff this virtual function table
	 pointer field has its storage reserved in this class.
	 This is normally the case without virtual baseclasses
	 or off-center multiple baseclasses.  */
      normal = (base == fcontext
		&& (VF_BINFO_VALUE (vfields) == NULL_TREE
		    || ! TREE_VIA_VIRTUAL (VF_BINFO_VALUE (vfields))));

      if (normal)
	tmp = TREE_CHAIN (TYPE_BINFO_VIRTUALS (type));
      else
	{
	  /* n.b.: VF_BASETYPE_VALUE (vfields) is the first basetype
	     that provides the virtual function table, whereas
	     VF_DERIVED_VALUE (vfields) is an immediate base type of TYPE
	     that dominates VF_BASETYPE_VALUE (vfields).  The list of
	     vfields we want lies between these two values.  */
	  tree binfo = get_binfo (VF_NORMAL_VALUE (vfields), type, 0);
	  tmp = TREE_CHAIN (BINFO_VIRTUALS (binfo));
	}

      /* Get around dossier entry if there is one.  */
      if (flag_dossier)
	tmp = TREE_CHAIN (tmp);

      while (tmp)
	{
	  tree base_pfn = FNADDR_FROM_VTABLE_ENTRY (TREE_VALUE (tmp));
	  tree base_fndecl = TREE_OPERAND (base_pfn, 0);
	  if (DECL_ABSTRACT_VIRTUAL_P (base_fndecl))
	    abstract_virtuals = tree_cons (NULL_TREE, base_fndecl, abstract_virtuals);
	  tmp = TREE_CHAIN (tmp);
	}
    }
  for (vbases = CLASSTYPE_VBASECLASSES (type); vbases; vbases = TREE_CHAIN (vbases))
    {
      if (! BINFO_VIRTUALS (vbases))
	continue;

      tmp = TREE_CHAIN (BINFO_VIRTUALS (vbases));
      while (tmp)
	{
	  tree base_pfn = FNADDR_FROM_VTABLE_ENTRY (TREE_VALUE (tmp));
	  tree base_fndecl = TREE_OPERAND (base_pfn, 0);
	  if (DECL_ABSTRACT_VIRTUAL_P (base_fndecl))
	    abstract_virtuals = tree_cons (NULL_TREE, base_fndecl, abstract_virtuals);
	  tmp = TREE_CHAIN (tmp);
	}
    }
  return nreverse (abstract_virtuals);
}

/* For the type TYPE, return a list of member functions available from
   base classes with name NAME.  The TREE_VALUE of the list is a chain of
   member functions with name NAME.  The TREE_PURPOSE of the list is a
   basetype, or a list of base types (in reverse order) which were
   traversed to reach the chain of member functions.  If we reach a base
   type which provides a member function of name NAME, and which has at
   most one base type itself, then we can terminate the search.  */

tree
get_baselinks (type_as_binfo_list, type, name)
     tree type_as_binfo_list;
     tree type, name;
{
  int head = 0, tail = 0, index;
  tree rval = 0, nval = 0;
  tree basetypes = type_as_binfo_list;
  tree binfo = TYPE_BINFO (type);

  search_stack = push_search_level (search_stack, &search_obstack);

  while (1)
    {
      tree binfos = BINFO_BASETYPES (binfo);
      int i, n_baselinks = binfos ? TREE_VEC_LENGTH (binfos) : 0;

      /* Process and/or queue base types.  */
      for (i = 0; i < n_baselinks; i++)
	{
	  tree base_binfo = TREE_VEC_ELT (binfos, i);
	  tree btypes;

	  btypes = hash_tree_cons (TREE_VIA_PUBLIC (base_binfo),
				   TREE_VIA_VIRTUAL (base_binfo),
				   TREE_VIA_PROTECTED (base_binfo),
				   NULL_TREE, base_binfo,
				   basetypes);
	  obstack_ptr_grow (&search_obstack, btypes);
	  search_stack->first = (tree *)obstack_base (&search_obstack);
	  tail += 1;
	}

    dont_queue:
      /* Process head of queue, if one exists.  */
      if (head >= tail)
	break;

      basetypes = search_stack->first[head++];
      binfo = TREE_VALUE (basetypes);
      type = BINFO_TYPE (binfo);
      index = lookup_fnfields_1 (type, name);
      if (index >= 0)
	{
	  nval = TREE_VEC_ELT (CLASSTYPE_METHOD_VEC (type), index);
	  rval = hash_tree_cons (0, 0, 0, basetypes, nval, rval);
	  if (TYPE_BINFO_BASETYPES (type) == 0)
	    goto dont_queue;
	  else if (TREE_VEC_LENGTH (TYPE_BINFO_BASETYPES (type)) == 1)
	    {
	      if (CLASSTYPE_BASELINK_VEC (type))
		TREE_TYPE (rval) = TREE_VEC_ELT (CLASSTYPE_BASELINK_VEC (type), index);
	      goto dont_queue;
	    }
	}
      nval = NULL_TREE;
    }

  search_stack = pop_search_level (search_stack);
  return rval;
}

tree
next_baselink (baselink)
     tree baselink;
{
  tree tmp = TREE_TYPE (baselink);
  baselink = TREE_CHAIN (baselink);
  while (tmp)
    {
      /* @@ does not yet add previous base types.  */
      baselink = tree_cons (TREE_PURPOSE (tmp), TREE_VALUE (tmp),
			    baselink);
      TREE_TYPE (baselink) = TREE_TYPE (tmp);
      tmp = TREE_CHAIN (tmp);
    }
  return baselink;
}

/* DEPTH-FIRST SEARCH ROUTINES.  */

/* Assign unique numbers to _CLASSTYPE members of the lattice
   specified by TYPE.  The root nodes are marked first; the nodes
   are marked depth-fisrt, left-right.  */

static int cid;

/* Matrix implementing a relation from CLASSTYPE X CLASSTYPE => INT.
   Relation yields 1 if C1 <= C2, 0 otherwise.  */
typedef char mi_boolean;
static mi_boolean *mi_matrix;

/* Type for which this matrix is defined.  */
static tree mi_type;

/* Size of the matrix for indexing purposes.  */
static int mi_size;

/* Return nonzero if class C2 derives from class C1.  */
#define BINFO_DERIVES_FROM(C1, C2)	\
  ((mi_matrix+mi_size*(BINFO_CID (C1)-1))[BINFO_CID (C2)-1])
#define TYPE_DERIVES_FROM(C1, C2)	\
  ((mi_matrix+mi_size*(CLASSTYPE_CID (C1)-1))[CLASSTYPE_CID (C2)-1])
#define BINFO_DERIVES_FROM_STAR(C)	\
  (mi_matrix+(BINFO_CID (C)-1))

/* This routine converts a pointer to be a pointer of an immediate
   base class.  The normal convert_pointer_to routine would diagnose
   the conversion as ambiguous, under MI code that has the base class
   as an ambiguous base class. */
static tree
convert_pointer_to_single_level (to_type, expr)
     tree to_type, expr;
{
  tree binfo_of_derived;
  tree last;

  binfo_of_derived = TYPE_BINFO (TREE_TYPE (TREE_TYPE (expr)));
  last = get_binfo (to_type, TREE_TYPE (TREE_TYPE (expr)), 0);
  BINFO_INHERITANCE_CHAIN (last) = binfo_of_derived;
  BINFO_INHERITANCE_CHAIN (binfo_of_derived) = NULL_TREE;
  return build_vbase_path (PLUS_EXPR, TYPE_POINTER_TO (to_type), expr, last, 1);
}

/* The main function which implements depth first search.

   This routine has to remember the path it walked up, when
   dfs_init_vbase_pointers is the work function, as otherwise there
   would be no record. */
static void
dfs_walk (binfo, fn, qfn)
     tree binfo;
     void (*fn)();
     int (*qfn)();
{
  tree binfos = BINFO_BASETYPES (binfo);
  int i, n_baselinks = binfos ? TREE_VEC_LENGTH (binfos) : 0;

  for (i = 0; i < n_baselinks; i++)
    {
      tree base_binfo = TREE_VEC_ELT (binfos, i);

      if ((*qfn)(base_binfo))
	{
	  if (fn == dfs_init_vbase_pointers)
	    {
	      /* When traversing an arbitrary MI hierarchy, we need to keep
		 a record of the path we took to get down to the final base
		 type, as otherwise there would be no record of it, and just
		 trying to blindly convert at the bottom would be ambiguous.

		 The easiest way is to do the conversions one step at a time,
		 as we know we want the immediate base class at each step.

		 The only special trick to converting one step at a time,
		 is that when we hit the last virtual base class, we must
		 use the SLOT value for it, and not use the normal convert
		 routine.  We use the last virtual base class, as in our
		 implementation, we have pointers to all virtual base
		 classes in the base object.  */

	      tree saved_vbase_decl_ptr_intermediate
		= vbase_decl_ptr_intermediate;

	      if (TREE_VIA_VIRTUAL (base_binfo))
		{
		  /* No need for the conversion here, as we know it is the
		     right type.  */
		  vbase_decl_ptr_intermediate
		    = (tree)CLASSTYPE_SEARCH_SLOT (BINFO_TYPE (base_binfo));
		}
	      else
		{
		  vbase_decl_ptr_intermediate
		    = convert_pointer_to_single_level (BINFO_TYPE (base_binfo),
						       vbase_decl_ptr_intermediate);
		}

	      dfs_walk (base_binfo, fn, qfn);

	      vbase_decl_ptr_intermediate = saved_vbase_decl_ptr_intermediate;
	    } else
	      dfs_walk (base_binfo, fn, qfn);
	}
    }

  fn (binfo);
}

/* Predicate functions which serve for dfs_walk.  */
static int numberedp (binfo) tree binfo;
{ return BINFO_CID (binfo); }
static int unnumberedp (binfo) tree binfo;
{ return BINFO_CID (binfo) == 0; }

static int markedp (binfo) tree binfo;
{ return BINFO_MARKED (binfo); }
static int bfs_markedp (binfo, i) tree binfo; int i;
{ return BINFO_MARKED (BINFO_BASETYPE (binfo, i)); }
static int unmarkedp (binfo) tree binfo;
{ return BINFO_MARKED (binfo) == 0; }
static int bfs_unmarkedp (binfo, i) tree binfo; int i;
{ return BINFO_MARKED (BINFO_BASETYPE (binfo, i)) == 0; }
static int marked_vtable_pathp (binfo) tree binfo;
{ return BINFO_VTABLE_PATH_MARKED (binfo); }
static int bfs_marked_vtable_pathp (binfo, i) tree binfo; int i;
{ return BINFO_VTABLE_PATH_MARKED (BINFO_BASETYPE (binfo, i)); }
static int unmarked_vtable_pathp (binfo) tree binfo;
{ return BINFO_VTABLE_PATH_MARKED (binfo) == 0; }
static int bfs_unmarked_vtable_pathp (binfo, i) tree binfo; int i;
{ return BINFO_VTABLE_PATH_MARKED (BINFO_BASETYPE (binfo, i)) == 0; }
static int marked_new_vtablep (binfo) tree binfo;
{ return BINFO_NEW_VTABLE_MARKED (binfo); }
static int bfs_marked_new_vtablep (binfo, i) tree binfo; int i;
{ return BINFO_NEW_VTABLE_MARKED (BINFO_BASETYPE (binfo, i)); }
static int unmarked_new_vtablep (binfo) tree binfo;
{ return BINFO_NEW_VTABLE_MARKED (binfo) == 0; }
static int bfs_unmarked_new_vtablep (binfo, i) tree binfo; int i;
{ return BINFO_NEW_VTABLE_MARKED (BINFO_BASETYPE (binfo, i)) == 0; }

static int dfs_search_slot_nonempty_p (binfo) tree binfo;
{ return CLASSTYPE_SEARCH_SLOT (BINFO_TYPE (binfo)) != 0; }

static int dfs_debug_unmarkedp (binfo) tree binfo;
{ return CLASSTYPE_DEBUG_REQUESTED (BINFO_TYPE (binfo)) == 0; }

/* The worker functions for `dfs_walk'.  These do not need to
   test anything (vis a vis marking) if they are paired with
   a predicate function (above).  */

/* Assign each type within the lattice a number which is unique
   in the lattice.  The first number assigned is 1.  */

static void
dfs_number (binfo)
     tree binfo;
{
  BINFO_CID (binfo) = ++cid;
}

static void
dfs_unnumber (binfo)
     tree binfo;
{
  BINFO_CID (binfo) = 0;
}

static void
dfs_mark (binfo) tree binfo;
{ SET_BINFO_MARKED (binfo); }

static void
dfs_unmark (binfo) tree binfo;
{ CLEAR_BINFO_MARKED (binfo); }

static void
dfs_mark_vtable_path (binfo) tree binfo;
{ SET_BINFO_VTABLE_PATH_MARKED (binfo); }

static void
dfs_unmark_vtable_path (binfo) tree binfo;
{ CLEAR_BINFO_VTABLE_PATH_MARKED (binfo); }

static void
dfs_mark_new_vtable (binfo) tree binfo;
{ SET_BINFO_NEW_VTABLE_MARKED (binfo); }

static void
dfs_unmark_new_vtable (binfo) tree binfo;
{ CLEAR_BINFO_NEW_VTABLE_MARKED (binfo); }

static void
dfs_clear_search_slot (binfo) tree binfo;
{ CLASSTYPE_SEARCH_SLOT (BINFO_TYPE (binfo)) = 0; }

static void
dfs_debug_mark (binfo)
     tree binfo;
{
  tree t = BINFO_TYPE (binfo);

  /* Use heuristic that if there are virtual functions,
     ignore until we see a non-inline virtual function.  */
  tree methods = CLASSTYPE_METHOD_VEC (t);

  CLASSTYPE_DEBUG_REQUESTED (t) = 1;

  /* If interface info is known, the value of (?@@?) is correct.  */
  if (methods == 0
      || CLASSTYPE_INTERFACE_KNOWN (t)
      || (write_virtuals == 2 && TYPE_VIRTUAL_P (t)))
    return;

  /* If debug info is requested from this context for this type, supply it.
     If debug info is requested from another context for this type,
     see if some third context can supply it.  */
  if (current_function_decl == NULL_TREE
      || DECL_CLASS_CONTEXT (current_function_decl) != t)
    {
      if (TREE_VEC_ELT (methods, 0))
	methods = TREE_VEC_ELT (methods, 0);
      else
	methods = TREE_VEC_ELT (methods, 1);
      while (methods)
	{
	  if (DECL_VINDEX (methods)
	      && DECL_SAVED_INSNS (methods) == 0
	      && DECL_PENDING_INLINE_INFO (methods) == 0
	      && DECL_ABSTRACT_VIRTUAL_P (methods) == 0)
	    {
	      /* Somebody, somewhere is going to have to define this
		 virtual function.  When they do, they will provide
		 the debugging info.  */
	      return;
	    }
	  methods = TREE_CHAIN (methods);
	}
    }
  /* We cannot rely on some alien method to solve our problems,
     so we must write out the debug info ourselves.  */
  if (write_symbols != DWARF_DEBUG)
    DECL_IGNORED_P (TYPE_NAME (t)) = 0;
  if (! TREE_ASM_WRITTEN (TYPE_NAME (t)))
    rest_of_type_compilation (t, global_bindings_p ());
}

/*  Attach to the type of the virtual base class, the pointer to the
    virtual base class, given the global pointer vbase_decl_ptr.  */
static void
dfs_find_vbases (binfo)
     tree binfo;
{
  tree binfos = BINFO_BASETYPES (binfo);
  int i, n_baselinks = binfos ? TREE_VEC_LENGTH (binfos) : 0;

  for (i = n_baselinks-1; i >= 0; i--)
    {
      tree base_binfo = TREE_VEC_ELT (binfos, i);

      if (TREE_VIA_VIRTUAL (base_binfo)
	  && CLASSTYPE_SEARCH_SLOT (BINFO_TYPE (base_binfo)) == 0)
	{
	  tree vbase = BINFO_TYPE (base_binfo);
	  tree binfo = binfo_member (vbase, vbase_types);

	  CLASSTYPE_SEARCH_SLOT (vbase)
	    = (char *) build (PLUS_EXPR, TYPE_POINTER_TO (vbase),
			      vbase_decl_ptr, BINFO_OFFSET (binfo));
	}
    }
  SET_BINFO_VTABLE_PATH_MARKED (binfo);
  SET_BINFO_NEW_VTABLE_MARKED (binfo);
}

static void
dfs_init_vbase_pointers (binfo)
     tree binfo;
{
  tree type = BINFO_TYPE (binfo);
  tree fields = TYPE_FIELDS (type);
  tree path, this_vbase_ptr;
  int distance;

  CLEAR_BINFO_VTABLE_PATH_MARKED (binfo);

  /* If there is a dossier, it is the first field, though perhaps from
     the base class.  Otherwise, the first fields are virtual base class
     pointer fields.  */
  if (CLASSTYPE_DOSSIER (type) && VFIELD_NAME_P (DECL_NAME (fields)))
    /* Get past vtable for the object.  */
    fields = TREE_CHAIN (fields);

  if (fields == NULL_TREE
      || DECL_NAME (fields) == NULL_TREE
      || ! VBASE_NAME_P (DECL_NAME (fields)))
    return;

  this_vbase_ptr = vbase_decl_ptr_intermediate;

  if (TYPE_POINTER_TO (type) != TREE_TYPE (this_vbase_ptr))
    my_friendly_abort (125);

  while (fields && DECL_NAME (fields)
	 && VBASE_NAME_P (DECL_NAME (fields)))
    {
      tree ref = build (COMPONENT_REF, TREE_TYPE (fields),
			build_indirect_ref (this_vbase_ptr, NULL_PTR), fields);
      tree init = (tree)CLASSTYPE_SEARCH_SLOT (TREE_TYPE (TREE_TYPE (fields)));
      vbase_init_result = tree_cons (binfo_member (TREE_TYPE (TREE_TYPE (fields)),
						   vbase_types),
				     build_modify_expr (ref, NOP_EXPR, init),
				     vbase_init_result);
      fields = TREE_CHAIN (fields);
    }
}

/* Sometimes this needs to clear both VTABLE_PATH and NEW_VTABLE.  Other
   times, just NEW_VTABLE, but optimizer should make both with equal
   efficiency (though it does not currently).  */
static void
dfs_clear_vbase_slots (binfo)
     tree binfo;
{
  tree type = BINFO_TYPE (binfo);
  CLASSTYPE_SEARCH_SLOT (type) = 0;
  CLEAR_BINFO_VTABLE_PATH_MARKED (binfo);
  CLEAR_BINFO_NEW_VTABLE_MARKED (binfo);
}

tree
init_vbase_pointers (type, decl_ptr)
     tree type;
     tree decl_ptr;
{
  if (TYPE_USES_VIRTUAL_BASECLASSES (type))
    {
      int old_flag = flag_this_is_variable;
      tree binfo = TYPE_BINFO (type);
      flag_this_is_variable = -2;
      vbase_types = CLASSTYPE_VBASECLASSES (type);
      vbase_decl_ptr = decl_ptr;
      vbase_decl = build_indirect_ref (decl_ptr, NULL_PTR);
      vbase_decl_ptr_intermediate = vbase_decl_ptr;
      vbase_init_result = NULL_TREE;
      dfs_walk (binfo, dfs_find_vbases, unmarked_vtable_pathp);
      dfs_walk (binfo, dfs_init_vbase_pointers, marked_vtable_pathp);
      dfs_walk (binfo, dfs_clear_vbase_slots, marked_new_vtablep);
      flag_this_is_variable = old_flag;
      return vbase_init_result;
    }
  return 0;
}

/* Build a COMPOUND_EXPR which when expanded will generate the code
   needed to initialize all the virtual function table slots of all
   the virtual baseclasses.  FOR_TYPE is the type which determines the
   virtual baseclasses to use; TYPE is the type of the object to which
   the initialization applies.  TRUE_EXP is the true object we are
   initializing, and DECL_PTR is the pointer to the sub-object we
   are initializing.

   When USE_COMPUTED_OFFSETS is non-zero, we can assume that the
   object was laidout by a top-level contructor and the computed
   offsets are valid to store vtables.  When zero, we must store new
   vtables through virtual baseclass pointers.  */

tree
build_vbase_vtables_init (main_binfo, binfo, true_exp, decl_ptr,
			  use_computed_offsets)
     tree main_binfo, binfo;
     tree true_exp, decl_ptr;
     int use_computed_offsets;
{
  tree for_type = BINFO_TYPE (main_binfo);
  tree type = BINFO_TYPE (binfo);
  if (TYPE_USES_VIRTUAL_BASECLASSES (type))
    {
      int old_flag = flag_this_is_variable;
      tree vtable_init_result = NULL_TREE;
      tree vbases = CLASSTYPE_VBASECLASSES (type);

      vbase_types = CLASSTYPE_VBASECLASSES (for_type);
      vbase_decl_ptr = true_exp ? build_unary_op (ADDR_EXPR, true_exp, 0) : decl_ptr;
      vbase_decl = true_exp ? true_exp : build_indirect_ref (decl_ptr, NULL_PTR);

      if (use_computed_offsets)
	{
	  /* This is an object of type IN_TYPE,  */
	  flag_this_is_variable = -2;
	  dfs_walk (main_binfo, dfs_find_vbases, unmarked_new_vtablep);
	}

      /* Initialized with vtables of type TYPE.  */
      while (vbases)
	{
	  /* This time through, not every class's vtable
	     is going to be initialized.  That is, we only initialize
	     the "last" vtable pointer.  */

	  if (CLASSTYPE_VSIZE (BINFO_TYPE (vbases)))
	    {
	      tree addr;
	      tree vtbl = BINFO_VTABLE (vbases);
	      tree init = build_unary_op (ADDR_EXPR, vtbl, 0);
	      assemble_external (vtbl);
	      TREE_USED (vtbl) = 1;

	      if (use_computed_offsets)
		addr = (tree)CLASSTYPE_SEARCH_SLOT (BINFO_TYPE (vbases));
	      else
		addr = convert_pointer_to (vbases, vbase_decl_ptr);

	      if (addr)
		{
		  tree ref = build_vfield_ref (build_indirect_ref (addr, NULL_PTR),
					       BINFO_TYPE (vbases));
		  init = convert_force (TREE_TYPE (ref), init);
		  vtable_init_result = tree_cons (NULL_TREE, build_modify_expr (ref, NOP_EXPR, init),
						  vtable_init_result);
		}
	    }
	  vbases = TREE_CHAIN (vbases);
	}

      dfs_walk (binfo, dfs_clear_vbase_slots, marked_new_vtablep);

      flag_this_is_variable = old_flag;
      if (vtable_init_result)
	return build_compound_expr (vtable_init_result);
    }
  return error_mark_node;
}

void
clear_search_slots (type)
     tree type;
{
  dfs_walk (TYPE_BINFO (type),
	    dfs_clear_search_slot, dfs_search_slot_nonempty_p);
}

/* get virtual base class types.
   This adds type to the vbase_types list in reverse dfs order.
   Ordering is very important, so don't change it.  */

static void
dfs_get_vbase_types (binfo)
     tree binfo;
{
  tree binfos = BINFO_BASETYPES (binfo);
  tree type = BINFO_TYPE (binfo);
  tree these_vbase_types = CLASSTYPE_VBASECLASSES (type);
  int i, n_baselinks = binfos ? TREE_VEC_LENGTH (binfos) : 0;

  if (these_vbase_types)
    {
      while (these_vbase_types)
	{
	  tree this_type = BINFO_TYPE (these_vbase_types);

	  /* We really need to start from a fresh copy of this
	     virtual basetype!  CLASSTYPE_MARKED2 is the shortcut
	     for BINFO_VBASE_MARKED.  */
	  if (! CLASSTYPE_MARKED2 (this_type))
	    {
	      vbase_types = make_binfo (integer_zero_node,
					this_type,
					TYPE_BINFO_VTABLE (this_type),
					TYPE_BINFO_VIRTUALS (this_type),
					vbase_types);
	      TREE_VIA_VIRTUAL (vbase_types) = 1;
	      SET_CLASSTYPE_MARKED2 (this_type);
	    }
	  these_vbase_types = TREE_CHAIN (these_vbase_types);
	}
    }
  else for (i = 0; i < n_baselinks; i++)
    {
      tree base_binfo = TREE_VEC_ELT (binfos, i);
      if (TREE_VIA_VIRTUAL (base_binfo) && ! BINFO_VBASE_MARKED (base_binfo))
	{
	  vbase_types = make_binfo (integer_zero_node, BINFO_TYPE (base_binfo),
				    BINFO_VTABLE (base_binfo),
				    BINFO_VIRTUALS (base_binfo), vbase_types);
	  TREE_VIA_VIRTUAL (vbase_types) = 1;
	  SET_BINFO_VBASE_MARKED (base_binfo);
	}
    }
  SET_BINFO_MARKED (binfo);
}

/* Some virtual baseclasses might be virtual baseclasses for
   other virtual baseclasses.  We sort the virtual baseclasses
   topologically: in the list returned, the first virtual base
   classes have no virtual baseclasses themselves, and any entry
   on the list has no dependency on virtual base classes later in the
   list.  */
tree
get_vbase_types (type)
     tree type;
{
  tree ordered_vbase_types = NULL_TREE, prev, next;
  tree vbases;

  vbase_types = NULL_TREE;
  dfs_walk (TYPE_BINFO (type), dfs_get_vbase_types, unmarkedp);
  dfs_walk (TYPE_BINFO (type), dfs_unmark, markedp);
  /* Rely upon the reverse dfs ordering from dfs_get_vbase_types, and now
     reverse it so that we get normal dfs ordering.  */
  vbase_types = nreverse (vbase_types);

  /* Almost all of the below is not needed now.  We should be able to just
     return vbase_types directly... (mrs) */
  while (vbase_types)
    {
      /* Now sort these types.  This is essentially a bubble merge.  */

      /* Farm out virtual baseclasses which have no marked ancestors.  */
      for (vbases = vbase_types, prev = NULL_TREE;
	   vbases; vbases = next)
	{
	  next = TREE_CHAIN (vbases);
	  /* If VBASES does not have any vbases itself, or it's
	     topologically safe, it goes into the sorted list.  */
	  if (1 /* ANSI C++ specifies dfs ordering now. */
	      || ! CLASSTYPE_VBASECLASSES (BINFO_TYPE (vbases))
	      || BINFO_VBASE_MARKED (vbases) == 0)
	    {
	      if (prev)
		TREE_CHAIN (prev) = TREE_CHAIN (vbases);
	      else
		vbase_types = TREE_CHAIN (vbases);
	      TREE_CHAIN (vbases) = NULL_TREE;
	      ordered_vbase_types = chainon (ordered_vbase_types, vbases);
	      CLEAR_BINFO_VBASE_MARKED (vbases);
	    }
	  else
	    prev = vbases;
	}

      /* Now unmark types all of whose ancestors are now on the
	 `ordered_vbase_types' list.  */
      for (vbases = vbase_types; vbases; vbases = TREE_CHAIN (vbases))
	{
	  /* If all our virtual baseclasses are unmarked, ok.  */
	  tree t = CLASSTYPE_VBASECLASSES (BINFO_TYPE (vbases));
	  while (t && (BINFO_VBASE_MARKED (t) == 0
		       || ! CLASSTYPE_VBASECLASSES (BINFO_TYPE (t))))
	    t = TREE_CHAIN (t);
	  if (t == NULL_TREE)
	    CLEAR_BINFO_VBASE_MARKED (vbases);
	}
    }

  return ordered_vbase_types;
}

static void
dfs_record_inheritance (binfo)
     tree binfo;
{
  tree binfos = BINFO_BASETYPES (binfo);
  int i, n_baselinks = binfos ? TREE_VEC_LENGTH (binfos) : 0;
  mi_boolean *derived_row = BINFO_DERIVES_FROM_STAR (binfo);

  for (i = n_baselinks-1; i >= 0; i--)
    {
      int j;
      tree base_binfo = TREE_VEC_ELT (binfos, i);
      tree baseclass = BINFO_TYPE (base_binfo);
      mi_boolean *base_row = BINFO_DERIVES_FROM_STAR (base_binfo);

      /* Don't search if there's nothing there!  MI_SIZE can be
	 zero as a result of parse errors.  */
      if (TYPE_BINFO_BASETYPES (baseclass) && mi_size > 0)
	for (j = mi_size*(CLASSTYPE_CID (baseclass)-1); j >= 0; j -= mi_size)
	  derived_row[j] |= base_row[j];
      TYPE_DERIVES_FROM (baseclass, BINFO_TYPE (binfo)) = 1;
    }

  SET_BINFO_MARKED (binfo);
}

/* Given a _CLASSTYPE node in a multiple inheritance lattice,
   convert the lattice into a simple relation such that,
   given to CIDs, C1 and C2, one can determine if C1 <= C2
   or C2 <= C1 or C1 <> C2.

   Once constructed, we walk the lattice depth fisrt,
   applying various functions to elements as they are encountered.

   We use xmalloc here, in case we want to randomly free these tables.  */

#define SAVE_MI_MATRIX

void
build_mi_matrix (type)
     tree type;
{
  tree binfo = TYPE_BINFO (type);
  cid = 0;

#ifdef SAVE_MI_MATRIX
  if (CLASSTYPE_MI_MATRIX (type))
    {
      mi_size = CLASSTYPE_N_SUPERCLASSES (type) + CLASSTYPE_N_VBASECLASSES (type);
      mi_matrix = CLASSTYPE_MI_MATRIX (type);
      mi_type = type;
      dfs_walk (binfo, dfs_number, unnumberedp);
      return;
    }
#endif

  mi_size = CLASSTYPE_N_SUPERCLASSES (type) + CLASSTYPE_N_VBASECLASSES (type);
  mi_matrix = (char *)xmalloc ((mi_size + 1) * (mi_size + 1));
  mi_type = type;
  bzero (mi_matrix, (mi_size + 1) * (mi_size + 1));
  dfs_walk (binfo, dfs_number, unnumberedp);
  dfs_walk (binfo, dfs_record_inheritance, unmarkedp);
  dfs_walk (binfo, dfs_unmark, markedp);
}

void
free_mi_matrix ()
{
  dfs_walk (TYPE_BINFO (mi_type), dfs_unnumber, numberedp);

#ifdef SAVE_MI_MATRIX
  CLASSTYPE_MI_MATRIX (mi_type) = mi_matrix;
#else
  free (mi_matrix);
  mi_size = 0;
  cid = 0;
#endif
}

/* Local variables for detecting ambiguities of virtual functions
   when two or more classes are joined at a multiple inheritance
   seam.  */
typedef struct {
  tree decl;
  tree args;
  tree ptr;
} mi_ventry;
static mi_ventry *mi_vmatrix;
static int *mi_vmax;
static int mi_vrows, mi_vcols;
#define MI_VMATRIX(ROW,COL) ((mi_vmatrix + (ROW)*mi_vcols)[COL])

/* Build a table of virtual functions for a multiple-inheritance
   structure.  Here, there are N base classes, and at most
   M entries per class.

   This function does nothing if N is 0 or 1.  */
void
build_mi_virtuals (rows, cols)
     int rows, cols;
{
  if (rows < 2 || cols == 0)
    return;
  mi_vrows = rows;
  mi_vcols = cols;
  mi_vmatrix = (mi_ventry *)xmalloc ((rows+1) * cols * sizeof (mi_ventry));
  mi_vmax = (int *)xmalloc ((rows+1) * sizeof (int));

  bzero (mi_vmax, rows * sizeof (int));

  /* Row indices start at 1, so adjust this.  */
  mi_vmatrix -= cols;
  mi_vmax -= 1;
}

/* Comparison function for ordering virtual function table entries.  */
static int
rank_mi_virtuals (v1, v2)
     mi_ventry *v1, *v2;
{
  tree p1, p2;
  int i;

  i = (long) (DECL_NAME (v1->decl)) - (long) (DECL_NAME (v2->decl));
  if (i)
    return i;
  p1 = v1->args;
  p2 = v2->args;

  if (p1 == p2)
    return 0;

  while (p1 && p2)
    {
      i = ((long) (TREE_VALUE (p1)) - (long) (TREE_VALUE (p2)));
      if (i)
	return i;

      if (TREE_CHAIN (p1))
	{
	  if (! TREE_CHAIN (p2))
	    return 1;
	  p1 = TREE_CHAIN (p1);
	  p2 = TREE_CHAIN (p2);
	}
      else if (TREE_CHAIN (p2))
	return -1;
      else
	{
	  /* When matches of argument lists occur, pick lowest
	     address to keep searching time to a minimum on
	     later passes--like hashing, only different.
	     *MUST BE STABLE*.  */
	  if ((long) (v2->args) < (long) (v1->args))
	    v1->args = v2->args;
	  else
	    v2->args = v1->args;
	  return 0;
	}
    }
  return 0;
}

/* Install the virtuals functions got from the initializer VIRTUALS to
   the table at index ROW.  */
void
add_mi_virtuals (row, virtuals)
     int row;
     tree virtuals;
{
  int col = 0;

  if (mi_vmatrix == 0)
    return;
  while (virtuals)
    {
      tree decl = TREE_OPERAND (FNADDR_FROM_VTABLE_ENTRY (TREE_VALUE (virtuals)), 0);
      MI_VMATRIX (row, col).decl = decl;
      MI_VMATRIX (row, col).args = FUNCTION_ARG_CHAIN (decl);
      MI_VMATRIX (row, col).ptr = TREE_VALUE (virtuals);
      virtuals = TREE_CHAIN (virtuals);
      col += 1;
    }
  mi_vmax[row] = col;

  qsort (mi_vmatrix + row * mi_vcols,
	 col,
	 sizeof (mi_ventry),
	 rank_mi_virtuals);
}

/* If joining two types results in an ambiguity in the virtual
   function table, report such here.  */
void
report_ambiguous_mi_virtuals (rows, type)
     int rows;
     tree type;
{
  int *mi_vmin;
  int row1, col1, row, col;

  if (mi_vmatrix == 0)
    return;

  /* Now virtuals are all sorted, so we merge to find ambiguous cases.  */
  mi_vmin = (int *)alloca ((rows+1) * sizeof (int));
  bzero (mi_vmin, rows * sizeof (int));

  /* adjust.  */
  mi_vmin -= 1;

  /* For each base class with virtual functions (and this includes views
     of the virtual baseclasses from different base classes), see that
     each virtual function in that base class has a unique meet.

     When the column loop is finished, THIS_DECL is in fact the meet.
     If that value does not appear in the virtual function table for
     the row, install it.  This happens when that virtual function comes
     from a virtual baseclass, or a non-leftmost baseclass.  */

  for (row1 = 1; row1 < rows; row1++)
    {
      tree this_decl = 0;

      for (col1 = mi_vmax[row1]-1; col1 >= mi_vmin[row1]; col1--)
	{
	  tree these_args = MI_VMATRIX (row1, col1).args;
	  tree this_context;

	  this_decl = MI_VMATRIX (row1, col1).decl;
	  if (this_decl == 0)
	    continue;
	  this_context = TYPE_BINFO (DECL_CLASS_CONTEXT (this_decl));

	  if (this_context != TYPE_BINFO (type))
	    this_context = get_binfo (this_context, type, 0);

	  for (row = row1+1; row <= rows; row++)
	    for (col = mi_vmax[row]-1; col >= mi_vmin[row]; col--)
	      {
		mi_ventry this_entry;

		if (MI_VMATRIX (row, col).decl == 0)
		  continue;

		this_entry.decl = this_decl;
		this_entry.args = these_args;
		this_entry.ptr = MI_VMATRIX (row1, col1).ptr;
		if (rank_mi_virtuals (&this_entry, &MI_VMATRIX (row, col)) == 0)
		  {
		    /* They are equal.  There are four possibilities:

		       (1) Derived class is defining this virtual function.
		       (2) Two paths to the same virtual function in the
		       same base class.
		       (3) A path to a virtual function declared in one base
		       class, and another path to a virtual function in a
		       base class of the base class.
		       (4) Two paths to the same virtual function in different
		       base classes.

		       The first three cases are ok (non-ambiguous).  */

		    tree that_context, tmp;
		    int this_before_that;

		    if (type == BINFO_TYPE (this_context))
		      /* case 1.  */
		      goto ok;
		    that_context = get_binfo (DECL_CLASS_CONTEXT (MI_VMATRIX (row, col).decl), type, 0);
		    if (that_context == this_context)
		      /* case 2.  */
		      goto ok;
		    if (that_context != NULL_TREE)
		      {
			tmp = get_binfo (that_context, this_context, 0);
			this_before_that = (that_context != tmp);
			if (this_before_that == 0)
			  /* case 3a.  */
			  goto ok;
			tmp = get_binfo (this_context, that_context, 0);
			this_before_that = (this_context == tmp);
			if (this_before_that != 0)
			  /* case 3b.  */
			  goto ok;

			/* case 4.  */
			/* These two are not hard errors, but could be
			   symptoms of bad code.  The resultant code
			   the compiler generates needs to be checked.
			   (mrs) */
#if 0
			cp_error_at ("ambiguous virtual function `%s'",
				       MI_VMATRIX (row, col).decl);
			cp_error_at ("ambiguating function `%D' (joined by type `%T')", this_decl, current_class_name);
#endif
		      }
		  ok:
		    MI_VMATRIX (row, col).decl = 0;

		    /* Let zeros propagate.  */
		    if (col == mi_vmax[row]-1)
		      {
			int i = col;
			while (i >= mi_vmin[row]
			       && MI_VMATRIX (row, i).decl == 0)
			  i--;
			mi_vmax[row] = i+1;
		      }
		    else if (col == mi_vmin[row])
		      {
			int i = col;
			while (i < mi_vmax[row]
			       && MI_VMATRIX (row, i).decl == 0)
			  i++;
			mi_vmin[row] = i;
		      }
		  }
	      }
	}
    }
  free (mi_vmatrix + mi_vcols);
  mi_vmatrix = 0;
  free (mi_vmax + 1);
  mi_vmax = 0;
}

/* If we want debug info for a type TYPE, make sure all its base types
   are also marked as being potentially interesting.  This avoids
   the problem of not writing any debug info for intermediate basetypes
   that have abstract virtual functions.  */

void
note_debug_info_needed (type)
     tree type;
{
  dfs_walk (TYPE_BINFO (type), dfs_debug_mark, dfs_debug_unmarkedp);
}

/* Subroutines of push_class_decls ().  */

/* Add the instance variables which this class contributed to the
   current class binding contour.  When a redefinition occurs,
   if the redefinition is strictly within a single inheritance path,
   we just overwrite (in the case of a data field) or
   cons (in the case of a member function) the old declaration with
   the new.  If the fields are not within a single inheritance path,
   we must cons them in either case.

   when NEW_CLASS_SCOPING is 1:

   In order to know what decls are new (stemming from the current
   invocation of push_class_decls) we enclose them in an "envelope",
   which is a TREE_LIST node where the TREE_PURPOSE slot contains the
   new decl (or possibly a list of competing ones), the TREE_VALUE slot
   points to the old value and the TREE_CHAIN slot chains together all
   envelopes which needs to be "opened" in push_class_decls.  Opening an
   envelope means: push the old value onto the class_shadowed list,
   install the new one and if it's a TYPE_DECL do the same to the
   IDENTIFIER_TYPE_VALUE.  Such an envelope is recognized by seeing that
   the TREE_PURPOSE slot is non-null, and that it is not an identifier.
   Because if it is, it could be a set of overloaded methods from an
   outer scope.  */

static void
dfs_pushdecls (binfo)
     tree binfo;
{
  tree type = BINFO_TYPE (binfo);
  tree fields, *methods, *end;
  tree method_vec;

  for (fields = TYPE_FIELDS (type); fields; fields = TREE_CHAIN (fields))
    {
      /* Unmark so that if we are in a constructor, and then find that
	 this field was initialized by a base initializer,
	 we can emit an error message.  */
      if (TREE_CODE (fields) == FIELD_DECL)
	TREE_USED (fields) = 0;

      /* Recurse into anonymous unions.  */
      if (DECL_NAME (fields) == NULL_TREE
	  && TREE_CODE (TREE_TYPE (fields)) == UNION_TYPE)
	{
	  dfs_pushdecls (TYPE_BINFO (TREE_TYPE (fields)));
	  continue;
	}

      if (TREE_CODE (fields) != TYPE_DECL)
	{
	  DECL_PUBLIC (fields) = 0;
	  DECL_PROTECTED (fields) = 0;
	  DECL_PRIVATE (fields) = 0;
	}

      if (DECL_NAME (fields))
	{
#if NEW_CLASS_SCOPING
	  tree class_value = IDENTIFIER_CLASS_VALUE (DECL_NAME (fields));

	  /* If the class value is an envelope of the kind described in
	     the comment above, we try to rule out possible ambiguities.
	     If we can't do that, keep a TREE_LIST with possibly ambiguous
	     decls in there.  */
	  if (class_value && TREE_CODE (class_value) == TREE_LIST
	      && TREE_PURPOSE (class_value) != NULL_TREE
	      && (TREE_CODE (TREE_PURPOSE (class_value))
		  != IDENTIFIER_NODE))
	    {
	      tree value = TREE_PURPOSE (class_value);
#else
	  tree value = IDENTIFIER_CLASS_VALUE (DECL_NAME (fields));
	  if (value)
	    {
#endif
	      tree context;

	      /* Possible ambiguity.  If its defining type(s)
		 is (are all) derived from us, no problem.  */
	      if (TREE_CODE (value) != TREE_LIST)
		{
		  context = (TREE_CODE (value) == FUNCTION_DECL
			     && DECL_VIRTUAL_P (value))
		    ? DECL_CLASS_CONTEXT (value)
		      : DECL_CONTEXT (value);

		  if (context && (context == type
				  || TYPE_DERIVES_FROM (context, type)))
		    value = fields;
		  else
		    value = tree_cons (NULL_TREE, fields,
				       build_tree_list (NULL_TREE, value));
		}
	      else
		{
		  /* All children may derive from us, in which case
		     there is no problem.  Otherwise, we have to
		     keep lists around of what the ambiguities might be.  */
		  tree values;
		  int problem = 0;

		  for (values = value; values; values = TREE_CHAIN (values))
		    {
		      tree sub_values = TREE_VALUE (values);

		      if (TREE_CODE (sub_values) == TREE_LIST)
			{
			  for (; sub_values; sub_values = TREE_CHAIN (sub_values))
			    {
			      register tree list_mbr = TREE_VALUE (sub_values);

			      context = (TREE_CODE (list_mbr) == FUNCTION_DECL
					 && DECL_VIRTUAL_P (list_mbr))
				? DECL_CLASS_CONTEXT (list_mbr)
				  : DECL_CONTEXT (list_mbr);

			      if (! TYPE_DERIVES_FROM (context, type))
				{
				  value = tree_cons (NULL_TREE, TREE_VALUE (values), value);
				  problem = 1;
				  break;
				}
			    }
			}
		      else
			{
			  context = (TREE_CODE (sub_values) == FUNCTION_DECL
				     && DECL_VIRTUAL_P (sub_values))
			    ? DECL_CLASS_CONTEXT (sub_values)
			      : DECL_CONTEXT (sub_values);

			  if (context && ! TYPE_DERIVES_FROM (context, type))
			    {
			      value = tree_cons (NULL_TREE, values, value);
			      problem = 1;
			      break;
			    }
			}
		    }
		  if (! problem) value = fields;
		}

	      /* Mark this as a potentially ambiguous member.  */
	      if (TREE_CODE (value) == TREE_LIST)
		{
		  /* Leaving TREE_TYPE blank is intentional.
		     We cannot use `error_mark_node' (lookup_name)
		     or `unknown_type_node' (all member functions use this).  */
		  TREE_NONLOCAL_FLAG (value) = 1;
		}

#if NEW_CLASS_SCOPING
	      /* Put the new contents in our envelope.  */
	      TREE_PURPOSE (class_value) = value;
	    }
	  else
	    {
	      /* See comment above for a description of envelopes.  */
	      tree envelope = tree_cons (fields, class_value,
					 closed_envelopes);

	      closed_envelopes = envelope;
	      IDENTIFIER_CLASS_VALUE (DECL_NAME (fields)) = envelope;
	    }
#else
	      IDENTIFIER_CLASS_VALUE (DECL_NAME (fields)) = value;
	    }
	  else IDENTIFIER_CLASS_VALUE (DECL_NAME (fields)) = fields;
#endif
	}
    }

  method_vec = CLASSTYPE_METHOD_VEC (type);
  if (method_vec != 0)
    {
      /* Farm out constructors and destructors.  */
      methods = &TREE_VEC_ELT (method_vec, 1);
      end = TREE_VEC_END (method_vec);

      /* This does not work for multiple inheritance yet.  */
      while (methods != end)
	{
	  /* This will cause lookup_name to return a pointer
	     to the tree_list of possible methods of this name.
	     If the order is a problem, we can nreverse them.  */
	  tree tmp;
#if NEW_CLASS_SCOPING
	  tree class_value = IDENTIFIER_CLASS_VALUE (DECL_NAME (*methods));

	  if (class_value && TREE_CODE (class_value) == TREE_LIST
	      && TREE_PURPOSE (class_value) != NULL_TREE
	      && TREE_CODE (TREE_PURPOSE (class_value)) != IDENTIFIER_NODE)
	    {
	      tree old = TREE_PURPOSE (class_value);

	      maybe_push_cache_obstack ();
	      if (TREE_CODE (old) == TREE_LIST)
		tmp = tree_cons (DECL_NAME (*methods), *methods, old);
	      else
		{
		  /* Only complain if we shadow something we can access.  */
		  if (old
		      && ((DECL_LANG_SPECIFIC (old)
			   && DECL_CLASS_CONTEXT (old) == current_class_type)
			  || ! TREE_PRIVATE (old)))
		    /* Should figure out visibility more accurately.  */
		    cp_warning ("shadowing member `%#D' with member function `%#D'",
				old, *methods);
		  tmp = build_tree_list (DECL_NAME (*methods), *methods);
		}
	      pop_obstacks ();

	      TREE_TYPE (tmp) = unknown_type_node;
#if 0
	      TREE_OVERLOADED (tmp) = DECL_OVERLOADED (*methods);
#endif
	      TREE_NONLOCAL_FLAG (tmp) = 1;
	      
	      /* Put the new contents in our envelope.  */
	      TREE_PURPOSE (class_value) = tmp;
	    }
	  else
	    {
	      maybe_push_cache_obstack ();
	      tmp = build_tree_list (DECL_NAME (*methods), *methods);
	      pop_obstacks ();

	      TREE_TYPE (tmp) = unknown_type_node;
#if 0
	      TREE_OVERLOADED (tmp) = DECL_OVERLOADED (*methods);
#endif
	      TREE_NONLOCAL_FLAG (tmp) = 1;
	      
	      /* See comment above for a description of envelopes.  */
	      closed_envelopes = tree_cons (tmp, class_value,
					    closed_envelopes);
	      IDENTIFIER_CLASS_VALUE (DECL_NAME (*methods)) = closed_envelopes;
	    }
#else
	  tree old = IDENTIFIER_CLASS_VALUE (DECL_NAME (*methods));

	  if (old && TREE_CODE (old) == TREE_LIST)
	    tmp = tree_cons (DECL_NAME (*methods), *methods, old);
	  else
	    {
	      /* Only complain if we shadow something we can access.  */
	      if (old && (DECL_CLASS_CONTEXT (old) == current_class_type
			  || ! TREE_PRIVATE (old)))
		/* Should figure out visibility more accurately.  */
		cp_warning_at ("member function `%D' shadows member `%s'", *methods,
				   IDENTIFIER_POINTER (DECL_NAME (old)));
	      tmp = build_tree_list (DECL_NAME (*methods), *methods);
	    }

	  TREE_TYPE (tmp) = unknown_type_node;
#if 0
	  TREE_OVERLOADED (tmp) = DECL_OVERLOADED (*methods);
#endif
	  TREE_NONLOCAL_FLAG (tmp) = 1;
	  IDENTIFIER_CLASS_VALUE (DECL_NAME (*methods)) = tmp;
#endif

	  tmp = *methods;
	  while (tmp != 0)
	    {
	      DECL_PUBLIC (tmp) = 0;
	      DECL_PROTECTED (tmp) = 0;
	      DECL_PRIVATE (tmp) = 0;
	      tmp = DECL_CHAIN (tmp);
	    }

	  methods++;
	}
    }
  SET_BINFO_MARKED (binfo);
}

/* Consolidate unique (by name) member functions.  */
static void
dfs_compress_decls (binfo)
     tree binfo;
{
  tree type = BINFO_TYPE (binfo);
  tree method_vec = CLASSTYPE_METHOD_VEC (type);

  if (method_vec != 0)
    {
      /* Farm out constructors and destructors.  */
      tree *methods = &TREE_VEC_ELT (method_vec, 1);
      tree *end = TREE_VEC_END (method_vec);

      for (; methods != end; methods++)
	{
#if NEW_CLASS_SCOPING
	  /* This is known to be an envelope of the kind described before
	     dfs_pushdecls.  */
	  tree class_value = IDENTIFIER_CLASS_VALUE (DECL_NAME (*methods));
	  tree tmp = TREE_PURPOSE (class_value);
#else
	  tree tmp = IDENTIFIER_CLASS_VALUE (DECL_NAME (*methods));
#endif

	  /* This was replaced in scope by somebody else.  Just leave it
	     alone.  */
	  if (TREE_CODE (tmp) != TREE_LIST)
	    continue;

	  if (TREE_CHAIN (tmp) == NULL_TREE
	      && TREE_VALUE (tmp)
	      && DECL_CHAIN (TREE_VALUE (tmp)) == NULL_TREE)
	    {
#if NEW_CLASS_SCOPING
	      TREE_PURPOSE (class_value) = TREE_VALUE (tmp);
#else
	      IDENTIFIER_CLASS_VALUE (DECL_NAME (*methods)) = TREE_VALUE (tmp);
#endif
	    }
	}
    }
  CLEAR_BINFO_MARKED (binfo);
}

/* When entering the scope of a class, we cache all of the
   fields that that class provides within its inheritance
   lattice.  Where ambiguities result, we mark them
   with `error_mark_node' so that if they are encountered
   without explicit qualification, we can emit an error
   message.  */
void
push_class_decls (type)
     tree type;
{
  tree id;
  struct obstack *ambient_obstack = current_obstack;

#if 0
  tree tags = CLASSTYPE_TAGS (type);

  while (tags)
    {
      tree code_type_node;
      tree tag;

      switch (TREE_CODE (TREE_VALUE (tags)))
	{
	case ENUMERAL_TYPE:
	  code_type_node = enum_type_node;
	  break;
	case RECORD_TYPE:
	  code_type_node = record_type_node;
	  break;
	case CLASS_TYPE:
	  code_type_node = class_type_node;
	  break;
	case UNION_TYPE:
	  code_type_node = union_type_node;
	  break;
	default:
	  my_friendly_abort (297);
	}
      tag = xref_tag (code_type_node, TREE_PURPOSE (tags),
		      TYPE_BINFO_BASETYPE (TREE_VALUE (tags), 0));
#if 0 /* not yet, should get fixed properly later */
      pushdecl (make_type_decl (TREE_PURPOSE (tags), TREE_VALUE (tags)));
#else
      pushdecl (build_decl (TYPE_DECL, TREE_PURPOSE (tags), TREE_VALUE (tags)));
#endif
    }
#endif

  search_stack = push_search_level (search_stack, &search_obstack);

  id = TYPE_IDENTIFIER (type);
  if (IDENTIFIER_TEMPLATE (id) != 0)
    {
#if 0
      tree tmpl = IDENTIFIER_TEMPLATE (id);
      push_template_decls (DECL_ARGUMENTS (TREE_PURPOSE (tmpl)),
			   TREE_VALUE (tmpl), 1);
#endif
#if NEW_CLASS_SCOPING
      overload_template_name (id, 1);
#else
      overload_template_name (id, 0);
#endif
    }

  /* Push class fields into CLASS_VALUE scope, and mark.  */
  dfs_walk (TYPE_BINFO (type), dfs_pushdecls, unmarkedp);

  /* Compress fields which have only a single entry
     by a given name, and unmark.  */
  dfs_walk (TYPE_BINFO (type), dfs_compress_decls, markedp);

#if NEW_CLASS_SCOPING
  /* Open up all the closed envelopes and push the contained decls into
     class scope.  */
  while (closed_envelopes)
    {
      tree new = TREE_PURPOSE (closed_envelopes);
      tree id;

      /* This is messy because the class value may be a *_DECL, or a
	 TREE_LIST of overloaded *_DECLs or even a TREE_LIST of ambiguous
	 *_DECLs.  The name is stored at different places in these three
	 cases.  */
      if (TREE_CODE (new) == TREE_LIST)
	{
	  if (TREE_PURPOSE (new) != NULL_TREE)
	    id = TREE_PURPOSE (new);
	  else
	    {
	      tree node = TREE_VALUE (new);

	      while (TREE_CODE (node) == TREE_LIST)
		node = TREE_VALUE (node);
	      id = DECL_NAME (node);
	    }
	}
      else
	id = DECL_NAME (new);

      /* Install the original class value in order to make
	 pushdecl_class_level work correctly.  */
      IDENTIFIER_CLASS_VALUE (id) = TREE_VALUE (closed_envelopes);
      if (TREE_CODE (new) == TREE_LIST)
	push_class_level_binding (id, new);
      else
	pushdecl_class_level (new);
      closed_envelopes = TREE_CHAIN (closed_envelopes);
    }
#endif
  current_obstack = ambient_obstack;
}

#if !NEW_CLASS_SCOPING
static void
dfs_popdecls (binfo)
     tree binfo;
{
  tree type = BINFO_TYPE (binfo);
  tree fields = TYPE_FIELDS (type);
  tree method_vec = CLASSTYPE_METHOD_VEC (type);

  while (fields)
    {
      if (DECL_NAME (fields) == NULL_TREE
	  && TREE_CODE (TREE_TYPE (fields)) == UNION_TYPE)
	{
	  dfs_popdecls (TYPE_BINFO (TREE_TYPE (fields)));
	}
      else if (DECL_NAME (fields))
	IDENTIFIER_CLASS_VALUE (DECL_NAME (fields)) = NULL_TREE;
      fields = TREE_CHAIN (fields);
    }
  if (method_vec != 0)
    {
      tree *methods = &TREE_VEC_ELT (method_vec, 0);
      tree *end = TREE_VEC_END (method_vec);

      /* Clear out ctors and dtors.  */
      if (*methods)
	IDENTIFIER_CLASS_VALUE (TYPE_IDENTIFIER (type)) = NULL_TREE;

      for (methods += 1; methods != end; methods++)
	IDENTIFIER_CLASS_VALUE (DECL_NAME (*methods)) = NULL_TREE;
    }

  SET_BINFO_MARKED (binfo);
}
#endif

void
pop_class_decls (type)
     tree type;
{
#if !NEW_CLASS_SCOPING
  tree binfo = TYPE_BINFO (type);

  /* Clear out the IDENTIFIER_CLASS_VALUE which this
     class may have occupied, and mark.  */
  dfs_walk (binfo, dfs_popdecls, unmarkedp);

  /* Unmark.  */
  dfs_walk (binfo, dfs_unmark, markedp);
#else
  /* We haven't pushed a search level when dealing with cached classes,
     so we'd better not try to pop it.  */
  if (search_stack)
#endif
    search_stack = pop_search_level (search_stack);
}

static int
bfs_unmark_finished_struct (binfo, i)
     tree binfo;
     int i;
{
  if (i >= 0)
    binfo = BINFO_BASETYPE (binfo, i);

  if (BINFO_NEW_VTABLE_MARKED (binfo))
    {
      tree decl, context;

      if (TREE_VIA_VIRTUAL (binfo))
	binfo = binfo_member (BINFO_TYPE (binfo),
			      CLASSTYPE_VBASECLASSES (current_class_type));

      decl = BINFO_VTABLE (binfo);
      context = DECL_CONTEXT (decl);
      DECL_CONTEXT (decl) = 0;
      if (write_virtuals >= 0
	  && DECL_INITIAL (decl) != BINFO_VIRTUALS (binfo))
	DECL_INITIAL (decl) = build_nt (CONSTRUCTOR, NULL_TREE,
					BINFO_VIRTUALS (binfo));
      finish_decl (decl, DECL_INITIAL (decl), NULL_TREE, 0);
      DECL_CONTEXT (decl) = context;
    }
  CLEAR_BINFO_VTABLE_PATH_MARKED (binfo);
  CLEAR_BINFO_NEW_VTABLE_MARKED (binfo);
  return 0;
}

void
unmark_finished_struct (type)
     tree type;
{
  tree binfo = TYPE_BINFO (type);
  bfs_unmark_finished_struct (binfo, -1);
  breadth_first_search (binfo, bfs_unmark_finished_struct, bfs_marked_vtable_pathp);
}

void
print_search_statistics ()
{
#ifdef GATHER_STATISTICS
  if (flag_memoize_lookups)
    {
      fprintf (stderr, "%d memoized contexts saved\n",
	       n_contexts_saved);
      fprintf (stderr, "%d local tree nodes made\n", my_tree_node_counter);
      fprintf (stderr, "%d local hash nodes made\n", my_memoized_entry_counter);
      fprintf (stderr, "fields statistics:\n");
      fprintf (stderr, "  memoized finds = %d; rejects = %d; (searches = %d)\n",
	       memoized_fast_finds[0], memoized_fast_rejects[0],
	       memoized_fields_searched[0]);
      fprintf (stderr, "  memoized_adds = %d\n", memoized_adds[0]);
      fprintf (stderr, "fnfields statistics:\n");
      fprintf (stderr, "  memoized finds = %d; rejects = %d; (searches = %d)\n",
	       memoized_fast_finds[1], memoized_fast_rejects[1],
	       memoized_fields_searched[1]);
      fprintf (stderr, "  memoized_adds = %d\n", memoized_adds[1]);
    }
  fprintf (stderr, "%d fields searched in %d[%d] calls to lookup_field[_1]\n",
	   n_fields_searched, n_calls_lookup_field, n_calls_lookup_field_1);
  fprintf (stderr, "%d fnfields searched in %d calls to lookup_fnfields\n",
	   n_outer_fields_searched, n_calls_lookup_fnfields);
  fprintf (stderr, "%d calls to get_base_type\n", n_calls_get_base_type);
#else
  fprintf (stderr, "no search statistics\n");
#endif
}

void
init_search_processing ()
{
  gcc_obstack_init (&search_obstack);
  gcc_obstack_init (&type_obstack);
  gcc_obstack_init (&type_obstack_entries);

  /* This gives us room to build our chains of basetypes,
     whether or not we decide to memoize them.  */
  type_stack = push_type_level (0, &type_obstack);
  _vptr_name = get_identifier ("_vptr");
}

void
reinit_search_statistics ()
{
  my_memoized_entry_counter = 0;
  memoized_fast_finds[0] = 0;
  memoized_fast_finds[1] = 0;
  memoized_adds[0] = 0;
  memoized_adds[1] = 0;
  memoized_fast_rejects[0] = 0;
  memoized_fast_rejects[1] = 0;
  memoized_fields_searched[0] = 0;
  memoized_fields_searched[1] = 0;
  n_fields_searched = 0;
  n_calls_lookup_field = 0, n_calls_lookup_field_1 = 0;
  n_calls_lookup_fnfields = 0, n_calls_lookup_fnfields_1 = 0;
  n_calls_get_base_type = 0;
  n_outer_fields_searched = 0;
  n_contexts_saved = 0;
}
