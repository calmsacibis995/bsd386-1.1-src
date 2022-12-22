/*
 * psgsimp.c
 *
 * Simplify a psgraph-style plot file, removing graph points closer
 * together than a specified threshold.
 *
 * Jeffrey Mogul	DECWRL		10 January 1992
 * 
 *               Copyright (c) 1992 Digital Equipment Corporation
 *                          All Rights Reserved
 * 
 * 
 * Permission to use, copy, and modify this software and its documentation
 * is hereby granted only under the following terms and conditions.  Both
 * the above copyright notice and this permission notice must appear in
 * all copies of the software, derivative works or modified versions, and
 * any portions threof, and both notices must appear in supporting
 * documentation.
 * 
 * Users of this software agree to the terms and conditions set forth
 * herein, and hereby grant back to Digital a non-exclusive, unrestricted,
 * royalty-free right and license under any changes, enhancements or
 * extensions made to the core functions of the software, including but
 * not limited to those affording compatibility with other hardware or
 * software environments, but excluding applications which incorporate
 * this software.  Users further agree to use their best efforts to return
 * to Digital any such changes, enhancements or extensions that they make
 * and inform Digital of noteworthy uses of this software.  Correspondence
 * should be provided to Digital at:
 * 
 *                       Director of Licensing
 *                       Western Research Laboratory
 *                       Digital Equipment Corporation
 *                       250 University Avenue
 *                       Palo Alto, California  94301  
 * 
 * This software may be distributed (but not offered for sale or
 * transferred for compensation) to third parties, provided such third
 * parties agree to abide by the terms and conditions of this notice.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS" AND DIGITAL EQUIPMENT CORP. DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS.   IN NO EVENT SHALL DIGITAL
 * EQUIPMENT CORPORATION BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <math.h>

#ifndef	HUGE_VAL
#define	HUGE_VAL	HUGE
#endif

Usage()
{
	fprintf(stderr, "Usage: psgsimp thresh-pct <infile >outfile\n");
}

main(argc, argv)
int argc;
char **argv;
{
	double thresh;
	static char linebuf[1024];
	static char lasthidden[1024];
	float lastx, lasty;
	float thisx, thisy;	/* sscanf() deals in floats, not doubles */
	float deltax, deltay;
	static char otherstuff[1024];

	if (argc != 2) {
	    Usage();
	    exit(1);
	}
	
	thresh = atof(argv[1]);
	
	lastx = HUGE_VAL;
	lasty = HUGE_VAL;

	lasthidden[0] = '\0';

	while (gets(linebuf)) {
	    if (sscanf(linebuf, "%f %f %s", &thisx, &thisy, otherstuff) == 2) {
		/* only x, y value on this line */

		/* compute deltax ratio */
		if ((thisx == 0.0) && (lastx == 0.0)) {
		    deltax = 0.0;
		}
		else if (thisx == 0.0) {
		    deltax = (thisx - lastx)/lastx;
		}
		else {
		    deltax = (thisx - lastx)/thisx;
		}
		if (deltax < 0.0)
		    deltax = -deltax;
		
		/* compute deltay ratio */
		if ((thisy == 0.0) && (lasty == 0.0)) {
		    deltay = 0.0;
		}
		else if (thisy == 0.0) {
		    deltay = (thisy - lasty)/lasty;
		}
		else {
		    deltay = (thisy - lasty)/thisy;
		}
		if (deltay < 0.0)
		    deltay = -deltay;
		
#ifdef	DEBUG
		printf("# x %f -> %f (%f), y %f -> %f (%f)\n",
			lastx, thisx, deltax,
			lasty, thisy, deltay);
#endif	DEBUG
		
		if ((deltax < thresh) && (deltay < thresh)) {
		    printf("##%s\n", linebuf);
		    strcpy(lasthidden, linebuf);
		    continue;
		}

		lastx = thisx;
		lasty = thisy;
		if (lasthidden[0] != '\0') {
		    printf("%s\n", lasthidden);
		}
		printf("%s\n", linebuf);
		lasthidden[0] = '\0';
	    }
	    else {
		if (lasthidden[0] != '\0') {
		    printf("%s\n", lasthidden);
		}
		printf("%s\n", linebuf);
		lasthidden[0] = '\0';
		if (linebuf[0] != '#') {
		    /* if non-comment, ensure next point is not suppressed */
		    lastx = HUGE_VAL;
		    lasty = HUGE_VAL;
		}
	    }
	}
}
