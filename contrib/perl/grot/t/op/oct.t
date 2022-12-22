#!./perl

# $Header: /bsdi/MASTER/BSDI_OS/contrib/perl/grot/t/op/oct.t,v 1.1.1.1 1992/07/27 23:24:42 polk Exp $

print "1..3\n";

if (oct('01234') == 01234) {print "ok 1\n";} else {print "not ok 1\n";}
if (oct('0x1234') == 0x1234) {print "ok 2\n";} else {print "not ok 2\n";}
if (hex('01234') == 0x1234) {print "ok 3\n";} else {print "not ok 3\n";}
