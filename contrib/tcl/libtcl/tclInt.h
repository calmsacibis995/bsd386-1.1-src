/*
 * tclInt.h --
 *
 *	Declarations of things used internally by the Tcl interpreter.
 *
 * Copyright (c) 1987-1993 The Regents of the University of California.
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
 *
 * $Header: /bsdi/MASTER/BSDI_OS/contrib/tcl/libtcl/tclInt.h,v 1.1.1.1 1994/01/03 23:14:43 polk Exp $ SPRITE (Berkeley)
 */

#ifndef _TCLINT
#define _TCLINT

/*
 * Common include files needed by most of the Tcl source files are
 * included here, so that system-dependent personalizations for the
 * include files only have to be made in once place.  This results
 * in a few extra includes, but greater modularity.  The order of
 * the three groups of #includes is important.  For example, stdio.h
 * is needed by tcl.h, and the _ANSI_ARGS_ declaration in tcl.h is
 * needed by stdlib.h in some configurations.
 */

#include <stdio.h>

#ifndef _TCL
#include "tcl.h"
#endif
#ifndef _REGEXP
#include "tclRegexp.h"
#endif

#include <ctype.h>
#ifdef NO_LIMITS_H
#   include "compat/limits.h"
#else
#   include <limits.h>
#endif
#ifdef NO_STDLIB_H
#   include "compat/stdlib.h"
#else
#   include <stdlib.h>
#endif
#ifdef NO_STRING_H
#include "compat/string.h"
#else
#include <string.h>
#endif
#include <varargs.h>

/*
 * At present (12/91) not all stdlib.h implementations declare strtod.
 * The declaration below is here to ensure that it's declared, so that
 * the compiler won't take the default approach of assuming it returns
 * an int.  There's no ANSI prototype for it because there would end
 * up being too many conflicts with slightly-different prototypes.
 */

extern double strtod();

/*
 *----------------------------------------------------------------
 * Data structures related to variables.   These are used primarily
 * in tclVar.c
 *----------------------------------------------------------------
 */

/*
 * The following structure defines a variable trace, which is used to
 * invoke a specific C procedure whenever certain operations are performed
 * on a variable.
 */

typedef struct VarTrace {
    Tcl_VarTraceProc *traceProc;/* Procedure to call when operations given
				 * by flags are performed on variable. */
    ClientData clientData;	/* Argument to pass to proc. */
    int flags;			/* What events the trace procedure is
				 * interested in:  OR-ed combination of
				 * TCL_TRACE_READS, TCL_TRACE_WRITES, and
				 * TCL_TRACE_UNSETS. */
    struct VarTrace *nextPtr;	/* Next in list of traces associated with
				 * a particular variable. */
} VarTrace;

/*
 * When a variable trace is active (i.e. its associated procedure is
 * executing), one of the following structures is linked into a list
 * associated with the variable's interpreter.  The information in
 * the structure is needed in order for Tcl to behave reasonably
 * if traces are deleted while traces are active.
 */

typedef struct ActiveVarTrace {
    struct Var *varPtr;		/* Variable that's being traced. */
    struct ActiveVarTrace *nextPtr;
				/* Next in list of all active variable
				 * traces for the interpreter, or NULL
				 * if no more. */
    VarTrace *nextTracePtr;	/* Next trace to check after current
				 * trace procedure returns;  if this
				 * trace gets deleted, must update pointer
				 * to avoid using free'd memory. */
} ActiveVarTrace;

/*
 * The following structure describes an enumerative search in progress on
 * an array variable;  this are invoked with options to the "array"
 * command.
 */

typedef struct ArraySearch {
    int id;			/* Integer id used to distinguish among
				 * multiple concurrent searches for the
				 * same array. */
    struct Var *varPtr;		/* Pointer to array variable that's being
				 * searched. */
    Tcl_HashSearch search;	/* Info kept by the hash module about
				 * progress through the array. */
    Tcl_HashEntry *nextEntry;	/* Non-null means this is the next element
				 * to be enumerated (it's leftover from
				 * the Tcl_FirstHashEntry call or from
				 * an "array anymore" command).  NULL
				 * means must call Tcl_NextHashEntry
				 * to get value to return. */
    struct ArraySearch *nextPtr;/* Next in list of all active searches
				 * for this variable, or NULL if this is
				 * the last one. */
} ArraySearch;

/*
 * The structure below defines a variable, which associates a string name
 * with a string value.  Pointers to these structures are kept as the
 * values of hash table entries, and the name of each variable is stored
 * in the hash entry.
 */

typedef struct Var {
    int valueLength;		/* Holds the number of non-null bytes
				 * actually occupied by the variable's
				 * current value in value.string (extra
				 * space is sometimes left for expansion).
				 * For array and global variables this is
				 * meaningless. */
    int valueSpace;		/* Total number of bytes of space allocated
				 * at value.string.  0 means there is no
				 * space allocated. */
    union {
	char *string;		/* String value of variable, used for scalar
				 * variables and array elements.  Malloc-ed. */
	Tcl_HashTable *tablePtr;/* For array variables, this points to
				 * information about the hash table used
				 * to implement the associative array. 
				 * Points to malloc-ed data. */
	struct Var *upvarPtr;	/* If this is a global variable being
				 * referred to in a procedure, or a variable
				 * created by "upvar", this field points to
				 * the record for the higher-level variable. */
    } value;
    Tcl_HashEntry *hPtr;	/* Hash table entry that refers to this
				 * variable, or NULL if the variable has
				 * been detached from its hash table (e.g.
				 * an array is deleted, but some of its
				 * elements are still referred to in upvars). */
    int refCount;		/* Counts number of active uses of this
				 * variable, not including its main hash
				 * table entry: 1 for each additional variable
				 * whose upVarPtr points here, 1 for each
				 * nested trace active on variable.  This
				 * record can't be deleted until refCount
				 * becomes 0. */
    VarTrace *tracePtr;		/* First in list of all traces set for this
				 * variable. */
    ArraySearch *searchPtr;	/* First in list of all searches active
				 * for this variable, or NULL if none. */
    int flags;			/* Miscellaneous bits of information about
				 * variable.  See below for definitions. */
} Var;

/*
 * Flag bits for variables:
 *
 * VAR_ARRAY	-		1 means this is an array variable rather
 *				than a scalar variable.
 * VAR_UPVAR - 			1 means this variable just contains a
 *				pointer to another variable that has the
 *				real value.  Variables like this come
 *				about through the "upvar" and "global"
 *				commands.
 * VAR_UNDEFINED -		1 means that the variable is currently
 *				undefined.  Undefined variables usually
 *				go away completely, but if an undefined
 *				variable has a trace on it, or if it is
 *				a global variable being used by a procedure,
 *				then it stays around even when undefined.
 * VAR_TRACE_ACTIVE -		1 means that trace processing is currently
 *				underway for a read or write access, so
 *				new read or write accesses should not cause
 *				trace procedures to be called and the
 *				variable can't be deleted.
 */

#define VAR_ARRAY		1
#define VAR_UPVAR		2
#define VAR_UNDEFINED		4
#define VAR_TRACE_ACTIVE	0x10

/*
 *----------------------------------------------------------------
 * Data structures related to procedures.   These are used primarily
 * in tclProc.c
 *----------------------------------------------------------------
 */

/*
 * The structure below defines an argument to a procedure, which
 * consists of a name and an (optional) default value.
 */

typedef struct Arg {
    struct Arg *nextPtr;	/* Next argument for this procedure,
				 * or NULL if this is the last argument. */
    char *defValue;		/* Pointer to arg's default value, or NULL
				 * if no default value. */
    char name[4];		/* Name of argument starts here.  The name
				 * is followed by space for the default,
				 * if there is one.  The actual size of this
				 * field will be as large as necessary to
				 * hold both name and default value.  THIS
				 * MUST BE THE LAST FIELD IN THE STRUCTURE!! */
} Arg;

/*
 * The structure below defines a command procedure, which consists of
 * a collection of Tcl commands plus information about arguments and
 * variables.
 */

typedef struct Proc {
    struct Interp *iPtr;	/* Interpreter for which this command
				 * is defined. */
    int refCount;		/* Reference count:  1 if still present
				 * in command table plus 1 for each call
				 * to the procedure that is currently
				 * active.  This structure can be freed
				 * when refCount becomes zero. */
    char *command;		/* Command that constitutes the body of
				 * the procedure (dynamically allocated). */
    Arg *argPtr;		/* Pointer to first of procedure's formal
				 * arguments, or NULL if none. */
} Proc;

/*
 * The structure below defines a command trace.  This is used to allow Tcl
 * clients to find out whenever a command is about to be executed.
 */

typedef struct Trace {
    int level;			/* Only trace commands at nesting level
				 * less than or equal to this. */
    Tcl_CmdTraceProc *proc;	/* Procedure to call to trace command. */
    ClientData clientData;	/* Arbitrary value to pass to proc. */
    struct Trace *nextPtr;	/* Next in list of traces for this interp. */
} Trace;

/*
 * The stucture below defines a deletion callback, which is
 * a procedure to invoke just before an interpreter is deleted.
 */

typedef struct DeleteCallback {
    Tcl_InterpDeleteProc *proc;	/* Procedure to call. */
    ClientData clientData;	/* Value to pass to procedure. */
    struct DeleteCallback *nextPtr;
				/* Next in list of callbacks for this
				 * interpreter (or NULL for end of list). */
} DeleteCallback;

/*
 * The structure below defines a frame, which is a procedure invocation.
 * These structures exist only while procedures are being executed, and
 * provide a sort of call stack.
 */

typedef struct CallFrame {
    Tcl_HashTable varTable;	/* Hash table containing all of procedure's
				 * local variables. */
    int level;			/* Level of this procedure, for "uplevel"
				 * purposes (i.e. corresponds to nesting of
				 * callerVarPtr's, not callerPtr's).  1 means
				 * outer-most procedure, 0 means top-level. */
    int argc;			/* This and argv below describe name and
				 * arguments for this procedure invocation. */
    char **argv;		/* Array of arguments. */
    struct CallFrame *callerPtr;
				/* Value of interp->framePtr when this
				 * procedure was invoked (i.e. next in
				 * stack of all active procedures). */
    struct CallFrame *callerVarPtr;
				/* Value of interp->varFramePtr when this
				 * procedure was invoked (i.e. determines
				 * variable scoping within caller;  same
				 * as callerPtr unless an "uplevel" command
				 * or something equivalent was active in
				 * the caller). */
} CallFrame;

/*
 * The structure below defines one history event (a previously-executed
 * command that can be re-executed in whole or in part).
 */

typedef struct {
    char *command;		/* String containing previously-executed
				 * command. */
    int bytesAvl;		/* Total # of bytes available at *event (not
				 * all are necessarily in use now). */
} HistoryEvent;

/*
 *----------------------------------------------------------------
 * Data structures related to history.   These are used primarily
 * in tclHistory.c
 *----------------------------------------------------------------
 */

/*
 * The structure below defines a pending revision to the most recent
 * history event.  Changes are linked together into a list and applied
 * during the next call to Tcl_RecordHistory.  See the comments at the
 * beginning of tclHistory.c for information on revisions.
 */

typedef struct HistoryRev {
    int firstIndex;		/* Index of the first byte to replace in
				 * current history event. */
    int lastIndex;		/* Index of last byte to replace in
				 * current history event. */
    int newSize;		/* Number of bytes in newBytes. */
    char *newBytes;		/* Replacement for the range given by
				 * firstIndex and lastIndex. */
    struct HistoryRev *nextPtr;	/* Next in chain of revisions to apply, or
				 * NULL for end of list. */
} HistoryRev;

/*
 *----------------------------------------------------------------
 * Data structures related to files.  These are used primarily in
 * tclUnixUtil.c and tclUnixAZ.c.
 *----------------------------------------------------------------
 */

/*
 * The data structure below defines an open file (or connection to
 * a process pipeline) as returned by the "open" command.
 */

typedef struct OpenFile {
    FILE *f;			/* Stdio file to use for reading and/or
				 * writing. */
    FILE *f2;			/* Normally NULL.  In the special case of
				 * a command pipeline with pipes for both
				 * input and output, this is a stdio file
				 * to use for writing to the pipeline. */
    int permissions;		/* OR-ed combination of TCL_FILE_READABLE
				 * and TCL_FILE_WRITABLE. */
    int numPids;		/* If this is a connection to a process
				 * pipeline, gives number of processes
				 * in pidPtr array below;  otherwise it
				 * is 0. */
    int *pidPtr;		/* Pointer to malloc-ed array of child
				 * process ids (numPids of them), or NULL
				 * if this isn't a connection to a process
				 * pipeline. */
    int errorId;		/* File id of file that receives error
				 * output from pipeline.  -1 means not
				 * used (i.e. this is a normal file). */
} OpenFile;

/*
 *----------------------------------------------------------------
 * Data structures related to expressions.  These are used only in
 * tclExpr.c.
 *----------------------------------------------------------------
 */

/*
 * The data structure below defines a math function (e.g. sin or hypot)
 * for use in Tcl expressions.
 */

#define MAX_MATH_ARGS 5
typedef struct MathFunc {
    int numArgs;		/* Number of arguments for function. */
    Tcl_ValueType argTypes[MAX_MATH_ARGS];
				/* Acceptable types for each argument. */
    Tcl_MathProc *proc;		/* Procedure that implements this function. */
    ClientData clientData;	/* Additional argument to pass to the function
				 * when invoking it. */
} MathFunc;

/*
 *----------------------------------------------------------------
 * This structure defines an interpreter, which is a collection of
 * commands plus other state information related to interpreting
 * commands, such as variable storage.  Primary responsibility for
 * this data structure is in tclBasic.c, but almost every Tcl
 * source file uses something in here.
 *----------------------------------------------------------------
 */

typedef struct Command {
    Tcl_CmdProc *proc;		/* Procedure to process command. */
    ClientData clientData;	/* Arbitrary value to pass to proc. */
    Tcl_CmdDeleteProc *deleteProc;
				/* Procedure to invoke when deleting
				 * command. */
    ClientData deleteData;	/* Arbitrary value to pass to deleteProc
				 * (usually the same as clientData). */
} Command;

#define CMD_SIZE(nameLength) ((unsigned) sizeof(Command) + nameLength - 3)

typedef struct Interp {

    /*
     * Note:  the first three fields must match exactly the fields in
     * a Tcl_Interp struct (see tcl.h).  If you change one, be sure to
     * change the other.
     */

    char *result;		/* Points to result returned by last
				 * command. */
    Tcl_FreeProc *freeProc;	/* Zero means result is statically allocated.
				 * If non-zero, gives address of procedure
				 * to invoke to free the result.  Must be
				 * freed by Tcl_Eval before executing next
				 * command. */
    int errorLine;		/* When TCL_ERROR is returned, this gives
				 * the line number within the command where
				 * the error occurred (1 means first line). */
    Tcl_HashTable commandTable;	/* Contains all of the commands currently
				 * registered in this interpreter.  Indexed
				 * by strings; values have type (Command *). */
    Tcl_HashTable mathFuncTable;/* Contains all of the math functions currently
				 * defined for the interpreter.  Indexed by
				 * strings (function names);  values have
				 * type (MathFunc *). */

    /*
     * Information related to procedures and variables.  See tclProc.c
     * and tclvar.c for usage.
     */

    Tcl_HashTable globalTable;	/* Contains all global variables for
				 * interpreter. */
    int numLevels;		/* Keeps track of how many nested calls to
				 * Tcl_Eval are in progress for this
				 * interpreter.  It's used to delay deletion
				 * of the table until all Tcl_Eval invocations
				 * are completed. */
    int maxNestingDepth;	/* If numLevels exceeds this value then Tcl
				 * assumes that infinite recursion has
				 * occurred and it generates an error. */
    CallFrame *framePtr;	/* Points to top-most in stack of all nested
				 * procedure invocations.  NULL means there
				 * are no active procedures. */
    CallFrame *varFramePtr;	/* Points to the call frame whose variables
				 * are currently in use (same as framePtr
				 * unless an "uplevel" command is being
				 * executed).  NULL means no procedure is
				 * active or "uplevel 0" is being exec'ed. */
    ActiveVarTrace *activeTracePtr;
				/* First in list of active traces for interp,
				 * or NULL if no active traces. */
    int returnCode;		/* Completion code to return if current
				 * procedure exits with a TCL_RETURN code. */
    char *errorInfo;		/* Value to store in errorInfo if returnCode
				 * is TCL_ERROR.  Malloc'ed, may be NULL */
    char *errorCode;		/* Value to store in errorCode if returnCode
				 * is TCL_ERROR.  Malloc'ed, may be NULL */

    /*
     * Information related to history:
     */

    int numEvents;		/* Number of previously-executed commands
				 * to retain. */
    HistoryEvent *events;	/* Array containing numEvents entries
				 * (dynamically allocated). */
    int curEvent;		/* Index into events of place where current
				 * (or most recent) command is recorded. */
    int curEventNum;		/* Event number associated with the slot
				 * given by curEvent. */
    HistoryRev *revPtr;		/* First in list of pending revisions. */
    char *historyFirst;		/* First char. of current command executed
				 * from history module or NULL if none. */
    int revDisables;		/* 0 means history revision OK;  > 0 gives
				 * a count of number of times revision has
				 * been disabled. */
    char *evalFirst;		/* If TCL_RECORD_BOUNDS flag set, Tcl_Eval
				 * sets this field to point to the first
				 * char. of text from which the current
				 * command came.  Otherwise Tcl_Eval sets
				 * this to NULL. */
    char *evalLast;		/* Similar to evalFirst, except points to
				 * last character of current command. */

    /*
     * Information used by Tcl_AppendResult to keep track of partial
     * results.  See Tcl_AppendResult code for details.
     */

    char *appendResult;		/* Storage space for results generated
				 * by Tcl_AppendResult.  Malloc-ed.  NULL
				 * means not yet allocated. */
    int appendAvl;		/* Total amount of space available at
				 * partialResult. */
    int appendUsed;		/* Number of non-null bytes currently
				 * stored at partialResult. */

    /*
     * A cache of compiled regular expressions.  See TclCompileRegexp
     * in tclUtil.c for details.
     */

#define NUM_REGEXPS 5
    char *patterns[NUM_REGEXPS];/* Strings corresponding to compiled
				 * regular expression patterns.  NULL
				 * means that this slot isn't used.
				 * Malloc-ed. */
    int patLengths[NUM_REGEXPS];/* Number of non-null characters in
				 * corresponding entry in patterns.
				 * -1 means entry isn't used. */
    regexp *regexps[NUM_REGEXPS];
				/* Compiled forms of above strings.  Also
				 * malloc-ed, or NULL if not in use yet. */

    /*
     * Information used by Tcl_PrintDouble:
     */

    char pdFormat[10];		/* Format string used by Tcl_PrintDouble. */
    int pdPrec;			/* Current precision (used to restore the
				 * the tcl_precision variable after a bogus
				 * value has been put into it). */

    /*
     * Miscellaneous information:
     */

    int cmdCount;		/* Total number of times a command procedure
				 * has been called for this interpreter. */
    int noEval;			/* Non-zero means no commands should actually
				 * be executed:  just parse only.  Used in
				 * expressions when the result is already
				 * determined. */
    int evalFlags;		/* Flags to control next call to Tcl_Eval.
				 * Normally zero, but may be set before
				 * calling Tcl_Eval to an OR'ed combination
				 * of TCL_BRACKET_TERM and TCL_RECORD_BOUNDS. */
    char *termPtr;		/* Character just after the last one in
				 * a command.  Set by Tcl_Eval before
				 * returning. */
    char *scriptFile;		/* NULL means there is no nested source
				 * command active;  otherwise this points to
				 * the name of the file being sourced (it's
				 * not malloc-ed:  it points to an argument
				 * to Tcl_EvalFile. */
    int flags;			/* Various flag bits.  See below. */
    Trace *tracePtr;		/* List of traces for this interpreter. */
    DeleteCallback *deleteCallbackPtr;
				/* First in list of callbacks to invoke when
				 * interpreter is deleted. */
    char resultSpace[TCL_RESULT_SIZE+1];
				/* Static space for storing small results. */
} Interp;

/*
 * Flag bits for Interp structures:
 *
 * DELETED:		Non-zero means the interpreter has been deleted:
 *			don't process any more commands for it, and destroy
 *			the structure as soon as all nested invocations of
 *			Tcl_Eval are done.
 * ERR_IN_PROGRESS:	Non-zero means an error unwind is already in progress.
 *			Zero means a command proc has been invoked since last
 *			error occured.
 * ERR_ALREADY_LOGGED:	Non-zero means information has already been logged
 *			in $errorInfo for the current Tcl_Eval instance,
 *			so Tcl_Eval needn't log it (used to implement the
 *			"error message log" command).
 * ERROR_CODE_SET:	Non-zero means that Tcl_SetErrorCode has been
 *			called to record information for the current
 *			error.  Zero means Tcl_Eval must clear the
 *			errorCode variable if an error is returned.
 * EXPR_INITIALIZED:	1 means initialization specific to expressions has
 *			been carried out.
 */

#define DELETED			1
#define ERR_IN_PROGRESS		2
#define ERR_ALREADY_LOGGED	4
#define ERROR_CODE_SET		8
#define EXPR_INITIALIZED	0x10

/*
 * Default value for the pdPrec and pdFormat fields of interpreters:
 */

#define DEFAULT_PD_PREC 6
#define DEFAULT_PD_FORMAT "%g"

/*
 *----------------------------------------------------------------
 * Data structures related to command parsing.   These are used in
 * tclParse.c and its clients.
 *----------------------------------------------------------------
 */

/*
 * The following data structure is used by various parsing procedures
 * to hold information about where to store the results of parsing
 * (e.g. the substituted contents of a quoted argument, or the result
 * of a nested command).  At any given time, the space available
 * for output is fixed, but a procedure may be called to expand the
 * space available if the current space runs out.
 */

typedef struct ParseValue {
    char *buffer;		/* Address of first character in
				 * output buffer. */
    char *next;			/* Place to store next character in
				 * output buffer. */
    char *end;			/* Address of the last usable character
				 * in the buffer. */
    void (*expandProc) _ANSI_ARGS_((struct ParseValue *pvPtr, int needed));
				/* Procedure to call when space runs out;
				 * it will make more space. */
    ClientData clientData;	/* Arbitrary information for use of
				 * expandProc. */
} ParseValue;

/*
 * A table used to classify input characters to assist in parsing
 * Tcl commands.  The table should be indexed with a signed character
 * using the CHAR_TYPE macro.  The character may have a negative
 * value.
 */

extern char tclTypeTable[];
#define CHAR_TYPE(c) (tclTypeTable+128)[c]

/*
 * Possible values returned by CHAR_TYPE:
 *
 * TCL_NORMAL -		All characters that don't have special significance
 *			to the Tcl language.
 * TCL_SPACE -		Character is space, tab, or return.
 * TCL_COMMAND_END -	Character is newline or null or semicolon or
 *			close-bracket.
 * TCL_QUOTE -		Character is a double-quote.
 * TCL_OPEN_BRACKET -	Character is a "[".
 * TCL_OPEN_BRACE -	Character is a "{".
 * TCL_CLOSE_BRACE -	Character is a "}".
 * TCL_BACKSLASH -	Character is a "\".
 * TCL_DOLLAR -		Character is a "$".
 */

#define TCL_NORMAL		0
#define TCL_SPACE		1
#define TCL_COMMAND_END		2
#define TCL_QUOTE		3
#define TCL_OPEN_BRACKET	4
#define TCL_OPEN_BRACE		5
#define TCL_CLOSE_BRACE		6
#define TCL_BACKSLASH		7
#define TCL_DOLLAR		8

/*
 * Additional flags passed to Tcl_Eval.  See tcl.h for other flags to
 * Tcl_Eval;  these ones are only used internally by Tcl.
 *
 * TCL_RECORD_BOUNDS	Tells Tcl_Eval to record information in the
 *			evalFirst and evalLast fields for each command
 *			executed directly from the string (top-level
 *			commands and those from command substitution).
 */

#define TCL_RECORD_BOUNDS	0x100

/*
 * Maximum number of levels of nesting permitted in Tcl commands (used
 * to catch infinite recursion).
 */

#define MAX_NESTING_DEPTH	1000

/*
 * The macro below is used to modify a "char" value (e.g. by casting
 * it to an unsigned character) so that it can be used safely with
 * macros such as isspace.
 */

#define UCHAR(c) ((unsigned char) (c))

/*
 * Given a size or address, the macro below "aligns" it to the machine's
 * memory unit size (e.g. an 8-byte boundary) so that anything can be
 * placed at the aligned address without fear of an alignment error.
 */

#define TCL_ALIGN(x) ((x + 7) & ~7)

/*
 * Variables shared among Tcl modules but not used by the outside
 * world:
 */

extern int		tclNumFiles;
extern OpenFile **	tclOpenFiles;
extern char *		tclRegexpError;

/*
 *----------------------------------------------------------------
 * Procedures shared among Tcl modules but not used by the outside
 * world:
 *----------------------------------------------------------------
 */

extern void		panic();
extern regexp *		TclCompileRegexp _ANSI_ARGS_((Tcl_Interp *interp,
			    char *string));
extern void		TclCopyAndCollapse _ANSI_ARGS_((int count, char *src,
			    char *dst));
extern void		TclDeleteVars _ANSI_ARGS_((Interp *iPtr,
			    Tcl_HashTable *tablePtr));
extern void		TclExpandParseValue _ANSI_ARGS_((ParseValue *pvPtr,
			    int needed));
extern int		TclFindElement _ANSI_ARGS_((Tcl_Interp *interp,
			    char *list, char **elementPtr, char **nextPtr,
			    int *sizePtr, int *bracePtr));
extern Proc *		TclFindProc _ANSI_ARGS_((Interp *iPtr,
			    char *procName));
extern int		TclGetFrame _ANSI_ARGS_((Tcl_Interp *interp,
			    char *string, CallFrame **framePtrPtr));
extern int		TclGetListIndex _ANSI_ARGS_((Tcl_Interp *interp,
			    char *string, int *indexPtr));
extern Proc *		TclIsProc _ANSI_ARGS_((Command *cmdPtr));
extern int		TclParseBraces _ANSI_ARGS_((Tcl_Interp *interp,
			    char *string, char **termPtr, ParseValue *pvPtr));
extern int		TclParseNestedCmd _ANSI_ARGS_((Tcl_Interp *interp,
			    char *string, int flags, char **termPtr,
			    ParseValue *pvPtr));
extern int		TclParseQuotes _ANSI_ARGS_((Tcl_Interp *interp,
			    char *string, int termChar, int flags,
			    char **termPtr, ParseValue *pvPtr));
extern int		TclParseWords _ANSI_ARGS_((Tcl_Interp *interp,
			    char *string, int flags, int maxWords,
			    char **termPtr, int *argcPtr, char **argv,
			    ParseValue *pvPtr));
extern char *		TclPrecTraceProc _ANSI_ARGS_((ClientData clientData,
			    Tcl_Interp *interp, char *name1, char *name2,
			    int flags));
extern void		TclSetupEnv _ANSI_ARGS_((Tcl_Interp *interp));
extern char *		TclWordEnd _ANSI_ARGS_((char *start, int nested,
			    int *semiPtr));

/*
 *----------------------------------------------------------------
 * Command procedures in the generic core:
 *----------------------------------------------------------------
 */

extern int	Tcl_AppendCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_ArrayCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_BreakCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_CaseCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_CatchCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_ConcatCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_ContinueCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_ErrorCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_EvalCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_ExprCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_ForCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_ForeachCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_FormatCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_GlobalCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_HistoryCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_IfCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_IncrCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_InfoCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_JoinCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_LappendCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_LindexCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_LinsertCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_LlengthCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_ListCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_LrangeCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_LreplaceCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_LsearchCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_LsortCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_ProcCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_RegexpCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_RegsubCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_RenameCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_ReturnCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_ScanCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_SetCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_SplitCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_StringCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_SwitchCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_TraceCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_UnsetCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_UplevelCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_UpvarCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_WhileCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_Cmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_Cmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));

/*
 *----------------------------------------------------------------
 * Command procedures in the UNIX core:
 *----------------------------------------------------------------
 */

extern int	Tcl_CdCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_CloseCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_EofCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_ExecCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_ExitCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_FileCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_FlushCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_GetsCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_GlobCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_OpenCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_PutsCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_PidCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_PwdCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_ReadCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_SeekCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_SourceCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_TellCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));
extern int	Tcl_TimeCmd _ANSI_ARGS_((ClientData clientData,
		    Tcl_Interp *interp, int argc, char **argv));

#endif /* _TCLINT */
