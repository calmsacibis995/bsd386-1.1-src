#include <stddef.h>
#include <unistd.h>
#include <stdio.h>

char *
cuserid(char *s)
{
    if (s) {
	strncpy(s, getlogin(), L_cuserid);
	return s;
    } else
	return getlogin();
}
