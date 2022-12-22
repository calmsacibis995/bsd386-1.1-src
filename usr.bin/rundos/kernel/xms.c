#include <stdio.h>
#include <sys/types.h>
#include "kernel.h"

#ifdef DEBUG
static char *xms_services[] = {
  "00h  Get XMS version number",
  "01h  Request High Memory Area (1M to 1M + 64K)",
  "02h  Release High Memory Area",
  "03h  Global enable A20, for using the HMA",
  "04h  Global disable A20",
  "05h  Local enable A20, for direct access to extended memory",
  "06h  Local disable A20",
  "07h  Query A20 state",
  "08h  Query free extended memory, not counting HMA",
  "09h  Allocate extended memory block",
  "0Ah  Free extended memory block",
  "0Bh  Move extended memory block",
  "0Ch  Lock extended memory block",
  "0Dh  Unlock extended memory block",
  "0Eh  Get handle information",
  "0Fh  Reallocate extended memory block",
  "10h  Request upper memory block (nonEMS memory above 640K)",
  "11h  Release upper memory block",
};
#endif

/*
 * xms error codes
 */
#define XMS_NOT_IMPLEMENTED	0x80
#define XMS_VDISK_DETECTED	0x81
#define XMS_A20_ERROR		0x82
#define XMS_DRIVER_ERROR	0x8e
#define XMS_FATAL_DRIVER_ERROR	0x8f
#define XMS_NO_HMA		0x90
#define XMS_HMA_INUSE		0x91
#define XMS_HMA_DX_TOO_SMALL	0x92
#define XMS_HMA_NOT_ALLOCATED	0x93
#define XMS_A20_STILL_ENABLED	0x94
#define XMS_EXT_NO_MEM		0xa0
#define XMS_EXT_NO_HANDLES	0xa1
#define XMS_EXT_BAD_HANDLE	0xa2
#define XMS_EXT_BAD_SRC_HANDLE	0xa3
#define XMS_EXT_BAD_SRC_OFFSET	0xa4
#define XMS_EXT_BAD_DST_HANDLE	0xa5
#define XMS_EXT_BAD_DST_OFFSET	0xa6
#define XMS_EXT_BAD_LENGTH	0xa7
#define XMS_EXT_OVERLAP		0xa8
#define XMS_EXT_PARITY		0xa9
#define XMS_EXT_NOT_LOCKED	0xaa
#define XMS_EXT_LOCKED		0xab
#define XMS_EXT_NO_LOCKS	0xac
#define XMS_EXT_LOCK_FAILED	0xad
#define XMS_UMB_SMALLER_AVAIL	0xb0
#define XMS_UMB_NO_MEM		0xb1
#define XMS_UMB_BAD_SEG		0xb2

#define XMS_HANDLES 256
#define XMS_MAX_SIZE (4 * 1024 - 64)	/* 4Mb - 64Kb hma */

#define xms_set_error_code(CODE) \
  tf->tf_bx = (tf->tf_bx & 0xff00) | (CODE)

static struct {
  void *data;
  int len;
} xms_handles[XMS_HANDLES];

static int xms_curr_handles = XMS_HANDLES;
static int xms_curr_size = XMS_MAX_SIZE;
static int xms_hma_locked = 0;

void xms_reset(void)
{
  int i;

  xms_curr_handles = XMS_HANDLES;
  xms_curr_size = XMS_MAX_SIZE;
  xms_hma_locked = 0;
  for (i = 0; i < XMS_HANDLES; i++)
    if (xms_handles[i].data) {
      free(xms_handles[i].data);
      xms_handles[i].data = NULL;
    }
}

static inline int xms_bad_handle(int handle)
{
  if ((handle < 0 || handle >= XMS_HANDLES) ||
      xms_handles[handle].data == NULL)
    return(1);
  return(0);
}

void xms_service(struct trapframe *tf)
{
  void *malloc(size_t);
  void *src, *dst;
  unsigned short service;
  int handle, addr;
  unsigned long length, offset;

  tf->tf_ax = tf->tf_fs;
  tf->tf_dx = tf->tf_gs;

  service = tf->tf_ax >> 8;

#ifdef DEBUG
  dprintf("XMS: %s\n",
	  service < 0x12 ? xms_services[service] : "unknown service");
#endif
  switch(service) {
  case 0x00:
    tf->tf_ax = 0;
    tf->tf_bx = 0;
    tf->tf_dx = 1;
    return;
  case 0x01:
    if (xms_hma_locked) {
      tf->tf_ax = 0;
      xms_set_error_code(XMS_HMA_INUSE);
    } else {
      tf->tf_ax = 1;
      xms_hma_locked++;
    }
    return;
  case 0x02:
    if (xms_hma_locked) {
      tf->tf_ax = 1;
      xms_hma_locked = 0;
    } else {
      tf->tf_ax = 0;
      xms_set_error_code(XMS_HMA_NOT_ALLOCATED);
    }
    return;
  case 0x03:
    tf->tf_ax = 1;
    return;
  case 0x04:
    tf->tf_ax = 0;
    xms_set_error_code(XMS_A20_STILL_ENABLED);
    return;
  case 0x05:
    tf->tf_ax = 1;
    return;
  case 0x06:
    tf->tf_ax = 0;
    xms_set_error_code(XMS_A20_STILL_ENABLED);
    return;
  case 0x07:
    tf->tf_ax = 1;
    xms_set_error_code(0);
    return;
  case 0x08:
    tf->tf_ax = xms_curr_size;
    tf->tf_dx = XMS_MAX_SIZE;
    xms_set_error_code(0);
    return;
  case 0x09:
    for (handle = 0; handle < XMS_HANDLES; handle++)
      if (xms_handles[handle].data == NULL)
	break;
    if (handle == XMS_HANDLES) {
      tf->tf_ax = 0;
      xms_set_error_code(XMS_EXT_NO_HANDLES);
    } else if (xms_curr_size < tf->tf_dx) {
      tf->tf_ax = 0;
      xms_set_error_code(XMS_EXT_NO_MEM);
    } else if ((xms_handles[handle].data = malloc(tf->tf_dx * 1024)) == NULL) {
      tf->tf_ax = 0;
      xms_set_error_code(XMS_EXT_NO_MEM);
    } else {
      xms_curr_handles--;
      xms_curr_size -= tf->tf_dx;
      xms_handles[handle].len = tf->tf_dx * 1024;
      tf->tf_ax = 1;
      tf->tf_dx = handle + 1;
    }
    return;
  case 0x0a:
    handle = tf->tf_dx - 1;
    if (xms_bad_handle(handle)) {
      tf->tf_ax = 0;
      xms_set_error_code(XMS_EXT_BAD_HANDLE);
    } else {
      tf->tf_ax = 1;
      free(xms_handles[handle].data);
      xms_handles[handle].data = NULL;
      xms_curr_handles++;
      xms_curr_size += xms_handles[handle].len / 1024;
    }
    return;
  case 0x0b:
    addr = MAKE_ADDR(tf->tf_ds,tf->tf_si);
    length = *(unsigned long *)addr;
    handle = *(unsigned short *)(addr + 4);
    offset = *(unsigned long *)(addr + 6);
    if (handle == 0)
      src = (void *)MAKE_ADDR(offset >> 16, offset & 0xffff);
    else {
      handle--;
      if (xms_bad_handle(handle)) {
	xms_set_error_code(XMS_EXT_BAD_SRC_HANDLE);
	tf->tf_ax = 0;
	return;
      } else if (offset > xms_handles[handle].len) {
	xms_set_error_code(XMS_EXT_BAD_SRC_OFFSET);
	tf->tf_ax = 0;
	return;
      } else if ((offset + length) > xms_handles[handle].len) {
	xms_set_error_code(XMS_EXT_BAD_LENGTH);
	tf->tf_ax = 0;
	return;
      } else
	src = (void *)((unsigned long)xms_handles[handle].data + offset);
    }
    handle = *(unsigned short *)(addr + 10);
    offset = *(unsigned long *)(addr + 12);
    if (handle == 0)
      dst = (void *)MAKE_ADDR(offset >> 16, offset & 0xffff);
    else {
      handle--;
      if (xms_bad_handle(handle)) {
	xms_set_error_code(XMS_EXT_BAD_DST_HANDLE);
	tf->tf_ax = 0;
	return;
      } else if (offset > xms_handles[handle].len) {
	xms_set_error_code(XMS_EXT_BAD_DST_OFFSET);
	tf->tf_ax = 0;
	return;
      } else if ((offset + length) > xms_handles[handle].len) {
	xms_set_error_code(XMS_EXT_BAD_LENGTH);
	tf->tf_ax = 0;
	return;
      } else
	dst = (void *)((unsigned long)xms_handles[handle].data + offset);
    }
    tf->tf_ax = 1;
    bcopy(src, dst, length);
    return;
  case 0x0c:
  case 0x0d:			
    tf->tf_ax = 0;
    xms_set_error_code(XMS_NOT_IMPLEMENTED);
    return;
  case 0x0e:
    handle = tf->tf_dx - 1;
    if (xms_bad_handle(handle)) {
      tf->tf_ax = 0;
      xms_set_error_code(XMS_EXT_BAD_HANDLE);
    } else {
      tf->tf_ax = 1;
      tf->tf_bx = xms_curr_handles;
      tf->tf_dx = xms_handles[handle].len / 1024;
    }
    return;
  case 0x0f:
    handle = tf->tf_dx - 1;
    if (xms_bad_handle(handle)) {
      tf->tf_ax = 0;
      xms_set_error_code(XMS_EXT_BAD_HANDLE);
    } else if ((xms_curr_size + xms_handles[handle].len / 1024) < tf->tf_bx) {
      tf->tf_ax = 0;
      xms_set_error_code(XMS_EXT_NO_MEM);
    } else {
      void *save_data = xms_handles[handle].data;

      if ((xms_handles[handle].data = malloc(tf->tf_bx * 1024)) == NULL) {
	xms_handles[handle].data = save_data;
	tf->tf_ax = 0;
	xms_set_error_code(XMS_EXT_NO_MEM);
      } else {
	free(save_data);
	tf->tf_ax = 1;
	xms_curr_size += xms_handles[handle].len / 1024;
	xms_curr_size -= tf->tf_bx;
	xms_handles[handle].len = tf->tf_bx * 1024;
      }
    }
    return;
  case 0x10:
  case 0x11:
    xms_set_error_code(XMS_UMB_NO_MEM);
    return;
  default:
    xms_set_error_code(XMS_NOT_IMPLEMENTED);
    return;
  }
}
