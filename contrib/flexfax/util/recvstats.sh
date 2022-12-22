#! /bin/bash
#	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/util/recvstats.sh,v 1.2 1994/01/23 17:34:47 polk Exp $
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
# Print Statistics about Received Facsimile.
#
SPOOL=/var/spool/flexfax
AWK=nawk

# look for an an awk: nawk, gawk, awk
($AWK '{}' </dev/null >/dev/null) 2>/dev/null ||
    { AWK=gawk; ($AWK '{}' </dev/null >/dev/null) 2>/dev/null || AWK=awk; }

PATH=/bin:/usr/bin:/etc
test -d /usr/ucb  && PATH=$PATH:/usr/ucb		# Sun and others
test -d /usr/bsd  && PATH=$PATH:/usr/bsd		# Silicon Graphics
test -d /usr/5bin && PATH=/usr/5bin:$PATH:/usr/etc	# Sun and others
test -d /usr/sbin && PATH=/usr/sbin:$PATH		# 4.4BSD-derived

FILES=
SORTKEY=-sender

while [ x"$1" != x"" ] ; do
    case $1 in
    -send*|-csi|-dest*|-speed|-rate|-format)
	    SORTKEY=$1;;
    -*)	    echo "Usage: $0 [-sortkey] [files]"; exit 1;;
    *)	    FILES="$FILES $1";;
    esac
    shift
done
if [ -z "$FILES" ]; then
    FILES=$SPOOL/etc/xferlog
fi

#
# Construct awk rules to collect information according
# to the desired sort key.  There are two rules for
# each; to deal with the two different formats that
# have existed over time.
#
case $SORTKEY in
-send*|-csi)
    AWKRULES='$2 == "RECV" && NF == 9  { acct($3, $7, $8, $5, $6, $9); }
     	      $2 == "RECV" && NF == 11 { acct($6, $9, $10, $7, $8, $11); }'
    ;;
-dest*)
    AWKRULES='$2 == "RECV" && NF == 9  { acct($4, $7, $8, $5, $6, $9); }
    	      $2 == "RECV" && NF == 11 { acct($5, $9, $10, $7, $8, $11); }'
    ;;
-speed|-rate)
    AWKRULES='$2 == "RECV" && NF == 9  { acct($5, $7, $8, $5, $6, $9); }
    	      $2 == "RECV" && NF == 11 { acct($7, $9, $10, $7, $8, $11); }'
    ;;
-format)
    AWKRULES='$2 == "RECV" && NF == 9  { acct($6, $7, $8, $5, $6, $9); }
    	      $2 == "RECV" && NF == 11 { acct($8, $9, $10, $7, $8, $11); }'
    ;;
esac

#
# Generate an awk program to process the statistics file.
#
tmpAwk=/tmp/xfer$$
trap "rm -f $tmpAwk; exit 1" 0 1 2 15

(cat<<'EOF'
#
# Convert hh:mm:ss to seconds.
#
func cvtTime(s)
{
    t = i = 0;
    for (n = split(s, a, ":"); i++ < n; )
	t = t*60 + a[i];
    return t;
}

func setupDigits()
{
  digits[0] = "0"; digits[1] = "1"; digits[2] = "2";
  digits[3] = "3"; digits[4] = "4"; digits[5] = "5";
  digits[6] = "6"; digits[7] = "7"; digits[8] = "8";
  digits[9] = "9";
}

#
# Format seconds as hh:mm:ss.
#
func fmtTime(t)
{
    v = int(t/3600);
    result = "";
    if (v > 0) {
	if (v >= 10)
	    result = digits[int(v/10)];
	result = result digits[int(v%10)] ":";
	t -= v*3600;
    }
    v = int(t/60);
    if (v >= 10 || result != "")
	result = result digits[int(v/10)];
    result = result digits[int(v%10)];
    t -= v*60;
    return (result ":" digits[int(t/10)] digits[int(t%10)]);
}

#
# Setup a map for histogram calculations.
#
func setupMap(s, map)
{
    n = split(s, a, ":");
    for (i = 1; i <= n; i++)
	map[a[i]] = i;
}

#
# Add pages to a histogram.
#
func addToMap(key, ix, pages, map)
{
    if (key == "") {
	for (i in map)
	    key = key ":";
    }
    n = split(key, a, ":");
    a[map[ix]] += pages;
    t = a[1];
    for (i = 2; i <= n; i++)
      t = t ":" a[i];
    return t;
}

#
# Return the name of the item with the
# largest number of accumulated pages.
#
func bestInMap(totals, map)
{
   n = split(totals, a, ":");
   imax = 1; max = -1;
   for (j = 1; j <= n; j++)
       if (a[j] > max) {
	   max = a[j];
	   imax = j;
       }
   split(map, a, ":");
   return a[imax];
}

#
# Sort array a[l..r]
#
function qsort(a, l, r) {
    i = l;
    k = r+1;
    item = a[l];
    for (;;) {
	while (i < r) {
            i++;
	    if (a[i] >= item)
		break;
        }
	while (k > l) {
            k--;
	    if (a[k] <= item)
		break;
        }
        if (i >= k)
	    break;
	t = a[i]; a[i] = a[k]; a[k] = t;
    }
    t = a[l]; a[l] = a[k]; a[k] = t;
    if (k != 0 && l < k-1)
	qsort(a, l, k-1);
    if (k+1 < r)
	qsort(a, k+1, r);
}

#
# Accumulate a statistics record.
#
func acct(key, pages, time, br, df, status)
{
    gsub("\"", "", key);
    gsub("^ +", "", key);
    gsub(" +$", "", key);
    recvpages[key] += pages;
    if (pages == 0 && time > 60)
	time = 0;
    recvtime[key] += cvtTime(time);
    gsub("\"", "", status);
    if (status != "")
	recverrs[key]++;
    gsub("\"", "", br);
    recvrate[key] = addToMap(recvrate[key], br, pages, rateMap);
    gsub("\"", "", df);
    recvdata[key] = addToMap(recvdata[key], df, pages, dataMap);
}

#
# Print a rule between the stats and the totals line.
#
func printRule(n, s)
{
    r = "";
    while (n-- >= 0)
	r = r s;
    printf "%s\n", r;
}

BEGIN		{ FS="\t";
		  rates = "2400:4800:7200:9600:12000:14400";
		  setupMap(rates, rateMap);
		  datas = "1-D MR:2-D MR:2-D Uncompressed Mode:2-D MMR";
		  setupMap(datas, dataMap);
		}
END		{ OFS="\t"; setupDigits();
		  maxlen = 15;
		  nsorted = 0;
		  for (i in recvpages) {
		      l = length(i);
		      if (l > maxlen)
			maxlen = l;
		      sorted[nsorted++] = i;
		  }
		  qsort(sorted, 0, nsorted-1);
		  fmt = "%-" maxlen "." maxlen "s";	# e.g. %-24.24s
		  printf fmt " %5s %8s %6s %4s %7s %7s\n",
		      "Sender", "Pages", "Time", "Pg/min",
		      "Errs", "TypRate", "TypData";
		  tpages = 0;
		  ttime = 0;
		  terrs = 0;
		  for (k = 0; k < nsorted; k++) {
		      i = sorted[k];
		      t = recvtime[i]/60; if (t == 0) t = 1;
		      n = recvpages[i]; if (n == 0) n = 1;
		      brate = best
		      printf fmt " %5d %8s %6.1f %4d %7d %7s\n",
			  i, recvpages[i], fmtTime(recvtime[i]),
			  recvpages[i] / t, recverrs[i],
			  bestInMap(recvrate[i], rates),
			  bestInMap(recvdata[i], datas);
			tpages += recvpages[i];
			ttime += recvtime[i];
			terrs += recverrs[i];
		  }
		  printRule(maxlen+1+5+1+8+6+1+4+1+7+1+7, "-");
		  t = ttime/60; if (t == 0) t = 1;
		  printf fmt " %5d %8s %6.1f %4d\n",
		      "Total", tpages, fmtTime(ttime), tpages/t, terrs;
		}
EOF
echo "$AWKRULES"
)>$tmpAwk
$AWK -f $tmpAwk $FILES
