#	BSDI $Id: iris.m,v 1.3 1994/01/21 19:17:44 donn Exp $
#
# magic.iris: Magic for mips from an iris4d
#
# Dunno what byte-order munging is needed; all of SGI's *current*
# machines and OSes run in big-endian mode on the MIPS machines,
# as far as I know, but they do have the MIPSEB and MIPSEL stuff
# here....
#
0	short		0x0160		mipseb
>20	short		0407		executable
>20	short		0410		pure
>20	short		0413		demand paged
>8	long		>0		not stripped
>8	long		0		stripped
>22	byte		>0		- version %ld.
>23	byte		>0		\b%ld
0	short		0x0162		mipsel
>20	short		0407		executable
>20	short		0410		pure
>20	short		0413		demand paged
>8	long		>0		not stripped
>8	long		0		stripped
>23	byte		>0		- version %ld.
>22	byte		>0		\b%ld
0	short		0x6001		swapped mipseb
>20	short		03401		executable
>20	short		04001		pure
>20	short		05401		demand paged
>8	long		>0		not stripped
>8	long		0		stripped
>22	byte		>0		- version %ld.
>23	byte		>0		\b%ld
0	short		0x6201		swapped mipsel
>20	short		03401		executable
>20	short		04001		pure
>20	short		05401		demand paged
>8	long		>0		not stripped
>8	long		0		stripped
>22	byte		>0		- version %ld.
>23	byte		>0		\b%ld
0	short		0x180		mipseb ucode
0	short		0x182		mipsel ucode
#
# IRIX core format version 1 (from /usr/include/core.out.h)
0	long		0xdeadadb0	IRIX core dump
>4	long		1		of
>16	string		>\0		'%s'
#
# Archives - This handles archive subtypes
#
0	string		!<arch>\n__________E	MIPS archive
>20	string		U			with mipsucode members
>21	string		L			with mipsel members
>21	string		B			with mipseb members
>19	string		L			and a EL hash table
>19	string		B			and a EB hash table
>22	string		X			-- out of date
