#!./perl

# $Header: /bsdi/MASTER/BSDI_OS/contrib/perl/grot/t/op/sleep.t,v 1.1.1.1 1992/07/27 23:24:44 polk Exp $

print "1..1\n";

$x = sleep 2;
if ($x >= 2 && $x <= 10) {print "ok 1\n";} else {print "not ok 1 $x\n";}
