
/* $Id: curses.h,v 5.1 1992/10/03 22:34:39 syd Exp  */

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
 *******************************************************************************
 * $Log: curses.h,v $
 * Revision 1.2  1994/01/14  00:52:30  cp
 * 2.4.23
 *
 * Revision 5.1  1992/10/03  22:34:39  syd
 * Initial checkin as of 2.4 Release at PL0
 *
 *
 ******************************************************************************/

     /*** Include file for seperate compilation.  ***/

#define OFF		0
#define ON 		1

int  InitScreen(),      /* This must be called before anything else!! */

     ClearScreen(), 	 CleartoEOLN(),

     MoveCursor(),

     StartBold(),        EndBold(), 
     StartUnderline(),   EndUnderline(), 
     StartHalfbright(),  EndHalfbright(),
     StartInverse(),     EndInverse(),
	
     transmit_functions(),

     Raw(),              RawState(),
     ReadCh();

char *return_value_of();
