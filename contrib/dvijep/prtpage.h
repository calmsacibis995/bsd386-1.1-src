/* -*-C-*- prtpage.h */
/*-->prtpage*/
/**********************************************************************/
/****************************** prtpage *******************************/
/**********************************************************************/

void
prtpage(bytepos)		/* print page whose BOP is at bytepos */
long bytepos;

{
    struct stack_entry
    {
	INT32 h;
	COORDINATE hh;
	INT32 v;
	COORDINATE vv;
	INT32 w, x, y, z;	/* what's on stack */
    };
    register BYTE command;  /* current command				*/
    register INT16 i;	    /* command parameter; loop index		*/
    char tc;		    /* temporary character			*/
    UNSIGN32 ht_rule;	    /* rule height                              */
    UNSIGN32 wd_rule;	    /* rule width                               */
    INT32 k,m;		    /* temporary parameter			*/
    BOOLEAN seen_bop;	    /* flag for noting processing of BOP	*/
    register INT16 sp;	    /* stack pointer				*/
    struct stack_entry stack[STACKSIZE];    /* stack			*/
    char specstr[MAXSPECIAL+1];		/* \special{} string		*/
    INT32 w;		    /* current horizontal spacing		*/
    INT32 x;		    /* current horizontal spacing		*/
    INT32 y;		    /* current vertical spacing			*/
    INT32 z;		    /* current vertical spacing			*/

/***********************************************************************
Process all commands  between the  BOP at bytepos  and the  next BOP  or
POST.  The page is  printed when the  EOP is met,  but font changes  can
also happen between EOP and BOP, so we have to keep going after EOP.
***********************************************************************/

    seen_bop = FALSE;			/* this is first time through */
    (void) FSEEK(dvifp,bytepos,0);	/* start at the desired position */

    for (;;)	/* "infinite" loop - exits when POST or second BOP met */
    {
#if    BBNBITGRAPH
	/* If the test here on every byte proves to carry objectionable
	   overhead, it can be moved into the default, PUTx, and SETx
	   command sections. The goal is to give the user instant
	   response in page screen positioning, without having to wait
	   for the entire screen to be painted when it is just going
	   to be moved anyway.  This is the sort of display algorithm
	   that EMACS uses. */
	if (kbinput() > 0)	/* user has typed something */
	    command = EOP;	/* so set for end-of-page action */
	else
#endif
	command = (BYTE)nosignex(dvifp,(BYTE)1);
	switch (command)
	{

	case SET1:
	case SET2:
	case SET3:
	case SET4:
	    (void)setchar((BYTE)nosignex(dvifp,(BYTE)(command-SET1+1)),TRUE);
	    break;

	case SET_RULE:
	    ht_rule = nosignex(dvifp,(BYTE)4);
	    wd_rule = nosignex(dvifp,(BYTE)4);
	    (void)setrule(ht_rule,wd_rule,TRUE);
	    break;

	case PUT1:
	case PUT2:
	case PUT3:
	case PUT4:
	    (void)setchar((BYTE)nosignex(dvifp,(BYTE)(command-PUT1+1)),FALSE);
	    break;

	case PUT_RULE:
	    ht_rule = nosignex(dvifp,(BYTE)4);
	    wd_rule = nosignex(dvifp,(BYTE)4);
	    (void)setrule(ht_rule,wd_rule,FALSE);
	    break;

	case NOP:
	    break;

	case BOP:
	    if (seen_bop)
		return;			/* we have been here already */
	    seen_bop = TRUE;

	    for (i=0; i<=9; i++)
		tex_counter[i] = (INT32)signex(dvifp,(BYTE)4);

#if    BBNBITGRAPH
	    (void)bopact();		/* display menu at top of page */
#else /* not BBNBITGRAPH */

#if    (CANON_A2 | HPJETPLUS | IMPRESS | POSTSCRIPT)
	    (void)bopact();
#else /* NOT (CANON_A2 | HPJETPLUS | IMPRESS | POSTSCRIPT) */
	    (void)clrbmap();
#endif /* (CANON_A2 | HPJETPLUS | IMPRESS | POSTSCRIPT) */

	    if (!quiet)
	        (void)fprintf(stderr,"{%s}",tctos()); /* TeX page counters */
	    (void) nosignex(dvifp,(BYTE)4);	/* skip prev. page ptr */
#endif /* BBNBITGRAPH */

	    h = v = w = x = y = z = 0;
	    hh = lmargin;
	    vv = tmargin;
	    sp = 0;
	    fontptr = (struct font_entry*)NULL;
	    break;

	case EOP:

#if    (BBNBITGRAPH | CANON_A2 | HPJETPLUS | IMPRESS | POSTSCRIPT)
	    (void)eopact();
#else
	    (void)prtbmap();
#endif /* (BBNBITGRAPH | CANON_A2 | HPJETPLUS | IMPRESS | POSTSCRIPT) */

	    return;

	case PUSH:
	    if (sp >= STACKSIZE)
		(void)fatal("prtpage():  stack overflow");
	    stack[sp].h = h;
	    stack[sp].hh = hh;
	    stack[sp].v = v;
	    stack[sp].vv = vv;
	    stack[sp].w = w;
	    stack[sp].x = x;
	    stack[sp].y = y;
	    stack[sp].z = z;
	    sp++;
	    break;

	case POP:
	    --sp;
	    if (sp < 0)
		(void)fatal("prtpage():  stack underflow");
	    h = stack[sp].h;
	    hh = stack[sp].hh;
	    v = stack[sp].v;
	    vv = stack[sp].vv;
	    w = stack[sp].w;
	    x = stack[sp].x;
	    y = stack[sp].y;
	    z = stack[sp].z;
	    break;

	case RIGHT1:
	case RIGHT2:
	case RIGHT3:
	case RIGHT4:
	    (void)moveover(signex(dvifp,(BYTE)(command-RIGHT1+1)));
	    break;

	case W0:
	    (void)moveover(w);
	    break;

	case W1:
	case W2:
	case W3:
	case W4:
	    w = (INT32)signex(dvifp,(BYTE)(command-W1+1));
	    (void)moveover(w);
	    break;

	case X0:
	    (void)moveover(x);
	    break;

	case X1:
	case X2:
	case X3:
	case X4:
	    x = (INT32)signex(dvifp,(BYTE)(command-X1+1));
	    (void)moveover(x);
	    break;

	case DOWN1:
	case DOWN2:
	case DOWN3:
	case DOWN4:
	    (void)movedown(signex(dvifp,(BYTE)(command-DOWN1+1)));
	    break;

	case Y0:
	    (void)movedown(y);
	    break;

	case Y1:
	case Y2:
	case Y3:
	case Y4:
	    y = signex(dvifp,(BYTE)(command-Y1+1));
	    (void)movedown(y);
	    break;

	case Z0:
	    (void)movedown(z);
	    break;

	case Z1:
	case Z2:
	case Z3:
	case Z4:
	    z = signex(dvifp,(BYTE)(command-Z1+1));
	    (void)movedown(z);
	    break;

	case FNT1:
	case FNT2:
	case FNT3:
	case FNT4:
	    (void)setfntnm((INT32)nosignex(dvifp,
		(BYTE)(command-FNT1+1)));
	    break;

	case XXX1:
	case XXX2:
	case XXX3:
	case XXX4:
	    k = (INT32)nosignex(dvifp,(BYTE)(command-XXX1+1));
	    if (k > MAXSPECIAL)
	    {
		(void)sprintf(message,
		    "prtpage():  \\special{} string of %d characters longer \
than DVI driver limit of %d -- truncated.",
		k,MAXSPECIAL);
		(void)warning(message);
	    }
	    m = 0;
	    while (k--)
	    {
		 tc = (char)nosignex(dvifp,(BYTE)1);
		 if (m < MAXSPECIAL)
		     specstr[m++] = tc;
	    }
	    specstr[m] = '\0';
 	    (void) special(specstr);
	    break;

	case FNT_DEF1:
	case FNT_DEF2:
	case FNT_DEF3:
	case FNT_DEF4:
	    if (preload)
		(void)skipfont ((INT32) nosignex(dvifp,
		    (BYTE)(command-FNT_DEF1+1)));
	    else
		(void)readfont ((INT32) nosignex(dvifp,
		    (BYTE)(command-FNT_DEF1+1)));
	    break;

	case PRE:
	    (void)fatal("prtpage():  PRE occurs within file");
	    break;

	case POST:
	    (void)devterm();		/* terminate device output */
	    (void)dviterm();		/* terminate DVI file processing */
	    (void)alldone();		/* this will never return */
	    break;

	case POST_POST:
	    (void)fatal("prtpage():  POST_POST with no preceding POST");
	    break;

	default:
	    if (command >= FONT_00 && command <= FONT_63)
		(void)setfntnm((INT32)(command - FONT_00));
	    else if (command >= SETC_000 && command <= SETC_127)

#if    (HPJETPLUS | POSTSCRIPT | IMPRESS | CANON_A2)
		(void)setstr((BYTE)command); /* this sets several chars */
#else
		(void)setchar((BYTE)(command-SETC_000), TRUE);
#endif /* (HPJETPLUS | POSTSCRIPT | IMPRESS | CANON_A2) */

	    else
	    {
		(void)sprintf(message,"prtpage():  %d is an undefined command",
		    command);
		(void)fatal(message);
	    }
	    break;
	}
    }
}
