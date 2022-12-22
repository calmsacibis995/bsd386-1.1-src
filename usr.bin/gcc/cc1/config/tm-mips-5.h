#define LD_SPEC "%{non_shared} %{shared} %{!shared: %{!non_shared: -non_shared}}

#include "tm-mips.h"
