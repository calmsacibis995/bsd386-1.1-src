PROG=	te
SRCS=	te_data.c te_exec0.c te_exec1.c te_exec2.c te_main.c te_rdcmd.c \
	te_srch.c te_subs.c te_utils.c te_window.c te_chario.c
LDADD+= -ltermcap
DPADD+=	${LIBTERMCAP}
MLINKS+=te.1 teco.1

.include <bsd.prog.mk>
