/* $Header: /bsdi/MASTER/BSDI_OS/contrib/psgraph/psgraph/main.c,v 1.1.1.1 1994/01/03 23:14:16 polk Exp $ */
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
 * main.c - produce PostScript graphs
 * 
 * Author:	Christopher A. Kent
 * 		Western Research Laboratory
 * 		Digital Equipment Corporation
 * Date:	Wed Jan  4 1989
 */

/*
 * $Log: main.c,v $
 * Revision 1.1.1.1  1994/01/03  23:14:16  polk
 * New contrib utilities
 *
 * Revision 1.10  1992/09/14  20:44:59  mogul
 * On MSDOS (and related systems), the read() call can return
 * less than the specified count even when additional data
 * remains in the file.  (This is documented behavior for text
 * files; apparently, some sort of end-of-line conversion is
 * done, using the caller's buffer.)  This means that copyFile()
 * needs to be somewhat more robust.
 *
 * Revision 1.9  1992/08/04  17:55:08  mogul
 * undo RCS botch
 *
 * Revision 1.8  1992/06/17  22:14:41  kent
 * Make axis specs with max < min work, fix some bugs with centering
 * and multiple args.
 *
 * Revision 1.7  1992/04/03  23:55:47  kent
 * Fixed a problem where "include" reset the world.
 *
 * Revision 1.6  1992/04/01  23:27:34  kent
 * Added datalabel verb, fixed a bug in handling blank input lines.
 *
 * Revision 1.5  1992/04/01  00:38:24  kent
 * Fixed a problem with half grids specified with -g.
 *
 * Revision 1.4  1992/03/31  02:31:34  kent
 * Added markergray verb and fixed inverted gray values.
 *
 * Revision 1.3  1992/03/31  00:21:29  kent
 * Added "include" verb
 *
 * Revision 1.2  1992/03/30  23:33:47  kent
 * Added halfopen, halfticks grid styles, range frames, and gray.
 *
 * Revision 1.1  1992/03/20  21:25:43  kent
 * Initial revision
 *
 * Revision 1.12  92/02/21  17:13:22  mogul
 * Added Digital license info
 * 
 * Revision 1.11  91/12/19  16:00:52  mogul
 * Avoid an infinite loop if the graph is a horizontal or vertical
 * line.
 * 
 * Revision 1.10  91/02/04  16:48:01  mogul
 * Fixed text, marker colors
 * 
 * Revision 1.9  90/12/11  20:39:42  reid
 * Added default values for LineColor and LineWidth
 * 
 * Revision 1.8  89/02/03  09:33:06  kent
 * Use floor in calculating log limits.
 * 
 * Revision 1.7  89/01/10  18:19:59  kent
 * Moved marker code to prolog, added error checking and messages.
 * 
 * Revision 1.6  89/01/09  22:18:43  kent
 * Added log scales.
 * 
 * Revision 1.5  89/01/04  18:12:19  kent
 * Added -p flag to specify alternate prologue file.
 * 
 * Revision 1.4  89/01/04  17:57:36  kent
 * Moved font stuff from main.c to output.c.
 * newfont() sets PS fontsize variable so white background is the right size.
 * 
 * Revision 1.3  89/01/04  17:30:29  kent
 * Made command line arguments override compiled-in defaults for
 * all plots in a run, not just the first one. 
 * 
 * Revision 1.2  89/01/04  15:22:05  kent
 * Massive renaming. No functional change.
 * 
 * Revision 1.1  89/01/04  13:57:54  kent
 * Initial revision
 * 
 */

static char rcs_ident[] = "$Header: /bsdi/MASTER/BSDI_OS/contrib/psgraph/psgraph/main.c,v 1.1.1.1 1994/01/03 23:14:16 polk Exp $";

/*
 * This program has a long history. It started out as a program to take
 * graph(1) input and produce pic/troff output. That was done by Bob Brown. In
 * the process, he added to the input language to make it a lot more useful.
 * Chris Kent then took it and made it produce PostScript. As it got more use,
 * Chris' users didn't care about troff, so that code all went away, but they
 * wanted even more features.
 *
 * This incarnation was started when Chris was asked to add "one more feature"
 * and the single-file version just got to be too much to handle. Now it's
 * broken into several files, and the effects of time and feeping creaturism
 * have been cleaned up. But it almost certainly retains some code from some of
 * these versions:
 *
 *  Bob Brown/RIACS/NASA Ames
 *  Mostly untested. 3/86
 *  Converted to emit PostScript instead of Pic
 *		Chris Kent, DECWRL, 5/87
 *  More PostScript cleanup -- cak 11/87
 *  A few new options, made it use a private dictionary, attacked some
 *	roundoff problems -- cak 2/88
 *
 */

#include <stdio.h>
#include <pwd.h>
#include <sys/file.h>

#include "psgraph.h"

main(argc,argv)
char *argv[];
{
	int	i;
	long	clock;
	char	hostname[256];
	struct passwd	*pwd;
	fontName_t	*fp;
	
	MinX = MinY = MaxX = MaxY = 0.0;
	Token = (token_t *) calloc(TOKENINC, sizeof(token_t));
	SizeofToken = TOKENINC;
	procargs(argc,argv);

	FontList = (fontName_t *) malloc(sizeof (fontName_t));
	FontList->name = newstr("Times-Roman");
	FontList->next = (fontName_t *) NULL;

	/* put out comment header */
	printf("%%!PS-Adobe-1.0\n");
	pwd = getpwuid(getuid());
	gethostname(hostname, sizeof hostname);
	printf("%%%%Creator: %s:%s (%s)\n",hostname,
		pwd->pw_name, pwd->pw_gecos);
	printf("%%%%Title: PostScript graph file\n");
	printf("%%%%CreationDate: %s",(time(&clock), ctime(&clock)));
	printf("%%%%DocumentFonts: (atend)\n");
	printf("%%%%Pages: (atend)\n");
	printf("%%%%BoundingBox: (atend)\n");
	printf("%%%%EndComments\n");
	
	/* interpolate prolog and fixed header routines */
	
	if (copyFile(Prolog, stdout) < 0) {
		perror(Prolog);
		exit(1);
	}
	printf("%%%%EndProlog\n");
	
	if ( Files==0 ) {
		process();
	} else {
		for ( i=0 ; i<Files ; i++ ) {
			if ( freopen(File[i],"r", stdin)==NULL )
				perror(File[i]);
			else
				process();
		}
	}
	printf("%%%%Trailer\n");
	printf("EndPSGraph\n");
	printf("%%%%DocumentFonts: %s", FontList->name);
	for (fp = FontList->next; fp; fp = fp->next)
		printf(", %s", fp->name);
	printf("\n");
	printf("%%%%Pages: %d\n", CurrentPage);
	printf("%%%%BoundingBox: %d %d %d %d\n",
		(int) (MinX * 72), (int) (MinY * 72),
		(int) (MaxX * 72), (int) (MaxY * 72));
	exit(0);
}

copyFile(fileName, stream)
char	*fileName;
FILE	*stream;
{
	int	fd, fo;
	char	buf[BUFSIZ];
	int	cnt;

	fflush(stream);
	fo = fileno(stream);
	if ((fd = open(fileName, O_RDONLY, 0)) < 0) 
		return -1;
	do if (cnt = read(fd, buf, sizeof(buf)))
		if (write(fo, buf, cnt) != cnt) return -2;
	while (cnt > 0);	/* OS2 read() can return < full buffer */
	close(fd);
	fflush(stream);
	return 0;
}

process()
{
	/* reset everything */
	init(&Xaxis);
	init(&Yaxis);

	/* copy command line defaults into place... */
	BreakAfterLabel = Args.breakAfterLabel;
	Xcenter = Args.center;
	Xaxis.size = Width;
	Yaxis.size = Height;
	Xaxis.offset = Xoffset;
	Yaxis.offset = Yoffset;
	copyLimit(&Xaxis, &Xlim);
	copyLimit(&Yaxis, &Ylim);

	/* reset state */
	DoAxisLabels = TRUE;
	TransparentLabels = FALSE;
	UseSpline = FALSE;
	NumTokens = 0;
	ClipDist = 0.05;
	LineType = "solid";
	LineColor = "black";
	LineWidth = "0.6";
	TickLen = 0.1;
	Tick2Len = 0.05;
	UseMarker = NULL;
	TextColor = "black";
	MarkColor = "black";
	NumTokens = 0;
	doinput(stdin);
	if ( numpnts() > 0 ) {
		transpose();
		dolimits(0);
		dolimits(1);
		doplot();
	}
}

init(p)
axis_t *p;
{
	p->tickflag = FALSE;
	p->label = NULL;
	p->gridtype = GridType;
	p->tickgray = 0.0;
	p->axisgray = 0.0;
	p->datatick = FALSE;
	p->datalabel = FALSE;
	p->halfgrid = HalfGrid;
	p->rangeframe = FALSE;
	p->intervals = 0;
}

initLimit(l)
limit_t	*l;
{
	l->minflag = FALSE;
	l->maxflag = FALSE;
	l->distf = FALSE;
	l->tform = IDENT;
}

copyLimit(a, l)
axis_t	*a;
limit_t	*l;
{
	a->tform   = l->tform;
	a->minflag = l->minflag;
	a->maxflag = l->maxflag;
	a->distf   = l->distf;
	a->min     = l->min;
	a->max     = l->max;
	a->dist    = l->dist;
}

procargs(argc, argv)
int argc;
char *argv[];
{
	int arg, more;
	char *swptr;

	Files = 0;
	File = (char **)calloc(argc, sizeof (char *));
	Args.breakAfterLabel = FALSE;
	Args.center = 0.0;
	GridType = FULL;
	HalfGrid = FALSE;
	Height = 6.5;
	Width = 6.5;
	Prolog = PROLOG;
	Title.title = NULL;
	Title.font = NULL;
	Xoffset = 0.0;
	Yoffset = 0.0;
	TransposeAxes = FALSE;

	for ( arg=1 ; arg<argc ; arg++ ) {
		if ( argv[arg][0] == '-' ) {
			more = 1;
			swptr = &argv[arg][1];
			while ( more && *swptr!='\0' ) {
				switch ( *swptr++ ) {
				case 'b':	/*breaks*/
					Args.breakAfterLabel = TRUE;
					break;
				case 'c':
					if (isfloat(argv[arg+1]))
						Args.center = 
							atof(argv[++arg]);
					else
						usagexit(argv[0]);
					break;
				case 'g':
					if ( arg+1 >= argc )
						usagexit(argv[0]);
					GridType = gridval(argv[++arg]);
					if (GridType == HALFOPEN) {
					    GridType = OPEN;
					    HalfGrid = TRUE;
					}
					if (GridType == HALFTICKS) {
					    GridType = TICKS;
					    HalfGrid = TRUE;
					}
					break;
				case 'h':
					if ( isfloat(argv[arg+1]) )
						Height = atof(argv[++arg]);
					else
						usagexit(argv[0]);
					break;
				case 'l':
					Title.title = newstr(argv[++arg]);
					break;
				case 'p':
					if (arg+1 >= argc)
						usagexit(argv[0]);
					Prolog = argv[++arg];
					break;
				case 'P':
					Preview = TRUE;
					break;
				case 'r':
					if ( isfloat(argv[arg+1]) )
						Xoffset = atof(argv[++arg]);
					else
						usagexit(argv[0]);
					break;
				case 't':
					TransposeAxes = TRUE;
					break;
				case 'u':
					if ( isfloat(argv[arg+1]) )
						Yoffset = atof(argv[++arg]);
					else
						usagexit(argv[0]);
					break;
				case 'w':
					if ( isfloat(argv[arg+1]) )
						Width = atof(argv[++arg]);
					else
						usagexit(argv[0]);
					break;
				case 'x':
					arg = limargs(&Xlim,argc,argv,arg+1)-1;
					break;
				case 'y':
					arg = limargs(&Ylim,argc,argv,arg+1)-1;
					break;
				default:
					usagexit(argv[0]);
				}
			}
		} else { /* there's no dash in front */
			File[Files++] = argv[arg];
		}
	}
}
usagexit(pgm)
char *pgm;
{
	fprintf(stderr,"usage: %s \n",pgm);
	exit(1);
}

/*
 * numpnts - returns the number of actual data points
 */

numpnts()
{
	int i, cnt;
	cnt = 0;
	for ( i=0 ; i<NumTokens ; i++ )
		if ( Token[i].type == POINT )
			cnt++;
	return cnt;
}

transpose()
{
	register int	i;
	float		f;
	axis_t		t;

	if(!TransposeAxes)
		return;
	t = Xaxis; 
	Xaxis = Yaxis; 
	Yaxis = t;
	for(i= 0; i < NumTokens; i++) {
		if ( Token[i].type != POINT ) continue;
		f = Token[i].xval; 
		Token[i].xval = Token[i].yval; 
		Token[i].yval = f;
	}
}

char *newstr(s)
char *s;
{
	char *t;
	t = (char *)malloc((unsigned)(strlen(s)+1));
	strcpy(t,s);
	return t;
}

/*
 * isfloat - returns TRUE if the argument is a floating point number
 */

isfloat(cp)
char *cp;
{
	while ( *cp && isspace(*cp) )
		cp++;
	if ( *cp == '-' )
		cp++;
	if ( isdigit(*cp) || *cp=='.' )
		return TRUE;
	return FALSE;
}

/*
 * dolimits - compute the minimum and maximum of the data points.
 *	      compute the minimum and maximum to plot on the grid.
 */

dolimits(v)
{

	if (AxisArgs[v].tform == IDENT)
		doLinearLimits(v);
	else
		doLogLimits(v);
}

doLinearLimits(v)
register int	v;
{
	register axis_t *argp;
	register int i;
	float min, max;

	argp = &AxisArgs[v];
	argp->min = HUGE;
	for ( i=0; i<NumTokens ; i++ )
	    if ( Token[i].type == POINT )
		argp->min = MIN(argp->min, Token[i].val[v]);

	argp->max = -HUGE;
	for ( i=0; i<NumTokens ; i++ )
	    if ( Token[i].type == POINT )
		argp->max = MAX(argp->max, Token[i].val[v]);

	if (argp->minflag)
	    min = argp->gmin;
	else
	    min = argp->min;
	if (argp->maxflag)
	    max = argp->gmax;
	else
	    max = argp->max;

	scale1(min, max, 5,
		&argp->pmin, &argp->pmax, &argp->distp);
	if ( !argp->minflag )
		argp->gmin = argp->pmin;
	if ( !argp->maxflag )
		argp->gmax = argp->pmax;
	if ( argp->distf )
		argp->distg = argp->dist;
	else
		argp->distg = argp->distp;

	/* avoid infinite loops */
	if (argp->gmax == argp->gmin) {
	    if (argp->gmin > 0.0)
		argp->gmin = 0.0;
	    else
		argp->gmax += 1.0;
	}
}

doLogLimits(v)
register int	v;
{
	register axis_t *argp;
	register int i;

	argp = &AxisArgs[v];

	argp->min = HUGE;
	for ( i=0; i<NumTokens ; i++ )
	    if ( Token[i].type == POINT ) {
		if (Token[i].val[v] <= 0.0) {
		    fprintf(stderr, 
			    "Bad log point (%g, %g)\n", 
			    Token[i].val[0], 
			    Token[i].val[1]);
		    Token[i].type = IGNORE;
		    continue;
		}
		argp->min = MIN(argp->min, Token[i].val[v]);
	    }
	if (argp->min <= 0.0) {
		fprintf(stderr, "Illegal log minimum %g\n", argp->min);
		exit(-1);
	}
	argp->max = -HUGE;
	for ( i=0; i<NumTokens ; i++ )
	    if ( Token[i].type == POINT )
		argp->max = MAX(argp->max, Token[i].val[v]);

	argp->pmin = ipow(10.0, (int)floor(log10(argp->min)));
	argp->pmax = ipow(10.0, (int)floor(log10(argp->max)) + 1);
	argp->distp = copysign(2.0, (argp->gmax - argp->gmin)) ;
	
/*	scale3(argp->min, argp->max, argp->intervals,
		&argp->pmin, &argp->pmax, &argp->distp);
*/
	if ( !argp->minflag )
		argp->gmin = argp->pmin;
	if ( !argp->maxflag )
		argp->gmax = argp->pmax;
	if ( argp->distf )
		argp->distg = argp->dist;
	else
		argp->distg = argp->distp;
	argp->lgmin = log10(argp->gmin);
	argp->lgmax = log10(argp->gmax);
}

/*
 *----------------------------------------------------------------------
 *
 * TEXT PARSING ROUTINES
 *
 * parse - break a string into substrings
 */

parse(line, argc, argv)
char *line;
int *argc;
char *argv[];
{
	char *ptr, *nextarg();

	ptr = line;
	*argc = 0;
	while ((ptr=nextarg(ptr,&argv[*argc])) != NULL )
		(*argc)++;
	argv[*argc] = NULL;
}
char *
nextarg(line, start)
register char *line, **start;
{
	bool esc;
	register char *out;
	char delim;
	while ( isspace(*line) && *line != EOS )
		line++;
	if ( *line == EOS )
		return NULL;
	*start = line;
	if ( *line=='\'' || *line=='"' ) {
		delim = *line;
		out = ++line;
		(*start)++;
		esc = FALSE;
		while(TRUE) {
			if ( *line == '\\' ) {
				if ( esc ) {
					out--;
					esc = FALSE;
				} else
					esc = TRUE;
			} else if ( *line == delim ) {
				if ( esc ) {
					out--;
					esc = FALSE;
				} else
					break;
			} else if ( *line == EOS ) {
				line--;
				break;
			} else
				esc = FALSE;
			*out++ = *line++;
		}
		*out = EOS;
		return (++line);
	} else {
		while ( !isspace(*line) && *line != EOS )
			line++;
		if ( *line != EOS )
			*line++ = EOS;
		return line;
	}
}
