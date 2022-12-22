#! /bin/bash
#	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/util/notify.sh,v 1.1.1.1 1994/01/14 23:10:50 torek Exp $
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
#
PS2FAX=bin/ps2fax
TRANSCRIPT=bin/transcript
TAR=tar
AWK=nawk
ENCODE=uuencode
DECODE=uudecode
COMPRESS=compress
DECOMPRESS=zcat
#
# notify qfile why jobtime server-pid number [nextTry]
#
# Return mail to the submitter of a job when notification is needed.
#

PATH=/bin:/usr/bin:
test -d /usr/ucb  && PATH=$PATH:/usr/ucb		# Sun and others
test -d /usr/bsd  && PATH=$PATH:/usr/bsd		# Silicon Graphics
test -d /usr/5bin && PATH=/usr/5bin:$PATH:/usr/etc	# Sun and others
test -d /usr/sbin && PATH=/usr/sbin:$PATH		# 4.4BSD-derived

if [ $# != 5 -a $# != 6 ]; then
    echo "Usage: $0 qfile why jobtime pid number [nextTry]"
    exit 1
fi
qfile=$1
nextTry=${6:-'??:??'}

# look for an an awk: nawk, gawk, awk
($AWK '{}' </dev/null >/dev/null) 2>/dev/null ||
    { AWK=gawk; ($AWK '{}' </dev/null >/dev/null) 2>/dev/null || AWK=awk; }

($AWK -F: '
#
# Construct a return-to-sender message.
#
func returnToSender()
{
    printf "\n    ---- Unsent job status ----\n\n"
    printf "%11s: %s\n", "Destination", number;
    printf "%11s: %s\n", "Sender", sender;
    printf "%11s: %s\n", "Mailaddr", mailaddr;
    if (modem != "any")
	printf "%11s: %s\n", "Modem", modem;
    printf "%11s: %u (mm)\n", "PageWidth", pagewidth;
    printf "%11s: %.0f (mm)\n", "PageLength", pagelength;
    printf "%11s: %.0f (lpi)\n", "Resolution", resolution;
    printf "%11s: %s\n", "Status", status;
    printf "%11s: %u (consecutive failed calls to destination)\n",
	"Dials", ndials;
    printf "%11s: %u (attempts to send current page)\n",
	"Attempts", ntries;
    printf "%11s: %u (directory of next page to send)\n",
	"Dirnum", dirnum;
    if (nfiles > 0) {
	printf "\n    ---- Unsent files submitted for transmission ----\n\n";
	printf "This archive was created with %s, %s, and %s.\n",
	    tar, compressor, encoder;
	printf "The original files can be recovered by applying the following commands:\n";
	printf "\n";
	printf "    %s			# generates rts.%s.Z file\n", decoder, tar;
	printf "    %s rts.%s.Z  | %s xvf -	# generates separate files\n",
	    decompressor, tar, tar;
	printf "\n";
	printf "and the job can be resubmitted using the following command:\n";
	printf "\n";
	printf "    sendfax -d %s" poll notify resopt "%s\n", number, files;
	printf "\n";

	system("cd docq;" tar " cf - " files \
			" | " compressor \
			" | " encoder " rts." tar ".Z");
    }
}

func returnTranscript(pid, canon)
{
    system(transcript " " pid " \"" canon "\" 2>&1");
}

func printStatus(s)
{
    if (s == "")
	print "<no reason recorded>";
    else
	print s
}

BEGIN		{ nfiles = 0; }
/^number/	{ number = $2; }
/^sender/	{ sender = $2; }
/^mailaddr/	{ mailaddr = $2; }
/^jobtag/	{ jobtag = $2; }
/^status/	{ status = $2; }
/^resolution/	{ resolution = $2;
		  if (resolution == 196)
		      resopt = " -m";
		}
/^npages/	{ npages = $2; }
/^dirnum/	{ dirnum = $2; }
/^ntries/	{ ntries = $2; }
/^ndials/	{ ndials = $2; }
/^pagewidth/	{ pagewidth = $2; }
/^pagelength/	{ pagelength = $2; }
/^modem/	{ modem = $2; }
/^notify/	{ if ($2 == "when done")
		      notify = " -D";
		  else if ($2 == "when requeued")
		      notify = " -R";
		}
/^sstatus/	{ jstatus = $2; }
/^[!]*post/	{ split($2, parts, "/");
		  if (!match(parts[2], "\.cover")) {	# skip cover pages
		     files = files " " parts[2];
		     nfiles++;
		  }
		}
/^!tiff/	{ split($2, parts, "/"); files = files " " parts[2]; nfiles++; }
/^poll/		{ poll = " -p"; }
END {
    if (jobtag == "")
	jobtag = FILENAME;
    print "To: " mailaddr;
    print "Subject: notice about facsimile job " jobtag;
    print "";
    printf "Your facsimile job \"" jobtag "\" to \"" number "\"";
    if (why == "done") {
	print " was completed successfully.";
	printf "Total transmission time was " jobTime ".";
	if (status != "")
	    print "  Additional information:\n    " status;
    } else if (why == "failed") {
	printf " failed because:\n    ";
	printStatus(status);
	returnTranscript(pid, canon);
	returnToSender();
    } else if (why == "requeued") {
	printf " was not sent because:\n    ";
	printStatus(status);
	print "";
	print "The job will be retried at " nextTry "."
	returnTranscript(pid, canon);
    } else if (why == "removed" || why == "killed") {
	print " was deleted from the queue.";
	if (why == "killed")
	    returnToSender();
    } else if (why == "timedout") {
	print " could not be sent after repeated attempts.";
	returnToSender();
    } else if (why == "format_failed") {
	print " was not sent because"
	print "conversion of PostScript to facsimile failed.";
	print "The output from \"" ps2fax "\" was:\n";
	print status "\n";
	printf "Check your job for non-standard fonts %s.\n",
	    "and invalid PostScript constructs";
	returnToSender();
    } else if (why == "no_formatter") {
	print " was not sent because";
	print "the conversion program \"" ps2fax "\" was not found.";
	returnToSender();
    } else if (match(why, "poll_*")) {
	printf ", a polling request,\ncould not be completed because ";
	if (why == "poll_rejected")
	    print "the remote side rejected your request.";
	else if (why == "poll_no_document")
	    print "no document was available for retrieval.";
	else if (why == "poll_failed")
	    print "an unspecified problem occurred.";
	print "";
	printf "Total connect time was %s.\n", jobTime;
    } else if (why == "file_rejected") {
	printf " was rejected because:\n    ";
	printStatus(status);
	returnToSender();
    } else {
	print " had something happen to it."
	print "Unfortunately, the notification script was invoked",
	    "with an unknown reason"
	print "so the rest of this message is for debugging:\n";
	print "why: " why;
	print "jobTime: " jobTime;
	print "pid: " pid;
	print "canon: " canon;
	print "nextTry: " nextTry;
	print  "";
	print "This should not happen, please report it to your administrator.";
	returnToSender();
    }
}
' why=$2 jobTime=$3 pid=$4 canon=$5 nextTry=$nextTry\
  ps2fax=$PS2FAX transcript=$TRANSCRIPT\
  tar=$TAR \
  encoder=$ENCODE decoder=$DECODE \
  compressor=$COMPRESS decompressor=$DECOMPRESS \
  $qfile || {
      echo ""
      echo "Sorry, there was a problem doing send notification;"
      echo "something went wrong in the shell script $0."
      echo ""
      exit 1;
  }
) | 2>&1 $SENDMAIL -t -ffax -oi
