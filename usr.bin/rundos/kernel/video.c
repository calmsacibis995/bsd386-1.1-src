/*******************************************************/

#define Wof2B(x,y) ( (x & 0xFF) | ((y & 0xFF) << 8) )

/*******************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernel.h"
#include "video.h"

/*******************************************************/

void InitVideo (struct trapframe *tf);
static void SaveVState (VGAREGS *vgar);
static void RestoreVState (VGAREGS *vgar);
static void SaveVGARegs (VGAREGS *vgar);
static void RestoreVGARegs (VGAREGS *vgar);
static int ArePlanes (void);
static void SaveVGAint10 (VGAREGS *vgar);
static void RestoreVGAint10 (VGAREGS *vgar);

/*******************************************************/
extern struct trapframe tf_forV86;

static int mnchrm; /* 0 - color, 0x20 - monochrom: subtracted from port num */

static int vga_save_v86 = 0;
static int vga_save86_len = 0;
static byte *save_v86_area_v;
static byte *place_to_save_video = (byte *)0x90000;

VGAREGS vgar_dos = { 0 };     /* saved = 0 ! */   /* temp. not static */
VGAREGS vgar_bsd = { 0 };     /* saved = 0 ! */   /* temp. not static */

word columns;

int no_save_video = 0;

/*******************************************************/

void InitVideo (struct trapframe *tf)
{
  if (no_save_video)
    return;

  if ( (vgar_bsd.buf = malloc (CGA_BUF_L+FONT_L)) == NULL  ||
      /* BSD assumed to be in text mode always */
       (vgar_dos.buf = malloc (MAX_VIDEO_SIZE)) == NULL )
    ErrExit (0x08, "Initialize video: out of memory\n");

  if (tf->tf_bx != 0) {
    vga_save86_len = tf->tf_bx * 64;
    if ( (vgar_bsd.save_v86_buf = malloc (vga_save86_len)) == NULL  ||
	(vgar_dos.save_v86_buf = malloc (vga_save86_len)) == NULL  ||
	(save_v86_area_v = malloc (vga_save86_len)) == NULL )
      vga_save_v86 = 0;
    else
      vga_save_v86 = 1;
  }
  else {
    vgar_bsd.save_v86_buf = NULL;
    vgar_dos.save_v86_buf = NULL;
    vga_save_v86 = 0;
  }
#ifdef DEBUG
  dprintf ("VIDEO: save by int10 = %d, buff length = %4.4X\n",
	   vga_save_v86,vga_save86_len);
#endif

  mnchrm = 0;  /* It is determined in 'SaveVGARegs' */

  SaveVState (&vgar_bsd);

}

SetPlaneStructure (VGAREGS * vgar)
{
    (void) InB (CGA_STATUS_REG-mnchrm);  /* to force address mode */
    OutB(ATC_REG_A,0x20|0x10);   /* 0x20: palette addr not from processor */
    OutB(ATC_REG_A, 0x01);    /* graphics mode */
    OutW(SEQ_REG_A, Wof2B (4, 0x06) );      /* enable plane graphics */
    OutW(GRAPH_REG_A, Wof2B (6, 0x05) );    /* set graphics -- A000 */
}

RestoreMemStruc (VGAREGS * vgar)
{
    (void) InB (CGA_STATUS_REG-mnchrm);  /* to force address mode */
    OutB(ATC_REG_A,0x20|0x10);   /* 0x20: palette addr not from processor */
    OutB(ATC_REG_A, vgar->atc_r[0x10]);               /* mode */
    OutW(SEQ_REG_A, Wof2B (2, vgar->seq_r[2]) );      /* map mask */
    OutW(SEQ_REG_A, Wof2B (4, vgar->seq_r[4]) );      /* memory mode */
    OutW(GRAPH_REG_A, Wof2B (1, vgar->graph_r[1]) );  /* enable set/reset */
    OutW(GRAPH_REG_A, Wof2B (4, vgar->graph_r[4]) );  /* read map select */
    OutW(GRAPH_REG_A, Wof2B (5, vgar->graph_r[5]) );  /* mode */
    OutW(GRAPH_REG_A,  Wof2B (6, vgar->graph_r[6]) ); /* graph control */
}

void SetStdVideoRWMode ()
{
  OutW(GRAPH_REG_A, Wof2B (1, 0x00) );  /* disable set/reset */
  OutW(GRAPH_REG_A, Wof2B (3, 0x00) );  /* no rotate, func = replace */
  OutW(GRAPH_REG_A, Wof2B (5, 0x00) );  /* write mode 0, read mode 0 */
}

void SaveVState (VGAREGS * vgar)
{
  int j;

  if (vga_save_v86)
    SaveVGAint10 (vgar);
  SaveVGARegs (vgar);

  if (vgar->planes) {
    SetPlaneStructure (vgar);
    for ( j=0 ; j<4 ; ++j ) {
      OutW (GRAPH_REG_A, Wof2B (4, j) );      /* read j-th plane */
      OutW (GRAPH_REG_A, Wof2B (5, 0x00) );   /* write mode 0, read mode 0 */
      vmemcpy (vgar->buf+(VGA_PLANE_L*j), BYS VGA_BUF_ADR, VGA_PLANE_L);
    }
    RestoreMemStruc (vgar);
  }
  else {
    /* save buffer */
    vmemcpy (vgar->buf, BYS vgar->buff_addr, CGA_BUF_L);
    if (vgar->text) {
      /* save fonts */
      SetPlaneStructure (vgar);
      OutW (GRAPH_REG_A, Wof2B (4, 2) );      /* read plane 2 */
      OutW (GRAPH_REG_A, Wof2B (5, 0x00) );   /* write mode 0, read mode 0 */
      vmemcpy (vgar->buf+CGA_BUF_L, BYS VGA_BUF_ADR, FONT_L);
      RestoreMemStruc (vgar);
    }
  }
  columns = vgar->crt_r[1] + 1;
  vgar->saved = 1;
}

void RestoreVState (VGAREGS * vgar)
{
  int j;

  if (! vgar->saved)
    return;
  if (vgar->planes) {
    SetPlaneStructure (vgar);
    for ( j=0 ; j<4 ; ++j ) {
      OutW(SEQ_REG_A, Wof2B (2, (1<<j) ) ); /* mask all maps exept j */
      SetStdVideoRWMode ();
      vmemcpy ( BYS VGA_BUF_ADR, vgar->buf + (VGA_PLANE_L * j), VGA_PLANE_L);
    }
  }
  else {
    if (vgar->text) {
      /* restore fonts */
      SetPlaneStructure (vgar);
      OutW(SEQ_REG_A, Wof2B (2, 0x04) );    /* mask all maps exept 2nd */
      SetStdVideoRWMode ();
      vmemcpy ( BYS VGA_BUF_ADR, vgar->buf+CGA_BUF_L, FONT_L);
    }
  }

  RestoreVGARegs (vgar);
  if (vga_save_v86)
    RestoreVGAint10 (vgar);

  if (! vgar->planes)          /* restore buffer */
    vmemcpy (BYS vgar->buff_addr, vgar->buf, CGA_BUF_L);
}

static void SaveVGARegs (VGAREGS * vgar)
{
  int j;

  vgar->misc_r = InB (MISC_REG_R);
  /* determine mnchrm by bit 0 of the misc. reg */
  mnchrm = (vgar->misc_r & 0x01) ?  0 : 0x20;

  vgar->atc_ar = InB (ATC_REG_A);  /* not correctly */
  for ( j=0 ; j<N_ATC_REG ; ++j ) {
    (void) InB (CGA_STATUS_REG-mnchrm);  /* to force address mode */
    OutB (ATC_REG_A, j);
    vgar->atc_r[j] = InB (ATC_REG_D);
  }
  (void) InB (CGA_STATUS_REG); /* to force address mode */
  OutB (ATC_REG_A, 0x20);      /* ATC address source not from processor */

  vgar->seq_ar = InB (SEQ_REG_A);
  for ( j=0 ; j<N_SEQ_REG ; ++j ) {
    OutB (SEQ_REG_A, j);
    vgar->seq_r[j] = InB (SEQ_REG_D);
  }
  vgar->graph_ar = InB (GRAPH_REG_A);
  for ( j=0 ; j<N_GRAPH_REG ; ++j ) {
    OutB (GRAPH_REG_A, j);
    vgar->graph_r[j] = InB (GRAPH_REG_D);
  }
  vgar->crt_ar = InB (CRT_REG_A-mnchrm);
  for ( j=0 ; j<N_CRT_REG ; ++j ) {
    OutB (CRT_REG_A-mnchrm, j);
    vgar->crt_r[j] = InB (CRT_REG_D-mnchrm);
  }
  vgar->dac_ar_r = InB (DAC_ADDR_R);
  vgar->dac_ar_w = InB (DAC_ADDR_W);
  for ( j=0 ; j<N_DAC_REG_BYTE ; j+=3 ) {
    OutB (DAC_ADDR_R, j);
    vgar->dac_r[j] = InB (DAC_DATA);
    vgar->dac_r[j+1] = InB (DAC_DATA);
    vgar->dac_r[j+2] = InB (DAC_DATA);
  }
  vgar->dac_mask = InB (DAC_MASK);
  vgar->feature_r = InB (FEATURE_R);
  vgar->sys_reg = InB (SYS_REG_INTRN);  /* do it ??? */

  if ( vgar->graph_r[6] & 0x08 ) {
    vgar->planes = 0;  /* not reliable -- anyone can change mapping !!! */
    if ( vgar->graph_r[6] & 0x04 )
      vgar->buff_addr = 0xB8000;
    else
      vgar->buff_addr = 0xB0000;
  }
  else {
    vgar->planes = 1;  /* not reliable -- anyone can change mapping !!! */
    vgar->buff_addr = 0xA0000;
  }
  if ( !(vgar->atc_r[0x10] & 0x01) && /* (vgar->seq_r[4] & 0x01) && */
      !(vgar->graph_r[6] & 0x01) )  /* text mode -- (ouch, NOT) reliable way */
    vgar->text = 1;
  else
    vgar->text = 0;

#ifdef DEBUG
  dprintf ("VIDEO: ATC regs: palette 0=%2.2X 1=%2.2X 2=%2.2X 3=%2.2X\n",
	   vgar->atc_r[0],vgar->atc_r[1],vgar->atc_r[2],vgar->atc_r[3] );
  dprintf ("VIDEO: ATC regs: palette 4=%2.2X 5=%2.2X 6=%2.2X 7=%2.2X\n",
	   vgar->atc_r[4],vgar->atc_r[5],vgar->atc_r[6],vgar->atc_r[7] );
  dprintf ("VIDEO: ATC regs: palette 8=%2.2X 9=%2.2X A=%2.2X B=%2.2X\n",
	   vgar->atc_r[8],vgar->atc_r[9],vgar->atc_r[0x0a],vgar->atc_r[0x0b] );
  dprintf ("VIDEO: ATC regs: palette C=%2.2X D=%2.2X E=%2.2X F=%2.2X\n",
	   vgar->atc_r[0x0c],vgar->atc_r[0x0d],vgar->atc_r[0x0e],vgar->atc_r[0x0f] );
  dprintf ("VIDEO: ATC: mode=%2.2X bord=%2.2X en_plane=%2.2X shft=%2.2X dac_pg=%2.2X\n",
	   vgar->atc_r[0x10],vgar->atc_r[0x11],vgar->atc_r[0x12],vgar->atc_r[0x13],vgar->atc_r[0x14] );
  dprintf ("VIDEO: SEQ: reset=%2.2X clk=%2.2X map_msk=%2.2X ch_map=%2.2X, mem=%2.2X\n",
	   vgar->seq_r[0],vgar->seq_r[1],vgar->seq_r[2],vgar->seq_r[3],vgar->seq_r[4]);
  dprintf ("VIDEO: GRAPH: set/re=%2.2X enbl_s/r=%2.2X clr_cmp=%2.2X rot/fun=%2.2X\n",
	   vgar->graph_r[0],vgar->graph_r[1],vgar->graph_r[2],vgar->graph_r[3] );
  dprintf ("VIDEO: GRAPH: rd_map=%2.2X mode=%2.2X misc=%2.2X clr_ms=%2.2X bit_ms=%2.2X\n",
	   vgar->graph_r[4],vgar->graph_r[5],vgar->graph_r[6],vgar->graph_r[7],vgar->graph_r[8] );
  dprintf ("VIDEO: ADRS: seq=%2.2X graph=%2.2X crt=%2.2X dac_r=%2.2X dac_w=%2.2X \n",
vgar->seq_ar,vgar->graph_ar,vgar->crt_ar,vgar->dac_ar_r,vgar->dac_ar_w );
  dprintf ("VIDEO: CRT: h_total=%2.2X h_dsp_end=%2.2X strt_h_bln=%2.2X end_h_bln=%2.2X\n",
	   vgar->crt_r[0],vgar->crt_r[1],vgar->crt_r[2],vgar->crt_r[3]);
  dprintf ("VIDEO: CRT: strt_h_rtrc=%2.2X end_h_rtrc=%2.2X v_total=%2.2X ovfl=%2.2X\n",
	   vgar->crt_r[4],vgar->crt_r[5],vgar->crt_r[6],vgar->crt_r[7]);
  dprintf ("VIDEO: CRT: preset_row=%2.2X max_scn_ln=%2.2X curs_s=%2.2X curs_e=%2.2X\n",
	   vgar->crt_r[8],vgar->crt_r[9],vgar->crt_r[0x0a],vgar->crt_r[0x0b]);
  dprintf ("VIDEO: CRT: s_adr_hi=%2.2X s_adr_lo=%2.2X cu_pos_hi=%2.2X cu_pos_lo=%2.2X\n",
	   vgar->crt_r[0x0c],vgar->crt_r[0x0d],vgar->crt_r[0x0e],vgar->crt_r[0x0f]);
  dprintf ("VIDEO: CRT: strt_v_rtrc=%2.2X end_v_rtrc=%2.2X v_dsp_end=%2.2X off=%2.2X\n",
	   vgar->crt_r[0x10],vgar->crt_r[0x11],vgar->crt_r[0x12],vgar->crt_r[0x13]);
  dprintf ("VIDEO: CRT: undlin=%2.2X strt_v_bln=%2.2X end_v_bln=%2.2X mode=%2.2X\n",
	   vgar->crt_r[0x14],vgar->crt_r[0x15],vgar->crt_r[0x16],vgar->crt_r[0x17]);
#endif
}

static void RestoreVGARegs (VGAREGS *vgar)
{
  int j;

/*  synch. sequencer reset */
/*  OutW (SEQ_REG_A, Wof2B (0, (vgar->seq_r[0] & 0xFD)) );*/
  if (mnchrm)    /* pre-check -- from X */
    vgar->misc_r &= 0xFE;
  else
    vgar->misc_r |= 0x01;
  OutB (MISC_REG_W, vgar->misc_r);

  for ( j=0 ; j<N_ATC_REG ; ++j ) {
    (void) InB (CGA_STATUS_REG-mnchrm);  /* to force address mode */
    OutB (ATC_REG_A, j);
    OutB (ATC_REG_A, vgar->atc_r[j] );  /* REG_A is NOT mistake */
  }
  (void) InB (CGA_STATUS_REG); /* to force address mode */
  OutB (ATC_REG_A, 0x20);      /* ATC address source not from processor */

  for ( j=0 ; j<N_SEQ_REG ; ++j )
    OutW (SEQ_REG_A, Wof2B (j, vgar->seq_r[j]) );
  OutB (SEQ_REG_A, vgar->seq_ar);

  for ( j=0 ; j<N_GRAPH_REG ; ++j )
    OutW (GRAPH_REG_A, Wof2B (j, vgar->graph_r[j]) );
  OutB (GRAPH_REG_A, vgar->graph_ar);

  OutW (CRT_REG_A-mnchrm, Wof2B (0x11, vgar->crt_r[0x11] & 0x7F) );
  for ( j=0 ; j<N_CRT_REG ; ++j )
    OutW (CRT_REG_A-mnchrm, Wof2B (j, vgar->crt_r[j]) );
  OutB (CRT_REG_A-mnchrm, vgar->crt_ar);

  for ( j=0 ; j<N_DAC_REG_BYTE ; j+=3 ) {
    OutB (DAC_ADDR_W, j);
    OutB (DAC_DATA, vgar->dac_r[j]);
    OutB (DAC_DATA, vgar->dac_r[j+1]);
    OutB (DAC_DATA, vgar->dac_r[j+2]);
  }
/*  OutB (DAC_ADDR_R, vgar->dac_ar_r);*/   /* ??? */
  OutB (DAC_ADDR_W, vgar->dac_ar_w);

  OutB (DAC_MASK, vgar->dac_mask);

  OutB (FEATURE_W - mnchrm, vgar->feature_r);
/*  OutB (SYS_REG_INTRN, vgar->sys_reg);*/  /* ??? */
}

/**************************************/

void vga_save_initial_mode(void)
{
  if (!no_save_video)
    SaveVState (&vgar_bsd);
}

void vga_restore_initial_mode(void)
{
  if (!no_save_video)
    RestoreVState (&vgar_bsd);
}

void vga_save_current_mode(void)
{
  if (!no_save_video)
    SaveVState (&vgar_dos);
}

void vga_restore_current_mode(void)
{
  if (!no_save_video)
    RestoreVState (&vgar_dos);
}

/**************************************/

static void SaveVGAint10 (VGAREGS *vgar)
{
  tf_forV86.tf_es = ((int)place_to_save_video & 0xFFFF0) >> 4;
  tf_forV86.tf_bx = ((int)place_to_save_video & 0x0000F);
  memcpy (save_v86_area_v, place_to_save_video, vga_save86_len);
  toV86_msgs (0x6E);
  memcpy (vgar->save_v86_buf, place_to_save_video, vga_save86_len);
  memcpy (place_to_save_video, save_v86_area_v, vga_save86_len);
}

static void RestoreVGAint10 (VGAREGS *vgar)
{
  tf_forV86.tf_es = ((int)place_to_save_video & 0xFFFF0) >> 4;
  tf_forV86.tf_bx = ((int)place_to_save_video & 0x0000F);
  memcpy (save_v86_area_v, place_to_save_video, vga_save86_len);
  memcpy (place_to_save_video, vgar->save_v86_buf, vga_save86_len);
  toV86_msgs (0x6F);
  memcpy (place_to_save_video, save_v86_area_v, vga_save86_len);
}
