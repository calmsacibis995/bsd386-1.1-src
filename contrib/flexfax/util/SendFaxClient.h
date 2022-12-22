/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/util/SendFaxClient.h,v 1.1.1.1 1994/01/14 23:10:49 torek Exp $
/*
 * Copyright (c) 1993 Sam Leffler
 * Copyright (c) 1993 Silicon Graphics, Inc.
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
#ifndef _SendFaxClient_
#define	_SendFaxClient_

#include "FaxClient.h"
#include "StrArray.h"

class TypeRule;
class TypeRules;
class DialStringRules;
class PageSizeInfo;
class FileInfo;
class FileInfoArray;

typedef unsigned int FaxNotify;

class SendFaxClient : public FaxClient {
public:
    enum {
	no_notice,
	when_done,
	when_requeued
    };
private:
    fxStrArray	destNames;		// FAX destination names
    fxStrArray	destNumbers;		// FAX destination dialstrings
    fxStrArray	externalNumbers;	// FAX destination displayable numbers
    fxStrArray	destLocations;		// FAX destination locations
    fxStrArray	destCompanys;		// FAX destination companys
    fxStrArray	coverPages;		// cover pages
    TypeRules*	typeRules;		// file type/conversion database
    DialStringRules* dialRules;		// dial string conversion database
    FileInfoArray* files;		// files to send (possibly converted)
    fxBool	pollCmd;		// do poll rather than send
    fxBool	coverSheet;		// if TRUE, prepend cover sheet
    fxBool	gotPermission;		// got response to checkPermission
    fxBool	permission;		// permission granted/denied
    fxBool	verbose;
    fxStr	killtime;		// job's time to be killed
    fxStr	sendtime;		// job's time to be sent
    float	hres, vres;		// sending resolution (dpi)
    float	pageWidth;		// sending page width (mm)
    float	pageLength;		// sending page length (mm)
    fxStr	pageSize;		// arg to pass to subprocesses
    int		totalPages;		// counted pages (for cover sheet)
    int		maxRetries;		// max number times to try send
    FaxNotify	notify;
    fxBool	setup;			// if true, then ready to send
    fxStrArray	tempFiles;		// stuff to cleanup on abort

    fxStr	mailbox;		// user@host return mail address
    fxStr	jobtag;			// user-specified job identifier
    fxStr	comments;		// comments on cover sheet
    fxStr	regarding;		// regarding line on cover sheet
    fxStr	from;			// command line from information
    fxStr	senderName;		// sender's full name

    static const fxStr TypeRulesFile;
protected:
    SendFaxClient();

    virtual fxStr makeCoverPage(const fxStr& name, const fxStr& number,
	const fxStr& company, const fxStr& location,
	const fxStr& senderName);

    virtual void printError(const char* va_alist ...) = 0;

    void countTIFFPages(const char* name);
    void estimatePostScriptPages(const char* name);
    const TypeRule* fileType(const char* filename);
    fxBool handleFile(FileInfo& info);
    fxBool verifyPermission();
    void sendCoverDef(const char* name, const char* value);
    void sendCoverLine(const char* va_alist ...);

    virtual void recvConf(const char* cmd, const char* tag);
    virtual void recvEof();
    virtual void recvError(const int err);
public:
    virtual ~SendFaxClient();

    virtual fxBool prepareSubmission();
    virtual fxBool submitJob();

    fxBool setupSenderIdentity(const fxStr&);
    const fxStr& getSenderName() const;

    fxBool addDestination(const char* person, const char* faxnum,
	const char* company, const char* location);
    u_int getNumberOfDestinations() const;

    void addFile(const char* filename);
    u_int getNumberOfFiles() const;

    fxBool getVerbose() const;
    void setVerbose(fxBool);
    float getPageWidth() const;		// sending page width (mm)
    float getPageLength() const;	// sending page length (mm)
    const fxStr& getPageSize() const;	// arg to pass to subprocesses
    u_int getTotalPages() const;	// counted pages (for cover sheet)

    void setCoverComments(const char*);
    const fxStr& getCoverComments() const;
    void setCoverRegarding(const char*);
    const fxStr& getCoverRegarding() const;
    void setCoverSheet(fxBool);

    void setResolution(float);
    void setPollRequest(fxBool);
    fxBool getPollRequest() const;
    void setNotification(FaxNotify);
    void setKillTime(const char*);
    void setSendTime(const char*);
    void setFromIdentity(const char*);
    void setJobTag(const char*);
    const fxStr& getFromIdentity() const;
    void setMailbox(const char*);
    const fxStr& getMailbox() const;
    fxBool setPageSize(const char* name);
    void setMaxRetries(int);
};
#endif /* _SendFaxClient_ */
