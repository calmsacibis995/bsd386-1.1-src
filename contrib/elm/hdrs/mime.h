/*******************************************************************************
 *  The Elm Mail System  -  $Revision: 1.2 $   $State: Exp $
 *
 * 			Copyright (c) 1988-1992 USENET Community Trust
 * 			Copyright (c) 1986,1987 Dave Taylor
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Syd Weinstein, Elm Coordinator
 *	elm@DSI.COM			dsinc!elm
 *
 ******************************************************************************
 * $Log: mime.h,v $
 * Revision 1.2  1994/01/14  00:52:42  cp
 * 2.4.23
 *
 * Revision 5.3  1992/11/07  20:50:08  syd
 * add some new names to make header_cmp use easier
 * From: Syd
 *
 * Revision 5.2  1992/10/25  01:47:45  syd
 * fixed a bug were elm didn't call metamail on messages with a characterset,
 * which could be displayed by elm itself, but message is encoded with QP
 * or BASE64
 * From: Klaus Steinberger <Klaus.Steinberger@Physik.Uni-Muenchen.DE>
 *
 * Revision 5.1  1992/10/03  22:34:39  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

#define	MIME_HEADER	"MIME-Version: 1.0"
#define	MIME_HEADER_NAME	"MIME-Version"
#define	MIME_HEADER_VERSION	"1.0"
#define	MIME_OLDVERSION	"MIME-Version: RFCXXXX"
#define	MIME_HEADER_OLDVERSION	"RFCXXXX"
#define	MIME_INCLUDE	"[include"
#define MIME_BOUNDARY	"%#%record%#%"	/* default boundary */
#define	MIME_CONTENTTYPE	"Content-Type:"
#define	MIME_HEADER_CONTENTTYPE	"Content-Type"
#define	MIME_CONTENTENCOD	"Content-Transfer-Encoding:"
#define	MIME_HEADER_CONTENTENCOD	"Content-Transfer-Encoding"

/* Encoding types */

#define	ENCODING_ILLEGAL	-1
#define	ENCODING_NONE	0
#define	ENCODING_7BIT	1
#define	ENCODING_8BIT	2
#define	ENCODING_BINARY	3
#define	ENCODING_QUOTED	4
#define	ENCODING_BASE64	5
#define	ENCODING_EXPERIMENTAL	6

#define	ENC_NAME_7BIT	"7bit"
#define	ENC_NAME_8BIT	"8bit"
#define	ENC_NAME_BINARY	"binary"
#define	ENC_NAME_QUOTED "quoted-printable"
#define	ENC_NAME_BASE64	"base64"

/* default charsets, which are a superset of US-ASCII, so we did not
   have to go out to metamail for us-ascii */

#define COMPAT_CHARSETS "ISO-8859-1 ISO-8859-2 ISO-8859-3 ISO-8859-4 ISO-8859-5 ISO-8859-7 ISO-8859-8 ISO-8859-9"
