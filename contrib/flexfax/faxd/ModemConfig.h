/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/faxd/ModemConfig.h,v 1.1.1.1 1994/01/14 23:09:45 torek Exp $
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
#ifndef _ModemConfig_
#define	_ModemConfig_
/*
 * Modem Configuration.
 */
#include "FaxModem.h"

struct ModemConfig {
    fxStr	type;			// hinted modem type
    fxStr	resetCmds;		// extra modem reset commands
    fxStr	dialCmd;		// cmd for dialing (%s for number)
    fxStr	answerCmd[4];		// cmd for answering phone
    fxStr	flowControlCmd;		// cmd for setting up flow control
    fxStr	setupDTRCmd;		// cmd for setting up DTR handling
    fxStr	setupDCDCmd;		// cmd for setting up DCD handling
    fxStr	setupAACmd;		// cmd for setting up adaptive answer
    fxStr	noAutoAnswerCmd;	// cmd for disabling auto-answer
    fxStr	setVolumeCmd[5];	// cmd for setting modem speaker volume
    fxStr	echoOffCmd;		// cmd for disabling echo
    fxStr	verboseResultsCmd;	// cmd for enabling verbose result codes
    fxStr	resultCodesCmd;		// cmd for enabling result codes
    fxStr	onHookCmd;		// cmd for placing phone ``on hook''
    fxStr	softResetCmd;		// cmd for doing soft reset
    fxStr	waitTimeCmd;		// cmd for setting carrier wait time
    fxStr	pauseTimeCmd;		// cmd for setting "," pause time
    fxStr	mfrQueryCmd;		// cmd for getting modem manufacturer
    fxStr	modelQueryCmd;		// cmd for getting modem model id
    fxStr	revQueryCmd;		// cmd for getting modem firmware rev
    fxStr	answerBeginCmd[4];	// cmd for start of inbound session
    fxStr	sendBeginCmd[4];	// cmd for start of outbound session

					// protocol timers
    u_int	t1Timer;		// T.30 T1 timer (ms)
    u_int	t2Timer;		// T.30 T2 timer (ms)
    u_int	t4Timer;		// T.30 T4 timer (ms)
    u_int	dialResponseTimeout;	// dialing command timeout (ms)
    u_int	answerResponseTimeout;	// answer command timeout (ms)
    u_int	pageStartTimeout;	// page send/receive timeout (ms)
    u_int	pageDoneTimeout;	// page send/receive timeout (ms)
					// for class 1:
    fxStr	class1Cmd;		// cmd for setting Class 1
    u_int	class1TCFResponseDelay;	// delay (ms) btwn TCF & ack/nak
    u_int	class1SendPPMDelay;	// delay (ms) before sending PPM
    u_int	class1SendTCFDelay;	// delay (ms) btwn sending DCS & TCF
    u_int	class1TrainingRecovery;	// delay (ms) after failed training
    u_int	class1RecvAbortOK;	// if non-zero, OK sent after recv abort
    u_int	class1FrameOverhead;	// overhead bytes in received frames
    u_int	class1RecvIdentTimer;	// timeout receiving initial identity
					// for class 2:
    fxStr	class2Cmd;		// cmd for setting Class 2
    fxStr	class2BORCmd;		// cmd to set bit order
    fxStr	class2RELCmd;		// cmd to enable byte-aligned EOL
    fxStr	class2CQCmd;		// cmd to setup copy quality checking
    fxStr	class2AbortCmd;		// cmd to abort a session
    fxStr	class2RecvDataTrigger;	// send to start recv
    fxBool	class2XmitWaitForXON;	// wait for XON before send

    FlowControl	flowControl;		// DTE-DCE flow control method
    BaudRate	maxRate;		// max DTE-DCE rate to try
    u_int	recvFillOrder;		// bit order of recvd data
    u_int	sendFillOrder;		// bit order of sent data
    u_int	frameFillOrder;		// bit order of HDLC frames
    u_int	hostFillOrder;		// host bit order
    u_int	resetDelay;		// delay (ms) after reseting modem
    u_int	baudRateDelay;		// delay (ms) after setting baud rate
    u_int	maxPacketSize;		// max page data packet size (bytes)
    u_int	interPacketDelay;	// delay (ms) between outgoing packets
    fxBool	waitForConnect;		// modem sends multiple answer responses

    ModemConfig();
    ~ModemConfig();

    fxBool parseItem(const char* tag, const char* value);
    void setVolumeCmds(const fxStr& value);
    fxStr parseATCmd(const char*);
};
#endif /* _ModemConfig_ */
