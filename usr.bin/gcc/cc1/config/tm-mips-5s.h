#define MIPS_SYSV

#define LD_SPEC "%{non_shared} %{shared} %{!shared: %{!non_shared: -non_shared}}

#include "tm-mips.h"

#define TARGET_MEM_FUNCTIONS
