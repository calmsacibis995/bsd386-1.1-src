#!./perl

# $Header: /bsdi/MASTER/BSDI_OS/contrib/perl/grot/t/op/exec.t,v 1.1.1.1 1992/07/27 23:24:40 polk Exp $

$| = 1;				# flush stdout
print "1..8\n";

print "not ok 1\n" if system "echo ok \\1";	# shell interpreted
print "not ok 2\n" if system "echo ok 2";	# split and directly called
print "not ok 3\n" if system "echo", "ok", "3"; # directly called

if (system "true") {print "not ok 4\n";} else {print "ok 4\n";}

if ((system "/bin/sh -c 'exit 1'") != 256) { print "not "; }
print "ok 5\n";

if ((system "lskdfj") == 255 << 8) {print "ok 6\n";} else {print "not ok 6\n";}

unless (exec "lskdjfalksdjfdjfkls") {print "ok 7\n";} else {print "not ok 7\n";}

exec "echo","ok","8";
