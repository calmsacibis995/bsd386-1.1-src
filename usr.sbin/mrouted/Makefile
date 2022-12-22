#	BSDI $Id: Makefile,v 1.1 1994/01/05 02:03:19 torek Exp $

SUBDIR=	mrinfo mrouted

# map-mbone is not normally installed (??)
.if !make(install)
SUBDIR+=map-mbone
.endif

.include <bsd.subdir.mk>
