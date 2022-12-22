# include "structs.h"

# define STRING_HASH_SIZE	1327

typedef	struct string {
		char		*s_val;
		struct string	*s_next;
	} str_t;

str_t	*ST[STRING_HASH_SIZE];

char *SaveString(str)	reg char *str; {
	reg str_t	*s;
	reg int		h;

	h = HashString(str) % STRING_HASH_SIZE;
	for (s = ST[h]; s != NULL; s = s->s_next) {
		if (strcmp(s->s_val, str) == 0) {
			return s->s_val;
		}
	}
	s = (str_t *)malloc((unsigned)(sizeof(str_t) + strlen(str) + 1));
	s->s_val = (char *)s + sizeof(str_t);
	(void)strcpy(s->s_val, str);
	s->s_next = ST[h];
	ST[h] = s;
	return s->s_val;
}



FreeString(text)	reg char *text; {
	free(text);
}

