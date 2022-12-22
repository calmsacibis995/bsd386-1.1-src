/* vreset.c-
 
 *
 * This code fragment resets the standard VGA registers to sane values.  I
 * have been using it to recover from debugging the Xserver when only the
 * video is messed up.  There seems to be a segmentation bug somewhere in
 * the code too, which I can't usually recover from.  Likewise, the
 * keyboard is not always released, causing a crash as soon as you hit
 * a key.  Hope you find it useful.
 *                                      Steve Sellgren
 *                                      San Francisco Indigo Company
 *                                      sfindigo!sellgren@uunet.uu.net
 */
 
 
unsigned char CRTC[24] = {
    0x5F, 0x4F, 0x50, 0x82, 0x55, 0x81, 0xBF, 0x1F, 0x0, 0x4F, 0xD, 0xE,
    0x0, 0x0, 0x7, 0x80, 0x9C, 0x8E, 0x8F, 0x28, 0x1F, 0x96, 0xB9, 0xA3};
unsigned char SEQ[5] = {0x3, 0x0, 0x3, 0x0, 0x2};
unsigned char GC[9] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x10, 0xE, 0x0, 0xFF};
unsigned char AC[21] = {
    0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x14, 0x7, 0x38, 0x39, 0x3A,
    0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0xC, 0x0, 0xF, 0x8, 0x0};
 
static __inline__ void
outb(short port, char val)
{
  __asm__ volatile("out%B0 %0,%1" : :"a" (val), "d" (port));
}
 
static __inline__ unsigned int
inb(short port)
{
  unsigned int ret;
  __asm__ volatile("in%B0 %1,%0" : "=a" (ret) : "d" (port));
  return ret;
}
 
void
main()
{
    int i;
    int value;
 
    outb(0x3C2, 0x67);
 
    for (i = 0; i < 24; i++) {
        outb(0x3D4, i);
        outb(0x3D5, CRTC[i]);
    }
 
    for (i = 0; i < 5; i++) {
        outb(0x3C4, i);
        outb(0x3C5, SEQ[i]);
    }
    for (i = 0; i < 9; i++) {
        outb(0x3CE, i);
        outb(0x3CF, GC[i]);
    }
    value = inb(0x3DA);         /* reset flip-flop */
    for (i = 0; i < 16; i++) {
        outb(0x3C0, i);
        outb(0x3C0, AC[i]);
    }
    for (i = 16; i < 21; i++) {
        outb(0x3C0, i | 0x20);
        outb(0x3C0, AC[i]);
    }
}

