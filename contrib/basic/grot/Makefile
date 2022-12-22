DEST = /usr/local/bin/basica
# sigset below for 4.2
CFLAGS = # -O2
FFLAGS = # -O2

SRCS =	parser.y expr.c mat.c run.c stmt.c temps.c lex.c \
	lines.c linpack.f linpackc.c main.c misc.c strings.c \
	symbols.c trees.c typecheck.c ops.c renum.c globals.c

OBJS =	parser.o expr.o mat.o run.o stmt.o temps.o lex.o lines.o \
	linpack.o linpackc.o main.o misc.o strings.o symbols.o \
	trees.o typecheck.o ops.o renum.o globals.o

basica:	${OBJS}
	cc -o ./basica ${CFLAGS} ${OBJS} -lF77 -lm -lI77  # -ljobs 

install:	basica
	cp ./basica ${DEST}
	@ls -l ${DEST}
	@chmod a+x ${DEST}
	-cp basica.l /usr/man/manl/basica.l
	@-chmod a+r /usr/man/manl/basica.l

manual:
	nitroff -man basica.l

tape:
	make clean
	tar cr *

parser.o:	structs.h

parser.c:	parser.y
	yacc -d $*.y
	sed -f $*.sed <y.tab.c >$*.c && rm y.tab.c
	sed -f parser.sed <y.tab.h > y.tab.h.new
	update y.tab.h.new tokens.h

ci:
	ci -l ${SRCS} makefile basica.l makefile parser.sed update

clean:
	rm -f parser.c ${OBJS} basica

lint:
	@lint -h `ls ${SRCS} | fgrep -v linpack \
		| sed 's/\.y/.c/'` | fgrep -v 'alignment problem'

fwc:
	@wc -l ${SRCS} | \
	 awk '{ printf "%4d %s\n", $$1, substr($$2, index($$2, "/")+1); \
			if ($$0 !~ /total/) ct += $$1 }\
	     END { printf "\n%4d average\n", ct/(NR-1);\
		   printf"%4d files\n", NR-1 }' | pr -4 -l1 -t

wc:
	@wc -l `ls ${SRCS} | fgrep -v linpack` | \
	 awk '{ printf "%4d %s\n", $$1, substr($$2, index($$2, "/")+1); \
			if ($$0 !~ /total/) ct += $$1 }\
	     END { printf "\n%4d average\n", ct/(NR-1);\
		   printf"%4d files\n", NR-1 }' | pr -4 -l1 -t

tags:	${SRCS}
	@ctags ${SRCS}

depend:
	@grep '^#[ 	]*include[ 	]*"' ${SRCS} *.h | \
	      sed -e 's/:[^"]*"\([^"]*\)".*/: \1/' \
	          -e 's/\.c/.o/' \
		  -e 's,../[a-z]*/,,' | \
	awk ' { if ($$1 != prev) { print rec; rec = $$0; prev = $$1; } \
		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
		       else rec = rec " " $$2 } } \
	      END { print rec } ' > makedep
	@echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	@echo '$$r makedep' >>eddep
	@echo 'w' >>eddep
	@cp makefile makefile.bak
	@ed - makefile < eddep
	@rm eddep makedep
	@echo '# DEPENDENCIES MUST END AT END OF FILE' >> makefile
	@echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> makefile
	@echo '# see make depend above' >> makefile


# DO NOT DELETE THIS LINE -- make depend uses it


parser.y: structs.h
expr.o: structs.h
mat.o: structs.h
run.o: structs.h
stmt.o: structs.h
temps.o: structs.h
lex.o: structs.h tokens.h
lines.o: structs.h
main.o: structs.h
misc.o: structs.h
strings.o: structs.h
symbols.o: structs.h
trees.o: structs.h
typecheck.o: structs.h
ops.o: structs.h
renum.o: structs.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
