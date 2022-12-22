/* -*-C-*- moveover.h */
/*-->moveover*/
/**********************************************************************/
/****************************** moveover ******************************/
/**********************************************************************/

void
moveover(b)
register INT32 b;

{
    /*	From DVITYPE Version 2.6:
	"Rounding to the nearest pixel is  best done in the manner shown
	here, so as to  be inoffensive to the  eye: When the  horizontal
	motion is small, like a kern, |hh| changes by rounding the kern;
	but when the motion is large, |hh| changes by rounding the  true
	position |h| so that  accumulated rounding errors disappear.  We
	allow a  larger space  in  the negative  direction than  in  the
	positive one, because TeX  makes comparatively large  backspaces
	when it positions accents."

	The one precaution we need to  take here is that fontptr can  be
	NULL, which we treat like  a large movement.  This NULL  pointer
	was used without error  on many different  machines for 2  years
	before it was caught on the VAX VMS implementation, which  makes
	memory page 0 inaccessible.
    */
    h += b;
    if ((fontptr == (struct font_entry *)NULL) ||
	ABS(b) >= fontptr->font_space)
	hh = PIXROUND(h, conv) + lmargin;
    else
    {
	hh += PIXROUND(b, conv);
	hh = fixpos(hh-lmargin,h,conv) + lmargin;
    }
}

