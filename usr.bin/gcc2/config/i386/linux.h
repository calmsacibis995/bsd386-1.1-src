/* Definitions for Intel 386 running Linux
 * Copyright (C) 1992 Free Software Foundation, Inc.
 *
 * Written by H.J. Lu (hlu@eecs.wsu.edu)
 *
 * Linux is a POSIX.1 compatible UNIX clone for i386, which uses GNU
 * stuffs as the native stuffs.
 */

#if 0	/* The FSF has fixed the known bugs. But ....... */

/* Linux has a hacked gas 1.38.1, which can handle repz, repnz
 * and fildll.
 */

#define GOOD_GAS

#endif

/* This is tested by i386/gas.h.  */
#define YES_UNDERSCORES

#ifndef LINUX_ELF
#include "i386/gstabs.h"
#endif

/* Specify predefined symbols in preprocessor.  */

#undef CPP_PREDEFINES
#define CPP_PREDEFINES "-Dunix -Di386 -Dlinux -Asystem(unix) -Asystem(posix) -Acpu(i386) -Amachine(i386)"

#undef CPP_SPEC
#if TARGET_CPU_DEFAULT == 2
#define CPP_SPEC "%{!m386:-D__i486__} %{posix:-D_POSIX_SOURCE}"
#else
#define CPP_SPEC "%{m486:-D__i486__} %{posix:-D_POSIX_SOURCE}"
#endif

#undef SIZE_TYPE
#define SIZE_TYPE "unsigned int"

#undef PTRDIFF_TYPE
#define PTRDIFF_TYPE "int"

#undef WCHAR_TYPE
#define WCHAR_TYPE "long int"

#undef WCHAR_TYPE_SIZE
#define WCHAR_TYPE_SIZE BITS_PER_WORD

#undef HAVE_ATEXIT
#define HAVE_ATEXIT

/* Linux uses ctype from glibc.a. I am not sure how complete it is.
 * For now, we play safe. It may change later.
 */
#if 0
#undef MULTIBYTE_CHARS
#define MULTIBYTE_CHARS	1
#endif

#undef LIB_SPEC
#define LIB_SPEC "%{mieee-fp:-lieee} %{p:-lgmon -lc_p} %{pg:-lgmon -lc_p} %{!p:%{!pg:%{!g*:-lc} %{g*:-lg}}}"


#undef STARTFILE_SPEC
#undef GPLUSPLUS_INCLUDE_DIR

#ifdef CROSS_COMPILE

/*
 * For cross-compile, we just need to search `$(tooldir)/lib'
 */

#define STARTFILE_SPEC  \
  "%{pg:gcrt0.o%s -static} %{!pg:%{p:gcrt0.o%s -static} %{!p:crt0.o%s %{g*:-static} %{!static:%{nojump:-nojump}} %{static:-static}}} -L"TOOLDIR"/lib"

/*
 *The cross-compile uses this.
 */
#define GPLUSPLUS_INCLUDE_DIR TOOLDIR"/g++-include"

#else

#define STARTFILE_SPEC  \
  "%{pg:gcrt0.o%s -static} %{!pg:%{p:gcrt0.o%s -static} %{!p:crt0.o%s %{g*:-static}%{!static:%{nojump:-nojump}} %{static:-static}}}"

/*
 *The native Linux system uses this.
 */
#define GPLUSPLUS_INCLUDE_DIR "/usr/g++-include"

#endif

/* There are conflicting reports about whether this system uses
   a different assembler syntax.  wilson@cygnus.com says # is right.  */
#undef COMMENT_BEGIN
#define COMMENT_BEGIN "#"

#undef ASM_APP_ON
#define ASM_APP_ON "#APP\n"

#undef ASM_APP_OFF
#define ASM_APP_OFF "#NO_APP\n"

/* Don't default to pcc-struct-return, because gcc is the only compiler, and
   we want to retain compatibility with older gcc versions.  */
#define DEFAULT_PCC_STRUCT_RETURN 0

/* We need that too. */
#define HANDLE_SYSV_PRAGMA

#undef LINK_SPEC

/* We want to pass -v to linker */
#if TARGET_CPU_DEFAULT == 2
#define LINK_SPEC	"%{v:-dll-verbose} %{!m386:-m486}"
#else
#define LINK_SPEC	"%{v:-dll-verbose} %{m486:-m486}"
#endif
