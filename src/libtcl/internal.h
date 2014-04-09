/*
 * tclInt.h --
 *
 *	Declarations of things used internally by the Tcl interpreter.
 *
 * Copyright 1987-1991 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

/*
 * Common include files needed by most of the Tcl source files are
 * included here, so that system-dependent personalizations for the
 * include files only have to be made in once place.
 */
#ifdef CROSS
#   include </usr/include/stdio.h>
#   include </usr/include/ctype.h>
#else
#   include <stdio.h>
#   include <ctype.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <tcl/tcl.h>
#include "hash.h"

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
    void *clientData;		/* Argument to pass to proc. */
    unsigned char flags;	/* What events the trace procedure is
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
    unsigned short id;		/* Integer id used to distinguish among
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
    unsigned short valueLength;	/* Holds the number of non-null bytes
				 * actually occupied by the variable's
				 * current value in value.string (extra
				 * space is sometimes left for expansion).
				 * For array and global variables this is
				 * meaningless. */
    unsigned short valueSpace;	/* Total number of bytes of space allocated
				 * at value. */
    unsigned short upvarUses;	/* Counts number of times variable is
				 * is referenced via global or upvar variables
				 * (i.e. how many variables have "upvarPtr"
				 * pointing to this variable).  Variable
				 * can't be deleted until this count reaches
				 * 0. */
    VarTrace *tracePtr;		/* First in list of all traces set for this
				 * variable. */
    ArraySearch *searchPtr;	/* First in list of all searches active
				 * for this variable, or NULL if none. */
    unsigned char flags;	/* Miscellaneous bits of information about
				 * variable.  See below for definitions. */
    union {
	unsigned char string[4]; /* String value of variable.  The actual
				 * length of this field is given by the
				 * valueSpace field above. */
	Tcl_HashTable *tablePtr;/* For array variables, this points to
				 * information about the hash table used
				 * to implement the associative array.
				 * Points to malloc-ed data. */
	Tcl_HashEntry *upvarPtr;
				/* If this is a global variable being
				 * referred to in a procedure, or a variable
				 * created by "upvar", this field points to
				 * the hash table entry for the higher-level
				 * variable. */
    } value;			/* MUST BE LAST FIELD IN STRUCTURE!!! */
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
 * VAR_ELEMENT_ACTIVE -		Used only in array variables;  1 means that
 *				an element of the array is currently being
 *				manipulated in some way, so that it isn't
 *				safe to delete the whole array.
 * VAR_TRACE_ACTIVE -		1 means that trace processing is currently
 *				underway for a read or write access, so
 *				new read or write accesses should not cause
 *				trace procedures to be called and the
 *				variable can't be deleted.
 */
#define VAR_ARRAY		1
#define VAR_UPVAR		2
#define VAR_UNDEFINED		4
#define VAR_ELEMENT_ACTIVE	0x10
#define VAR_TRACE_ACTIVE	0x20
#define VAR_SEARCHES_POSSIBLE	0x40

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
    unsigned char *defValue;	/* Pointer to arg's default value, or NULL
				 * if no default value. */
    unsigned char name[4];	/* Name of argument starts here.  The name
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
    unsigned char *command;	/* Command that constitutes the body of
				 * the procedure (dynamically allocated). */
    Arg *argPtr;		/* Pointer to first of procedure's formal
				 * arguments, or NULL if none. */
} Proc;

/*
 * The structure below defines a command trace.  This is used to allow Tcl
 * clients to find out whenever a command is about to be executed.
 */
typedef struct Trace {
    unsigned short level;	/* Only trace commands at nesting level
				 * less than or equal to this. */
    Tcl_CmdTraceProc *proc;	/* Procedure to call to trace command. */
    void *clientData;		/* Arbitrary value to pass to proc. */
    struct Trace *nextPtr;	/* Next in list of traces for this interp. */
} Trace;

/*
 * The structure below defines a frame, which is a procedure invocation.
 * These structures exist only while procedures are being executed, and
 * provide a sort of call stack.
 */
typedef struct CallFrame {
    Tcl_HashTable varTable;	/* Hash table containing all of procedure's
				 * local variables. */
    unsigned short level;	/* Level of this procedure, for "uplevel"
				 * purposes (i.e. corresponds to nesting of
				 * callerVarPtr's, not callerPtr's).  1 means
				 * outer-most procedure, 0 means top-level. */
    int argc;			/* This and argv below describe name and
				 * arguments for this procedure invocation. */
    unsigned char **argv;	/* Array of arguments. */
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
    unsigned char *command;	/* String containing previously-executed
				 * command. */
    unsigned short bytesAvl;	/* Total # of bytes available at *event (not
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
    unsigned short firstIndex;	/* Index of the first byte to replace in
				 * current history event. */
    unsigned short lastIndex;	/* Index of last byte to replace in
				 * current history event. */
    unsigned short newSize;	/* Number of bytes in newBytes. */
    unsigned char *newBytes;	/* Replacement for the range given by
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
    int readable;		/* Non-zero means file may be read. */
    int writable;		/* Non-zero means file may be written. */
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
 * This structure defines an interpreter, which is a collection of
 * commands plus other state information related to interpreting
 * commands, such as variable storage.  Primary responsibility for
 * this data structure is in tclBasic.c, but almost every Tcl
 * source file uses something in here.
 *----------------------------------------------------------------
 */
typedef struct Command {
    Tcl_CmdProc *proc;		/* Procedure to process command. */
    void *clientData;		/* Arbitrary value to pass to proc. */
    Tcl_CmdDeleteProc *deleteProc;
				/* Procedure to invoke when deleting
				 * command. */
} Command;

#define CMD_SIZE(nameLength) ((unsigned) sizeof(Command) + nameLength - 3)

typedef struct Interp {

    /*
     * Note:  the first four fields must match exactly the fields in
     * a Tcl_Interp struct (see tcl.h).  If you change one, be sure to
     * change the other.
     */

    unsigned char *result;	/* Points to result returned by last
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

    /*
     * Information related to procedures and variables.  See tclProc.c
     * and tclvar.c for usage.
     */

    Tcl_HashTable globalTable;	/* Contains all global variables for
				 * interpreter. */
    unsigned short numLevels;	/* Keeps track of how many nested calls to
				 * Tcl_Eval are in progress for this
				 * interpreter.  It's used to delay deletion
				 * of the table until all Tcl_Eval invocations
				 * are completed. */
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

    /*
     * Information related to history:
     */
    unsigned short numEvents;	/* Number of previously-executed commands
				 * to retain. */
    HistoryEvent *events;	/* Array containing numEvents entries
				 * (dynamically allocated). */
    unsigned short curEvent;	/* Index into events of place where current
				 * (or most recent) command is recorded. */
    unsigned short curEventNum;	/* Event number associated with the slot
				 * given by curEvent. */
    HistoryRev *revPtr;		/* First in list of pending revisions. */
    unsigned char *historyFirst; /* First char. of current command executed
				 * from history module or NULL if none. */
    unsigned short revDisables;	/* 0 means history revision OK;  > 0 gives
				 * a count of number of times revision has
				 * been disabled. */
    unsigned char *evalFirst;	/* If TCL_RECORD_BOUNDS flag set, Tcl_Eval
				 * sets this field to point to the first
				 * char. of text from which the current
				 * command came.  Otherwise Tcl_Eval sets
				 * this to NULL. */
    unsigned char *evalLast;	/* Similar to evalFirst, except points to
				 * last character of current command. */

    /*
     * Information used by Tcl_AppendResult to keep track of partial
     * results.  See Tcl_AppendResult code for details.
     */
    unsigned char *appendResult; /* Storage space for results generated
				 * by Tcl_AppendResult.  Malloc-ed.  NULL
				 * means not yet allocated. */
    unsigned short appendAvl;	/* Total amount of space available at
				 * partialResult. */
    unsigned short appendUsed;	/* Number of non-null bytes currently
				 * stored at partialResult. */

    /*
     * Information related to files.  See tclUnixAZ.c and tclUnixUtil.c
     * for details.
     */
    unsigned short numFiles;	/* Number of entries in filePtrArray
				 * below.  0 means array hasn't been
				 * created yet. */
    OpenFile **filePtrArray;	/* Pointer to malloc-ed array of pointers
				 * to information about open files.  Entry
				 * N corresponds to the file with fileno N.
				 * If an entry is NULL then the corresponding
				 * file isn't open.  If filePtrArray is NULL
				 * it means no files have been used, so even
				 * stdin/stdout/stderr entries haven't been
				 * setup yet. */
    /*
     * A cache of compiled regular expressions.  See TclCompileRegexp
     * in tclUtil.c for details.
     */
#define NUM_REGEXPS 5
    unsigned char *patterns [NUM_REGEXPS];
				/* Strings corresponding to compiled
				 * regular expression patterns.  NULL
				 * means that this slot isn't used.
				 * Malloc-ed. */
    unsigned short patLengths [NUM_REGEXPS];
				/* Number of non-null characters in
				 * corresponding entry in patterns.
				 * -1 means entry isn't used. */
    struct _regexp_t *regexps [NUM_REGEXPS];
				/* Compiled forms of above strings.  Also
				 * malloc-ed, or NULL if not in use yet. */


    /*
     * Miscellaneous information:
     */
    unsigned long cmdCount;	/* Total number of times a command procedure
				 * has been called for this interpreter. */
    unsigned char *scriptFile;	/* NULL means there is no nested source
				 * command active;  otherwise this points to
				 * the name of the file being sourced (it's
				 * not malloc-ed:  it points to an argument
				 * to Tcl_EvalFile. */
    unsigned char noEval;	/* Non-zero means no commands should actually
				 * be executed:  just parse only.  Used in
				 * expressions when the result is already
				 * determined. */
    unsigned char flags;	/* Various flag bits.  See below. */
    Trace *tracePtr;		/* List of traces for this interpreter. */
    unsigned char resultSpace [TCL_RESULT_SIZE+1];
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
 */
#define DELETED			1
#define ERR_IN_PROGRESS		2
#define ERR_ALREADY_LOGGED	4
#define ERROR_CODE_SET		8

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
    unsigned char *buffer;	/* Address of first character in
				 * output buffer. */
    unsigned char *next;	/* Place to store next character in
				 * output buffer. */
    unsigned char *end;		/* Address of the last usable character
				 * in the buffer. */
    void (*expandProc) (struct ParseValue *pvPtr, unsigned short needed);
				/* Procedure to call when space runs out;
				 * it will make more space. */
    void *clientData;		/* Arbitrary information for use of
				 * expandProc. */
} ParseValue;

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
 * Maximum number of levels of nesting permitted in Tcl commands.
 */
#define MAX_NESTING_DEPTH	100

/*
 *----------------------------------------------------------------
 * Procedures shared among Tcl modules but not used by the outside
 * world:
 *----------------------------------------------------------------
 */
extern struct _regexp_t *TclCompileRegexp (Tcl_Interp *interp,
			    unsigned char *string);
extern void		TclCopyAndCollapse (int count, unsigned char *src,
			    unsigned char *dst);
extern void		TclDeleteVars (Interp *iPtr,
			    Tcl_HashTable *tablePtr);
extern void		TclExpandParseValue (ParseValue *pvPtr,
			    unsigned short needed);
extern int		TclFindElement (Tcl_Interp *interp,
			    unsigned char *list, unsigned char **elementPtr,
			    unsigned char **nextPtr, int *sizePtr, int *bracePtr);
extern Proc *		TclFindProc (Interp *iPtr,
			    unsigned char *procName);
extern int		TclGetFrame (Tcl_Interp *interp,
			    unsigned char *string, CallFrame **framePtrPtr);
extern int		TclGetListIndex (Tcl_Interp *interp,
			    unsigned char *string, int *indexPtr);
extern int		TclGetOpenFile (Tcl_Interp *interp,
			    unsigned char *string, OpenFile **filePtrPtr);
extern Proc *		TclIsProc (Command *cmdPtr);
extern void		TclMakeFileTable (Interp *iPtr,
			    int index);
extern int		TclParseBraces (Tcl_Interp *interp,
			    unsigned char *string, unsigned char **termPtr,
			    ParseValue *pvPtr);
extern int		TclParseNestedCmd (Tcl_Interp *interp,
			    unsigned char *string, int flags, unsigned char **termPtr,
			    ParseValue *pvPtr);
extern int		TclParseQuotes (Tcl_Interp *interp,
			    unsigned char *string, int termChar, int flags,
			    unsigned char **termPtr, ParseValue *pvPtr);
extern int		TclParseWords (Tcl_Interp *interp,
			    unsigned char *string, int flags, int maxWords,
			    unsigned char **termPtr, int *argcPtr, unsigned char **argv,
			    ParseValue *pvPtr);
extern void		TclSetupEnv (Tcl_Interp *interp);
extern unsigned char *	TclWordEnd (unsigned char *start, int nested);
extern unsigned char *  Tcl_UnixError (Tcl_Interp *interp);

/*
 *----------------------------------------------------------------
 * Command procedures in the generic core:
 *----------------------------------------------------------------
 */
extern int	Tcl_AppendCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_ArrayCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_BreakCmd (void *clientData,	Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_CaseCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_CatchCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_ConcatCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_ContinueCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_ErrorCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_EvalCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_ExprCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_ForCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_ForeachCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_FormatCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_GlobalCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_HistoryCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_IfCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_IncrCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_InfoCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_JoinCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_LappendCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_LindexCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_LinsertCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_LlengthCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_ListCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_LrangeCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_LreplaceCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_LsearchCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_LsortCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_ProcCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_RegexpCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_RegsubCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_RenameCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_ReturnCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_ScanCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_SetCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_SplitCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_StringCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_TraceCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_UnsetCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_UplevelCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_UpvarCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_WhileCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_Cmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_Cmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);

/*
 *----------------------------------------------------------------
 * Command procedures in the UNIX core:
 *----------------------------------------------------------------
 */
extern int	Tcl_CdCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_CloseCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_EofCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_ExecCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_ExitCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_FileCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_FlushCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_GetsCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_GlobCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_OpenCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_PutsCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_PwdCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_ReadCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_SeekCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_SourceCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_TellCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
extern int	Tcl_TimeCmd (void *clientData, Tcl_Interp *interp, int argc, unsigned char **argv);
