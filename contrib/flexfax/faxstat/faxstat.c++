/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxstat/faxstat.c++,v 1.1.1.1 1994/01/14 23:09:57 torek Exp $
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
#include <ctype.h>
#include <osfcn.h>
#include <pwd.h>
#include <termios.h>
#if defined(TIOCGWINSZ) && defined(sco)
#include <sys/stream.h>
#include <sys/ptem.h>
#endif

#include "FaxStatClient.h"
#include "SendStatus.h"
#include "RecvStatus.h"
#include "ServerStatus.h"
#include "StrArray.h"
#include "config.h"

class faxStatApp {
private:
    fxStr		appName;	// for error messages
    fxStrArray		senderNames;	// sender's full name (if available)
    fxStrArray		jobs;		// jobs to ask status for
    fxBool		allJobs;	// if true, ask for all send jobs
    fxBool		recvJobs;	// if true, ask for received status
    fxBool		debug;
    fxBool		verbose;	// if true, enable protocol tracing
    fxBool		multipleClients;// more than 1 client thread
    int			ncols;		// number of columns for output
    FaxStatClientArray	clients;	// client ``threads''
    FaxSendStatusArray	sendStatus;	// collected send job status
    FaxRecvStatusArray	recvStatus;	// collected recv status
    FaxServerStatusArray serverStatus;	// collected server status

    FaxStatClient* newClient(const char* host);
    void setupUserIdentity();
    void addAllHosts();
public:
    faxStatApp();
    ~faxStatApp();

    void initialize(int argc, char** argv);
    void open();

    void printServerStatus(FILE*);
    fxBool hasServerStatus()		{ return serverStatus.length() > 0; }
    void printSendStatus(FILE*);
    fxBool hasSendStatus()		{ return sendStatus.length() > 0; }
    void printRecvStatus(FILE*);
    fxBool hasRecvStatus()		{ return recvStatus.length() > 0; }

    fxBool isRunning()
	{ return FaxStatClient::getClientsDone() < clients.length(); }
};

faxStatApp::faxStatApp()
{
    allJobs = FALSE;
    recvJobs = FALSE;
    debug = FALSE;
    verbose = FALSE;
    ncols = -1;
}

faxStatApp::~faxStatApp()
{
    for (int i = 0, n = clients.length(); i < n; i++) {
	FaxStatClient* c = clients[i];
	delete c;
    }
}

FaxStatClient*
faxStatApp::newClient(const char* host)
{
    return new FaxStatClient(host, sendStatus, recvStatus, serverStatus);
}

void
faxStatApp::initialize(int argc, char** argv)
{
    extern int optind;
    extern char* optarg;
    int c;

    appName = argv[0];
    u_int l = appName.length();
    appName = appName.tokenR(l, '/');
    fxBool noSendJobs = FALSE;
    while ((c = getopt(argc, argv, "h:u:arstwvD")) != -1)
	switch (c) {
	case 'h':			// server's host
	    if (debug)
		fxFatal("Debug incompatible with host specification.");
	    clients.append(newClient(optarg));
	    break;
	case 'r':
	    recvJobs = TRUE;
	    break;
	case 's':
	    noSendJobs = TRUE;
	    break;
	case 't':
	    if (debug)
		fxFatal("Debug incompatible with host specification.");
	    addAllHosts();
	    break;
	case 'u':
	    senderNames.append(optarg);
	    break;
	case 'a':			// all send jobs
	    allJobs = TRUE;
	    break;
	case 'D':			// debug
	    if (clients.length() > 0)
		fxFatal("Debug incompatible with host specification.");
	    debug = TRUE;
	    setvbuf(stdout, NULL, _IONBF, BUFSIZ);
	    break;
	case 'v':
	    verbose = TRUE;
	    break;
	case 'w':
	    if (ncols == -1)
		ncols = 132;
	    else if (ncols == 132)
		ncols = 1000;		// something big
	    break;
	case '?':
	    fxFatal("usage: %s"
		" [-h server-host]"
		" [-u user-name]"
		" [-arsv]"
		" [jobs]",
		(char*) appName);
	}

    if (clients.length() == 0)
	clients.append(newClient(""));
    if (senderNames.length() == 0 && !noSendJobs)
	setupUserIdentity();
    if (optind < argc) {
	// list of jobs to check on
	jobs.resize(argc-optind);
	for (u_int i = 0; optind < argc; i++, optind++)
	    jobs[i] = argv[optind];
    }
    multipleClients = (clients.length() > 1);
#ifdef TIOCGWINSZ
    winsize ws;
    if (ncols == -1 && ioctl(fileno(stdout), TIOCGWINSZ, &ws) == 0)
	ncols = ws.ws_col;
#endif
    if (ncols == -1)
	ncols = 80;
}

void
faxStatApp::open()
{
    int n = clients.length();
    for (int i = 0; i < n; i++) {
	FaxStatClient* c = clients[i];
	c->setVerbose(verbose);
	if (c->start(debug)) {
	    c->sendLine("serverStatus:\n");
	    if (jobs.length() > 0) {
		for (int j = 0; j < jobs.length(); j++)
		    c->sendLine("jobStatus", jobs[j]);
	    } else if (allJobs) {
		c->sendLine("allStatus:\n");
	    } else {
		for (int j = 0; j < senderNames.length(); j++)
		    c->sendLine("userStatus", senderNames[j]);
	    }
	    if (recvJobs)
		c->sendLine("recvStatus:\n");
	    c->sendLine(".\n");
	}
    }
}

void
faxStatApp::setupUserIdentity()
{
    struct passwd* pwd = NULL;
    char* name = cuserid(NULL);
    if (!name) {
	name = getlogin();
	if (name)
	    pwd = getpwnam(name);
    }
    if (!pwd)
	pwd = getpwuid(getuid());
    if (!pwd)
	fxFatal("Can not determine your user name.");

    fxStr namestr;
    if (pwd->pw_gecos && pwd->pw_gecos[0] != '\0') {
	namestr = pwd->pw_gecos;
	u_int l = namestr.next(0, '&');
	if (l < namestr.length()) {
	    /*
	     * Do the '&' substitution and raise the
	     * case of the first letter of the inserted
	     * string (the usual convention...)
	     */
	    namestr.remove(l);
	    namestr.insert(pwd->pw_name, l);
	    if (islower(namestr[l]))
		namestr[l] = toupper(namestr[l]);
	}
	namestr.resize(namestr.next(0,','));
    }
    if (namestr.length() == 0)
	fxFatal("Bad (null) user name.");
    senderNames.append(namestr);
}

#include <dirent.h>

void
faxStatApp::addAllHosts()
{
    char* cp = getenv("FAXSERVER");
    fxBool foundEnv = (cp != 0);
    if (foundEnv)
	clients.append(newClient(cp));

    if (::chdir(FAX_SPOOLDIR) < 0) {
nofiles:
	if (!foundEnv)
	    fxFatal("No FAXSERVER environment variable and no spool directory:\n"
		"What hosts do you want to query?"
	    );
	else
	    return;
    }
    DIR* dirp = opendir(FAX_ETCDIR);
    if (!dirp)
	goto nofiles;
    for (dirent* dp = readdir(dirp); dp; dp = readdir(dirp)) {
	if (strncmp(dp->d_name, FAX_REMOTEPREF, strlen(FAX_REMOTEPREF))
	== 0) {
	    cp = strchr(dp->d_name, '.');
	    if (cp && *++cp)
		clients.append(newClient(cp));
	} else
	if (!foundEnv
	&&  strncmp(dp->d_name, FAX_CONFIGPREF, strlen(FAX_CONFIGPREF))
	== 0) {
	    // XXX should probably use something in the gethostbyname
	    // family to decide whether FAXSERVER == localhost
	    // and append localhost if there are config files and
	    // FAXSERVER != localhost.
	    clients.append(newClient("localhost"));
	    foundEnv = TRUE;
	}
    }
    closedir(dirp);
}

void
faxStatApp::printServerStatus(FILE* fp)
{
    if (serverStatus.length() == 0) {
	if (!multipleClients) {
	    const fxStr& modem = clients[0]->getModem();
	    if (modem == "")
		fprintf(fp, "No servers active on \"%s\".\n",
		    (char*) clients[0]->getHost());
	    else
		fprintf(fp, "No server active on \"%s:%s\".\n",
		    (char*) clients[0]->getHost(), (char*) modem);
	} else
	    fprintf(fp, "No servers active on hosts.\n");
    } else {
	for (u_int i = 0; i < serverStatus.length(); i++)
	    serverStatus[i].print(fp);
    }
}

void
faxStatApp::printSendStatus(FILE* fp)
{
    // sort and print send job status
    sendStatus.qsort(0, sendStatus.length());
    FaxSendStatus::printHeader(fp, ncols, multipleClients);
    for (u_int i = 0; i < sendStatus.length(); i++)
	sendStatus[i].print(fp, ncols, multipleClients);
}

void
faxStatApp::printRecvStatus(FILE* fp)
{
    // sort and print receive status
    recvStatus.qsort(0, recvStatus.length());
    FaxRecvStatus::printHeader(fp, multipleClients);
    for (u_int i = 0; i < recvStatus.length(); i++)
	recvStatus[i].print(fp, multipleClients);
}

#include <Dispatch/dispatcher.h>

extern "C" int
main(int argc, char** argv)
{
    faxStatApp app;
    app.initialize(argc, argv);
    app.open();
    while (app.isRunning())
	Dispatcher::instance().dispatch();
    app.printServerStatus(stdout);
    if (app.hasSendStatus()) {
	printf("\n");
	app.printSendStatus(stdout);
    }
    if (app.hasRecvStatus()) {
	printf("\n");
	app.printRecvStatus(stdout);
    }
    return 0;
}
