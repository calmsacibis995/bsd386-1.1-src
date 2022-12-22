/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxstat/FaxStatClient.c++,v 1.1.1.1 1994/01/14 23:09:56 torek Exp $
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

#include "FaxStatClient.h"
#include "SendStatus.h"
#include "RecvStatus.h"
#include "ServerStatus.h"

// FaxStatClient public methods

int FaxStatClient::clientsDone = 0;

FaxStatClient::FaxStatClient(const char* host,
    FaxSendStatusArray& sa, FaxRecvStatusArray& ra, FaxServerStatusArray& va)
    : FaxClient(host)
    , sendStatus(sa)
    , recvStatus(ra)
    , serverStatus(va)
{
}

FaxStatClient::~FaxStatClient()
{
}

fxBool
FaxStatClient::start(fxBool debug)
{
    if (debug)
	setFds(0, 1);		// use stdin, stdout instead of socket
    else if (callServer() == FaxClient::Failure) {
	fprintf(stderr,
	    "Could not call server on host %s", (char*) getHost());
	return (FALSE);
    }
    return (TRUE);
}

#define	isCmd(s)	(strcasecmp(s, cmd) == 0)

void
FaxStatClient::recvConf(const char* cmd, const char* tag)
{
    if (isCmd("server")) {
	FaxServerStatus stat;
	if (stat.parse(tag)) {
	    stat.host = getHost();
	    serverStatus.append(stat);
	}
    } else if (isCmd("jobStatus")) {
	if (!parseSendStatus(tag)) {
	    fprintf(stderr, "Malformed statusMessage \"%s\"\n", tag);
	    goto bad;
	}
    } else if (isCmd("jobAtStatus")) {
	if (!parseSendAtStatus(tag)) {
	    fprintf(stderr, "Malformed statusMessage \"%s\"\n", tag);
	    goto bad;
	}
    } else if (isCmd("recvJob")) {
	if (!parseRecvStatus(tag)) {
	    fprintf(stderr, "Malformed statusMessage \"%s\"\n", tag);
	    goto bad;
	}
    } else if (isCmd("error")) {
	printf("%s\n", tag);
    } else if (isCmd(".")) {
bad:
	hangupServer();
	clientsDone++;
    } else {
	printf("Unknown status message \"%s:%s\"\n", cmd, tag);
    }
}

fxBool
FaxStatClient::parseSendStatus(const char* tag)
{
    FaxSendStatus stat;

    if (stat.parse(tag)) {
	stat.host = getHost();
	sendStatus.append(stat);
	return (TRUE);
    } else
	return (FALSE);
}

fxBool
FaxStatClient::parseSendAtStatus(const char* tag)
{
    FaxSendAtStatus stat;

    if (stat.parse(tag)) {
	stat.host = getHost();
	sendStatus.append(stat);
	return (TRUE);
    } else
	return (FALSE);
}

fxBool
FaxStatClient::parseRecvStatus(const char* tag)
{
    FaxRecvStatus stat;

    if (stat.parse(tag)) {
	stat.host = getHost();
	recvStatus.append(stat);
	return (TRUE);
    } else
	return (FALSE);
}

void
FaxStatClient::recvEof()
{
    handleEof(0, FALSE);
}

void
FaxStatClient::recvError(const int err)
{
    handleEof(err, TRUE);
}

// FaxStatClient private methods

void
FaxStatClient::handleEof(const int err, const fxBool isError)
{
    if (isError)
	printf("Socket read error: %s\n", strerror(err));
    (void) hangupServer();
    clientsDone++;
}

fxIMPLEMENT_PtrArray(FaxStatClientArray, FaxStatClient*);
