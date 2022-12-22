/* -*-C-*- getfntdf.h */
/*-->getfntdf*/
/**********************************************************************/
/****************************** getfntdf ******************************/
/**********************************************************************/

void
getfntdf()

/***********************************************************************
Read the font definitions as they are in the postamble of the DVI  file.
Note that the font directory is not yet loaded.
***********************************************************************/

{
    register BYTE    byte;

    while (((byte = (BYTE)nosignex(dvifp,(BYTE)1)) >= FNT_DEF1)
	&& (byte <= FNT_DEF4))
	readfont ((INT32)nosignex(dvifp,(BYTE)(byte-FNT_DEF1+1)));
    if (byte != POST_POST)
	(void)fatal ("getfntdf():  POST_POST missing after fontdefs");
}


