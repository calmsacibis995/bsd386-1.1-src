/* -*-C-*- special.h */
/*-->special*/
/**********************************************************************/
/****************************** special *******************************/
/**********************************************************************/

void
special(s)			/* process TeX \special{} string in s[] */
register char *s;
{
    (void)sprintf(message,
	"special():  TeX \\special{%s} not implemented in this driver",s);
    (void)warning(message);
}
