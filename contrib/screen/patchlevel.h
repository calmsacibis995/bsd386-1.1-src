/* Copyright (c) 1993
 *      Juergen Weigert (jnweiger@immd4.informatik.uni-erlangen.de)
 *      Michael Schroeder (mlschroe@immd4.informatik.uni-erlangen.de)
 * Copyright (c) 1987 Oliver Laumann
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (see the file COPYING); if not, write to the
 * Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 ****************************************************************
 * $Id: patchlevel.h,v 1.2 1994/01/29 17:20:57 polk Exp $ FAU
 */

/****************************************************************
 *
 * patchlevel.h: Our life story.
 *
 * 08.07.91 -- 3.00.01 -wipe and a 'setenv TERM dumb' bugfix.
 * 17.07.91 -- 3.00.02 another patchlevel by Wayne Davison
 * 31.07.91 -- 3.00.03 E0, S0, C0 for flexible semi-graphics, nonblocking 
 *                     window title input and 'C-a :' command input.
 * 10.08.91 -- 3.00.04 scrollback, markkeys and some bugfixes.
 * 13.08.91 -- 3.00.05 mark routine improved, ansi prototypes added.
 * 20.08.91 -- 3.00.06 screen -h, faster GotoPos in overlay, termcap %.
 *                     instead of %c
 * 28.08.91 -- 3.00.07 environment variable support. security. terminfo.
 *                     pyramid and ultrix support.
 * 07.09.91 -- 3.00.99 secopen(), MIPS support, SVR4 support.
 * 09.09.91 -- 3.01.00 backspace bug fixed.
 * 03.10.91 -- 3.01.01 ansi.c: null-ptr fixed, CLS now saves to scrollback.
 *                     Using setresuid on hpux. Memory leak fixed.
 *		       Better GotoPos(). Support for IC. Another resize bug.
 *                     Detach() w/o fore crashed. -T and -A(dapt) option.
 *                     GNU copyleft.
 * 19.12.91 -- 3.01.02 flow now really automatic (autoflow killed).
 *		       7 bit restriction removed from WriteString().
 * 09.01.92 -- 3.01.03 flow reattach bug fixed. VDISCARD bug fixed.
 * 13.01.92 -- 3.01.04 new flow concept: ^Af toggles now three states
 * 21.01.92 -- 3.01.05 '^A:screen 11' bug fixed. aflag in DoScreen().
 *                     Some code cleanup. attach_tty and display_tty[]
 *                     added.
 * 26.01.92 -- 3.01.06 apollo support, "hardcopy_append on", "bufferfile", 
 *                     SECURITY PROBLEM cleared..
 * 28.01.92 -- 3.01.07 screen after su allowed. Pid became part of 
 *                     SockName. sysvish 14 character restriction considered.
 * 31.01.92 -- 3.02.00 Ultrix port, Irix 3.3 SGI port, shadow pw support,
 *                     data loss on stdin overflow fixed. "refresh off".
 * 12.02.92 -- 3.02.02 stripdev() moved, -S introduced, bufferfile improved,
 *                     ShellProg coredump cleared. SVR4 bugfixes.
 *                     I/O code speedup added.
 * 24.04.92 -- 3.02.03 perfectly stackable overlays. One scrollback per window,
 *                     not per display.
 * 05.05.92 -- 3.02.04 very nasty initialisation bug fixed.
 * 09.05.92 -- 3.02.05 parsing for $:cl: termcap strings and \012 octal notation
 *                     in screenrc file. More structuring. Detached startup
 *                     with 'screen -d -m -S...' bugfixed.	
 * 11.05.92 -- 3.02.06 setreuid() bugs cleared, C-a : setenv added.
 *		       "xn" capability in TERMCAP needed since "am" is there.
 * 25.06.92 -- 3.02.07 The multi display test version. Have merci.
 * 15.07.92 -- 3.02.08 :B8: supports automatic charset switching for 8-bit
 * 26.09.92 -- 3.02.09 Ported to linux. Ignoring bad files in $SCREENDIR
 * 22.10.92 -- 3.02.10 screen.c/ansi.c splitted in several pieces.
 *		       Better ISearch. Cleanup of loadav.c
 * 29.10.92 -- 3.02.11 Key mechanism rewritten. New command names.
 *		       New iscreenrc syntax. 
 * 02.11.92 -- 3.02.12 'bind g copy_reg' and 'bind x ins_reg' as suggested by
 *		       stillson@tsfsrv.mitre.org (Ken Stillson).
 * 03.11.92 -- 3.02.13 Ported to SunOs 4.1.2. Gulp. Some NULL ptrs fixed and
 *		       misc. braindamage fixed.
 * 03.11.92 -- 3.02.14 Argument number checking, AKA fixed.
 * 05.11.92 -- 3.02.15 Memory leaks in Detach() and KillWindow() fixed.
 *                     Lockprg powerdetaches on SIGHUP.
 * 12.11.92 -- 3.02.16 Introduced two new termcaps: "CS" and "CE".
 *		       (Switch cursorkeys in application mode)
 *		       Tim's async output patch.
 *		       Fixed an ugly bug in WriteString().
 *		       New command: 'process'
 * 16.11.92 -- 3.02.17 Nuking unsent tty output is now optional, (toxic 
 *		       ESC radiation). 
 * 30.11.92 -- 3.02.18 Lots of multi display bugs fixed. New layer
 *		       function 'Restore'. MULTIUSER code cleanup.
 *		       Rudimental acls added for multiuser.
 *                     No more error output, when output gives write errors.
 * 02.12.92 -- 3.02.19 BROKEN_PIPE and SOCK_NOT_IN_FS defines added for 
 *                     braindead systems. Bug in recover socket code fixed.
 *                     Can create windows again from shell.
 * 22.12.92 -- 3.02.20 Made a superb configure script. STY and break fixed.
 * 01.02.93 -- 3.02.21 Coredump bug fixed: 8-bit output in background windows.
 *                     Console grabbing somewhat more useable.
 * 23.02.93 -- 3.02.22 Added ^:exec command, but not tested at all.
 * 23.02.93 -- 3.02.23 Added 'hardcopydir' and 'logdir' commands.
 * 11.03.93 -- 3.02.24 Prefixed display and window structure elements.
 *                     Screen now handles autowrapped lines correctly
 *                     in the redisplay and mark function.
 * 19.03.93 -- 3.03.00 Patched for BSD386. pseudos work.
 * 31.03.93 -- 3.03.01 Don't allocate so much empty attr and font lines.
 * 04.04.93 -- 3.03.02 fixed :exec !..| less and :|less, patched BELL_DONE & 
 *                     ^B/^F. Fixed auto_nuke attribute resetting. Better linux 
 *                     configure. ^AW shows '&' when window has other attacher.
 *                     MAXWIN > 10 patch. KEEP_UNDEF in config.h.in, shellaka 
 *                     bug fixed. dec alpha port. Solaris port. 
 * 02.05.93 -- 3.03.03 Configure tweaked for sgi. Update environment with 
 *                     setenv command. Silence on|off, silencewait <sec>, 
 *                     defautonuke commands added. Manual page updated.
 * 13.05.93 -- 3.03.04 exit in newsyntax script, finished _CX_UX port.
 *                     Texinfo page added by Jason Merrill. Much longish debug 
 *                     output removed. Select window by title (or number).
 * 16.06.93 -- 3.04.00 Replaced ^A- by ^A^H to be complementary to ^A SPACE.
 *                     Moved into CVS. Yacc.
 * 28.06.93 -- 3.04.01 Fixed selecting windows with numeric title. Silence 
 *                     now works without nethackoption set.
 * 01.07.93 -- 3.04.02 Implementing real acls.
 * 22.07.93 -- 3.05.00 Fixed SVR4, some multiuser bugs, -- DISTRIBUTED
 * 05.08.93 -- 3.05.01 ${srcdir} feature added. Shellprog bug fixed.
 *                     Motorola reattach bug fixed. Writelock bug fixed.
 *                     Copybuffer moved into struct user. Configure.in
 *                     uglified for Autoconf1.5. Paste may now have an
 *                     argument. Interactive setenv. Right margin bug
 *                     fixed. IRIX 5 patches. -- DISTRIBUTED
 * 13.08.93 -- 3.05.02 ultrix support added. expand_vars removed from
 *                     register function. Paste bug fixed.
 *                     sysmacros.h now included in pty.c on sgis
 *                     Strange hpux hack added for TTYCMP. 
 *                     Zombie feature improved.
 */

#define ORIGIN "FAU"
#define REV 3
#define VERS 5
#define PATCHLEVEL 2
#define DATE "19-Aug-93"
#define STATE "" 
