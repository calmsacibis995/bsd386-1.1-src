/*-
 * Copyright (c) 1993 Berkeley Software Design, Inc. All rights reserved.
 * The Berkeley Software Design Inc. software License Agreement specifies
 * the terms and conditions for redistribution.
 */
 
/*	BSDI $Id: interactive.c,v 1.3 1993/02/23 18:20:03 polk Exp $	*/

#ifdef __STDC__
#include <unistd.h>
#include <stdlib.h>
#endif

#include <curses.h>
#include "installsw.h"
#include <pwd.h>
#include <ctype.h>

#ifdef CTRL
#undef CTRL
#endif
#define CTRL(x) ((x)&037)

char *progname;

WINDOW *iowin;

int spaceused;

char selchar[]="abcdefghijklmnoprstuvwyzABCDEFGHIJKL";

lookup(x)
	int x;
{
	char *cp;

	for (cp=selchar; *cp; cp++)
		if (*cp == x)
			break;
    	if (*cp)
    		return (cp-selchar);
    	return (-1);
}

/*
 * Run the display screen to query users about which packages
 * they wish to load.
 */
interactive()
{
	char *p;
	int  pkg;

	labelscreen();
	for (;;) {
		char c, reply;
		int sawfalse;	/* used for fancy toggling */
		int i;

		showpkgs();
		leaveok(stdscr, 0);
		move(LINES - 1, 0);	/* lower left corner */
		refresh();
		c = getch();
		clearline(WARNLINE);

		/* toggle single item: */
		pkg = lookup(c);
		if (pkg >= 0) {
			if (pkg >= numpkgs) {
				warn("Package `%c' is not available", c);
				printf("");
				fflush(stdout);
				continue;
			}
			if (pkgs[pkg].present && pkgs[pkg].selected == 0) {
				reply = confirm("Package `%c' may already be loaded -- Load anyway?", c);
				if (reply != 'y' && reply != 'Y')
					continue;
			}
			pkgs[pkg].selected = 1 - pkgs[pkg].selected;
			continue;
		}
		switch (c) {
		/* refresh: */
		case '\014':
			clearok(stdscr, 1);
			continue;

		/* required: */
		case '1':
			sawfalse = 0;
			for (i = 0; i < numpkgs; i++) {
				if (pkgs[i].present)
					continue;
				if (pkgs[i].pref != P_REQUIRED)
					continue;
				if (pkgs[i].selected == 0) {
					sawfalse = 1;
					break;
				}
			}
			/* set them all to `sawfalse' */
			for (i = 0; i < numpkgs; i++) {
				if (sawfalse && pkgs[i].present)
					continue;
				if (pkgs[i].pref != P_REQUIRED)
					continue;
				pkgs[i].selected = sawfalse;
			}
			break;
		/* desired: */
		case '2':
			sawfalse = 0;
			for (i = 0; i < numpkgs; i++) {
				if (pkgs[i].present)
					continue;
				if (pkgs[i].pref != P_DESIRABLE)
					continue;
				if (pkgs[i].selected == 0) {
					sawfalse = 1;
					break;
				}
			}
			/* set them all to `sawfalse' */
			for (i = 0; i < numpkgs; i++) {
				if (sawfalse && pkgs[i].present)
					continue;
				if (pkgs[i].pref != P_DESIRABLE)
					continue;
				pkgs[i].selected = sawfalse;
			}
			break;
		/* optional: */
		case '3':
			sawfalse = 0;
			for (i = 0; i < numpkgs; i++) {
				if (pkgs[i].present)
					continue;
				if (pkgs[i].pref != P_OPTIONAL)
					continue;
				if (pkgs[i].selected == 0) {
					sawfalse = 1;
					break;
				}
			}
			/* set them all to `sawfalse' */
			for (i = 0; i < numpkgs; i++) {
				if (sawfalse && pkgs[i].present)
					continue;
				if (pkgs[i].pref != P_OPTIONAL)
					continue;
				pkgs[i].selected = sawfalse;
			}
			break;
		case '4':	/* all: */
			sawfalse = 0;
			for (i = 0; i < numpkgs; i++) {
				if (pkgs[i].present)
					continue;
				if (pkgs[i].selected == 0) {
					sawfalse = 1;
					break;
				}
			}
			/* set them all to `sawfalse' */
			for (i = 0; i < numpkgs; i++) {
				if (sawfalse && pkgs[i].present)
					continue;
				pkgs[i].selected = sawfalse;
			}
			break;
		case 'Q':	/* quit: */
			return;
			break;
		case 'X':
			cleanup();
			exit(0);
		default:
			if (c == '\n')
				c = ' ';
			warn("Invalid command character: `%c'", c);
			break;
		}
	}
}

clearline(line)
{
	move(line, 0);
	clrtoeol();
}


labelscreen()
{
	int i;

	erase ();
/*
	for (i = 0; i < COMMAND1; i++)
		mvprintw(i, 39, "|");
*/
	mvprintw(COMMAND0, 0, "*=selected -=already loaded     R=Required  D=Desired  space=optional");
	mvprintw(COMMAND1, 0, "Letter:  Toggle package                               +-----------------------+");
	mvprintw(COMMAND2, 0, "     1:  Toggle `required'  3:  Toggle `optional'     | Space req'd: ######KB |");
	mvprintw(COMMAND3, 0, "     2:  Toggle `desired'   4:  Toggle all            +-----------------------+");
	mvprintw(COMMAND4, 0, "     Q:  Done (Install selected packages)  X:  Exit (without installing)");
	clearline(WARNLINE);
}

/*
 * Display packages with a few of their attrributes
 */
showpkgs()
{
	int row, col;
	int i;
	char letter;

	row = 0;
	col = 0;
	spaceused = 0;
	for (i = 0; i < numpkgs; i++, row++) {
		letter = selchar[i];
		if (col == 0 && i >= COMMAND0 - 1) {
			row = 0;
			col += 40;
		}
		move(row, col);
		printw("%c", letter);
		if (pkgs[i].selected) {
			printw("*");
			spaceused += pkgs[i].size;
		}
		else if (pkgs[i].present)
			printw("-");
		else
			printw(" ");
		if (pkgs[i].pref == P_REQUIRED)
			printw("R");
		else if (pkgs[i].pref == P_DESIRABLE)
			printw("D");
		else if (pkgs[i].pref == P_OPTIONAL)
			printw(" ");
		else
			printw("?");
		printw(" %6dK ", pkgs[i].size);
		printw("%s", pkgs[i].desc);
	}
	mvprintw(COMMAND2, 69, "%6d", spaceused);
}
