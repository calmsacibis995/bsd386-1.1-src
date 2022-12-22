/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/Everex.h,v 1.1.1.1 1994/01/14 23:09:43 torek Exp $
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
#ifndef _EVEREX_
#define	_EVEREX_
/*
 * ``Old'' Everex Class 1-style Modem Driver.
 */
#include "FaxModem.h"
#include <setjmp.h>

class EverexModem : public FaxModem {
private:
    enum ModemType {		// FaxModemType
	UNKNOWN,
	EV958,			// old Everex 4800 baud
	EV968,			// new Everex 9600 baud
	ABATON,			// generic for EV958 or EV968
    };
    ModemType	type;			// deduce modem type
    fxBool	is2D;			// page being sent/recvd has 2D data
    fxBool	prevPage;		// a previous page was received
    fxBool	pageGood;		// quality of last page received
    u_int	dis;			// current remote DIS
    Class2Params params;		// current params during send
    fxStr	lid;			// local id string in modem format
// for reception
    int		shdata;			// current input byte
    int		shbit;			// bit mask for current input byte
    jmp_buf	recvEOF;		// for unexpected EOF on recv

    static int modemRateCodes[8];	// map Class 2 to modem parameter
    static int modemScanCodes[8];	// map Class 2 to modem parameter
    static u_int modemPFMCodes[8];	// map Class 2 to T.30 FCF code
    static const u_int modemPPMCodes[8];// map T.30 FCF to Class 2 PPM
    static const char* ppmNames[16];	// post-page-message descriptions

// modem setup and configuration
    fxBool	setupModem();
// send support
    fxBool	sendTraining(Class2Params& params, fxStr& emsg);
    fxBool	dropToNextBR(Class2Params& params);
    fxBool	waitForCarrierToDrop(fxStr& emsg);
    fxBool	sendPPM(int ppm, fxStr& emsg);
    fxBool	sendPage(TIFF* tif, const Class2Params&, fxStr& emsg);
    fxBool	sendRTC(fxBool is2D);
// receive support
    fxBool	recvIdentification(u_int f1, u_int f2, fxStr& emsg);
    fxBool	recvDCSFrames(const char* cp, u_int& fcf);
    fxBool	recvTSI();
    fxBool	recvDCS();
    fxBool	recvNSS();
    fxBool	recvTraining();
    fxBool	recvPageData(TIFF*, fxStr& emsg);
    void	recvDone(TIFF*, u_long totbytes, u_long nrows,
		    u_long bad, int maxbad);
    void	recvCode(int& len, int& code);
    int		recvBit();
    fxBool	recvPPM(int& ppm, fxStr& emsg);
    fxBool	processPPM(const char* buf, int& ppm, fxStr& emsg);
// miscellaneous
    fxBool	modemFaxConfigure(int bits);
    void	encodeCSI(fxStr& binary, const fxStr& ascii);
    void	decodeCSI(fxStr& ascii, const fxStr& binary);
// HDLC frame support
    fxBool	sendFrame(int f1);
    fxBool	sendFrame(int f1, int f2);
    fxBool	sendFrame(int f1, int f2, int f3);
    fxBool	setupFrame(int f, int v);
    fxBool	setupFrame(int f, const char* v);
    fxBool	recvFrame(char* buf, long ms = 10*1000);
public:
    EverexModem(FaxServer& s, const ModemConfig&);
    virtual ~EverexModem();

// send support
    void	sendBegin();
    CallStatus	dialResponse();
    fxBool	getPrologue(Class2Params&, u_int& nsf, fxStr&, fxBool& hasDoc);
    void	sendSetupPhaseB();
    fxBool	sendPhaseB(TIFF* tif, Class2Params&, FaxMachineInfo&,
		    fxStr& pph, fxStr& emsg);
    void	sendEnd();

// receive support
    CallType	answerCall(AnswerType, fxStr& emsg);
    fxBool	recvBegin(fxStr& emsg);
    fxBool	recvPage(TIFF*, int& ppm, fxStr& emsg);
    fxBool	recvEnd(fxStr& emsg);

// polling support
    fxBool	requestToPoll();
    fxBool	pollBegin(const fxStr& pollID, fxStr& emsg);

// miscellaneous
    fxBool	dataService();			// establish data service
    fxBool	reset(long ms = 5*1000);	// reset modem
    fxBool	abort(long ms = 5*1000);	// abort session
    void	setLID(const fxStr& number);	// set local id string
    fxBool	supportsEOLPadding() const;	// modem supports EOL padding
};
#endif /* _EVEREX_ */
