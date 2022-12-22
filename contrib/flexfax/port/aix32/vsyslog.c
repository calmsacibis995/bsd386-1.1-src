#include <stdio.h>
#include <sys/types.h>
#include <stdarg.h>
#include <sys/syslog.h>

void
vsyslog(pri, fmt, ap)
	int pri;
	register const char *fmt;
	va_list ap;
{
	char tbuf[2048];

	(void)vsprintf(tbuf, fmt, ap);
	(void)syslog(pri, tbuf);
	return;
}
