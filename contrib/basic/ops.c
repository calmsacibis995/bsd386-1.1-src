# include "structs.h"

enum treeop MatOpMap(op)	enum treeop op; {
	switch (op) {
	case T_PLUS:	return T_MAT_ADD;
	case T_MINUS:	return T_MAT_SUB;
	case T_TIMES:	return T_MAT_MUL;
	default:
		Error("Wrong operator for matrices.");
		return T_NULL;
	}
}

enum treeop MatScOpMap(op)	enum treeop op; {
	switch (op) {
	case T_PLUS:	return T_MAT_SC_ADD;
	case T_MINUS:	return T_MAT_SC_SUB;
	case T_TIMES:	return T_MAT_SC_MUL;
	default:
		Error("Wrong operator between scalar and matrix.");
		return T_NULL;
	}
}

enum treeop NumStrOpMap(op)	enum treeop op; {
	switch (op) {
	case T_ASSIGN:	return T_STR_ASSIGN;
	case T_EQUAL:	return T_STR_EQUAL;
	case T_GEQ:	return T_STR_GEQ;
	case T_GT:	return T_STR_GT;
	case T_NEQ:	return T_STR_NEQ;
	case T_LEQ:	return T_STR_LEQ;
	case T_LT:	return T_STR_LT;

	default:
		Error("Basic Bug: Try to map operator '%s'", OpName(op));
		abort();
		/*NOTREACHED*/
	}
}

/* For format specifiers (%<ch>), see PrintTree() */
struct op_descr OpDescrs[] = {
	{ "=",		2, "%t = %t" },
	{ "data",	1, "data %t" },
	{ "DATALIST",	2, "%*%t%?, %t" },
	{ "dim",	1, "dim %t" },
	{ "DIMLIST",	2, "%t%?, %t" },
	{ "DIM0",	1, "%t" },
	{ "DIM1",	2, "%t[%t]" },
	{ "DIM2",	3, "%t[%t,%t]" },
	{ "DIM_INT",	1, "%s" },
	{ "DIM_STRING",	2, "%s%n" },
	{ "DIM_NUM",	1, "%s" },
	{ "syntax",	1, "%s" },
	{ "/",		2, "%o/%o" },
	{ "=",		2, "%o = %o" },
	{ "LONGPRINT",	2, "%t%?, %t" }, 
	{ "SHORTPRINT",	2, "%t;%? %t" },
	{ "for",	4, "for %s = %t to %t%? step %t" },
	{ "for (int)",	4, "for %s = %t to %t%? step %t" },
	{ ">=",		2, "%o >= %o" },
	{ "gosub",	1, "gosub %l" },
	{ "goto",	1, "goto %l" },
	{ ">",		2, "%o > %o" },
	{ "<id>",	1, "%s" },
	{ "if",		3, "if %t then %t%? else %t" },
	{ "input",	1, "input %t" },
	{ "read",	1, "read %t" },
	{ "restore",	0, "restore" },
	{ "<=",		2, "%o <= %o" },
	{ "<",		2, "%o < %o" },
	{ "-",		2, "%o-%o" },
	{ "<>",		2, "%o <> %o" },
	{ "next",	1, "next %s" },
	{ "NULL",	0, "(null)" },
	{ "<number>",	1, "%g" },
	{ "+",		2, "%o+%o" },
	{ "^",		2, "%o^%o" },
	{ "print",	1, "print %t" },
	{ "return",	0, "return" },
	{ "stop",	0, "stop" },
	{ "<string>",	1, "%q" },
	{ "[]",		2, "%t[%t]" },
	{ "[,]",	3, "%t[%t,%t]" },
	{ "*",		2, "%o*%o" },
	{ "-",		1, "-%o" },
	{ "VARLIST",	2, "%t%?, %t" },
	{ "end",	0, "end" },
	{ "rem",	1, "%s" },
	{ "or",		2, "%o or %o" },
	{ "and",	2, "%o and %o" },
	{ "printnocr",	1, "print %t;" },
	{ "not",	1, "not %o" },
	{ "<intid>",	1, "%s" },
	{ "<stringid>",	1, "%s" },
	{ "||",		2, "%o || %o" },
	{ "break",	0, "break" },
	{ "()",		1, "(%t)" },
	{ "mat assign",	2, "mat %s = %s" },
	{ "mat add",	3, "mat %s = %s+%s" },
	{ "mat sub",	3, "mat %s = %s-%s" },
	{ "mat mult",	3, "mat %s = %s*%s" },
	{ "mat sc ass",	2, "mat %s = (%t)" },
	{ "mat sc add",	3, "mat %s = (%t) + %s" },
	{ "mat sc sub", 3, "mat %s = (%t) - %s" },
	{ "mat sc mul",	3, "mat %s = (%t) * %s" },
	{ "mat idn",	1, "mat %s = idn" },
	{ "mat zer",	1, "mat %s = zer" },
	{ "mat inv",	2, "mat %s = inv(%s)" },
	{ "mat trn",	2, "mat %s = trn(%s)" },
	{ "mat unary -",2, "mat %s = -%s" },
	{ "mat input",	1, "mat input %t" },
	{ "mat read",	1, "mat read %t" },
	{ "mat print",	1, "mat print %t" },
	{ "string =",	2, "%o = %o" },
	{ "string >=",	2, "%o >= %o" },
	{ "string >",	2, "%o > %o" },
	{ "string <>",	2, "%o <> %o" },
	{ "string <=",	2, "%o <= %o" },
	{ "string <",	2, "%o < %o" },
	{ "string ass",	2, "%t = %t" },
	{ "mat fn",	2, "mat %s = %f" },
	{ "mat ()",	3, "mat %s = %f(%s)" },
	{ "mat (,)",	4, "mat %s = %f(%s,%s)" },
	{ "mat (...)",	3, "mat %s = %f(%t)" },
	{ "fn0",	1, "%f" },
	{ "()",		2, "%f(%t)" },
	{ "(,)",	3, "%f(%t, %t)" },
	{ "(...)",	2, "%f(%t)" },
	{ "sin",	1, "sin(%t)" },
	{ "cos",	1, "cos(%t)" },
	{ "tan",	1, "tan(%t)" },
	{ "sqr",	1, "sqr(%t)" },
	{ "exp",	1, "exp(%t)" },
	{ "log",	1, "log(%t)" },
	{ "abs",	1, "abs(%t)" },
	{ "int",	1, "int(%t)" },
	{ "acs",	1, "acs(%t)" },
	{ "asn",	1, "asn(%t)" },
	{ "atn",	1, "atn(%t)" },
	{ "cot",	1, "cot(%t)" },
	{ "csc",	1, "csc(%t)" },
	{ "hcs",	1, "hcs(%t)" },
	{ "hsn",	1, "hsn(%t)" },
	{ "htn",	1, "htn(%t)" },
	{ "lgt",	1, "lgt(%t)" },
	{ "ltw",	1, "ltw(%t)" },
	{ "sec",	1, "sec(%t)" },
	{ "rnd",	0, "rnd" },
	{ "setrnd",	1, "rnd(%t)" },
	{ "deg",	1, "deg(%t)" },
	{ "rad",	1, "rad(%t)" },
	{ "max",	2, "max(%t, %t)" },
	{ "min",	2, "min(%t, %t)" },
	{ "chr",	1, "chr(%t)" },
	{ "num",	1, "num(%t)" },
	{ "len",	1, "len(%t)" },
	{ "sgn",	1, "sgn(%t)" },
	{ "sum",	1, "sum(%s)" },
	{ "prd",	1, "prd(%s)" },
	{ "dot",	2, "dot(%s, %s)" },
	{ "tim",	0, "tim" },
	{ "cpu",	0, "cpu" },
	{ "idx",	2, "idx(%t, %t)" },
	{ "str",	2, "str(%t, %t)" },
	{ "str",	3, "str(%t, %t, %t)" },
	{ "det",	1, "det(%s)" },
};

char *OpName(op)	enum treeop op; {
	struct op_descr *od;

	od = OpDescrs + (int)op;
	return od->o_name;
}
