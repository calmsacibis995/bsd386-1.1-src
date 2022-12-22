/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/util/SendFaxClient.c++,v 1.1.1.1 1994/01/14 23:10:47 torek Exp $
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
#include <stdarg.h>
#include <ctype.h>
#include <paths.h>

#include <osfcn.h>
#include <unistd.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <string.h>
extern "C" {
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>		// XXX
}

#include <Dispatch/dispatcher.h>

#include "SendFaxClient.h"
#include "TypeRules.h"
#include "DialRules.h"
#include "PageSize.h"
#include "config.h"

struct FileInfo : public fxObj {
    fxStr	name;
    const TypeRule* rule;
    fxBool	isTemp;
    fxBool	tagLine;

    FileInfo();
    ~FileInfo();
};
fxDECLARE_ObjArray(FileInfoArray, FileInfo);


const fxStr SendFaxClient::TypeRulesFile(FAX_TYPERULES);

SendFaxClient::SendFaxClient()
{
    typeRules = NULL;
    dialRules = NULL;
    files = new FileInfoArray;
    pollCmd = FALSE;
    coverSheet = TRUE;
    gotPermission = FALSE;
    permission = FALSE;
    verbose = FALSE;
    killtime = "now + 1 day";	// default time to kill the job
    hres = 204;			// G3 standard
    vres = FAX_DEFVRES;		// default resolution
    pageWidth = 0;
    pageLength = 0;
    totalPages = 0;
    maxRetries = -1;
    notify = no_notice;		// default is no email notification
    setup = FALSE;
}

SendFaxClient::~SendFaxClient()
{
    u_int i;
    for (i = 0; i < coverPages.length(); i++)
	unlink((char*) coverPages[i]);
    for (i = 0; i < tempFiles.length(); i++)
	unlink((char*) tempFiles[i]);
    delete typeRules;
    delete dialRules;
    delete files;
}

fxBool
SendFaxClient::prepareSubmission()
{
    u_int i, n;

    if (!setupSenderIdentity(from))
	return (FALSE);
    if (pageSize == "" && !setPageSize("default"))
	return (FALSE);
    typeRules = TypeRules::read(fxStr(FAX_LIBDATA) | "/" | TypeRulesFile);
    if (!typeRules) {
	printError("Unable to setup file typing and conversion rules");
	return (FALSE);
    }
    typeRules->setVerbose(verbose);
    dialRules = new DialStringRules(fxStr(FAX_LIBDATA) | "/" | FAX_DIALRULES);
    dialRules->setVerbose(verbose);
    if (!dialRules->parse() && verbose)		// NB: not fatal
	printError("Warning, unable to setup dialstring rules");
    for (i = 0, n = files->length(); i < n; i++)
	if (!handleFile((*files)[i]))
	    return (FALSE);
    /*
     * Convert dialstrings to a displayable format.  This
     * deals with problems like calling card access codes
     * getting stuck on the cover sheet and/or displayed in
     * status messages.
     */
    externalNumbers.resize(destNumbers.length());
    for (i = 0, n = destNumbers.length(); i < n; i++)
	externalNumbers[i] = dialRules->displayNumber(destNumbers[i]);
    /*
     * Suppress the cover page if we're just doing a poll;
     * otherwise, generate a cover sheet for each destination
     * (We do it now so that we can be sure everything is ready
     * to send before we setup a connection to the server.)
     */
    if (pollCmd && files->length() == 0)
	coverSheet = FALSE;
    if (coverSheet) {
	coverPages.resize(externalNumbers.length());
	for (i = 0, n = externalNumbers.length(); i < n; i++)
	    coverPages[i] = makeCoverPage(destNames[i], externalNumbers[i],
		destCompanys[i], destLocations[i], senderName);
    }
    return (setup = TRUE);
}

fxBool
SendFaxClient::submitJob()
{
    if (!setup)
	return (FALSE);
    if (callServer() == FaxClient::Failure) {
	printError("Could not call server");
	return (FALSE);
    }
    startRunning();
    /*
     * Explicitly check for permission to submit a job
     * before sending the input documents.  This way we
     * we avoid sending a load of stuff just to find out
     * that the user/host is not permitted to submit jobs.
     */
    sendLine("checkPerm", "send");
    permission = gotPermission = FALSE;
    while (isRunning() && !getPeerDied() && !gotPermission)
	Dispatcher::instance().dispatch();
    if (!permission)
	return (FALSE);

    /*
     * Transfer the document files first so that they
     * can be referenced multiple times for different
     * destinations.
     */
    for (u_int i = 0; i < files->length(); i++) {
	const FileInfo& info = (*files)[i];
	sendData(info.rule->getResult() == TypeRule::TIFF ?
	    "tiff" : "postscript", info.name);
    }
    if (pollCmd)
	sendLine("poll", "");
    for (i = 0; i < destNumbers.length(); i++) {
	sendLine("begin", i);
	if (sendtime != "")
	    sendLine("sendAt", sendtime);
	if (maxRetries >= 0)
	    sendLine("maxdials", maxRetries);
	if (killtime != "")
	    sendLine("killtime", killtime);
	/*
	 * If the dialstring is different from the
	 * displayable number then pass both.
	 */
	if (destNumbers[i] != externalNumbers[i]) {
	    /*
	     * XXX
	     * We should bump the protocol version and
	     * warn the submitter if this isn't supported
	     * by the server.
	     */
	    sendLine("external", externalNumbers[i]);
	}
	sendLine("number", destNumbers[i]);
	sendLine("sender", senderName);
	sendLine("mailaddr", mailbox);
	if (jobtag != "")
	    sendLine("jobtag", jobtag);
	sendLine("resolution", (int) vres);
	sendLine("pagewidth", (int) pageWidth);
	sendLine("pagelength", (int) pageLength);
	if (notify == when_done)
	    sendLine("notify", "when done");
	else if (notify == when_requeued)
	    sendLine("notify", "when requeued");
	if (coverSheet) {
	    FILE* fp = fopen(coverPages[i], "r");
	    if (fp != NULL) {
		sendLine("cover", 1);	// cover sheet sub-protocol version 1
		// copy prototype cover page
		char line[1024];
		while (fgets(line, sizeof (line)-1, fp)) {
		    char* cp = strchr(line, '\n');
		    if (cp)
			*cp = '\0';
		    sendCoverLine("%s", line);
		}
		sendLine("..\n");
		fclose(fp);
	    } else
		printError("Beware, the cover page for \"%s\" disappeared; "
		    "none was sent", (char*) destNumbers[i]);
	}
	sendLine("end", i);
    }
    sendLine(".\n");
    return (TRUE);
}

u_int SendFaxClient::getNumberOfDestinations() const
   { return destNames.length(); }
u_int SendFaxClient::getNumberOfFiles() const	{ return files->length(); }
u_int SendFaxClient::getTotalPages() const	{ return totalPages; }

void SendFaxClient::setCoverComments(const char* s)	{ comments = s; }
const fxStr& SendFaxClient::getCoverComments() const	{ return comments; }
void SendFaxClient::setCoverRegarding(const char* s)	{ regarding = s; }
const fxStr& SendFaxClient::getCoverRegarding() const	{ return regarding; }
void SendFaxClient::setCoverSheet(fxBool b)		{ coverSheet = b; }
void SendFaxClient::setResolution(float r)		{ vres = r; }
void SendFaxClient::setPollRequest(fxBool b)		{ pollCmd = b; }
fxBool SendFaxClient::getPollRequest() const		{ return pollCmd; }
void SendFaxClient::setNotification(FaxNotify n)	{ notify = n; }
void SendFaxClient::setKillTime(const char* s)		{ killtime = s; }
void SendFaxClient::setSendTime(const char* s)		{ sendtime = s; }
void SendFaxClient::setFromIdentity(const char* s)	{ from = s; }
void SendFaxClient::setJobTag(const char* s)		{ jobtag = s; }
const fxStr& SendFaxClient::getFromIdentity() const	{ return from; }
void SendFaxClient::setMaxRetries(int n)		{ maxRetries = n; }
void SendFaxClient::setVerbose(fxBool b)		{ verbose = b; }
fxBool SendFaxClient::getVerbose() const		{ return verbose; }

fxBool
SendFaxClient::setPageSize(const char* name)
{
    PageSizeInfo* info = PageSizeInfo::getPageSizeByName(name);
    if (info) {
	pageWidth = info->width();
	pageLength = info->height();
	pageSize = name;
	delete info;
	return (TRUE);
    } else {
	printError("Unknown page size \"%s\"", name);
	return (FALSE);
    }
}
const fxStr& SendFaxClient::getPageSize() const	{ return pageSize; }
float SendFaxClient::getPageWidth() const	{ return pageWidth; }
float SendFaxClient::getPageLength() const	{ return pageLength; }

/*
 * Add a new destination name and number.
 */
fxBool
SendFaxClient::addDestination(
    const char* person,
    const char* faxnum,
    const char* company,
    const char* location
)
{
    if (!faxnum || faxnum[0] == '\0') {
	printError("No fax number specified");
	return (FALSE);
    }
    destNumbers.append(faxnum);
    destNames.append(person ? person : "");
    destLocations.append(location ? location : "");
    destCompanys.append(company ? company : "");
    return (TRUE);
}

/*
 * Add a new file to send to each destination.
 */
void
SendFaxClient::addFile(const char* filename)
{
    u_int i = files->length();
    files->resize(i+1);
    (*files)[i].name = filename;
}

/*
 * Check parsed sender identity against identity obtained
 * by looking at the real uid.  If they're different and
 * we are not running as a "trusted user", abort.
 */
fxBool
SendFaxClient::verifyPermission()
{
    uid_t uid = getuid();
    if (uid == 0)
	return (TRUE);
    passwd* pwd = getpwuid(uid);
    fxStr name(pwd ? pwd->pw_name : "nobody");
    if (name == "daemon" || name == "uucp" || name == "fax")
	return (TRUE);
    // check specified address against real uid's address
    fxStr user(mailbox.head(mailbox.next(0, '@')));
    user.remove(0, user.nextR(user.length(), '!'));
    if (user != getUserName()) {
	printError(
	    "Unauthorized attempt to set sender's identity (from %s, uid %s)",
	    (char*) user, (char*) getUserName());
	return (FALSE);
    }
    return (TRUE);
}

/*
 * Create the mail address for a local user.
 */
void
SendFaxClient::setMailbox(const char* user)
{
    fxStr acct(user);
    if (acct.next(0, '@') == acct.length()) {
	char hostname[64];
	(void) gethostname(hostname, sizeof (hostname));
	struct hostent* hp = gethostbyname(hostname);
	mailbox = acct | "@" | (hp ? hp->h_name : hostname);
    } else
	mailbox = acct;
}
const fxStr& SendFaxClient::getMailbox() const		{ return mailbox; }

/*
 * Setup the sender's identity.
 */
fxBool
SendFaxClient::setupSenderIdentity(const fxStr& from)
{
    FaxClient::setupUserIdentity();		// client identity

    if (from != "") {
	u_int l = from.next(0, '<');
	if (l == from.length()) {
	    l = from.next(0, '(');
	    if (l != from.length()) {		// joe@foobar (Joe Schmo)
		mailbox = from.head(l);
		l++; senderName = from.token(l, ')');
	    } else {				// joe
		setMailbox(from);
		if (from == getUserName())
		    senderName = FaxClient::getSenderName();
		else
		    senderName = "";
	    }
	} else {				// Joe Schmo <joe@foobar>
	    senderName = from.head(l);
	    l++; mailbox = from.token(l, '>');
	}
	if (senderName == "" && mailbox != "") {
	    /*
	     * Mail address, but no "real name"; construct one from
	     * the account name.  Do this by first stripping anything
	     * to the right of an '@' and then stripping any leading
	     * uucp patch (host!host!...!user).
	     */
	    senderName = mailbox;
	    senderName.resize(senderName.next(0, '@'));
	    senderName.remove(0, senderName.nextR(senderName.length(), '!'));
	}

	// strip and leading&trailing white space
	senderName.remove(0, senderName.skip(0, " \t"));
	senderName.resize(senderName.skipR(senderName.length(), " \t"));
	mailbox.remove(0, mailbox.skip(0, " \t"));
	mailbox.resize(mailbox.skipR(mailbox.length(), " \t"));
	if (senderName == "" || mailbox == "") {
	    printError("Malformed (null) sender name or mail address");
	    return (FALSE);
	}

	if (!verifyPermission())
	    return (FALSE);
    } else {
	senderName = FaxClient::getSenderName();
	setMailbox(getUserName());
    }
    return (TRUE);
}
const fxStr& SendFaxClient::getSenderName() const	{ return senderName; }

/*
 * Process a file submitted for transmission.
 */
fxBool
SendFaxClient::handleFile(FileInfo& info)
{
    info.rule = fileType(info.name);
    if (!info.rule)
	return (FALSE);
    if (info.rule->getCmd() != "") {	// conversion required
	fxStr temp(_PATH_TMP "faxsndXXXXXX");
	tempFiles.append(temp);
	mktemp((char*) temp);
	fxStr sysCmd = info.rule->getFmtdCmd(info.name, temp,
		hres, vres, "1", pageSize);
	if (verbose)
	    printf("CONVERT \"%s\"\n", (char*) sysCmd);
	if (system((char*) sysCmd) != 0) {
	    unlink((char*) temp);
	    u_int ix = tempFiles.find(temp);
	    if (ix != fx_invalidArrayIndex)
		tempFiles.remove(ix);
	    printError("Error converting data; command was \"%s\"",
		(char*) sysCmd);
	    return (FALSE);
	}
	info.name = temp;
	info.isTemp = TRUE;
    } else				// already postscript or tiff
	info.isTemp = FALSE;
    switch (info.rule->getResult()) {
    case TypeRule::TIFF:
	countTIFFPages(info.name);
	break;
    case TypeRule::POSTSCRIPT:
	estimatePostScriptPages(info.name);
	break;
    }
    return (TRUE);
}

/*
 * Return a TypeRule for the specified file.
 */
const TypeRule*
SendFaxClient::fileType(const char* filename)
{
    struct stat sb;
    int fd = ::open(filename, O_RDONLY);
    if (fd < 0) {
	printError("%s: Can not open file", filename);
	return (NULL);
    }
    if (fstat(fd, &sb) < 0) {
	printError("%s: Can not stat file", filename);
	::close(fd);
	return (NULL);
    }
    if ((sb.st_mode & S_IFMT) != S_IFREG) {
	printError("%s: Not a regular file", filename);
	::close(fd);
	return (NULL);
    }
    char buf[512];
    int cc = read(fd, buf, sizeof (buf));
    ::close(fd);
    if (cc == 0) {
	printError("%s: Empty file", filename);
	return (NULL);
    }
    const TypeRule* tr = typeRules->match(buf, cc);
    if (!tr) {
	printError("%s: Can not determine file type", filename);
	return (NULL);
    }
    if (tr->getResult() == TypeRule::ERROR) {
	fxStr emsg(tr->getErrMsg());
	printError("%s: %s", filename, (char*) emsg);
	return (NULL);
    }
    return tr;   
}

#include "tiffio.h"

/*
 * Count the number of ``pages'' in a TIFF file.
 */
void
SendFaxClient::countTIFFPages(const char* filename)
{
    TIFF* tif = TIFFOpen(filename, "r");
    if (tif) {
	do {
	    totalPages++;
	} while (TIFFReadDirectory(tif));
	TIFFClose(tif);
    }
}

/*
 * Count the number of pages in a PostScript file.
 * We can really only estimate the number as we
 * depend on the DSC comments to figure this out.
 */
void
SendFaxClient::estimatePostScriptPages(const char* filename)
{
    FILE* fd = fopen(filename, "r");
    if (fd != NULL) {
	char line[2048];
	if (fgets(line, sizeof (line)-1, fd) != NULL) {
	    /*
	     * We only consider ``conforming'' PostScript documents.
	     */
	    if (line[0] == '%' && line[1] == '!') {
		int npagecom = 0;	// # %%Page comments
		int npages = 0;		// # pages according to %%Pages comments
		while (fgets(line, sizeof (line)-1, fd) != NULL) {
		    int n;
		    if (strncmp(line, "%%Page:", 7) == 0)
			npagecom++;
		    else if (sscanf(line, "%%%%Pages: %u", &n) == 1)
			npages += n;
		}
		/*
		 * Believe %%Pages comments over counting of %%Page comments.
		 */
		if (npages > 0)
		    totalPages += npages;
		else if (npagecom > 0)
		    totalPages += npagecom;
	    }
	}
	fclose(fd);
    }
}

/*
 * Invoke the cover page generation program.
 */
fxStr
SendFaxClient::makeCoverPage(
    const fxStr& name,
    const fxStr& number,
    const fxStr& company,
    const fxStr& location,
    const fxStr& sender
)
{
    fxStr templ(_PATH_TMP "sndfaxXXXXXX");
    tempFiles.append(templ);
    int fd = mkstemp((char*) templ);
    if (fd >= 0) {
	fxStr cmd("faxcover");
	if (getPageSize() != "default")
	    cmd.append(" -s " | getPageSize());
	cmd.append(" -n \"" | number | "\"");
	cmd.append(" -t \"" | name | "\"");
	cmd.append(" -f \"" | sender | "\"");
	if (getCoverComments() != "") {
	    cmd.append(" -c \"");
	    cmd.append(getCoverComments());
	    cmd.append("\"");
	}
	if (getCoverRegarding() != "") {
	    cmd.append(" -r \"");
	    cmd.append(getCoverRegarding());
	    cmd.append("\"");
	}
	if (location != "") {
	    cmd.append(" -l \"");
	    cmd.append(location);
	    cmd.append("\"");
	}
	if (company != "") {
	    cmd.append(" -x \"");
	    cmd.append(company);
	    cmd.append("\"");
	}
	if (getTotalPages() > 0) {
	    fxStr pages((int) getTotalPages(), "%u");
	    cmd.append(" -p " | pages);
	}
	if (getVerbose())
	    printf("COVER SHEET \"%s\"\n", (char*) cmd);
	FILE* fp = popen(cmd, "r");
	if (fp != NULL) {
	    // copy prototype cover page
	    char line[1024];
	    while (fgets(line, sizeof (line)-1, fp))
		(void) write(fd, line, strlen(line));
	    if (pclose(fp) == 0) {
		::close(fd);
		return (templ);
	    }
	}
	printError("Error creating cover sheet; command was \"%s\"",
	    (char*) cmd);
    } else
	printError("%s: Can not create temporary file for cover sheet",
	    (char*) templ);
    unlink((char*) templ);
    templ = "";
    return (templ);
}

void
SendFaxClient::sendCoverLine(const char* va_alist ...)
#define	fmt va_alist
{
    va_list ap;
    va_start(ap, va_alist);
    char buf[4096];
    vsprintf(buf, fmt, ap);
    va_end(ap);
    sendLine("!", buf);
}
#undef fmt

#define	isCmd(s)	(strcasecmp(s, cmd) == 0)

void
SendFaxClient::recvConf(const char* cmd, const char* tag)
{
    if (isCmd("permission")) {
	gotPermission = TRUE;
	permission = (strcasecmp(tag, "granted") == 0);
    } else
	printError("Unknown status message received: \"%s:%s\"", cmd, tag);
}

void
SendFaxClient::recvEof()
{
    stopRunning();
}

void
SendFaxClient::recvError(const int err)
{
    printError("Fatal socket read error: %s", strerror(err));
    stopRunning();
}

FileInfo::FileInfo()
{
    isTemp = FALSE;
    tagLine = TRUE;
    rule = NULL;
}
FileInfo::~FileInfo()
{
    if (isTemp)
	unlink((char*) name);
}
fxIMPLEMENT_ObjArray(FileInfoArray, FileInfo);
