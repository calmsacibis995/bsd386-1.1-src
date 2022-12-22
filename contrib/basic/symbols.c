# include "structs.h"

struct name	*SymBuckets[SYMBOL_HASH_SIZE];

InitSymbols() {
	reg struct name	**n, *m;

	for (n = SymBuckets; n < SymBuckets + SYMBOL_HASH_SIZE; ++n) {
		for (m = *n; m != NULL; m = m->s_next)
			FreeSymbol(m);
		*n = NULL;
	}
	MasterSymbolVersion++;
}


struct name *AllocSymbol(name)	reg char *name; {
	reg struct name	*n;
	reg int		h;

	n = (struct name *) malloc(sizeof(struct name));
	n->s_name = name;
	n->s_val.v_kind = V_NULL;
	h = HashString(name) % SYMBOL_HASH_SIZE;
	n->s_next = SymBuckets[h];
	SymBuckets[h] = n;
	return n;
}

FreeSymbol(n)	reg struct name *n; {
	switch (n->s_val.v_kind) {
	case V_1D_REAL:
	case V_2D_REAL:
		free((char *)(n->s_val.v_rmat.m_area));
		break;
	case V_1D_INT:
	case V_2D_INT:
		free((char *)(n->s_val.v_imat.m_area));
		break;
	case V_1D_STRING:
	case V_2D_STRING:
		free((char *)(n->s_val.v_smat.m_area));
		break;
	case V_STRING_VAR:
		free((char *)(n->s_val.v_stringvar.s_p));
		break;
	}
	free((char *)n);
}


struct name *LookupSymbol(name)	reg char *name; {
	reg int		h;
	reg struct name	*n;
	reg char *s;

	for (h=0,s=name;*s;) 
		h = (h<<3) + *s++;
	if (h < 0) h = -h;
	h %= SYMBOL_HASH_SIZE;
	for (n = SymBuckets[h]; n ; n = n->s_next) {
		if (n->s_name[0]!=name[0])continue;
		if (n->s_name[1]==0 && name[1]==0) return n;
		if (strcmp(n->s_name, name) == 0) return n;
	}
	return NULL;
}

HashString(s)	reg char *s; {
	reg int		h = 0;

	while (*s) 
		h = (h<<3) + *s++;
	if (h < 0) h = -h;
	return h;
}
