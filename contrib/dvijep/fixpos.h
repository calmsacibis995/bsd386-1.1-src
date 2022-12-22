/* -*-C-*- fixpos.h */
/*-->fixpos*/
/**********************************************************************/
/******************************* fixpos *******************************/
/**********************************************************************/
COORDINATE
fixpos(cc,c,cnvfac)
register COORDINATE cc;		/* coordinates in device pixels */
register INT32 c;		/* coordinates in DVI units */
register float cnvfac;		/* converts DVI units to pixels */
{
    register COORDINATE ccc;

    /*
    A sequence  of consecutive  rules, or  consecutive characters  in  a
    fixed-width font whose width is not an integer number of pixels, can
    cause |cc| to  drift far away  from a correctly  rounded value.   We
    follow DVITYPE Version 2.6 to ensure  that the amount of drift  will
    never exceed MAXDRIFT pixels.  DVITYPE Version 2.0 did not do  this,
    and the results could be visibly AWFUL!!
    */

    ccc = PIXROUND(c,cnvfac);
    if (ABS(ccc-cc) > MAXDRIFT)
    {		/* drag cc toward the correctly rounded value ccc */
        if (ccc > cc)
	{
	    cc = ccc - MAXDRIFT;
	}
        else
	{
            cc = ccc + MAXDRIFT;
	}
    }
    return (cc);
}
