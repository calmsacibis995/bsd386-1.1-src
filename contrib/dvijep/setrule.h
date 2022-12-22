/* -*-C-*- setrule.h */
/*-->setrule*/
/**********************************************************************/
/****************************** setrule *******************************/
/**********************************************************************/

void
setrule(height, width, update_h)
register UNSIGN32 height, width;
register BOOLEAN update_h;

{   /* draw a rule with bottom left corner at (h,v) */

    if ((height > 0) && (width > 0))		/* non-empty rule */

#if    BBNBITGRAPH
	fillrect(hh+xscreen, YSIZE-vv+yscreen,
	    rulepxl(width,conv)  ,  rulepxl(height,conv));
#else
	fillrect(hh, YSIZE-vv,
	    rulepxl(width,conv), rulepxl(height,conv));
#endif

    if (update_h)
    {
	h += (INT32)width;
	hh += rulepxl(width, conv);
	hh = fixpos(hh-lmargin,h,conv) + lmargin;
    }
}
