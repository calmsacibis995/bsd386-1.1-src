#	BSDI	$Id: Makefile,v 1.2 1994/01/06 02:16:07 polk Exp $
#	@(#)Makefile	5.3 (Berkeley) 4/1/91

LIB=	des
CFLAGS+=-DKERBEROS -I. -I${.CURDIR}/../include
SRCS=	cbc_encrypt.c cksum.c debug_decl.c des.c key_parity.c \
	key_sched.c new_rnd_key.c pcbc_encrypt.c quad_cksum.c \
	random_key.c read_password.c string_to_key.c util.c weak_key.c
CLEANFILES+=key_perm.h s_table.h ip.c p.c fp.c odd.h p_table.h
NOPROFILE=noprofile

TDIR=	${.CURDIR}/..
key_perm.h:
	cd ${TDIR}/make_key_perm; make
	${TDIR}/make_key_perm/obj/make_key_perm key_perm.h

p_table.h:
	cd ${TDIR}/make_p_table; make
	${TDIR}/make_p_table/obj/make_p_table p_table.h

s_table.h:
	cd ${TDIR}/make_s_table; make
	${TDIR}/make_s_table/obj/make_s_table s_table.h

ip.c:
	cd ${TDIR}/make_ip; make
	${TDIR}/make_ip/obj/make_ip ip.c

p.c:
	cd ${TDIR}/make_p; make
	${TDIR}/make_p/obj/make_p p.c

fp.c:
	cd ${TDIR}/make_fp; make
	${TDIR}/make_fp/obj/make_fp fp.c

odd.h:
	cd ${TDIR}/make_odd; make
	${TDIR}/make_odd/obj/make_odd odd.h

.include <bsd.lib.mk>

${OBJS} ${POBJS} .depend: key_perm.h p_table.h s_table.h ip.c p.c fp.c odd.h
