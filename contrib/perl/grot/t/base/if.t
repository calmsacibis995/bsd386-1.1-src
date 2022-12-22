#!./perl

# $Header: /bsdi/MASTER/BSDI_OS/contrib/perl/grot/t/base/if.t,v 1.1.1.1 1992/07/27 23:24:31 polk Exp $

print "1..2\n";

# first test to see if we can run the tests.

$x = 'test';
if ($x eq $x) { print "ok 1\n"; } else { print "not ok 1\n";}
if ($x ne $x) { print "not ok 2\n"; } else { print "ok 2\n";}
