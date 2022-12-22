/*	BSDI $Id: midiioctl.h,v 1.1 1992/08/21 21:15:35 karels Exp $	*/

/*
 *	Definitions for MusicQuest MIDI driver 
 */

#define	MIDI_NOTE_ON	0x90
#define	MIDI_NOTE_OFF	0x80
#define	MIDI_NOOP	0xf8
#define	MIDI_MAX_TICKS	0xef


typedef struct {
	unsigned char	*data;
	int		num;
} MIDI_MESS;

/*
 *	These are all those wonderful ioctl commands
 */
#define  MCLRQ				_IO('M', 0x04)
#define  MFLUSHQ			_IO('M', 0x05)

#define	 MRESET				_IO('m', 0xff)
#define	 MUART				_IO('m', 0x3f)
#define	 MSTART				_IO('m', 0x0a)
#define	 MCONT				_IO('m', 0x0b)
#define	 MSTOP				_IO('m', 0x05)
#define	 MRECSTART			_IO('m', 0x22)
#define	 MRECSTDBY			_IO('m', 0x20)
#define	 MRECCONT			_IO('m', 0x23)
#define	 MRECSTOP			_IO('m', 0x11)
#define	 MSTARTANDREC			_IO('m', 0x2a)
#define	 MCONTANDREC			_IO('m', 0x2b)
#define	 MSTARTANDSTDBY			_IO('m', 0x28)
#define	 MSTOPANDREC			_IO('m', 0x15)
#define	 MSNDSTART			_IO('m', 0x02)
#define	 MSNDSTOP			_IO('m', 0x01)
#define	 MSNDCONT			_IO('m', 0x03)
#define	 MRTMOFF			_IO('m', 0x32)
#define	 MTIMEON			_IO('m', 0x34)
#define	 MEXTCONTOFF			_IO('m', 0x90)
#define	 MEXTCONTON			_IO('m', 0x91)
#define	 MSTOPTRANOFF			_IO('m', 0x8a)
#define	 MSTOPTRANON			_IO('m', 0x8b)
#define	 MCLRPC				_IO('m', 0xb8)
#define	 MCLRPM				_IO('m', 0xb9)
#define	 MCLRRECCNT			_IO('m', 0xba)
#define	 MCONDOFF			_IO('m', 0x8e)
#define	 MCONDON			_IO('m', 0x8f)
#define	 MRESETRELTMP			_IO('m', 0xb1)
#define	 MMETOFF			_IO('m', 0x84)
#define	 MMETACNTOFF			_IO('m', 0x83)
#define	 MMETACNTON			_IO('m', 0x85)
#define	 MMEASENDOFF			_IO('m', 0x8c)
#define	 MMEASENDON			_IO('m', 0x8d)
#define	 MCLKINTRN			_IO('m', 0x80)
#define	 MCLKMIDI			_IO('m', 0x82)
#define	 MRES48				_IO('m', 0xc2)
#define	 MRES72				_IO('m', 0xc3)
#define	 MRES96				_IO('m', 0xc4)
#define	 MRES120			_IO('m', 0xc5)
#define	 MRES144			_IO('m', 0xc6)
#define	 MRES168			_IO('m', 0xc7)
#define	 MRES192			_IO('m', 0xc8)
#define	 MCLKPCOFF			_IO('m', 0x94)
#define	 MCLKPCON			_IO('m', 0x95)
#define	 MSYSTHRU			_IO('m', 0x37)
#define	 MTHRUOFF			_IO('m', 0x33)
#define	 MBADTHRU			_IO('m', 0x88)
#define	 MALLTHRU			_IO('m', 0x89)
#define	 MMODEPC			_IO('m', 0x35)
#define	 MCOMMPC			_IO('m', 0x38)
#define	 MRTPC				_IO('m', 0x39)
#define	 MCONTPCOFF			_IO('m', 0x86)
#define	 MCONTPCON			_IO('m', 0x87)
#define	 MSYSPCOFF			_IO('m', 0x96)
#define	 MSYSPCON			_IO('m', 0x97)

#define	 MSELTRKS			_IOW('m', 0xec, unsigned char)
#define	 MSNDPLAYCNT			_IOW('m', 0xed, unsigned char)
#define	 MSELCHN8			_IOW('m', 0xee, unsigned char)
#define	 MSELCHN16			_IOW('m', 0xef, unsigned char)
#define	 MSETBASETMP			_IOW('m', 0xe0, unsigned char)
#define	 MSETRELTMP			_IOW('m', 0xe1, unsigned char)
#define	 MSETRELTMPGRD			_IOW('m', 0xe2, unsigned char)
#define	 MSETMETFRQ			_IOW('m', 0xe4, unsigned char)
#define	 MSETBPM			_IOW('m', 0xe6, unsigned char)
#define	 MCLKPCRATE			_IOW('m', 0xe7, unsigned char)
#define	 MSETREMAP			_IOW('m', 0xe8, unsigned char)

/* requests to send messages */
#define	 MMESTRK0			_IOW('m', 0xd0, MIDI_MESS)
#define	 MMESTRK1			_IOW('m', 0xd1, MIDI_MESS)
#define	 MMESTRK2			_IOW('m', 0xd2, MIDI_MESS)
#define	 MMESTRK3			_IOW('m', 0xd3, MIDI_MESS)
#define	 MMESTRK4			_IOW('m', 0xd4, MIDI_MESS)
#define	 MMESTRK5			_IOW('m', 0xd5, MIDI_MESS)
#define	 MMESTRK6			_IOW('m', 0xd6, MIDI_MESS)
#define	 MMESTRK7			_IOW('m', 0xd7, MIDI_MESS)
#define	 MMESSYS			_IOW('m', 0xdf, MIDI_MESS)

/* requests with responses */
#define	 MREQPC0			_IOR('m', 0xa0, unsigned char)
#define	 MREQPC1			_IOR('m', 0xa1, unsigned char)
#define	 MREQPC2			_IOR('m', 0xa2, unsigned char)
#define	 MREQPC3			_IOR('m', 0xa3, unsigned char)
#define	 MREQPC4			_IOR('m', 0xa4, unsigned char)
#define	 MREQPC5			_IOR('m', 0xa5, unsigned char)
#define	 MREQPC6			_IOR('m', 0xa6, unsigned char)
#define	 MREQPC7			_IOR('m', 0xa7, unsigned char)
#define	 MREQRCCLR			_IOR('m', 0xab, unsigned char)
#define	 MREQVER			_IOR('m', 0xac, unsigned char)
#define	 MREQREV			_IOR('m', 0xad, unsigned char)
#define	 MREQTMP			_IOR('m', 0xaf, unsigned char)

/* not implemented ignored */
#define	 MALLOFF			_IO('m', 0x30)
#define	 MCLKFSK			_IO('m', 0x81)
#define	 MFSKINTRN			_IO('m', 0x92)
#define	 MFSKMIDI			_IO('m', 0x93)
