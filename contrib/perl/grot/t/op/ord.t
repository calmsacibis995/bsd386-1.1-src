#!./perl

# $Header: /bsdi/MASTER/BSDI_OS/contrib/perl/grot/t/op/ord.t,v 1.1.1.1 1992/07/27 23:24:42 polk Exp $

print "1..2\n";

# compile time evaluation

if (ord('A') == 65) {print "ok 1\n";} else {print "not ok 1\n";}

# run time evaluation

$x = 'ABC';
if (ord($x) == 65) {print "ok 2\n";} else {print "not ok 2\n";}
