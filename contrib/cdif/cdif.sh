#!/usr/bin/perl
'di';
'ig00';
;#
;# cdif: word context diff
;#
;# Copyright (c) 1992 Kazumasa Utashiro <utashiro@sra.co.jp>
;# Software Research Associates, Inc., Japan
;#
;; $rcsid = 'Id: cdif,v 1.8 1992/12/28 11:49:23 utashiro Exp ';
;# Original: 1992/03/11
;#
;; ($myname = $0) =~ s,.*/,,;
;; $usage = <<_;
Usage: $myname [-B] [-v] [-n] [-A #] [-C #] [-D #] [-I #] [-[bwcu]] file1 file2
       $myname [-rcs] [-q] [-rrev1 [-rrev2]] [$myname options] file
       $myname [$myname options] [diff-output-file]
Options:
	-B	byte compare
	-v	use video standout (default for tty)
	-n	use nroff style overstrike (default for non-tty)
	-b	ignore trailing blank
	-w	ignore whitespace
	-c[#]	context diff
	-u[#]	unified diff (if diff has -u option)
	-A, -C, -D (Append, Change, Delete) takes one of
		vso: video standout
		vul: video underline
		vbd: video bold
		bd:  nroff style overstrike
		ul:  nroff style underline
		or any sequence or sequences separated by comma
	-I	specify string to be shown on insertion point
		Following strings have special meanings.
		vbar:	print vertical bar at the point
		caret:	print caret under the point
	-diff=command
		specify any diff command
_

$opts = 'A:C:D:I:Bcubdvqnt:'.'ibw';
while ($_ = $ARGV[0], /^-.+/ && shift) {
    next unless ($car, $cdr) = /^-?(.)(.*)/;
    if (/-rcs$/) { $rcs++; next; }
    if (/-diff=(.+)$/) { $diff = $1; next; }
    if ($car eq 'h') { &usage; }
    if ($car eq 'H') { exec "nroff -man $0|".($ENV{'PAGER'}||'more')." -s"; }
    if ($car eq 'r') {
	&usage("$_: Too many revisions\n\n") if @rcs == 2;
	$rcs++; push(@rcs, $_); next;
    }
    if (s/^-?([cu]\d+)//) {
	push(@diffopts, "-$1"); eval "\$opt_$car++"; redo;
    }
    push(@diffopts, "-$car") if index("iwcbu", $car) >= $[;
    push(@sub_diffopts, "-$car") if index("iw", $car) >= $[;
    if (/-(i[bxu])$/) {
	eval "\$opt_$1 = 1;"; next;
    }
    if (index($opts, "$car:") >= $[) {
	eval "\$opt_$car = length(\$cdr) ? \$cdr : \@ARGV ? shift : &usage";
	next;
    }
    if (index($opts, $car) >= $[) {
	eval "\$opt_$car++"; $_ = $cdr; redo;
    }
    &usage("Unknown option: $car\n");
}
$diff = $rcs ? 'rcsdiff' : 'diff' unless $diff;

sub usage {
    select(STDERR);
    print @_, $usage;
    print "$rcsid\n" if $rcsid =~ /:/;
    exit;
}

if ($rcs) {
    push(@rcsopt, '-q') if $opt_q;
    $rcsfile = shift || &usage("No RCS filename\n\n");
    $DIFF = "$diff @diffopts @rcsopt @rcs $dopts $rcsfile|";
} elsif (@ARGV == 2) {
    ($OLD, $NEW) = splice(@ARGV, 0, 2);
    $DIFF = "$diff @diffopts $OLD $NEW |";
} elsif (@ARGV < 2) {
    $DIFF = shift || '-';
} else {
    &usage("Arguments error.\n\n") if @ARGV;
}
print "DIFF=\"$DIFF\"\n" if $opt_d;

%func = ('bd', *bd, 'ul', *ul, 'vso', *vso, 'vul', *vul, 'vbd', *vbd);
($A, $C, $D) = (!$opt_n && ($opt_v || -t STDOUT))
    ? ('vso', 'vbd', 'vso') : ('bd', 'ul', 'bd');
for ('A', 'C', 'D') {
    eval "\$opt = \$opt_$_ || \$$_";
    if ($func{$opt}) {
	eval "*$_ = \$func{$opt}";
	&termcap if ($opt =~ /^v/) && !%TC;
    } else {
	$S = $S ? ++$S : 'S001';
	($start, $end) = $opt =~ /,/ ? split(/,/, $opt) : ($opt, $opt);
	$start =~ s/\W/\\$&/g; $end =~ s/\W/\\$&/g;
	eval "sub $S {join('', \"$start\", \@_, \"$end\");} *$_ = *$S;\n";
    }
}	

if ($opt_B) {
    $pat = '[\200-\377]?[\000-\377]';
} else {
    $pat = '[\200-\377].|\w+|[ \t\r\f]*\n|\s+|[\000-\377]';
}
@ul[1,2] = ("_\010", "__\010\010");
@bs[1,2] = ("\010", "\010\010");

if (defined($opt_I)) {
    if ($opt_I =~ /^vbar$/i) {			# special string 'vbar'
	$opt_ix = 1; $opt_ib = $opt_I =~ /[A-Z]/;
	$opt_I = '|';
    }
    if ($opt_I =~ /^caret$/i) {			# special string 'caret'
	$opt_ix = $opt_iu = 1; $opt_ib = $opt_I =~ /[A-Z]/;
	$opt_I = '^';
    }
    $opt_I = &bd($opt_I) if $opt_ib;
    ($hcr, $hcf, $hlr, $hlf) = ("\0336", "\0337", "\0338", "\0339");
    $opt_I = $hcr . $opt_I . $hcr if $opt_ix;
    $opt_I = $hlf . $opt_I . $hlr if $opt_iu;
}

$tmp = $opt_t || '/tmp';
$T1 = "$tmp/cdif_" . ($opt_d >= 2 ? $< : $$) . '_1';
$T2 = "$tmp/cdif_" . ($opt_d >= 2 ? $< : $$) . '_2';

@SIG{'INT', 'HUP', 'QUIT', 'TERM'} = ('cleanup') x 4;
$* = 1;

open(DIFF) || die "$diff: $!\n";
while (<DIFF>) {
    print;
    if (($left, $right) = /^([\d,]+)c([\d,]+)$/) {
	$old = &read(DIFF, scalar(&range($left)));
	$del = &read(DIFF, 1);
	$new = &read(DIFF, scalar(&range($right)));
	$old =~ s/^< //g; $new =~ s/^> //g;
	($old, $new) = &context($old, $new);
	$old =~ s/^/< /g; $new =~ s/^/> /g;
	print $old, $del, $new;
    }
    elsif (($left) = /^\*\*\* ([\d,]+) \*\*\*\*$/) {
	@old = &read_diffc(DIFF, scalar(&range($left)));
	print(@old), next if @old < 2;
	$new = $_ = <DIFF>; @new = ();
	if (($right) = /^--- ([\d,]+) ----$/) {
	    @new = &read_diffc(DIFF, scalar(&range($right)));
	    for ($i = $[ + 1; $i <= $#old; $i += 2) {
		$old[$i] =~ s/^! //g; $new[$i] =~ s/^! //g;
		($old[$i], $new[$i]) = &context($old[$i], $new[$i]);
		$old[$i] =~ s/^/! /g; $new[$i] =~ s/^/! /g;
	    }
	}
	print @old, $new, @new;
    }
    elsif (($ol, $nl) = /^\@\@ -\d+,(\d+) \+\d+,(\d+) \@\@$/) {
	@buf = &read_diffu(DIFF, $ol, $nl);
	while (($same, $old, $new) = splice(@buf, 0, 3)) {
	    if ($old && $new) {
		$old =~ s/^\-//g; $new =~ s/^\+//g;
		($old, $new) = &context($old, $new);
		$old =~ s/^/-/g; $new =~ s/^/+/g;
	    }
	    print $same, $old, $new;
	}
    }
}
close(DIFF);
exit($? >> 8);

######################################################################

sub context {
    local($old, $new, $_) = @_;
    local(@control);
    @c{'a', 'd', 'c'} = (0, 0, 0);
    @owlist = &maketmp($T1, $old);
    @nwlist = &maketmp($T2, $new);
    open(CDIF, "diff @sub_diffopts $T1 $T2 |") || die "diff: $!\n";
    /[\d,]+([adc])[\d,]+/ && push(@control, $_) && $c{$1}++ while (<CDIF>);
    close(CDIF);
    &cleanup() if $opt_d < 2;
    if ($opt_d >= 1) {
	printf "old=%d new=%d control=%d\n", @owlist+0, @nwlist+0, @control+0;
	printf "append=$c{'a'} delete=$c{'d'} change=$c{'c'}\n";
    }
    &makebuf;
    die "$myname: illegal status of subprocess\n" if ($?>>8) > 1;
    return ($obuf, $nbuf);
}

sub cleanup {
    unlink($T1), unlink($T2) if $opt_d < 2;
    warn("\nSignal @_\n"), exit 2 if @_;
}

sub maketmp {
    local($tmp, $_, @words) = @_;
    local(@notspace) = (0);
    open (TMP, ">$tmp") || warn "$tmp: $!\n", return 0;

    foreach (/$pat/go) {
	if ($opt_w) {
	    push(@notspace, !/\s/);
	    if (shift(@notspace) && $notspace[0]) {
		push(@words, '');
		print TMP "\n";
	    }
	}
	if (s/^(\s*)\n/\n/ && (length($1) || $opt_b || $opt_w)) {
	    #     ^ This have to be *.  Don't change to +.
	    print TMP ($opt_b || $opt_w) ? "\n" : "$1\n";
	    push(@words, $1);
	}
	push(@words, $_);
	print TMP $_;
	print TMP "\n" unless /\n$/;
    }
    close(TMP);
    if ($opt_d && @words != &wc_l($tmp)) {
	die "Error! (\@words != `wc -l $tmp`)\n";
    }
    @words;
}

sub makebuf {
    local($_, $old, $ctrl, $new, $o1, $o2, $n1, $n2);
    local($o, $n);
    $obuf = $nbuf = '';
    for (@control) {
	($old, $ctrl, $new) = /([\d,]+)([adc])([\d,]+)/;
	($o1, $o2) = &range($old); $o1--; $o2--;
	($n1, $n2) = &range($new); $n1--; $n2--;
	$obuf .= join('', @owlist[$o .. $o1 - 1]), $o = $o1 if $o < $o1;
	$nbuf .= join('', @nwlist[$n .. $n1 - 1]), $n = $n1 if $n < $n1;
	if ($ctrl eq 'd') {
	    $obuf .= &D(@owlist[$o1 .. $o2]);
	    $o = $o2 + 1;
	    $nbuf .= $nwlist[$n++] . $opt_I if $opt_I;
	}
	if ($ctrl eq 'c') {
	    $obuf .= &C(@owlist[$o1 .. $o2]);
	    $nbuf .= &C(@nwlist[$n1 .. $n2]);
	    $o = $o2 + 1;
	    $n = $n2 + 1;
	}
	if ($ctrl eq 'a') {
	    $nbuf .= &A(@nwlist[$n1 .. $n2]);
	    $n = $n2 + 1;
	    $obuf .= $owlist[$o++] . $opt_I if $opt_I
	}
    }
    $obuf .= join('', @owlist[$o .. $#owlist]);
    $nbuf .= join('', @nwlist[$n .. $#nwlist]);
}

sub read_diffc {
    local($FH, $n, $i, @buf, $_) = @_;
    for ($i = 0; $n-- && ($_ = <$FH>); $buf[$i] .= $_) {
	$i++ if ($i % 2) != /^!/;
    }
    @buf;
}

sub read_diffu {
    local($FH, @l[1,2,0]) = @_;
    local($i, @buf, $slot);
    %slot = (' ', 0, '-', 1, '+', 2) unless %slot;
    for (; 2 * $l[0] + $l[1] + $l[2] > 0 && ($_ = <$FH>); $buf[$i] .= $_) {
	$i++ while ($i % 3) != ($slot = $slot{substr($_, 0, 1)});
	$l[$slot]--;
    }
    @buf;
}

sub vso { join('', $so, @_, $se); }
sub vul { join('', $us, @_, $ue); }
sub vbd { join('', $md, @_, $me); }
sub ul { join('', grep(s/[\200-\377]?./$ul[length($&)].$&/ge || 1, @_)); }
sub bd { join('', grep(s/[\200-\377]?./$&.$bs[length($&)].$&/ge || 1, @_)); }

sub range {
    local($_, $from, $to) = @_;
    ($from, $to) = /,/ ? split(/,/) : ($_, $_);
    wantarray ? ($from, $to) : $to - $from + 1;
}

sub read {
    local($FH, $c, @buf) = (@_);
    push(@buf, scalar(<$FH>)) while $c--;
    wantarray ? @buf : join('', @buf);
}

sub termcap {
    do 'ioctl.ph' || do 'sys/ioctl.ph';
    require('termcap.pl'); $ospeed = 1 unless $ospeed;
    &Tgetent;
    $so = &Tputs($TC{'so'}); $se = &Tputs($TC{'se'});
    $us = &Tputs($TC{'us'}); $ue = &Tputs($TC{'ue'});
    $md = &Tputs($TC{'md'}) || $so; $me = &Tputs($TC{'me'}) || $se;
}

sub wc_l {
    local($file) = shift;
    local(*FILE, $line);
    open(FILE, $file) || die "$file: $!\n";
    $line++ while <FILE>;
    close(FILE);
    $line;
}
######################################################################
.00;			# finish .ig

'di			\" finish diversion--previous line must be blank
.nr nl 0-1		\" fake up transition to first page again
.nr % 0			\" start at page 1
'; __END__ ############# From here on it's a standard manual page ####
.de XX
.ds XX \\$4\ (v\\$3)
..
.XX $Id: cdif.sh,v 1.1.1.1 1993/03/07 18:58:04 sanders Exp $
.TH CDIF 1 \*(XX
.SH NAME
cdif \- word context diff
.SH SYNOPSIS
.nr ww \w'\fBcdif\fP\ '
.in +\n(wwu
.ta \n(wwu
.ti -\n(wwu
\fBcdif\fP	\c
[-B] [-v] [-n] [-A #] [-C #] [-D #] [-[bwcu]]
\fIfile1\fP \fIfile2\fP
.br
.ti -\n(wwu
\fBcdif\fP	\c
[-rcs] [-q] [-r\fIrev1\fP [-r\fIrev2\fP]] [\fIcdif options\fP] \fIfile\fP
.br
.ti -\n(wwu
\fBcdif\fP	\c
[\fIcdif options\fP] [\fIdiff-output-file\fP]
.SH DESCRIPTION
.I Cdif
is a post-processor of the Unix \fIdiff\fP command.  It
highlights deleted, changed and added words based on word
context.  Highlighting is usually done by using different
style font and the output looks like this:
.nf

	1c1
	< highlights \fBdeleted\fP and \fIchanged\fP and words
	---
	> highlights and \fImodified\fP and \fBadded\fP words

.fi
.PP
Appended and deleted lines are not effected at all.
.PP
You may want to compare character-by-character rather than
word-by-word.  Option \-B option can be used for that
purpose and the output looks like:
.nf

	1c1
	< $opts = '\fBcu\fPbdvqnB\fIA\fP:\fIC\fP:\fID\fP:';
	---
	> $opts = 'bdvqn\fBxy\fPB\fIP\fP:\fIQ\fP:\fIR\fP:';

.fi
.PP
If the standard output is a terminal, \fIcdif\fP uses
termcap to highlights words.  It uses \fImd\fP and \fIme\fP
(bold or bright) sequence for changed words and \fIso\fP and
\fIse\fP (standout) sequence for deleted and added part.
.PP
If the standard output is not a terminal, it uses nroff
style overstriking to highlight them.  You won't be able to
tell the difference on normal screen but using pager command
like \fImore\fP or \fIless\fP, or printing to appropriate
printer will make them visible.
.PP
If only one file is specified, \fIcdif\fP reads that file
(stdin if no file) as a output from diff command.  Lines
those don't look like diff output are simply ignored and
printed same as input.  So you can use \fIcdif\fP from
\fIrn\fP(1) like `|cdif'.
.SH OPTIONS
.IP \-B
Compare the data character-by-character context.
.IP \-v
Force to use video effect even if the output is not a
terminal.
.IP \-n
Force not to use video effect even if the output is a
terminal.
.IP "\-[ACD] \fIeffect\fP"
Specify the effect to use for added, changed and deleted
words respectively.  Special effects are:
.nf

	vso: video standout
	vul: video underline
	vbd: video bold
	bd: nroff style overstrike
	ul: nroff style underline

.fi
.IP
If specified effect doesn't match to any of special words,
the sequence is used to highlighting string.  Start and end
string are separated by comma.  So command:
.nf

	cdif -D'<,>' -C'[,]' -A'{,}'

.fi
produces following output.
.nf

	1c1
	< highlights <deleted >and [changed] and words
	---
	> highlights and [modified] and{ added} words

.fi
.IP "\-I \fIstring\fP"
Specify the string to be shown at the inserted or deleted
point.  Normally the point at where new text is inserted
or old text is deleted is not indicated in the output.  \-I
option specifies the string which is to be shown at the
insertion point.  For example, command
.nf

	cdif -D'<,>' -C'[,]' -A'{,}' -I'|'

.fi
produces the output like below.
.nf

	1c1
	< highlights <deleted >and [changed] and| words
	---
	> highlights |and [modified] and{ added} words

.fi
Following four options are used to specify how this string
is printed in the output.
.RS
.IP \-ib
Makes the string overstruck.
.IP \-iu
Put the string under the line.  According to the sequence of
Teletype model 37, the string is enclosed by ESC-9 and ESC-8
sequences.
.IP \-ix
Put the string at the exact point.  The string is enclosed
by non-standard ESC-6 sequences which indecate to move the
printing point half-character backward.  Currently this
sequence is understood only by my perl version of a2ps.  See
RELATED COMMANDS.
.RE
.IP ""
Special strings \fIvbar\fP, \fIVBAR\fP, \fIcaret\fP,
\fICARET\fP are prepared for shortcut.  `\fIvbar\fP' means
putting the vertical bar (|) at the exact insertion point.
`\fIcaret\fP' means putting the caret mark (^) under the
exact inserntion point.  Uppercase chacacter makes them
overstruck.
.IP "\-diff=\fIcommand\fP"
Specify the diff command to use.
.IP "\-rcs, \-r\fIrev\fP"
Use rcsdiff instead of normal diff.  Option \-rcs is not
required when \-r\fIrev\fP is supplied.
.IP "\-b, \-w, \-c, \-u"
Passed through to the back-end diff command.  \fICdif\fP can
process the output from context diff (\-c) and unified diff
(\-u) if those are available.
.SH RELATED COMMANDS
.IP a2ps
is a command which converts ascii text to postscript
program.  C version was originally posted to usenet by
miguel@imag.imag.fr (Miguel Santana).  I reimplemented it by
perl and enhanced to use different font family for
overstruck and underlined characters.  This is a convenient
tool to print the output from cdif to postscript printer.
.IP sdif
is a System V's sdiff clone written in perl.  It also can
handle overstruck and underlined character, and has \-cdif
option to use \fIcdif\fP instead of normal diff.
.IP termfix
is a program to change termcap capability of current
terminal temporarily.  Recent more and less uses \fImd\fP
and \fIme\fP capability to display overstruck character if
available.  Sometimes we want to use standout rather than
bolding.  Termfix allows to cancel \fImd\fP and \fImd\fP
capability only for the invoking command by ``md@'' option.
.PP
All of these programs can be ftp'ed from sra.co.jp
(133.137.4.3) by anonymous ftp from the directory
~ftp/pub/lang/perl/sra-scripts.
.SH AUTHOR
Kazumasa Utashiro <utashiro@sra.co.jp>
.br
Software Research Associates, Inc., Japan
.SH "SEE ALSO"
perl(1), diff(1), sdif(1), a2ps(1), termfix(1)
.SH BUGS
\fICdif\fP is naturally slow because it uses normal diff
command as a back-end processor to compare words.
.ex
