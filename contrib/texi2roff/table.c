/*
 * table.c - set up translation tables for texi2roff
 *		Release 1.0a	August 1988
 *		Release 2.0	January 1990
 *
 * Copyright 1988, 1989, 1990  Beverly A.Erlebacher
 * erlebach@cs.toronto.edu    ...uunet!utai!erlebach
 *
 *
 * When adding more commands: 
 *
 * - be sure that gettoken() can recognize not just the ending token
 *   (texend) but also the end of the starting token (texstart) for
 *   the command, if it doesnt already occur in a table.
 *
 * - keep the tables sorted
 *
 * - try to keep all troff strings here and in the table header files
 *
 * - strive diligently to keep the program table-driven
 */

#include <stdio.h>	/* just to get NULL */
#include "texi2roff.h"
#include "tablems.h"
#include "tableme.h"
#include "tablemm.h"

char indexmacro[] = ".de iX \n.tm \\\\$1   \\\\n%\n..\n";
char trquotes[] = ".tr \\(is'\n.tr \\(if`\n.tr \\(pd\"\n";

struct misccmds * cmds;
struct tablerecd * table, * endoftable;

void
initialize(macropkg, showInfo, makeindex)
int macropkg;
int showInfo;
int makeindex;
{
    extern void patchtable();
    int tablesize;

    switch (macropkg) {
    case MS:
	table = mstable;
	tablesize = sizeof mstable;
	cmds = &mscmds;
	break;
    case MM:
	table = mmtable;
	tablesize = sizeof mmtable;
	cmds = &mmcmds;
	break;
    case ME:
	table = metable;
	tablesize = sizeof metable;
	cmds = &mecmds;
	break;
    }
    endoftable = table + tablesize/sizeof table[0];
    if (showInfo == NO)
       (void) patchtable();
    if (makeindex == YES)
       puts(indexmacro);
    puts(cmds->init);
    puts(trquotes);
}

 
/*
 * lookup - linear search for texinfo command in table
 * i used bsearch() for a while but it isnt portable and makes lint squawk.
 */

struct tablerecd *
lookup(token)
    char	   *token;
{
    register struct tablerecd *tptr;

    for (tptr = table; tptr < endoftable; ++tptr) {
	if (STREQ(tptr->texstart, token))
	    return tptr;
    }
    return NULL;
}

/*
 * real Texinfo has a sort of hypertext feature called Info files,
 * using menus, nodes and 'ifinfo' sections. Although i can't simulate
 * this here, and the material would not normally be printed by Texinfo,
 * it could be useful to the user searching through a machine-readable
 * manual. If the -I option is not specified, patch the table to discard 
 * rather than display this material.
 */

static void
patchtable()
{
    struct tablerecd *tp;
    char **p;
    static char *discard[] = { "@menu", "@node", "@ifinfo", "@inforef", 0 };

    for (p = discard; *p != NULL; ++p)
	    if ((tp = lookup(*p)) != NULL) 
		tp->type = DISCARD;
}
