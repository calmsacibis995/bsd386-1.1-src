#	BSDI $Id: Makefile,v 1.8 1994/01/31 04:00:23 polk Exp $
#	@(#)Makefile	5.13.1.1 (Berkeley) 5/7/91

MAN4=	appp.0 bpf.0 cisco_hdlc.0 clnp.0 cltp.0 drum.0 esis.0 fd.0 icmp.0 \
	idp.0 imp.0 inet.0 ip.0 iso.0 iso9660.0 kadb.0 lo.0 msdosfs.0 \
	netintro.0 ns.0 nsip.0 null.0 ppp.0 pty.0 route.0 sd.0 st.0 spp.0 \
	tcp.0 termios.0 tp.0 tty.0 udp.0 unix.0
MLINKS=	netintro.4 networking.4 msdosfs.4 msdos.4

# SUBDIR=	man4.hp300 man4.tahoe man4.vax man4.i386
SUBDIR= man4.${MACHINE}

.include <bsd.prog.mk>
