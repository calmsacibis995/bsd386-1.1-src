#!./perl

$^I = '.bak';

# $Header: /bsdi/MASTER/BSDI_OS/contrib/perl/grot/t/io/inplace.t,v 1.1.1.1 1992/07/27 23:24:34 polk Exp $

print "1..2\n";

@ARGV = ('.a','.b','.c');
`echo foo | tee .a .b .c`;
while (<>) {
    s/foo/bar/;
}
continue {
    print;
}

if (`cat .a .b .c` eq "bar\nbar\nbar\n") {print "ok 1\n";} else {print "not ok 1\n";}
if (`cat .a.bak .b.bak .c.bak` eq "foo\nfoo\nfoo\n") {print "ok 2\n";} else {print "not ok 2\n";}

unlink '.a', '.b', '.c', '.a.bak', '.b.bak', '.c.bak';
