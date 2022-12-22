#	@(#)Makefile	5.8.1.1 (Berkeley) 5/8/91
#	BSDI $Id: Makefile,v 1.40 1994/01/29 07:04:17 donn Exp $

# missing:
#	apply dc deroff diction graph
#	learn plot ptx ratfor spline
#	struct xsend 

SUBDIR=	ar arch areacode at awk banner basename bc biff bpatch bsd2dos cal \
	calendar cdctl checknr chflags chpass cksum cmp col colcrt \
	colrm column comm compress crontab ctags cut cvs dc diff \
	dirname dos2bsd du ed env error expand false fdformat file \
	find finger fmt fold fpr from fsplit fstat ftp gcore \
	gdb gprof grep groff groups head hexdump id indent join \
	ktrace last lastcomm ld leave lex line locate lock logger \
	login logname look lorder m4 machine mail make man mesg \
	mkdep mkfifo mklocale mkstr more msgs netstat nfsstat nice nm nohup \
	pagesize passwd paste patch ppp pr printenv printf pstat quota \
	ranlib rcs rdist renice rev rlogin rpcgen rpcinfo rsh rstat \
	rundos ruptime rwho sccs script sed shar showmount size soelim \
	sort spell split strings strip su symorder tail talk tcopy \
	tee telnet tftp time tip tn3270 touch tput tr true tset tsort tty \
	ul uname unexpand unifdef uniq units unvis users uucp uudecode \
	uuencode vacation vgrind vi vis vmstat w wall wc what \
	whereis which who whoami whois window write xargs xinstall \
	xstr yacc yes

.if	${MACHINE} == "hp300"
SUBDIR+=gas gcc
.elif	${MACHINE} == "i386"
SUBDIR+=doscmd gas gcc gcc2 vgafont vreset
.elif	${MACHINE} == "tahoe"
SUBDIR+=pascal
.elif	${MACHINE} == "vax"
SUBDIR+=gas pascal
.endif

afterinstall:
	rm -f ${DESTDIR}/usr/ucb
	ln -s bin ${DESTDIR}/usr/ucb

.include <bsd.subdir.mk>
