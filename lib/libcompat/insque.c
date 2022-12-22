/*	BSDI	$Id: insque.c,v 1.1 1993/12/21 02:22:02 polk Exp $	*/
/* copied from the GNU Emacs distribution (a.k.a. copylefted) */

struct qelem {
	struct qelem *q_forw;
	struct qelem *q_back;
	char q_data[1];
};

/* Insert ELEM into a doubly-linked list, after PREV.  */

void
insque(elem, prev)
	struct qelem *elem, *prev;
{
	struct qelem *next = prev->q_forw;

	prev->q_forw = elem;
	if (next)
		next->q_back = elem;
	elem->q_forw = next;
	elem->q_back = prev;
}

/* Unlink ELEM from the doubly-linked list that it is in.  */

remque(elem)
	struct qelem *elem;
{
	struct qelem *next = elem->q_forw;
	struct qelem *prev = elem->q_back;

	if (next)
		next->q_back = prev;
	if (prev)
		prev->q_forw = next;
}
