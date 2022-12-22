#ifndef BYTE
typedef unsigned char byte;
typedef unsigned short word;
#define BYS (byte *)
#endif
#define BYTE

/*******************************************************/

#define ATC_REG_A 0x3C0
#define ATC_REG_D 0x3C1
#define MISC_REG_W 0x3C2
#define I_STAT_0_R 0x3C2
#define SYS_REG_INTRN 0x3C3
#define SEQ_REG_A 0x3C4
#define SEQ_REG_D 0x3C5
#define DAC_MASK 0x3C6
#define DAC_STATE 0x3C7
#define DAC_ADDR_R 0x3C7
#define DAC_ADDR_W 0x3C8
#define DAC_DATA 0x3C9
#define FEATURE_R 0x3CA
#define MISC_REG_R 0x3CC
#define GRAPH_REG_A 0x3CE
#define GRAPH_REG_D 0x3CF
#define CRT_REG_A 0x3D4
#define CRT_REG_D 0x3D5
#define CGA_MODE_REG 0x3D8
#define CGA_COLOR_REG 0x3D9
#define CGA_STATUS_REG 0x3DA
#define FEATURE_W 0x3DA
/* not supported by VGA:
#define CGA_LPEN_RESET 0x3DB
#define CGA_LPEN_SET 0x3DC
*/
#define SYS_REG_EXTRN 0x46E8

#define N_ATC_REG 0x15
#define N_SEQ_REG 0x05
#define N_GRAPH_REG 0x09
#define N_CRT_REG 0x19
#define N_DAC_REG 0x100
#define N_DAC_REG_BYTE 0x100*3

/*******************************************************/

#define MDA_BUF_ADR 0xB0000
#define CGA_BUF_ADR 0xB8000
#define VGA_BUF_ADR 0xA0000
#define CGA_BUF_L 0x8000
#define VGA_PLANE_L 0x10000
#define FONT_L 0x4000  /* throw away other fonts ... */
/*#define FONT_L VGA_PLANE_L*/
#define MAX_VIDEO_SIZE 0x40000  /* 256 K */

/*******************************************************/

typedef struct VGAREGS {
  int saved;
  byte atc_ar;
  byte atc_r[N_ATC_REG];
  byte seq_ar;
  byte seq_r[N_SEQ_REG];
  byte graph_ar;
  byte graph_r[N_GRAPH_REG];
  byte crt_ar;
  byte crt_r[N_CRT_REG];
  byte dac_ar_r;  /* it is DAC state also !!! */
  byte dac_ar_w;
  byte dac_r[N_DAC_REG*3];
  byte dac_mask;
  byte misc_r;
  byte feature_r;
  byte sys_reg;  /* ??? */
  byte *buf;
  int planes;
  int text;
  int buff_addr;
  byte *save_v86_buf;
} VGAREGS;

/*******************************************************/

#define InW(y) \
({ unsigned short _tmp__; \
	asm volatile("inw %1, %0" : "=a" (_tmp__) : "d" ((unsigned short)(y))); \
	_tmp__; })

#define InB(y) \
({ unsigned char _tmp__; \
	asm volatile("inb %1, %0" : "=a" (_tmp__) : "d" ((unsigned short)(y))); \
	_tmp__; })


#define OutW(x, y) \
{asm volatile("outw %0, %1" : : "a" ((unsigned short)(y)) , "d" ((unsigned short)(x))); }


#define OutB(x, y) \
{ asm volatile("outb %0, %1" : : "a" ((unsigned char)(y)) , "d" ((unsigned short)(x))); }

#ifdef notdef
static inline byte InB (word port)
{
  byte rs;

  asm ( "movw %0,%%dx" : : "g" (port) : "dx" );
  asm ( "inb %%dx,%%al" : : : "ax" );
  asm ( "movb %%al,%0" : "=g" (rs) );
  return rs;
}

static inline word InW (word port)

{
  word rs;

  asm ( "movw %0,%%dx" : : "g" (port) : "dx" );
  asm ( "inw %%dx,%%ax" : : : "ax" );
  asm ( "movw %%ax,%0" : "=g" (rs) );
  return rs;
}

static inline void OutB (word port, byte val)
{
  asm ( "movw %0,%%dx" : : "g" (port) : "dx" );
  asm ( "movb %0,%%al" : : "g" (val) : "ax" );
  asm ( "outb %al,%dx" );
}

static inline void OutW (word port, word val)
{
  asm ( "movw %0,%%dx" : : "g" (port) : "dx" );
  asm ( "movw %0,%%ax" : : "g" (val) : "ax" );
  asm ( "outw %ax,%dx" );
}

#endif

/*******************************************************/

#ifdef notdef

static inline void vmemcpy (byte *dst, byte *src, int count)
{
  __asm__ volatile("cld");
  __asm__ volatile("movl %0,%%esi" : : "g"(src) : "si");
  __asm__ volatile("movl %0,%%edi" : : "g"(dst) : "di");
  __asm__ volatile("movl %0,%%ecx" : : "g"(count) : "cx");
  __asm__ volatile("rep; movsb");
}

#endif

#define vmemcpy memcpy

/*******************************************************/

