#include <sys/param.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <utime.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

#define	HOST_PAGE_SIZE		NBPG
#define	HOST_MACHINE_ARCH	bfd_arch_i386
/* XXX hack! */
#define	HOST_DATA_START_ADDR	((bfd_vma)u.u_kproc.kp_eproc.e_vm.vm_daddr)
#define	HOST_STACK_END_ADDR	USRSTACK

#define	u_comm			u_kproc.kp_proc.p_comm

#define	MINIMIZE		1
#define	TRAD_CORE		1
#define	DEFAULT_VECTOR		bsdaout_vec
#define	SELECT_ARCHITECTURES	bfd_i386_arch

/* EXACT TYPES */
typedef char int8e_type;
typedef unsigned char uint8e_type;
typedef short int16e_type;
typedef unsigned short uint16e_type;
typedef int int32e_type;
typedef unsigned int uint32e_type;

/* CORRECT SIZE OR GREATER */
typedef char int8_type;
typedef unsigned char uint8_type;
typedef short int16_type;
typedef unsigned short uint16_type;
typedef int int32_type;
typedef unsigned int uint32_type;

#include "fopen-same.h"
