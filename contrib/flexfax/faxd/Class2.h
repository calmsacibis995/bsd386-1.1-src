/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/Class2.h,v 1.1.1.1 1994/01/14 23:09:43 torek Exp $
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
#ifndef _CLASS2_
#define	_CLASS2_
/*
 * Class 2-style Modem Driver.
 */
#include "FaxModem.h"
#include <stdarg.h>

class Class2Modem : public FaxModem {
protected:
    fxStr	cqCmds;			// copy quality setup commands
    Class2Params params;		// current params during send
    char	recvDataTrigger;	// char to send to start recv'ing data
    char	hangupCode[4];		// hangup reason (from modem)
    long	group3opts;		// for writing received TIFF

    static const char* ppmCodeName(int code);
    static const char* pprCodeName(int code);

// modem setup stuff
    virtual fxBool setupModem();
    virtual fxBool setupModel(fxStr& model);
    virtual fxBool setupRevision(fxStr& rev);
    virtual fxBool setupDCC();
    virtual fxBool setupClass2Parameters();
// transmission support
    fxBool	sendPage(TIFF* tif);
    fxBool	sendEOT();
    fxBool	dataTransfer();
    fxBool	pageDone(u_int ppm, u_int& ppr);
    void	abortDataTransfer();
// reception support
    const AnswerMsg* findAnswer(const char*);
    fxBool	recvDCS(const char*);
    fxBool	recvPageData(TIFF*);
    fxBool	recvPPM(TIFF*, int& ppr);
    void	recvData(TIFF*, u_char* buf, int n);
    fxBool	parseFPTS(TIFF*, const char* cp, int& ppr);
// miscellaneous
    enum {			// Class 2-specific AT responses
	AT_FHNG		= 100,	// "+FHNG:" (remote hangup)
	AT_FCON		= 101,	// "+FCON"
	AT_FPOLL	= 102,	// "+FPOLL"
	AT_FDIS		= 103,	// "+FDIS:"
	AT_FNSF		= 104,	// "+FNSF:"
	AT_FCSI		= 105,	// "+FCSI:"
	AT_FPTS		= 106,	// "+FPTS:"
	AT_FDCS		= 107,	// "+FDCS:"
	AT_FNSS		= 108,	// "+FNSS:"
	AT_FTSI		= 109,	// "+FTSI:"
	AT_FET		= 110,	// "+FET:"
    };
    virtual ATResponse atResponse(char* buf, long ms = 30*1000);
    fxBool	waitFor(ATResponse wanted, long ms = 30*1000);
    fxStr	stripQuotes(const char*);
// hangup processing
    void	processHangup(const char*);
    fxBool	isNormalHangup();
    const char*	hangupCause(const char* code);
// class 2 command support routines
    fxBool	class2Cmd(const char* cmd, const Class2Params& p);
    fxBool	class2Cmd(const char* cmd);
    fxBool	class2Cmd(const char* cmd, int a0);
    fxBool	class2Cmd(const char* cmd, const char* s);
// parsing routines for capability&parameter strings
    fxBool	parseClass2Capabilities(const char* cap, Class2Params&);
    fxBool	parseRange(const char*, Class2Params&);
public:
    Class2Modem(FaxServer&, const ModemConfig&);
    virtual ~Class2Modem();

// send support
    CallStatus	dialResponse();
    fxBool	getPrologue(Class2Params&, u_int& nsf, fxStr&, fxBool& hasDoc);
    void	sendBegin();
    fxBool	sendPhaseB(TIFF* tif, Class2Params&, FaxMachineInfo&,
		    fxStr& pph, fxStr& emsg);

// receive support
    fxBool	recvBegin(fxStr& emsg);
    fxBool	recvPage(TIFF*, int& ppm, fxStr& emsg);
    fxBool	recvEnd(fxStr& emsg);

// polling support
    fxBool	requestToPoll();
    fxBool	pollBegin(const fxStr& pollID, fxStr& emsg);

// miscellaneous
    fxBool	dataService();			// establish data service
    fxBool	voiceService();			// establish voice service
    fxBool	reset(long ms = 5*1000);	// reset modem
    fxBool	abort(long ms = 5*1000);	// abort session
    void	setLID(const fxStr& number);	// set local id string
};
#endif /* _CLASS2_ */
