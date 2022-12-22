PROG=	dvips
SRCS=	dospecial.c dviinput.c fontdef.c loadfont.c dvips.c tfmload.c \
	download.c prescan.c scanpage.c skippage.c output.c scalewidth.c \
	dosection.c dopage.c resident.c search.c unpack.c drawPS.c \
	header.c makefont.c repack.c virtualfont.c dpicheck.c finclude.c \
	pprescan.c papersiz.c flib.c color.c bbox.c \
	concat.c concat3.c dir-p.c file-p.c pathsrch.c xmalloc.c \
	xopendir.c xrealloc.c xstat.c xstrdup.c

NOMAN=

# Add -DDEFRES=dpi to DEFS to change the default resolution from 300dpi.
# Add -DDEBUG to turn on debugging.
# Add -DTPIC for tpic support.
# Add -DFONTLIB to search font libraries.
# Add -DEMTEX for EMTEX specials.
# Add -DSECURE if you will install dvips with special privileges.
# 
# For VM/CMS, need -DVMCMS; see the vmcms/mkdvips.exec.
# For VMS, need -DVMS; see the vms subdirectory.
# For DOS, need -DDOS; see the pc subdirectory.
# For Xenix, need -DXENIX; sorry, configure should figure that out.
# 
DEFS = -DTPIC -DDEBUG #-DDEFRES=400

# Perhaps you will want to optimize or use -Wall here.
CFLAGS+= -I${.CURDIR}/../include ${DEFS}  -g
LDADD+=	-lm

.include <bsd.prog.mk>
