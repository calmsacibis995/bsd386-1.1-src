#!/usr/bin/perl
'di';
'ig00';
;#
;# sdif: sdiff clone
;#
;# Copyright (c) 1992 Kazumasa Utashiro <utashiro@sra.co.jp>
;# Software Research Associates, Inc., Japan
;#
;# Original version on Jul 24 1991
;; $rcsid = 'Id: sdif,v 1.14 1992/12/11 01:27:56 utashiro Exp ';
;#
;# Incompatibility:
;#	- has -n option for numbering
;#	- has -f option for folding
;#	- has -F option for word boundary folding
;#	- has -c option for context diff
;#	- has -m option to keep diff mark
;#	- handle backspace and caridge return
;#	- no -o option
;#	- all tabs are expanded
;#	- default width is 80
;#	- rcs support
;#	- cdif (word context diff) support
;#	- read diff data from file or stdin
;#
$myname = ('sdif', split('/', $0));
$usage = <<_;
Usage: $myname [-n] [-l] [-s] [-[fF]] [-b] [-i] [-w] [-w #] [-c] [file1 [file2]]
       $myname [-rcs] [-q] [-rrev1 [-rrev2]] [$myname options] file
Options:
	-n	print line number
	-l	print left column only for identical line
	-s	don't print identical line
	-f	fold line instead of truncating
		(folded line is marked by '+' character)
	-F	fold line by word boundaries
	-c[#]	do context diff
	-b	ignore trailing blanks
	-i	ignore the case of letters
	-w	ignore all blanks
	-w #	specify width of output (default is 80)
	-m	leave diff marks
	-cdif	use "cdif"
	-rcs	compare rcs files
		(not required when revision number is supplied)
	-rrev	RCS revision number
	-q	don't print rcs diagnostics
_

$opts = 'nlsfhqdxmF'.'cbiwuB';
while ($_ = $ARGV[0], /^-/ && shift) {
    next unless ($car, $cdr) = /^-?(.)(.*)/;
    if (/-rcs$/) { $rcs++; next; }
    if (/-cdif$/) { $diff = 'cdif'; next; }
    if (/-diff=(.+)$/) { $diff = $1; next; }
    if ($car eq 'h') { &usage; next; }
    if ($car eq 'H') { exec "nroff -man $0|".($ENV{'PAGER'}||'more')." -s"; }
    if ($car eq 'r') {
	&usage("$_: Too many revisions\n") if (@rcs == 2);
	$rcs++; push(@rcs, $_); next;
    }
    if ($car eq 'w') {
	if ($cdr =~ /^\d+$/) { $screen_width = $cdr; next; }
	if ($ARGV[$[] =~ /^\d+$/) { $screen_width = shift; next; }
    }
    if (s/^-?([cu]\d+)//) {
	push(@diffopts, "-$1"); eval "\$opt_$car++"; redo;
    }
    push(@diffopts, "-$car") if index("cbiwuB", $car) >= $[;
    if (index($opts, "$car:") >= $[) {
	eval "\$opt_$car = length(\$cdr) ? \$cdr : \@ARGV ? shift : &usage";
	next;
    }
    if (index($opts, $car) >= $[) {
	eval "\$opt_$car++"; $_ = $cdr; redo;
    }
    &usage("Unknown option: $car\n\n");
}

$opt_f = 1 if $opt_F;
push(@rcsopts, '-q') if $opt_q;
$diff = $rcs ? 'rcsdiff' : 'diff' unless $diff;
$diff .= ' -rcs' if $rcs && $diff eq 'cdif';

if ($rcs) {
    $rcsfile = shift || &usage("No RCS filename\n\n");
    $DIFF = "$diff @diffopts @rcsopts @rcs $rcsfile |";
    $OLD = "co @rcsopts -p " . $rcs[0] . " $rcsfile|";
    $NEW = @rcs > 1 ? "co @rcsopts -p " . $rcs[1] . " $rcsfile|" : $rcsfile;
} elsif (@ARGV == 2) {
    ($OLD, $NEW) = @ARGV;
    $DIFF = "$diff @diffopts $OLD $NEW |";
} elsif (@ARGV < 2) {
    $DIFF = shift || '-';
    $opt_s++;
} else {
    &usage("Arguments error.\n\n");
}
$readfile = $OLD && $NEW && !($opt_s || $opt_c || $opt_u);

sub usage {
    select(STDERR);
    print @_, $usage;
    print "$rcsid\n" if $rcsid =~ /:/;
    exit;
}

$screen_width = &getwidth unless defined($screen_width);
$width = int(($screen_width - 5) / 2);

if ($opt_d) {
    print STDERR "\$OLD = $OLD\n";
    print STDERR "\$NEW = $NEW\n";
    print STDERR "\$DIFF = $DIFF\n";
}

if ($readfile) {
    open(OLD) || die "$OLD: $!\n";
    open(NEW) || die "$NEW: $!\n";
}
open(DIFF) || die "cannot open diff: $!\n";

$nformat = '%-4d %s';
for (0..7) { $tab[$_] = ' ' x (8 - $_); }
$markwidth = 2;
$oline = $nline = 1;

DIFF: while (<DIFF>) {
    @old = @new = ();
    #
    # normal diff
    #
    if (($left, $ctrl, $right) = /([\d,]+)([adc])([\d,]+)/) {
	$markwidth = 2;
	($l1, $l2) = &range($left);
	($r1, $r2) = &range($right);
	if ($readfile) {
	    $identical_line = $l1 - $oline + 1 - ($ctrl ne 'a');
	    &print_identical($identical_line);
	}
	print if $opt_d || $opt_s;
	if ($ctrl eq 'd' || $ctrl eq 'c') {
	    $oline = $left;
	    @old = &read(DIFF, $n = $l2 - $l1 + 1);
	    &read(OLD, $n) if $readfile;
	}
	&read(DIFF, 1) if $ctrl eq 'c';
	if ($ctrl eq 'a' || $ctrl eq 'c') {
	    $nline = $right;
	    @new = &read(DIFF, $n = $r2 - $r1 + 1);
	    &read(NEW, $n) if $readfile;
	}
	&flush;
    }
    #
    # context diff
    #
    elsif (/^\*\*\* /) {
	&out($_, ' ', scalar(<DIFF>), 1);
	$markwidth = 2;
    }
    elsif ($cdif = ($_ eq "***************\n")) {
	&out($_, ' ', $_, 1);
	$ohead = $_ = <DIFF>;
	print, next unless ($left) = /^\*\*\* ([\d,]+) \*\*\*\*$/;
	$nhead = $_ = <DIFF>;
	unless (($right) = /^--- ([\d,]+) ----$/) {
	    @old = &read(DIFF, &range($left) - 1, $nhead);
	    $nhead = $_ = <DIFF>;
	    unless (($right) = /^--- ([\d,]+) ----$/) {
		print $ohead, @old, $_; next;
	    }
	}
	unless ($redo = (($_ = <DIFF>) !~ /^[\+\-\! ]/)) {
	    @new = &read(DIFF, scalar(&range($right)) - 1, $_);
	}
	&out($ohead, ' ', $nhead, 1) unless $opt_n;
	($oline, $nline) = ($left, $right);
	&flush;
	redo if $redo;
    }
    #
    # unified diff
    #
    elsif (/^--- /) {
	&out($_, ' ', scalar(<DIFF>), 1);
    }
    elsif (($oline, $nline) = /^\@\@ -(\d+),\d+ \+(\d+),\d+ \@\@$/) {
	$markwidth = 1;
	&out($_, ' ', $_, 1);
	while (<DIFF>) {
	    push(@old, $_), next if /^\-/;
	    push(@new, $_), next if /^\+/;
	    &flush();
	    redo DIFF unless /^ /;
	    s/^.// unless $opt_m;
	    &out($_, ' ', $_);
	    $oline++; $nline++;
	}
	&flush;
    }
    else {
	warn "Unrecognizable line -- $_";
    }
}
close(DIFF);
$exit = $? >> 8;
if ($readfile) {
    &print_identical(-1) if $exit < 2;
    close(OLD);
    close(NEW);
}
exit($exit == 2);

######################################################################

sub flush {
    while (@old || @new) {
	$old = $new = undef;
	if    ($cdif && $old[0] =~ /^\-/) { $old = shift(@old); }
	elsif ($cdif && $new[0] =~ /^\+/) { $new = shift(@new); }
	else {
	    $old = shift(@old) if $old[0] =~ /^[\-\!\<]/;
	    $new = shift(@new) if $new[0] =~ /^[\+\!\>]/;
	}
	if ($old || $new) {
	    $mark = $old ? $new ? '|' : '<' : '>';
	} else {
	    ($old, $new) = (shift(@old), shift(@new));
	    ($old, $new) = ($old || $new, $new || $old);
	    $mark = ' ';
	}
	unless ($opt_m) {
	    substr($old, 0, $markwidth) = '' if $old;
	    substr($new, 0, $markwidth) = '' if $new;
	}
	&out($old, $mark, $new);
	$oline += defined($old);
	$nline += defined($new);
    }
}

sub print_identical {
    local($n, $old, $new) = @_;
    while ($n--) {
	$old = <OLD>; $new = <NEW>;
	last if !defined($old) && !defined($new);
	$old =~ s/^/  /, $new =~ s/^/  / if $opt_m;
	if ($opt_l) {
	    $old = &num($oline, &expand($old)) if $opt_n;
	    print $old;
	} else {
	    &out($old, ' ', $new);
	}
	$oline++; $nline++;
    }
}

sub out {
    local($old, $mark, $new, $noline) = @_;
    local($o, $n);
    local($ocont, $ncont, $contmark) = (' ', ' ', '+');
    if (defined($old)) {
	($old = &expand($old)) =~ s/\n$//;
	$old = &num($oline, $old) if $opt_n && !$noline;
    }
    if (defined($new)) {
	($new = &expand($new)) =~ s/\n$//;
	$new = &num($nline, $new) if $opt_n && !$noline;
    }
    while (1) {
	($o, $old) = &fold($old, $width, 1, $opt_F);
	($n, $new) = &fold($new, $width, 0, $opt_F);
	print $o, ' ', $ocont, $mark, $ncont, ' ', $n, "\n";
	last if !$opt_f || ($old eq '' && $new eq '');
	if ($opt_n) {
	    $old = ' ' x 5 . $old if length($old);
	    $new = ' ' x 5 . $new if length($new);
	} else {
	    $ocont = length($old) ? $contmark : ' ';
	    $ncont = length($new) ? $contmark : ' ';
	}
    }
}

sub fold {
    local($_, $width, $pad, $byword) = @_;
    local($l, $room) = ('', $width);
    local($n, $c, $r, $mb);
    while (length) {
	if (s/^\010//) {
	    $room < $width && $room++; $c = $&; next;
	}
	if (s/^\r//) {
	    $c = "\010" x ($width - $room - $opt_n * 5);
	    $room = $width - $opt_n * 5;
	    next;
	}
	last if $room == 0 || (/^[\200-\377]/ && $room < 2);
	if (($mb = s/^([\200-\377].)+//) || s/[^\b\r\200-\377]+//) {
	    $n = $room; $n -= $room % 2 if $mb;
	    ($c, $r) = unpack("a$n a*", $&);
	    $room -= length($c);
	    $_ = $r . $_;
	} else {
	    die "$myname: panic";
	}
    } continue {
	$l .= $c;
    }
    if ($byword && /^\w/ && !$mb && $l =~ s/([^\w\b])([\w\b]+)$/\1/) {
	$cut = $2;
	if ($l =~ /[\200-\377]$/) { # This check is not necessary for EUC
	    local(@tmp) = $l =~ /[\200-\377]?./g;
	    pop(@tmp) =~ /^[\200-\377]$/ && $cut =~ s/^.// && $l .= $&;
	}
	$_ = $cut . $_;
	$room += &pwidth($cut) if $pad;
    }
    $l .= ' ' x $room if $pad;
    ($l, $_);
}

sub read {
    local($FH, $c, @buf) = (@_);
    push(@buf, scalar(<$FH>)) while $c--;
    wantarray ? @buf : join('', @buf);
}

sub range {
    local($_, $from, $to) = @_;
    ($from, $to) = /,/ ? split(/,/) : ($_, $_);
    wantarray ? ($from, $to) : $to - $from + 1;
}

sub num {
    local($num, $_) = @_;
    sprintf($nformat, $num, $_);
}

sub expand {
    local($_, $t) = @_;
    if (($test || (($test = '10') =~ s/0/$`/, $test)) eq '11') {
	1 while s/\t/$tab[&pwidth($`) % 8]/e;
    } else {
	substr($_, $t, 1) = $tab[$t % 8] while(($t = index($_, "\t")) >= $[);
    }
    $_;
}

sub pwidth {
    local($_) = @_;
    substr($_, 0, $markwidth) = '' if $opt_m;
    return(length) unless /[\033\010\f\r]/;
    s/^.*[\f\r]//;
    s/\033\$[\@B]|\033\([JB]//g;
    1 while s/[^\010]\010//;
    s/^\010+//;
    length($_);
}

# taken from Tom Christiansen's checknews script
sub getwidth {
    local($rows, $cols, $xpixel, $ypixel);
    $TIOCGWINSZ = 0x40087468;  # should be require sys/ioctl.pl
    if (ioctl(STDERR, $TIOCGWINSZ, $winsize)) {
	($rows, $cols, $xpixel, $ypixel) = unpack('S4', $winsize);
    }
    $cols = 80 unless $cols;
    $cols - 1;
}

sub max { $_[ ($_[$[] < $_[$[+1]) + $[ ]; }
sub min { $_[ ($_[$[] > $_[$[+1]) + $[ ]; }

######################################################################
.00;			# finish .ig

'di			\" finish diversion--previous line must be blank
.nr nl 0-1		\" fake up transition to first page again
.nr % 0			\" start at page 1
'; __END__ ############# From here on it's a standard manual page ####
.de XX
.ds XX \\$4\ (v\\$3)
..
.XX $Id: sdif.sh,v 1.1.1.1 1993/03/07 18:57:22 sanders Exp $
.TH SDIF 1 \*(XX
.SH NAME
sdif \- sdiff clone
.SH SYNOPSIS
.nr ww \w'\fBsdif\fP\ '
.in +\n(wwu
.ta \n(wwu
.ti -\n(wwu
\fBsdif\fP	\c
[\-n] [\-l] [\-s] [\-[fF]] [\-b] [\-i] [\-w] [\-w \fIwidth\fP]
[\-c] [file1 [file2]]
.br
.ti -\n(wwu
\fBsdif\fP	\c
[\-rcs] [\-q] [\-r\fIrev1\fP [\-r\fIrev2\fP]] [\fIsdif options\fP] file
.SH DESCRIPTION
.I Sdif
is a clone program of System V \fIsdiff\fP(1) command.  The
basic feature of sdif and sdiff is making a side-by-side
listing of two different files.  It makes much easier to
compare two files than looking at the normal diff output.
All contents of two files a listed on left and right sides.
Center column is used to indicate how different the line.
No mark means there is no difference.  The line only
included in left file is indicated by `<' mark, and `>' is
used to lines only in right file.  Modified line has a mark
`|'.  Example output from sdif is like this:
.nf
.cs R 25
.ft CW

	1    deleted       <
	2    same             1    same
	3    changed       |  2    modified
	4    same             3    same
	                   >  4    added

.ft R
.cs R
.fi
.PP
.I Sdif
has some incompatibilities with sdiff.  Negative
incompatibility is a lack of \-o option and expanding all
tabs to spaces.  Other incompatibilities are:
.nf

	\(bu line numbering
	\(bu folding a long line
	\(bu context diff support
	\(bu unified diff support
	\(bu option to keep diff mark
	\(bu handle backspace and caridge return
	\(bu default width is 80
	\(bu rcs support
	\(bu cdif (word context diff) support
	\(bu read diff data from file or stdin

.fi
.SH OPTIONS
.IP "\-w \fIwidth\fP"
Use \fIwidth\fP as a width of output listing.  Default width
is 80.  Original \fIsdiff\fP has a default value 130 but
nobody uses 132 column line printer in these days.  If the
standard error is to a terminal, the width of that terminal
is taken as a output width if possible.
.IP \-l
Print only left column if the line is identical.
.IP \-s
Silent.  No output for identical lines.  Reading diff output
from file or stdin put this switch on automatically.
.IP \-n
Print line number on each lines.
.IP \-f
Fold the line if it is longer than printing width.  Folded
line is indicated by `+' mark at top of the line.  No
continue mark is printed when numbering option is on.
.IP \-F
Fold longs line at word boundaries.
.IP \-m
Usually diff mark from diff output are trimed off.  This
option forces to leave them on output listing.  I prefer to
use this option with \-c.
.IP "\-rcs, \-r\fIrev\fP"
Use rcsdiff instead of normal diff.  Option \-rcs is not
required when \-r\fIrev\fP is supplied.
.IP \-cdif
Use \fIcdif\fP command instead of normal diff command.
.IP "\-b, \-w, \-c, \-u, \-B"
Passed through to the back-end diff command.  \fISdif\fP can
process the output from context diff (\-c) and unified diff
(\-u) if those are available.  Option \-B is valid only for
\fIcdif\fP.
.SH RELATED COMMANDS
.IP a2ps
is a command which converts ascii text to postscript
program.  C version was originally posted to usenet by
miguel@imag.imag.fr (Miguel Santana).  I reimplemented it by
perl and enhanced to use different font family for
overstruck and underlined characters.  This is a convenient
tool to print the output from sdif to postscript printer.
.IP cdif
is a program which post-processes normal diff output and
highlights deleted, changed and added word on modified line.
It uses overstiking to highlighting.  Sdif can handle there
sequence correctly.  Cdif has to be installed to use \-cdif
option with sdif.
.PP
Next is a example that I'm using often.
.nf

	sdif -cdif -w180 file1 file2 | a2ps -w

.fi
.PP
All of these programs can be ftp'ed from sra.co.jp
(133.137.4.3) by anonymous ftp from the directory
~ftp/pub/lang/perl/sra-scripts.
.SH AUTHOR
Kazumasa Utashiro <utashiro@sra.co.jp>
.br
Software Research Associates, Inc., Japan
.SH "SEE ALSO"
perl(1), diff(1), cdif(1), a2ps(1)
.ex
