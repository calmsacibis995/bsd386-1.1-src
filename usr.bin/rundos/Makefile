SUBDIR= rundos kernel

.include "Makefile.inc" 

afterinstall:
	install -c rombios ${DESTDIR}${ROMBIOS_FILE_NAME}

.include <bsd.subdir.mk>
