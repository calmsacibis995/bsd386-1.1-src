# include <ctype.h>
# include "structs.h"
# include "tokens.h"

InitLexicon() {
	IPtr = InputLine;
	LookForCmd = 1;
	SaveStrings = 1;
}



Lexicon() {
	reg int		val;

	while (*IPtr == ' ' || *IPtr == '\t' || *IPtr == '\f')
		++IPtr;
	switch (*IPtr) {

	case 'a': 	case 'b':	case 'c':	case 'd':
	case 'e':	case 'f':	case 'g':	case 'h':
	case 'i':	case 'j':	case 'k':	case 'l':
	case 'm':	case 'n':	case 'o':	case 'p':
	case 'q':	case 'r':	case 's':	case 't':
	case 'u':	case 'v':	case 'w':	case 'x':
	case 'y':	case 'z':
	case 'A': 	case 'B':	case 'C':	case 'D':
	case 'E':	case 'F':	case 'G':	case 'H':
	case 'I':	case 'J':	case 'K':	case 'L':
	case 'M':	case 'N':	case 'O':	case 'P':
	case 'Q':	case 'R':	case 'S':	case 'T':
	case 'U':	case 'V':	case 'W':	case 'X':
	case 'Y':	case 'Z':
		val = IdLex();
		break;

	case '0':	case '1':	case '2':	case '3':
	case '4':	case '5':	case '6':	case '7':
	case '8':	case '9':	case '.':
		val = NumberLex();
		break;
	
	case '(':	case ')':	case '[':	case ']':
	case ',':	case ';':	case '%':	case '$':
	case '=':
		val = *IPtr++;
		break;
	
	case '+':	case '-':
		val = KAddop;
		yylval.o_val = (*IPtr == '+'? T_PLUS: T_MINUS);
		++IPtr;
		break;

	case '*':
		if (IPtr[1] == '*') {
			IPtr += 2;
			val = KExpop;
			yylval.o_val = T_POWER;
		} else {
			val = KMulop;
			yylval.o_val = T_TIMES;
			++IPtr;
		}
		break;

	case '/':
		val = KMulop;
		yylval.o_val = T_DIV;
		++IPtr;
		break;

	case '^':
		val = KExpop;
		yylval.o_val = T_POWER;
		++IPtr;
		break;

	case '<':
		if (IPtr[1] == '=') {
			IPtr += 2;
			val = KRelop;
			yylval.o_val = T_LEQ;
		} else if (IPtr[1] == '>') {
			IPtr += 2;
			val = KRelop;
			yylval.o_val = T_NEQ;
		} else {
			IPtr++;
			val = KRelop;
			yylval.o_val = T_LT;
		}
		break;
	
	case '>':
		if (IPtr[1] == '=') {
			IPtr += 2;
			val = KRelop;
			yylval.o_val = T_GEQ;
		} else {
			IPtr++;
			val = KRelop;
			yylval.o_val = T_GT;
		}
		break;

	case '|':
		if (IPtr[1] == '|') {
			IPtr += 2;
			val = KMulop;
			yylval.o_val = T_CONCAT;
		} else {
			val = *IPtr++;
		}
		break;

	case '\'':	case '"':
		val = StringLex();
		break;

	case '\0':	case '\n':
		*IPtr = '\0';
		val = 0;
		break;

	default:
		SyntaxError(IPtr-InputLine+1, "Invalid character (0%o).", *IPtr);
		val = *IPtr++;
		break;
	}

	LookForCmd = 0;		/* No commands after first token */
	return val;
}

struct keyword {
	char	*k_name;
	int	k_char;
	enum treeop	k_op;
} Commands[] = {			/* must be in alphabetical order */
	{ " ",		NULL,		T_NULL },
	{ "bye",	KQuit,		T_NULL },
	{ "clear",	KClear,		T_NULL },
	{ "continue",	KContinue,	T_NULL },
	{ "debug",	KDebug,		T_NULL },
	{ "delete",	KDelete,	T_NULL },
	{ "dump",	KDump,		T_NULL },
	{ "edit",	KEd,		T_NULL },
	{ "ex",		KEd,		T_NULL },
	{ "help",	KHelp,		T_NULL },
	{ "list",	KList,		T_NULL },
	{ "load",	KLoad,		T_NULL },
	{ "quit",	KQuit,		T_NULL },
	{ "renumber",	KRenumber,	T_NULL },
	{ "run",	KRun,		T_NULL },
	{ "save",	KSave,		T_NULL },
	{ "step",	KStep,		T_NULL },
	{ "usage",	KUsage,		T_NULL },
	{ "visual",	KVi,		T_NULL },
	{ "\177",	NULL,		T_NULL },
};

struct keyword Keywords[] = {		/* must be in alpha order */
	{ "abs",	KFunction,	T_ABS },
	{ "acs",	KFunction,	T_ACS },
	{ "and",	KLogop,		T_AND },
	{ "asn",	KFunction,	T_ASN },
	{ "atn",	KFunction,	T_ATN },
	{ "break",	KBreak,		T_NULL },
	{ "chr",	KChr,		T_CHR },
	{ "cos",	KFunction,	T_COS },
	{ "cot",	KFunction,	T_COT },
	{ "cpu",	KFunction,	T_CPU },
	{ "csc",	KFunction,	T_CSC },
	{ "data",	KData,		T_NULL },
	{ "deg",	KFunction,	T_DEG },
	{ "det",	KDet,		T_NULL },
	{ "dim",	KDim,		T_NULL },
	{ "dot",	KDot,		T_DOT },
	{ "else",	KElse,		T_NULL },
	{ "end",	KEnd,		T_NULL },
	{ "exp",	KFunction,	T_EXP },
	{ "for",	KFor,		T_NULL },
	{ "gosub",	KGosub,		T_NULL },
	{ "goto",	KGoto,		T_NULL },
	{ "hcs",	KFunction,	T_HCS },
	{ "hsn",	KFunction,	T_HSN },
	{ "htn",	KFunction,	T_HTN },
	{ "idn",	KIdn,		T_NULL },
	{ "idx",	KIdx,		T_REALIDX },
	{ "if",		KIf,		T_NULL },
	{ "input",	KInput,		T_NULL },
	{ "int",	KFunction,	T_INT },
	{ "inv",	KInv,		T_NULL },
	{ "len",	KLen,		T_LEN },
	{ "let",	KLet,		T_NULL },
	{ "lgt",	KFunction,	T_LGT },
	{ "log",	KFunction,	T_LOG },
	{ "ltw",	KFunction,	T_LTW },
	{ "mat",	KMat,		T_NULL },
	{ "max",	KFunction,	T_MAX },
	{ "min",	KFunction,	T_MIN },
	{ "next",	KNext,		T_NULL },
	{ "not",	KNot,		T_NOT },
	{ "num",	KNum,		T_NUM },
	{ "or",		KLogop,		T_OR },
	{ "prd",	KPrd,		T_PRD },
	{ "print",	KPrint,		T_NULL },
	{ "rad",	KFunction,	T_RAD },
	{ "read",	KRead,		T_NULL },
	{ "rem",	KRem,		T_NULL },
	{ "restore",	KRestore,	T_NULL },
	{ "return",	KReturn,	T_NULL },
	{ "rnd",	KRnd,		T_NULL },
	{ "sec",	KFunction,	T_SEC },
	{ "sgn",	KFunction,	T_SGN },
	{ "sin",	KFunction,	T_SIN },
	{ "sqr",	KFunction,	T_SQR },
	{ "step",	KStep,		T_NULL },
	{ "stop",	KStop,		T_NULL },
	{ "str",	KStr,		T_NULL },
	{ "sum",	KSum,		T_SUM },
	{ "tan",	KFunction,	T_TAN },
	{ "then",	KThen,		T_NULL },
	{ "tim",	KFunction,	T_TIM },
	{ "to",		KTo,		T_NULL },
	{ "trn",	KTrn,		T_NULL },
	{ "zer",	KZer,		T_NULL },
	{ "\177",	KId,		T_NULL },
};

int IdLex() {
	char	id[100], *idp;
	reg struct keyword *k, *kl, *kh;
	int	len, tok;

	tok = KId;
	idp = id;
	while (isalpha(*IPtr) || isdigit(*IPtr))
		*idp++ = *IPtr++;
	if (*IPtr == '$') {
		*idp++ = *IPtr++;
		tok = KStringId;
	} else if (*IPtr == '%') {
		*idp++ = *IPtr++;
		tok = KIntId;
	}
	len = idp - id;
#ifdef DEBUG
	if (Opts['L'] > 1) printf("\tlength %d\n", len);
#endif
	*idp++ = '\0';
	yylval.s_val = SaveString(id);

	if (LookForCmd && !IsLikeStmt(InputLine)) {
#ifdef DEBUG
		if (Opts['L'] > 1) printf("\tSearching Commands.\n");
#endif
		kl = Commands+1;
		kh = Commands + sizeof(Commands)/sizeof(Commands[0]);
		while (kl < kh) {
			k = kl + (kh-kl)/2;
			if (strncmp(id, k->k_name, len) > 0) {
#ifdef DEBUG
				if (Opts['L'] > 1)
					printf("\t%s > %s, new low %s\n",
						id, k->k_name, (k+1)->k_name);
#endif
				kl = k + 1;
			} else {
#ifdef DEBUG
				if (Opts['L'] > 1)
					printf("\t%s <= %s, new hi %s\n",
						id, k->k_name, k->k_name);
#endif
				kh = k;
			}
		}
		if (strncmp(id, kl->k_name, len) == 0) {
			/* Check for ambiguities */
			if (strncmp(id, (kl-1)->k_name, len) == 0
			   || strncmp(id, (kl+1)->k_name, len) == 0) {
				SyntaxError(LexColumn(), "'%s' is an ambiguous command name.", id);
				return 0;
			}
#ifdef DEBUG
			if (Opts['L'])
				printf("lex returns %s: %d\n", id, kl->k_char);
#endif
			return kl->k_char;
		}
	}

	kl = Keywords;
	kh = Keywords + sizeof(Keywords)/sizeof(Keywords[0]);
#ifdef DEBUG
	if (Opts['L']) printf("Searching keywords\n");
#endif
	while (kl < kh) {
		k = kl + (kh-kl)/2;
		if (strcmp(id, k->k_name) > 0) {
#ifdef DEBUG
			if (Opts['L'])
				printf("%s > %s, new low %s\n",
				id, k->k_name, (k+1)->k_name);
#endif
			kl = k + 1;
		} else {
#ifdef DEBUG
			if (Opts['L']) printf("%s <= %s, new hi %s\n",
				id, k->k_name, k->k_name);
#endif
			kh = k;
		}
	}
	if (strcmp(id, kl->k_name) == 0) {
		if (kl->k_op != T_NULL)
			yylval.o_val = kl->k_op;
#ifdef DEBUG
		if (Opts['L']) printf("lex returns %s: %d\n", id, kl->k_char);
#endif
		return kl->k_char;
	}

#ifdef DEBUG
	if (Opts['L']) { printf("lex returns %s: ID\n", id); }
#endif
	return tok;
}

# define CHECKCP	if (cp-num >= sizeof num - 1) {\
	Error("Number too large.");\
	yylval.n_val = 0.0;\
	return KNumber;\
}

int NumberLex() {
	char		num[200];
	reg char	*cp;
	double		atof();

	cp = num;
	if (*IPtr == '-') *cp++ = *IPtr++;
	else if (*IPtr == '+') IPtr++;
	while (isdigit(*IPtr)) {
		*cp++ = *IPtr++;
		CHECKCP
	}
	if (*IPtr == '.') {
		*cp++ = *IPtr++;
		while (isdigit(*IPtr)) {
			*cp++ = *IPtr++;
			CHECKCP
		}
	}

	if (*IPtr == 'e' || *IPtr == 'E') {
		*cp++ = *IPtr++;
		if (*IPtr == '-') *cp++ = *IPtr++;
		else if (*IPtr == '+') *cp++ = *IPtr++;
		while (isdigit(*IPtr)) {
			*cp++ = *IPtr++;
			CHECKCP
		}
	}
	*cp++ = '\0';
	yylval.n_val = atof(num);
#ifdef DEBUG
	if (Opts['L']) printf("number: %20.18e\n", yylval.n_val);
#endif
	return KNumber;
}


double TenPower(i)	int i; {
	double	t, exp;

#ifdef DEBUG
	if (Opts['L']) printf("Tenpower(%d) == ", i);
#endif
	if (i < 0) return 1.0/TenPower(-i);
	t = exp = 1.0;
	while (i > 0) {
		if (exp == 1.0) exp = 10.0;
		else exp *= exp;
		if (i & 01) t *= exp;
		i >>= 1;
	}
#ifdef DEBUG
	if (Opts['L']) fprintf(stderr, "%g\n", t);
#endif
	return t;
}


# define CHUNKSIZE	9	/* nine decimal digits per chunk */
# define reg	register

double atof(str)	char *str; {
	int		nc, tmp, nd;
	int		k, exp, dp;
	int		a[4], expsign, xsigned;
	double		f, TenPower();
	char		*cp;

	cp = str;
	nc = tmp = nd = exp = dp = xsigned = 0;
	if (*cp == '+') ++cp;
	else if (*cp == '-') {
		xsigned = 1;
		++cp;
	}
	while (*cp == '0') ++cp;
	while (isdigit(*cp) || *cp == '.') {
		if (*cp == '.') {
			dp = 1;
			++cp;
		} else {
			tmp = tmp*10 + *cp++ - '0';
			if (++nd >= CHUNKSIZE) {
				a[nc++] = tmp;
				tmp = nd = 0;
			}
			if (!dp) ++exp;
		}
	}

	if (nd > 0) {
		while (nd++ < CHUNKSIZE)
			tmp *= 10;
		a[nc++] = tmp;
	}

	if (*cp == 'e' || *cp == 'E') {
		++cp;
		expsign = 0;
		if (*cp == '-') {
			expsign = 1;
			++cp;
		} else if (*cp == '+')
			++cp;
		tmp = 0;
		while (isdigit(*cp)) {
			tmp = tmp*10 + *cp++ - '0';
		}
		if (expsign) exp -= tmp;
		else exp += tmp;
	}
	f = 0;
	if (exp > 38) {
		Error("Exponent too large: %d", exp-1);
	} else if (exp < -38) {
		Error("Exponent too small: %d", exp-1);
	} else {
		tmp = exp;
		for (k = 0; k < nc; ++k) {
			tmp -= CHUNKSIZE;
			f += a[k]*TenPower(tmp);
#ifdef DEBUG
			if (Opts['L']) printf("next f: %g\n", f);
#endif
		}
	}
	if (dp == 0 && exp > 0 && exp <= CHUNKSIZE) {
#ifdef DEBUG
		if (Opts['L']) fprintf(stderr, "prev f: %g\n", f);
#endif
		f = (int)(f + 0.5);
#ifdef DEBUG
		if (Opts['L']) fprintf(stderr, "next f: %g\n", f);
#endif
	}
	if (xsigned) f = -f;
#ifdef DEBUG
	if (Opts['L']) fprintf(stderr, "returning f: %g\n", f);
#endif
	return f;
}

int StringLex() {
	static char	str[256], delim;
	reg char	*strp;

	strp = str;
	delim = *IPtr++;
	while (*IPtr) {
		if (*IPtr == delim) {
			if (IPtr[1] == delim) {
				*strp++ = delim;
				IPtr += 2;
			} else break;
		} else if (*IPtr == '\\') {
			++IPtr;
			if (isdigit(*IPtr)) {
				char ch = *IPtr++ - '0';

				if (isdigit(*IPtr)) ch = ch*8 + *IPtr++ - '0';
				if (isdigit(*IPtr)) ch = ch*8 + *IPtr++ - '0';
				if (ch == '\0') {
					Error("\\0 ignored.");
					continue;
				}
				*strp++ = ch;
			} else {
				switch (*IPtr++) {
				case 'b':	*strp++ = '\b'; break;
				case 'f':	*strp++ = '\f'; break;
				case 'n':	*strp++ = '\n'; break;
				case 'r':	*strp++ = '\r'; break;
				case 't':	*strp++ = '\t'; break;
				case '\\':	*strp++ = '\\'; break;
				case '\'':	*strp++ = '\''; break;
				case '"':	*strp++ = '"'; break;
				default:	*strp++ = *(IPtr-1); break;
				}
			}
		} else {
			*strp++ = *IPtr++;
		}
	}
	*strp++ = '\0';
	if (*IPtr != delim) {
		Error("String not terminated.");
	} else {
		++IPtr;
	}
	if (SaveStrings) yylval.s_val = SaveString(str);
	else yylval.s_val = str;
	return KString;
}

char *FindStart(lin)	reg char *lin; {
	reg char	*cp;

	cp = lin;
	SkipBlanks(cp);
	while (isdigit(*cp))
		++cp;
	SkipBlanks(cp);
	return cp;
}



IsLikeStmt(l)	char *l; {
	int	lev;

	SkipBlanks(l);
	if (!isalpha(*l)) return 0;
	while (isalpha(*l)) ++l;
	SkipBlanks(l);
	if (*l == '(' || *l == ')') {
		/* Skip to matching char */
		lev = 0;
		do {
			if (*l == '(' || *l == '[') ++lev;
			else if (*l == ')' || *l == ']') --lev;
			else if (*l == '\'' || *l == '"') {
				char	match = *l;

				while (*++l && *l != match)
					;
			}
			if (*l) ++l;
		} while (lev > 0);
		SkipBlanks(l);
	}
	if (*l == '=') return 1;
	return 0;
}



LexColumn() {
	return IPtr - InputLine;
}



FlushInput() {
	while (*IPtr != '\0' && *IPtr != '\n')
		++IPtr;
	*IPtr = '\0';
}
