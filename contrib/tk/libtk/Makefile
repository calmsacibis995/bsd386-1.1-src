#	BSDI	$Id: Makefile,v 1.1.1.1 1994/01/03 23:15:20 polk Exp $

LIB=	tk
SRCS=	tk3d.c tkArgv.c tkAtom.c tkBind.c tkBitmap.c tkButton.c \
	tkCanvArc.c tkCanvBmap.c tkCanvLine.c tkCanvPoly.c tkCanvPs.c \
	tkCanvText.c tkCanvWind.c tkCanvas.c tkCmds.c tkColor.c \
	tkConfig.c tkCursor.c tkEntry.c tkError.c tkEvent.c tkFocus.c \
	tkFont.c tkFrame.c tkGC.c tkGeometry.c tkGet.c tkGrab.c \
	tkListbox.c tkMain.c tkMenu.c tkMenubutton.c tkMessage.c \
	tkOption.c tkPack.c tkPlace.c tkPreserve.c tkRectOval.c \
	tkScale.c tkScrollbar.c tkSelect.c tkSend.c tkText.c \
	tkTextBTree.c tkTextDisp.c tkTextIndex.c tkTextTag.c tkTrig.c \
	tkWindow.c tkWm.c
MAN3=	3DBorder.0 BackgdErr.0 ClrSelect.0 ConfigWidg.0 ConfigWind.0 \
	CoordToWin.0 CrtErrHdlr.0 CrtGenHdlr.0 CrtMainWin.0 CrtSelHdlr.0 \
	DoOneEvent.0 DoWhenIdle.0 EventHndlr.0 FileHndlr.0 GeomReq.0 \
	GetAnchor.0 GetBitmap.0 GetCapStyl.0 GetColor.0 GetCursor.0 \
	GetFontStr.0 GetGC.0 GetJoinStl.0 GetJustify.0 GetOption.0 \
	GetPixels.0 GetRelief.0 GetRootCrd.0 GetSelect.0 GetUid.0 \
	GetVRoot.0 InternAtom.0 MainWin.0 ManageGeom.0 MapWindow.0 \
	MoveToplev.0 Name.0 OwnSelect.0 ParseArgv.0 Preserve.0 \
	RegInterp.0 Restack.0 RestrictEv.0 SetCModel.0 SetClass.0 \
	SetGrid.0 SetVisual.0 Sleep.0 TimerHndlr.0 WindowId.0
CFLAGS+=-I${.CURDIR} -I${.CURDIR}/../../tcl/libtcl -I/usr/X11/include
CFLAGS+=-DHAVE_UNISTD_H -DTK_LIBRARY=\"/usr/contrib/lib/tk\"

afterinstall:
	install -c -o ${BINOWN} -g ${BINGRP} -m 0444 ${.CURDIR}/tk.h \
		${DESTDIR}/usr/include/

.include "../../Makefile.inc"
.include <bsd.lib.mk>
