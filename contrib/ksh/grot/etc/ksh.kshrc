:
# NAME:
#	ksh.kshrc - global initialization for ksh 
#
# DESCRIPTION:
#	Each invocation of /bin/ksh processes the file pointed
#	to by $ENV (usually $HOME/.kshrc).
#	This file is intended as a global .kshrc file for the
#	Korn shell.  A user's $HOME/.kshrc file simply requires
#	the line:
#		. /etc/ksh.kshrc
#	at or near the start to pick up the defaults in this
#	file which can then be overridden as desired.
#
# SEE ALSO:
#	$HOME/.kshrc
#

# RCSid:
#	$Id: ksh.kshrc,v 1.2 1993/12/21 02:00:24 polk Exp $
#
#	@(#)Copyright (c) 1991 Simon J. Gerraty
#
#	This file is provided in the hope that it will
#	be of use.  There is absolutely NO WARRANTY.
#	Permission to copy, redistribute or otherwise
#	use this file is hereby granted provided that 
#	the above copyright notice and this notice are
#	left intact. 

case "$-" in
*i*)	# we are interactive
	# we may have su'ed so reset these
	# NOTE: SCO-UNIX doesn't have whoami,
	#	install whoami.sh
	USER=`whoami`
	PROMPT="<$USER@$HOSTNAME:!>$ "
	PPROMPT='<$USER@$HOSTNAME:$PWD:!>$ '
	PS1=$PPROMPT
	# $TTY is the tty we logged in on,
	# $tty is that which we are in now (might by pty)
	tty=`tty`
	tty=`basename $tty`

	set -o $EDITOR

	alias ls='ls -CF'
	alias h='fc -l | more'
	# the PD ksh is not 100% compatible
	case "$KSH_VERSION" in
	*PD*)	# PD ksh
		case "$TERM" in
		xterm*)
			# bind arrow keys
			bind '^[['=prefix-2
			bind '^XA'=up-history
			bind '^XB'=down-history
			bind '^XC'=forward-char
			bind '^XD'=backward-char
			;;
		esac
		;;
	*)	# real ksh ?
		;;
	esac
	case "$TERM" in
	sun*)
		# these are not as neat as their csh equivalents
		if [ "$tty" != console ]; then
			# ilabel
			ILS='\033]L'; ILE='\033\\'
			# window title bar
			WLS='\033]l'; WLE='\033\\'
		fi
		;;
	xterm*)
		ILS='\033]1;'; ILE='\007'
		WLS='\033]2;xterm: '; WLE='\007'
		;;
	*)	;;
	esac
	# do we want window decorations?
	if [ "$ILS" ]; then
		ilabel () { print -n "${ILS}$*${ILE}"; }
		label () { print -n "${WLS}$*${WLE}"; }

		alias stripe='label "$USER@$HOST ($tty) - $PWD"'
		alias istripe='ilabel "$USER@$HOST ($tty)"'

		wftp () { ilabel "ftp $*"; "ftp" $*; eval istripe; }
		wcd () { "cd" $*; eval stripe; }
		wtelnet ()
		{
			"telnet" "$@"
			eval istripe
			eval stripe
		}
		wsu ()
		{
			"su" "$@"
			eval istripe
			eval stripe
		}
		alias su=wsu
		alias cd=wcd
		alias ftp=wftp
		alias telnet=wtelnet
		eval stripe
		eval istripe
		PS1=$PROMPT
	fi
	alias quit=exit
	alias cls=clear
	alias logout=exit
	alias bye=exit
	alias p='ps -l'
	alias j=jobs
	alias o='fg %-'


# add your favourite aliases here
;;
*)	# non-interactive
;;
esac
# commands for both interactive and non-interactive shells
