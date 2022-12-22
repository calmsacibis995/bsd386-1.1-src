/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/util/FaxClient.h,v 1.1.1.1 1994/01/14 23:10:49 torek Exp $
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
#ifndef _FaxClient_
#define	_FaxClient_

#include "Types.h"
#include "Str.h"
#include <Dispatch/iohandler.h>

typedef unsigned int FaxClientRC;

class FaxClient : public IOHandler {
private:
    fxStr	host;		// server's host
    fxStr	modem;		// server's modem
    fxBool	verbose;	// print data as sent or received
    fxBool	running;	// client is still running
    fxBool	peerdied;	// server went away
    fxStr	userName;	// sender's account name
    fxStr	senderName;	// sender's full name (if available)
    int		fd;		// input socket
    int		fdOut;		// normally same as fd
    char	buf[1024];	// input buffer
    int		prevcc;		// previous unprocessed input
    u_int	version;	// protocol version

    void init();
protected:
    FaxClient();
    FaxClient(const fxStr& hostarg);
    FaxClient(const char* hostarg);
    ~FaxClient();

    virtual void setupUserIdentity();
    virtual void setProtocolVersion(u_int);
    virtual void setupHostModem(const char*);
    virtual void setupHostModem(const fxStr&);

    virtual void startRunning();
    virtual void stopRunning();

    virtual void recvConf(const char* cmd, const char* tag) = 0;
    virtual void recvEof() = 0;
    virtual void recvError(const int err) = 0;
public:
    enum { Failure = -1, Success = 0 };

    fxBool isRunning() const;
    fxBool getPeerDied() const;

    // bookkeeping
    void setHost(const fxStr&);
    void setHost(const char*);
    const fxStr& getHost() const;
    virtual void setModem(const fxStr&);
    virtual void setModem(const char*);
    const fxStr& getModem() const;
    virtual FaxClientRC callServer();
    virtual FaxClientRC hangupServer();
    virtual void setFds(const int in, const int out);

    void setVerbose(fxBool);
    fxBool getVerbose() const;

    u_int getProtocolVersion() const;

    const fxStr& getSenderName() const;
    const fxStr& getUserName() const;

    // output
    virtual void sendData(const char* type, const char* filename);
    virtual void sendLine(const char* cmd);
    virtual void sendLine(const char* cmd, int v);
    virtual void sendLine(const char* cmd, const fxStr& s);
    virtual void sendLine(const char* cmd, const char* tag);

    // input
    virtual int inputReady(int);
};
inline fxBool FaxClient::isRunning() const		{ return running; }
inline fxBool FaxClient::getPeerDied() const		{ return peerdied; }
inline const fxStr& FaxClient::getSenderName() const	{ return senderName; }
inline const fxStr& FaxClient::getUserName() const	{ return userName; }
inline u_int FaxClient::getProtocolVersion() const	{ return version; }
inline const fxStr& FaxClient::getHost() const		{ return host; }
inline const fxStr& FaxClient::getModem() const		{ return modem; }
inline fxBool FaxClient::getVerbose() const		{ return verbose; }

extern void fxFatal(const char* fmt, ...);
#endif /* _FaxClient_ */
