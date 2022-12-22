/*
 * Copyright (c) 1992 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */

static const char rcsid[] =
    "$Id: table.c,v 1.3 1993/12/15 04:04:54 donn Exp $";

#include <sys/cdefs.h>
#include <sys/types.h>
#include "scsicmd.h"

/* buffers: buf, len, overlap, name, description */
/* parameters: bufno, byte, bit, len, name, description, value, string */

ucdb cdb_erase;

bf bf_erase[] = {
	{ cdb_erase, sizeof (ucdb), 0, "cdb", "command descriptor block" },
	{ 0 }
};

pm pm_erase[] = {
	{ 0, 0, 0, 8, "code", "operation code", 0x19 },
	{ 0, 1, 5, 3, "lun", "logical unit" },
	{ 0, 1, 1, 1, "immed", "immediate return" },
	{ 0, 1, 0, 1, "long", "long erase for entire tape" },
	{ 0 }
};

ucdb cdb_format_unit;
u_char misc_format_unit_parmlist[BUFFER_SIZE];

bf bf_format_unit[] = {
	{ cdb_format_unit, sizeof (ucdb), 0, "cdb",
		"command descriptor block" },
	{ misc_format_unit_parmlist, sizeof misc_format_unit_parmlist, 0,
		"pl", "parameter list" },
	{ 0 }
};

pm pm_format_unit[] = {
	{ 0, 0, 0, 8, "code", "operation code", 0x04 },
	{ 0, 1, 5, 3, "lun", "logical unit" },
	{ 0, 1, 4, 1, "fmtdata", "format data" },
	{ 0, 1, 3, 1, "cmplst", "complete list" },
	{ 0, 1, 0, 3, "dlf", "defect list format" },
	{ 0, 2, 0, 8, "vendor", "vendor specific" },
	{ 0, 3, 0, 16, "intlv", "interleave" },

	/* FORMAT UNIT parameter list */
	{ 1, 1, 7, 1, "fov", "format options valid" },
	{ 1, 1, 6, 1, "dpry", "disable primary" },
	{ 1, 1, 5, 1, "dcrt", "disable certification" },
	{ 1, 1, 4, 1, "stpf", "stop format" },
	{ 1, 1, 3, 1, "ip", "initialization pattern" },
	{ 1, 1, 2, 1, "dsp", "disable saving parameters" },
	{ 1, 1, 1, 1, "immed", "immediate status" },
	{ 1, 1, 0, 1, "vs", "vendor specific bit" },
	{ 1, 2, 0, 16, "dll", "defect list length" },
	{ 0 }
};

ucdb cdb_inquiry;
u_char misc_inquiry_page[BUFFER_SIZE];

bf bf_inquiry[] = {
	{ cdb_inquiry, sizeof (ucdb), 0, "cdb",
		"command descriptor block" },
	{ misc_inquiry_page, sizeof misc_inquiry_page, 0, "sid",
		"standard inquiry data [evpd=0]" },
	{ 0, 0, 1, "svpdp", "supported vital product data pages [evpd,pc=0]" },
	{ 0, 0, 1, "aip", "ASCII information page [evpd,pc=1]" },
	{ 0, 0, 1, "usnp", "unit serial number page [evpd,pc=0x80]" },
	{ 0, 0, 1, "iodp",
		"implemented operating definitions page [evpd,pc=0x81]" },
	{ 0, 0, 1, "aiodp",
		"ASCII implemented operating definition page [evpd,pc=0x82]" },
	{ 0 }
};

pm pm_inquiry[] = {
	{ 0, 0, 0, 8, "code", "operation code", 0x12 },
	{ 0, 1, 5, 3, "lun", "logical unit" },
	{ 0, 1, 0, 1, "evpd", "enable vital product data" },
	{ 0, 2, 0, 8, "pc", "page code" },
	{ 0, 4, 0, 8, "length", "allocation length", BUFFER_SIZE - 1 },

	/* standard INQUIRY data (evpd=0) */
	{ 1, 0, 5, 3, "pq", "peripheral qualifier" },
	{ 1, 0, 0, 5, "pdt", "peripheral device type", 0, 0, "" },
	{ 1, 1, 7, 1, "rmb", "removable medium" },
	{ 1, 1, 0, 7, "dtm", "device type modifier" },
	{ 1, 2, 6, 2, "iso", "ISO version" },
	{ 1, 2, 3, 3, "ecma", "ECMA version" },
	{ 1, 2, 0, 3, "ansi", "ANSI version", 0, 0, "" },
	{ 1, 3, 7, 1, "aenc", "asynchronous event notification" },
	{ 1, 3, 6, 1, "trmiop", "terminate I/O process" },
	{ 1, 3, 0, 4, "rdf", "response data format", 0, 0, "" },
	{ 1, 4, 0, 8, "addlen", "additional length" },
	{ 1, 7, 7, 1, "reladr", "relative addressing" },
	{ 1, 7, 6, 1, "wbus32", "32-bit wide bus" },
	{ 1, 7, 5, 1, "wbus16", "16-bit wide bus" },
	{ 1, 7, 4, 1, "sync", "synchronous transfers" },
	{ 1, 7, 3, 1, "linked", "linked commands" },
	{ 1, 7, 1, 1, "cmdque", "tagged command queuing" },
	{ 1, 7, 0, 1, "sftre", "soft resets" },
	{ 1, 8, 0, 64, "vi", "vendor identification", 0, "" },
	{ 1, 16, 0, 128, "pi", "product identification", 0, "" },
	{ 1, 32, 0, 32, "prl", "product revision level", 0, "" },
	{ 1, 36, 0, 160, "vsi", "vendor specific information", 0, "" },

	/* supported vital product data pages (evpd=1, pc=0) */
	{ 2, 0, 5, 3, "pq", "peripheral qualifier" },
	{ 2, 0, 0, 5, "pdt", "peripheral device type", 0, 0, "" },
	{ 2, 1, 0, 8, "pc", "page code" },
	{ 2, 3, 0, 8, "pl", "page length" },
	{ 2, 4, 0, 8, "spl/0", "supported page list" },
	{ 2, 5, 0, 8, "spl/1", "supported page list" },
	{ 2, 6, 0, 8, "spl/2", "supported page list" },
	{ 2, 7, 0, 8, "spl/3", "supported page list" },
	{ 2, 8, 0, 8, "spl/4", "supported page list" },
	{ 2, 9, 0, 8, "spl/5", "supported page list" },
	{ 2, 10, 0, 8, "spl/6", "supported page list" },
	{ 2, 11, 0, 8, "spl/7", "supported page list" },
	{ 2, 12, 0, 8, "spl/8", "supported page list" },
	{ 2, 13, 0, 8, "spl/9", "supported page list" },

	/* ASCII information page (evpd=1, pc=0x01-0x7f) */
	{ 3, 0, 5, 3, "pq", "peripheral qualifier" },
	{ 3, 0, 0, 5, "pdt", "peripheral device type", 0, 0, "" },
	{ 3, 1, 0, 8, "pc", "page code" },
	{ 3, 3, 0, 8, "pl", "page length" },
	{ 3, 4, 0, 8, "al", "ASCII length" },
	{ 3, 5, 0, 251*8, "ai", "ASCII information", 0, "" },

	/* unit serial number page (evpd=1, pc=0x80) */
	{ 4, 0, 5, 3, "pq", "peripheral qualifier" },
	{ 4, 0, 0, 5, "pdt", "peripheral device type", 0, 0, "" },
	{ 4, 1, 0, 8, "pc", "page code" },
	{ 4, 3, 0, 8, "pl", "page length" },
	{ 4, 4, 0, 252*8, "psn", "product serial number", 0, "" },

	/* implemented operating definitions page (evpd=1, pc=0x81) */
	{ 5, 0, 5, 3, "pq", "peripheral qualifier" },
	{ 5, 0, 0, 5, "pdt", "peripheral device type", 0, 0, "" },
	{ 5, 1, 0, 8, "pc", "page code" },
	{ 5, 3, 0, 8, "pl", "page length" },
	{ 5, 4, 0, 7, "cod", "current operating definition" },
	{ 5, 5, 7, 1, "savimp/dod", "save implemented" },
	{ 5, 5, 0, 7, "dod", "default operating definition" },
	{ 5, 6, 7, 1, "savimp/0", "save implemented" },
	{ 5, 6, 0, 7, "sodl/0", "supported operating definition list" },
	{ 5, 7, 7, 1, "savimp/1", "save implemented" },
	{ 5, 7, 0, 7, "sodl/1", "supported operating definition list" },
	{ 5, 8, 7, 1, "savimp/2", "save implemented" },
	{ 5, 8, 0, 7, "sodl/2", "supported operating definition list" },
	{ 5, 9, 7, 1, "savimp/3", "save implemented" },
	{ 5, 9, 0, 7, "sodl/3", "supported operating definition list" },
	{ 5, 10, 7, 1, "savimp/4", "save implemented" },
	{ 5, 10, 0, 7, "sodl/4", "supported operating definition list" },
	{ 5, 11, 7, 1, "savimp/5", "save implemented" },
	{ 5, 11, 0, 7, "sodl/5", "supported operating definition list" },
	{ 5, 12, 7, 1, "savimp/6", "save implemented" },
	{ 5, 12, 0, 7, "sodl/6", "supported operating definition list" },
	{ 5, 13, 7, 1, "savimp/7", "save implemented" },
	{ 5, 13, 0, 7, "sodl/7", "supported operating definition list" },
	{ 5, 14, 7, 1, "savimp/8", "save implemented" },
	{ 5, 14, 0, 7, "sodl/8", "supported operating definition list" },
	{ 5, 15, 7, 1, "savimp/9", "save implemented" },
	{ 5, 15, 0, 7, "sodl/9", "supported operating definition list" },

	/* ASCII implemented operating definition page (evpd=1, pc=0x82) */
	{ 6, 0, 5, 3, "pq", "peripheral qualifier" },
	{ 6, 0, 0, 5, "pdt", "peripheral device type", 0, 0, "" },
	{ 6, 1, 0, 8, "pc", "page code" },
	{ 6, 3, 0, 8, "pl", "page length" },
	{ 6, 4, 0, 8, "aoddl",
		"ASCII operating definition description length" },
	{ 6, 5, 0, 251*8, "aoddd",
		"ASCII operating definition description data", 0, "" },
	{ 0 }
};

ucdb cdb_load;

bf bf_load[] = {
	{ cdb_load, sizeof (ucdb), 0, "cdb", "command descriptor block" },
	{ 0 }
};

pm pm_load[] = {
	{ 0, 0, 0, 8, "code", "operation code", 0x1b },
	{ 0, 1, 5, 3, "lun", "logical unit" },
	{ 0, 1, 0, 1, "immed", "immediate return" },
	{ 0, 4, 2, 1, "eot", "end of tape positioning" },
	{ 0, 4, 1, 1, "re-ten", "retension" },
	{ 0, 4, 0, 1, "load", "load rather than unload" },
	{ 0 }
};

/*
 * We use the same buffers for MODE SENSE and MODE SELECT.
 * This makes it easy to call SENSE, twiddle a page, and call SELECT.
 */
ucdb cdb_mode_page;
u_char misc_mode_page[BUFFER_SIZE];

bf bf_mode_page[] = {
	{ cdb_mode_page, sizeof (ucdb), 0, "cdb",
		"command descriptor block" },
	{ misc_mode_page, sizeof misc_mode_page, 0, "mp", "mode page" },

	/* mode pages that are independent of device type: */
	{ &misc_mode_page[4], sizeof misc_mode_page - 4, 0, "drp",
		"disconnect-reconnect page [pcode=0x02]" },
	{ 0, 0, 2, "pdp", "peripheral device page [pcode=0x09]" },
	{ 0, 0, 2, "cmp", "control mode page [pcode=0x0a]" },

	/* mode pages for disks: */
	{ 0, 0, 2, "drwerp",
		"disk read-write error recovery page [pcode=0x01]" },
	{ 0, 0, 2, "dfdp", "disk format device page [pcode=0x03]" },
	{ 0, 0, 2, "rddgp", "rigid disk drive geometry page [pcode=0x04]" },
	{ 0, 0, 2, "fdp", "flexible disk page [pcode=0x05]" },
	{ 0, 0, 2, "dverp", "disk verify error recovery page [pcode=0x07]" },
	{ 0, 0, 2, "dcp", "disk caching page [pcode=0x08]" },
	{ 0, 0, 2, "dmtsp", "disk medium types supported page [pcode=0x0b]" },
	{ 0, 0, 2, "dnapp", "disk notch and partition page [pcode=0x0c]" },

	/* mode pages for tapes: */
	{ 0, 0, 2, "trwerp",
		"tape read-write error recovery page [pcode=0x01]" },
	{ 0, 0, 2, "tdcp", "tape device configuration page [pcode=0x10]" },
	{ 0, 0, 2, "tmpp1", "tape medium partition page 1 [pcode=0x11]" },
	{ 0, 0, 2, "tmpp2", "tape medium partition page 2 [pcode=0x12]" },
	{ 0, 0, 2, "tmpp3", "tape medium partition page 3 [pcode=0x13]" },
	{ 0, 0, 2, "tmpp4", "tape medium partition page 4 [pcode=0x14]" },

	/* mode pages for printers: */
	/* mode pages for processors: */
	/* mode pages for WORMs: */
	/* mode pages for CD-ROMs: */
	/* mode pages for scanners: */
	/* mode pages for optical disks: */
	/* mode pages for jukeboxes: */
	/* mode pages for communications: */
	{ 0 }
};

/*
 * Argh -- all of these parameter names must be distinct,
 * and must not overlap with buffer names.
 * (Exception: overlap is permitted for the same buffer at the same offset.)
 * The 'page code/control' fields are the most obnoxious;
 * we use pc for page control, pcode for page code and
 * mpcode for the page code field in the mode page itself.
 */ 
pm pm_mode_page[] = {
	{ 0, 0, 0, 8, "code", "operation code" }, /* XXX no defaults */
	{ 0, 1, 5, 3, "lun", "logical unit" },
	{ 0, 1, 4, 1, "pf", "page format" },			/* SELECT */
	{ 0, 1, 3, 1, "dbd", "disable block descriptors" },	/* SENSE */
	{ 0, 1, 0, 1, "sp", "save pages" },			/* SELECT */
	{ 0, 2, 6, 2, "pc", "page control" },			/* SENSE */
	{ 0, 2, 0, 6, "pcode", "page code" },			/* SENSE */
	{ 0, 4, 0, 8, "length", "allocation or parameter list length" },

	/* mode parameter header */
	{ 1, 0, 0, 8, "mdl", "mode descriptor length" },
	{ 1, 1, 0, 8, "mt", "medium type" },
	/* the following byte is overloaded, sigh */
	{ 1, 2, 0, 8, "dsp", "device specific parameter" },
	{ 1, 2, 7, 1, "wp", "write protect" },
	{ 1, 2, 4, 1, "dpofua", "disk DPO/FUA support" },
	{ 1, 2, 4, 3, "tbm", "tape buffered mode" },
	{ 1, 2, 0, 3, "ts", "tape speed" },
	{ 1, 3, 0, 8, "bdl", "block descriptor length" },

	/* mode parameter block descriptor */
	{ 1, 4, 0, 8, "dc/1", "density code" },
	{ 1, 5, 0, 24, "nb/1", "number of blocks" },
	{ 1, 9, 0, 24, "bl/1", "block length" },
	{ 1, 12, 0, 8, "dc/2", "density code" },
	{ 1, 13, 0, 24, "nb/2", "number of blocks" },
	{ 1, 17, 0, 24, "bl/2", "block length" },
	{ 1, 20, 0, 8, "dc/3", "density code" },
	{ 1, 21, 0, 24, "nb/3", "number of blocks" },
	{ 1, 25, 0, 24, "bl/3", "block length" },
	{ 1, 28, 0, 8, "dc/4", "density code" },
	{ 1, 29, 0, 24, "nb/4", "number of blocks" },
	{ 1, 33, 0, 24, "bl/4", "block length" },

	/* mode parameters that are independent of device type: */

	/* 0x02, "drp", disconnect-reconnect page */
	{ 2, 0, 7, 1, "ps", "parameters saveable" },		/* SENSE */
	{ 2, 0, 0, 6, "mpcode", "mode page code" },
	{ 2, 1, 0, 8, "mpl", "mode page length" },
	{ 2, 2, 0, 8, "bfr", "buffer full ratio" },
	{ 2, 3, 0, 8, "ber", "buffer empty ratio" },
	{ 2, 4, 0, 16, "bil", "buffer inactivity limit" },
	{ 2, 6, 0, 16, "dtl", "disconnect time limit" },
	{ 2, 8, 0, 16, "ctl", "connect time limit" },
	{ 2, 10, 0, 16, "mbs", "maximum burst size" },
	{ 2, 12, 0, 2, "dtdc", "data transfer disconnect control" },

	/* 0x09, "pdp", peripheral device page */
	{ 3, 0, 7, 1, "ps", "parameters saveable" },		/* SENSE */
	{ 3, 0, 0, 6, "mpcode", "mode page code" },
	{ 3, 1, 0, 8, "mpl", "mode page length" },
	{ 3, 2, 0, 16, "ii", "interface identifier" },
	{ 3, 8, 0, 32, "pdvs/1", "peripheral device vendor specific" },
	{ 3, 12, 0, 32, "pdvs/2", "peripheral device vendor specific" },
	{ 3, 16, 0, 32, "pdvs/3", "peripheral device vendor specific" },
	{ 3, 20, 0, 32, "pdvs/4", "peripheral device vendor specific" },

	/* 0x0a, "cmp", control mode page */
	{ 4, 0, 7, 1, "ps", "parameters saveable" },		/* SENSE */
	{ 4, 0, 0, 6, "mpcode", "mode page code" },
	{ 4, 1, 0, 8, "mpl", "mode page length" },
	{ 4, 2, 0, 1, "rlec", "report log exception condition" },
	{ 4, 3, 4, 4, "qam", "queue algorithm modifier" },
	{ 4, 3, 1, 1, "qerr", "queue error management" },
	{ 4, 3, 0, 1, "dque", "disable queuing" },
	{ 4, 4, 7, 1, "eeca", "enable extended contingent allegiance" },
	{ 4, 4, 2, 1, "raenp", "ready AEN permission" },
	{ 4, 4, 1, 1, "uaenp", "unit attention AEN permission" },
	{ 4, 4, 0, 1, "eaenp", "error AEN permission" },
	{ 4, 6, 0, 16, "raenhp", "ready AEN holdoff period" },

	/* mode parameters for disks: */

	/* 0x01, "drwerp", disk read-write error recovery page */
	{ 5, 0, 7, 1, "ps", "parameters saveable" },		/* SENSE */
	{ 5, 0, 0, 6, "mpcode", "mode page code" },
	{ 5, 1, 0, 8, "mpl", "mode page length" },
	{ 5, 2, 7, 1, "awre", "automatic write allocation enabled" },
	{ 5, 2, 6, 1, "arre", "automatic read reallocation enabled" },
	{ 5, 2, 5, 1, "tb", "transfer block" },
	{ 5, 2, 4, 1, "rc", "read continuous" },
	{ 5, 2, 3, 1, "eer", "enable early recovery" },
	{ 5, 2, 2, 1, "per", "post error recovery" },
	{ 5, 2, 1, 1, "dte", "disable transfer on error" },
	{ 5, 2, 0, 1, "dcr", "disable correction" },
	{ 5, 3, 0, 8, "rrc", "read retry count" },
	{ 5, 4, 0, 8, "cs", "correction span" },
	{ 5, 5, 0, 8, "hoc", "head offset count" },
	{ 5, 6, 0, 8, "dsoc", "data strobe offset count" },
	{ 5, 8, 0, 8, "wrc", "write retry count" },
	{ 5, 10, 0, 16, "rtl", "recovery time limit" },

	/* 0x03, "dfdp", disk format device page */
	{ 6, 0, 7, 1, "ps", "parameters saveable" },		/* SENSE */
	{ 6, 0, 0, 6, "mpcode", "mode page code" },
	{ 6, 1, 0, 8, "mpl", "mode page length" },
	{ 6, 2, 0, 16, "tpz", "tracks per zone" },
	{ 6, 4, 0, 16, "aspz", "alternate sectors per zone" },
	{ 6, 6, 0, 16, "atpz", "alternate tracks per zone" },
	{ 6, 8, 0, 16, "atplu", "alternate tracks per logical unit" },
	{ 6, 10, 0, 16, "spt", "sectors per track" },
	{ 6, 12, 0, 16, "dbpps", "data bytes per physical sector" },
	{ 6, 14, 0, 16, "il", "interleave" },
	{ 6, 16, 0, 16, "tsf", "track skew factor" },
	{ 6, 18, 0, 16, "csf", "cylinder skew factor" },
	{ 6, 20, 7, 1, "ssec", "soft sectoring enable control" },
	{ 6, 20, 6, 1, "hsec", "hard sectoring enable control" },
	{ 6, 20, 5, 1, "rmb", "removable media" },
	{ 6, 20, 4, 1, "surf", "surfaces instead of cylinders" },

	/* 0x04, "rddgp", rigid disk drive geometry page */
	{ 7, 0, 7, 1, "ps", "parameters saveable" },		/* SENSE */
	{ 7, 0, 0, 6, "mpcode", "mode page code" },
	{ 7, 1, 0, 8, "mpl", "mode page length" },
	{ 7, 2, 0, 24, "noc", "number of cylinders" },
	{ 7, 5, 0, 8, "noh", "number of heads" },
	{ 7, 6, 0, 24, "scwp", "starting cylinder for write precompensation" },
	{ 7, 9, 0, 24, "scrwc", "starting cylinder for reduced write current" },
	{ 7, 12, 0, 16, "dsr", "drive step rate" },
	{ 7, 14, 0, 24, "lzc", "landing zone cylinder" },
	{ 7, 17, 0, 2, "rpl", "rotational position locking" },
	{ 7, 18, 0, 8, "ro", "rotational offset" },
	{ 7, 20, 0, 16, "mrr", "medium rotation rate" },

	/* 0x05, "fdp", flexible disk page */
	{ 8, 0, 7, 1, "ps", "parameters saveable" },		/* SENSE */
	{ 8, 0, 0, 6, "mpcode", "mode page code" },
	{ 8, 1, 0, 8, "mpl", "mode page length" },
	/*8we prepend 'f' for 'floppy' to names to prevent collisions */
	{ 8, 2, 0, 16, "ftr", "floppy transfer rate" },
	{ 8, 4, 0, 8, "fnoh", "floppy number of heads" },
	{ 8, 5, 0, 8, "fspt", "floppy sectors per track" },
	{ 8, 6, 0, 16, "fdbps", "floppy data bytes per sector" },
	{ 8, 8, 0, 16, "fnoc", "floppy number of cylinders" },
	{ 8, 10, 0, 16, "fscwp",
		"floppy starting cylinder for write precompensation" },
	{ 8, 12, 0, 16, "fscrwc",
		"floppy starting cylinder for reduced write current" },
	{ 8, 14, 0, 16, "fdsr", "floppy drive step rate" },
	{ 8, 16, 0, 8, "fdspw", "floppy drive step pulse width" },
	{ 8, 17, 0, 16, "fhsd", "floppy head settle delay" },
	{ 8, 19, 0, 8, "fmond", "floppy motor on delay" },
	{ 8, 20, 0, 8, "fmoffd", "floppy motor off delay" },
	{ 8, 21, 7, 1, "ftrdy", "floppy true ready" },
	{ 8, 21, 6, 1, "sfsn", "floppy start sector number" },
	{ 8, 21, 5, 1, "fmo", "floppy motor on" },
	{ 8, 22, 0, 4, "fspc", "floppy step pulse per cylinder" },
	{ 8, 23, 0, 8, "fwc", "floppy write compensation" },
	{ 8, 24, 0, 8, "fhld", "floppy head load delay" },
	{ 8, 25, 0, 8, "fhud", "floppy head unload delay" },
	{ 8, 26, 4, 4, "fp34", "floppy pin 34" },
	{ 8, 26, 0, 4, "fp2", "floppy pin 2" },
	{ 8, 27, 4, 4, "fp4", "floppy pin 4" },
	{ 8, 27, 0, 4, "fp1", "floppy pin 1" },
	{ 8, 28, 0, 16, "fmrr", "floppy medium rotation rate" },

	/* 0x07, "dverp", disk verify error recovery page */
	{ 9, 0, 7, 1, "ps", "parameters saveable" },		/* SENSE */
	{ 9, 0, 0, 6, "mpcode", "mode page code" },
	{ 9, 1, 0, 8, "mpl", "mode page length" },
	/* EER, PER, DTE, DCR are identical to r/w error recovery page */
	{ 9, 2, 3, 1, "eer", "enable early recovery" },
	{ 9, 2, 2, 1, "per", "post error recovery" },
	{ 9, 2, 1, 1, "dte", "disable transfer on error" },
	{ 9, 2, 0, 1, "dcr", "disable correction" },
	{ 9, 3, 0, 8, "vrc", "verify retry count" },
	{ 9, 4, 0, 8, "vcs", "verify correction span" },
	{ 9, 10, 0, 16, "vrtl", "verify recovery time limit" },

	/* 0x08, "dcp", disk caching page */
	{ 10, 0, 7, 1, "ps", "parameters saveable" },		/* SENSE */
	{ 10, 0, 0, 6, "mpcode", "mode page code" },
	{ 10, 1, 0, 8, "mpl", "mode page length" },
	{ 10, 2, 2, 1, "wce", "write cache enable" },
	{ 10, 2, 1, 1, "mf", "multiplication factor" },
	{ 10, 2, 0, 1, "rcd", "read cache disable" },
	{ 10, 3, 4, 4, "drrp", "demand read retention priority" },
	{ 10, 3, 0, 4, "wrp", "write retention priority" },
	{ 10, 4, 0, 16, "dpftl", "disable pre-fetch transfer length" },
	{ 10, 6, 0, 16, "minpf", "minimum pre-fetch" },
	{ 10, 8, 0, 16, "maxpf", "maximum pre-fetch" },
	{ 10, 10, 0, 16, "maxpfc", "maximum pre-fetch ceiling" },

	/* 0x0b, "dmtsp", "disk medium types supported page */
	{ 11, 0, 7, 1, "ps", "parameters saveable" },		/* SENSE */
	{ 11, 0, 0, 6, "mpcode", "mode page code" },
	{ 11, 1, 0, 8, "mpl", "mode page length" },
	{ 11, 4, 0, 8, "mts/1", "medium type supported" },
	{ 11, 5, 0, 8, "mts/2", "medium type supported" },
	{ 11, 6, 0, 8, "mts/3", "medium type supported" },
	{ 11, 7, 0, 8, "mts/4", "medium type supported" },

	/* 0x0c, "dnapp", disk notch and partition page */
	{ 12, 0, 7, 1, "ps", "parameters saveable" },		/* SENSE */
	{ 12, 0, 0, 6, "mpcode", "mode page code" },
	{ 12, 1, 0, 8, "mpl", "mode page length" },
	{ 12, 2, 7, 1, "nd", "notched drive" },
	{ 12, 2, 6, 1, "lpn", "logical (vs. physical) notch" },
	{ 12, 4, 0, 16, "mnon", "maximum number of notches" },
	{ 12, 6, 0, 16, "an", "active notch" },
	{ 12, 8, 0, 32, "sb", "starting boundary" },
	{ 12, 12, 0, 32, "eb", "ending boundary" },
	{ 12, 16, 0, 32, "pn/1", "pages notched" },
	{ 12, 20, 0, 32, "pn/2", "pages notched" },

	/* mode parameters for tapes: */

	/* 0x01, "trwerp", tape read-write error recovery page */
	/* all parameters in this page overlap with disk version */
	{ 13, 0, 7, 1, "ps", "parameters saveable" },		/* SENSE */
	{ 13, 0, 0, 6, "mpcode", "mode page code" },
	{ 13, 1, 0, 8, "mpl", "mode page length" },
	{ 13, 2, 5, 1, "tb", "transfer block" },
	{ 13, 2, 3, 1, "eer", "enable early recovery" },
	{ 13, 2, 2, 1, "per", "post error recovery" },
	{ 13, 2, 1, 1, "dte", "disable transfer on error" },
	{ 13, 2, 0, 1, "dcr", "disable correction" },
	{ 13, 3, 0, 8, "rrc", "read retry count" },
	{ 13, 8, 0, 8, "wrc", "write retry count" },

	/* 0x10, "tdcp", tape device configuration page */
	{ 14, 0, 7, 1, "ps", "parameters saveable" },		/* SENSE */
	{ 14, 0, 0, 6, "mpcode", "mode page code" },
	{ 14, 1, 0, 8, "mpl", "mode page length" },
	{ 14, 2, 6, 1, "cap", "change active partition" },
	{ 14, 2, 5, 1, "caf", "change active format" },
	{ 14, 2, 0, 5, "af", "active format" },
	{ 14, 3, 0, 8, "ap", "active partition" },
	{ 14, 4, 0, 8, "wbfr", "write buffer full ratio" },
	{ 14, 5, 0, 8, "rber", "read buffer empty ratio" },
	{ 14, 6, 0, 16, "wdt", "write delay time" },
	{ 14, 8, 7, 1, "dbr", "data buffer recovery" },
	{ 14, 8, 6, 1, "bis", "block identifiers supported" },
	{ 14, 8, 5, 1, "rsmk", "report setmarks" },
	{ 14, 8, 4, 1, "avc", "automatic velocity control" },
	{ 14, 8, 2, 2, "socf", "stop on consecutive filemarks" },
	{ 14, 8, 1, 1, "rbo", "recover buffer order" },
	{ 14, 8, 0, 1, "rew", "report early warning" },
	{ 14, 9, 0, 8, "gs", "gap size" },
	{ 14, 10, 5, 3, "eodd", "end-of-data defined" },
	{ 14, 10, 4, 1, "eeg", "enable EOD generation" },
	{ 14, 10, 3, 1, "sew", "synchronize at early warning" },
	{ 14, 11, 0, 24, "bsaew", "buffer size at early warning" },
	{ 14, 14, 0, 8, "sdca", "select data compression algorithm" },

	/* 0x11, "tmpp1", tape medium partition page 1 */
	{ 15, 0, 7, 1, "ps", "parameters saveable" },		/* SENSE */
	{ 15, 0, 0, 6, "mpcode", "mode page code" },
	{ 15, 1, 0, 8, "mpl", "mode page length" },
	{ 15, 2, 0, 8, "map", "maximum additional partitions" },
	{ 15, 3, 0, 8, "apd", "additional partitions defined" },
	{ 15, 4, 7, 1, "fdpart", "fixed data partitions" },
	{ 15, 4, 6, 1, "sdpart", "select data partitions" },
	{ 15, 4, 5, 1, "idpart", "initiator-defined partitions" },
	{ 15, 4, 3, 2, "psum", "partition size unit of measure" },
	{ 15, 5, 0, 8, "mfr", "medium format recognition" },
	{ 15, 8, 0, 16, "ps/1", "partition size" },
	{ 15, 10, 0, 16, "ps/2", "partition size" },
	{ 15, 12, 0, 16, "ps/3", "partition size" },
	{ 15, 14, 0, 16, "ps/4", "partition size" },

	/* 0x12, "tmpp2", tape medium partition page 2 */
	{ 16, 0, 7, 1, "ps", "parameters saveable" },		/* SENSE */
	{ 16, 0, 0, 6, "mpcode", "mode page code" },
	{ 16, 1, 0, 8, "mpl", "mode page length" },
	{ 16, 2, 0, 16, "ps/65", "partition size" },
	{ 16, 4, 0, 16, "ps/66", "partition size" },
	{ 16, 6, 0, 16, "ps/67", "partition size" },
	{ 16, 8, 0, 16, "ps/68", "partition size" },

	/* 0x13, "tmpp3", tape medium partition page 3 */
	{ 17, 0, 7, 1, "ps", "parameters saveable" },		/* SENSE */
	{ 17, 0, 0, 6, "mpcode", "mode page code" },
	{ 17, 1, 0, 8, "mpl", "mode page length" },
	{ 17, 2, 0, 16, "ps/129", "partition size" },
	{ 17, 4, 0, 16, "ps/130", "partition size" },
	{ 17, 6, 0, 16, "ps/131", "partition size" },
	{ 17, 8, 0, 16, "ps/132", "partition size" },

	/* 0x14, "tmpp4", tape medium partition page 4 */
	{ 18, 0, 7, 1, "ps", "parameters saveable" },		/* SENSE */
	{ 18, 0, 0, 6, "mpcode", "mode page code" },
	{ 18, 1, 0, 8, "mpl", "mode page length" },
	{ 18, 2, 0, 16, "ps/193", "partition size" },
	{ 18, 4, 0, 16, "ps/194", "partition size" },
	{ 18, 6, 0, 16, "ps/195", "partition size" },
	{ 18, 8, 0, 16, "ps/196", "partition size" },

	{ 0 }
};

ucdb cdb_read6;

bf bf_read6[] = {
	{ cdb_read6, sizeof (ucdb), 0, "cdb", "command descriptor block" },
	{ 0 }
};

pm pm_read6[] = {
	{ 0, 0, 0, 8, "code", "operation code", 0x08 },
	{ 0, 1, 5, 3, "lun", "logical unit" },
	{ 0, 1, 0, 21, "lba", "logical block address" },
	{ 0, 4, 0, 8, "tl", "transfer length" },
	{ 0 }
};

ucdb cdb_read10;

bf bf_read10[] = {
	{ cdb_read10, sizeof (ucdb), 0, "cdb", "command descriptor block" },
	{ 0 }
};

pm pm_read10[] = {
	{ 0, 0, 0, 8, "code", "operation code", 0x28 },
	{ 0, 1, 5, 3, "lun", "logical unit" },
	{ 0, 1, 4, 1, "dpo", "disable page out" },
	{ 0, 1, 3, 1, "fua", "force unit access" },
	{ 0, 1, 0, 1, "reladr", "relative address" },
	{ 0, 2, 0, 32, "lba", "logical block address" },
	{ 0, 7, 0, 16, "tl", "transfer length" },
	{ 0 }
};

ucdb cdb_read_capacity;
u_char misc_read_capacity_data[BUFFER_SIZE];

bf bf_read_capacity[] = {
	{ cdb_read_capacity, sizeof (ucdb), 0, "cdb",
		"command descriptor block" },
	{ misc_read_capacity_data, sizeof misc_read_capacity_data, 0, "rcd",
		"read capacity data" },
	{ 0 }
};

pm pm_read_capacity[] = {
	{ 0, 0, 0, 8, "code", "operation code", 0x25 },
	{ 0, 1, 5, 3, "lun", "logical unit" },
	{ 0, 1, 0, 1, "reladr", "relative address" },
	{ 0, 2, 0, 32, "lba", "logical block address" },
	{ 0, 8, 0, 1, "pmi", "partial medium indicator" },

	/* READ CAPACITY data */
	{ 1, 0, 0, 32, "rlba", "returned logical block address" },
	{ 1, 4, 0, 32, "blib", "block length in bytes" },
	{ 0 }
};

ucdb cdb_request_sense;
u_char misc_request_sense_data[BUFFER_SIZE];

bf bf_request_sense[] = {
	{ cdb_request_sense, sizeof (ucdb), 0, "cdb",
		"command descriptor block" },
	{ misc_request_sense_data, sizeof misc_request_sense_data, 0, "rsd",
		"request sense data" },
	{ 0 }
};

pm pm_request_sense[] = {
	{ 0, 0, 0, 8, "code", "operation code", 0x03 },
	{ 0, 1, 5, 3, "lun", "logical unit" },
	{ 0, 4, 0, 8, "tl", "transfer length", BUFFER_SIZE - 1 },

	/* error codes 0x70 and 0x71 sense data format */
	{ 1, 0, 7, 1, "valid", "valid sense data" },
	{ 1, 0, 0, 7, "ec", "error code" },
	{ 1, 1, 0, 8, "sn", "segment number" },
	{ 1, 2, 7, 1, "filemark", "filemark read" },
	{ 1, 2, 6, 1, "eom", "end of medium" },
	{ 1, 2, 5, 1, "ili", "incorrect length indicator" },
	{ 1, 2, 0, 4, "sk", "sense key", 0, 0, "" },
	{ 1, 3, 0, 32, "info", "information" },
	{ 1, 7, 0, 8, "addlen", "additional sense length" },
	{ 1, 8, 0, 32, "csi", "command specific information" },
	{ 1, 12, 0, 8, "asc", "additional sense code", 0, 0, "" },
	{ 1, 13, 0, 8, "ascq", "additional sense code qualifier" },
	{ 1, 14, 0, 8, "fruc", "field replaceable unit code" },
	{ 1, 15, 7, 1, "sksv", "sense key specific valid" },
	{ 1, 15, 0, 23, "sks", "sense key specific" },
	{ 0 }
};

ucdb cdb_rewind;

bf bf_rewind[] = {
	{ cdb_rewind, sizeof (ucdb), 0, "cdb",
		"command descriptor block" },
	{ 0 }
};

pm pm_rewind[] = {
	{ 0, 0, 0, 8, "code", "operation code", 0x01 },
	{ 0, 1, 5, 3, "lun", "logical unit" },
	{ 0, 1, 0, 1, "immed", "immediate return" },
	{ 0 }
};

ucdb cdb_space;

bf bf_space[] = {
	{ cdb_space, sizeof (ucdb), 0, "cdb",
		"command descriptor block" },
	{ 0 }
};

pm pm_space[] = {
	{ 0, 0, 0, 8, "code", "operation code", 0x11 },
	{ 0, 1, 5, 3, "lun", "logical unit" },
	{ 0, 1, 0, 3, "cf", "code field" },
	{ 0, 2, 0, 24, "count", "count" },
	{ 0 }
};

ucdb cdb_test_unit_ready;

bf bf_test_unit_ready[] = {
	{ cdb_test_unit_ready, sizeof (ucdb), 0, "cdb",
		"command descriptor block" },
	{ 0 }
};

pm pm_test_unit_ready[] = {
	{ 0, 0, 0, 8, "code", "operation code", 0x00 },
	{ 0, 1, 5, 3, "lun", "logical unit" },
	{ 0 }
};

cmd scsicmdtab[] = {
	{ bf_test_unit_ready, pm_test_unit_ready, read_cmd,
		"tur", "test unit ready" },
	{ bf_rewind, pm_rewind, read_cmd, "rew", "rewind" },
	/* rezero unit */
	{ bf_request_sense, pm_request_sense, read_var_cmd,
		"rs", "request sense", request_sense_code },
	{ bf_format_unit, pm_format_unit, write_cmd,
		"fu", "format unit" },
	/* read block limits */
	/* initialize element status */
	/* reassign blocks */
	/* get message */
	{ bf_read6, pm_read6, read_data_cmd, "rd", "read (6)" },
	/* receive */
	/* print */
	/* send message (6) */
	/* send (6) */
	/* write (6) */
	/* seek (6) */
	/* slew and print */
	/* read reverse */
	/* synchronize buffer */
	/* write filemarks */
	{ bf_space, pm_space, read_cmd, "sp", "space" },
	{ bf_inquiry, pm_inquiry, read_var_cmd, "in", "inquiry",
		inquiry_code },
	/* verify (6) */
	/* recover buffered data */
	{ bf_mode_page, pm_mode_page, mode_select_cmd, "msel", "mode select" },
	/* reserve */
	/* reserve unit */
	/* release */
	/* release unit */
	/* copy */
	{ bf_erase, pm_erase, read_cmd, "er", "erase" },
	{ bf_mode_page, pm_mode_page, mode_sense_cmd, "msen", "mode sense" },
	{ bf_load, pm_load, read_cmd, "lu", "load/unload" },
	/* scan */
	/* stop print */
	/* stop start unit */
	/* receive diagnostic results */
	/* send diagnostic */
	/* prevent allow medium removal */
	/* set window */
	/* get window */
	{ bf_read_capacity, pm_read_capacity, read_cmd,
		"rc", "read capacity" },
	/* read cd-rom capacity */
	/* get message (10) */
	{ bf_read10, pm_read10, read_data_cmd, "rd10", "read (10)" },
	/* read generation */
	/* send message (10) */
	/* send (10) */
	/* write (10) */
	/* locate */
	/* position to element */
	/* seek (10) */
	/* erase (10) */
	/* read updated block */
	/* write and verify (10) */
	/* verify (10) */
	/* search data high (10) */
	/* object position */
	/* search data equal (10) */
	/* search data low (10) */
	/* set limits (10) */
	/* get data buffer status */
	/* pre-fetch */
	/* read position */
	/* synchronize cache */
	/* lock unlock cache */
	/* read defect data (10) */
	/* medium scan */
	/* compare */
	/* copy and verify */
	/* write buffer */
	/* read buffer */
	/* update block */
	/* read long */
	/* write long */
	/* change definition */
	/* write same */
	/* read sub-channel */
	/* read TOC */
	/* read header */
	/* play audio (10) */
	/* play audio MSF */
	/* play audio track index */
	/* play track relative (10) */
	/* pause resume */
	/* log select */
	/* log sense */
	/* mode select (10) */
	/* mode sense (10) */
	/* move medium */
	/* play audio (12) */
	/* exchange medium */
	/* get message (12) */
	/* read (12) */
	/* play track relative (12) */
	/* send message (12) */
	/* write (12) */
	/* erase (12) */
	/* write and verify (12) */
	/* verify (12) */
	/* search data high (12) */
	/* search data equal (12) */
	/* search data low (12) */
	/* set limits (12) */
	/* request volume element address */
	/* send volume tag */
	/* read defect data (12) */
	/* read element status */
	{ 0 }
};

ht httab[] = {
	{ scsicmdtab, "scsi", "SCSI commands" },
	/* later, add host adapter commands here */
	{ 0 }
};
