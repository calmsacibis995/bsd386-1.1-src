/*
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

static const char rcsid[] =
    "$Id: support.c,v 1.2 1992/11/09 18:47:20 trent Exp $";

/*
 * Command-specific support functions for scsicmd.
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <scsi/scsi.h>
#include <scsi/scsi_ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "scsicmd.h"
#include "bomb.h"


/*
 * Command execution functions.
 * We run these after we submit a CDB using an ioctl().
 * The data is transferred by a read() or a write();
 * we occasionally need some specialization to deal with buffers.
 */

/*
 * The generic scsi command; takes read() or write() as an argument.
 * Currently we have to submit a command, then read or write
 * to execute the command and transfer any data.
 * The code makes sure to provide a buffer even if it isn't needed;
 * this is probably paranoid.
 */
void
scsi_cmd(iofunc)
	ssize_t (*iofunc)();
{
	cmd *cmdp = current_cmd;
	bf *bftab = cmdp->cmd_bf;
	u_char *buf, *malloc_buf = 0;

	if (ioctl(scsi, SDIOCSCSICOMMAND, bftab[0].bf_buf) == -1)
		bomb("can't submit SCSI command to %s", scsiname);

	if (bftab[1].bf_name)
		buf = bftab[1].bf_buf;
	else if ((buf = malloc_buf = malloc(BUFFER_SIZE)) == 0)
		bomb("%s: can't allocate dummy buffer", cmdp->cmd_name);
	if ((*iofunc)(scsi, buf, BUFFER_SIZE) == -1)
		scsi_sense();
	if (malloc_buf)
		free(malloc_buf);
}

void
read_cmd()
{
	scsi_cmd(read);
}

void
write_cmd()
{
	scsi_cmd(write);
}

/*
 * Variable length read function.
 * We assume that a parameter 'addlen' describes the actual transfer size.
 */
void
read_var_cmd()
{
	bf *bftab = current_cmd->cmd_bf;
	pm *pmp;

	scsi_cmd(read);

	pmp = extract_bf("addlen", 0);
	bftab[1].bf_len = pmp->pm_byte + pmp->pm_value + 1;
}

/*
 * Read unstructured data and write it to stdout.
 * Used for real READ commands.
 */
void
read_data_cmd()
{
	cmd *cmdp = current_cmd;
	bf *bftab = cmdp->cmd_bf;
	pm *pmp;
	char *buf;
	size_t len;

	pmp = extract_bf("tl", 0);
	if (pmp->pm_value == 0) {
		scomplain("%s: zero-length read", cmdp->cmd_name);
		if ((buf = malloc(BUFFER_SIZE)) == 0)	/* XXX */
			bomb("%s: can't allocate dummy buffer", cmdp->cmd_name);
		len = BUFFER_SIZE;
	} else if ((buf = malloc(pmp->pm_value)) == 0)
		bomb("%s: can't allocate data buffer", cmdp->cmd_name);
	else
		len = pmp->pm_value;

	if (ioctl(scsi, SDIOCSCSICOMMAND, bftab[0].bf_buf) == -1)
		bomb("can't submit SCSI command to %s", scsiname);

	if (read(scsi, buf, len) == -1)
		scsi_sense();
	else
		if (pmp->pm_value)
			fwrite(buf, len, 1, stdout);
	free(buf);
}


/*
 * Fun and games for MODE SENSE and MODE SELECT...
 * We assume that a MODE SELECT is always preceded by a MODE SENSE.
 * MODE SELECT works from a buffer that is created by MODE SENSE;
 * this makes it simple to alter individual values and save them.
 */

/* saved MODE SENSE values */
static unsigned dbd;
static unsigned page_control;
static unsigned page_code;

/* saved MODE SELECT values */
static unsigned pf;
static unsigned sp;

/*
 * If the user didn't change the value of parameter parm, restore it from *pv.
 * This only works for fields shorter than 32 bits...
 */
void
restore_default(parm, pv)
	char *parm;
	unsigned *pv;
{
	pm *pmp;

	for (pmp = current_cmd->cmd_pm; pmp->pm_name; ++pmp)
		if (strcmp(pmp->pm_name, parm) == 0)
			break;
	if (pmp->pm_name == 0)
		return;
	if (pmp->pm_value == -1) {
		pmp->pm_value = *pv;
		store_pm(pmp);
	} else
		*pv = pmp->pm_value;
}

/*
 * Save the value of parameter parm in *pv, clear the value in the buffer
 * and mark the converted value so that we can spot changes later.
 */
void
save_default(parm, pv)
	char *parm;
	unsigned *pv;
{
	pm *pmp = extract_bf(parm, 0);

	*pv = pmp->pm_value;
	pmp->pm_value = 0;
	store_pm(pmp);
	pmp->pm_value = -1;
}


/*
 * MODE SENSE is funny because it has TWO variable length regions.
 * Worse, it shares buffers with MODE SELECT!
 */
void
mode_sense_cmd()
{
	bf *bftab = current_cmd->cmd_bf;
	pm *pmp;

	restore_default("dbd", &dbd);
	restore_default("pc", &page_control);
	restore_default("pcode", &page_code);

	save_default("pf", &pf);
	save_default("sp", &sp);

	set_parameters("code=0x1a,length=255");

	scsi_cmd(read);

	pmp = extract_bf("bdl", 0);
	bftab[1].bf_len = pmp->pm_byte + pmp->pm_value + 1;

	pmp = extract_bf("mdl", 0);
	bftab[2].bf_buf = &bftab[1].bf_buf[bftab[1].bf_len];
	bftab[2].bf_len = pmp->pm_value + 1 -
	    (bftab[2].bf_buf - bftab[1].bf_buf);
}

/*
 * And this one's funny because it requires a previous MODE SENSE
 * for the proper parameter set.
 * We have to clean out some crap left over from MODE SENSE.
 */
void
mode_select_cmd()
{
	pm *pmp;
	char parmlist[128];

	restore_default("pf", &pf);
	restore_default("sp", &sp);

	save_default("dbd", &dbd);
	save_default("pc", &page_control);
	save_default("pcode", &page_code);

	pmp = extract_bf("mpl", 0);
	sprintf(parmlist, "code=0x15,ps=0,length=%d", pmp->pm_value + 1);
	set_parameters(parmlist);

	scsi_cmd(write);
}


/*
 * Code translation functions.
 * A code translation is a mapping from a numeric code (pm_value)
 * to a string (pm_code) which describes the code.
 * Parameters which can be translated are marked with a nonzero pm_code.
 * We assume on entry that pm_value contains a current value and pm_code is set.
 */

const char * scsistatusmsg[32] = {
	"good",
	"check condition",
	"condition met",
	0,
	"busy",
	0, 0, 0,
	"intermediate status during linked command",
	0,
	"condition met during linked command",
	0,
	"reservation conflict",
	0, 0, 0, 0,
	"command terminated",
	0, 0,
	"queue full",
};

const char * scsisensekeymsg[16] = {
	"no sense",
	"recovered error",
	"not ready",
	"medium error",
	"hardware error",
	"illegal request",
	"unit attention",
	"data protect",
	"blank check",
	"vendor specific error",
	"copy aborted",
	"aborted command",
	"equal comparison",
	"volume overflow",
	"miscompare",
	"reserved error code"
};

const struct asccode {
	u_char	a_asc;
	u_char	a_ascq;
	const	char *a_code;
} asctab[] = {
	{ 0x00, 0x00, "no additional sense information" },
	{ 0x00, 0x01, "filemark detected" },
	{ 0x00, 0x02, "end of partition/medium detected" },
	{ 0x00, 0x03, "setmark detected" },
	{ 0x00, 0x04, "beginning-of-partition/medium detected" },
	{ 0x00, 0x05, "end-of-data detected" },
	{ 0x00, 0x06, "I/O process terminated" },
	{ 0x00, 0x11, "audio play operation in progress" },
	{ 0x00, 0x12, "audio play operation paused" },
	{ 0x00, 0x13, "audio play operation successfully completed" },
	{ 0x00, 0x14, "audio play operation stopped due to error" },
	{ 0x00, 0x15, "no current audio status to return" },
	{ 0x01, 0x00, "no index/sector signal" },
	{ 0x02, 0x00, "no seek complete" },
	{ 0x03, 0x00, "peripheral device write fault" },
	{ 0x03, 0x01, "no write current" },
	{ 0x03, 0x02, "excessive write errors" },
	{ 0x04, 0x00, "logical unit not ready, cause not reportable" },
	{ 0x04, 0x01, "logical unit is in process of becoming ready" },
	{ 0x04, 0x02, "logical unit not ready, initializing command required" },
	{ 0x04, 0x03, "logical unit not ready, manual intervention required" },
	{ 0x04, 0x04, "logical unit not ready, format in progress" },
	{ 0x05, 0x00, "logical unit does not respond to selection" },
	{ 0x06, 0x00, "no reference position found" },
	{ 0x07, 0x00, "multiple peripheral devices selected" },
	{ 0x08, 0x00, "logical unit communication failure" },
	{ 0x08, 0x01, "logical unit communication time-out" },
	{ 0x08, 0x02, "logical unit communication parity error" },
	{ 0x09, 0x00, "track following error" },
	{ 0x09, 0x01, "track servo failure" },
	{ 0x09, 0x02, "focus servo failure" },
	{ 0x09, 0x03, "spindle servo failure" },
	{ 0x0a, 0x00, "error log overflow" },
	{ 0x0c, 0x00, "write error" },
	{ 0x0c, 0x01, "write error recovered with auto reallocation" },
	{ 0x0c, 0x02, "write error, auto reallocation failed" },
	{ 0x10, 0x00, "ID CRC or ECC error" },
	{ 0x11, 0x00, "unrecovered read error" },
	{ 0x11, 0x01, "read retries exhausted" },
	{ 0x11, 0x02, "error too long to correct" },
	{ 0x11, 0x03, "multiple read errors" },
	{ 0x11, 0x04, "unrecovered read error, auto reallocation failed" },
	{ 0x11, 0x05, "L-EC uncorrectable error" },
	{ 0x11, 0x06, "CIRC unrecovered error" },
	{ 0x11, 0x07, "data resynchronization error" },
	{ 0x11, 0x08, "incomplete block read" },
	{ 0x11, 0x09, "no gap found" },
	{ 0x11, 0x0a, "miscorrected error" },
	{ 0x11, 0x0b, "unrecovered read error, recommend reassignment" },
	{ 0x11, 0x0c, "unrecovered read error, recommend rewrite the data" },
	{ 0x12, 0x00, "address mark not found for ID field" },
	{ 0x13, 0x00, "address mark not found for data field" },
	{ 0x14, 0x00, "recorded entity not found" },
	{ 0x14, 0x01, "record not found" },
	{ 0x14, 0x02, "filemark or setmark not found" },
	{ 0x14, 0x03, "end-of-data not found" },
	{ 0x14, 0x04, "block sequence error" },
	{ 0x15, 0x00, "random positioning error" },
	{ 0x15, 0x01, "mechanical positioning error" },
	{ 0x15, 0x02, "positioning error detected by read of medium" },
	{ 0x16, 0x00, "data synchronization mark error" },
	{ 0x17, 0x00, "recovered data with no error correction applied" },
	{ 0x17, 0x01, "recovered data with retries" },
	{ 0x17, 0x02, "recovered data with positive head offset" },
	{ 0x17, 0x03, "recovered data with negative head offset" },
	{ 0x17, 0x04, "recovered data with retries and or CIRC applied" },
	{ 0x17, 0x05, "recovered data using previous sector ID" },
	{ 0x17, 0x06, "recovered data without ECC, data auto-reallocated" },
	{ 0x17, 0x07, "recovered data without ECC, recommend reassignment" },
	{ 0x17, 0x08, "recovered data without ECC, recommend rewrite" },
	{ 0x18, 0x00, "recovered data with error correction applied" },
	{ 0x18, 0x01, "recovered data with error correction and retries \
applied" },
	{ 0x18, 0x02, "recovered data, data auto-reallocated" },
	{ 0x18, 0x03, "recovered data with CIRC" },
	{ 0x18, 0x04, "recovered data with LEC" },
	{ 0x18, 0x05, "recovered data, recommend reassignment" },
	{ 0x18, 0x06, "recovered data, recommend rewrite" },
	{ 0x19, 0x00, "defect list error" },
	{ 0x19, 0x01, "defect list not available" },
	{ 0x19, 0x02, "defect list error in primary list" },
	{ 0x19, 0x03, "defect list error in grown list" },
	{ 0x1a, 0x00, "parameter list length error" },
	{ 0x1b, 0x00, "synchronous data transfer error" },
	{ 0x1c, 0x00, "defect list not found" },
	{ 0x1c, 0x01, "primary defect list not found" },
	{ 0x1c, 0x02, "grown defect list not found" },
	{ 0x1d, 0x00, "miscompare during verify operation" },
	{ 0x1e, 0x00, "recovered ID with ECC correction" },
	{ 0x20, 0x00, "invalid command operation code" },
	{ 0x21, 0x00, "logical block address out of range" },
	{ 0x21, 0x01, "invalid element address" },
	{ 0x22, 0x00, "illegal function" },
	{ 0x24, 0x00, "illegal field in CDB" },
	{ 0x25, 0x00, "logical unit not supported" },
	{ 0x26, 0x00, "invalid field in parameter list" },
	{ 0x26, 0x01, "parameter not supported" },
	{ 0x26, 0x02, "parameter value invalid" },
	{ 0x26, 0x03, "threshold parameters not supported" },
	{ 0x27, 0x00, "write protected" },
	{ 0x28, 0x00, "not ready to ready transition (medium may have \
changed)" },
	{ 0x29, 0x00, "power on, reset, or bus device reset occurred" },
	{ 0x2a, 0x00, "parameters changed" },
	{ 0x2a, 0x01, "mode parameters changed" },
	{ 0x2a, 0x02, "log parameters changed" },
	{ 0x2b, 0x00, "copy cannot execute since host cannot disconnect" },
	{ 0x2c, 0x00, "command sequence error" },
	{ 0x2c, 0x01, "too many windows specified" },
	{ 0x2c, 0x02, "invalid combination of windows specified" },
	{ 0x2d, 0x00, "overwrite error on update in place" },
	{ 0x2f, 0x00, "commands cleared by another initiator" },
	{ 0x30, 0x00, "incompatible medium installed" },
	{ 0x30, 0x01, "cannot read medium, unknown format" },
	{ 0x30, 0x02, "cannot read medium, incompatible format" },
	{ 0x30, 0x03, "cleaning cartridge installed" },
	{ 0x31, 0x00, "medium format corrupted" },
	{ 0x31, 0x01, "format command failed" },
	{ 0x32, 0x00, "no defect spare location available" },
	{ 0x32, 0x01, "defect list update failure" },
	{ 0x33, 0x00, "tape length error" },
	{ 0x36, 0x00, "ribbon, ink or toner failure" },
	{ 0x37, 0x00, "rounded parameter" },
	{ 0x39, 0x00, "saving parameters not supported" },
	{ 0x3a, 0x00, "medium not present" },
	{ 0x3b, 0x00, "sequential positioning error" },
	{ 0x3b, 0x01, "tape position error at beginning-of-medium" },
	{ 0x3b, 0x02, "tape position error at end-of-medium" },
	{ 0x3b, 0x03, "tape or electronic vertical forms unit not ready" },
	{ 0x3b, 0x04, "slew failure" },
	{ 0x3b, 0x05, "paper jam" },
	{ 0x3b, 0x06, "failed to sense top-of-form" },
	{ 0x3b, 0x07, "failed to sense bottom-of-form" },
	{ 0x3b, 0x08, "reposition error" },
	{ 0x3b, 0x09, "read past end of medium" },
	{ 0x3b, 0x0a, "read past beginning of medium" },
	{ 0x3b, 0x0b, "position past end of medium" },
	{ 0x3b, 0x0c, "position past beginning of medium" },
	{ 0x3b, 0x0d, "medium destination element full" },
	{ 0x3b, 0x0e, "medium source element empty" },
	{ 0x3d, 0x00, "invalid bits in identify message" },
	{ 0x3e, 0x00, "logical unit has not self-configured yet" },
	{ 0x3f, 0x00, "target operating conditions have changed" },
	{ 0x3f, 0x01, "microcode has been changed" },
	{ 0x3f, 0x02, "changed operating definition" },
	{ 0x3f, 0x03, "inquiry data has changed" },
	{ 0x40, 0x00, "RAM failure" },
	{ 0x41, 0x00, "data path failure" },
	{ 0x42, 0x00, "power-on or self-test failure" },
	{ 0x43, 0x00, "message error" },
	{ 0x44, 0x00, "internal target failure" },
	{ 0x45, 0x00, "select or reselect failure" },
	{ 0x46, 0x00, "unsuccessful soft reset" },
	{ 0x47, 0x00, "SCSI parity error" },
	{ 0x48, 0x00, "initiator detected error message received" },
	{ 0x49, 0x00, "invalid message error" },
	{ 0x4a, 0x00, "command phase error" },
	{ 0x4b, 0x00, "data phase error" },
	{ 0x4c, 0x00, "logical unit failed self-configuration" },
	{ 0x4e, 0x00, "overlapped commands attempted" },
	{ 0x50, 0x00, "write append error" },
	{ 0x50, 0x01, "write append position error" },
	{ 0x50, 0x02, "position error related to timing" },
	{ 0x51, 0x00, "erase failure" },
	{ 0x52, 0x00, "cartridge fault" },
	{ 0x53, 0x00, "media load or eject failed" },
	{ 0x53, 0x01, "unload tape failed" },
	{ 0x53, 0x02, "medium removal prevented" },
	{ 0x54, 0x00, "SCSI to host system interface failure" },
	{ 0x55, 0x00, "system resource failure" },
	{ 0x57, 0x00, "unable to recover table-of-contents" },
	{ 0x58, 0x00, "generation does not exist" },
	{ 0x59, 0x00, "updated block read" },
	{ 0x5a, 0x00, "operator request for state change input (unspecified)" },
	{ 0x5a, 0x01, "operator medium removal request" },
	{ 0x5a, 0x02, "operator selected write protect" },
	{ 0x5a, 0x03, "operator selected write permit" },
	{ 0x5b, 0x00, "log exception" },
	{ 0x5b, 0x01, "threshold condition met" },
	{ 0x5b, 0x02, "log counter at maximum" },
	{ 0x5b, 0x03, "log list codes exhausted" },
	{ 0x5c, 0x00, "RPL status change" },
	{ 0x5c, 0x01, "spindles synchronized" },
	{ 0x5c, 0x02, "spindles not synchronized" },
	{ 0x60, 0x00, "lamp failure" },
	{ 0x61, 0x00, "video acquisition error" },
	{ 0x61, 0x01, "unable to acquire video" },
	{ 0x61, 0x02, "out of focus" },
	{ 0x62, 0x00, "scan head positioning error" },
	{ 0x63, 0x00, "end of user area encountered on this track" },
	{ 0x64, 0x00, "illegal mode for this track" },
	{ 0 }
};

char *
sprintasc(asc, ascq)
	unsigned asc, ascq;
{
	const struct asccode *a;
	unsigned save_ascq = ascq;
	char *s = malloc(128);

	if (ascq >= 0x80) {
		if (asc == 0x40) {	/* XXX special case */
			sprintf(s, "diagnostic failure on component %#x", ascq);
			return s;
		}
		ascq = 0;
	}
	for (a = asctab; a->a_code; ++a)
		if (a->a_asc == asc && a->a_ascq == ascq) {
			if (ascq == save_ascq)
				strcpy(s, a->a_code);
			else
				sprintf(s, "%s (ascq %#x)", a->a_code,
				    save_ascq);
			return s;
		}
	sprintf(s, "asc %#x, ascq %#x", asc, save_ascq);
	return s;
}

/*
 * This function doesn't really belong here,
 * but it's doing the same kind of work with
 * the same translation tables...
 */
void
scsi_sense()
{
	static struct scsi_fmt_sense s;
	struct scsi_sense *sn = (struct scsi_sense *)s.sense;
	char *p;
	int key;

	if (ioctl(scsi, SDIOCSENSE, &s) == -1) {
		complain("ioctl(SDIOCSENSE) failed on %s", scsiname);
		return;
	}

	fprintf(stderr, "scsi status: ");
	if (p = (char *)scsistatusmsg[(s.status & STS_MASK) >> 1])
		fputs(p, stderr);
	else
		fprintf(stderr, "%#x", s.status);
	if (s.status & STS_CHECKCOND) {
		if (SENSE_ISXSENSE(sn) && XSENSE_ISSTD(sn)) {
			/*
			 * Standard extended sense: can examine sense key
			 * and (if valid) info.
			 */
			key = XSENSE_KEY(sn);
			fprintf(stderr, ": %s", scsisensekeymsg[key]);
			if (XSENSE_HASASC(sn) && XSENSE_ASC(sn)) {
				p = sprintasc(XSENSE_ASC(sn),
					XSENSE_HASASCQ(sn) ?
					    XSENSE_ASCQ(sn) : 0);
				fprintf(stderr, ": %s", p);
				free(p);
			}
			if (XSENSE_IVALID(sn))
				fprintf(stderr, ", block %d", XSENSE_INFO(sn));
		} else
			fprintf(stderr, ", class %d, code %d",
			    SENSE_ECLASS(sn), SENSE_ECODE(sn));
	}
	fprintf(stderr, "\n");
}

const char *inq_peripheral_devices[] = {
	"direct-access device (e.g. disks)",
	"sequential-access device (e.g. tapes)",
	"printer device",
	"processor device",
	"write-once device (e.g. some optical disks)",
	"CD-ROM device",
	"scanner device",
	"optical memory device (e.g. some optical disks)",
	"medium changer device (e.g. jukeboxes)",
	"communications device",
	"defined by ASC ITB (graphic arts pre-press devices)",
	"defined by ASC ITB (graphic arts pre-press devices)",
};

void
inquiry_code(pmp)
	pm *pmp;
{
	pmp->pm_code = "";

	if (strcmp(pmp->pm_name, "pdt") == 0) {
		if (pmp->pm_value <
		    sizeof inq_peripheral_devices / sizeof (char *))
			pmp->pm_code = inq_peripheral_devices[pmp->pm_value];
		else if (pmp->pm_value == 0x1f)
			pmp->pm_code = "unknown device type";
		return;
	}

	if (strcmp(pmp->pm_name, "ansi") == 0) {
		if (pmp->pm_value == 1)
			pmp->pm_code = "SCSI-1";
		else if (pmp->pm_value == 2)
			pmp->pm_code = "SCSI-2";
		return;
	}

	if (strcmp(pmp->pm_name, "rdf") == 0) {
		if (pmp->pm_value == 0)
			pmp->pm_code = "SCSI-1";
		else if (pmp->pm_value == 1)
			pmp->pm_code = "CCS";
		else if (pmp->pm_value == 2)
			pmp->pm_code = "SCSI-2";
		return;
	}
}

void
request_sense_code(pmp)
	pm *pmp;
{
	pm *pmpq;
	unsigned asc, ascq;

	pmp->pm_code = "";

	if (strcmp(pmp->pm_name, "sk") == 0) {
		pmp->pm_code = scsisensekeymsg[pmp->pm_value];
		return;
	}

	if (strcmp(pmp->pm_name, "asc") == 0) {
		asc = pmp->pm_value;
		if (pmpq = extract_bf("ascq", 0))
			ascq = pmpq->pm_value;
		else
			ascq = 0;
		pmp->pm_code = sprintasc(asc, ascq);
		return;
	}
}
