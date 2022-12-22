/* Target-dependent code for the MIPS architecture, for GDB, the GNU Debugger.
   Copyright 1988, 1989, 1990, 1991, 1992 Free Software Foundation, Inc.
   Contributed by Alessandro Forin(af@cs.cmu.edu) at CMU
   and by Per Bothner(bothner@cs.wisc.edu) at U.Wisconsin.

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

#include "defs.h"
#include "frame.h"
#include "inferior.h"
#include "symtab.h"
#include "value.h"
#include "gdbcmd.h"
#include "language.h"

#ifdef USG
#include <sys/types.h>
#endif

#include <sys/param.h>
#include <sys/dir.h>
#include <signal.h>
#include <sys/ioctl.h>

#ifdef sgi
/* Must do it this way only for SGIs, as other mips platforms get their
   JB_ symbols from machine/pcb.h (included via sys/user.h). */
#include <setjmp.h>
#endif

#include "gdbcore.h"
#include "symfile.h"
#include "objfiles.h"

#ifndef	MIPSMAGIC
#ifdef MIPSEL
#define MIPSMAGIC	MIPSELMAGIC
#else
#define MIPSMAGIC	MIPSEBMAGIC
#endif
#endif

#define VM_MIN_ADDRESS (unsigned)0x400000

#include <sys/user.h>		/* After a.out.h  */
#include <sys/file.h>
#include <sys/stat.h>


#define PROC_LOW_ADDR(proc) ((proc)->pdr.adr) /* least address */
#define PROC_HIGH_ADDR(proc) ((proc)->pdr.iline) /* upper address bound */
#define PROC_FRAME_OFFSET(proc) ((proc)->pdr.frameoffset)
#define PROC_FRAME_REG(proc) ((proc)->pdr.framereg)
#define PROC_REG_MASK(proc) ((proc)->pdr.regmask)
#define PROC_FREG_MASK(proc) ((proc)->pdr.fregmask)
#define PROC_REG_OFFSET(proc) ((proc)->pdr.regoffset)
#define PROC_FREG_OFFSET(proc) ((proc)->pdr.fregoffset)
#define PROC_PC_REG(proc) ((proc)->pdr.pcreg)
#define PROC_SYMBOL(proc) (*(struct symbol**)&(proc)->pdr.isym)
#define _PROC_MAGIC_ 0x0F0F0F0F
#define PROC_DESC_IS_DUMMY(proc) ((proc)->pdr.isym == _PROC_MAGIC_)
#define SET_PROC_DESC_IS_DUMMY(proc) ((proc)->pdr.isym = _PROC_MAGIC_)

struct linked_proc_info
{
  struct mips_extra_func_info info;
  struct linked_proc_info *next;
} * linked_proc_desc_table = NULL;


#define READ_FRAME_REG(fi, regno) read_next_frame_reg((fi)->next, regno)

static int
read_next_frame_reg(fi, regno)
     FRAME fi;
     int regno;
{
#define SIGFRAME_BASE   sizeof(struct sigcontext)
#define SIGFRAME_PC_OFF (-SIGFRAME_BASE+ 2*sizeof(int))
#define SIGFRAME_SP_OFF (-SIGFRAME_BASE+32*sizeof(int))
#define SIGFRAME_RA_OFF (-SIGFRAME_BASE+34*sizeof(int))
  for (; fi; fi = fi->next)
      if (in_sigtramp(fi->pc, 0)) {
	  /* No idea if this code works. --PB. */
	  int offset;
	  if (regno == PC_REGNUM) offset = SIGFRAME_PC_OFF;
	  else if (regno == RA_REGNUM) offset = SIGFRAME_RA_OFF;
	  else if (regno == SP_REGNUM) offset = SIGFRAME_SP_OFF;
	  else return 0;
	  return read_memory_integer(fi->frame + offset, 4);
      }
      else if (regno == SP_REGNUM) return fi->frame;
      else if (fi->saved_regs->regs[regno])
	return read_memory_integer(fi->saved_regs->regs[regno], 4);
  return read_register(regno);
}

int
mips_frame_saved_pc(frame)
     FRAME frame;
{
  mips_extra_func_info_t proc_desc = frame->proc_desc;
  int pcreg = proc_desc ? PROC_PC_REG(proc_desc) : RA_REGNUM;

  if (proc_desc && PROC_DESC_IS_DUMMY(proc_desc))
      return read_memory_integer(frame->frame - 4, 4);

  return read_next_frame_reg(frame, pcreg);
}

static struct mips_extra_func_info temp_proc_desc;
static struct frame_saved_regs temp_saved_regs;

static CORE_ADDR
heuristic_proc_start(pc)
    CORE_ADDR pc;
{

    CORE_ADDR start_pc = pc;
    CORE_ADDR fence = start_pc - 200;
    if (fence < VM_MIN_ADDRESS) fence = VM_MIN_ADDRESS;
    /* search back for previous return */
    for (start_pc -= 4; ; start_pc -= 4)
	if (start_pc < fence) return 0; 
	else if (ABOUT_TO_RETURN(start_pc))
	    break;

    start_pc += 8; /* skip return, and its delay slot */
#if 0
    /* skip nops (usually 1) 0 - is this */
    while (start_pc < pc && read_memory_integer (start_pc, 4) == 0)
	start_pc += 4;
#endif
    return start_pc;
}

static mips_extra_func_info_t
heuristic_proc_desc(start_pc, limit_pc, next_frame)
    CORE_ADDR start_pc, limit_pc;
    FRAME next_frame;
{
    CORE_ADDR sp = next_frame ? next_frame->frame : read_register (SP_REGNUM);
    CORE_ADDR cur_pc;
    int frame_size;
    int has_frame_reg = 0;
    int reg30; /* Value of $r30. Used by gcc for frame-pointer */
    unsigned long reg_mask = 0;

    if (start_pc == 0) return NULL;
    bzero(&temp_proc_desc, sizeof(temp_proc_desc));
    bzero(&temp_saved_regs, sizeof(struct frame_saved_regs));
    if (start_pc + 200 < limit_pc) limit_pc = start_pc + 200;
  restart:
    frame_size = 0;
    for (cur_pc = start_pc; cur_pc < limit_pc; cur_pc += 4) {
	unsigned long word;
	int status;

	status = read_memory_nobpt (cur_pc, &word, 4); 
	if (status) memory_error (status, cur_pc); 
	SWAP_TARGET_AND_HOST (&word, sizeof (word));
	if ((word & 0xFFFF0000) == 0x27bd0000) /* addiu $sp,$sp,-i */
	    frame_size += (-word) & 0xFFFF;
	else if ((word & 0xFFFF0000) == 0x23bd0000) /* addu $sp,$sp,-i */
	    frame_size += (-word) & 0xFFFF;
	else if ((word & 0xFFE00000) == 0xafa00000) { /* sw reg,offset($sp) */
	    int reg = (word & 0x001F0000) >> 16;
	    reg_mask |= 1 << reg;
	    temp_saved_regs.regs[reg] = sp + (short)word;
	}
	else if ((word & 0xFFFF0000) == 0x27be0000) { /* addiu $30,$sp,size */
	    if ((unsigned short)word != frame_size)
		reg30 = sp + (unsigned short)word;
	    else if (!has_frame_reg) {
		int alloca_adjust;
		has_frame_reg = 1;
		reg30 = read_next_frame_reg(next_frame, 30);
		alloca_adjust = reg30 - (sp + (unsigned short)word);
		if (alloca_adjust > 0) {
		    /* FP > SP + frame_size. This may be because
		    /* of an alloca or somethings similar.
		     * Fix sp to "pre-alloca" value, and try again.
		     */
		    sp += alloca_adjust;
		    goto restart;
		}
	    }
	}
	else if ((word & 0xFFE00000) == 0xafc00000) { /* sw reg,offset($30) */
	    int reg = (word & 0x001F0000) >> 16;
	    reg_mask |= 1 << reg;
	    temp_saved_regs.regs[reg] = reg30 + (short)word;
	}
    }
    if (has_frame_reg) {
	PROC_FRAME_REG(&temp_proc_desc) = 30;
	PROC_FRAME_OFFSET(&temp_proc_desc) = 0;
    }
    else {
	PROC_FRAME_REG(&temp_proc_desc) = SP_REGNUM;
	PROC_FRAME_OFFSET(&temp_proc_desc) = frame_size;
    }
    PROC_REG_MASK(&temp_proc_desc) = reg_mask;
    PROC_PC_REG(&temp_proc_desc) = RA_REGNUM;
    return &temp_proc_desc;
}

static mips_extra_func_info_t
find_proc_desc(pc, next_frame)
    CORE_ADDR pc;
    FRAME next_frame;
{
  mips_extra_func_info_t proc_desc;
  struct block *b = block_for_pc(pc);
  struct symbol *sym =
      b ? lookup_symbol(".gdbinfo.", b, LABEL_NAMESPACE, 0, NULL) : NULL;

  if (sym)
    {
	/* IF this is the topmost frame AND
	 * (this proc does not have debugging information OR
	 * the PC is in the procedure prologue)
	 * THEN create a "heuristic" proc_desc (by analyzing
	 * the actual code) to replace the "official" proc_desc.
	 */
	proc_desc = (mips_extra_func_info_t)SYMBOL_VALUE(sym);
	if (next_frame == NULL) {
	    struct symtab_and_line val;
	    struct symbol *proc_symbol =
		PROC_DESC_IS_DUMMY(proc_desc) ? 0 : PROC_SYMBOL(proc_desc);

	    if (proc_symbol) {
		val = find_pc_line (BLOCK_START
				    (SYMBOL_BLOCK_VALUE(proc_symbol)),
				    0);
		val.pc = val.end ? val.end : pc;
	    }
	    if (!proc_symbol || pc < val.pc) {
		mips_extra_func_info_t found_heuristic =
		    heuristic_proc_desc(PROC_LOW_ADDR(proc_desc),
					pc, next_frame);
		if (found_heuristic) proc_desc = found_heuristic;
	    }
	}
    }
  else
    {
      /* Is linked_proc_desc_table really necessary?  It only seems to be used
	 by procedure call dummys.  However, the procedures being called ought
	 to have their own proc_descs, and even if they don't,
	 heuristic_proc_desc knows how to create them! */

      register struct linked_proc_info *link;
      for (link = linked_proc_desc_table; link; link = link->next)
	  if (PROC_LOW_ADDR(&link->info) <= pc
	      && PROC_HIGH_ADDR(&link->info) > pc)
	      return &link->info;
      proc_desc =
	  heuristic_proc_desc(heuristic_proc_start(pc), pc, next_frame);
    }
  return proc_desc;
}

mips_extra_func_info_t cached_proc_desc;

FRAME_ADDR
mips_frame_chain(frame)
    FRAME frame;
{
    mips_extra_func_info_t proc_desc;
    CORE_ADDR saved_pc = FRAME_SAVED_PC(frame);

    if (saved_pc == 0 || inside_entry_file (saved_pc))
      return 0;

    proc_desc = find_proc_desc(saved_pc, frame);
    if (!proc_desc)
      return 0;

    cached_proc_desc = proc_desc;
    return read_next_frame_reg(frame, PROC_FRAME_REG(proc_desc))
      + PROC_FRAME_OFFSET(proc_desc);
}

void
init_extra_frame_info(fci)
     struct frame_info *fci;
{
  extern struct obstack frame_cache_obstack;
  /* Use proc_desc calculated in frame_chain */
  mips_extra_func_info_t proc_desc = fci->next ? cached_proc_desc :
      find_proc_desc(fci->pc, fci->next);

  fci->saved_regs = (struct frame_saved_regs*)
    obstack_alloc (&frame_cache_obstack, sizeof(struct frame_saved_regs));
  bzero(fci->saved_regs, sizeof(struct frame_saved_regs));
  fci->proc_desc =
      proc_desc == &temp_proc_desc ? 0 : proc_desc;
  if (proc_desc)
    {
      int ireg;
      CORE_ADDR reg_position;
      unsigned long mask;
      /* r0 bit means kernel trap */
      int kernel_trap = PROC_REG_MASK(proc_desc) & 1;

      /* Fixup frame-pointer - only needed for top frame */
      /* This may not be quite right, if procedure has a real frame register */
      if (fci->pc == PROC_LOW_ADDR(proc_desc))
	  fci->frame = read_register (SP_REGNUM);
      else
	  fci->frame = READ_FRAME_REG(fci, PROC_FRAME_REG(proc_desc))
	      + PROC_FRAME_OFFSET(proc_desc);

      if (proc_desc == &temp_proc_desc)
	  *fci->saved_regs = temp_saved_regs;
      else
      {
	  /* find which general-purpose registers were saved */
	  reg_position = fci->frame + PROC_REG_OFFSET(proc_desc);
	  mask = kernel_trap ? 0xFFFFFFFF : PROC_REG_MASK(proc_desc);
	  for (ireg= 31; mask; --ireg, mask <<= 1)
	      if (mask & 0x80000000)
	      {
		  fci->saved_regs->regs[ireg] = reg_position;
		  reg_position -= 4;
	      }
	  /* find which floating-point registers were saved */
	  reg_position = fci->frame + PROC_FREG_OFFSET(proc_desc);
	  /* The freg_offset points to where the first *double* register is saved.
	   * So skip to the high-order word. */
	  reg_position += 4;
	  mask = kernel_trap ? 0xFFFFFFFF : PROC_FREG_MASK(proc_desc);
	  for (ireg = 31; mask; --ireg, mask <<= 1)
	      if (mask & 0x80000000)
	      {
		  fci->saved_regs->regs[FP0_REGNUM+ireg] = reg_position;
		  reg_position -= 4;
	      }
      }

      /* hack: if argument regs are saved, guess these contain args */
      if ((PROC_REG_MASK(proc_desc) & 0xF0) == 0) fci->num_args = -1;
      else if ((PROC_REG_MASK(proc_desc) & 0x80) == 0) fci->num_args = 4;
      else if ((PROC_REG_MASK(proc_desc) & 0x40) == 0) fci->num_args = 3;
      else if ((PROC_REG_MASK(proc_desc) & 0x20) == 0) fci->num_args = 2;
      else if ((PROC_REG_MASK(proc_desc) & 0x10) == 0) fci->num_args = 1;

      fci->saved_regs->regs[PC_REGNUM] = fci->saved_regs->regs[RA_REGNUM];
    }
}


CORE_ADDR
mips_push_arguments(nargs, args, sp, struct_return, struct_addr)
  int nargs;
  value *args;
  CORE_ADDR sp;
  int struct_return;
  CORE_ADDR struct_addr;
{
  CORE_ADDR buf;
  register i;
  int accumulate_size = struct_return ? 4 : 0;
  struct mips_arg { char *contents; int len; int offset; };
  struct mips_arg *mips_args =
      (struct mips_arg*)alloca(nargs * sizeof(struct mips_arg));
  register struct mips_arg *m_arg;
  for (i = 0, m_arg = mips_args; i < nargs; i++, m_arg++) {
    extern value value_arg_coerce();
    value arg = value_arg_coerce (args[i]);
    m_arg->len = TYPE_LENGTH (VALUE_TYPE (arg));
    /* This entire mips-specific routine is because doubles must be aligned
     * on 8-byte boundaries. It still isn't quite right, because MIPS decided
     * to align 'struct {int a, b}' on 4-byte boundaries (even though this
     * breaks their varargs implementation...). A correct solution
     * requires an simulation of gcc's 'alignof' (and use of 'alignof'
     * in stdarg.h/varargs.h).
     */
    if (m_arg->len > 4) accumulate_size = (accumulate_size + 7) & -8;
    m_arg->offset = accumulate_size;
    accumulate_size = (accumulate_size + m_arg->len + 3) & -4;
    m_arg->contents = VALUE_CONTENTS(arg);
  }
  accumulate_size = (accumulate_size + 7) & (-8);
  if (accumulate_size < 16) accumulate_size = 16; 
  sp -= accumulate_size;
  for (i = nargs; m_arg--, --i >= 0; )
    write_memory(sp + m_arg->offset, m_arg->contents, m_arg->len);
  if (struct_return) {
    buf = struct_addr;
    write_memory(sp, &buf, sizeof(CORE_ADDR));
}
  return sp;
}

/* MASK(i,j) == (1<<i) + (1<<(i+1)) + ... + (1<<j)). Assume i<=j<31. */
#define MASK(i,j) ((1 << (j)+1)-1 ^ (1 << (i))-1)

void
mips_push_dummy_frame()
{
  int ireg;
  struct linked_proc_info *link = (struct linked_proc_info*)
      xmalloc(sizeof(struct linked_proc_info));
  mips_extra_func_info_t proc_desc = &link->info;
  CORE_ADDR sp = read_register (SP_REGNUM);
  CORE_ADDR save_address;
  REGISTER_TYPE buffer;
  link->next = linked_proc_desc_table;
  linked_proc_desc_table = link;
#define PUSH_FP_REGNUM 16 /* must be a register preserved across calls */
#define GEN_REG_SAVE_MASK MASK(1,16)|MASK(24,28)|(1<<31)
#define GEN_REG_SAVE_COUNT 22
#define FLOAT_REG_SAVE_MASK MASK(0,19)
#define FLOAT_REG_SAVE_COUNT 20
#define SPECIAL_REG_SAVE_COUNT 4
  /*
   * The registers we must save are all those not preserved across
   * procedure calls. Dest_Reg (see tm-mips.h) must also be saved.
   * In addition, we must save the PC, and PUSH_FP_REGNUM.
   * (Ideally, we should also save MDLO/-HI and FP Control/Status reg.)
   *
   * Dummy frame layout:
   *  (high memory)
   * 	Saved PC
   *	Saved MMHI, MMLO, FPC_CSR
   *	Saved R31
   *	Saved R28
   *	...
   *	Saved R1
   *    Saved D18 (i.e. F19, F18)
   *    ...
   *    Saved D0 (i.e. F1, F0)
   *	CALL_DUMMY (subroutine stub; see m-mips.h)
   *	Parameter build area (not yet implemented)
   *  (low memory)
   */
  PROC_REG_MASK(proc_desc) = GEN_REG_SAVE_MASK;
  PROC_FREG_MASK(proc_desc) = FLOAT_REG_SAVE_MASK;
  PROC_REG_OFFSET(proc_desc) = /* offset of (Saved R31) from FP */
      -sizeof(long) - 4 * SPECIAL_REG_SAVE_COUNT;
  PROC_FREG_OFFSET(proc_desc) = /* offset of (Saved D18) from FP */
      -sizeof(double) - 4 * (SPECIAL_REG_SAVE_COUNT + GEN_REG_SAVE_COUNT);
  /* save general registers */
  save_address = sp + PROC_REG_OFFSET(proc_desc);
  for (ireg = 32; --ireg >= 0; )
    if (PROC_REG_MASK(proc_desc) & (1 << ireg))
      {
	buffer = read_register (ireg);
	write_memory (save_address, &buffer, sizeof(REGISTER_TYPE));
	save_address -= 4;
      }
  /* save floating-points registers */
  save_address = sp + PROC_FREG_OFFSET(proc_desc);
  for (ireg = 32; --ireg >= 0; )
    if (PROC_FREG_MASK(proc_desc) & (1 << ireg))
      {
	buffer = read_register (ireg + FP0_REGNUM);
	write_memory (save_address, &buffer, 4);
	save_address -= 4;
      }
  write_register (PUSH_FP_REGNUM, sp);
  PROC_FRAME_REG(proc_desc) = PUSH_FP_REGNUM;
  PROC_FRAME_OFFSET(proc_desc) = 0;
  buffer = read_register (PC_REGNUM);
  write_memory (sp - 4, &buffer, sizeof(REGISTER_TYPE));
  buffer = read_register (HI_REGNUM);
  write_memory (sp - 8, &buffer, sizeof(REGISTER_TYPE));
  buffer = read_register (LO_REGNUM);
  write_memory (sp - 12, &buffer, sizeof(REGISTER_TYPE));
  buffer = read_register (FCRCS_REGNUM);
  write_memory (sp - 16, &buffer, sizeof(REGISTER_TYPE));
  sp -= 4 * (GEN_REG_SAVE_COUNT+FLOAT_REG_SAVE_COUNT+SPECIAL_REG_SAVE_COUNT);
  write_register (SP_REGNUM, sp);
  PROC_LOW_ADDR(proc_desc) = sp - CALL_DUMMY_SIZE + CALL_DUMMY_START_OFFSET;
  PROC_HIGH_ADDR(proc_desc) = sp;
  SET_PROC_DESC_IS_DUMMY(proc_desc);
  PROC_PC_REG(proc_desc) = RA_REGNUM;
}

void
mips_pop_frame()
{ register int regnum;
  FRAME frame = get_current_frame ();
  CORE_ADDR new_sp = frame->frame;
  mips_extra_func_info_t proc_desc = (mips_extra_func_info_t)frame->proc_desc;
  if (PROC_DESC_IS_DUMMY(proc_desc))
    {
      struct linked_proc_info **ptr = &linked_proc_desc_table;;
      for (; &ptr[0]->info != proc_desc; ptr = &ptr[0]->next )
	  if (ptr[0] == NULL) abort();
      *ptr = ptr[0]->next;
      free (ptr[0]);
      write_register (HI_REGNUM, read_memory_integer(new_sp - 8, 4));
      write_register (LO_REGNUM, read_memory_integer(new_sp - 12, 4));
      write_register (FCRCS_REGNUM, read_memory_integer(new_sp - 16, 4));
    }
  write_register (PC_REGNUM, FRAME_SAVED_PC(frame));
  if (frame->proc_desc) {
    for (regnum = 32; --regnum >= 0; )
      if (PROC_REG_MASK(proc_desc) & (1 << regnum))
	write_register (regnum,
		  read_memory_integer (frame->saved_regs->regs[regnum], 4));
    for (regnum = 32; --regnum >= 0; )
      if (PROC_FREG_MASK(proc_desc) & (1 << regnum))
	write_register (regnum + FP0_REGNUM,
		  read_memory_integer (frame->saved_regs->regs[regnum + FP0_REGNUM], 4));
  }
  write_register (SP_REGNUM, new_sp);
  flush_cached_frames ();
  set_current_frame (create_new_frame (new_sp, read_pc ()));
}

static void
mips_print_register(regnum, all)
     int regnum, all;
{
      unsigned char raw_buffer[MAX_REGISTER_RAW_SIZE];
      REGISTER_TYPE val;

      /* Get the data in raw format.  */
      if (read_relative_register_raw_bytes (regnum, raw_buffer))
	{
	  printf_filtered ("%s: [Invalid]", reg_names[regnum]);
	  return;
	}
      
      /* If an even floating pointer register, also print as double. */
      if (regnum >= FP0_REGNUM && regnum < FP0_REGNUM+32
	  && !((regnum-FP0_REGNUM) & 1)) {
	  read_relative_register_raw_bytes (regnum+1, raw_buffer+4);
	  printf_filtered ("(d%d: ", regnum-FP0_REGNUM);
	  val_print (builtin_type_double, raw_buffer, 0,
		     stdout, 0, 1, 0, Val_pretty_default);
	  printf_filtered ("); ");
      }
      fputs_filtered (reg_names[regnum], stdout);
#ifndef NUMERIC_REG_NAMES
      if (regnum < 32)
	  printf_filtered ("(r%d): ", regnum);
      else
#endif
	  printf_filtered (": ");

      /* If virtual format is floating, print it that way.  */
      if (TYPE_CODE (REGISTER_VIRTUAL_TYPE (regnum)) == TYPE_CODE_FLT
	  && ! INVALID_FLOAT (raw_buffer, REGISTER_VIRTUAL_SIZE(regnum))) {
	  val_print (REGISTER_VIRTUAL_TYPE (regnum), raw_buffer, 0,
		     stdout, 0, 1, 0, Val_pretty_default);
      }
      /* Else print as integer in hex.  */
      else
	{
	  long val;

	  bcopy (raw_buffer, &val, sizeof (long));
	  SWAP_TARGET_AND_HOST ((char *)&val, sizeof (long));
	  if (val == 0)
	    printf_filtered ("0");
	  else if (all)
	    printf_filtered (local_hex_format(), val);
	  else
	    printf_filtered ("%s=%d", local_hex_string(val), val);
	}
}

/* Replacement for generic do_registers_info.  */
void
mips_do_registers_info (regnum, fpregs)
     int regnum;
     int fpregs;
{
  if (regnum != -1) {
      mips_print_register (regnum, 0);
      printf_filtered ("\n");
  }
  else {
      for (regnum = 0; regnum < NUM_REGS; ) {
	  if ((!fpregs) && regnum >= FP0_REGNUM && regnum <= FCRIR_REGNUM) {
	    regnum++;
	    continue;
	  }
	  mips_print_register (regnum, 1);
	  regnum++;
	  if ((regnum & 3) == 0 || regnum == NUM_REGS)
	      printf_filtered (";\n");
	  else
	      printf_filtered ("; ");
      }
  }
}
/* Return number of args passed to a frame. described by FIP.
   Can return -1, meaning no way to tell.  */

int
mips_frame_num_args(fip)
	FRAME fip;
{
#if 0
	struct chain_info_t *p;

	p = mips_find_cached_frame(FRAME_FP(fip));
	if (p->valid)
		return p->the_info.numargs;
#endif
	return -1;
}


/* Bad floats: Returns 0 if P points to a valid IEEE floating point number,
   1 if P points to a denormalized number or a NaN. LEN says whether this is
   a single-precision or double-precision float */
#define SINGLE_EXP_BITS  8
#define DOUBLE_EXP_BITS 11
int
isa_NAN(p, len)
     int *p, len;
{
  int exponent;
  if (len == 4)
    {
      exponent = *p;
      exponent = exponent << 1 >> (32 - SINGLE_EXP_BITS - 1);
      return ((exponent == -1) || (! exponent && *p));
    }
  else if (len == 8)
    {
      exponent = *(p+1);
      exponent = exponent << 1 >> (32 - DOUBLE_EXP_BITS - 1);
      return ((exponent == -1) || (! exponent && *p * *(p+1)));
    }
  else return 1;
}

/*
 * Implemented for Irix 4.x by Garrett A. Wollman
 */
#ifdef USE_PROC_FS		/* Target-dependent /proc support */

#include <sys/time.h>
#include <sys/procfs.h>

typedef unsigned int greg_t;	/* why isn't this defined? */

/*
 * See the comment in m68k-tdep.c regarding the utility of these functions.
 */

void 
supply_gregset (gregsetp)
     gregset_t *gregsetp;
{
  register int regno;
  register greg_t *regp = (greg_t *)(gregsetp->gp_regs);

  /* FIXME: somewhere, there should be a #define for the meaning
     of this magic number 32; we should use that. */
  for(regno = 0; regno < 32; regno++)
    supply_register (regno, (char *)(regp + regno));

  supply_register (PC_REGNUM, (char *)&(gregsetp->gp_pc));
  supply_register (HI_REGNUM, (char *)&(gregsetp->gp_mdhi));
  supply_register (LO_REGNUM, (char *)&(gregsetp->gp_mdlo));
  supply_register (PS_REGNUM, (char *)&(gregsetp->gp_cause));
}

void
fill_gregset (gregsetp, regno)
     gregset_t *gregsetp;
     int regno;
{
  int regi;
  register greg_t *regp = (greg_t *)(gregsetp->gp_regs);
  extern char registers[];

  /* same FIXME as above wrt 32*/
  for (regi = 0; regi < 32; regi++)
    if ((regno == -1) || (regno == regi))
      *(regp + regno) = *(greg_t *) &registers[REGISTER_BYTE (regi)];

  if ((regno == -1) || (regno == PC_REGNUM))
    gregsetp->gp_pc = *(greg_t *) &registers[REGISTER_BYTE (PC_REGNUM)];

  if ((regno == -1) || (regno == PS_REGNUM))
    gregsetp->gp_cause = *(greg_t *) &registers[REGISTER_BYTE (PS_REGNUM)];

  if ((regno == -1) || (regno == HI_REGNUM))
    gregsetp->gp_mdhi = *(greg_t *) &registers[REGISTER_BYTE (HI_REGNUM)];

  if ((regno == -1) || (regno == LO_REGNUM))
    gregsetp->gp_mdlo = *(greg_t *) &registers[REGISTER_BYTE (LO_REGNUM)];
}

/*
 * Now we do the same thing for floating-point registers.
 * We don't bother to condition on FP0_REGNUM since any
 * reasonable MIPS configuration has an R3010 in it.
 *
 * Again, see the comments in m68k-tdep.c.
 */

void
supply_fpregset (fpregsetp)
     fpregset_t *fpregsetp;
{
  register int regno;

  for (regno = 0; regno < 32; regno++)
    supply_register (FP0_REGNUM + regno,
		     (char *)&fpregsetp->fp_r.fp_regs[regno]);

  supply_register (FCRCS_REGNUM, (char *)&fpregsetp->fp_csr);

  /* FIXME: how can we supply FCRIR_REGNUM?  SGI doesn't tell us. */
}

void
fill_fpregset (fpregsetp, regno)
     fpregset_t *fpregsetp;
     int regno;
{
  int regi;
  char *from, *to;
  extern char registers[];

  for (regi = FP0_REGNUM; regi < FP0_REGNUM + 32; regi++)
    {
      if ((regno == -1) || (regno == regi))
	{
	  from = (char *) &registers[REGISTER_BYTE (regi)];
	  to = (char *) &(fpregsetp->fp_r.fp_regs[regi]);
	  bcopy(from, to, REGISTER_RAW_SIZE (regno));
	}
    }

  if ((regno == -1) || (regno == FCRCS_REGNUM))
    fpregsetp->fp_csr = *(unsigned *) &registers[REGISTER_BYTE(FCRCS_REGNUM)];
}

#endif /* USE_PROC_FS */

/* To skip prologues, I use this predicate. Returns either PC
   itself if the code at PC does not look like a function prologue,
   PC+4 if it does (our caller does not need anything more fancy). */

CORE_ADDR
mips_skip_prologue(pc)
     CORE_ADDR pc;
{
    struct symbol *f;
    struct block *b;
    unsigned long inst;
    int offset;

    /* For -g modules and most functions anyways the
       first instruction adjusts the stack.
       But we allow some number of stores before the stack adjustment.
       (These are emitted by varags functions compiled by gcc-2.0. */
    for (offset = 0; offset < 100; offset += 4) {
	inst = read_memory_integer(pc + offset, 4);
	if ((inst & 0xffff0000) == 0x27bd0000) /* addiu $sp,$sp,offset */
	    return pc + offset + 4;
	if ((inst & 0xFFE00000) != 0xAFA00000) /* sw reg,n($sp) */
	    break;
    }

    /* Well, it looks like a frameless. Let's make sure.
       Note that we are not called on the current PC,
       but on the function`s start PC, and I have definitely
       seen optimized code that adjusts the SP quite later */
    b = block_for_pc(pc);
    if (!b) return pc;

    f = lookup_symbol(".gdbinfo.", b, LABEL_NAMESPACE, 0, NULL);
    if (!f) return pc;
    /* Ideally, I would like to use the adjusted info
       from mips_frame_info(), but for all practical
       purposes it will not matter (and it would require
       a different definition of SKIP_PROLOGUE())

       Actually, it would not hurt to skip the storing
       of arguments on the stack as well. */
    if (((mips_extra_func_info_t)SYMBOL_VALUE(f))->pdr.frameoffset)
	return pc + 4;

    return pc;
}

/* Figure out where the longjmp will land.
   We expect the first arg to be a pointer to the jmp_buf structure from which
   we extract the pc (JB_PC) that we will land at.  The pc is copied into PC.
   This routine returns true on success. */

int
get_longjmp_target(pc)
     CORE_ADDR *pc;
{
  CORE_ADDR jb_addr;

  jb_addr = read_register(A0_REGNUM);

  if (target_read_memory(jb_addr + JB_PC * JB_ELEMENT_SIZE, pc,
			 sizeof(CORE_ADDR)))
    return 0;

  SWAP_TARGET_AND_HOST(pc, sizeof(CORE_ADDR));

  return 1;
}
