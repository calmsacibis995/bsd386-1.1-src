/* Read dbx symbol tables and convert to internal format, for GDB.
   Copyright 1986, 1987, 1988, 1989, 1990, 1991 Free Software Foundation, Inc.

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

/* This module provides three functions: dbx_symfile_init,
   which initializes to read a symbol file; dbx_new_init, which 
   discards existing cached information when all symbols are being
   discarded; and dbx_symfile_read, which reads a symbol table
   from a file.

   dbx_symfile_read only does the minimum work necessary for letting the
   user "name" things symbolically; it does not read the entire symtab.
   Instead, it reads the external and static symbols and puts them in partial
   symbol tables.  When more extensive information is requested of a
   file, the corresponding partial symbol table is mutated into a full
   fledged symbol table by going back and reading the symbols
   for real.  dbx_psymtab_to_symtab() is the function that does this */

#include "defs.h"
#include <string.h>

#ifdef USG
#include <sys/types.h>
#include <fcntl.h>
#define L_SET 0
#define L_INCR 1
#endif

#include <obstack.h>
#include <sys/param.h>
#ifndef	NO_SYS_FILE
#include <sys/file.h>
#endif
#include <sys/stat.h>
#include <ctype.h>
#include "symtab.h"
#include "breakpoint.h"
#include "command.h"
#include "target.h"
#include "gdbcore.h"		/* for bfd stuff */
#include "libbfd.h"		/* FIXME Secret internal BFD stuff (bfd_read) */
#include "libaout.h"	 	/* FIXME Secret internal BFD stuff for a.out */
#include "symfile.h"
#include "objfiles.h"
#include "buildsym.h"

#include "aout/aout64.h"
#include "aout/stab_gnu.h"	/* We always use GNU stabs, not native, now */

/* Information is passed among various dbxread routines for accessing
   symbol files.  A pointer to this structure is kept in the sym_private
   field of the objfile struct.  */
 
struct dbx_symfile_info {
  asection *text_sect;		/* Text section accessor */
  int symcount;			/* How many symbols are there in the file */
  char *stringtab;		/* The actual string table */
  int stringtab_size;		/* Its size */
  off_t symtab_offset;		/* Offset in file to symbol table */
};

#define DBX_SYMFILE_INFO(o)	((struct dbx_symfile_info *)((o)->sym_private))
#define DBX_TEXT_SECT(o)	(DBX_SYMFILE_INFO(o)->text_sect)
#define DBX_SYMCOUNT(o)		(DBX_SYMFILE_INFO(o)->symcount)
#define DBX_STRINGTAB(o)	(DBX_SYMFILE_INFO(o)->stringtab)
#define DBX_STRINGTAB_SIZE(o)	(DBX_SYMFILE_INFO(o)->stringtab_size)
#define DBX_SYMTAB_OFFSET(o)	(DBX_SYMFILE_INFO(o)->symtab_offset)

/* Each partial symbol table entry contains a pointer to private data for the
   read_symtab() function to use when expanding a partial symbol table entry
   to a full symbol table entry.

   For dbxread this structure contains the offset within the file symbol table
   of first local symbol for this file, and length (in bytes) of the section
   of the symbol table devoted to this file's symbols (actually, the section
   bracketed may contain more than just this file's symbols).  If ldsymlen is
   0, the only reason for this thing's existence is the dependency list.
   Nothing else will happen when it is read in. */

#define LDSYMOFF(p) (((struct symloc *)((p)->read_symtab_private))->ldsymoff)
#define LDSYMLEN(p) (((struct symloc *)((p)->read_symtab_private))->ldsymlen)

struct symloc {
  int ldsymoff;
  int ldsymlen;
};

/* Macro to determine which symbols to ignore when reading the first symbol
   of a file.  Some machines override this definition. */
#ifndef IGNORE_SYMBOL
/* This code is used on Ultrix systems.  Ignore it */
#define IGNORE_SYMBOL(type)  (type == (int)N_NSYMS)
#endif

/* Macro for name of symbol to indicate a file compiled with gcc. */
#ifndef GCC_COMPILED_FLAG_SYMBOL
#define GCC_COMPILED_FLAG_SYMBOL "gcc_compiled."
#endif

/* Macro for name of symbol to indicate a file compiled with gcc2. */
#ifndef GCC2_COMPILED_FLAG_SYMBOL
#define GCC2_COMPILED_FLAG_SYMBOL "gcc2_compiled."
#endif

/* Define this as 1 if a pcc declaration of a char or short argument
   gives the correct address.  Otherwise assume pcc gives the
   address of the corresponding int, which is not the same on a
   big-endian machine.  */

#ifndef BELIEVE_PCC_PROMOTION
#define BELIEVE_PCC_PROMOTION 0
#endif

/* Nonzero means give verbose info on gdb action.  From main.c.  */
extern int info_verbose;

/* The BFD for this file -- implicit parameter to next_symbol_text.  */

static bfd *symfile_bfd;

/* The size of each symbol in the symbol file (in external form).
   This is set by dbx_symfile_read when building psymtabs, and by
   dbx_psymtab_to_symtab when building symtabs.  */

static unsigned symbol_size;

/* Complaints about the symbols we have encountered.  */

struct complaint lbrac_complaint = 
  {"bad block start address patched", 0, 0};

struct complaint string_table_offset_complaint =
  {"bad string table offset in symbol %d", 0, 0};

struct complaint unknown_symtype_complaint =
  {"unknown symbol type %s", 0, 0};

struct complaint lbrac_rbrac_complaint =
  {"block start larger than block end", 0, 0};

struct complaint lbrac_unmatched_complaint =
  {"unmatched N_LBRAC before symtab pos %d", 0, 0};

struct complaint lbrac_mismatch_complaint =
  {"N_LBRAC/N_RBRAC symbol mismatch at symtab pos %d", 0, 0};

/* During initial symbol readin, we need to have a structure to keep
   track of which psymtabs have which bincls in them.  This structure
   is used during readin to setup the list of dependencies within each
   partial symbol table. */

struct header_file_location
{
  char *name;			/* Name of header file */
  int instance;			/* See above */
  struct partial_symtab *pst;	/* Partial symtab that has the
				   BINCL/EINCL defs for this file */
};

/* The actual list and controling variables */
static struct header_file_location *bincl_list, *next_bincl;
static int bincls_allocated;

/* Local function prototypes */

static void
free_header_files PARAMS ((void));

static void
init_header_files PARAMS ((void));

static struct pending *
copy_pending PARAMS ((struct pending *, int, struct pending *));

static struct symtab *
read_ofile_symtab PARAMS ((struct objfile *, int, int, CORE_ADDR, int, int));

static void
dbx_psymtab_to_symtab PARAMS ((struct partial_symtab *));

static void
psymtab_to_symtab_1 PARAMS ((struct partial_symtab *, int));

static void
read_dbx_symtab PARAMS ((CORE_ADDR, struct objfile *, CORE_ADDR, int));

static void
free_bincl_list PARAMS ((struct objfile *));

static struct partial_symtab *
find_corresponding_bincl_psymtab PARAMS ((char *, int));

static void
add_bincl_to_list PARAMS ((struct partial_symtab *, char *, int));

static void
init_bincl_list PARAMS ((int, struct objfile *));

static void
init_psymbol_list PARAMS ((struct objfile *));

static char *
dbx_next_symbol_text PARAMS ((void));

static void
fill_symbuf PARAMS ((bfd *));

static void
dbx_symfile_init PARAMS ((struct objfile *));

static void
dbx_new_init PARAMS ((struct objfile *));

static void
dbx_symfile_read PARAMS ((struct objfile *, CORE_ADDR, int));

static void
dbx_symfile_finish PARAMS ((struct objfile *));

static void
record_minimal_symbol PARAMS ((char *, CORE_ADDR, int, struct objfile *));

static void
add_new_header_file PARAMS ((char *, int));

static void
add_old_header_file PARAMS ((char *, int));

static void
add_this_object_header_file PARAMS ((int));

/* Free up old header file tables */

static void
free_header_files ()
{
  register int i;

  if (header_files != NULL)
    {
      for (i = 0; i < n_header_files; i++)
	{
	  free (header_files[i].name);
	}
      free ((PTR)header_files);
      header_files = NULL;
      n_header_files = 0;
    }
  if (this_object_header_files)
    {
      free ((PTR)this_object_header_files);
      this_object_header_files = NULL;
    }
  n_allocated_header_files = 0;
  n_allocated_this_object_header_files = 0;
}

/* Allocate new header file tables */

static void
init_header_files ()
{
  n_header_files = 0;
  n_allocated_header_files = 10;
  header_files = (struct header_file *)
    xmalloc (10 * sizeof (struct header_file));

  n_allocated_this_object_header_files = 10;
  this_object_header_files = (int *) xmalloc (10 * sizeof (int));
}

/* Add header file number I for this object file
   at the next successive FILENUM.  */

static void
add_this_object_header_file (i)
     int i;
{
  if (n_this_object_header_files == n_allocated_this_object_header_files)
    {
      n_allocated_this_object_header_files *= 2;
      this_object_header_files
	= (int *) xrealloc ((char *) this_object_header_files,
			    n_allocated_this_object_header_files * sizeof (int));
    }

  this_object_header_files[n_this_object_header_files++] = i;
}

/* Add to this file an "old" header file, one already seen in
   a previous object file.  NAME is the header file's name.
   INSTANCE is its instance code, to select among multiple
   symbol tables for the same header file.  */

static void
add_old_header_file (name, instance)
     char *name;
     int instance;
{
  register struct header_file *p = header_files;
  register int i;

  for (i = 0; i < n_header_files; i++)
    if (!strcmp (p[i].name, name) && instance == p[i].instance)
      {
	add_this_object_header_file (i);
	return;
      }
  error ("Invalid symbol data: \"repeated\" header file that hasn't been seen before, at symtab pos %d.",
	 symnum);
}

/* Add to this file a "new" header file: definitions for its types follow.
   NAME is the header file's name.
   Most often this happens only once for each distinct header file,
   but not necessarily.  If it happens more than once, INSTANCE has
   a different value each time, and references to the header file
   use INSTANCE values to select among them.

   dbx output contains "begin" and "end" markers for each new header file,
   but at this level we just need to know which files there have been;
   so we record the file when its "begin" is seen and ignore the "end".  */

static void
add_new_header_file (name, instance)
     char *name;
     int instance;
{
  register int i;
  header_file_prev_index = -1;

  /* Make sure there is room for one more header file.  */

  if (n_header_files == n_allocated_header_files)
    {
      n_allocated_header_files *= 2;
      header_files = (struct header_file *)
	xrealloc ((char *) header_files,
		  (n_allocated_header_files * sizeof (struct header_file)));
    }

  /* Create an entry for this header file.  */

  i = n_header_files++;
  header_files[i].name = savestring (name, strlen(name));
  header_files[i].instance = instance;
  header_files[i].length = 10;
  header_files[i].vector
    = (struct type **) xmalloc (10 * sizeof (struct type *));
  bzero (header_files[i].vector, 10 * sizeof (struct type *));

  add_this_object_header_file (i);
}

#if 0
static struct type **
explicit_lookup_type (real_filenum, index)
     int real_filenum, index;
{
  register struct header_file *f = &header_files[real_filenum];

  if (index >= f->length)
    {
      f->length *= 2;
      f->vector = (struct type **)
	xrealloc (f->vector, f->length * sizeof (struct type *));
      bzero (&f->vector[f->length / 2],
	     f->length * sizeof (struct type *) / 2);
    }
  return &f->vector[index];
}
#endif

static void
record_minimal_symbol (name, address, type, objfile)
     char *name;
     CORE_ADDR address;
     int type;
     struct objfile *objfile;
{
  enum minimal_symbol_type ms_type;

  switch (type &~ N_EXT) {
    case N_TEXT:  ms_type = mst_text; break;
    case N_DATA:  ms_type = mst_data; break;
    case N_BSS:   ms_type = mst_bss;  break;
    case N_ABS:   ms_type = mst_abs;  break;
#ifdef N_SETV
    case N_SETV:  ms_type = mst_data; break;
#endif
    default:      ms_type = mst_unknown; break;
  }

  prim_record_minimal_symbol (obsavestring (name, strlen (name), &objfile -> symbol_obstack),
			     address, ms_type);
}

/* Scan and build partial symbols for a symbol file.
   We have been initialized by a call to dbx_symfile_init, which 
   put all the relevant info into a "struct dbx_symfile_info",
   hung off the objfile structure.

   ADDR is the address relative to which the symbols in it are (e.g.
   the base address of the text segment).
   MAINLINE is true if we are reading the main symbol
   table (as opposed to a shared lib or dynamically loaded file).  */

static void
dbx_symfile_read (objfile, addr, mainline)
     struct objfile *objfile;
     CORE_ADDR addr;
     int mainline;	/* FIXME comments above */
{
  bfd *sym_bfd;
  int val;

  sym_bfd = objfile->obfd;
  val = bfd_seek (objfile->obfd, DBX_SYMTAB_OFFSET (objfile), L_SET);
  if (val < 0)
    perror_with_name (objfile->name);

  /* If we are reinitializing, or if we have never loaded syms yet, init */
  if (mainline || objfile->global_psymbols.size == 0 || objfile->static_psymbols.size == 0)
    init_psymbol_list (objfile);

  /* FIXME POKING INSIDE BFD DATA STRUCTURES */
  symbol_size = obj_symbol_entry_size (sym_bfd);

  pending_blocks = 0;
  make_cleanup (really_free_pendings, 0);

  init_minimal_symbol_collection ();
  make_cleanup (discard_minimal_symbols, 0);

  /* Now that the symbol table data of the executable file are all in core,
     process them and define symbols accordingly.  */

  addr -= bfd_section_vma (sym_bfd, DBX_TEXT_SECT (objfile)); /*offset*/
  read_dbx_symtab (addr, objfile,
		   bfd_section_vma  (sym_bfd, DBX_TEXT_SECT (objfile)),
		   bfd_section_size (sym_bfd, DBX_TEXT_SECT (objfile)));

  /* Install any minimal symbols that have been collected as the current
     minimal symbols for this objfile. */

  install_minimal_symbols (objfile);

  if (!have_partial_symbols ()) {
    wrap_here ("");
    printf_filtered ("(no debugging symbols found)...");
    wrap_here ("");
  }
}

/* Initialize anything that needs initializing when a completely new
   symbol file is specified (not just adding some symbols from another
   file, e.g. a shared library).  */

static void
dbx_new_init (ignore)
     struct objfile *ignore;
{
  buildsym_new_init ();
  init_header_files ();
}


/* dbx_symfile_init ()
   is the dbx-specific initialization routine for reading symbols.
   It is passed a struct objfile which contains, among other things,
   the BFD for the file whose symbols are being read, and a slot for a pointer
   to "private data" which we fill with goodies.

   We read the string table into malloc'd space and stash a pointer to it.

   Since BFD doesn't know how to read debug symbols in a format-independent
   way (and may never do so...), we have to do it ourselves.  We will never
   be called unless this is an a.out (or very similar) file. 
   FIXME, there should be a cleaner peephole into the BFD environment here.  */

static void
dbx_symfile_init (objfile)
     struct objfile *objfile;
{
  int val;
  bfd *sym_bfd = objfile->obfd;
  char *name = bfd_get_filename (sym_bfd);
  unsigned char size_temp[4];

  /* Allocate struct to keep track of the symfile */
  objfile->sym_private = (PTR)
    xmmalloc (objfile -> md, sizeof (struct dbx_symfile_info));

  /* FIXME POKING INSIDE BFD DATA STRUCTURES */
#define	STRING_TABLE_OFFSET	(sym_bfd->origin + obj_str_filepos (sym_bfd))
#define	SYMBOL_TABLE_OFFSET	(sym_bfd->origin + obj_sym_filepos (sym_bfd))
  /* FIXME POKING INSIDE BFD DATA STRUCTURES */

  DBX_TEXT_SECT (objfile) = bfd_get_section_by_name (sym_bfd, ".text");
  if (!DBX_TEXT_SECT (objfile))
    abort();
  DBX_SYMCOUNT (objfile) = bfd_get_symcount (sym_bfd);

  /* Read the string table and stash it away in the psymbol_obstack.  It is
     only needed as long as we need to expand psymbols into full symbols,
     so when we blow away the psymbol the string table goes away as well.
     Note that gdb used to use the results of attempting to malloc the
     string table, based on the size it read, as a form of sanity check
     for botched byte swapping, on the theory that a byte swapped string
     table size would be so totally bogus that the malloc would fail.  Now
     that we put in on the psymbol_obstack, we can't do this since gdb gets
     a fatal error (out of virtual memory) if the size is bogus.  We can
     however at least check to see if the size is zero or some negative
     value. */

  val = bfd_seek (sym_bfd, STRING_TABLE_OFFSET, L_SET);
  if (val < 0)
    perror_with_name (name);

  val = bfd_read ((PTR)size_temp, sizeof (long), 1, sym_bfd);
  if (val < 0)
    perror_with_name (name);

  DBX_STRINGTAB_SIZE (objfile) = bfd_h_get_32 (sym_bfd, size_temp);
  if (DBX_STRINGTAB_SIZE (objfile) <= 0)
    error ("ridiculous string table size (%d bytes).",
	   DBX_STRINGTAB_SIZE (objfile));

  DBX_STRINGTAB (objfile) =
    (char *) obstack_alloc (&objfile -> psymbol_obstack,
			    DBX_STRINGTAB_SIZE (objfile));

  /* Now read in the string table in one big gulp.  */

  val = bfd_seek (sym_bfd, STRING_TABLE_OFFSET, L_SET);
  if (val < 0)
    perror_with_name (name);
  val = bfd_read (DBX_STRINGTAB (objfile), DBX_STRINGTAB_SIZE (objfile), 1,
		  sym_bfd);
  if (val != DBX_STRINGTAB_SIZE (objfile))
    perror_with_name (name);

  /* Record the position of the symbol table for later use.  */

  DBX_SYMTAB_OFFSET (objfile) = SYMBOL_TABLE_OFFSET;
}

/* Perform any local cleanups required when we are done with a particular
   objfile.  I.E, we are in the process of discarding all symbol information
   for an objfile, freeing up all memory held for it, and unlinking the
   objfile struct from the global list of known objfiles. */

static void
dbx_symfile_finish (objfile)
     struct objfile *objfile;
{
  if (objfile->sym_private != NULL)
    {
      mfree (objfile -> md, objfile->sym_private);
    }
  free_header_files ();
}


/* Buffer for reading the symbol table entries.  */
static struct internal_nlist symbuf[4096];
static int symbuf_idx;
static int symbuf_end;

/* The address in memory of the string table of the object file we are
   reading (which might not be the "main" object file, but might be a
   shared library or some other dynamically loaded thing).  This is set
   by read_dbx_symtab when building psymtabs, and by read_ofile_symtab 
   when building symtabs, and is used only by next_symbol_text.  */
static char *stringtab_global;

/* Refill the symbol table input buffer
   and set the variables that control fetching entries from it.
   Reports an error if no data available.
   This function can read past the end of the symbol table
   (into the string table) but this does no harm.  */

static void
fill_symbuf (sym_bfd)
     bfd *sym_bfd;
{
  int nbytes = bfd_read ((PTR)symbuf, sizeof (symbuf), 1, sym_bfd);
  if (nbytes < 0)
    perror_with_name (bfd_get_filename (sym_bfd));
  else if (nbytes == 0)
    error ("Premature end of file reading symbol table");
  symbuf_end = nbytes / symbol_size;
  symbuf_idx = 0;
}

#define SWAP_SYMBOL(symp, abfd) \
  { \
    (symp)->n_strx = bfd_h_get_32(abfd,			\
				(unsigned char *)&(symp)->n_strx);	\
    (symp)->n_desc = bfd_h_get_16 (abfd,			\
				(unsigned char *)&(symp)->n_desc);  	\
    (symp)->n_value = bfd_h_get_32 (abfd,			\
				(unsigned char *)&(symp)->n_value); 	\
  }

/* Invariant: The symbol pointed to by symbuf_idx is the first one
   that hasn't been swapped.  Swap the symbol at the same time
   that symbuf_idx is incremented.  */

/* dbx allows the text of a symbol name to be continued into the
   next symbol name!  When such a continuation is encountered
   (a \ at the end of the text of a name)
   call this function to get the continuation.  */

static char *
dbx_next_symbol_text ()
{
  if (symbuf_idx == symbuf_end)
    fill_symbuf (symfile_bfd);
  symnum++;
  SWAP_SYMBOL(&symbuf[symbuf_idx], symfile_bfd);
  return symbuf[symbuf_idx++].n_strx + stringtab_global;
}

/* Initializes storage for all of the partial symbols that will be
   created by read_dbx_symtab and subsidiaries.  */

static void
init_psymbol_list (objfile)
     struct objfile *objfile;
{
  /* Free any previously allocated psymbol lists.  */
  if (objfile -> global_psymbols.list)
    mfree (objfile -> md, (PTR)objfile -> global_psymbols.list);
  if (objfile -> static_psymbols.list)
    mfree (objfile -> md, (PTR)objfile -> static_psymbols.list);

  /* Current best guess is that there are approximately a twentieth
     of the total symbols (in a debugging file) are global or static
     oriented symbols */
  objfile -> global_psymbols.size = DBX_SYMCOUNT (objfile) / 10;
  objfile -> static_psymbols.size = DBX_SYMCOUNT (objfile) / 10;
  objfile -> global_psymbols.next = objfile -> global_psymbols.list = (struct partial_symbol *)
    xmmalloc (objfile -> md, objfile -> global_psymbols.size * sizeof (struct partial_symbol));
  objfile -> static_psymbols.next = objfile -> static_psymbols.list = (struct partial_symbol *)
    xmmalloc (objfile -> md, objfile -> static_psymbols.size * sizeof (struct partial_symbol));
}

/* Initialize the list of bincls to contain none and have some
   allocated.  */

static void
init_bincl_list (number, objfile)
     int number;
     struct objfile *objfile;
{
  bincls_allocated = number;
  next_bincl = bincl_list = (struct header_file_location *)
    xmmalloc (objfile -> md, bincls_allocated * sizeof(struct header_file_location));
}

/* Add a bincl to the list.  */

static void
add_bincl_to_list (pst, name, instance)
     struct partial_symtab *pst;
     char *name;
     int instance;
{
  if (next_bincl >= bincl_list + bincls_allocated)
    {
      int offset = next_bincl - bincl_list;
      bincls_allocated *= 2;
      bincl_list = (struct header_file_location *)
	xmrealloc (pst->objfile->md, (char *)bincl_list,
		  bincls_allocated * sizeof (struct header_file_location));
      next_bincl = bincl_list + offset;
    }
  next_bincl->pst = pst;
  next_bincl->instance = instance;
  next_bincl++->name = name;
}

/* Given a name, value pair, find the corresponding
   bincl in the list.  Return the partial symtab associated
   with that header_file_location.  */

static struct partial_symtab *
find_corresponding_bincl_psymtab (name, instance)
     char *name;
     int instance;
{
  struct header_file_location *bincl;

  for (bincl = bincl_list; bincl < next_bincl; bincl++)
    if (bincl->instance == instance
	&& !strcmp (name, bincl->name))
      return bincl->pst;

  return (struct partial_symtab *) 0;
}

/* Free the storage allocated for the bincl list.  */

static void
free_bincl_list (objfile)
     struct objfile *objfile;
{
  mfree (objfile -> md, (PTR)bincl_list);
  bincls_allocated = 0;
}

/* Given pointers to an a.out symbol table in core containing dbx
   style data, setup partial_symtab's describing each source file for
   which debugging information is available.
   SYMFILE_NAME is the name of the file we are reading from
   and ADDR is its relocated address (if incremental) or 0 (if not).  */

static void
read_dbx_symtab (addr, objfile, text_addr, text_size)
     CORE_ADDR addr;
     struct objfile *objfile;
     CORE_ADDR text_addr;
     int text_size;
{
  register struct internal_nlist *bufp = 0;	/* =0 avoids gcc -Wall glitch */
  register char *namestring;
  int nsl;
  int past_first_source_file = 0;
  CORE_ADDR last_o_file_start = 0;
  struct cleanup *old_chain;
  bfd *abfd;

  /* End of the text segment of the executable file.  */
  CORE_ADDR end_of_text_addr;

  /* Current partial symtab */
  struct partial_symtab *pst;

  /* List of current psymtab's include files */
  char **psymtab_include_list;
  int includes_allocated;
  int includes_used;

  /* Index within current psymtab dependency list */
  struct partial_symtab **dependency_list;
  int dependencies_used, dependencies_allocated;

  stringtab_global = DBX_STRINGTAB (objfile);
  
  pst = (struct partial_symtab *) 0;

  includes_allocated = 30;
  includes_used = 0;
  psymtab_include_list = (char **) alloca (includes_allocated *
					   sizeof (char *));

  dependencies_allocated = 30;
  dependencies_used = 0;
  dependency_list =
    (struct partial_symtab **) alloca (dependencies_allocated *
				       sizeof (struct partial_symtab *));

  old_chain = make_cleanup (free_objfile, objfile);

  /* Init bincl list */
  init_bincl_list (20, objfile);
  make_cleanup (free_bincl_list, objfile);

  last_source_file = 0;

#ifdef END_OF_TEXT_DEFAULT
  end_of_text_addr = END_OF_TEXT_DEFAULT;
#else
  end_of_text_addr = text_addr + addr + text_size;	/* Relocate */
#endif

  symfile_bfd = objfile->obfd;	/* For next_text_symbol */
  abfd = objfile->obfd;
  symbuf_end = symbuf_idx = 0;
  next_symbol_text_func = dbx_next_symbol_text;

  for (symnum = 0; symnum < DBX_SYMCOUNT (objfile); symnum++)
    {
      /* Get the symbol for this run and pull out some info */
      QUIT;	/* allow this to be interruptable */
      if (symbuf_idx == symbuf_end)
	fill_symbuf (abfd);
      bufp = &symbuf[symbuf_idx++];

      /*
       * Special case to speed up readin.
       */
      if (bufp->n_type == (unsigned char)N_SLINE) continue;

      SWAP_SYMBOL (bufp, abfd);

      /* Ok.  There is a lot of code duplicated in the rest of this
         switch statement (for efficiency reasons).  Since I don't
         like duplicating code, I will do my penance here, and
         describe the code which is duplicated:

	 *) The assignment to namestring.
	 *) The call to strchr.
	 *) The addition of a partial symbol the the two partial
	    symbol lists.  This last is a large section of code, so
	    I've imbedded it in the following macro.
	 */
      
/* Set namestring based on bufp.  If the string table index is invalid, 
   give a fake name, and print a single error message per symbol file read,
   rather than abort the symbol reading or flood the user with messages.  */
#define SET_NAMESTRING()\
  if (((unsigned)bufp->n_strx) >= DBX_STRINGTAB_SIZE (objfile)) {	\
    complain (&string_table_offset_complaint, (char *) symnum);		\
    namestring = "foo";							\
  } else								\
    namestring = bufp->n_strx + DBX_STRINGTAB (objfile)

#define CUR_SYMBOL_TYPE bufp->n_type
#define CUR_SYMBOL_VALUE bufp->n_value
#define DBXREAD_ONLY
#define START_PSYMTAB(ofile,addr,fname,low,symoff,global_syms,static_syms)\
  start_psymtab(ofile, addr, fname, low, symoff, global_syms, static_syms)
#define END_PSYMTAB(pst,ilist,ninc,c_off,c_text,dep_list,n_deps)\
  end_psymtab(pst,ilist,ninc,c_off,c_text,dep_list,n_deps)

#include "partial-stab.h"
    }

  /* If there's stuff to be cleaned up, clean it up.  */
  if (DBX_SYMCOUNT (objfile) > 0			/* We have some syms */
      && objfile -> ei.entry_point < bufp->n_value
      && objfile -> ei.entry_point >= last_o_file_start)
    {
      objfile -> ei.entry_file_lowpc = last_o_file_start;
      objfile -> ei.entry_file_highpc = bufp->n_value;
    }

  if (pst)
    {
      end_psymtab (pst, psymtab_include_list, includes_used,
		   symnum * symbol_size, end_of_text_addr,
		   dependency_list, dependencies_used);
    }

  free_bincl_list (objfile);
  discard_cleanups (old_chain);
}

/* Allocate and partially fill a partial symtab.  It will be
   completely filled at the end of the symbol list.

   SYMFILE_NAME is the name of the symbol-file we are reading from, and ADDR
   is the address relative to which its symbols are (incremental) or 0
   (normal). */


struct partial_symtab *
start_psymtab (objfile, addr,
	       filename, textlow, ldsymoff, global_syms, static_syms)
     struct objfile *objfile;
     CORE_ADDR addr;
     char *filename;
     CORE_ADDR textlow;
     int ldsymoff;
     struct partial_symbol *global_syms;
     struct partial_symbol *static_syms;
{
  struct partial_symtab *result =
      start_psymtab_common(objfile, addr,
			   filename, textlow, global_syms, static_syms);

  result->read_symtab_private = (char *)
    obstack_alloc (&objfile -> psymbol_obstack, sizeof (struct symloc));
  LDSYMOFF(result) = ldsymoff;
  result->read_symtab = dbx_psymtab_to_symtab;

  return result;
}

/* Close off the current usage of a partial_symbol table entry.  This
   involves setting the correct number of includes (with a realloc),
   setting the high text mark, setting the symbol length in the
   executable, and setting the length of the global and static lists
   of psymbols.

   The global symbols and static symbols are then seperately sorted.

   Then the partial symtab is put on the global list.
   *** List variables and peculiarities of same. ***
   */

void
end_psymtab (pst, include_list, num_includes, capping_symbol_offset,
	     capping_text, dependency_list, number_dependencies)
     struct partial_symtab *pst;
     char **include_list;
     int num_includes;
     int capping_symbol_offset;
     CORE_ADDR capping_text;
     struct partial_symtab **dependency_list;
     int number_dependencies;
/*     struct partial_symbol *capping_global, *capping_static;*/
{
  int i;
  struct objfile *objfile = pst -> objfile;

  if (capping_symbol_offset != -1)
      LDSYMLEN(pst) = capping_symbol_offset - LDSYMOFF(pst);
  pst->texthigh = capping_text;

  pst->n_global_syms =
    objfile->global_psymbols.next - (objfile->global_psymbols.list + pst->globals_offset);
  pst->n_static_syms =
    objfile->static_psymbols.next - (objfile->static_psymbols.list + pst->statics_offset);

  pst->number_of_dependencies = number_dependencies;
  if (number_dependencies)
    {
      pst->dependencies = (struct partial_symtab **)
	obstack_alloc (&objfile->psymbol_obstack,
		       number_dependencies * sizeof (struct partial_symtab *));
      memcpy (pst->dependencies, dependency_list,
	     number_dependencies * sizeof (struct partial_symtab *));
    }
  else
    pst->dependencies = 0;

  for (i = 0; i < num_includes; i++)
    {
      struct partial_symtab *subpst =
	allocate_psymtab (include_list[i], objfile);

      subpst->addr = pst->addr;
      subpst->read_symtab_private =
	  (char *) obstack_alloc (&objfile->psymbol_obstack,
				  sizeof (struct symloc));
      LDSYMOFF(subpst) =
	LDSYMLEN(subpst) =
	  subpst->textlow =
	    subpst->texthigh = 0;

      /* We could save slight bits of space by only making one of these,
	 shared by the entire set of include files.  FIXME-someday.  */
      subpst->dependencies = (struct partial_symtab **)
	obstack_alloc (&objfile->psymbol_obstack,
		       sizeof (struct partial_symtab *));
      subpst->dependencies[0] = pst;
      subpst->number_of_dependencies = 1;

      subpst->globals_offset =
	subpst->n_global_syms =
	  subpst->statics_offset =
	    subpst->n_static_syms = 0;

      subpst->readin = 0;
      subpst->symtab = 0;
      subpst->read_symtab = dbx_psymtab_to_symtab;
    }

  sort_pst_symbols (pst);

  /* If there is already a psymtab or symtab for a file of this name, remove it.
     (If there is a symtab, more drastic things also happen.)
     This happens in VxWorks.  */
  free_named_symtabs (pst->filename);

  if (num_includes == 0
   && number_dependencies == 0
   && pst->n_global_syms == 0
   && pst->n_static_syms == 0) {
    /* Throw away this psymtab, it's empty.  We can't deallocate it, since
       it is on the obstack, but we can forget to chain it on the list.  */
    struct partial_symtab *prev_pst;

    /* First, snip it out of the psymtab chain */

    if (pst->objfile->psymtabs == pst)
      pst->objfile->psymtabs = pst->next;
    else
      for (prev_pst = pst->objfile->psymtabs; prev_pst; prev_pst = pst->next)
	if (prev_pst->next == pst)
	  prev_pst->next = pst->next;

    /* Next, put it on a free list for recycling */

    pst->next = pst->objfile->free_psymtabs;
    pst->objfile->free_psymtabs = pst;
  }
}

static void
psymtab_to_symtab_1 (pst, sym_offset)
     struct partial_symtab *pst;
     int sym_offset;
{
  struct cleanup *old_chain;
  int i;
  
  if (!pst)
    return;

  if (pst->readin)
    {
      fprintf (stderr, "Psymtab for %s already read in.  Shouldn't happen.\n",
	       pst->filename);
      return;
    }

  /* Read in all partial symtabs on which this one is dependent */
  for (i = 0; i < pst->number_of_dependencies; i++)
    if (!pst->dependencies[i]->readin)
      {
	/* Inform about additional files that need to be read in.  */
	if (info_verbose)
	  {
	    fputs_filtered (" ", stdout);
	    wrap_here ("");
	    fputs_filtered ("and ", stdout);
	    wrap_here ("");
	    printf_filtered ("%s...", pst->dependencies[i]->filename);
	    wrap_here ("");		/* Flush output */
	    fflush (stdout);
	  }
	psymtab_to_symtab_1 (pst->dependencies[i], sym_offset);
      }

  if (LDSYMLEN(pst))		/* Otherwise it's a dummy */
    {
      /* Init stuff necessary for reading in symbols */
      buildsym_init ();
      old_chain = make_cleanup (really_free_pendings, 0);

      /* Read in this files symbols */
      bfd_seek (pst->objfile->obfd, sym_offset, L_SET);
      pst->symtab =
	read_ofile_symtab (pst->objfile, LDSYMOFF(pst), LDSYMLEN(pst),
			   pst->textlow, pst->texthigh - pst->textlow,
			   pst->addr);
      sort_symtab_syms (pst->symtab);

      do_cleanups (old_chain);
    }

  pst->readin = 1;
}

/* Read in all of the symbols for a given psymtab for real.
   Be verbose about it if the user wants that.  */

static void
dbx_psymtab_to_symtab (pst)
     struct partial_symtab *pst;
{
  bfd *sym_bfd;

  if (!pst)
    return;

  if (pst->readin)
    {
      fprintf (stderr, "Psymtab for %s already read in.  Shouldn't happen.\n",
	       pst->filename);
      return;
    }

  if (LDSYMLEN(pst) || pst->number_of_dependencies)
    {
      /* Print the message now, before reading the string table,
	 to avoid disconcerting pauses.  */
      if (info_verbose)
	{
	  printf_filtered ("Reading in symbols for %s...", pst->filename);
	  fflush (stdout);
	}

      sym_bfd = pst->objfile->obfd;

      /* FIXME POKING INSIDE BFD DATA STRUCTURES */
      symbol_size = obj_symbol_entry_size (sym_bfd);

      next_symbol_text_func = dbx_next_symbol_text;

      /* FIXME, this uses internal BFD variables.  See above in
	 dbx_symbol_file_open where the macro is defined!  */
      psymtab_to_symtab_1 (pst, SYMBOL_TABLE_OFFSET);

      /* Match with global symbols.  This only needs to be done once,
         after all of the symtabs and dependencies have been read in.   */
      scan_file_globals (pst->objfile);

      /* Finish up the debug error message.  */
      if (info_verbose)
	printf_filtered ("done.\n");
    }
}

/*
 * Read in a defined section of a specific object file's symbols.
 *
 * DESC is the file descriptor for the file, positioned at the
 * beginning of the symtab
 * SYM_OFFSET is the offset within the file of
 * the beginning of the symbols we want to read
 * SYM_SIZE is the size of the symbol info to read in.
 * TEXT_OFFSET is the beginning of the text segment we are reading symbols for
 * TEXT_SIZE is the size of the text segment read in.
 * OFFSET is a relocation offset which gets added to each symbol
 */

static struct symtab *
read_ofile_symtab (objfile, sym_offset, sym_size, text_offset, text_size,
		   offset)
     struct objfile *objfile;
     int sym_offset;
     int sym_size;
     CORE_ADDR text_offset;
     int text_size;
     int offset;
{
  register char *namestring;
  register struct internal_nlist *bufp;
  unsigned char type;
  unsigned max_symnum;
  register bfd *abfd;

  current_objfile = objfile;
  subfile_stack = 0;

  stringtab_global = DBX_STRINGTAB (objfile);
  last_source_file = 0;

  abfd = objfile->obfd;
  symfile_bfd = objfile->obfd;	/* Implicit param to next_text_symbol */
  symbuf_end = symbuf_idx = 0;

  /* It is necessary to actually read one symbol *before* the start
     of this symtab's symbols, because the GCC_COMPILED_FLAG_SYMBOL
     occurs before the N_SO symbol.

     Detecting this in read_dbx_symtab
     would slow down initial readin, so we look for it here instead.  */
  if (sym_offset >= (int)symbol_size)
    {
      bfd_seek (symfile_bfd, sym_offset - symbol_size, L_INCR);
      fill_symbuf (abfd);
      bufp = &symbuf[symbuf_idx++];
      SWAP_SYMBOL (bufp, abfd);

      SET_NAMESTRING ();

      processing_gcc_compilation =
	(bufp->n_type == N_TEXT
	 && (strcmp (namestring, GCC_COMPILED_FLAG_SYMBOL) == 0
	     || strcmp(namestring, GCC2_COMPILED_FLAG_SYMBOL) == 0));
    }
  else
    {
      /* The N_SO starting this symtab is the first symbol, so we
	 better not check the symbol before it.  I'm not this can
	 happen, but it doesn't hurt to check for it.  */
      bfd_seek (symfile_bfd, sym_offset, L_INCR);
      processing_gcc_compilation = 0;
    }

  if (symbuf_idx == symbuf_end)
    fill_symbuf (abfd);
  bufp = &symbuf[symbuf_idx];
  if (bufp->n_type != (unsigned char)N_SO)
    error("First symbol in segment of executable not a source symbol");

  max_symnum = sym_size / symbol_size;

  for (symnum = 0;
       symnum < max_symnum;
       symnum++)
    {
      QUIT;			/* Allow this to be interruptable */
      if (symbuf_idx == symbuf_end)
	fill_symbuf(abfd);
      bufp = &symbuf[symbuf_idx++];
      SWAP_SYMBOL (bufp, abfd);

      type = bufp->n_type;
      if (type == (unsigned char)N_CATCH)
	{
	  /* N_CATCH is not fixed up by the linker, and unfortunately,
	     there's no other place to put it in the .stab map.  */
	  bufp->n_value += text_offset - offset;
	}

      SET_NAMESTRING ();

      if (type & N_STAB) {
	  process_one_symbol (type, bufp->n_desc, bufp->n_value,
			      namestring, offset, objfile);
      }
      /* We skip checking for a new .o or -l file; that should never
         happen in this routine. */
      else if (type == N_TEXT
	       && (strcmp (namestring, GCC_COMPILED_FLAG_SYMBOL) == 0
		   || strcmp (namestring, GCC2_COMPILED_FLAG_SYMBOL) == 0))
	/* I don't think this code will ever be executed, because
	   the GCC_COMPILED_FLAG_SYMBOL usually is right before
	   the N_SO symbol which starts this source file.
	   However, there is no reason not to accept
	   the GCC_COMPILED_FLAG_SYMBOL anywhere.  */
	processing_gcc_compilation = 1;
      else if (type & N_EXT || type == (unsigned char)N_TEXT
	       || type == (unsigned char)N_NBTEXT
	       ) {
	  /* Global symbol: see if we came across a dbx defintion for
	     a corresponding symbol.  If so, store the value.  Remove
	     syms from the chain when their values are stored, but
	     search the whole chain, as there may be several syms from
	     different files with the same name. */
	  /* This is probably not true.  Since the files will be read
	     in one at a time, each reference to a global symbol will
	     be satisfied in each file as it appears. So we skip this
	     section. */
	  ;
        }
    }

  current_objfile = NULL;
  return (end_symtab (text_offset + text_size, 0, 0, objfile));
}

/* This handles a single symbol from the symbol-file, building symbols
   into a GDB symtab.  It takes these arguments and an implicit argument.

   TYPE is the type field of the ".stab" symbol entry.
   DESC is the desc field of the ".stab" entry.
   VALU is the value field of the ".stab" entry.
   NAME is the symbol name, in our address space.
   OFFSET is the amount by which this object file was relocated 
	  when it was loaded into memory.  All symbols that refer
	  to memory locations need to be offset by this amount.
   OBJFILE is the object file from which we are reading symbols.
 	       It is used in end_symtab.  */

void
process_one_symbol (type, desc, valu, name, offset, objfile)
     int type, desc;
     CORE_ADDR valu;
     char *name;
     int offset;
     struct objfile *objfile;
{
#ifndef SUN_FIXED_LBRAC_BUG
  /* This records the last pc address we've seen.  We depend on there being
     an SLINE or FUN or SO before the first LBRAC, since the variable does
     not get reset in between reads of different symbol files.  */
  static CORE_ADDR last_pc_address;
#endif
  register struct context_stack *new;
  char *colon_pos;

  /* Something is wrong if we see real data before
     seeing a source file name.  */

  if (last_source_file == 0 && type != (unsigned char)N_SO)
    {
      /* Currently this ignores N_ENTRY on Gould machines, N_NSYM on machines
	 where that code is defined.  */
      if (IGNORE_SYMBOL (type))
	return;

      /* FIXME, this should not be an error, since it precludes extending
         the symbol table information in this way...  */
      error ("Invalid symbol data: does not start by identifying a source file.");
    }

  switch (type)
    {
    case N_FUN:
    case N_FNAME:
#if 0
/* It seems that the Sun ANSI C compiler (acc) replaces N_FUN with N_GSYM and
   N_STSYM with a type code of f or F.  Can't enable this until we get some
   stuff straightened out with psymtabs. */

    case N_GSYM:
    case N_STSYM:
#endif /* 0 */

      valu += offset;		/* Relocate for dynamic loading */

      /* Either of these types of symbols indicates the start of
	 a new function.  We must process its "name" normally for dbx,
	 but also record the start of a new lexical context, and possibly
	 also the end of the lexical context for the previous function.  */
      /* This is not always true.  This type of symbol may indicate a
         text segment variable.  */

      colon_pos = strchr (name, ':');
      if (!colon_pos++
	  || (*colon_pos != 'f' && *colon_pos != 'F'))
	{
	  define_symbol (valu, name, desc, type, objfile);
	  break;
	}

#ifndef SUN_FIXED_LBRAC_BUG
      last_pc_address = valu;	/* Save for SunOS bug circumcision */
#endif

      within_function = 1;
      if (context_stack_depth > 0)
	{
	  new = pop_context ();
	  /* Make a block for the local symbols within.  */
	  finish_block (new->name, &local_symbols, new->old_blocks,
			new->start_addr, valu, objfile);
	}
      /* Stack must be empty now.  */
      if (context_stack_depth != 0)
	complain (&lbrac_unmatched_complaint, (char *) symnum);

      new = push_context (0, valu);
      new->name = define_symbol (valu, name, desc, type, objfile);
      break;

    case N_CATCH:
      /* Record the address at which this catch takes place.  */
      define_symbol (valu+offset, name, desc, type, objfile);
      break;

    case N_LBRAC:
      /* This "symbol" just indicates the start of an inner lexical
	 context within a function.  */

#if defined (BLOCK_ADDRESS_ABSOLUTE)
      valu += offset;		/* Relocate for dynamic loading */
#else
      /* On most machines, the block addresses are relative to the
	 N_SO, the linker did not relocate them (sigh).  */
      valu += last_source_start_addr;
#endif

#ifndef SUN_FIXED_LBRAC_BUG
      if (valu < last_pc_address) {
	/* Patch current LBRAC pc value to match last handy pc value */
 	complain (&lbrac_complaint, 0);
	valu = last_pc_address;
      }
#endif
      new = push_context (desc, valu);
      break;

    case N_RBRAC:
      /* This "symbol" just indicates the end of an inner lexical
	 context that was started with N_LBRAC.  */

#if defined (BLOCK_ADDRESS_ABSOLUTE)
      valu += offset;		/* Relocate for dynamic loading */
#else
      /* On most machines, the block addresses are relative to the
	 N_SO, the linker did not relocate them (sigh).  */
      valu += last_source_start_addr;
#endif

      new = pop_context();
      if (desc != new->depth)
	complain (&lbrac_mismatch_complaint, (char *) symnum);

      /* Some compilers put the variable decls inside of an
         LBRAC/RBRAC block.  This macro should be nonzero if this
	 is true.  DESC is N_DESC from the N_RBRAC symbol.
	 GCC_P is true if we've detected the GCC_COMPILED_SYMBOL
	 or the GCC2_COMPILED_SYMBOL.  */
#if !defined (VARIABLES_INSIDE_BLOCK)
#define VARIABLES_INSIDE_BLOCK(desc, gcc_p) 0
#endif

      /* Can only use new->locals as local symbols here if we're in
         gcc or on a machine that puts them before the lbrack.  */
      if (!VARIABLES_INSIDE_BLOCK(desc, processing_gcc_compilation))
	local_symbols = new->locals;

      /* If this is not the outermost LBRAC...RBRAC pair in the
	 function, its local symbols preceded it, and are the ones
	 just recovered from the context stack.  Defined the block for them.

	 If this is the outermost LBRAC...RBRAC pair, there is no
	 need to do anything; leave the symbols that preceded it
	 to be attached to the function's own block.  However, if
	 it is so, we need to indicate that we just moved outside
	 of the function.  */
      if (local_symbols
	  && (context_stack_depth
	      > !VARIABLES_INSIDE_BLOCK(desc, processing_gcc_compilation)))
	{
	  /* FIXME Muzzle a compiler bug that makes end < start.  */
	  if (new->start_addr > valu)
	    {
	      complain(&lbrac_rbrac_complaint, 0);
	      new->start_addr = valu;
	    }
	  /* Make a block for the local symbols within.  */
	  finish_block (0, &local_symbols, new->old_blocks,
			new->start_addr, valu, objfile);
	}
      else
	{
	  within_function = 0;
	}
      if (VARIABLES_INSIDE_BLOCK(desc, processing_gcc_compilation))
	/* Now pop locals of block just finished.  */
	local_symbols = new->locals;
      break;

    case N_FN:
    case N_FN_SEQ:
      /* This kind of symbol indicates the start of an object file.  */
      valu += offset;		/* Relocate for dynamic loading */
      break;

    case N_SO:
      /* This type of symbol indicates the start of data
	 for one source file.
	 Finish the symbol table of the previous source file
	 (if any) and start accumulating a new symbol table.  */
      valu += offset;		/* Relocate for dynamic loading */

#ifndef SUN_FIXED_LBRAC_BUG
      last_pc_address = valu;	/* Save for SunOS bug circumcision */
#endif
  
#ifdef PCC_SOL_BROKEN
      /* pcc bug, occasionally puts out SO for SOL.  */
      if (context_stack_depth > 0)
	{
	  start_subfile (name, NULL);
	  break;
	}
#endif
      if (last_source_file)
	{
	  /* Check if previous symbol was also an N_SO (with some
	     sanity checks).  If so, that one was actually the directory
	     name, and the current one is the real file name.
	     Patch things up. */	   
	  if (previous_stab_code == N_SO
	      && current_subfile && current_subfile->dirname == NULL
	      && current_subfile->name != NULL
	      && current_subfile->name[strlen(current_subfile->name)-1] == '/')
	    {
	      current_subfile->dirname = current_subfile->name;
	      current_subfile->name =
		  obsavestring (name, strlen (name),
				&objfile -> symbol_obstack);
	      break;
	    }
	  (void) end_symtab (valu, 0, 0, objfile);
	}
      start_symtab (name, NULL, valu);
      break;


    case N_SOL:
      /* This type of symbol indicates the start of data for
	 a sub-source-file, one whose contents were copied or
	 included in the compilation of the main source file
	 (whose name was given in the N_SO symbol.)  */
      valu += offset;		/* Relocate for dynamic loading */
      start_subfile (name, NULL);
      break;

    case N_BINCL:
      push_subfile ();
      add_new_header_file (name, valu);
      start_subfile (name, NULL);
      break;

    case N_EINCL:
      start_subfile (pop_subfile (), NULL);
      break;

    case N_EXCL:
      add_old_header_file (name, valu);
      break;

    case N_SLINE:
      /* This type of "symbol" really just records
	 one line-number -- core-address correspondence.
	 Enter it in the line list for this symbol table.  */
      valu += offset;		/* Relocate for dynamic loading */
#ifndef SUN_FIXED_LBRAC_BUG
      last_pc_address = valu;	/* Save for SunOS bug circumcision */
#endif
      record_line (current_subfile, desc, valu);
      break;

    case N_BCOMM:
      if (common_block)
	error ("Invalid symbol data: common within common at symtab pos %d",
	       symnum);
      common_block = local_symbols;
      common_block_i = local_symbols ? local_symbols->nsyms : 0;
      break;

    case N_ECOMM:
      /* Symbols declared since the BCOMM are to have the common block
	 start address added in when we know it.  common_block points to
	 the first symbol after the BCOMM in the local_symbols list;
	 copy the list and hang it off the symbol for the common block name
	 for later fixup.  */
      {
	int i;
	struct symbol *sym =
	  (struct symbol *) xmmalloc (objfile -> md, sizeof (struct symbol));
	bzero (sym, sizeof *sym);
	SYMBOL_NAME (sym) = savestring (name, strlen (name));
	SYMBOL_CLASS (sym) = LOC_BLOCK;
	SYMBOL_NAMESPACE (sym) = (enum namespace)((long)
	  copy_pending (local_symbols, common_block_i, common_block));
	i = hashname (SYMBOL_NAME (sym));
	SYMBOL_VALUE_CHAIN (sym) = global_sym_chain[i];
	global_sym_chain[i] = sym;
	common_block = 0;
	break;
      }

    /* The following symbol types need to have the offset added to their
       value; then we process symbol definitions in the name.  */
    case N_STSYM:		/* Global symbol */
    case N_LCSYM:		/* Local symbol */
    case N_DSLINE:		/* Source line number, data seg */
    case N_BSLINE:		/* Source line number, bss seg */
    /*   N_BROWS:	overlaps with N_BSLINE */
    case N_ENTRY:		/* Alternate entry point */
      valu += offset;		/* Relocate for dynamic loading */
      /* FALL THROUGH */

    /* The following symbol types don't need the address field relocated,
       since it is either unused, or is absolute.  */
    case N_GSYM:		/* Global variable */
    case N_NSYMS:		/* Number of symbols (ultrix) */
    case N_NOMAP:		/* No map?  (ultrix) */
    case N_DEFD:		/* Modula-2 GNU module dependency */
    case N_SSYM:		/* Struct or union element */
    case N_LSYM:		/* Local symbol in stack */
    case N_PSYM:		/* Parameter variable */
    case N_LENG:		/* Length of preceding symbol type */
      if (name)
	define_symbol (valu, name, desc, type, objfile);
      break;

    case N_RSYM:		/* Register variable */
	/* XXX - bugs in gcc 2.1: the bogus name 'in$chrg' is output
	 * for no clear reason with totally bogus values.  We ignore
	 * it.  Also, when inline functions are invoked, an entry for
	 * their "this" pointer is output.  Unfortunately, it is usually
	 * output as type 'P' instead of 'r' so gdb thinks there are
	 * multiple 'this' arguments to the same function.  We change
	 * the type of all but the first 'P' to 'r' (reg var).  (Note
	 * that we (ab)use the fact that 'this' is the first parameter
	 * and, thus, the first local symbol.)
	 * CHECK THIS WHEN WE SWITCH TO GCC 2.2.x! */
	if (name) {
		if (strncmp(name, "__in$chrg:", 10) == 0)
			break;
		if (local_symbols && strncmp(name, "$t:P", 4) == 0)
			name[3] = 'r';

		define_symbol (valu, name, desc, type, objfile);
	}
	break;

    /* The following symbol types we don't know how to process.  Handle
       them in a "default" way, but complain to people who care.  */
    default:
    case N_EHDECL:		/* Exception handler name */
    case N_MAIN:		/* Name of main routine (not used in C) */
    case N_PC:			/* Global symbol in Pascal */
    case N_M2C:			/* Modula-2 compilation unit */
    /*   N_MOD2:	overlaps with N_EHDECL */
    case N_SCOPE:		/* Modula-2 scope information */
    case N_ECOML:		/* End common (local name) */
    case N_NBTEXT:		/* Gould Non-Base-Register symbols??? */
    case N_NBDATA:
    case N_NBBSS:
    case N_NBSTS:
    case N_NBLCS:
      complain (&unknown_symtype_complaint, local_hex_string(type));
      if (name)
	define_symbol (valu, name, desc, type, objfile);
    }

  previous_stab_code = type;
}

/* Copy a pending list, used to record the contents of a common
   block for later fixup.  */
static struct pending *
copy_pending (beg, begi, end)
    struct pending *beg;
    int begi;
    struct pending *end;
{
  struct pending *new = 0;
  struct pending *next;

  for (next = beg; next != 0 && (next != end || begi < end->nsyms);
       next = next->next, begi = 0)
    {
      register int j;
      for (j = begi; j < next->nsyms; j++)
	add_symbol_to_list (next->symbol[j], &new);
    }
  return new;
}

/* Register our willingness to decode symbols for SunOS and a.out and
   b.out files handled by BFD... */
static struct sym_fns sunos_sym_fns =
{
  "sunOs",		/* sym_name: name or name prefix of BFD target type */
  6,			/* sym_namelen: number of significant sym_name chars */
  dbx_new_init,		/* sym_new_init: init anything gbl to entire symtab */
  dbx_symfile_init,	/* sym_init: read initial info, setup for sym_read() */
  dbx_symfile_read,	/* sym_read: read a symbol file into symtab */
  dbx_symfile_finish,	/* sym_finish: finished with file, cleanup */
  NULL			/* next: pointer to next struct sym_fns */
};

static struct sym_fns aout_sym_fns =
{
  "a.out",		/* sym_name: name or name prefix of BFD target type */
  5,			/* sym_namelen: number of significant sym_name chars */
  dbx_new_init,		/* sym_new_init: init anything gbl to entire symtab */
  dbx_symfile_init,	/* sym_init: read initial info, setup for sym_read() */
  dbx_symfile_read,	/* sym_read: read a symbol file into symtab */
  dbx_symfile_finish,	/* sym_finish: finished with file, cleanup */
  NULL			/* next: pointer to next struct sym_fns */
};

static struct sym_fns bout_sym_fns =
{
  "b.out",		/* sym_name: name or name prefix of BFD target type */
  5,			/* sym_namelen: number of significant sym_name chars */
  dbx_new_init,		/* sym_new_init: init anything gbl to entire symtab */
  dbx_symfile_init,	/* sym_init: read initial info, setup for sym_read() */
  dbx_symfile_read,	/* sym_read: read a symbol file into symtab */
  dbx_symfile_finish,	/* sym_finish: finished with file, cleanup */
  NULL			/* next: pointer to next struct sym_fns */
};

void
_initialize_dbxread ()
{
  add_symtab_fns(&sunos_sym_fns);
  add_symtab_fns(&aout_sym_fns);
  add_symtab_fns(&bout_sym_fns);
}
