/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxalter/faxalter.c++,v 1.1.1.1 1994/01/14 23:09:42 torek Exp $
/*
 * Copyright (c) 1990, 1991, 1992, 1993 Sam Leffler
 * Copyright (c) 1991, 1992, 1993 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */
#include "FaxClient.h"
#include "StrArray.h"
#include "config.h"

#include <stdlib.h>
#include <string.h>

class faxAlterApp : public FaxClient {
private:
    fxStr	appName;		// for error messages
    fxStr	notify;
    fxStr	tts;
    fxStr	killtime;
    fxStr	maxdials;
    fxStrArray	jobids;

    void usage();
public:
    faxAlterApp();
    ~faxAlterApp();

    void initialize(int argc, char** argv);
    void open();

    void recvConf(const char* cmd, const char* tag);
    void recvEof();
    void recvError(const int err);
};
faxAlterApp::faxAlterApp() {}
faxAlterApp::~faxAlterApp() {}

void
faxAlterApp::initialize(int argc, char** argv)
{
    int c, n;

    appName = argv[0];
    u_int l = appName.length();
    appName = appName.tokenR(l, '/');
    while ((c = getopt(argc, argv, "a:h:k:n:t:DQRpv")) != -1)
	switch (c) {
	case 'D':			// set notification to when done
	    notify = "when done";
	    break;
	case 'Q':			// no notification (quiet)
	    notify = "none";
	    break;
	case 'R':			// set notification to when requeued
	    notify = "when requeued";
	    break;
	case 'a':			// send at specified time
	    tts = optarg;
	    break;
	case 'h':			// server's host
	    setHost(optarg);
	    break;
	case 'k':			// kill job at specified time
	    killtime = optarg;
	    break;
	case 'n':			// set notification
	    if (strcmp(optarg, "done") == 0)
		notify = "when done";
	    else if (strcmp(optarg, "requeued") == 0)
		notify = "when requeued";
	    else
		notify = optarg;
	    break;
	case 'p':			// send now (push)
	    tts = "now";
	    break;
	case 't':			// set max number of retries
	    n = atoi(optarg);
	    if (n < 0)
		fxFatal("Bad number of retries for -t option: %s", optarg);
	    maxdials = fxStr(n, "%d");
	    break;
	case 'v':			// trace protocol
	    setVerbose(TRUE);
	    break;
	case '?':
	    usage();
	}
    if (optind >= argc)
	usage();
    if (tts == "" && notify == "" && killtime == "" && maxdials == "")
	fxFatal("No job parameters specified for alteration.");
    setupUserIdentity();
    for (; optind < argc; optind++)
	jobids.append(argv[optind]);
}

void
faxAlterApp::usage()
{
    fxFatal("usage: %s"
      " [-h server-host]"
      " [-a time]"
      " [-k time]"
      " [-n notify]"
      " [-t tries]"
      " [-p]"
      " [-DQR]"
      " jobID...", (char*) appName);
}

void
faxAlterApp::open()
{
    if (callServer() == FaxClient::Success) {
	for (int i = 0; i < jobids.length(); i++) {
	    fxStr line = jobids[i] | ":" | getSenderName();
	    // do notify first 'cuz setting tts causes q rescan
	    if (notify != "")
		sendLine("alterNotify", (char*)(line | ":" | notify));
	    if (tts != "")
		sendLine("alterTTS", (char*)(line | ":" | tts));
	    if (killtime != "")
		sendLine("alterKillTime", (char*)(line | ":" | killtime));
	    if (maxdials != "")
		sendLine("alterMaxDials", (char*)(line | ":" | maxdials));
	}
	sendLine(".\n");
	startRunning();
    } else
	fxFatal("Could not call server.");
}

#define isCmd(s)	(strcasecmp(s, cmd) == 0)

void
faxAlterApp::recvConf(const char* cmd, const char* tag)
{
    if (isCmd("altered")) {
//	printf("Job %s altered.\n", tag);
    } else if (isCmd("notQueued")) {
	printf("Job %s not altered: it was not queued.\n", tag);
    } else if (isCmd("jobLocked")) {
	printf("Job %s not altered: it is being sent.\n", tag);
    } else if (isCmd("openFailed")) {
	printf("Job %s not altered: open failed; check SYSLOG.\n", tag);
    } else if (isCmd("sOwner")) {
	printf(
	"Job %s not altered: could not establish server process owner.\n",
	    tag);
    } else if (isCmd("jobOwner")) {
	printf(
	"Job %s not altered: you neither own it nor are the fax user.\n",
	    tag);
    } else if (isCmd("error")) {
	printf("%s\n", tag);
    } else {
	printf("Unknown status message \"%s:%s\"\n", cmd, tag);
    }
}

void
faxAlterApp::recvEof()
{
    stopRunning();
}

void
faxAlterApp::recvError(const int err)
{
    printf("Fatal socket read error: %s\n", strerror(err));
    stopRunning();
}

#include <Dispatch/dispatcher.h>

extern "C" int
main(int argc, char** argv)
{
    faxAlterApp app;
    app.initialize(argc, argv);
    app.open();
    while (app.isRunning())
	Dispatcher::instance().dispatch();
    return 0;
}
