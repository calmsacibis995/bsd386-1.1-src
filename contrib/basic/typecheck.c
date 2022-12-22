# include "structs.h"

struct tree_node *TypeCheck(t)	reg struct tree_node *t; {
	reg int		i;
	reg enum valkind	lt, rt, mt;
	enum valkind		TypeOf();
	reg enum treeop	op;

	op = t->t_op;
	switch (op) {
	case T_IF:
		lt = TypeOf(tree0(t));
		if (lt != V_BOOLEAN) {
			Error("Condition in 'if' should be boolean.");
			return NULL;
		}
		return t;

	case T_FOR:
	case T_FOR_INT:
		for (i = 1; i < 3; ++i) {
			lt = TypeOf(treei(t, i));
			if (lt != V_REAL) {
				Error("%s value should be numeric.",
					i==1? "initial": i==2? "final": "step");
				return NULL;
			}
		}
		return t;

	case T_ASSIGN:	case T_EQUAL:	case T_GEQ:	case T_GT:
	case T_LEQ:	case T_LT:	case T_NEQ:
		lt = TypeOf(tree0(t));
		rt = TypeOf(tree1(t));
		if (!TypesMatch(op, lt, rt)) return NULL;
		if (lt == V_STRING) {
			t->t_op = NumStrOpMap(op);
		}
		return t;

	case T_AND:	case T_OR:
		lt = TypeOf(tree0(t));
		rt = TypeOf(tree1(t));
		if (lt != V_BOOLEAN) {
			Error("Left operand of %s should be boolean.",
				OpName(t->t_op));
			return NULL;
		}
		if (rt != V_BOOLEAN) {
			Error("Right operand of %s should be boolean.",
				OpName(t->t_op));
			return NULL;
		}
		return t;

	case T_PLUS:
		lt = TypeOf(tree0(t));
		rt = TypeOf(tree1(t));
		if (!TypesMatch(op, lt, rt)) return NULL;
		if (lt == V_STRING) op = T_CONCAT;
		return t;

	case T_DIV:	case T_MINUS:	case T_POWER:	case T_TIMES:
		lt = TypeOf(tree0(t));
		rt = TypeOf(tree1(t));
		if (!TypesMatch(op, lt, rt)) return NULL;
		if (!NumberType(op, lt)) return NULL;
		return t;

	case T_CONCAT:
		lt = TypeOf(tree0(t));
		rt = TypeOf(tree1(t));
		if (!TypesMatch(op, lt, rt)) return NULL;
		if (!StringType(op, lt)) return NULL;
		return t;

	case T_MAT_ADD:		case T_MAT_SUB:		case T_MAT_MUL:
	case T_MAT_SC_ADD:	case T_MAT_SC_SUB:	case T_MAT_SC_MUL:
		lt = TypeOf(tree0(t));
		mt = TypeOf(tree1(t));
		rt = TypeOf(tree2(t));
		if (lt != V_REAL || mt != V_REAL || rt != V_REAL) {
			Error("Operands of MAT statement must be numeric.");
			return NULL;
		}
		return t;

	case T_MAT_ASSIGN:	case T_MAT_SC_ASS:
	case T_MAT_INV:		case T_MAT_TRN:
		lt = TypeOf(tree0(t));
		rt = TypeOf(tree1(t));
		if (lt != V_REAL || rt != V_REAL) {
			Error("Operands of MAT statement must be numeric.");
			return NULL;
		}
		return t;

	case T_MAT_REALIDN:
	case T_MAT_ZER:
		lt = TypeOf(tree0(t));
		if (lt != V_REAL) {
			Error("Operands of MAT statement must be numeric.");
			return NULL;
		}
		return t;

	case T_MAT_FUNCALL0:
		if (!ArgCheck(op1(t), (int)t->t_len-2, 0)) return NULL;
		return t;

	case T_MAT_FUNCALL1:
		if (!ArgCheck(op1(t), (int)t->t_len-2, 1)) return NULL;
		return t;

	case T_MAT_FUNCALL2:
		if (!ArgCheck(op1(t), (int)t->t_len-2, 2)) return NULL;
		return t;

	case T_SUB1:	case T_SUB2:
		for (i = 1; i <= 2; ++i) {
			lt = TypeOf(tree0(t));
			if (lt != V_REAL) {
				Error("Subscript %d of %s should be numeric.",
					i, VarName(t));
				return NULL;
			}
			if (op == T_SUB1) break;
		}
		return t;

	case T_NOT:	case T_UMINUS:
	case T_CHR:
		if (!ArgCheck(op, (int)t->t_len-1, 0)) return NULL;
		lt = TypeOf(tree0(t));
		if (!NumberType(op, lt)) return NULL;
		return t;

	case T_PARENS:
		return t;

	case T_FUNCALL0:
		return t;

	case T_FUNCALL1:
		op = op0(t);
		if (!ArgCheck(op, (int)t->t_len-1, 1)) return NULL;
		lt = TypeOf(tree1(t));
		if (!NumberType(op, lt)) return NULL;
		return t;

	case T_FUNCALL2:
		op = op0(t);
		if (!ArgCheck(op, (int)t->t_len-1, 2)) return NULL;
		lt = TypeOf(tree1(t));
		if (!NumberType(op, lt)) return NULL;
		lt = TypeOf(tree2(t));
		if (!NumberType(op, lt)) return NULL;
		return t;

	case T_SUM:	case T_PRD:	case T_DET:
		/* Guaranteed by grammar */
		return t;

	case T_NUM:	case T_LEN:
		if (!ArgCheck(op, (int)t->t_len, 1)) return NULL;
		lt = TypeOf(tree0(t));
		if (!StringType(op, lt)) return NULL;
		return t;

	case T_STR2: case T_STR3:
		lt = TypeOf(tree0(t));
		if (lt != V_STRING) {
			Error("First operand of %s should be string.",
				OpName(op));
			return NULL;
		}
		for (i = 1; i <= 2; ++i) {
			lt = TypeOf(treei(t, i));
			if (lt != V_REAL) {
				Error("Argument %d of %s should be numeric.",
					i, OpName(op));
				return NULL;
			}
			if (op == T_STR2) break;
		}
		return t;

	case T_REALIDX:
		if (!ArgCheck(op, (int)t->t_len, 2)) return NULL;
		for (i = 0; i <= 1; ++i) {
			lt = TypeOf(treei(t, i));
			if (!StringType(op, lt)) return NULL;
		}
		return t;

	case T_TIM:
		if (!ArgCheck(op, (int)t->t_len, 0)) return NULL;
		return t;

	default:
		Error("Basic Bug!  You forgot about TypeCheck-ing %s.",
			OpName(op));
		return t;
	}
}


TypesMatch(op, lt, rt)	enum treeop op;  enum valkind lt, rt; {
	if (lt != rt) {
		Error("Operands of '%s' don't match.", OpName(op));
		return 0;
	}
	return 1;
}


enum valkind TypeOf(t)	reg struct tree_node *t; {
	if (t == NULL) return V_REAL;

	switch (t->t_op) {
	case T_ASSIGN:	case T_DIV:
	case T_MINUS:	case T_PLUS:	case T_POWER:	case T_NOT:
	case T_TIMES:	case T_SUB1:	case T_SUB2:	case T_PARENS:
		return TypeOf(tree0(t));

	case T_NEQ:	case T_EQUAL:	case T_GEQ:
	case T_GT:	case T_LEQ:	case T_LT:
	case T_STR_NEQ:	case T_STR_EQUAL:	case T_STR_GEQ:
	case T_STR_GT:	case T_STR_LEQ:	case T_STR_LT:
	case T_OR:	case T_AND:
		return V_BOOLEAN;

	case T_REAL:	case T_REALID:	case T_NUM:	case T_LEN:
	case T_DET:	case T_SUM:	case T_PRD:	case T_DOT:
	case T_INTID:	case T_REALIDX:
	case T_FUNCALL0:	case T_FUNCALL1:
	case T_FUNCALL2:	case T_FUNCALLN:
	case T_UMINUS:
		return V_REAL;

	case T_STRING:	case T_STRINGID: case T_CHR:	case T_STR2:
	case T_STR3:	case T_CONCAT:
		return V_STRING;

	default:
		Error("Basic Bug!  Forgot TypeOf %d, %s.",
			t->t_op, OpName(t->t_op));
		return V_REAL;
	}
}


ArgCheck(fun, found, required)	enum treeop fun; {
	if (found < required) {
		Error("Too few arguments to function %s", OpName(fun));
		return 0;
	} else if (found > required) {
		Error("Too many arguments to function %s", OpName(fun));
		return 0;
	}
	return 1;
}


NumberType(op, t)	enum treeop op;  enum valkind t; {
	if (t != V_REAL) {
		Error("Operand(s) of '%s' should be numeric.", OpName(op));
		return 0;
	}
	return 1;
}


StringType(op, t)	enum treeop op;  enum valkind t; {
	if (t != V_STRING) {
		Error("Operand(s) of '%s' should be strings.", OpName(op));
		return 0;
	}
	return 1;
}
