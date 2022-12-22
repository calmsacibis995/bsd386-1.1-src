/* $Header: /bsdi/MASTER/BSDI_OS/contrib/psgraph/psgraph/input.c,v 1.1.1.1 1994/01/03 23:14:16 polk Exp $ */

/*
 *               Copyright 1989, 1992 Digital Equipment Corporation
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

/* 
 * input.c - Read and parse command and data input
 * 
 * Author:	Christopher A. Kent
 * 		Western Research Laboratory
 * 		Digital Equipment Corporation
 * Date:	Wed Jan  4 1989
 */

/*
 * $Log: input.c,v $
 * Revision 1.1.1.1  1994/01/03  23:14:16  polk
 * New contrib utilities
 *
 * Revision 1.9  92/08/04  17:55:05  mogul
 * undo RCS botch
 * 
 * Revision 1.8  1992/04/03  23:55:47  kent
 * Fixed a problem where "include" reset the world.
 *
 * Revision 1.7  1992/04/01  23:27:34  kent
 * Added datalabel verb, fixed a bug in handling blank input lines.
 *
 * Revision 1.6  1992/03/31  23:13:12  kent
 * Added "dataticks" verb.
 *
 * Revision 1.5  1992/03/31  02:31:34  kent
 * Added markergray verb and fixed inverted gray values.
 *
 * Revision 1.4  1992/03/31  00:21:29  kent
 * Added "include" verb
 *
 * Revision 1.3  1992/03/31  00:07:39  kent
 * Added markerscale verb.
 *
 * Revision 1.2  1992/03/30  23:33:47  kent
 * Added halfopen, halfticks grid styles, range frames, and gray.
 *
 * Revision 1.1  1992/03/20  21:25:43  kent
 * Initial revision
 *
 * Revision 1.7  92/02/21  17:13:21  mogul
 * Added Digital license info
 * 
 * Revision 1.6  90/12/11  20:41:26  reid
 * Added code to parse input for new "color" and "linewidth" commands
 * 
 * Revision 1.5  89/01/10  18:19:57  kent
 * Moved marker code to prolog, added error checking and messages.
 * 
 * Revision 1.4  89/01/09  22:18:41  kent
 * Added log scales.
 * 
 * Revision 1.3  89/01/04  17:30:27  kent
 * Made command line arguments override compiled-in defaults for
 * all plots in a run, not just the first one. 
 * 
 * Revision 1.2  89/01/04  15:22:03  kent
 * Massive renaming. No functional change.
 * 
 * Revision 1.1  89/01/04  13:57:49  kent
 * Initial revision
 * 
 */

static char rcs_ident[] = "$Header: /bsdi/MASTER/BSDI_OS/contrib/psgraph/psgraph/input.c,v 1.1.1.1 1994/01/03 23:14:16 polk Exp $";

#include <stdio.h>

#include "psgraph.h"

/*
 * Read the input consisting of both numeric and text data.  An array, Token[*]
 * is built by this routine containing both numeric (type==POINT) and non-numeric
 * data.  Only the commands that take affect at a point relative to their
 * appearence in the input are placed in the Token array.  All others just
 * affect global data.
 */

doinput(s)
    FILE *s;
{
	char *argv[ARGC];
	int argc;
	char buf[BUFSIZ], cmd[BUFSIZ];

	for(;;) {
		if ( NumTokens >= SizeofToken ) {
			SizeofToken += TOKENINC;
			Token = (token_t *)realloc((char *)Token,(unsigned)(SizeofToken*sizeof(token_t)));
		}
		if( fgets(buf, BUFSIZ, s) == NULL )
			break;
		if ( strlen(buf)==0 || *buf=='#' || buf[0]=='\n')
			continue;
		strcpy(cmd,buf);
		parse(cmd,&argc,argv);
#ifdef DEBUG
		if ( Debug ) {
			fprintf(stderr,"argc=%d, ",argc);
			fprintf(stderr,"input=%s\n",buf);
		}
#endif
		if ( isalpha(argv[0][0]) ) {
			if ( docmd(argc,argv) )
				fprintf(stderr,"Error in input: %s\n",buf);
		} else {
			Token[NumTokens].type = POINT;
			Token[NumTokens].xval = atof(argv[0]);
			Token[NumTokens].yval = 0.0;
			if ( argc > 1 ) Token[NumTokens].yval = atof(argv[1]);
			if ( argc > 2 )
				Token[NumTokens].label = newstr(argv[2]);
			else
				Token[NumTokens].label = NULL;
			NumTokens++;
		}
	}
}

docmd(argc,argv)
int argc;
char *argv[];
{
    	FILE *f;
	
	if ( strcmp(argv[0],"break")==0 ) {
		Token[NumTokens].type = BREAK;
		NumTokens++;
	} else if ( strcmp(argv[0],"include")==0 ) {
	    	if ( argc > 1 ) {
		    f = fopen(argv[1], "r");
		    if (f != NULL)
			doinput(f);
		}
	} else if ( strcmp(argv[0],"line")==0 ) {
		Token[NumTokens].type = LINETYPE;
		if ( argc > 1 )
			Token[NumTokens].label = newstr(argv[1]);
		else
			Token[NumTokens].label = NULL;
		NumTokens++;
	} else if ( strcmp(argv[0],"color")==0 ) {
		Token[NumTokens].type = LINECOLOR;
		if ( argc > 1 )
			Token[NumTokens].label = newstr(argv[1]);
		else
			Token[NumTokens].label = NULL;
		NumTokens++;
	} else if ( strcmp(argv[0],"linewidth")==0 ) {
		Token[NumTokens].type = LINEWIDTH;
		if ( argc > 1 )
			Token[NumTokens].label = newstr(argv[1]);
		else
			Token[NumTokens].label = "0.6";
		NumTokens++;
	} else if ( strcmp(argv[0],"spline")==0 ) {
		Token[NumTokens].type = SPLINE;
		NumTokens++;
	} else if ( strcmp(argv[0],"label")==0 )
		DoAxisLabels = TRUE;
	else if ( strcmp(argv[0],"nolabel")==0 )
		DoAxisLabels = FALSE;
	else if ( strcmp(argv[0],"transparent")==0 ) {
		Token[NumTokens].type = TRANS;
		Token[NumTokens].ival = TRUE;
		NumTokens++;
	} else if ( strcmp(argv[0],"notransparent")==0 ) {
		Token[NumTokens].type = TRANS;
		Token[NumTokens].ival = FALSE;
		NumTokens++;
	} else if ( strcmp(argv[0],"grid")==0 ) {
		Xaxis.gridtype = Yaxis.gridtype = gridval(argv[1]);
		if (Xaxis.gridtype == HALFOPEN) {
		    Xaxis.gridtype = Yaxis.gridtype = OPEN;
		    Xaxis.halfgrid = Yaxis.halfgrid = TRUE;
		}
		if (Xaxis.gridtype == HALFTICKS) {
		    Xaxis.gridtype = Yaxis.gridtype = TICKS;
		    Xaxis.halfgrid = Yaxis.halfgrid = TRUE;
		}
	} else if ( strncmp(argv[0],"datatick",8)==0 ) {
	    	Xaxis.datatick = Yaxis.datatick = TRUE;
	} else if ( strncmp(argv[0],"datalabel",9)==0 ) {
	    	Xaxis.datalabel = Yaxis.datalabel = TRUE;
	} else if ( strcmp(argv[0],"tickgray")==0 )
		Xaxis.tickgray = Yaxis.tickgray = 1.0 - atof(argv[1])/100.0;
	else if ( strcmp(argv[0],"axisgray")==0 )
		Xaxis.axisgray = Yaxis.axisgray = 1.0 - atof(argv[1])/100.0;
	else if ( strcmp(argv[0],"title")==0 ) {
	  	if (argc > 1) 
			Title.title = newstr(argv[1]);
		else 
			Title.title = "";
	} else if ( strcmp(argv[0],"titlefont")==0 )
		Title.font = newstr(argv[1]);
	else if ( strncmp(argv[0],"tick", 4)==0 ) {
		TickLen = Tick2Len = atof(argv[1]);
		if ( argc > 2 )
			Tick2Len = atof(argv[2]);
	} else if ( strcmp(argv[0], "clip")==0 ) {
		if ( argc > 1 )
			ClipDist = atof(argv[1]);
	} else if ( strcmp(argv[0],"width")==0 )
		Xaxis.size = atof(argv[1]);
	else if ( strcmp(argv[0],"height")==0 )
		Yaxis.size = atof(argv[1]);
	else if ( strcmp(argv[0],"rangeframe")==0 )
	    	Xaxis.rangeframe = Yaxis.rangeframe = TRUE;
	else if ( strcmp(argv[0],"center")==0 )
		Xcenter = atof(argv[1]);
	else if ( strcmp(argv[0],"marker")==0 ) {
		if ( argc > 1 ) {
			Token[NumTokens].type = MARKER;
			Token[NumTokens].label = newstr(argv[1]);
			NumTokens++;
		}
	} else if ( strcmp(argv[0],"markerscale")==0 ) {
	    	if ( argc > 1 ) {
			Token[NumTokens].type = MARKERSCALE;
			Token[NumTokens].val[0] = atof(argv[1]);
			NumTokens++;
		}
	} else if ( strcmp(argv[0],"markergray")==0 ) {
	    	if ( argc > 1 ) {
			Token[NumTokens].type = MARKERGRAY;
			Token[NumTokens].val[0] = 1.0 - atof(argv[1])/100.0;
			NumTokens++;
		}
	} else if ( strcmp(argv[0],"x")==0 )
		return domods(argc, argv, &Xaxis);
	else if ( strcmp(argv[0],"y")==0 )
		return domods(argc, argv, &Yaxis);
	else if ( strcmp(argv[0],"font")==0 ) {
		Token[NumTokens].type = FONT;
		if (argc > 1)
			Token[NumTokens].label = newstr(argv[1]);
		else
			Token[NumTokens].label = NULL;
		NumTokens++;
	} else
		return TRUE;
	return FALSE;
}
/*
 * domods - parse and handle input lines for making modifications to
 * x & y argument structure.
 *
 * The input lines handled by this module are
 *
 *	x options
 *
 * where "options" is one or more of the following
 *
 *	intervals N
 *	log
 *	min N
 *	max N
 *	rangeframe
 *	step N
 *	tick N
 *	size N
 *	offset N
 *	label "foo"
 *	grid {none,open,ticks,full,halfopen,halfticks}
 *	words
 *	font "foo"
 */

domods(argc, argv, p)
int argc;
char *argv[];
axis_t *p;
{
	int arg;
	for ( arg=1 ; arg<argc ; arg++ ) {
		if (strcmp(argv[arg], "intervals") == 0)
			p->intervals = atoi(argv[++arg]);
		else if ( strcmp(argv[arg],"log")==0 )
			p->tform = LOG10;
		else if ( strcmp(argv[arg],"min")==0 ) {
			p->minflag = TRUE;
			p->gmin = atof(argv[++arg]);
		} else if ( strcmp(argv[arg],"max")==0 ) {
			p->maxflag = TRUE;
			p->gmax = atof(argv[++arg]);
		} else if ( strcmp(argv[arg],"step")==0 ) {
			p->distf = TRUE;
			p->dist = atof(argv[++arg]);
		} else if ( strncmp(argv[arg],"tick",4)==0 ) {
			p->tickflag = TRUE;
			p->tick = atof(argv[++arg]);
		} else if ( strcmp(argv[arg],"rangeframe")==0 ) {
		    	p->rangeframe = TRUE;
		} else if ( strcmp(argv[arg],"offset")==0 ) {
			p->offset = atof(argv[++arg]);
		} else if ( strcmp(argv[arg],"label")== 0 ) {
			p->label = newstr(argv[++arg]);
		} else if ( strcmp(argv[arg],"font")== 0 ) {
			p->font = newstr(argv[++arg]);
		} else if ( strcmp(argv[arg],"size")== 0 ) {
			p->size = atof(argv[++arg]);
		} else if ( strcmp(argv[arg],"grid")==0 ) {
			p->gridtype = gridval(argv[++arg]);
			if (p->gridtype == HALFOPEN) { /* hack hack */
			    p->gridtype = OPEN;
			    p->halfgrid = TRUE;
			}
			if (p->gridtype == HALFTICKS) {
			    p->gridtype = TICKS;
			    p->halfgrid = TRUE;
			}
		} else if ( strncmp(argv[0],"datatick",8)==0 ) {
		    	p->datatick = TRUE;
		} else if ( strncmp(argv[0],"datalabel",9)==0 ) {
		    	p->datalabel = TRUE;
		} else if ( strcmp(argv[0],"tickgray")==0 )
		    	p->tickgray =  1.0 - atof(argv[++arg])/100.0;
		else if ( strcmp(argv[0],"axisgray")==0 )
		    	p->axisgray = 1.0 - atof(argv[++arg])/100.0;
		else
			return TRUE;
	}
	return FALSE;
}

