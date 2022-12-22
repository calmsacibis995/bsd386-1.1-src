# include "structs.h"

# define argv	t->t_args[argc]

/*VARARGS2*/
struct tree_node *Tree(ct, op, a1, a2, a3, a4, a5)	reg int ct; reg enum treeop op; {
	reg struct tree_node	*node;
	struct tree_node		dummy;
	unsigned	nbytes;

	nbytes = (char *) &dummy.t_args[ct] - (char *) &dummy;
	node = (struct tree_node *) malloc(nbytes);
	if (node == NULL) {
		Error("Can't allocate any more tree nodes.");
		exit(1);
	}

	node->t_op = op;
	node->t_len = ct;
	switch (ct) {
	case 5:	int4(node) = a5;	/* Fall through */
		node->t_symbols[4] = 0; node->t_version[4] = 0;
	case 4:	int3(node) = a4;	/* Fall through */
		node->t_symbols[4] = 0; node->t_version[4] = 0;
	case 3:	int2(node) = a3;	/* Fall through */
		node->t_symbols[2] = 0; node->t_version[4] = 0;
	case 2:	int1(node) = a2;	/* Fall through */
		node->t_symbols[1] = 0; node->t_version[4] = 0;
	case 1:	int0(node) = a1;
		node->t_symbols[0] = 0; node->t_version[4] = 0;
	case 0:
		break;
	default:
		Error("Wrong number of arguments to Tree().");
		exit(1);
	}
	return node;
}



FreeTree(t)	reg struct tree_node *t; {
	reg int		argc;
	struct op_descr	*od;
	reg char	*cp;

	if (t == NULL) return;
	argc = 0;
	od = OpDescrs + (int)(t->t_op);
	cp = od->o_fmt;
	while (*cp) {
		if (*cp++ == '%') {
			switch(*cp++) {
			case 't': /* Print another tree (recursive call) */
			case 'o':
				FreeTree(argv.t_tree);
				++argc;
				break;
			case '?': /* If next arg NULL, return */
				if (argv.t_tree == NULL) goto ret;
				break;
			default:
				++argc;
				break;
			}
		} 
	}
ret:
	free((char *) t);
}



DumpTree(t, depth)	reg struct tree_node *t; int depth; {
	reg int		i, argc;
	struct op_descr	*od;
	reg char	*cp;

	if (t == NULL) return;
	argc = 0;

	od = OpDescrs + (int)(t->t_op);
	for (i = 1;  i <= depth; ++i) printf( "   ");
	printf( "%s (%d):\n", od->o_name, od->o_nargs);
	cp = od->o_fmt;
	while (*cp) {
		if (*cp++ == '%') {
			switch(*cp++) {
			case 't': /* Print another tre (recursive call) */
			case 'o':
			case '*': /* Print 'number*' if next arg > 1 */
				DumpTree(argv.t_tree, depth+1);
				++argc;
				continue;
			case '?': /* If next arg NULL, return */
				if (argv.t_tree == NULL) {
					putchar('\n');
					return;
				}
				continue;
			case 's': /* Print a string */
				for (i = 1;  i <= depth+1; ++i) printf( "   ");
				printf( "%s", argv.t_string);
				++argc;
				break;
			case 'q': /* Print a quoted string */
				for (i = 1;  i <= depth+1; ++i) printf( "   ");
				PrintString(stdout, argv.t_string);
				++argc;
				break;
			case 'g': /* Print a number */
				for (i = 1;  i <= depth+1; ++i) printf( "   ");
				printf( "%g", argv.t_number);
				++argc;
				break;
			case 'i': /* Print an embedded integer */
				for (i = 1;  i <= depth+1; ++i) printf( "   ");
				printf( "%d", argv.t_int);
				++argc;
				break;
			case 'l': /* Print a lin number */
				for (i = 1;  i <= depth+1; ++i) printf( "   ");
				printf( "%d", argv.t_lineno);
				++argc;
				break;
			case 'n': /* Print a string length */
				for (i = 1;  i <= depth+1; ++i) printf( "   ");
				if (argv.t_int != DEFAULT_STRLEN)
					printf("%d", argv.t_int);
				break;
			default:
				printf( "%%%c", *(cp-1));
				++argc;
				break;
			}
			putchar('\n');
		} 
	}
}


ListCode(start, end)	reg long start, end; {
	reg struct line_descr	*ln;

	ln = FindLine(start);

	for (; IsValidLine(ln) && ln->l_no <= end; ln = LineAfter(ln)) {
		printf("%5d:", ln->l_no);
		DumpTree(ln->l_tree, 1);
	}
}



PrintTree(f, t)	reg FILE *f; reg struct tree_node *t; {
	reg int		argc = 0;
	struct op_descr	*od;
	reg char	*cp;

	if (t == NULL) return;
	od = OpDescrs + (int)(t->t_op);
	cp = od->o_fmt;

	while (*cp) {
		if (*cp != '%') {
			putc(*cp, f);
			++cp;
		} else {
			++cp;
			switch(*cp++) {
			case 't': /* Print another tree (recursive call) */
				PrintTree(f, argv.t_tree);
				++argc;
				break;
			case '?': /* If next arg NULL, return */
				if (argv.t_tree == NULL) return;
				break;
			case '*': /* Print 'number*' if next arg > 1 */
				if (t->t_args[argc].t_int != 1)
					fprintf(f, "%d*", argv.t_int);
				++argc;
				break;
			case 's': /* Print a string */
				fprintf(f, "%s", argv.t_string);
				++argc;
				break;
			case 'q': /* Print a quoted string */
				PrintString(f, argv.t_string);
				++argc;
				break;
			case 'o': /* Print a tree, parenthesize if necessary */
				if (PrecLevel(argv.t_tree->t_op)
				    < PrecLevel(t->t_op)) {
					fprintf(f, "(");
					PrintTree(f, argv.t_tree);
					fprintf(f, ")");
				} else {
					PrintTree(f, argv.t_tree);
				}
				++argc;
				break;
			case 'f': /* Print a function name */
				fprintf(f, "%s", OpName(argv.t_op));
				++argc;
				break;
			case 'g': /* Print a number */
				fprintf(f, "%g", argv.t_number);
				++argc;
				break;
			case 'i': /* Print an embedded integer */
				fprintf(f, "%d", argv.t_int);
				++argc;
				break;
			case 'l': /* Print a line number */
				fprintf(f, "%d", argv.t_lineno);
				++argc;
				break;
			case 'n': /* Print a string length */
				if (argv.t_int != -DEFAULT_STRLEN)
					fprintf(f, "%d", argv.t_int);
				break;
			default:
				fprintf(f, "%%c", *(cp-1));
				break;
			}
		} 
	}
}

PrintString(f, str)	reg FILE *f; reg char *str; {
	putc('\'', f);
	while (*str) {
		if (*str == '\'') putc('\'', f);
		putc(*str, f);
		++str;
	}
	putc('\'', f);
}


char *VarName(t)	reg struct tree_node *t; {
	char	name[100];

	switch (t->t_op) {
	case T_REALID:
		return string0(t);
	case T_STRINGID:
		(void)sprintf(name, "%s$", string0(t));
		return name;
	case T_SUB1:	case T_SUB2:
		return VarName(tree0(t));
	default:
		return OpName(t->t_op);
	}
}


PrecLevel(op)	reg enum treeop op; {
	switch (op) {
	case T_POWER:
		return 6;
	case T_TIMES:
	case T_DIV:
		return 5;
	case T_PLUS:
	case T_MINUS:
		return 4;
	case T_EQUAL:
	case T_LT:
	case T_LEQ:
	case T_GT:
	case T_GEQ:
	case T_NEQ:
		return 3;
	case T_AND:
	case T_OR:
		return 2;
	default:
		/* Everything else binds very tightly */
		return 9;
	}
}
