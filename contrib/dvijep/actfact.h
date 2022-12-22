/* -*-C-*- actfact.h */
/*-->actfact*/
/**********************************************************************/
/****************************** actfact *******************************/
/**********************************************************************/

float
actfact(unmodsize)
register UNSIGN32 unmodsize;

/***********************************************************************
Compute the actual size factor given the integer approximation unmodsize
= (magnification  factor)*1000.   Values  not found  in  the  table  are
rounded to the nearest table  entry; this ensures that rounding  errors,
or  user  magnification  parameter  input  errors  result  in  something
reasonable.  mag_table[] has a  wider range of magnifications  available
than most sites will have, and can be used to find a nearest match  font
when one is missing.
***********************************************************************/

{
    register INT16 k;
    register UNSIGN32 tab_entry;

    for (k = 1; k < MAGTABSIZE; ++k)
    {
	tab_entry = MAGSIZE(mag_table[k]);	/* round entry */
	if (unmodsize < tab_entry) /* choose nearer of k-1,k entries */
	{
	    if (unmodsize < MAGSIZE((mag_table[k-1] + mag_table[k])/2.0))
	    	k--;			/* choose smaller one */
	    break;
	}
	else if (unmodsize == tab_entry)
	    break;
    }
    mag_index = ((k < MAGTABSIZE) ? k : (MAGTABSIZE-1));/* set global index */
    return((float)mag_table[mag_index]);
}



