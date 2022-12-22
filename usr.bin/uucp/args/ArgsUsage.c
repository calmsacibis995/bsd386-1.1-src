/*
**	Copyright 1992 UUNET Technologies Inc.
**
**	All rights reserved.
**
**	Use of this software is subject to a licence agreement.
*/

/*
**	Return a usage string from an "Args" array.
**
**	RCSID $Id: ArgsUsage.c,v 1.1.1.1 1992/09/28 20:08:31 trent Exp $
**
**	$Log: ArgsUsage.c,v $
 * Revision 1.1.1.1  1992/09/28  20:08:31  trent
 * Latest UUCP from ziegast@uunet
 *
 * Revision 1.1  1992/04/14  21:29:38  piers
 * Initial revision
 *
*/

#define	ARGS

#include	"global.h"


#define	MAXLEN	79	/* Line length */

char *		AVersion;
bool		ExplainArgs;
bool		ExplVersion;



char *
ArgsUsage(usage)
	Args *		usage;
{
	register Args *	up;
	register char *	s;
	register char *	cp;
	register int	len;
	register int	t;
	char *		exp;
	char *		result;
	static char	versnstr[]	= english("Version: ");
	static char	usagestr[]	= english("Usage: \"");
	static char	linebrk[]	= "\\\n\t";
#	define		linebrklen		3
	static char	optspace[]	= english("\n\n\tWhere indicated, a space between flag and argument is optional.");

#	if	DEBUG
	static char	badtype[] = "{BAD TYPE (%d) SPECIFIED FOR ARGUMENT '%c'}";
	static char	badbool[] = "{CAN'T SPECIFY ``OPTVAL'' or no ``OPTARG'' FOR BOOLEAN '%c'}";
	static char	badposn[] = "{CAN'T SPECIFY ``MANY'' FOR POSITIONAL ARGUMENT %d}";
#	endif	/* DEBUG */

	Trace((1, "ArgsUsage"));

	if ( ExplVersion )
	{
		if ( AVersion == NULLSTR )
			AVersion = "???";

		len = strlen(versnstr) + strlen(AVersion) + strlen(Version) + 2;
		result = s = Malloc(len);
		s = strcpyend(s, versnstr);
		s = strcpyend(s, AVersion);
		*s++ = '\n';
		s = strcpyend(s, Version);
		*s++ = '\0';
		return result;
	}

#	ifdef	EXPLAINDIR
	if ( ExplainArgs && Name != NULLSTR && Name[0] != '\0' )
	{
		exp = ReadFile(s = concat(EXPLAINDIR, Name, NULLSTR));
		free(s);
	}
	else
#	endif	/* EXPLAINDIR */
	{
		exp = NULLSTR;
		RdFileSize = 0;
	}

	RdFileSize += strlen(optspace);

	for ( len = 0, up = usage ; up->type ; up++ )
	{
		register int	x;

		if ( up->opt & SPECIAL )
			continue;
		x = 1;
		if ( up->posn == 0 )
		{
			x += ((Name==NULLSTR)?0:strlen(Name)) + strlen(usagestr);
			up->len = x;
			len += x;
			continue;
		}
#		if	DEBUG
		if ( (t = up->type) == 0 || t > MAXTYPE )
			x += sizeof badtype + 8;
		if ( t == BOOL && (up->opt & (OPTVAL|OPTARG)) != OPTARG )
			x += sizeof badbool;
		if ( up->posn != OPTPOS && (up->opt & MANY) )
			x += sizeof badposn + 8;
#		endif	/* DEBUG */
		if ( up->flagchar > SPECIALFLAGS )
			x += 2;
		else
		if ( up->flagchar == MINFLAG )
			x += 1;
		if ( (cp = up->descr) == NULLSTR )
			cp = ArgTypes[up->type];
		if ( cp != NULLSTR && *cp != '\0' )
			x += strlen(cp) + 2;
		if ( (t = up->opt) & OPTARG )
			x += 2;
		if ( t & OPTVAL )
			x += 2;
		else
		if ( up->flagchar > SPECIALFLAGS && up->type != BOOL )
			x += 1;
		if ( t & MANY )
			x += 4;
		up->len = x;
		len += x;
		Trace((3, "ArgsUsage \"%s\" len %d", cp, x));
	}

	len += ((len / ((MAXLEN-8)/2)) * linebrklen) + 2;	/* Allow for line breaks */
	Trace((2, "ArgsUsage: totlen=%d", len));
	result = s = Malloc((int)RdFileSize + len);

	for ( len = 0, up = usage ; up->type ; up++ )
	{
		char *	pos;

		if ( up->opt & SPECIAL )
			continue;	/* Ignore catchalls */

		if ( (len + up->len) >= MAXLEN )
		{
			s = strcpyend(s, linebrk);
			len = 8;
		}

		if ( up->posn == 0 )
		{
			s = strcpyend(s, usagestr);
			s = strcpyend(s, Name);
			len += up->len;
			continue;
		}

		pos = s;

		*s++ = ' ';

#		if	DEBUG
		if ( (t = up->type) == 0 || t > MAXTYPE )
		{
			(void)sprintf(s, badtype, t, up->flagchar);
			s += strlen(s);
			len += s-pos;
			continue;
		}
#		endif	/* DEBUG */

		if ( up->opt & OPTARG )
			*s++ = '[';

#		if	DEBUG
		if ( up->posn != OPTPOS && (up->opt & MANY) )
		{
			(void)sprintf(s, badposn, up->posn);
			s += strlen(s);
		}
#		endif	/* DEBUG */

		if ( up->flagchar != NONFLAG && up->flagchar != NULLFLAG )
		{
			*s++ = '-';
			if ( up->flagchar != MINFLAG )
			{
				register Args *	np = up + 1;

				if
				(
					up->type == BOOL
					&&
					np->type == BOOL
					&&
					np->flagchar > SPECIALFLAGS
				)
				{
loop:					*s++ = '[';
					t = 1;
				}
				else
					t = 0;

				*s++ = up->flagchar;
		
#				if	DEBUG
				if ( up->type == BOOL && (up->opt & (OPTVAL|OPTARG)) != OPTARG )
				{
					(void)sprintf(s, badbool, up->flagchar);
					s += strlen(s);
				}
#				endif	/* DEBUG */

				if ( t )
				{
					*s++ = ']';
					if
					(
						np->type == BOOL
						&&
						np->flagchar > SPECIALFLAGS
					)
					{
						up = np;
						np = up + 1;
						len += s-pos;
						if ( (len + up->len) >= MAXLEN )
						{
							s = strcpyend(s, linebrk);
							len = 8;
						}
						pos = s;
						goto loop;
					}
				}
			}
		}

		if ( (t = up->opt) & OPTVAL )
			*s++ = '[';
		else
		if ( up->flagchar > SPECIALFLAGS && up->type != BOOL )
			*s++ = ' ';

		if ( (cp = up->descr) == NULLSTR )
			cp = ArgTypes[up->type];

		if ( cp != NULLSTR && strlen(cp) > 0 )
		{
			*s++ = '<';
			s = strcpyend(s, cp);
			*s++ = '>';
		}

		if ( t & OPTVAL )
			*s++ = ']';

		if ( t & MANY )
			s = strcpyend(s, " ...");

		if ( t & OPTARG )
			*s++ = ']';

		len += s-pos;
	}

	*s++ = '"';

	if ( exp != NULLSTR )
	{
		*s++ = '\n';
		s = strcpyend(s, exp);
		free(exp);
	}

	if ( ExplainArgs )
		(void)strcpy(s, optspace);
	else
		*s = '\0';

	return result;
}
