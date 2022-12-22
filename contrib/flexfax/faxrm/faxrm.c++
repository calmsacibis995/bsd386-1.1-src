/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxrm/faxrm.c++,v 1.1.1.1 1994/01/14 23:09:51 torek Exp $
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
#include <stdlib.h>
#include <string.h>

#include "FaxClient.h"
#include "StrArray.h"
#include "config.h"

class faxRmApp : public FaxClient {
private:
    fxStr	appName;		// for error messages
    fxStrArray	jobids;
    fxStr	request;

    void usage();
public:
    faxRmApp();
    ~faxRmApp();

    void initialize(int argc, char** argv);
    void open();

    void recvConf(const char* cmd, const char* tag);
    void recvEof();
    void recvError(const int err);
};

faxRmApp::faxRmApp() : request("remove") {}
faxRmApp::~faxRmApp() {}

void
faxRmApp::initialize(int argc, char** argv)
{
    extern int optind;
    extern char* optarg;
    int c;

    appName = argv[0];
    u_int l = appName.length();
    appName = appName.tokenR(l, '/');
    while ((c = getopt(argc, argv, "h:kv")) != -1)
	switch (c) {
	case 'h':			// server's host
	    setHost(optarg);
	    break;
	case 'k':
	    request = "kill";
	    break;
	case 'v':
	    setVerbose(TRUE);
	    break;
	case '?':
	    usage();
	}
    if (optind >= argc)
	usage();
    setupUserIdentity();
    for (; optind < argc; optind++)
	jobids.append(argv[optind]);
}

void
faxRmApp::usage()
{
    fxFatal("usage: %s [-h server-host] [-kv] jobID...", (char*) appName);
}

void
faxRmApp::open()
{
    if (callServer() == FaxClient::Success) {
	for (int i = 0; i < jobids.length(); i++) {
	    fxStr line = jobids[i] | ":" | getSenderName();
	    sendLine(request, line);
	}
	sendLine(".\n");
	startRunning();
    } else
	fxFatal("Could not call server.");
}

#define isCmd(s)	(strcasecmp(cmd, s) == 0)

void
faxRmApp::recvConf(const char* cmd, const char* tag)
{
    if (isCmd("removed")) {
	printf("Job %s removed.\n", tag);
    } else if (isCmd("notQueued")) {
	printf("Job %s not removed: it was not queued.\n", tag);
    } else if (isCmd("jobLocked")) {
	printf("Job %s not removed: it is being sent.\n", tag);
    } else if (isCmd("openFailed")) {
	printf("Job %s not removed: open failed; check SYSLOG.\n", tag);
    } else if (isCmd("unlinkFailed")) {
	printf("Job %s not removed: unlink failed; check SYSLOG.\n", tag);
    } else if (isCmd("docUnlinkFailed")) {
	printf(
	    "Document for job %s not removed: unlink failed; check SYSLOG.\n",
	    tag);
    } else if (isCmd("sOwner")) {
	printf(
	    "Job %s not removed: could not establish server process owner.\n",
	    tag);
    } else if (isCmd("jobOwner")) {
	printf("Job %s not removed: you neither own it nor are the fax user.\n",
	    tag);
    } else if (isCmd("error")) {
	printf("%s\n", tag);
    } else {
	printf("Unknown status message \"%s:%s\"\n", cmd, tag);
    }
}

void
faxRmApp::recvEof()
{
    stopRunning();
}

void
faxRmApp::recvError(const int err)
{
    printf("Fatal socket read error: %s\n", strerror(err));
    stopRunning();
}

#include <Dispatch/dispatcher.h>

extern "C" int
main(int argc, char** argv)
{
    faxRmApp app;
    app.initialize(argc, argv);
    app.open();
    while (app.isRunning())
	Dispatcher::instance().dispatch();
    return 0;
}
