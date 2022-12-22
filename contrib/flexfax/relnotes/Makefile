#	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/relnotes/Makefile,v 1.1.1.1 1994/01/14 23:10:41 torek Exp $
#
# FlexFAX Facsimile Software
#
# Copyright (c) 1988, 1989, 1990, 1991, 1992, 1993 Sam Leffler
# Copyright (c) 1991, 1992, 1993 Silicon Graphics, Inc.
# 
# Permission to use, copy, modify, distribute, and sell this software and 
# its documentation for any purpose is hereby granted without fee, provided
# that (i) the above copyright notices and this permission notice appear in
# all copies of the software and related documentation, and (ii) the names of
# Sam Leffler and Silicon Graphics may not be used in any advertising or
# publicity relating to the software without the specific, prior written
# permission of Sam Leffler and Silicon Graphics.
# 
# THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
# EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
# WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
# 
# IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
# ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
# OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
# WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
# LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
# OF THIS SOFTWARE.
#
DEPTH=..
include ${DEPTH}/defs

CMM=	\
	ch1.cmm \
	ch2.cmm \
	ch3.cmm \
	ch4.cmm \
	ch5.cmm \
	ch6.cmm \
	${NULL}
AMM	=

IDB_TAG	= ${PRODUCT}.man.relnotes
IDB_PATH = /usr/relnotes/${PRODUCT}

# It should not be necessary to modify anything below this line.  If 
# changes are made, they should be carefully considered, and reflected
# in the master copy on the source repository.

# The following operations are supported:
#
#	make			makes all out-of-date chapters.
#	make contents.out	makes the table of contents, list of tables,
#				and list of figures.
#	make index.out		makes the index.
#	make cover.out		makes the cover sheet.
#	make credit.out		makes the credits page.
#	
#	make book		makes all of the above.
#
#	make <anything>.psv	makes the corresponding .out file, and then
#				psview's it.
#	make ch3.out		makes a specific chapter.
#
#	make clobber		destroy all reproduceable files, so that
#				everything can be rebuilt from scratch.
#
# For the "online" side of the world:
#
#	make all		make all of the packed online chapters and TC
#
#	make install		make and install the online chapters and TC.

OUT	= $(CMM:.cmm=.out) $(AMM:.amm=.out)
Z	= $(CMM:.cmm=.z) $(AMM:.amm=.z)
PSV	= $(OUT:.out=.psv)
CFRONT	= cFront

default: $(Z)

all:	$(Z) TC

install:$(Z) TC
	$(INSTALL) -idb $(IDB_TAG) -F $(IDB_PATH) $(Z) TC

clean clobber:
	${RM} -f *.CKP *.BAK *.out *.aux *.z TC contents

book:	$(OUT) contents.out index.out cover.out credit.out

$(OUT) index.out contents.out:	$(CFRONT)

contents: $(OUT)
	${RM} -f contents
	sed -n -e 's///g' -e '/^TC /s/TC //p' *.aux >> contents
	sed -n -e 's///g' -e '/^FG /s/FG //p' *.aux >> contents
	sed -n -e 's///g' -e '/^TB /s/TB //p' *.aux >> contents

contents.out: contents
	psroff $(PSFLAGS) -t -mm -rx2 $(CFRONT) contents > contents.out

index: $(OUT)
	ixtool *.aux > index

index.out: index
	psroff $(PSFLAGS) -t -mm -rx3 $(CFRONT) index > index.out

# Make the online TC.  The TC.online macros are very simple, and just gather
# up the calls to ".H 1 " to output the current chapter number and title,
# separated by a " (t) ".  We slurp out the pseudo-font changes with col -b,
# and then translate the fake tabs "(t)" into real ones.

TC: $(CMM) $(AMM)
	echo ".H A" | nroff TC.online $(CMM) - $(AMM) | col -b | \
		sed -n 's: (t) :	:p' > TC

# Built-in rules, for turning any ".this" into a ".that".

.SUFFIXES: .psv .p .out .amm .cmm .x .z

# Turn chapter or appendix source into a postcript output file.
#
# The egrep lines collect "everything but" from the aux files, and show them
# on stderr; otherwise error messages would be hidden in the .aux files.

.cmm.out:
	pic $*.cmm | tbl - | eqn | \
		psroff $(PSFLAGS) -t -mm $(CFRONT) - 1>$*.out 2>$*.aux 
	@egrep -v '^(TC|IX|TB|FG|EQ|EX) ' $*.aux 1>&2 || true

.amm.out:
	pic $(CFRONT) $*.amm | \
		tbl - | eqn | psroff $(PSFLAGS) -t -mm -rx1 - 1>$*.out 2>$*.aux 
	@egrep -v '^(TC|IX|TB|FG|EQ|EX) ' $*.aux 1>&2 || true

# turn non-mm source into postscript output files:

.x.out:
	psroff -t > $*.out $*.x

# Show a postscript file right now:

.out.psv:
	psview $(PSVIEWOPTS) $?

# Turn chapter or appendix source into packed nroff files for the online notes:

.cmm.z:
	pic $*.cmm | tbl - | eqn | nroff -mm macros.online - 1> $*
	rm -f $*.z ; pack -f $* || true

.amm.z:
	pic $*.amm | tbl -| eqn | nroff -mm macros.online - 1> $*
	rm -f $*.z ; pack -f $* || true
