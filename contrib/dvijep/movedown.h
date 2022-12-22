/* -*-C-*- movedown.h */
/*-->movedown*/
/**********************************************************************/
/****************************** movedown ******************************/
/**********************************************************************/

void
movedown(a)
register INT32 a;

{
    /*	From DVITYPE Version 2.6:
	"Vertical motion is done similarly [to horizontal motion handled
	in moveover()],  but with  the threshold  between ``small''  and
	``large'' increased by  a factor of  five. The idea  is to  make
	fractions  like  ``1/2''  round  consistently,  but  to   absorb
	accumulated rounding errors in the baseline-skip moves."

	The one precaution we need to  take here is that fontptr can  be
	NULL, which we treat like  a large movement.  This NULL  pointer
	was used without error  on many different  machines for 2  years
	before it was caught on the VAX VMS implementation, which  makes
	memory page 0 inaccessible.
    */
    v += a;
    if ((fontptr == (struct font_entry *)NULL) ||
        (ABS(a) >= 5*fontptr->font_space))
	vv = PIXROUND(v, conv) + tmargin;
    else
    {
	vv += PIXROUND(a, conv);
	vv = fixpos(vv-tmargin,v,conv) + tmargin;
    }
}

