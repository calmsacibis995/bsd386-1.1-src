#! /bin/sh
#	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/util/transcript.sh,v 1.1.1.1 1994/01/14 23:10:51 torek Exp $
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

#
# transcript server-pid number
#
# Print the transcript of the last call placed to
# number by the server process with server-pid.
#
PATH=/bin:/usr/bin:
test -d /usr/ucb  && PATH=$PATH:/usr/ucb		# Sun and others
test -d /usr/bsd  && PATH=$PATH:/usr/bsd		# Silicon Graphics
test -d /usr/5bin && PATH=/usr/5bin:$PATH:/usr/etc	# Sun and others
test -d /usr/sbin && PATH=/usr/sbin:$PATH		# 4.4BSD-derived

if [ $# != 2 ]; then
    echo "Usage: transcript pid number"
    exit 1
fi
pid=$1 number=$2

echo ""
echo "    ---- Transcript of session follows ----"
echo ""
LOGFILE=log/`echo $number | sed -e 's/[^0-9]//g'`
if [ -f $LOGFILE ]; then
    RANGE="`grep -n $pid $LOGFILE | grep SESSION | tail -2 | cut -d: -f 1`"
    START=`echo "$RANGE" | sed '2,$d'`
    END=`echo "$RANGE" | tail -1`
    if [ "$START" -a "$END" ]; then
	sed -n -e "1,${START}d" -e "${END},\$d" \
	    -e '/start.*timer/d' -e '/stop.*timer/d' \
	    -e '/-- data/d' \
	    -e "/$pid/p" $LOGFILE
    else
	echo "No transcript available."
    fi
else
    echo "No transcript available."
fi
