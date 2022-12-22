#!./perl

# $Header: /bsdi/MASTER/BSDI_OS/contrib/perl/grot/t/op/sprintf.t,v 1.1.1.1 1992/07/27 23:24:45 polk Exp $

print "1..1\n";

$x = sprintf("%3s %-4s%%foo %5d%c%3.1f","hi",123,456,65,3.0999);
if ($x eq ' hi 123 %foo   456A3.1') {print "ok 1\n";} else {print "not ok 1 '$x'\n";}
