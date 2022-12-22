/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/sendfax/sendfax.c++,v 1.1.1.1 1994/01/14 23:10:43 torek Exp $
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
#include "SendFaxClient.h"
#include "FaxDB.h"
#include "config.h"

#include "Dispatch/dispatcher.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <paths.h>
#include <pwd.h>
#include <osfcn.h>
extern "C" {
#include <locale.h>
}

class sendFaxApp : public SendFaxClient {
private:
    fxStr	appName;		// for error messages
    fxStr	stdinTemp;		// temp file for collecting from pipe
    FaxDB*	db;
    int		verbose;
    fxStr	company;
    fxStr	location;

    static fxStr dbName;

    void addDestination(const char* cp);
    void copyToTemporary(int fin, fxStr& tmpl);
    fxStr tildeExpand(const fxStr& filename);
    void printError(const char* va_alist ...);
    void fatal(const char* fmt ...);
    void usage();

    virtual void recvConf(const char* cmd, const char* tag);
public:
    sendFaxApp();
    ~sendFaxApp();

    fxBool initialize(int argc, char** argv);
};

fxStr sendFaxApp::dbName("~/.faxdb");

sendFaxApp::sendFaxApp()
{
    db = NULL;
    verbose = 0;
}

sendFaxApp::~sendFaxApp()
{
    if (stdinTemp != "")
	unlink((char*) stdinTemp);
    delete db;
}

fxBool
sendFaxApp::initialize(int argc, char** argv)
{
    extern int optind;
    extern char* optarg;
    int c;

    appName = argv[0];
    u_int l = appName.length();
    appName = appName.tokenR(l, '/');

    db = new FaxDB(tildeExpand(dbName));
    while ((c = getopt(argc, argv, "a:c:d:f:h:i:k:r:s:t:x:y:lmnpvDR")) != -1)
	switch (c) {
	case 'a':			// time at which to transmit job
	    setSendTime(optarg);
	    break;
	case 'c':			// cover sheet: comment field
	    setCoverComments(optarg);
	    break;
	case 'D':			// notify when done
	    setNotification(SendFaxClient::when_done);
	    break;
	case 'd':			// destination name and number
	    addDestination(optarg);
	    break;
	case 'f':			// sender's identity
	    setFromIdentity(optarg);
	    break;
	case 'h':			// server's host
	    setHost(optarg);
	    break;
	case 'i':			// user-specified job identifier
	    setJobTag(optarg);
	    break;
	case 'k':			// time to kill job
	    setKillTime(optarg);
	    break;
	case 'l':			// low resolution
	    setResolution(98.);
	    break;
	case 'm':			// medium resolution
	    setResolution(196.);
	    break;
	case 'n':			// no cover sheet
	    setCoverSheet(FALSE);
	    break;
	case 'p':			// submit polling request
	    setPollRequest(TRUE);
	    break;
	case 'r':			// cover sheet: regarding field
	    setCoverRegarding(optarg);
	    break;
	case 'R':			// notify when requeued or done
	    setNotification(SendFaxClient::when_requeued);
	    break;
	case 's':			// set page size
	    setPageSize(optarg);
	    break;
	case 't':			// times to retry sending
	    setMaxRetries(atoi(optarg));
	    break;
	case 'v':			// verbose mode
	    verbose++;
	    break;
	case 'x':			// cover page: to's company
	    company = optarg;
	    break;
	case 'y':			// cover page: to's location
	    location = optarg;
	    break;
	case '?':
	    usage();
	    /*NOTREACHED*/
	}
    if (getNumberOfDestinations() == 0) {
	printError("No destination specified");
	usage();
    }

    if (verbose)
	setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
    SendFaxClient::setVerbose(verbose > 0);	// type rules & basic operation
    FaxClient::setVerbose(verbose > 1);		// protocol tracing

    if (optind < argc) {
	for (; optind < argc; optind++)
	    addFile(argv[optind]);
    } else if (!getPollRequest()) {
	copyToTemporary(fileno(stdin), stdinTemp);
	addFile(stdinTemp);
    }
    return (prepareSubmission() && submitJob());
}

void
sendFaxApp::usage()
{
    fxFatal("usage: %s"
	" [-d destination]"
	" [-h host[:modem]]"
	" [-k kill-time]\n"
	"    "
	" [-a time-to-send]"
	" [-c comments]"
	" [-x company]"
	" [-y location]"
	" [-r regarding]\n"
	"    "
	" [-f from]"
	" [-i identifier]"
	" [-s pagesize]"
	" [-t max-tries]"
	" [-lmnpvDR]"
	" [files]",
	(char*) appName);
}

/*
 * Add a destination; parse ``person@number'' syntax.
 */
void
sendFaxApp::addDestination(const char* cp)
{
    fxStr recipient;
    const char* tp = strchr(cp, '@');
    if (tp) {
	recipient = fxStr(cp, tp-cp);
	cp = tp+1;
    } else
	recipient = "";
    fxStr dest(cp);
    if (db && dest.length() > 0) {
	fxStr name;
	FaxDBRecord* r = db->find(dest, &name);
	if (r) {
	    if (recipient == "")
		recipient = name;
	    dest = r->find(FaxDB::numberKey);
	}
    }
    if (dest.length() == 0)
	fatal("Null destination for \"%s\"", cp);
    if (!SendFaxClient::addDestination(recipient, dest, company, location))
	fatal("Could not add fax destination");
}

/*
 * Expand a filename that might have `~' in it.
 */
fxStr
sendFaxApp::tildeExpand(const fxStr& filename)
{
    fxStr path(filename);
    if (filename.length() > 1 && filename[0] == '~') {
	path.remove(0);
	char* cp = getenv("HOME");
	if (!cp || *cp == '\0') {
	    struct passwd* pwd = getpwuid(getuid());
	    if (!pwd)
		fatal("Can not figure out who you are");
	    cp = pwd->pw_dir;
	}
	path.insert(cp);
    }
    return (path);
}

/*
 * Copy data from fin to a temporary file.
 */
void
sendFaxApp::copyToTemporary(int fin, fxStr& tmpl)
{
    tmpl = _PATH_TMP "sndfaxXXXXXX";
    int fd = mkstemp((char*) tmpl);
    if (fd < 0)
	fatal("%s: Can not create temporary file", (char*) tmpl);
    int cc, total = 0;
    char buf[16*1024];
    while ((cc = read(fin, buf, sizeof (buf))) > 0) {
	if (write(fd, buf, cc) != cc) {
	    unlink((char*) tmpl);
	    fatal("%s: write error", (char*) tmpl);
	}
	total += cc;
    }
    ::close(fd);
    if (total == 0) {
	unlink((char*) tmpl);
	tmpl = "";
	fatal("No input data; tranmission aborted");
    }
}

void
sendFaxApp::printError(const char* va_alist ...)
#define	fmt va_alist
{
    va_list ap;
    va_start(ap, va_alist);
    fprintf(stderr, "%s: ", (char*) appName);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, ".\n");
}
#undef fmt

#define	isCmd(s)	(strcasecmp(s, cmd) == 0)

void
sendFaxApp::recvConf(const char* cmd, const char* tag)
{
    if (isCmd("job")) {
	u_int len = getNumberOfFiles();
	printf("request id is %s for host %s (%u %s)\n",
	    tag, (char*) getHost(), len, len > 1 ? "files" : "file");
    } else if (isCmd("error")) {
	printf("%s\n", tag);
    } else
	SendFaxClient::recvConf(cmd, tag);
}

#include <signal.h>
#include <setjmp.h>

static	sendFaxApp* app = NULL;

static void
cleanup()
{
    sendFaxApp* a = app;
    app = NULL;
    delete a;
}

static void
sigDone(int)
{
    cleanup();
    exit(-1);
}

int
main(int argc, char** argv)
{
#ifdef LC_CTYPE
    setlocale(LC_CTYPE, "");
#endif
    signal(SIGHUP, fxSIGHANDLER(sigDone));
    signal(SIGINT, fxSIGHANDLER(sigDone));
    signal(SIGTERM, fxSIGHANDLER(sigDone));
    app = new sendFaxApp;
    if (!app->initialize(argc, argv))
	sigDone(0);
    while (app->isRunning())
	Dispatcher::instance().dispatch();
    signal(SIGHUP, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    cleanup();
    return (0);
}

static void
vfatal(FILE* fd, const char* fmt, va_list ap)
{
    vfprintf(fd, fmt, ap);
    va_end(ap);
    fputs(".\n", fd);
    sigDone(0);
}

void
fxFatal(const char* va_alist ...)
#define	fmt va_alist
{
    va_list ap;
    va_start(ap, fmt);
    vfatal(stderr, fmt, ap);
    /*NOTTEACHED*/
}
#undef fmt

void
sendFaxApp::fatal(const char* va_alist ...)
#define	fmt va_alist
{
    fprintf(stderr, "%s: ", (char*) appName);
    va_list ap;
    va_start(ap, fmt);
    vfatal(stderr, fmt, ap);
    /*NOTTEACHED*/
}
#undef fmt
