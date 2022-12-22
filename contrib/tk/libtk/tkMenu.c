/* 
 * tkMenu.c --
 *
 *	This module implements menus for the Tk toolkit.  The menus
 *	support normal button entries, plus check buttons, radio
 *	buttons, iconic forms of all of the above, and separator
 *	entries.
 *
 * Copyright (c) 1990-1993 The Regents of the University of California.
 * All rights reserved.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

#ifndef lint
static char rcsid[] = "$Header: /bsdi/MASTER/BSDI_OS/contrib/tk/libtk/tkMenu.c,v 1.1.1.1 1994/01/03 23:15:26 polk Exp $ SPRITE (Berkeley)";
#endif

#include "tkConfig.h"
#include "default.h"
#include "tkInt.h"

/*
 * One of the following data structures is kept for each entry of each
 * menu managed by this file:
 */

typedef struct MenuEntry {
    int type;			/* Type of menu entry;  see below for
				 * valid types. */
    struct Menu *menuPtr;	/* Menu with which this entry is associated. */
    char *label;		/* Main text label displayed in entry (NULL
				 * if no label).  Malloc'ed. */
    int labelLength;		/* Number of non-NULL characters in label. */
    int underline;		/* Index of character to underline. */
    Pixmap bitmap;		/* Bitmap to display in menu entry, or None.
				 * If not None then label is ignored. */
    char *accel;		/* Accelerator string displayed at right
				 * of menu entry.  NULL means no such
				 * accelerator.  Malloc'ed. */
    int accelLength;		/* Number of non-NULL characters in
				 * accelerator. */

    /*
     * Information related to displaying entry:
     */

    Tk_Uid state;		/* State of button for display purposes:
				 * normal, active, or disabled. */
    int height;			/* Number of pixels occupied by entry in
				 * vertical dimension. */
    int y;			/* Y-coordinate of topmost pixel in entry. */
    int selectorDiameter;	/* Size of selector display, in pixels. */
    Tk_3DBorder border;		/* Structure used to draw background for
				 * entry.  NULL means use overall border
				 * for menu. */
    Tk_3DBorder activeBorder;	/* Used to draw background and border when
				 * element is active.  NULL means use
				 * activeBorder from menu. */
    XFontStruct *fontPtr;	/* Text font for menu entries.  NULL means
				 * use overall font for menu. */
    GC textGC;			/* GC for drawing text in entry.  NULL means
				 * use overall textGC for menu. */
    GC activeGC;		/* GC for drawing text in entry when active.
				 * NULL means use overall activeGC for
				 * menu. */
    GC disabledGC;		/* Used to produce disabled effect for entry.
				 * NULL means use overall disabledGC from
				 * menu structure.  See comments for
				 * disabledFg in menu structure for more
				 * information. */

    /*
     * Information used to implement this entry's action:
     */

    char *command;		/* Command to invoke when entry is invoked.
				 * Malloc'ed. */
    char *name;			/* Name of variable (for check buttons and
				 * radio buttons) or menu (for cascade
				 * entries).  Malloc'ed.*/
    char *onValue;		/* Value to store in variable when selected
				 * (only for radio and check buttons).
				 * Malloc'ed. */
    char *offValue;		/* Value to store in variable when not
				 * selected (only for check buttons).
				 * Malloc'ed. */

    /*
     * Miscellaneous information:
     */

    int flags;			/* Various flags.  See below for definitions. */
} MenuEntry;

/*
 * Flag values defined for menu entries:
 *
 * ENTRY_SELECTED:		Non-zero means this is a radio or check
 *				button and that it should be drawn in
 *				the "selected" state.
 * ENTRY_NEEDS_REDISPLAY:	Non-zero means the entry should be redisplayed.
 */

#define ENTRY_SELECTED		1
#define ENTRY_NEEDS_REDISPLAY	4

/*
 * Types defined for MenuEntries:
 */

#define COMMAND_ENTRY		0
#define SEPARATOR_ENTRY		1
#define CHECK_BUTTON_ENTRY	2
#define RADIO_BUTTON_ENTRY	3
#define CASCADE_ENTRY		4

/*
 * Mask bits for above types:
 */

#define COMMAND_MASK		TK_CONFIG_USER_BIT
#define SEPARATOR_MASK		(TK_CONFIG_USER_BIT << 1)
#define CHECK_BUTTON_MASK	(TK_CONFIG_USER_BIT << 2)
#define RADIO_BUTTON_MASK	(TK_CONFIG_USER_BIT << 3)
#define CASCADE_MASK		(TK_CONFIG_USER_BIT << 4)
#define ALL_MASK		(COMMAND_MASK | SEPARATOR_MASK \
	| CHECK_BUTTON_MASK | RADIO_BUTTON_MASK | CASCADE_MASK)

/*
 * Configuration specs for individual menu entries:
 */

static Tk_ConfigSpec entryConfigSpecs[] = {
    {TK_CONFIG_BORDER, "-activebackground", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_ACTIVE_BG, Tk_Offset(MenuEntry, activeBorder),
	COMMAND_MASK|CHECK_BUTTON_MASK|RADIO_BUTTON_MASK|CASCADE_MASK
	|TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-accelerator", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_ACCELERATOR, Tk_Offset(MenuEntry, accel),
	COMMAND_MASK|CHECK_BUTTON_MASK|RADIO_BUTTON_MASK|CASCADE_MASK
	|TK_CONFIG_NULL_OK},
    {TK_CONFIG_BORDER, "-background", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_BG, Tk_Offset(MenuEntry, border),
	COMMAND_MASK|CHECK_BUTTON_MASK|RADIO_BUTTON_MASK|CASCADE_MASK
	|TK_CONFIG_NULL_OK},
    {TK_CONFIG_BITMAP, "-bitmap", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_BITMAP, Tk_Offset(MenuEntry, bitmap),
	COMMAND_MASK|CHECK_BUTTON_MASK|RADIO_BUTTON_MASK|CASCADE_MASK
	|TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-command", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_COMMAND, Tk_Offset(MenuEntry, command),
	COMMAND_MASK|CHECK_BUTTON_MASK|RADIO_BUTTON_MASK|CASCADE_MASK
	|TK_CONFIG_NULL_OK},
    {TK_CONFIG_FONT, "-font", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_FONT, Tk_Offset(MenuEntry, fontPtr),
	COMMAND_MASK|CHECK_BUTTON_MASK|RADIO_BUTTON_MASK|CASCADE_MASK
	|TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-label", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_LABEL, Tk_Offset(MenuEntry, label),
	COMMAND_MASK|CHECK_BUTTON_MASK|RADIO_BUTTON_MASK|CASCADE_MASK},
    {TK_CONFIG_STRING, "-menu", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_MENU, Tk_Offset(MenuEntry, name),
	CASCADE_MASK|TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-offvalue", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_OFF_VALUE, Tk_Offset(MenuEntry, offValue),
	CHECK_BUTTON_MASK},
    {TK_CONFIG_UID, "-state", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_STATE, Tk_Offset(MenuEntry, state),
	COMMAND_MASK|CHECK_BUTTON_MASK|RADIO_BUTTON_MASK|CASCADE_MASK
	|TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_STRING, "-onvalue", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_ON_VALUE, Tk_Offset(MenuEntry, onValue),
	CHECK_BUTTON_MASK},
    {TK_CONFIG_STRING, "-value", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_VALUE, Tk_Offset(MenuEntry, onValue),
	RADIO_BUTTON_MASK|TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-variable", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_CHECK_VARIABLE, Tk_Offset(MenuEntry, name),
	CHECK_BUTTON_MASK|TK_CONFIG_NULL_OK},
    {TK_CONFIG_STRING, "-variable", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_RADIO_VARIABLE, Tk_Offset(MenuEntry, name),
	RADIO_BUTTON_MASK},
    {TK_CONFIG_INT, "-underline", (char *) NULL, (char *) NULL,
	DEF_MENU_ENTRY_UNDERLINE, Tk_Offset(MenuEntry, underline),
	COMMAND_MASK|CHECK_BUTTON_MASK|RADIO_BUTTON_MASK|CASCADE_MASK
	|TK_CONFIG_DONT_SET_DEFAULT},
    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, 0}
};

/*
 * A data structure of the following type is kept for each
 * menu managed by this file:
 */

typedef struct Menu {
    Tk_Window tkwin;		/* Window that embodies the pane.  NULL
				 * means that the window has been destroyed
				 * but the data structures haven't yet been
				 * cleaned up.*/
    Display *display;		/* Display containing widget.  Needed, among
				 * other things, so that resources can be
				 * freed up even after tkwin has gone away. */
    Tcl_Interp *interp;		/* Interpreter associated with menu. */
    MenuEntry **entries;	/* Array of pointers to all the entries
				 * in the menu.  NULL means no entries. */
    int numEntries;		/* Number of elements in entries. */
    int active;			/* Index of active entry.  -1 means
				 * nothing active. */

    /*
     * Information used when displaying widget:
     */

    Tk_3DBorder border;		/* Structure used to draw 3-D
				 * border and background for menu. */
    int borderWidth;		/* Width of border around whole menu. */
    Tk_3DBorder activeBorder;	/* Used to draw background and border for
				 * active element (if any). */
    int activeBorderWidth;	/* Width of border around active element. */
    XFontStruct *fontPtr;	/* Text font for menu entries. */
    XColor *fg;			/* Foreground color for entries. */
    GC textGC;			/* GC for drawing text and other features
				 * of menu entries. */
    XColor *disabledFg;		/* Foreground color when disabled.  NULL
				 * means use normalFg with a 50% stipple
				 * instead. */
    Pixmap gray;		/* Bitmap for drawing disabled entries in
				 * a stippled fashion.  None means not
				 * allocated yet. */
    GC disabledGC;		/* Used to produce disabled effect.  If
				 * disabledFg isn't NULL, this GC is used to
				 * draw text and icons for disabled entries.
				 * Otherwise text and icons are drawn with
				 * normalGC and this GC is used to stipple
				 * background across them. */
    XColor *activeFg;		/* Foreground color for active entry. */
    GC activeGC;		/* GC for drawing active entry. */
    XColor *selectorFg;		/* Color for selectors in radio and check
				 * button entries. */
    GC selectorGC;		/* For drawing selectors. */
    int selectorSpace;		/* Number of pixels to allow for displaying
				 * selectors in menu entries (includes extra
				 * space around selector). */
    int labelWidth;		/* Number of pixels to allow for displaying
				 * labels in menu entries. */

    /*
     * Miscellaneous information:
     */

    Cursor cursor;		/* Current cursor for window, or None. */
    char *postCommand;		/* Command to execute just before posting
				 * this menu, or NULL.  Malloc-ed. */
    MenuEntry *postedCascade;	/* Points to menu entry for cascaded
				 * submenu that is currently posted, or
				 * NULL if no submenu posted. */
    int flags;			/* Various flags;  see below for
				 * definitions. */
} Menu;

/*
 * Flag bits for menus:
 *
 * REDRAW_PENDING:		Non-zero means a DoWhenIdle handler
 *				has already been queued to redraw
 *				this window.
 * RESIZE_PENDING:		Non-zero means a call to ComputeMenuGeometry
 *				has already been scheduled.
 */

#define REDRAW_PENDING		1
#define RESIZE_PENDING		2

/*
 * Configuration specs valid for the menu as a whole:
 */

static Tk_ConfigSpec configSpecs[] = {
    {TK_CONFIG_BORDER, "-activebackground", "activeBackground", "Foreground",
	DEF_MENU_ACTIVE_BG_COLOR, Tk_Offset(Menu, activeBorder),
	TK_CONFIG_COLOR_ONLY},
    {TK_CONFIG_BORDER, "-activebackground", "activeBackground", "Foreground",
	DEF_MENU_ACTIVE_BG_MONO, Tk_Offset(Menu, activeBorder),
	TK_CONFIG_MONO_ONLY},
    {TK_CONFIG_PIXELS, "-activeborderwidth", "activeBorderWidth", "BorderWidth",
	DEF_MENU_ACTIVE_BORDER_WIDTH, Tk_Offset(Menu, activeBorderWidth), 0},
    {TK_CONFIG_COLOR, "-activeforeground", "activeForeground", "Background",
	DEF_MENU_ACTIVE_FG_COLOR, Tk_Offset(Menu, activeFg),
	TK_CONFIG_COLOR_ONLY},
    {TK_CONFIG_COLOR, "-activeforeground", "activeForeground", "Background",
	DEF_MENU_ACTIVE_FG_MONO, Tk_Offset(Menu, activeFg),
	TK_CONFIG_MONO_ONLY},
    {TK_CONFIG_BORDER, "-background", "background", "Background",
	DEF_MENU_BG_COLOR, Tk_Offset(Menu, border), TK_CONFIG_COLOR_ONLY},
    {TK_CONFIG_BORDER, "-background", "background", "Background",
	DEF_MENU_BG_MONO, Tk_Offset(Menu, border), TK_CONFIG_MONO_ONLY},
    {TK_CONFIG_SYNONYM, "-bd", "borderWidth", (char *) NULL,
	(char *) NULL, 0, 0},
    {TK_CONFIG_SYNONYM, "-bg", "background", (char *) NULL,
	(char *) NULL, 0, 0},
    {TK_CONFIG_PIXELS, "-borderwidth", "borderWidth", "BorderWidth",
	DEF_MENU_BORDER_WIDTH, Tk_Offset(Menu, borderWidth), 0},
    {TK_CONFIG_ACTIVE_CURSOR, "-cursor", "cursor", "Cursor",
	DEF_MENU_CURSOR, Tk_Offset(Menu, cursor), TK_CONFIG_NULL_OK},
    {TK_CONFIG_COLOR, "-disabledforeground", "disabledForeground",
	"DisabledForeground", DEF_MENU_DISABLED_FG_COLOR,
	Tk_Offset(Menu, disabledFg), TK_CONFIG_COLOR_ONLY|TK_CONFIG_NULL_OK},
    {TK_CONFIG_COLOR, "-disabledforeground", "disabledForeground",
	"DisabledForeground", DEF_MENU_DISABLED_FG_MONO,
	Tk_Offset(Menu, disabledFg), TK_CONFIG_MONO_ONLY|TK_CONFIG_NULL_OK},
    {TK_CONFIG_SYNONYM, "-fg", "foreground", (char *) NULL,
	(char *) NULL, 0, 0},
    {TK_CONFIG_FONT, "-font", "font", "Font",
	DEF_MENU_FONT, Tk_Offset(Menu, fontPtr), 0},
    {TK_CONFIG_COLOR, "-foreground", "foreground", "Foreground",
	DEF_MENU_FG, Tk_Offset(Menu, fg), 0},
    {TK_CONFIG_STRING, "-postcommand", "postCommand", "Command",
	DEF_MENU_POST_COMMAND, Tk_Offset(Menu, postCommand), TK_CONFIG_NULL_OK},
    {TK_CONFIG_COLOR, "-selector", "selector", "Foreground",
	DEF_MENU_SELECTOR_COLOR, Tk_Offset(Menu, selectorFg),
	TK_CONFIG_COLOR_ONLY},
    {TK_CONFIG_COLOR, "-selector", "selector", "Foreground",
	DEF_MENU_SELECTOR_MONO, Tk_Offset(Menu, selectorFg),
	TK_CONFIG_MONO_ONLY},
    {TK_CONFIG_END, (char *) NULL, (char *) NULL, (char *) NULL,
	(char *) NULL, 0, 0}
};

/*
 * Various geometry definitions:
 */

#define CASCADE_ARROW_HEIGHT 10
#define CASCADE_ARROW_WIDTH 8
#define MARGIN_WIDTH 2

/*
 * Forward declarations for procedures defined later in this file:
 */

static int		ActivateMenuEntry _ANSI_ARGS_((Menu *menuPtr,
			    int index));
static void		ComputeMenuGeometry _ANSI_ARGS_((
			    ClientData clientData));
static int		ConfigureMenu _ANSI_ARGS_((Tcl_Interp *interp,
			    Menu *menuPtr, int argc, char **argv,
			    int flags));
static int		ConfigureMenuEntry _ANSI_ARGS_((Tcl_Interp *interp,
			    Menu *menuPtr, MenuEntry *mePtr, int index,
			    int argc, char **argv, int flags));
static void		DestroyMenu _ANSI_ARGS_((ClientData clientData));
static void		DestroyMenuEntry _ANSI_ARGS_((ClientData clientData));
static void		DisplayMenu _ANSI_ARGS_((ClientData clientData));
static void		EventuallyRedrawMenu _ANSI_ARGS_((Menu *menuPtr,
			    MenuEntry *mePtr));
static int		GetMenuIndex _ANSI_ARGS_((Tcl_Interp *interp,
			    Menu *menuPtr, char *string, int *indexPtr));
static void		MenuEventProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static char *		MenuVarProc _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, char *name1, char *name2,
			    int flags));
static int		MenuWidgetCmd _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, int argc, char **argv));
static int		PostSubmenu _ANSI_ARGS_((Tcl_Interp *interp,
			    Menu *menuPtr, MenuEntry *mePtr));

/*
 *--------------------------------------------------------------
 *
 * Tk_MenuCmd --
 *
 *	This procedure is invoked to process the "menu" Tcl
 *	command.  See the user documentation for details on
 *	what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

int
Tk_MenuCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tk_Window tkwin = (Tk_Window) clientData;
    Tk_Window new;
    register Menu *menuPtr;
    XSetWindowAttributes atts;

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " pathName ?options?\"", (char *) NULL);
	return TCL_ERROR;
    }

    /*
     * Create the new window.  Set override-redirect so the window
     * manager won't add a border or argue about placement, and set
     * save-under so that the window can pop up and down without a
     * lot of re-drawing.
     */

    new = Tk_CreateWindowFromPath(interp, tkwin, argv[1], "");
    if (new == NULL) {
	return TCL_ERROR;
    }
    atts.override_redirect = True;
    atts.save_under = True;
    Tk_ChangeWindowAttributes(new, CWOverrideRedirect|CWSaveUnder, &atts);

    /*
     * Initialize the data structure for the menu.
     */

    menuPtr = (Menu *) ckalloc(sizeof(Menu));
    menuPtr->tkwin = new;
    menuPtr->display = Tk_Display(new);
    menuPtr->interp = interp;
    menuPtr->entries = NULL;
    menuPtr->numEntries = 0;
    menuPtr->active = -1;
    menuPtr->border = NULL;
    menuPtr->borderWidth = 0;
    menuPtr->activeBorder = NULL;
    menuPtr->activeBorderWidth = 0;
    menuPtr->fontPtr = NULL;
    menuPtr->fg = NULL;
    menuPtr->textGC = None;
    menuPtr->disabledFg = NULL;
    menuPtr->gray = None;
    menuPtr->disabledGC = None;
    menuPtr->activeFg = NULL;
    menuPtr->activeGC = None;
    menuPtr->selectorFg = NULL;
    menuPtr->selectorGC = None;
    menuPtr->selectorSpace = 0;
    menuPtr->labelWidth = 0;
    menuPtr->cursor = None;
    menuPtr->postCommand = NULL;
    menuPtr->postedCascade = NULL;
    menuPtr->flags = 0;

    Tk_SetClass(new, "Menu");
    Tk_CreateEventHandler(menuPtr->tkwin, ExposureMask|StructureNotifyMask,
	    MenuEventProc, (ClientData) menuPtr);
    Tcl_CreateCommand(interp, Tk_PathName(menuPtr->tkwin), MenuWidgetCmd,
	    (ClientData) menuPtr, (void (*)()) NULL);
    if (ConfigureMenu(interp, menuPtr, argc-2, argv+2, 0) != TCL_OK) {
	goto error;
    }

    interp->result = Tk_PathName(menuPtr->tkwin);
    return TCL_OK;

    error:
    Tk_DestroyWindow(menuPtr->tkwin);
    return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * MenuWidgetCmd --
 *
 *	This procedure is invoked to process the Tcl command
 *	that corresponds to a widget managed by this module.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

static int
MenuWidgetCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Information about menu widget. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    register Menu *menuPtr = (Menu *) clientData;
    register MenuEntry *mePtr;
    int result = TCL_OK;
    int length, type;
    char c;

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " option ?arg arg ...?\"", (char *) NULL);
	return TCL_ERROR;
    }
    Tk_Preserve((ClientData) menuPtr);
    c = argv[1][0];
    length = strlen(argv[1]);
    if ((c == 'a') && (strncmp(argv[1], "activate", length) == 0)
	    && (length >= 2)) {
	int index;

	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " activate index\"", (char *) NULL);
	    goto error;
	}
	if (GetMenuIndex(interp, menuPtr, argv[2], &index) != TCL_OK) {
	    goto error;
	}
	if (menuPtr->active == index) {
	    goto done;
	}
	if (index >= 0) {
	    if ((menuPtr->entries[index]->type == SEPARATOR_ENTRY)
		    || (menuPtr->entries[index]->state == tkDisabledUid)) {
		index = -1;
	    }
	}
	result = ActivateMenuEntry(menuPtr, index);
    } else if ((c == 'a') && (strncmp(argv[1], "add", length) == 0)
	    && (length >= 2)) {
	MenuEntry **newEntries;

	if (argc < 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " add type ?options?\"", (char *) NULL);
	    goto error;
	}

	/*
	 * Figure out the type of the new entry.
	 */

	c = argv[2][0];
	length = strlen(argv[2]);
	if ((c == 'c') && (strncmp(argv[2], "cascade", length) == 0)
		&& (length >= 2)) {
	    type = CASCADE_ENTRY;
	} else if ((c == 'c') && (strncmp(argv[2], "checkbutton", length) == 0)
		&& (length >= 2)) {
	    type = CHECK_BUTTON_ENTRY;
	} else if ((c == 'c') && (strncmp(argv[2], "command", length) == 0)
		&& (length >= 2)) {
	    type = COMMAND_ENTRY;
	} else if ((c == 'r')
		&& (strncmp(argv[2], "radiobutton", length) == 0)) {
	    type = RADIO_BUTTON_ENTRY;
	} else if ((c == 's')
		&& (strncmp(argv[2], "separator", length) == 0)) {
	    type = SEPARATOR_ENTRY;
	} else {
	    Tcl_AppendResult(interp, "bad menu entry type \"",
		    argv[2], "\":  must be cascade, checkbutton, ",
		    "command, radiobutton, or separator", (char *) NULL);
	    goto error;
	}

	/*
	 * Add a new entry to the end of the menu's array of entries,
	 * and process options for it.  Be sure to initialize even
	 * fields that look like they should be initialized by
	 * Tk_ConfigureWidget, because not all fields are processed
	 * for all kinds of entries, yet they all need to be
	 * initialized.
	 */

	mePtr = (MenuEntry *) ckalloc(sizeof(MenuEntry));
	newEntries = (MenuEntry **) ckalloc((unsigned)
		((menuPtr->numEntries+1)*sizeof(MenuEntry *)));
	if (menuPtr->numEntries != 0) {
	    memcpy((VOID *) newEntries, (VOID *) menuPtr->entries,
		    menuPtr->numEntries*sizeof(MenuEntry *));
	    ckfree((char *) menuPtr->entries);
	}
	menuPtr->entries = newEntries;
	menuPtr->entries[menuPtr->numEntries] = mePtr;
	menuPtr->numEntries++;
	mePtr->type = type;
	mePtr->menuPtr = menuPtr;
	mePtr->label = NULL;
	mePtr->labelLength = 0;
	mePtr->underline = -1;
	mePtr->bitmap = None;
	mePtr->accel = NULL;
	mePtr->accelLength = 0;
	mePtr->state = tkNormalUid;
	mePtr->height = 0;
	mePtr->y = 0;
	mePtr->selectorDiameter = 0;
	mePtr->border = NULL;
	mePtr->activeBorder = NULL;
	mePtr->fontPtr = NULL;
	mePtr->textGC = None;
	mePtr->activeGC = None;
	mePtr->disabledGC = None;
	mePtr->command = NULL;
	mePtr->name = NULL;
	mePtr->onValue = NULL;
	mePtr->offValue = NULL;
	mePtr->flags = 0;
	if (ConfigureMenuEntry(interp, menuPtr, mePtr, menuPtr->numEntries-1,
		argc-3, argv+3, 0) != TCL_OK) {
	    DestroyMenuEntry((ClientData) mePtr);
	    menuPtr->numEntries--;
	    goto error;
	}
    } else if ((c == 'c') && (strncmp(argv[1], "configure", length) == 0)) {
	if (argc == 2) {
	    result = Tk_ConfigureInfo(interp, menuPtr->tkwin, configSpecs,
		    (char *) menuPtr, (char *) NULL, 0);
	} else if (argc == 3) {
	    result = Tk_ConfigureInfo(interp, menuPtr->tkwin, configSpecs,
		    (char *) menuPtr, argv[2], 0);
	} else {
	    result = ConfigureMenu(interp, menuPtr, argc-2, argv+2,
		    TK_CONFIG_ARGV_ONLY);
	}
    } else if ((c == 'd') && (strncmp(argv[1], "delete", length) == 0)
	    && (length >= 2)) {
	int first, last, i, numDeleted;

	if ((argc != 3) && (argc != 4)) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " delete first ?last?\"", (char *) NULL);
	    goto error;
	}
	if (GetMenuIndex(interp, menuPtr, argv[2], &first) != TCL_OK) {
	    goto error;
	}
	if (argc == 3) {
	    last = first;
	} else {
	    if (GetMenuIndex(interp, menuPtr, argv[3], &last) != TCL_OK) {
	        goto error;
	    }
	}
	if ((first < 0) || (last < first)) {
	    goto done;
	}
	numDeleted = last + 1 - first;
	for (i = first; i <= last; i++) {
	    Tk_EventuallyFree((ClientData) menuPtr->entries[i],
			      DestroyMenuEntry);
	}
	for (i = last+1; i < menuPtr->numEntries; i++) {
	    menuPtr->entries[i-numDeleted] = menuPtr->entries[i];
	}
	menuPtr->numEntries -= numDeleted;
	if ((menuPtr->active >= first) && (menuPtr->active <= last)) {
	    menuPtr->active = -1;
	} else if (menuPtr->active > last) {
	    menuPtr->active -= numDeleted;
	}
	if (!(menuPtr->flags & RESIZE_PENDING)) {
	    menuPtr->flags |= RESIZE_PENDING;
	    Tk_DoWhenIdle(ComputeMenuGeometry, (ClientData) menuPtr);
	}
    } else if ((c == 'd') && (strncmp(argv[1], "disable", length) == 0)
	    && (length >= 2)) {
	int index;

	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " disable index\"", (char *) NULL);
	    goto error;
	}
	if (GetMenuIndex(interp, menuPtr, argv[2], &index) != TCL_OK) {
	    goto error;
	}
	if (index < 0) {
	    goto done;
	}
	menuPtr->entries[index]->state = tkDisabledUid;
	if (menuPtr->active == index) {
	    menuPtr->active = -1;
	}
	EventuallyRedrawMenu(menuPtr, menuPtr->entries[index]);
    } else if ((c == 'e') && (length >= 3)
	    && (strncmp(argv[1], "enable", length) == 0)) {
	int index;

	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " enable index\"", (char *) NULL);
	    goto error;
	}
	if (GetMenuIndex(interp, menuPtr, argv[2], &index) != TCL_OK) {
	    goto error;
	}
	if (index < 0) {
	    goto done;
	}
	menuPtr->entries[index]->state = tkNormalUid;
	EventuallyRedrawMenu(menuPtr, menuPtr->entries[index]);
    } else if ((c == 'e') && (length >= 3)
	    && (strncmp(argv[1], "entryconfigure", length) == 0)) {
	int index;

	if (argc < 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " entryconfigure index ?option value ...?\"",
		    (char *) NULL);
	    goto error;
	}
	if (GetMenuIndex(interp, menuPtr, argv[2], &index) != TCL_OK) {
	    goto error;
	}
	if (index < 0) {
	    goto done;
	}
	mePtr = menuPtr->entries[index];
	Tk_Preserve((ClientData) mePtr);
	if (argc == 3) {
	    result = Tk_ConfigureInfo(interp, menuPtr->tkwin, entryConfigSpecs,
		    (char *) mePtr, (char *) NULL,
		    COMMAND_MASK << mePtr->type);
	} else if (argc == 4) {
	    result = Tk_ConfigureInfo(interp, menuPtr->tkwin, entryConfigSpecs,
		    (char *) mePtr, argv[3], COMMAND_MASK << mePtr->type);
	} else {
	    result = ConfigureMenuEntry(interp, menuPtr, mePtr, index, argc-3,
		    argv+3, TK_CONFIG_ARGV_ONLY | COMMAND_MASK << mePtr->type);
	}
	Tk_Release((ClientData) mePtr);
    } else if ((c == 'i') && (strncmp(argv[1], "index", length) == 0)
	    && (length >= 3)) {
	int index;

	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " index string\"", (char *) NULL);
	    goto error;
	}
	if (GetMenuIndex(interp, menuPtr, argv[2], &index) != TCL_OK) {
	    goto error;
	}
	if (index < 0) {
	    interp->result = "none";
	} else {
	    sprintf(interp->result, "%d", index);
	}
    } else if ((c == 'i') && (strncmp(argv[1], "invoke", length) == 0)
	    && (length >= 3)) {
	int index;

	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " invoke index\"", (char *) NULL);
	    goto error;
	}
	if (GetMenuIndex(interp, menuPtr, argv[2], &index) != TCL_OK) {
	    goto error;
	}
	if (index < 0) {
	    goto done;
	}
	mePtr = menuPtr->entries[index];
	if (mePtr->state == tkDisabledUid) {
	    goto done;
	}
	Tk_Preserve((ClientData) mePtr);
	if (mePtr->type == CHECK_BUTTON_ENTRY) {
	    if (mePtr->flags & ENTRY_SELECTED) {
		Tcl_SetVar(interp, mePtr->name, mePtr->offValue,
			TCL_GLOBAL_ONLY);
	    } else {
		Tcl_SetVar(interp, mePtr->name, mePtr->onValue,
			TCL_GLOBAL_ONLY);
	    }
	} else if (mePtr->type == RADIO_BUTTON_ENTRY) {
	    Tcl_SetVar(interp, mePtr->name, mePtr->onValue, TCL_GLOBAL_ONLY);
	}
	if ((mePtr->command != NULL) && (mePtr->type != CASCADE_ENTRY)) {
	    result = TkCopyAndGlobalEval(interp, mePtr->command);
	}
	Tk_Release((ClientData) mePtr);
    } else if ((c == 'p') && (strncmp(argv[1], "post", length) == 0)) {
	int x, y, tmp, vRootX, vRootY;
	unsigned int vRootWidth, vRootHeight;

	if (argc != 4) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " post x y\"", (char *) NULL);
	    goto error;
	}
	if ((Tcl_GetInt(interp, argv[2], &x) != TCL_OK)
		|| (Tcl_GetInt(interp, argv[3], &y) != TCL_OK)) {
	    goto error;
	}

	/*
	 * If there is a command for the menu, execute it.
	 */

	if (menuPtr->postCommand != NULL) {
	    result = TkCopyAndGlobalEval(menuPtr->interp,
		    menuPtr->postCommand);
	    if (result != TCL_OK) {
		return result;
	    }
	}

	/*
	 * Adjust the position of the menu if necessary to keep it
	 * visible on the screen.  There are two special tricks to
	 * make this work right:
	 *
	 * 1. If a virtual root window manager is being used then
	 *    the coordinates are in the virtual root window of
	 *    menuPtr's parent;  since the menu uses override-redirect
	 *    mode it will be in the *real* root window for the screen,
	 *    so we have to map the coordinates from the virtual root
	 *    (if any) to the real root.  Can't get the virtual root
	 *    from the menu itself (it will never be seen by the wm)
	 *    so use its parent instead (it would be better to have an
	 *    an option that names a window to use for this...).
	 * 2. The menu may not have been mapped yet, so its current size
	 *    might be the default 1x1.  To compute how much space it
	 *    needs, use its requested size, not its actual size.
	 */

	Tk_GetVRootGeometry(Tk_Parent(menuPtr->tkwin), &vRootX, &vRootY,
		&vRootWidth, &vRootHeight);
	x += vRootX;
	y += vRootY;
	tmp = WidthOfScreen(Tk_Screen(menuPtr->tkwin))
		- Tk_ReqWidth(menuPtr->tkwin);
	if (x > tmp) {
	    x = tmp;
	}
	if (x < 0) {
	    x = 0;
	}
	tmp = HeightOfScreen(Tk_Screen(menuPtr->tkwin))
		- Tk_ReqHeight(menuPtr->tkwin);
	if (y > tmp) {
	    y = tmp;
	}
	if (y < 0) {
	    y = 0;
	}
	if ((x != Tk_X(menuPtr->tkwin)) || (y != Tk_Y(menuPtr->tkwin))) {
	    Tk_MoveToplevelWindow(menuPtr->tkwin, x, y);
	}
	if (!Tk_IsMapped(menuPtr->tkwin)) {
	    Tk_MapWindow(menuPtr->tkwin);
	}
	XRaiseWindow(menuPtr->display, Tk_WindowId(menuPtr->tkwin));
    } else if ((c == 'u') && (strncmp(argv[1], "unpost", length) == 0)) {
	if (argc != 2) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " unpost\"", (char *) NULL);
	    goto error;
	}
	Tk_UnmapWindow(menuPtr->tkwin);
	if (result == TCL_OK) {
	    result = PostSubmenu(interp, menuPtr, (MenuEntry *) NULL);
	}
    } else if ((c == 'y') && (strncmp(argv[1], "yposition", length) == 0)) {
	int index;

	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " yposition index\"", (char *) NULL);
	    goto error;
	}
	if (GetMenuIndex(interp, menuPtr, argv[2], &index) != TCL_OK) {
	    goto error;
	}
	if (index < 0) {
	    interp->result = "0";
	} else {
	    sprintf(interp->result, "%d", menuPtr->entries[index]->y);
	}
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": must be activate, add, configure, delete, disable, ",
		"enable, entryconfigure, index, invoke, post, ",
		"unpost, or yposition", (char *) NULL);
	goto error;
    }
    done:
    Tk_Release((ClientData) menuPtr);
    return result;

    error:
    Tk_Release((ClientData) menuPtr);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * DestroyMenu --
 *
 *	This procedure is invoked by Tk_EventuallyFree or Tk_Release
 *	to clean up the internal structure of a menu at a safe time
 *	(when no-one is using it anymore).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the menu is freed up.
 *
 *----------------------------------------------------------------------
 */

static void
DestroyMenu(clientData)
    ClientData clientData;	/* Info about menu widget. */
{
    register Menu *menuPtr = (Menu *) clientData;
    int i;

    /*
     * Free up all the stuff that requires special handling, then
     * let Tk_FreeOptions handle all the standard option-related
     * stuff.
     */

    for (i = 0; i < menuPtr->numEntries; i++) {
	DestroyMenuEntry((ClientData) menuPtr->entries[i]);
    }
    if (menuPtr->entries != NULL) {
	ckfree((char *) menuPtr->entries);
    }
    if (menuPtr->textGC != None) {
	Tk_FreeGC(menuPtr->display, menuPtr->textGC);
    }
    if (menuPtr->gray != None) {
	Tk_FreeBitmap(menuPtr->display, menuPtr->gray);
    }
    if (menuPtr->disabledGC != None) {
	Tk_FreeGC(menuPtr->display, menuPtr->disabledGC);
    }
    if (menuPtr->activeGC != None) {
	Tk_FreeGC(menuPtr->display, menuPtr->activeGC);
    }
    if (menuPtr->selectorGC != None) {
	Tk_FreeGC(menuPtr->display, menuPtr->selectorGC);
    }
    Tk_FreeOptions(configSpecs, (char *) menuPtr, menuPtr->display, 0);
    ckfree((char *) menuPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * DestroyMenuEntry --
 *
 *	This procedure is invoked by Tk_EventuallyFree or Tk_Release
 *	to clean up the internal structure of a menu entry at a safe time
 *	(when no-one is using it anymore).
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the menu entry is freed up.
 *
 *----------------------------------------------------------------------
 */

static void
DestroyMenuEntry(clientData)
    ClientData clientData;		/* Pointer to entry to be freed. */
{
    register MenuEntry *mePtr = (MenuEntry *) clientData;
    Menu *menuPtr = mePtr->menuPtr;

    /*
     * Free up all the stuff that requires special handling, then
     * let Tk_FreeOptions handle all the standard option-related
     * stuff.
     */

    if (mePtr->name != NULL) {
	Tcl_UntraceVar(menuPtr->interp, mePtr->name,
		TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		MenuVarProc, (ClientData) mePtr);
    }
    if (menuPtr->postedCascade == mePtr) {
	/*
	 * Ignore errors while unposting the menu, since it's possible
	 * that the menu has already been deleted and the unpost will
	 * generate an error.
	 */

	PostSubmenu(menuPtr->interp, menuPtr, (MenuEntry *) NULL);
    }
    if (mePtr->textGC != None) {
	Tk_FreeGC(menuPtr->display, mePtr->textGC);
    }
    if (mePtr->activeGC != None) {
	Tk_FreeGC(menuPtr->display, mePtr->activeGC);
    }
    if (mePtr->disabledGC != None) {
	Tk_FreeGC(menuPtr->display, mePtr->disabledGC);
    }
    Tk_FreeOptions(entryConfigSpecs, (char *) mePtr, menuPtr->display, 
	    (COMMAND_MASK << mePtr->type));
    ckfree((char *) mePtr);
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureMenu --
 *
 *	This procedure is called to process an argv/argc list, plus
 *	the Tk option database, in order to configure (or
 *	reconfigure) a menu widget.
 *
 * Results:
 *	The return value is a standard Tcl result.  If TCL_ERROR is
 *	returned, then interp->result contains an error message.
 *
 * Side effects:
 *	Configuration information, such as colors, font, etc. get set
 *	for menuPtr;  old resources get freed, if there were any.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigureMenu(interp, menuPtr, argc, argv, flags)
    Tcl_Interp *interp;		/* Used for error reporting. */
    register Menu *menuPtr;	/* Information about widget;  may or may
				 * not already have values for some fields. */
    int argc;			/* Number of valid entries in argv. */
    char **argv;		/* Arguments. */
    int flags;			/* Flags to pass to Tk_ConfigureWidget. */
{
    XGCValues gcValues;
    GC newGC;
    unsigned long mask;
    int i;

    if (Tk_ConfigureWidget(interp, menuPtr->tkwin, configSpecs,
	    argc, argv, (char *) menuPtr, flags) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * A few options need special processing, such as setting the
     * background from a 3-D border, or filling in complicated
     * defaults that couldn't be specified to Tk_ConfigureWidget.
     */

    Tk_SetBackgroundFromBorder(menuPtr->tkwin, menuPtr->border);

    gcValues.font = menuPtr->fontPtr->fid;
    gcValues.foreground = menuPtr->fg->pixel;
    gcValues.background = Tk_3DBorderColor(menuPtr->border)->pixel;
    newGC = Tk_GetGC(menuPtr->tkwin, GCForeground|GCBackground|GCFont,
	    &gcValues);
    if (menuPtr->textGC != None) {
	Tk_FreeGC(menuPtr->display, menuPtr->textGC);
    }
    menuPtr->textGC = newGC;

    if (menuPtr->disabledFg != NULL) {
	gcValues.foreground = menuPtr->disabledFg->pixel;
	mask = GCForeground|GCBackground|GCFont;
    } else {
	gcValues.foreground = gcValues.background;
	if (menuPtr->gray == None) {
	    menuPtr->gray = Tk_GetBitmap(interp, menuPtr->tkwin,
		    Tk_GetUid("gray50"));
	    if (menuPtr->gray == None) {
		return TCL_ERROR;
	    }
	}
	gcValues.fill_style = FillStippled;
	gcValues.stipple = menuPtr->gray;
	mask = GCForeground|GCFillStyle|GCStipple;
    }
    newGC = Tk_GetGC(menuPtr->tkwin, mask, &gcValues);
    if (menuPtr->disabledGC != None) {
	Tk_FreeGC(menuPtr->display, menuPtr->disabledGC);
    }
    menuPtr->disabledGC = newGC;

    gcValues.font = menuPtr->fontPtr->fid;
    gcValues.foreground = menuPtr->activeFg->pixel;
    gcValues.background = Tk_3DBorderColor(menuPtr->activeBorder)->pixel;
    newGC = Tk_GetGC(menuPtr->tkwin, GCForeground|GCBackground|GCFont,
	    &gcValues);
    if (menuPtr->activeGC != None) {
	Tk_FreeGC(menuPtr->display, menuPtr->activeGC);
    }
    menuPtr->activeGC = newGC;

    gcValues.foreground = menuPtr->selectorFg->pixel;
    newGC = Tk_GetGC(menuPtr->tkwin, GCForeground|GCFont, &gcValues);
    if (menuPtr->selectorGC != None) {
	Tk_FreeGC(menuPtr->display, menuPtr->selectorGC);
    }
    menuPtr->selectorGC = newGC;

    /*
     * After reconfiguring a menu, we need to reconfigure all of the
     * entries in the menu, since some of the things in the children
     * (such as graphics contexts) may have to change to reflect changes
     * in the parent.
     */

    for (i = 0; i < menuPtr->numEntries; i++) {
	MenuEntry *mePtr;

	mePtr = menuPtr->entries[i];
	ConfigureMenuEntry(interp, menuPtr, mePtr, i, 0, (char **) NULL,
		TK_CONFIG_ARGV_ONLY | COMMAND_MASK << mePtr->type);
    }

    if (!(menuPtr->flags & RESIZE_PENDING)) {
	menuPtr->flags |= RESIZE_PENDING;
	Tk_DoWhenIdle(ComputeMenuGeometry, (ClientData) menuPtr);
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureMenuEntry --
 *
 *	This procedure is called to process an argv/argc list, plus
 *	the Tk option database, in order to configure (or
 *	reconfigure) one entry in a menu.
 *
 * Results:
 *	The return value is a standard Tcl result.  If TCL_ERROR is
 *	returned, then interp->result contains an error message.
 *
 * Side effects:
 *	Configuration information such as label and accelerator get
 *	set for mePtr;  old resources get freed, if there were any.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigureMenuEntry(interp, menuPtr, mePtr, index, argc, argv, flags)
    Tcl_Interp *interp;			/* Used for error reporting. */
    Menu *menuPtr;			/* Information about whole menu. */
    register MenuEntry *mePtr;		/* Information about menu entry;  may
					 * or may not already have values for
					 * some fields. */
    int index;				/* Index of mePtr within menuPtr's
					 * entries. */
    int argc;				/* Number of valid entries in argv. */
    char **argv;			/* Arguments. */
    int flags;				/* Additional flags to pass to
					 * Tk_ConfigureWidget. */
{
    XGCValues gcValues;
    GC newGC, newActiveGC, newDisabledGC;
    unsigned long mask;

    /*
     * If this entry is a cascade and the cascade is posted, then unpost
     * it before reconfiguring the entry (otherwise the reconfigure might
     * change the name of the cascaded entry, leaving a posted menu
     * high and dry).
     */

    if (menuPtr->postedCascade == mePtr) {
	if (PostSubmenu(menuPtr->interp, menuPtr, (MenuEntry *) NULL)
		!= TCL_OK) {
	    Tk_BackgroundError(menuPtr->interp);
	}
    }

    /*
     * If this entry is a check button or radio button, then remove
     * its old trace procedure.
     */

    if ((mePtr->name != NULL) &&
	    ((mePtr->type == CHECK_BUTTON_ENTRY)
	    || (mePtr->type == RADIO_BUTTON_ENTRY))) {
	Tcl_UntraceVar(menuPtr->interp, mePtr->name,
		TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		MenuVarProc, (ClientData) mePtr);
    }

    if (Tk_ConfigureWidget(interp, menuPtr->tkwin, entryConfigSpecs,
	    argc, argv, (char *) mePtr,
	    flags | (COMMAND_MASK << mePtr->type)) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * The code below handles special configuration stuff not taken
     * care of by Tk_ConfigureWidget, such as special processing for
     * defaults, sizing strings, graphics contexts, etc.
     */

    if (mePtr->label == NULL) {
	mePtr->labelLength = 0;
    } else {
	mePtr->labelLength = strlen(mePtr->label);
    }
    if (mePtr->accel == NULL) {
	mePtr->accelLength = 0;
    } else {
	mePtr->accelLength = strlen(mePtr->accel);
    }

    if (mePtr->state == tkActiveUid) {
	if (index != menuPtr->active) {
	    ActivateMenuEntry(menuPtr, index);
	}
    } else {
	if (index == menuPtr->active) {
	    ActivateMenuEntry(menuPtr, -1);
	}
	if ((mePtr->state != tkNormalUid) && (mePtr->state != tkDisabledUid)) {
	    Tcl_AppendResult(interp, "bad state value \"", mePtr->state,
		    "\":  must be normal, active, or disabled", (char *) NULL);
	    mePtr->state = tkNormalUid;
	    return TCL_ERROR;
	}
    }

    if (mePtr->fontPtr != NULL) {
	gcValues.foreground = menuPtr->fg->pixel;
	gcValues.background = Tk_3DBorderColor(
		(mePtr->border != NULL) ? mePtr->border : menuPtr->border)
		->pixel;
	gcValues.font = mePtr->fontPtr->fid;

	/*
	 * Note: disable GraphicsExpose events;  we know there won't be
	 * obscured areas when copying from an off-screen pixmap to the
	 * screen and this gets rid of unnecessary events.
	 */

	gcValues.graphics_exposures = False;
	newGC = Tk_GetGC(menuPtr->tkwin,
		GCForeground|GCBackground|GCFont|GCGraphicsExposures,
		&gcValues);

	if (menuPtr->disabledFg != NULL) {
	    gcValues.foreground = menuPtr->disabledFg->pixel;
	    mask = GCForeground|GCBackground|GCFont|GCGraphicsExposures;
	} else {
	    gcValues.foreground = gcValues.background;
	    gcValues.fill_style = FillStippled;
	    gcValues.stipple = menuPtr->gray;
	    mask = GCForeground|GCFillStyle|GCStipple;
	}
	newDisabledGC = Tk_GetGC(menuPtr->tkwin, mask, &gcValues);

	gcValues.foreground = menuPtr->activeFg->pixel;
	gcValues.background = Tk_3DBorderColor(
		(mePtr->activeBorder != NULL) ? mePtr->activeBorder
		: menuPtr->activeBorder)->pixel;
	newActiveGC = Tk_GetGC(menuPtr->tkwin,
		GCForeground|GCBackground|GCFont|GCGraphicsExposures,
		&gcValues);
    } else {
	newGC = None;
	newActiveGC = None;
	newDisabledGC = None;
    }
    if (mePtr->textGC != None) {
	    Tk_FreeGC(menuPtr->display, mePtr->textGC);
    }
    mePtr->textGC = newGC;
    if (mePtr->activeGC != None) {
	    Tk_FreeGC(menuPtr->display, mePtr->activeGC);
    }
    mePtr->activeGC = newActiveGC;
    if (mePtr->disabledGC != None) {
	    Tk_FreeGC(menuPtr->display, mePtr->disabledGC);
    }
    mePtr->disabledGC = newDisabledGC;

    if ((mePtr->type == CHECK_BUTTON_ENTRY)
	    || (mePtr->type == RADIO_BUTTON_ENTRY)) {
	char *value;

	if (mePtr->name == NULL) {
	    mePtr->name = ckalloc((unsigned) (strlen(mePtr->label) + 1));
	    strcpy(mePtr->name, mePtr->label);
	}
	if (mePtr->onValue == NULL) {
	    mePtr->onValue = ckalloc((unsigned) (strlen(mePtr->label) + 1));
	    strcpy(mePtr->onValue, mePtr->label);
	}

	/*
	 * Select the entry if the associated variable has the
	 * appropriate value, initialize the variable if it doesn't
	 * exist, then set a trace on the variable to monitor future
	 * changes to its value.
	 */

	value = Tcl_GetVar(interp, mePtr->name, TCL_GLOBAL_ONLY);
	mePtr->flags &= ENTRY_SELECTED;
	if (value != NULL) {
	    if (strcmp(value, mePtr->onValue) == 0) {
		mePtr->flags |= ENTRY_SELECTED;
	    }
	} else {
	    Tcl_SetVar(interp, mePtr->name,
		    (mePtr->type == CHECK_BUTTON_ENTRY) ? mePtr->offValue : "",
		    TCL_GLOBAL_ONLY);
	}
	Tcl_TraceVar(interp, mePtr->name,
		TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		MenuVarProc, (ClientData) mePtr);
    }

    if (!(menuPtr->flags & RESIZE_PENDING)) {
	menuPtr->flags |= RESIZE_PENDING;
	Tk_DoWhenIdle(ComputeMenuGeometry, (ClientData) menuPtr);
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * ComputeMenuGeometry --
 *
 *	This procedure is invoked to recompute the size and
 *	layout of a menu.  It is called as a when-idle handler so
 *	that it only gets done once, even if a group of changes is
 *	made to the menu.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Fields of menu entries are changed to reflect their
 *	current positions, and the size of the menu window
 *	itself may be changed.
 *
 *--------------------------------------------------------------
 */

static void
ComputeMenuGeometry(clientData)
    ClientData clientData;		/* Structure describing menu. */
{
    Menu *menuPtr = (Menu *) clientData;
    register MenuEntry *mePtr;
    XFontStruct *fontPtr;
    int maxLabelWidth, maxSelectorWidth, maxAccelWidth;
    int width, height, selectorSpace;
    int i, y;

    if (menuPtr->tkwin == NULL) {
	return;
    }

    maxLabelWidth = maxSelectorWidth = maxAccelWidth = 0;
    y = menuPtr->borderWidth;

    for (i = 0; i < menuPtr->numEntries; i++) {
	mePtr = menuPtr->entries[i];
	selectorSpace = 0;
	fontPtr = mePtr->fontPtr;
	if (fontPtr == NULL) {
	    fontPtr = menuPtr->fontPtr;
	}

	/*
	 * For each entry, compute the height required by that
	 * particular entry, plus three widths:  the width of the
	 * label, the width to allow for a selector to be displayed
	 * to the left of the label (if any), and the width of the
	 * accelerator to be displayed to the right of the label
	 * (if any).  These sizes depend, of course, on the type
	 * of the entry.
	 */

	if (mePtr->bitmap != None) {
	    unsigned int bitmapWidth, bitmapHeight;

	    Tk_SizeOfBitmap(menuPtr->display, mePtr->bitmap,
		    &bitmapWidth, &bitmapHeight);
	    mePtr->height = bitmapHeight;
	    width = bitmapWidth;
	    if (mePtr->type == CHECK_BUTTON_ENTRY) {
		selectorSpace = (14*mePtr->height)/10;
		mePtr->selectorDiameter = (65*mePtr->height)/100;
	    } else if (mePtr->type == RADIO_BUTTON_ENTRY) {
		selectorSpace = (14*mePtr->height)/10;
		mePtr->selectorDiameter = (75*mePtr->height)/100;
	    }
	} else {
	    mePtr->height = fontPtr->ascent + fontPtr->descent;
	    if (mePtr->label != NULL) {
		(void) TkMeasureChars(fontPtr, mePtr->label,
			mePtr->labelLength, 0, (int) 100000,
			TK_NEWLINES_NOT_SPECIAL, &width);
	    } else {
		width = 0;
	    }
	    if (mePtr->type == CHECK_BUTTON_ENTRY) {
		selectorSpace = mePtr->height;
		mePtr->selectorDiameter = (80*mePtr->height)/100;
	    } else if (mePtr->type == RADIO_BUTTON_ENTRY) {
		selectorSpace = mePtr->height;
		mePtr->selectorDiameter = mePtr->height;
	    }
	}
	mePtr->height += 2*menuPtr->activeBorderWidth + 2;
	if (width > maxLabelWidth) {
	    maxLabelWidth = width;
	}
	if (mePtr->type == CASCADE_ENTRY) {
	    width = 2*CASCADE_ARROW_WIDTH;
	} else if (mePtr->accel != NULL) {
	    (void) TkMeasureChars(fontPtr, mePtr->accel, mePtr->accelLength,
		    0, (int) 100000, TK_NEWLINES_NOT_SPECIAL, &width);
	} else {
	    width = 0;
	}
	if (width > maxAccelWidth) {
	    maxAccelWidth = width;
	}
	if (mePtr->type == SEPARATOR_ENTRY) {
	    mePtr->height = 4*menuPtr->borderWidth;
	}
	if (selectorSpace > maxSelectorWidth) {
	    maxSelectorWidth = selectorSpace;
	}
	mePtr->y = y;
	y += mePtr->height;
    }

    /*
     * Got all the sizes.  Update fields in the menu structure, then
     * resize the window if necessary.  Leave margins on either side
     * of the selector (or just one margin if there is no selector).
     * Leave another margin on the right side of the label, plus yet
     * another margin to the right of the accelerator (if there is one).
     */

    menuPtr->selectorSpace = maxSelectorWidth + MARGIN_WIDTH;
    if (maxSelectorWidth != 0) {
	menuPtr->selectorSpace += MARGIN_WIDTH;
    }
    menuPtr->labelWidth = maxLabelWidth + MARGIN_WIDTH;
    width = menuPtr->selectorSpace + menuPtr->labelWidth + maxAccelWidth
	    + 2*menuPtr->borderWidth + 2*menuPtr->activeBorderWidth;
    if (maxAccelWidth != 0) {
	width += MARGIN_WIDTH;
    }
    height = y + menuPtr->borderWidth;

    /*
     * The X server doesn't like zero dimensions, so round up to at least
     * 1 (a zero-sized menu should never really occur, anyway).
     */

    if (width <= 0) {
	width = 1;
    }
    if (height <= 0) {
	height = 1;
    }
    if ((width != Tk_ReqWidth(menuPtr->tkwin)) ||
	    (height != Tk_ReqHeight(menuPtr->tkwin))) {
	Tk_GeometryRequest(menuPtr->tkwin, width, height);
    } else {
	/*
	 * Must always force a redisplay here if the window is mapped
	 * (even if the size didn't change, something else might have
	 * changed in the menu, such as a label or accelerator).  The
	 * resize will force a redisplay above.
	 */

	EventuallyRedrawMenu(menuPtr, (MenuEntry *) NULL);
    }

    menuPtr->flags &= ~RESIZE_PENDING;
}

/*
 *----------------------------------------------------------------------
 *
 * DisplayMenu --
 *
 *	This procedure is invoked to display a menu widget.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands are output to X to display the menu in its
 *	current mode.
 *
 *----------------------------------------------------------------------
 */

static void
DisplayMenu(clientData)
    ClientData clientData;	/* Information about widget. */
{
    register Menu *menuPtr = (Menu *) clientData;
    register MenuEntry *mePtr;
    register Tk_Window tkwin = menuPtr->tkwin;
    Tk_3DBorder bgBorder, activeBorder;
    XFontStruct *fontPtr;
    int index, baseline;
    GC gc;
    XPoint points[3];

    menuPtr->flags &= ~REDRAW_PENDING;
    if ((menuPtr->tkwin == NULL) || !Tk_IsMapped(tkwin)) {
	return;
    }

    /*
     * Loop through all of the entries, drawing them one at a time.
     */

    for (index = 0; index < menuPtr->numEntries; index++) {
	mePtr = menuPtr->entries[index];
	if (!(mePtr->flags & ENTRY_NEEDS_REDISPLAY)) {
	    continue;
	}
	mePtr->flags &= ~ENTRY_NEEDS_REDISPLAY;

	/*
	 * Background.
	 */

	activeBorder = mePtr->activeBorder;
	if (activeBorder == NULL) {
	    activeBorder = menuPtr->activeBorder;
	}
	if ((mePtr->state == tkActiveUid)
		|| (menuPtr->postedCascade == mePtr))  {
	    bgBorder = activeBorder;
	    Tk_Fill3DRectangle(menuPtr->display, Tk_WindowId(tkwin),
		    bgBorder, menuPtr->borderWidth, mePtr->y,
		    Tk_Width(tkwin) - 2*menuPtr->borderWidth, mePtr->height,
		    menuPtr->activeBorderWidth, TK_RELIEF_RAISED);
	    gc = mePtr->activeGC;
	    if (gc == NULL) {
		gc = menuPtr->activeGC;
	    }
	} else {
	    bgBorder = mePtr->border;
	    if (bgBorder == NULL) {
		bgBorder = menuPtr->border;
	    }
	    Tk_Fill3DRectangle(menuPtr->display, Tk_WindowId(tkwin),
		    bgBorder, menuPtr->borderWidth, mePtr->y,
		    Tk_Width(tkwin) - 2*menuPtr->borderWidth, mePtr->height,
		    0, TK_RELIEF_FLAT);
	    if ((mePtr->state == tkDisabledUid)
		    && (menuPtr->disabledFg != NULL)) {
		gc = mePtr->disabledGC;
		if (gc == NULL) {
		    gc = menuPtr->disabledGC;
		}
	    } else {
		gc = mePtr->textGC;
		if (gc == NULL) {
		    gc = menuPtr->textGC;
		}
	    }
	}

	/*
	 * Draw label or bitmap for entry.
	 */

	fontPtr = mePtr->fontPtr;
	if (fontPtr == NULL) {
	    fontPtr = menuPtr->fontPtr;
	}
	baseline = mePtr->y + (mePtr->height + fontPtr->ascent
		- fontPtr->descent)/2;
	if (mePtr->bitmap != None) {
	    unsigned int width, height;

	    Tk_SizeOfBitmap(menuPtr->display, mePtr->bitmap, &width, &height);
	    XCopyPlane(menuPtr->display, mePtr->bitmap, Tk_WindowId(tkwin),
		    gc, 0, 0, width, height,
		    menuPtr->borderWidth + menuPtr->selectorSpace,
		    (int) (mePtr->y + (mePtr->height - height)/2), 1);
	} else {
	    baseline = mePtr->y + (mePtr->height + fontPtr->ascent
		    - fontPtr->descent)/2;
	    if (mePtr->label != NULL) {
		TkDisplayChars(menuPtr->display, Tk_WindowId(tkwin), gc,
			fontPtr, mePtr->label, mePtr->labelLength,
			menuPtr->borderWidth + menuPtr->selectorSpace,
			baseline, TK_NEWLINES_NOT_SPECIAL);
		if (mePtr->underline >= 0) {
		    TkUnderlineChars(menuPtr->display, Tk_WindowId(tkwin), gc,
			    fontPtr, mePtr->label,
			    menuPtr->borderWidth + menuPtr->selectorSpace,
			    baseline, TK_NEWLINES_NOT_SPECIAL,
			    mePtr->underline, mePtr->underline);
		}
	    }
	}

	/*
	 * Draw accelerator or cascade arrow.
	 */

	if (mePtr->type == CASCADE_ENTRY) {
	    points[0].x = Tk_Width(tkwin) - 2*menuPtr->borderWidth
		    - MARGIN_WIDTH - CASCADE_ARROW_WIDTH;
	    points[0].y = mePtr->y + (mePtr->height - CASCADE_ARROW_HEIGHT)/2;
	    points[1].x = points[0].x;
	    points[1].y = points[0].y + CASCADE_ARROW_HEIGHT;
	    points[2].x = points[0].x + CASCADE_ARROW_WIDTH;
	    points[2].y = points[0].y + CASCADE_ARROW_HEIGHT/2;
	    Tk_Fill3DPolygon(menuPtr->display, Tk_WindowId(tkwin), activeBorder,
		    points, 3, menuPtr->activeBorderWidth,
		    (menuPtr->postedCascade == mePtr) ? TK_RELIEF_SUNKEN
		    : TK_RELIEF_RAISED);
	} else if (mePtr->accel != NULL) {
	    TkDisplayChars(menuPtr->display, Tk_WindowId(tkwin), gc,
		    fontPtr, mePtr->accel, mePtr->accelLength,
		    menuPtr->borderWidth + menuPtr->selectorSpace
		    + menuPtr->labelWidth, baseline, TK_NEWLINES_NOT_SPECIAL);
	}

	/*
	 * Draw check-button selector.
	 */

	if (mePtr->type == CHECK_BUTTON_ENTRY) {
	    int dim, x, y;

	    dim = mePtr->selectorDiameter;
	    x = menuPtr->borderWidth + (menuPtr->selectorSpace - dim)/2;
	    y = mePtr->y + (mePtr->height - dim)/2;
	    Tk_Fill3DRectangle(menuPtr->display, Tk_WindowId(tkwin),
		    menuPtr->border, x, y, dim, dim,
		    menuPtr->activeBorderWidth, TK_RELIEF_SUNKEN);
	    x += menuPtr->activeBorderWidth;
	    y += menuPtr->activeBorderWidth;
	    dim -= 2*menuPtr->activeBorderWidth;
	    if ((dim > 0) && (mePtr->flags & ENTRY_SELECTED)) {
		XFillRectangle(menuPtr->display, Tk_WindowId(tkwin),
			menuPtr->selectorGC, x, y, (unsigned int) dim,
			(unsigned int) dim);
	    }
	}

	/*
	 * Draw radio-button selector.
	 */

	if (mePtr->type == RADIO_BUTTON_ENTRY) {
	    XPoint points[4];
	    int radius;

	    radius = mePtr->selectorDiameter/2;
	    points[0].x = menuPtr->borderWidth
		    + (menuPtr->selectorSpace - mePtr->selectorDiameter)/2;
	    points[0].y = mePtr->y + (mePtr->height)/2;
	    points[1].x = points[0].x + radius;
	    points[1].y = points[0].y + radius;
	    points[2].x = points[1].x + radius;
	    points[2].y = points[0].y;
	    points[3].x = points[1].x;
	    points[3].y = points[0].y - radius;
	    if (mePtr->flags & ENTRY_SELECTED) {
		XFillPolygon(menuPtr->display, Tk_WindowId(tkwin),
			menuPtr->selectorGC, points, 4, Convex,
			CoordModeOrigin);
	    } else {
		Tk_Fill3DPolygon(menuPtr->display, Tk_WindowId(tkwin),
			menuPtr->border, points, 4, menuPtr->activeBorderWidth,
			TK_RELIEF_FLAT);
	    }
	    Tk_Draw3DPolygon(menuPtr->display, Tk_WindowId(tkwin),
		    menuPtr->border, points, 4, menuPtr->activeBorderWidth,
		    TK_RELIEF_SUNKEN);
	}

	/*
	 * Draw separator.
	 */

	if (mePtr->type == SEPARATOR_ENTRY) {
	    XPoint points[2];
	    int margin;

	    margin = (fontPtr->ascent + fontPtr->descent)/2;
	    points[0].x = 2*menuPtr->borderWidth + margin;
	    points[0].y = mePtr->y + mePtr->height/2;
	    points[1].x = Tk_Width(tkwin) - 2*menuPtr->borderWidth - margin;
	    points[1].y = points[0].y;
	    Tk_Draw3DPolygon(menuPtr->display, Tk_WindowId(tkwin),
		    menuPtr->border, points, 2, 1, TK_RELIEF_RAISED);
	}

	/*
	 * If the entry is disabled with a stipple rather than a special
	 * foreground color, generate the stippled effect.
	 */

	if ((mePtr->state == tkDisabledUid) && (menuPtr->disabledFg == NULL)) {
	    XFillRectangle(menuPtr->display, Tk_WindowId(tkwin),
		    menuPtr->disabledGC, menuPtr->borderWidth,
		    mePtr->y,
		    (unsigned) (Tk_Width(tkwin) - 2*menuPtr->borderWidth),
		    (unsigned) mePtr->height);
	}
    }

    Tk_Draw3DRectangle(menuPtr->display, Tk_WindowId(tkwin),
	    menuPtr->border, 0, 0, Tk_Width(tkwin), Tk_Height(tkwin),
	    menuPtr->borderWidth, TK_RELIEF_RAISED);
}

/*
 *--------------------------------------------------------------
 *
 * GetMenuIndex --
 *
 *	Parse a textual index into a menu and return the numerical
 *	index of the indicated entry.
 *
 * Results:
 *	A standard Tcl result.  If all went well, then *indexPtr is
 *	filled in with the entry index corresponding to string
 *	(ranges from -1 to the number of entries in the menu minus
 *	one).  Otherwise an error message is left in interp->result.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static int
GetMenuIndex(interp, menuPtr, string, indexPtr)
    Tcl_Interp *interp;		/* For error messages. */
    Menu *menuPtr;		/* Menu for which the index is being
				 * specified. */
    char *string;		/* Specification of an entry in menu.  See
				 * manual entry for valid .*/
    int *indexPtr;		/* Where to store converted relief. */
{
    int i, y;

    if ((string[0] == 'a') && (strcmp(string, "active") == 0)) {
	*indexPtr = menuPtr->active;
	return TCL_OK;
    }

    if ((string[0] == 'l') && (strcmp(string, "last") == 0)) {
	*indexPtr = menuPtr->numEntries-1;
	return TCL_OK;
    }

    if ((string[0] == 'n') && (strcmp(string, "none") == 0)) {
	*indexPtr = -1;
	return TCL_OK;
    }

    if (string[0] == '@') {
	if (Tcl_GetInt(interp, string+1,  &y) == TCL_OK) {
	    if (y < 0) {
		*indexPtr = -1;
		return TCL_OK;
	    }
	    for (i = 0; i < menuPtr->numEntries; i++) {
		y -= menuPtr->entries[i]->height;
		if (y < 0) {
		    break;
		}
	    }
	    if (i >= menuPtr->numEntries) {
		i = -1;
	    }
	    *indexPtr = i;
	    return TCL_OK;
	} else {
	    Tcl_SetResult(interp, (char *) NULL, TCL_STATIC);
	}
    }

    if (isdigit(UCHAR(string[0]))) {
	if (Tcl_GetInt(interp, string,  &i) == TCL_OK) {
	    if (i >= menuPtr->numEntries) {
		i = menuPtr->numEntries - 1;
	    } else if (i < 0) {
		i = -1;
	    }
	    *indexPtr = i;
	    return TCL_OK;
	}
	Tcl_SetResult(interp, (char *) NULL, TCL_STATIC);
    }

    for (i = 0; i < menuPtr->numEntries; i++) {
	char *label;

	label = menuPtr->entries[i]->label;
	if ((label != NULL)
		&& (Tcl_StringMatch(menuPtr->entries[i]->label, string))) {
	    *indexPtr = i;
	    return TCL_OK;
	}
    }

    Tcl_AppendResult(interp, "bad menu entry index \"",
	    string, "\"", (char *) NULL);
    return TCL_ERROR;
}

/*
 *--------------------------------------------------------------
 *
 * MenuEventProc --
 *
 *	This procedure is invoked by the Tk dispatcher for various
 *	events on menus.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	When the window gets deleted, internal structures get
 *	cleaned up.  When it gets exposed, it is redisplayed.
 *
 *--------------------------------------------------------------
 */

static void
MenuEventProc(clientData, eventPtr)
    ClientData clientData;	/* Information about window. */
    XEvent *eventPtr;		/* Information about event. */
{
    Menu *menuPtr = (Menu *) clientData;
    if ((eventPtr->type == Expose) && (eventPtr->xexpose.count == 0)) {
	EventuallyRedrawMenu(menuPtr, (MenuEntry *) NULL);
    } else if (eventPtr->type == DestroyNotify) {
	Tcl_DeleteCommand(menuPtr->interp, Tk_PathName(menuPtr->tkwin));
	menuPtr->tkwin = NULL;
	if (menuPtr->flags & REDRAW_PENDING) {
	    Tk_CancelIdleCall(DisplayMenu, (ClientData) menuPtr);
	}
	if (menuPtr->flags & RESIZE_PENDING) {
	    Tk_CancelIdleCall(ComputeMenuGeometry, (ClientData) menuPtr);
	}
	Tk_EventuallyFree((ClientData) menuPtr, DestroyMenu);
    }
}

/*
 *--------------------------------------------------------------
 *
 * MenuVarProc --
 *
 *	This procedure is invoked when someone changes the
 *	state variable associated with a radiobutton or checkbutton
 *	menu entry.  The entry's selected state is set to match
 *	the value of the variable.
 *
 * Results:
 *	NULL is always returned.
 *
 * Side effects:
 *	The menu entry may become selected or deselected.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static char *
MenuVarProc(clientData, interp, name1, name2, flags)
    ClientData clientData;	/* Information about menu entry. */
    Tcl_Interp *interp;		/* Interpreter containing variable. */
    char *name1;		/* First part of variable's name. */
    char *name2;		/* Second part of variable's name. */
    int flags;			/* Describes what just happened. */
{
    MenuEntry *mePtr = (MenuEntry *) clientData;
    Menu *menuPtr;
    char *value;

    menuPtr = mePtr->menuPtr;

    /*
     * If the variable is being unset, then re-establish the
     * trace unless the whole interpreter is going away.
     */

    if (flags & TCL_TRACE_UNSETS) {
	mePtr->flags &= ~ENTRY_SELECTED;
	if ((flags & TCL_TRACE_DESTROYED) && !(flags & TCL_INTERP_DESTROYED)) {
	    Tcl_TraceVar2(interp, name1, name2,
		    TCL_GLOBAL_ONLY|TCL_TRACE_WRITES|TCL_TRACE_UNSETS,
		    MenuVarProc, clientData);
	}
	EventuallyRedrawMenu(menuPtr, (MenuEntry *) NULL);
	return (char *) NULL;
    }

    /*
     * Use the value of the variable to update the selected status of
     * the menu entry.
     */

    value = Tcl_GetVar2(interp, name1, name2, flags & TCL_GLOBAL_ONLY);
    if (strcmp(value, mePtr->onValue) == 0) {
	if (mePtr->flags & ENTRY_SELECTED) {
	    return (char *) NULL;
	}
	mePtr->flags |= ENTRY_SELECTED;
    } else if (mePtr->flags & ENTRY_SELECTED) {
	mePtr->flags &= ~ENTRY_SELECTED;
    } else {
	return (char *) NULL;
    }
    EventuallyRedrawMenu(menuPtr, (MenuEntry *) NULL);
    return (char *) NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * EventuallyRedrawMenu --
 *
 *	Arrange for an entry of a menu, or the whole menu, to be
 *	redisplayed at some point in the future.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A when-idle hander is scheduled to do the redisplay, if there
 *	isn't one already scheduled.
 *
 *----------------------------------------------------------------------
 */

static void
EventuallyRedrawMenu(menuPtr, mePtr)
    register Menu *menuPtr;	/* Information about menu to redraw. */
    register MenuEntry *mePtr;	/* Entry to redraw.  NULL means redraw
				 * all the entries in the menu. */
{
    int i;
    if (menuPtr->tkwin == NULL) {
	return;
    }
    if (mePtr != NULL) {
	mePtr->flags |= ENTRY_NEEDS_REDISPLAY;
    } else {
	for (i = 0; i < menuPtr->numEntries; i++) {
	    menuPtr->entries[i]->flags |= ENTRY_NEEDS_REDISPLAY;
	}
    }
    if ((menuPtr->tkwin == NULL) || !Tk_IsMapped(menuPtr->tkwin)
	    || (menuPtr->flags & REDRAW_PENDING)) {
	return;
    }
    Tk_DoWhenIdle(DisplayMenu, (ClientData) menuPtr);
    menuPtr->flags |= REDRAW_PENDING;
}

/*
 *--------------------------------------------------------------
 *
 * PostSubmenu --
 *
 *	This procedure arranges for a particular submenu (i.e. the
 *	menu corresponding to a given cascade entry) to be
 *	posted.
 *
 * Results:
 *	A standard Tcl return result.  Errors may occur in the
 *	Tcl commands generated to post and unpost submenus.
 *
 * Side effects:
 *	If there is already a submenu posted, it is unposted.
 *	The new submenu is then posted.
 *
 *--------------------------------------------------------------
 */

static int
PostSubmenu(interp, menuPtr, mePtr)
    Tcl_Interp *interp;		/* Used for invoking sub-commands and
				 * reporting errors. */
    register Menu *menuPtr;	/* Information about menu as a whole. */
    register MenuEntry *mePtr;	/* Info about submenu that is to be
				 * posted.  NULL means make sure that
				 * no submenu is posted. */
{
    char string[30];
    int result, x, y;

    if (mePtr == menuPtr->postedCascade) {
	return TCL_OK;
    }

    if (menuPtr->postedCascade != NULL) {
	EventuallyRedrawMenu(menuPtr, menuPtr->postedCascade);
	result = Tcl_VarEval(interp, menuPtr->postedCascade->name,
		" unpost", (char *) NULL);
	menuPtr->postedCascade = NULL;
	if (result != TCL_OK) {
	    return result;
	}
    }

    if ((mePtr != NULL) && (mePtr->name != NULL)) {
	Tk_GetRootCoords(menuPtr->tkwin, &x, &y);
	x += Tk_Width(menuPtr->tkwin);
	y += mePtr->y;
	sprintf(string, "%d %d", x, y);
	result = Tcl_VarEval(interp, mePtr->name, " post ", string,
		(char *) NULL);
	if (result != TCL_OK) {
	    return result;
	}
	menuPtr->postedCascade = mePtr;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ActivateMenuEntry --
 *
 *	This procedure is invoked to make a particular menu entry
 *	the active one, deactivating any other entry that might
 *	currently be active.
 *
 * Results:
 *	The return value is a standard Tcl result (errors can occur
 *	while posting and unposting submenus).
 *
 * Side effects:
 *	Menu entries get redisplayed, and the active entry changes.
 *	Submenus may get posted and unposted.
 *
 *----------------------------------------------------------------------
 */

static int
ActivateMenuEntry(menuPtr, index)
    register Menu *menuPtr;		/* Menu in which to activate. */
    int index;				/* Index of entry to activate, or
					 * -1 to deactivate all entries. */
{
    register MenuEntry *mePtr;
    int result = TCL_OK;

    if (menuPtr->active >= 0) {
	mePtr = menuPtr->entries[menuPtr->active];

	/*
	 * Don't change the state unless it's currently active (state
	 * might already have been changed to disabled).
	 */

	if (mePtr->state == tkActiveUid) {
	    mePtr->state = tkNormalUid;
	}
	EventuallyRedrawMenu(menuPtr, menuPtr->entries[menuPtr->active]);
    }
    menuPtr->active = index;
    if (index >= 0) {
	mePtr = menuPtr->entries[index];
	mePtr->state = tkActiveUid;
	EventuallyRedrawMenu(menuPtr, mePtr);
	Tk_Preserve((ClientData) mePtr);
	if (mePtr->type == CASCADE_ENTRY) {
	    if (mePtr->command != NULL) {
		result = TkCopyAndGlobalEval(menuPtr->interp, mePtr->command);
	    }
	    if (result == TCL_OK) {
		result = PostSubmenu(menuPtr->interp, menuPtr, mePtr);
	    }
	} else {
	    result = PostSubmenu(menuPtr->interp, menuPtr, (MenuEntry *) NULL);
	}
	Tk_Release((ClientData) mePtr);
    }
    return result;
}
