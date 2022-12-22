# include "structs.h"

struct renum {
		long	r_prev;
		long  r_new;
};

RenumberLines(start, end, firstnew)	long start, end, firstnew; {
	reg long	ln;
	reg int		incr, ct;
	reg struct renum	*renlist, *r;
	reg struct line_descr	*l;

	incr = 10;
	ct = LineCount();

	if (ct == 0) {
		Error("No lines in range.");
		return;
	}

	l = FindLine(start);
	if (l != FirstLine && LineBefore(l)->l_no >= firstnew) {
err:		Error("Can't overlap lines in renumber.");
		return;
	}

	l = FindLine(end);
	if (IsValidLine(l) && ct*incr + firstnew >= LineAfter(l)->l_no)
		goto err;

	r = renlist = (struct renum *) malloc((unsigned) (ct * sizeof *r));
	ln = firstnew;
	for (l = FindLine(start); IsValidLine(l) && l->l_no <= end;
		l = LineAfter(l), r++, ln += incr) {
		r->r_prev = l->l_no;
		r->r_new = ln;
	}

	for (l = FindLine(ZERO); IsValidLine(l); l = LineAfter(l))
		RenumberStmt(l->l_tree, renlist, ct);

	for (l = FindLine(start), r = renlist; IsValidLine(l) && l->l_no <= end;
		++r, l = LineAfter(l)) {
		l->l_no = r->r_new;
	}
	MadeChanges();
}


RenumberStmt(t, renlist, ct)
	reg struct tree_node	*t;		/* statement tre to renumber */
	reg struct renum	*renlist;	/* list if lines renumbered */
	reg int		ct;		/* number of lines in renlist */
{
	/* Check for terminal cases */
	switch (t->t_op) {
	case T_GOTO:
		lineno0(t) = MatchLine(lineno0(t), renlist, ct);
		return;
	case T_GOSUB:
		lineno0(t) = MatchLine(lineno0(t), renlist, ct);
		return;

	case T_IF:
	    if (tree1(t))
		RenumberStmt(tree1(t), renlist, ct);
	    if (tree2(t))
		RenumberStmt(tree2(t), renlist, ct);
		return;

	default:
		return;
	}
}


MatchLine(lno, renlist, ct)
	reg long	lno;		/* statement number to renumber */
	reg struct renum	*renlist;	/* list if lines renumbered */
	reg int		ct;		/* number of lines in renlist */
{
	reg struct renum	*high, *low, *mid;

	if (lno < renlist[0].r_prev)
		return lno;
	low = renlist;
	high = low + ct - 1;
	while (low < high) {
		mid = low + (high - low)/2;
		if (lno < mid->r_prev) {
			high = mid - 1;
		} else if (lno > mid->r_prev) {
			low = mid + 1;
		} else {
			return mid->r_new;
		}
	}
	return low->r_new;
}
