# @(#) $Header: /bsdi/MASTER/BSDI_OS/usr.sbin/tcpdump/grot/ostype.awk,v 1.1.1.1 1993/03/08 17:46:15 polk Exp $ (LBL)

BEGIN {
	os = "UNKNOWN"
}

$0 ~ /^Sun.* Release 3\./ {
	os = "sunos3"
}

$0 ~ /^SunOS.* Release 4\./ {
	os = "sunos4"
}

$0 ~ /^4.[1-9]\ ?BSD / {
	os = "bsd"
}

$0 ~ /^BSDI / {
	os = "bsd"
}

# XXX need an example Ultrix motd
$0 ~ /[Uu][Ll][Tt][Rr][Ii][Xx]/ {
	os = "ultrix"
}

END {
	print os
}
