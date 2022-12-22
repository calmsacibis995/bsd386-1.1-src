
static char rcsid[] = "@(#)Id: encode.c,v 5.5 1993/05/08 20:25:33 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.2 $   $State: Exp $
 *
 *			Copyright (c) 1988-1992 USENET Community Trust
 *			Copyright (c) 1986,1987 Dave Taylor
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein, Elm Coordinator
 *	elm@DSI.COM			dsinc!elm
 *
 *******************************************************************************
 * $Log: encode.c,v $
 * Revision 1.2  1994/01/14  00:54:48  cp
 * 2.4.23
 *
 * Revision 5.5  1993/05/08  20:25:33  syd
 * Add sleepmsg to control transient message delays
 * From: Syd
 *
 * Revision 5.4  1993/04/12  01:26:06  syd
 * According to the SVID (version 3) the function crypt is
 *
 *     char *     crypt( const char *, const char * )
 *
 * However, it is declared as
 *
 *     unsigned char crypt();
 *
 * on line 179 of src/encode.c.  The "unsigned" keyword causes the compile
 * to fail on SVID3 compliant systems.  Upon inspection, it appeared that
 * the declaration was not even required if CRYPT was not defined, so
 * changed it to be conditionally compiled.
 * From: Larry Philps <larryp@sco.COM>
 *
 * Revision 5.3  1993/02/03  17:12:53  syd
 * move more declarations to defs.h, including sleep
 * From: Syd
 *
 * Revision 5.2  1992/12/07  02:34:56  syd
 * Traditional C used 'unsigned preserving' rules when an integral data
 * value is widened to integer and ANSI C changed the rules to 'value
 * preserving'. This is one of the few things that the ANSI X3J11 comitte
 * did that might break existing programs.  Casting to (int)
 * From: Bo.Asbjorn.Muldbak <bam@jutland.ColumbiaSC.NCR.COM>
 *
 * Revision 5.1  1992/10/03  22:58:40  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

/** This is a heavily mangled version of the 'cypher' program written by
    person or persons unknown.  

**/

#include "headers.h"
#include "s_elm.h"

#define RTRSZ	94
#define RN	4
#define RMASK	0x7fff	/* use only 15 bits */

/*
 *  NOTICE: Some systems take a char as a signed value,
 *	    a byte wide int. For encryption to work you
 *	    absolutely need an unsigned char !!
 *	    According to K&R2 and ANSI it is always
 *	    permissible to specify unsigned with char.
 *	    (ukkonen@csc.fi)
 */

static unsigned char	r[RTRSZ][RN];	/* rotors */
static unsigned char	ir[RTRSZ][RN];	/* inverse rotors */
static unsigned char	h[RTRSZ];		/* half rotor */
static unsigned char	s[RTRSZ];		/* shuffle vector */
static int		p[RN];		/* rotor indices */

static unsigned char the_key[SLEN];	/* unencrypted key */
static unsigned char *encrypted_key;	/* encrypted key   */

static char *decrypt_prompt = NULL;
static char *first_enc_prompt = NULL;
static char *second_enc_prompt = NULL;
#define PROMPT_LINE		LINES-1

getkey(send)
int send;
{
	/** this routine prompts for and returns an encode/decode
	    key for use in the rest of the program. **/

	char buffer[2][NLEN];

	if (decrypt_prompt == NULL) {
		decrypt_prompt = catgets(elm_msg_cat, ElmSet, ElmDecryptPrompt,
			"Enter decryption key: ");
		first_enc_prompt = catgets(elm_msg_cat, ElmSet, ElmFirstEncryptPrompt,
			"Enter encryption key: ");
		second_enc_prompt = catgets(elm_msg_cat, ElmSet, ElmSecondEncryptPrompt,
			"Please enter it again: ");
	}

	while (1) {
	  PutLine0(PROMPT_LINE, 0, (send ? first_enc_prompt : decrypt_prompt));
	  CleartoEOLN();
	  optionally_enter(buffer[0], PROMPT_LINE,
	    strlen(send ? first_enc_prompt : decrypt_prompt), FALSE, TRUE);
	  if (send) {
	    PutLine0(PROMPT_LINE, 0, second_enc_prompt);
	    CleartoEOLN();
	    optionally_enter(buffer[1], PROMPT_LINE, strlen(second_enc_prompt),
	      FALSE, TRUE);
	    if(strcmp(buffer[0], buffer[1]) != 0) {
	      error(catgets(elm_msg_cat, ElmSet, ElmKeysNotSame,
		"Your keys were not the same!"));
	      if (sleepmsg > 0)
		    sleep(sleepmsg);
	      clear_error();
	      continue;
	    }
	  }
	  break;
	}
        strcpy((char *) the_key, buffer[0]);	/* save unencrypted key */
	makekey(buffer[0]);

	setup();		/** initialize the rotors etc. **/

	ClearLine(PROMPT_LINE);		
	clear_error();
}

get_key_no_prompt()
{
	/** This performs the same action as get_key, but assumes that
	    the current value of 'the_key' is acceptable.  This is used
	    when a message is encrypted twice... **/

	char buffer[SLEN];

	strcpy(buffer, (char *) the_key);

	makekey( buffer );

	setup();
}

encode(line)
char *line;
{
	/** encrypt or decrypt the specified line.  Uses the previously
	    entered key... **/

	register int i, j, ph = 0;

	for (; *line; line++) {
	  i = (int) *line;

	  if ( (i >= ' ') && (i < '~') ) {
	    i -= ' ';

	    for ( j = 0; j < RN; j++ )		/* rotor forwards */
	      i = r[(i+p[j])%RTRSZ][j];

	    i = (((int)h[(i+ph)%RTRSZ])-ph+RTRSZ)%RTRSZ;	/* half rotor */

	    for ( j--  ; j >= 0; j-- )		/* rotor backwards */
	      i = ((int)ir[i][j]+RTRSZ-p[j])%RTRSZ;

	    j = 0;				/* rotate rotors */
	    p[0]++;
	    while ( p[j] == RTRSZ ) {
	      p[j] = 0;
	      j++;
	      if ( j == RN ) break;
	      p[j]++;
            }
  
	    if ( ++ph == RTRSZ )
	      ph = 0;

	    i += ' ';
	  }
	  
	  *line = (char) i;	/* replace with altered one */
	}
}


makekey( rkey)
char *rkey;
{
	/** encrypt the key using the system routine 'crypt' **/

	static char key[9];
	char salt[2];
#ifdef CRYPT
	char *crypt();
#endif /* CRYPT */

	strncpy( key, rkey, 8);
	key[8] = '\0';
	salt[0] = key[0];
	salt[1] = key[1];
#ifdef CRYPT
	encrypted_key = (unsigned char *) crypt( key, salt);
#else
	encrypted_key = (unsigned char *) key;
#endif
}

/*
 * shuffle rotors.
 * shuffle each of the rotors indiscriminately.  shuffle the half-rotor
 * using a special obvious and not very tricky algorithm which is not as
 * sophisticated as the one in crypt(1) and Oh God, I'm so depressed.
 * After all this is done build the inverses of the rotors.
 */

setup()
{
	register long i, j, k, temp;
	long seed;

	for ( j = 0; j < RN; j++ ) {
		p[j] = 0;
		for ( i = 0; i < RTRSZ; i++ )
			r[i][j] = i;
	}

	seed = 123;
	for ( i = 0; i < 13; i++)		/* now personalize the seed */
	  seed = (seed*encrypted_key[i] + i) & RMASK;

	for ( i = 0; i < RTRSZ; i++ )		/* initialize shuffle vector */
	  h[i] = s[i] = i;

	for ( i = 0; i < RTRSZ; i++) {		/* shuffle the vector */
	  seed = (5 * seed + encrypted_key[i%13]) & RMASK;;
	  k = ((seed % 65521) & RMASK) % RTRSZ;
	  temp = s[k];
	  s[k] = s[i];
	  s[i] = temp;
	}

	for ( i = 0; i < RTRSZ; i += 2 ) {	/* scramble the half-rotor */
	  temp = h[s[i]];			/* swap rotor elements ONCE */
	  h[s[i]] = h[s[i+1]];
	  h[s[i+1]] = temp;
	}

	for ( j = 0; j < RN; j++) {			/* select a rotor */

	  for ( i = 0; i < RTRSZ; i++) {		/* shuffle the vector */
	    seed = (5 * seed + encrypted_key[i%13]) & RMASK;;
	    k = ((seed % 65521) & RMASK) % RTRSZ;
	    temp = r[i][j];
	    r[i][j] = r[k][j];
	    r[k][j] = temp;
	  }

	  for ( i = 0; i < RTRSZ; i++) 		/* create inverse rotors */
	    ir[r[i][j]][j] = i;
       }
}
