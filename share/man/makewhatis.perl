#!/usr/bin/perl
#
#	BSDI	$Id: makewhatis.perl,v 1.3 1993/12/17 03:45:41 sanders Exp $
#
# Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
# The Berkeley Software Design Inc. software License Agreement specifies
# the terms and conditions for redistribution.
#
# Usage: find /usr/share/man -type f -name '*.0*' -print |
#            makewhatis.perl | sort -u > /usr/share/man/whatis.db
#
# or: makewhatis.perl manpage ...
#
if ($#ARGV < 0) {
    while (chop($file = <stdin>)) { &makewhatis($file); }
} else {
    for $file (@ARGV) { &makewhatis($file); }
}
exit 0;

sub makewhatis
{
    local($file) = @_;
    local($line) = "";
    local($section, $name, $desc);

    unless (&sopen(IN, $file)) { warn "Can't open $file\n"; return; }

    while (<IN>) { last if ($section) = /\(([^)]*)/; }
    if (eof(IN)) { warn "No section header in $file\n"; close(IN); return; }

    while (<IN>) { s/.[\b]//g; last if /^NAME/; }
    if (eof(IN)) { warn "No description in $file\n"; close(IN); return; }

    while (<IN>) {				# join text until blank line
	chop; s/^\s*//; last if /^$/;
	s/.[\b]//g; $line .= $_ . " ";
    }
    close(IN);

    ($name, $desc) = split(/ -/, $line, 2);
    $name =~ s/^\s*//; $name =~ s/\s+/ /g; $name =~ s/\s*$//;
    $desc =~ s/^\s*//; $desc =~ s/\s+/ /g; $desc =~ s/\s*$//;
    $desc =~ s/- //g;

    warn "Null name in $file\n" if ($name eq "");
    warn "Null section in $file\n" if ($section eq "");
    warn "Null description in $file\n" if ($desc eq "");

    printf("%s (%s) - %s\n", $name, $section, $desc);
}

sub sopen {
    local($fh, $file) = @_;
    $file =~ s#^\s#./$&#;	# protect leading spaces

    $file =~ m/\.Z$/ && return &popen($fh, "/usr/bin/zcat", $file);
    $file =~ m/\.gz$/ && return &popen($fh, "/usr/contrib/bin/gunzip", "-c", $file);
    $file =~ m/\.z$/ && return &popen($fh, "/usr/contrib/bin/gunzip", "-c", $file);
    return open($fh, "< $file\0");
}

sub popen {
    local($fh, @cmd) = @_;
    open($fh, "-|") || exec @cmd;
}
