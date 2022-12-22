/* tm.h file for fmtowns, a 386 machine.  */

#include "tm-i386gas.h"

/* Control a conditional in i386.md, in call_value.  */
#define FMTOWNS 

/* On this machine, we really avoid using the fpu registers
   if we have no fpu.  */

#undef VALUE_REGNO
#define VALUE_REGNO(MODE) \
  ((TARGET_80387 && ((MODE)==SFmode || (MODE)==DFmode)) ? FIRST_FLOAT_REG : 0)

#undef FUNCTION_VALUE_REGNO
#define FUNCTION_VALUE_REGNO_P(N) \
  (TARGET_80387 ? ((N) == 0 || ((N)== FIRST_FLOAT_REG)) : ((N) == 0))

#undef REG_CLASS_FROM_LETTER
#define REG_CLASS_FROM_LETTER(C)		\
  ((C) == 'r' ? GENERAL_REGS :			\
   (C) == 'q' ? Q_REGS :			\
   (C) == 'f' ? (TARGET_80387 ? FLOAT_REGS : NO_REGS) :	\
   (C) == 'a' ? AREG : (C) == 'b' ? BREG :	\
   (C) == 'c' ? CREG : (C) == 'd' ? DREG :	\
   (C) == 'A' ? ADREG :				\
   (C) == 'S' ? SIREG :				\
   (C) == 'D' ? DIREG : NO_REGS)
