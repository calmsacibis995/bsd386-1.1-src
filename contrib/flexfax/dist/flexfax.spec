#	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/dist/flexfax.spec,v 1.1.1.1 1994/01/14 23:09:33 torek Exp $
#
# FlexFAX Facsimile Software
#
# Copyright (c) 1990, 1991, 1992, 1993 Sam Leffler
# Copyright (c) 1991, 1992, 1993 Silicon Graphics, Inc.
# 
# Permission to use, copy, modify, distribute, and sell this software and 
# its documentation for any purpose is hereby granted without fee, provided
# that (i) the above copyright notices and this permission notice appear in
# all copies of the software and related documentation, and (ii) the names of
# Sam Leffler and Silicon Graphics may not be used in any advertising or
# publicity relating to the software without the specific, prior written
# permission of Sam Leffler and Silicon Graphics.
# 
# THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
# EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
# WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
# 
# IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
# ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
# OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
# WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
# LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
# OF THIS SOFTWARE.
#
define CUR_MAJ_VERS	1006		# Major Version number
define CUR_MIN_VERS	002		# Minor Version number
define CUR_VERS		${CUR_MAJ_VERS}${CUR_MIN_VERS}${ALPHA}
define FAX_NAME		"FlexFAX"
define FAX_VNUM		2.2.0

include flexfax.alpha

product flexfax
    id	"${FAX_NAME} Facsimile Software, Version ${FAX_VNUM}"
    inplace

    image sw
	id	"${FAX_NAME} Software"
	version	"${CUR_VERS}"
	subsys client default
	    id	"${FAX_NAME} Client Software"
	    exp	"flexfax.sw.client"
	endsubsys
	subsys server
	    id	"${FAX_NAME} Server Software"
	    # need DPS fonts and VM startup file for ps2fax
	    prereq (
		dps_eoe.sw.dps		1006000000 maxint
		dps_eoe.sw.dpsfonts	1006000000 maxint
	    )
	    exp		"flexfax.sw.server"
	endsubsys
    endimage

    image man
	id	"${FAX_NAME} Documentation"
	version	"${CUR_VERS}"
	subsys relnotes default
	    id	"${FAX_NAME} Release Notes"
	    exp	"flexfax.man.relnotes"
	endsubsys
	subsys readme default
	    id "${FAX_NAME} README Notes"
	    exp "flexfax.man.readme"
	endsubsys
	subsys client default
	    id	"${FAX_NAME} Client Manual Pages"
	    exp	"flexfax.man.client"
	endsubsys
	subsys server
	    id	"${FAX_NAME} Server Manual Pages"
	    exp	"flexfax.man.server"
	endsubsys
	subsys doc
	    id	"${FAX_NAME} Supplementary Materials"
	    exp "flexfax.man.doc"
	endsubsys
    endimage
endproduct
