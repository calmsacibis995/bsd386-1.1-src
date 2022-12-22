#!./perl

# $Header: /bsdi/MASTER/BSDI_OS/contrib/perl/grot/t/base/pat.t,v 1.1.1.1 1992/07/27 23:24:31 polk Exp $

print "1..2\n";

# first test to see if we can run the tests.

$_ = 'test';
if (/^test/) { print "ok 1\n"; } else { print "not ok 1\n";}
if (/^foo/) { print "not ok 2\n"; } else { print "ok 2\n";}
