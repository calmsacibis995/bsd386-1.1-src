/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/FaxModem.h,v 1.1.1.1 1994/01/14 23:09:44 torek Exp $
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
#ifndef _FAXMODEM_
#define	_FAXMODEM_
/*
 * Fax Modem Driver Interface.
 */
#include <stdarg.h>
#include "Class2Params.h"
#include "tiffio.h"

class FaxServer;
class FaxMachineInfo;
class ModemConfig;

// NB: these would be enums in the FaxModem class
//     if there were a portable way to refer to them!
typedef unsigned int CallStatus;	// return status from dialing op
typedef	unsigned int CallType;		// type detected for incoming call
typedef	unsigned int AnswerType;	// type of call to answer for
typedef unsigned int SpeakerVolume;
typedef	unsigned int ATResponse;	// response code from AT command
typedef	unsigned int BaudRate;		// serial line communication rate
typedef	unsigned int FlowControl;	// serial line flow control scheme
typedef	unsigned int SetAction;		// how to act when setting line
typedef struct {
    const char*	msg;		// string to match
    u_short	len;		// string length
    ATResponse	expect;		// next AT response to expect
    CallStatus	status;		// resultant call status
    CallType	type;		// resultant call type
} AnswerMsg;

/*
 * AT command escape codes.  Command strings specified in the
 * modem configuration file are parsed and embedded commands
 * are converted to one of the escape codes below.
 */ 
#define	ord(e)	((int)(e))

#define	ESC_BR300	(0x80|ord(FaxModem::BR300))	// 300 bps
#define	ESC_BR1200	(0x80|ord(FaxModem::BR1200))	// 1200 bps
#define	ESC_BR2400	(0x80|ord(FaxModem::BR2400))	// 2400 bps
#define	ESC_BR4800	(0x80|ord(FaxModem::BR4800))	// 4800 bps
#define	ESC_BR9600	(0x80|ord(FaxModem::BR9600))	// 9600 bps
#define	ESC_BR19200	(0x80|ord(FaxModem::BR19200))	// 19200 bps
#define	ESC_BR38400	(0x80|ord(FaxModem::BR38400))	// 38400 bps
#define	ESC_BR57600	(0x80|ord(FaxModem::BR57600))	// 57600 bps
#define	ESC_XON		(0x80|0xf)			// XON/XOFF flow control
#define	ESC_RTS		(0x80|0xe)			// RTS/CTS flow control

/*
 * This is an abstract class that defines the interface to
 * the set of modem drivers.  Real drivers are derived from
 * this and fill in the pure virtual methods to, for example,
 * send specific commands to the modem.  The Class2Params
 * structure defines the session parameters used/supported
 * by this interface.  Class2Params is derived from the
 * set of parameters supported by the Class 2 interface spec.
 */
class FaxModem {
public:
    static FaxModem* deduceModem(FaxServer&, const ModemConfig& config);

    enum {			// FaxModemCallStatus
	OK	   = 0,		// phone answered & carrier received
	BUSY	   = 1,		// destination phone busy
	NOCARRIER  = 2,		// no carrier from remote
	NOANSWER   = 3,		// no answer from remote
	NODIALTONE = 4,		// no local dialtone (phone not plugged in?)
	ERROR	   = 5,		// error in dial command
	FAILURE	   = 6,		// other problem (e.g. modem turned off)
    };
    static const char* callStatus[7];

    enum {			// FaxModemCallType
	CALLTYPE_ERROR	= 0,	// error deducing type of incoming call
	CALLTYPE_DATA	= 1,	// data connection
	CALLTYPE_FAX	= 2,	// fax connection
	CALLTYPE_VOICE	= 3,	// voice connection
	CALLTYPE_UNKNOWN = 4,	// unknown variety
    };

    enum {			// FaxModemSpeakerVolume
	OFF	= 0,		// nothing
	QUIET	= 1,		// somewhere near a dull chirp
	LOW	= 2,		// normally acceptable
	MEDIUM	= 3,		// heard above a stereo
	HIGH	= 4,		// ear splitting
    };

    enum {			// DTE-DCE communication rate
	BR0	= 0,		// force hangup/drop DTR
	BR300	= 1,		// 300 bits/sec
	BR1200	= 2,		// 1200 bits/sec
	BR2400	= 3,		// 2400 bits/sec
	BR4800	= 4,		// 4800 bits/sec
	BR9600	= 5,		// 9600 bits/sec
	BR19200	= 6,		// 19200 bits/sec
	BR38400	= 7,		// 38400 bits/sec
	BR57600	= 8,		// 57600 bits/sec
    };

    enum {
	FLOW_NONE	= 0,	// no flow control
	FLOW_CURRENT	= 1,	// whatever current setting is
	FLOW_XONXOFF	= 2,	// XON/XOFF software flow control
	FLOW_RTSCTS	= 3	// RTS/CTS hardware flow control
    };

    enum {
	ACT_NOW		= 0,	// set terminal parameters now
	ACT_DRAIN	= 1,	// set parameters after draining output queue
	ACT_FLUSH	= 2,	// set parameters after flush both queues
    };

    enum {			// FaxModemAnswerType
	ANSTYPE_ANY	= 0,	// any kind of call
	ANSTYPE_DATA	= 1,	// data call
	ANSTYPE_FAX	= 2,	// fax call
	ANSTYPE_VOICE	= 3,	// voice call
    };

    enum {			// ATResponse
	AT_NOTHING	= 0,	// for passing as a parameter
	AT_OK		= 1,	// "OK" response
	AT_CONNECT	= 2,	// "CONNECT" response
	AT_NOANSWER	= 3,	// "NO ANSWER" response
	AT_NOCARRIER	= 4,	// "NO CARRIER" response
	AT_NODIALTONE	= 5,	// "NO DIALTONE" response
	AT_BUSY		= 6,	// "BUSY" response
	AT_OFFHOOK	= 7,	// "PHONE OFF-HOOK" response
	AT_RING		= 8,	// "RING" response
	AT_ERROR	= 9,	// "ERROR" response
	AT_EMPTYLINE	= 10,	// empty line (0 characters received)
	AT_TIMEOUT	= 11,	// timeout waiting for response
	AT_OTHER	= 12,	// unknown response (not one of above)
    };
private:
    FaxServer&	server;		// server for getting to device
    fxStr	resetCmds;	// commands to use for reset operation
    long	dataTimeout;	// baud rate-dependent data timeout
    BaudRate	rate;		// selected DTE-DCE communication rate
    FlowControl	iFlow;		// input flow control scheme
    FlowControl	oFlow;		// output flow control scheme
    u_char	RTCbuf[40];	// buffer for RTC parsing on receive
    u_long	recvWord;	// for counting EOLs on receive
    u_long	recvEOLCount;	// EOL count for received page
    u_long	savedWriteOff;	// file offset to start of page data
protected:
// NB: these are defined protected for convenience (XXX)
    const ModemConfig& conf;	// configuration parameters
    FlowControl	flowControl;	// current DTE-DCE flow control scheme
    u_int	modemServices;	// services modem supports
    fxStr	modemMfr;	// manufacturer identification
    fxStr	modemModel;	// model identification
    fxStr	modemRevision;	// product revision identification
    Class2Params modemParams;	// NOTE: these are masks of Class 2 codes
    char	rbuf[1024];	// last input line
    ATResponse	lastResponse;	// last atResponse code
    fxStr	mfrQueryCmd;	// manufacturer identification command
    fxStr	modelQueryCmd;	// model identification command
    fxStr	revQueryCmd;	// product revision identification command

    static const char* serviceNames[9];	// class 2 services
    static const u_char digitMap[12*2+1];// Table 3/T.30 digit encoding table

    FaxModem(FaxServer&, const ModemConfig&);

// setup and configuration
    virtual fxBool selectBaudRate(BaudRate max, FlowControl i, FlowControl o);
    virtual fxBool setupModem() = 0;
    virtual fxBool setupManufacturer(fxStr& mfr);
    virtual fxBool setupModel(fxStr& model);
    virtual fxBool setupRevision(fxStr& rev);

    fxBool doQuery(fxStr& queryCmd, fxStr& result);
// dial/answer interactions with derived classes
    virtual const AnswerMsg* findAnswer(const char* s);
    virtual CallType answerResponse(fxStr& emsg);
    virtual CallStatus dialResponse() = 0;
// data transfer timeout controls
    void	setDataTimeout(long secs, u_int br);
    long	getDataTimeout() const;
// miscellaneous
    void	pause(u_int ms);
    void	countPage();
    void	modemTrace(const char* fmt, ...);
    void	modemSupports(const char* fmt, ...);
    void	modemCapability(const char* fmt, ...);
    void	protoTrace(const char* fmt, ...);
    void	serverTrace(const char* fmt, ...);
    void	traceBits(u_int bits, const char* bitNames[]);
    void	traceModemParams();

    static fxBool EOLcode(u_long& w);

    void	startPageRecv();
    void	recvPageData(TIFF* tif, u_char* bp, int n);
    void	endPageRecv(const Class2Params&);
    u_long	getRecvEOLCount() const;
// modem i/o support
    void	trimModemLine(char buf[], int& cc);
    int		getModemLine(char buf[], long ms = 0);
// support to write to modem w/ timeout
    void	beginTimedTransfer();
    void	endTimedTransfer();
    fxBool	wasTimeout();
    void	setTimeout(fxBool);
    void	flushModemInput();
    fxBool	putModem(void* data, int n, long ms = 0);
    fxBool	putModemData(void* data, int n);
    fxBool	putModemDLEData(const u_char* data, u_int,
		    const u_char* brev, long ms);
    void	putModemLine(const char* cp);
    int		getModemChar(long ms = 0);
    int		getModemDataChar();
    void	startTimeout(long ms);
    void	stopTimeout(const char* whichdir);
// host-modem protocol parsing support
    static const char* ATresponses[13];
    virtual ATResponse atResponse(char* buf, long ms = 30*1000);
    virtual fxBool waitFor(ATResponse wanted, long ms = 30*1000);
    fxBool	atCmd(const fxStr& cmd, ATResponse = AT_OK, long ms = 30*1000);
    fxBool	atQuery(const char* what, u_int& v);
    fxBool	atQuery(const char* what, fxStr& v);

    u_int	fromHex(const char*, int = -1);
    fxStr	toHex(int, int ndigits);
    fxBool	parseRange(const char*, u_int&);
    fxBool	vparseRange(const char*, int nargs ...);
// class 1+2 command support
    fxBool	vatFaxCmd(ATResponse resp, const char* cmd ... );
// modem line control
    fxBool	sendBreak(fxBool pause);
    fxBool	setBaudRate(BaudRate rate);
    fxBool	setBaudRate(BaudRate rate, FlowControl i, FlowControl o);
    fxBool	setXONXOFF(FlowControl i, FlowControl o, SetAction);
    fxBool	setDTR(fxBool on);
    fxBool	setInputBuffering(fxBool on);
    fxBool	modemStopOutput();
// server-related stuff
    fxBool	getProtocolTracing();
    fxBool	getHDLCTracing();
    fxBool	sendSetupParams(TIFF*, Class2Params&, FaxMachineInfo&, fxStr&);
    fxBool	recvCheckTSI(const fxStr&);
    void	recvCSI(fxStr&);
    void	recvDCS(Class2Params&);
    void	recvNSF(u_int nsf);
    void	recvSetupPage(TIFF* tif, long group3opts, int fillOrder);
    void	recvResetPage(TIFF* tif);
    fxBool	abortRequested();
public:
    virtual ~FaxModem();

    /*
     * Modems are assumed to be configured for facsimile
     * service in normal operation.  If a client wants to
     * switch to voice or data service, these methods should
     * be invoked.  Note that these will fail if the modem
     * does not support the appropriate service.
     */
    virtual fxBool dataService();		// transition to data service
    virtual fxBool voiceService();		// transition to voice service

    virtual fxBool sync(long ms = 0);		// synchronize (wait for "OK")
    virtual fxBool reset(long ms = 5*1000);	// reset modem state
    virtual fxBool abort(long ms = 5*1000);	// abort current session
    virtual void hangup();			// hangup the phone

// configuration controls
    virtual void setSpeakerVolume(SpeakerVolume);
    virtual void setLID(const fxStr& number) = 0;
// configuration query
    const fxStr& getModel() const;
    const fxStr& getManufacturer() const;
    const fxStr& getRevision() const;
// methods for querying modem capabilities
    virtual fxBool supports2D() const;
    virtual fxBool supportsEOLPadding() const;
    virtual fxBool supportsVRes(float res) const;
    virtual fxBool supportsPageWidth(u_int w) const;
    virtual fxBool supportsPageLength(u_int l) const;

    virtual int selectSignallingRate(int br) const;
    u_int getBestSignallingRate() const;

    u_int getBestScanlineTime() const;
    virtual int selectScanlineTime(int st) const;

    u_int getBestVRes() const;
    u_int getBestDataFormat() const;
    u_int getBestPageWidth() const;
    u_int getBestPageLength() const;
    u_int modemDIS() const;

    /*
     * Send protocol.  The expected sequence is:
     *
     * if (dial(number) == OK) {
     *	  sendBegin();
     *	  if (getPrologue() and parameters acceptable) {
     *	     select send parameters
     *	     sendSetupPhaseB();
     *	     for (each file)
     *		if (!sendPhaseB()) break;
     *	  }
     *	  sendEnd();
     * }
     * hangup();
     *
     * The post page handling parameter to sendPhaseB enables the
     * client to control whether successive files are lumped together
     * as a single T.30 document or split apart.  This is important
     * for doing things like keeping cover pages & documents in a
     * single T.30 document.
     */
    virtual CallStatus dial(const char* number);
    virtual void sendBegin();
    virtual fxBool getPrologue(Class2Params&,
	u_int& nsf, fxStr& csi, fxBool& hasDoc) = 0;
    virtual void sendSetupPhaseB();
    virtual fxBool sendPhaseB(TIFF*, Class2Params&, FaxMachineInfo&,
	fxStr& pph, fxStr& emsg) = 0;
    virtual void sendEnd();

    /*
     * Receive protocol.  The expected sequence is:
     *
     * if (waitForRings(nrings)) {	# wait before answering phone
     *    case (answerCall(type, emsg)) {
     *    CALLTYPE_FAX:
     *	    if (recvBegin()) {
     *	      do {
     *		TIFF* tif = TIFFOpen(..., "w");
     *		int ppm = PPM_EOP;
     *		do {
     *		    if (!recvPage(tif, ppm, emsg))
     *		        error during receive;
     *	        } while (ppm == PPM_MPS);
     *		deal with received file
     *	      } while (ppm != PPM_EOP);
     *	      recvEnd();
     *	    }
     *	    hangup();
     *	  CALLTYPE_DATA:
     *	    dataService();
     *	    do data kinds of things...
     *	  CALLTYPE_VOICE:
     *	    voiceService();
     *	    do voice kinds of things...
     *    }
     * }
     */
    virtual fxBool waitForRings(u_int rings);
    virtual CallType answerCall(AnswerType, fxStr& emsg);
    virtual fxBool recvBegin(fxStr& emsg) = 0;
    virtual fxBool recvPage(TIFF*, int& ppm, fxStr& em) = 0;
    virtual fxBool recvEnd(fxStr& emsg) = 0;

    /*
     * Polling protocol (for polling a remote site).  This is done
     * in conjunction with a send operation: first, before dialing,
     * call requestToPoll(), then after sending any files, do:
     *
     * if (pollBegin(...)) {
     *    do {
     *	    TIFF* tif = TIFFOpen(..., "w");
     *	    if (recvPhaseB(tif, ..., ppm, ...) deal with received file
     *	  } while (ppm != PPM_EOP);
     *	  recvEnd();
     * }
     *
     * (i.e. it's just like a receive operation.)
     */
    virtual fxBool requestToPoll() = 0;
    virtual fxBool pollBegin(const fxStr& pollID, fxStr& emsg) = 0;
};
inline long FaxModem::getDataTimeout() const		{ return dataTimeout; }
inline const fxStr& FaxModem::getModel() const		{ return modemModel; }
inline const fxStr& FaxModem::getManufacturer() const	{ return modemMfr; }
inline const fxStr& FaxModem::getRevision() const	{ return modemRevision; }

#define streq(a, b)	(strcmp(a,b) == 0)
#define	strneq(a, b, n)	(strncmp(a,b,n) == 0)
#endif /* _FAXMODEM_ */
