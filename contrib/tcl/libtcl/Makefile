#	BSDI	$Id: Makefile,v 1.1.1.1 1994/01/03 23:14:43 polk Exp $

LIB=	tcl
SRCS=	panic.c regexp.c tclAsync.c tclBasic.c tclCkalloc.c tclCmdAH.c \
	tclCmdIL.c tclCmdMZ.c tclEnv.c tclExpr.c tclGet.c tclGlob.c \
	tclHash.c tclHistory.c tclLink.c tclMain.c tclParse.c tclProc.c \
	tclUnixAZ.c tclUnixStr.c tclUnixUtil.c tclUtil.c tclVar.c
MAN3=	AddErrInfo.0 AppInit.0 Async.0 Backslash.0 CallDel.0 CmdCmplt.0 \
	Concat.0 CrtCommand.0 CrtInterp.0 CrtMathFnc.0 CrtPipelin.0 \
	CrtTrace.0 DString.0 DetachPids.0 EnterFile.0 Eval.0 ExprLong.0 \
	GetInt.0 Hash.0 Interp.0 LinkVar.0 PrintDbl.0 RecordEval.0 \
	RegExp.0 SetRecLmt.0 SetResult.0 SetVar.0 SplitList.0 StrMatch.0 \
	TildeSubst.0 TraceVar.0
CFLAGS+=-I${.CURDIR} -DHAVE_UNISTD_H -DTCL_LIBRARY=\"/usr/contrib/lib/tcl\"

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 0444 ${.CURDIR}/tcl.h \
		${DESTDIR}/usr/include/

.include "../../Makefile.inc"
.include <bsd.lib.mk>
