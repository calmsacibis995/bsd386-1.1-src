/*
 * July 5, 1991
 * Copyright 1991 Lance Norskog And Sundry Contributors
 * This source code is freely redistributable and may be used for
 * any purpose.  This copyright notice must be maintained. 
 * Lance Norskog And Sundry Contributors are not responsible for 
 * the consequences of using this software.
 */

/*
 * Sound Tools IRCAM SoundFile format handler.
 * 
 * Derived from: Sound Tools skeleton handler file.
 */

#include "st.h"
#include "sfheader.h"

/* Private data for SF file */
typedef struct sfstuff {
	struct sfinfo info;
} *sf_t;

extern int summary, verbose;

/*
 * Do anything required before you start reading samples.
 * Read file header. 
 *	Find out sampling rate, 
 *	size and style of samples, 
 *	mono/stereo/quad.
 */
sfstartread(ft) 
ft_t ft;
{
	sf_t sf = (sf_t) ft->priv;
	int i;
	
	if (fread(&sf->info, 1, sizeof(sf->info), ft->fp) != sizeof(sf->info))
		fail("unexpected EOF in SF header");
	if (ft->swap) sf->info.sf_magic = swapl(sf->info.sf_magic);
	if (ft->swap) sf->info.sf_srate = swapl(sf->info.sf_srate);
	if (ft->swap) sf->info.sf_packmode = swapl(sf->info.sf_packmode);
	if (ft->swap) sf->info.sf_chans = swapl(sf->info.sf_chans);
	if (ft->swap) sf->info.sf_codes = swapl(sf->info.sf_codes);
	if (sf->info.sf_magic != SF_MAGIC)
		if (sf->info.sf_magic == swapl(SF_MAGIC))
			fail("SF %s file: can't read, it is probably byte-swapped");
		else
			fail("SF %s file: can't read, it is not an IRCAM SoundFile");

	/*
	 * If your format specifies or your file header contains
	 * any of the following information. 
	 */
	ft->info.rate = sf->info.sf_srate;
	switch(sf->info.sf_packmode) {
		case SF_SHORT:
			ft->info.size = WORD;
			ft->info.style = SIGN2;
			break;
		case SF_FLOAT:
			ft->info.size = FLOAT;
			ft->info.style = SIGN2;
			break;
		default:
			fail("Soundfile input: unknown format 0x%x\n",
				sf->info.sf_packmode);
	}
	ft->info.channels = sf->info.sf_chans;
	/* Future: Read codes and print as comments. */

	/* Skip all the comments */
	for(i = sizeof(struct sfinfo); i < SIZEOF_BSD_HEADER; i++)
		getc(ft->fp);

}

sfstartwrite(ft) 
ft_t ft;
{
	sf_t sf = (sf_t) ft->priv;
	int i;

	sf->info.sf_magic = SF_MAGIC;
	sf->info.sf_srate = ft->info.rate;
#ifdef	LATER
	/* 
	 * CSound sound-files have many formats. 
	 * We stick with the IRCAM short-or-float scheme.
	 */
	if (ft->info.size == WORD) {
		sf->info.sf_packmode = SF_SHORT;
		ft->info.style = SIGN2;		/* Default to signed words */
	} else if (ft->info.size == FLOAT)
		sf->info.sf_packmode = SF_FLOAT;
	else
		fail("SoundFile %s: must set output as signed shorts or floats",
			ft->filename);
#else
	if (ft->info.size == FLOAT) {
		sf->info.sf_packmode = SF_FLOAT;
		ft->info.size = FLOAT;
	} else {
		sf->info.sf_packmode = SF_SHORT;
		ft->info.size = WORD;
		ft->info.style = SIGN2;		/* Default to signed words */
	}
#endif
	sf->info.sf_chans = ft->info.channels;
	sf->info.sf_codes = SF_END;		/* No comments */

	(void) fwrite(&sf->info, 1, sizeof(sf->info), ft->fp);
	/* Skip all the comments */
	for(i = sizeof(struct sfinfo); i < SIZEOF_BSD_HEADER; i++)
		putc(0, ft->fp);

}

/* Read and write are supplied by raw.c */



