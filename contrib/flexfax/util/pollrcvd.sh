#! /bin/sh
#	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/util/pollrcvd.sh,v 1.1.1.1 1994/01/14 23:10:51 torek Exp $
#
# FlexFAX Facsimile Software
#
# Copyright (c) 1990, 1991, 1992, 1993 Sam Leffler
# Copyright (c) 1991, 1992, 1993 Silicon Graphics, Inc.
# 
# Permission to use, copy, modify, distribute, and sell this software and 
# its documentation for any purpose is hereby granted without fee, provided
# that (i) the above copyright notices and this permission notice appear in
# all copies of the software and related documentation, and (ii) the names of
# Sam Leffler and Silicon Graphics may not be used in any advertising or
# publicity relating to the software without the specific, prior written
# permission of Sam Leffler and Silicon Graphics.
# 
# THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
# EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
# WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
# 
# IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
# ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
# OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
# WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
# LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
# OF THIS SOFTWARE.
#
SPOOL=/var/spool/flexfax
SENDMAIL=/usr/sbin/sendmail
ENCODE=uuencode
DECODE=uudecode
#
# pollrcvd mailaddr faxfile time protocol sigrate error-msg
#
if test $# != 6
then
    echo "Usage: $0 mailaddr faxfile time sigrate protocol error-msg"
    exit 1
fi
mailaddr=$1; shift

PATH=/bin:/usr/bin:
test -d /usr/ucb  && PATH=$PATH:/usr/ucb		# Sun and others
test -d /usr/bsd  && PATH=$PATH:/usr/bsd		# Silicon Graphics
test -d /usr/5bin && PATH=/usr/5bin:$PATH:/usr/etc	# Sun and others
test -d /usr/sbin && PATH=/usr/sbin:$PATH		# 4.4BSD-derived

(
echo "Subject: document received by polling request";
echo "";
$SPOOL/bin/faxinfo $1
cat<<EOF
TimeToRecv: $2 minutes
SignalRate: $3
DataFormat: $4
EOF
if [ "$5" ]; then
    echo ""
    echo "The full document was not received because:"
    echo ""
    echo "    $5"
fi
cat<<EOF

    ---- Encoded facsimile file ----

The data enclosed below is encoded by $ENCODE.  The binary facsimile
data can be recovered by passing this message through the $DECODE
command to generate the file fax.tif.

EOF
cat $1 | $ENCODE fax.tif
) | 2>&1 $SENDMAIL -ffax -oi $mailaddr
