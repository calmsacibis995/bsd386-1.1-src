/* -*-C-*- gblprocs.h */
/*-->gblprocs*/
/**********************************************************************/
/****************************** gblprocs ******************************/
/**********************************************************************/

/**********************************************************************/
/*************************  Global Procedures  ************************/
/**********************************************************************/

#if    ANSI_PROTOTYPES
void	abortrun(int);
float	actfact(UNSIGN32);
void	alldone(void);

#if    ANSI_LIBRARY
double	atof(const char *);
int	atoi(const char *);
#else
double	atof(char *);
int	atoi(char *);
#endif /* ANSI_LIBRARY */

#if    (BBNBITGRAPH | CANON_A2 | HPJETPLUS | IMPRESS | POSTSCRIPT)
void	bopact();
#endif /* (BBNBITGRAPH | CANON_A2 | HPJETPLUS | IMPRESS | POSTSCRIPT) */

int	chargf(BYTE,void(*)());
int	charpk(BYTE,void(*)());
int	charpxl(BYTE,void(*)());

#if    (BBNBITGRAPH | HPJETPLUS | POSTSCRIPT | IMPRESS | CANON_A2)
#else
void	clrbmap(void);
#endif /* (BBNBITGRAPH | HPJETPLUS | POSTSCRIPT | IMPRESS | CANON_A2) */

void	clrrow(void);

#if    POSTSCRIPT
void	cppsfile(void);
#endif /* POSTSCRIPT */

#if    ANSI_LIBRARY
char*   ctime(const time_t *);
#else
char*	ctime(long *);
#endif /* ANSI_LIBRARY */

char*	cuserid(char *);
void	dbgopen(FILE*, char*, char*);
void	devinit(int, char *[]);
void	devterm(void);

#if    (BBNBITGRAPH | HPJETPLUS | POSTSCRIPT | IMPRESS | CANON_A2)
#else
void	dispchar(BYTE);
#endif /* (BBNBITGRAPH | HPJETPLUS | POSTSCRIPT | IMPRESS | CANON_A2) */

void	dvifile(int, char *[], char *);
void	dviinit(char *);
void	dviterm(void);

#if    POSTSCRIPT
void	emitchar(BYTE);
#endif /* POSTSCRIPT */

#if    (BBNBITGRAPH | CANON_A2 | HPJETPLUS | POSTSCRIPT)
void	eopact();
#endif /* (BBNBITGRAPH | CANON_A2 | HPJETPLUS | POSTSCRIPT) */

void	EXIT(int);
void	fatal(char *);
void	fillrect(COORDINATE, COORDINATE, COORDINATE, COORDINATE);
void	findpost(void);
COORDINATE	fixpos(COORDINATE, INT32, float);
void	fontfile(char *[MAXFORMATS],char *,char *,int);
BOOLEAN	fontsub(char *,int *,char *,int);

#if    ANSI_LIBRARY
void	free(void *);
#else
void	free(char *);
#endif /* ANSI_LIBRARY */

int	FSEEK(FILE *,long,int);
void	getbmap(void);
void	getbytes(FILE *, char *, BYTE);

#if    ANSI_LIBRARY
char*	GETENV(const char *);
#else
char*	GETENV(char *);
#endif /* ANSI_LIBRARY */

void	getfntdf(void);

#if    OS_VAXVMS
char*	getjpi(int);
#endif

char*	getlogin(void);
void	getpgtab(long);
void	initglob(void);
float	inch(char *);

#if    BBNBITGRAPH
void	initterm();
#endif /* BBNBITGRAPH */

#if    (CANON_A2 | HPJETPLUS)
void	loadbmap(BYTE);
#endif /* (CANON_A2 | HPJETPLUS) */

void	loadchar(BYTE);

#if    BBNBITGRAPH
void	loadrast(FILE *, COORDINATE, COORDINATE);
#endif /* BBNBITGRAPH */

int	main(int ,char *[]);

#if    ANSI_LIBRARY
void*	malloc(size_t);
#else
char*	malloc(unsigned);
#endif /* ANSI_LIBRARY */

#if    (APPLEIMAGEWRITER | OKIDATA2410)
char	makechar(UNSIGN32 *[],UNSIGN32);
#endif /* (APPLEIMAGEWRITER | OKIDATA2410) */

#if    HPJETPLUS
void	makefont(void);
#endif

void	movedown(INT32);
void	moveover(INT32);
void	moveto(COORDINATE, COORDINATE);

#if    (BBNBITGRAPH | HPJETPLUS | POSTSCRIPT | IMPRESS | CANON_A2)
void	newfont(void);
#endif

UNSIGN32	nosignex(FILE *,BYTE);
void	openfont(char *);
void	option(char *);

#if    HPLASERJET
void	outline(UNSIGN32 *);
#else
void	outline(char *);
#endif /* HPLASERJET */

#if    EPSON
#if    HIRES
void	outpaperfeed(INT16);
#endif /* HIRES */
#endif /* EPSON */

#if    HPJETPLUS
void	outraster(BYTE,UNSIGN16);
#endif /* HPJETPLUS */

void	outrow(BYTE,UNSIGN16);

#if    (BBNBITGRAPH | HPJETPLUS | POSTSCRIPT | IMPRESS | CANON_A2)
#else
void	prtbmap(void);
#endif /* (BBNBITGRAPH | HPJETPLUS | POSTSCRIPT | IMPRESS | CANON_A2) */

void	prtpage(long);

#if    POSTSCRIPT
char*	putfontname(struct font_entry *);
void	putname(FILE *,struct font_entry *);
#endif /* POSTSCRIPT */

#if    BBNBITGRAPH
void	putout(INT16);
#endif /* BBNBITGRAPH */

void	readfont(INT32);
int	readgf(void);
int	readpk(void);
void	readpost(void);
int	readpxl(void);
void	reldfont(struct font_entry *);

#if    BBNBITGRAPH
void	rsetterm();
#endif /* BBNBITGRAPH */

COORDINATE	rulepxl(UNSIGN32,float);

#if    HPJETPLUS
void	saverow(BYTE,UNSIGN16);
#endif /* HPJETPLUS */

void	setchar(BYTE,BOOLEAN);

#if    HPJETPLUS
void	setfont(void);
#endif

#if    (HPJETPLUS | POSTSCRIPT | IMPRESS | CANON_A2)
void	setstr(BYTE);
#endif /* (HPJETPLUS | POSTSCRIPT | IMPRESS | CANON_A2) */

void	setfntnm(INT32);
void	setrule(UNSIGN32, UNSIGN32, BOOLEAN);
INT32	signex(FILE *,BYTE);
void	skipfont(INT32);
void	skgfspec(void);
void	skpkspec(void);
void	special(char *);

#if    ANSI_LIBRARY
char*	strcat(char *,const char *);
char*	strchr(const char *,int);
char*	strcpy(char *,const char *);
size_t	strlen(const char *);
int	strncmp(const char *,const char *,size_t);
char*	strncpy(char *,const char *,size_t);
char*	strrchr(const char *,int);
#else /* NOT ANSI_LIBRARY conformant */
char*	strcat(char *,char *);
char*	strchr(char *,char);
char*	strcpy(char *,char *);
int	strlen(char *);
int	strncmp(char *,char *,int);
char*	strncpy(char *,char *,int);
char*	strrchr(char *,char);
#endif /* ANSI_LIBRARY */

int	strcm2(char *,char *);
int	strid2(char[],char[]);

char*	tctos(void);

#if    POSTSCRIPT
void	textchr(char);
void	textflush();
void	textnum(long);
void	textstr(char *);
#endif /* POSTSCRIPT */

#if    ANSI_LIBRARY
time_t	time(time_t *);
#else
long	time(long *);
#endif /* ANSI_LIBRARY */

#if    BBNBITGRAPH
void	unloadfonts();
#endif

void	usage(FILE *);

#if    VIRTUAL_FONTS
void	virtfree(FILE *);
#endif

void	warning(char *);

#if    FASTZERO
void	zerom(UNSIGN32 *,UNSIGN32);
#endif /* FASTZERO */

#else /* NOT ANSI_PROTOTYPES */
double	atof();
int	atoi();

#if    (BBNBITGRAPH | CANON_A2 | HPJETPLUS | IMPRESS | POSTSCRIPT)
void	bopact();
#endif /* (BBNBITGRAPH | CANON_A2 | HPJETPLUS | IMPRESS | POSTSCRIPT) */

char*	cuserid();
char*	ctime();
void	EXIT();
void	free();
int	FSEEK();
long	FTELL();
char*	GETENV();

#if    OS_VAXVMS
char*	getjpi();
#endif

char*	getlogin();
#ifndef _IBMR2
char*	malloc();
#endif /* _IBMR2 */

#if    (IBM_PC_WIZARD | KCC_20 | OS_VAXVMS | _IBMR2 | BSD44)
/* stdio.h declares sprintf(); */
#else
char*	sprintf();		/* Berkeley 4.1 BSD style */
#endif /* (IBM_PC_WIZARD | KCC_20 | OS_VAXVMS | _IBMR2) */

char*	strcpy();
char*	strcat();
char*	strchr();		/* private version of this 4.2BSD function */
int	strcm2();		/* local addition (used by inch()) */
int	strcmp();
int	strid2();		/* local addition (used by initglob()) */
int	strncmp();
char*	strncpy();
char*	strrchr();		/* private version of this 4.2BSD function */
long	time();

/***********************************************************************
Note: Global procedures  are declared here  in alphabetical order,  with
those which do not  return values typed "void".   Their bodies occur  in
alphabetical order following the main()  procedure, usually in the  form
of "#include" statements.   The names  are kept  unique in  the first  6
characters for portability.
***********************************************************************/

void	abortrun();
float	actfact();
void	alldone();
FILE*	FOPEN();

int	chargf();
int	charpk();
int	charpxl();
void	clrrow();

#if    POSTSCRIPT
void	cppsfile();
#endif /* POSTSCRIPT */

void	dbgopen();
void	devinit();
void	devterm();

#if    (BBNBITGRAPH | HPJETPLUS | POSTSCRIPT | IMPRESS | CANON_A2)
#else /* NOT (BBNBITGRAPH | HPJETPLUS | POSTSCRIPT | IMPRESS | CANON_A2) */
void	clrbmap();
void	dispchar();
#endif /* (BBNBITGRAPH | HPJETPLUS | POSTSCRIPT | IMPRESS | CANON_A2) */

void	dvifile();
void	dviinit();
void	dviterm();

#if    POSTSCRIPT
void	emitchar();
#endif /* POSTSCRIPT */

#if    (BBNBITGRAPH | CANON_A2 | HPJETPLUS | POSTSCRIPT)
void	eopact();
#endif /* (BBNBITGRAPH | CANON_A2 | HPJETPLUS | POSTSCRIPT) */

void	fatal();
void	fillrect();
void	findpost();
void	fontfile();
BOOLEAN	fontsub();
COORDINATE	fixpos();
void	getbmap();
void	getbytes();
void	getfntdf();
void	getpgtab();

#if    BBNBITGRAPH
void	gotint();
#endif /* BBNBITGRAPH */

float	inch();
void	initglob();

#if    BBNBITGRAPH
void	initterm();
#endif /* BBNBITGRAPH */

#if    (CANON_A2 | HPJETPLUS)
void	loadbmap();
#endif /* (CANON_A2 | HPJETPLUS) */

void	loadchar();

#if    BBNBITGRAPH
void	loadrast();
#endif /* BBNBITGRAPH */

int	main();

#if    APPLEIMAGEWRITER
char	makechar();
#endif /* APPLEIMAGEWRITER */

#if    OKIDATA2410
char	makechar();
#endif /* OKIDATA2410 */

#if    HPJETPLUS
void	makefont();
#endif

void	movedown();
void	moveover();
void	moveto();

#if    (BBNBITGRAPH | HPJETPLUS | POSTSCRIPT | IMPRESS | CANON_A2)
void	newfont();
#endif

UNSIGN32	nosignex();

#if    BBNBITGRAPH
#else /* NOT BBNBITGRAPH */
void	outline();
#endif /* BBNBITGRAPH */

#if    EPSON
#if    HIRES
void	outpaperfeed();
#endif /* HIRES */
#endif /* EPSON */

#if    HPJETPLUS
void	outraster();
#endif /* HPJETPLUS */

void	outrow();
void	openfont();
void	option();

#if    (BBNBITGRAPH | HPJETPLUS | POSTSCRIPT | IMPRESS | CANON_A2)
#else /* NOT (BBNBITGRAPH | HPJETPLUS | POSTSCRIPT | IMPRESS | CANON_A2) */
void	prtbmap();
#endif /* (BBNBITGRAPH | HPJETPLUS | POSTSCRIPT | IMPRESS | CANON_A2) */

void	prtpage();

#if    POSTSCRIPT
char*	putfontname();
void	putname();
#endif /* POSTSCRIPT */

#if    BBNBITGRAPH
void	putout();
#endif /* BBNBITGRAPH */

void	readfont();
int	readgf();
int	readpk();
void	readpost();
int	readpxl();
void	reldfont();

#if    BBNBITGRAPH
void	rsetterm();
#endif /* BBNBITGRAPH */

COORDINATE	rulepxl();

#if    HPJETPLUS
void	saverow();
#endif /* HPJETPLUS */

void	setchar();
void	setfntnm();

#if    HPJETPLUS
void	setfont();
#endif

void	setrule();

#if    (HPJETPLUS | POSTSCRIPT | IMPRESS | CANON_A2)
void	setstr();
#endif /* (HPJETPLUS | POSTSCRIPT | IMPRESS | CANON_A2) */

INT32	signex();
void	skipfont();
void	skgfspec();
void	skpkspec();
void	special();
char*	tctos();

#if    POSTSCRIPT
void	textchr();
void	textflush();
void	textnum();
void	textstr();
#endif /* POSTSCRIPT */

#if    BBNBITGRAPH
void	unloadfonts();
#endif

void	usage();

#if    VIRTUAL_FONTS
void	virtfree();
#endif

void	warning();

#if    FASTZERO
void	zerom();
#endif /* FASTZERO */

#endif /* ANSI_PROTOTYPES */
