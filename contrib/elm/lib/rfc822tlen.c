
static char rcsid[] = "@(#)Id: rfc822tlen.c,v 5.2 1993/07/20 03:15:15 syd Exp ";

/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.1 $   $State: Exp $
 *
 *			Copyright (c) 1993 USENET Community Trust
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein, Elm Coordinator
 *	elm@DSI.COM			dsinc!elm
 *
 *******************************************************************************
 * $Log: rfc822tlen.c,v $
 * Revision 1.1  1994/01/14  01:35:02  cp
 * 2.4.23
 *
 * Revision 5.2  1993/07/20  03:15:15  syd
 * remove extra garbage line
 *
 * Revision 5.1  1993/06/10  03:02:20  syd
 * Initial Checkin
 *
 *
 ******************************************************************************/

#include <stdio.h>

/*
 * rfc822_toklen(str) - Returns length of RFC-822 token that starts at "str".
 *
 * We understand the following tokens:
 *
 *	linear-white-space
 *	specials
 *	"quoted string"
 *	[domain.literal]
 *	(comment)
 *	CTL  (control chars)
 *	atom
 */

#define charlen(s)	((s)[0] == '\\' && (s)[1] != '\0' ? 2 : 1)

#define IS822_SPECIAL(c) ( \
	((c) == '(') || ((c) == ')') || ((c) == '<') || ((c) == '>') \
	|| ((c) == '@') || ((c) == ',') || ((c) == ';') || ((c) == ':') \
	|| ((c) == '\\') || ((c) == '"') || ((c) == '.') || ((c) == '[') \
	|| ((c) == ']') \
)

/*
 * RFC-822 defines SPACE to be just < > and HTAB, but after LWSP folding
 * CR and NL should be equivalent.
 */
#define IS822_SPACE(c) ((c) == ' ' || (c) == '\t' || (c) == '\r' || (c) == '\n')

/*
 * We've thrown non-ASCII (value > 0177) into this.
 */
#define IS822_CTL(c)	((c) <= 037 || (c) >= 0177)

#define IS822_ATOMCH(c)	(!IS822_SPECIAL(c) && !IS822_SPACE(c) && !IS822_CTL(c))


int rfc822_toklen(str)
register char *str;
{
	char *str0;
	int depth;
	register int chlen;

	str0 = str;

	if (*str == '"') {			/* quoted-string */
		++str;
		while (*str != '\0' && *str != '"')
			str += charlen(str);
		if (*str != '\0')
			++str;
		return (str-str0);
	}

	if (*str == '(' ) {			/* comment */
		++str;
		depth = 0;
		while (*str != '\0' && (*str != ')' || depth > 0)) {
			switch (*str) {
			case '(':
				++str;
				++depth;
				break;
			case ')':
				++str;
				--depth;
				break;
			default:
				str += charlen(str);
				break;
			}
		}
		if (*str != '\0')
			++str;
		return (str-str0);
	}


	if (*str == '[') {			/* domain-literal */
		++str;
		while (*str != '\0' && *str != ']')
			str += charlen(str);
		if (*str != '\0')
			++str;
		return (str-str0);
	}

	if (IS822_SPACE(*str)) {		/* linear-white-space */
		while (++str, IS822_SPACE(*str))
			;
		return (str-str0);
	}

	if (IS822_SPECIAL(*str) || IS822_CTL(*str))
		return charlen(str);		/* specials and CTL */

	/*
	 * Treat as an "atom".
	 */
	while (IS822_ATOMCH(*str))
		++str;
	return (str-str0);
}


#ifdef _TEST
main()
{
	char buf[1024], *bp;
	int len;
	for (;;) {
		fputs("\nstr> ", stdout);
		fflush(stdout);
		if (gets(buf) == NULL) {
			putchar('\n');
			break;
		}
		bp = buf;
		while (*bp != '\0') {
			len = rfc822_toklen(bp);
			printf("len %4d  |%.*s|\n", len, len, bp);
			bp += len;
		}
	}
	exit(0);
}
#endif

