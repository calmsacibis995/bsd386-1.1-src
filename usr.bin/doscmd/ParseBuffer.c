/*-
 * Copyright (c) 1992,1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
/*      Krystal $Id: ParseBuffer.c,v 1.1 1994/01/14 23:22:19 polk Exp $ */
#include <stdlib.h>

int
ParseBuffer(obuf, av, mac)
char *obuf;
char **av;
int mac;
{
	static char *_buf;
	char *buf;
	static int buflen = 0;
	int len;

        register char *b = buf;
        register char *p;
        register char **a;
	register char **e;

	len = strlen(obuf) + 1;
	if (len > buflen) {
		if (buflen)
			free(_buf);
		buflen = (len + 1023) & 1023;
		_buf = malloc(buflen);
	} 
	buf = _buf;
	strcpy(buf, obuf);

        a = av;
	e = &av[mac];

        while (*buf) {
                while (*buf == ' ' || *buf == '\t' || *buf == '\n')
                        ++buf;
                if (*buf) {
                        p = b = buf;

                        *a++ = buf;
			if (a == e) {
				a[-1] = (char *)0;
				return(mac - 1);
			}

                        while (*p && !(*p == ' ' || *p == '\t' || *p == '\n')) {
                                *b++ = *p++ & 0177;
                        }
                        if (*p)
                                ++p;
                        *b = 0;
                        buf = p;
                }
        }
        *a = (char *)0;
        return(a - av);
}
