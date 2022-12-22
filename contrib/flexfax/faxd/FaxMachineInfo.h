/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/FaxMachineInfo.h,v 1.1.1.1 1994/01/14 23:09:43 torek Exp $
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
#ifndef _FaxMachineInfo_
#define	_FaxMachineInfo_
/*
 * Fax Machine Information Database Support.
 */
#include <stdio.h>
#include "Str.h"

/*
 * Each remote machine the server sends a facsimile to
 * has information that describes capabilities that are
 * important in formatting outgoing documents, and, potentially,
 * controls on what the server should do when presented
 * with documents to send to the destination.  The capabilities
 * are treated as a cache; information is initialized to
 * be a minimal set of capabilities that all machines are
 * required (by T.30) to support and then updated according
 * to the DIS/DTC messages received during send operations.
 */
class FaxMachineCtlInfo {
private:
    fxStr	rejectNotice;		// if set, reject w/ this notice
    int		tracingLevel;		// destination-specific tracing
    // XXX time-of-day restrictions

    static const fxStr ctlDir;
protected:
    FaxMachineCtlInfo();

    void restore(const fxStr& number);
public:
    virtual ~FaxMachineCtlInfo();

    virtual const fxStr& getRejectNotice() const;
    fxBool getTracingLevel(int&) const;
};

class FaxMachineInfo : public FaxMachineCtlInfo {
private:
    FILE*	fp;			// open file
    u_int	locked;			// bit vector of locked items
    fxBool	changed;		// changed since restore
    fxBool	supportsHighRes;	// capable of 7.7 line/mm vres
    fxBool	supports2DEncoding;	// handles Group 3 2D
    fxBool	supportsPostScript;	// handles Adobe NSF protocol
    fxBool	calledBefore;		// successfully called before
    short	maxPageWidth;		// max capable page width
    short	maxPageLength;		// max capable page length
    short	maxSignallingRate;	// max capable signalling rate
    fxStr	csi;			// last received CSI
    fxStr	jobInProgress;		// jobid of send in progress
    fxStr	rejectNotice;		// status of last completed call
    int		sendFailures;		// count of failed send attempts
    int		dialFailures;		// count of failed dial attempts
    fxStr	lastSendFailure;	// reason for last failed send attempt
    fxStr	lastDialFailure;	// reason for last failed dial attempt

    static const fxStr infoDir;

    void restore();
    void update();
public:
    FaxMachineInfo(const fxStr& number, fxBool block);
    ~FaxMachineInfo();

    int operator==(const FaxMachineInfo&) const;
    int operator!=(const FaxMachineInfo&) const;

    fxBool isBusy() const		 	{ return fp == NULL; }

    fxBool getSupportsHighRes() const	 	{ return supportsHighRes; }
    fxBool getSupports2DEncoding() const 	{ return supports2DEncoding; }
    fxBool getSupportsPostScript() const 	{ return supportsPostScript; }
    fxBool getCalledBefore() const	 	{ return calledBefore; }
    int getMaxPageWidth() const		 	{ return maxPageWidth; }
    int getMaxPageLength() const	 	{ return maxPageLength; }
    int getMaxSignallingRate() const	 	{ return maxSignallingRate; }
    const fxStr& getCSI() const 	 	{ return csi; }

    const fxStr& getJobInProgress() const	{ return jobInProgress; }
    virtual const fxStr& getRejectNotice() const;
    int getSendFailures() const			{ return sendFailures; }
    int getDialFailures() const			{ return dialFailures; }
    const fxStr& getLastSendFailure() const	{ return lastSendFailure; }
    const fxStr& getLastDialFailure() const	{ return lastDialFailure; }

    void setSupportsHighRes(fxBool);
    void setSupports2DEncoding(fxBool);
    void setSupportsPostScript(fxBool);
    void setCalledBefore(fxBool);
    void setMaxPageWidth(int);
    void setMaxPageLength(int);
    void setMaxSignallingRate(int);
    void setCSI(const fxStr&);

    void setJobInProgress(const fxStr&);
    void setRejectNotice(const fxStr&);
    void setSendFailures(int);
    void setDialFailures(int);
    void setLastSendFailure(const fxStr&);
    void setLastDialFailure(const fxStr&);
};
#endif /* _FaxMachineInfo_ */
