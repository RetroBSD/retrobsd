/*
 * tclBasic.c --
 *
 *	Contains the basic facilities for TCL command interpretation,
 *	including interpreter creation and deletion, command creation
 *	and deletion, and command parsing and execution.
 *
 * Copyright 1987-1992 Regents of the University of California
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */
#include "internal.h"
#include <stdarg.h>

/*
 * The following structure defines all of the commands in the Tcl core,
 * and the C procedures that execute them.
 */
typedef struct {
	unsigned char *name;	/* Name of command. */
	Tcl_CmdProc *proc;	/* Procedure that executes command. */
} CmdInfo;

/*
 * Built-in commands, and the procedures associated with them:
 */

static CmdInfo builtin_cmds[] = {
    /*
     * Commands in the generic core:
     */

    {(unsigned char*) "append",		Tcl_AppendCmd},
    {(unsigned char*) "array",		Tcl_ArrayCmd},
    {(unsigned char*) "break",		Tcl_BreakCmd},
    {(unsigned char*) "case",		Tcl_CaseCmd},
    {(unsigned char*) "catch",		Tcl_CatchCmd},
    {(unsigned char*) "concat",		Tcl_ConcatCmd},
    {(unsigned char*) "continue",	Tcl_ContinueCmd},
    {(unsigned char*) "error",		Tcl_ErrorCmd},
    {(unsigned char*) "eval",		Tcl_EvalCmd},
    {(unsigned char*) "expr",		Tcl_ExprCmd},
    {(unsigned char*) "for",		Tcl_ForCmd},
    {(unsigned char*) "foreach",	Tcl_ForeachCmd},
    {(unsigned char*) "format",		Tcl_FormatCmd},
    {(unsigned char*) "global",		Tcl_GlobalCmd},
    {(unsigned char*) "if",		Tcl_IfCmd},
    {(unsigned char*) "incr",		Tcl_IncrCmd},
    {(unsigned char*) "info",		Tcl_InfoCmd},
    {(unsigned char*) "join",		Tcl_JoinCmd},
    {(unsigned char*) "lappend",	Tcl_LappendCmd},
    {(unsigned char*) "lindex",		Tcl_LindexCmd},
    {(unsigned char*) "linsert",	Tcl_LinsertCmd},
    {(unsigned char*) "list",		Tcl_ListCmd},
    {(unsigned char*) "llength",	Tcl_LlengthCmd},
    {(unsigned char*) "lrange",		Tcl_LrangeCmd},
    {(unsigned char*) "lreplace",	Tcl_LreplaceCmd},
    {(unsigned char*) "lsearch",	Tcl_LsearchCmd},
    {(unsigned char*) "lsort",		Tcl_LsortCmd},
    {(unsigned char*) "proc",		Tcl_ProcCmd},
    {(unsigned char*) "regexp",		Tcl_RegexpCmd},
    {(unsigned char*) "regsub",		Tcl_RegsubCmd},
    {(unsigned char*) "rename",		Tcl_RenameCmd},
    {(unsigned char*) "return",		Tcl_ReturnCmd},
    {(unsigned char*) "scan",		Tcl_ScanCmd},
    {(unsigned char*) "set",		Tcl_SetCmd},
    {(unsigned char*) "split",		Tcl_SplitCmd},
    {(unsigned char*) "string",		Tcl_StringCmd},
    {(unsigned char*) "trace",		Tcl_TraceCmd},
    {(unsigned char*) "unset",		Tcl_UnsetCmd},
    {(unsigned char*) "uplevel",	Tcl_UplevelCmd},
    {(unsigned char*) "upvar",		Tcl_UpvarCmd},
    {(unsigned char*) "while",		Tcl_WhileCmd},

    /*
     * Commands in the UNIX core:
     */
#ifdef TCL_FILE_CMDS
    {(unsigned char*) "glob",		Tcl_GlobCmd},
    {(unsigned char*) "cd",		Tcl_CdCmd},
    {(unsigned char*) "close",		Tcl_CloseCmd},
    {(unsigned char*) "eof",		Tcl_EofCmd},
    {(unsigned char*) "exit",		Tcl_ExitCmd},
    {(unsigned char*) "file",		Tcl_FileCmd},
    {(unsigned char*) "flush",		Tcl_FlushCmd},
    {(unsigned char*) "gets",		Tcl_GetsCmd},
    {(unsigned char*) "open",		Tcl_OpenCmd},
    {(unsigned char*) "puts",		Tcl_PutsCmd},
    {(unsigned char*) "pwd",		Tcl_PwdCmd},
    {(unsigned char*) "read",		Tcl_ReadCmd},
    {(unsigned char*) "seek",		Tcl_SeekCmd},
    {(unsigned char*) "source",		Tcl_SourceCmd},
    {(unsigned char*) "tell",		Tcl_TellCmd},
#endif
    {0,					0}
};

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateInterp --
 *
 *	Create a new TCL command interpreter.
 *
 * Results:
 *	The return value is a token for the interpreter, which may be
 *	used in calls to procedures like Tcl_CreateCmd, Tcl_Eval, or
 *	Tcl_DeleteInterp.
 *
 * Side effects:
 *	The command interpreter is initialized with an empty variable
 *	table and the built-in commands.
 *
 *----------------------------------------------------------------------
 */

Tcl_Interp *
Tcl_CreateInterp ()
{
    Interp *iPtr;
    Command *c;
    CmdInfo *ci;
    int i;

    iPtr = (Interp*) malloc (sizeof(Interp));
    iPtr->result = iPtr->resultSpace;
    iPtr->freeProc = 0;
    iPtr->errorLine = 0;
    Tcl_InitHashTable (&iPtr->commandTable, TCL_STRING_KEYS);
    Tcl_InitHashTable (&iPtr->globalTable, TCL_STRING_KEYS);
    iPtr->numLevels = 0;
    iPtr->framePtr = 0;
    iPtr->varFramePtr = 0;
    iPtr->activeTracePtr = 0;
    iPtr->numEvents = 0;
    iPtr->events = 0;
    iPtr->curEvent = 0;
    iPtr->curEventNum = 0;
    iPtr->revPtr = 0;
    iPtr->historyFirst = 0;
    iPtr->revDisables = 1;
    iPtr->evalFirst = iPtr->evalLast = 0;
    iPtr->appendResult = 0;
    iPtr->appendAvl = 0;
    iPtr->appendUsed = 0;
    iPtr->numFiles = 0;
    iPtr->filePtrArray = 0;
    for (i = 0; i < NUM_REGEXPS; i++) {
	iPtr->patterns[i] = 0;
	iPtr->patLengths[i] = -1;
	iPtr->regexps[i] = 0;
    }
    iPtr->cmdCount = 0;
    iPtr->noEval = 0;
    iPtr->scriptFile = 0;
    iPtr->flags = 0;
    iPtr->tracePtr = 0;
    iPtr->resultSpace[0] = 0;

    /*
     * Create the built-in commands.  Do it here, rather than calling
     * Tcl_CreateCommand, because it's faster (there's no need to
     * check for a pre-existing command by the same name).
     */
     for (ci = builtin_cmds; ci->name != 0; ci++) {
	int new;
	Tcl_HashEntry *he;

	he = Tcl_CreateHashEntry (&iPtr->commandTable, ci->name, &new);
	if (new) {
	    c = (Command*) malloc (sizeof(Command));
	    c->proc = ci->proc;
	    c->clientData = (void*) 0;
	    c->deleteProc = 0;
	    Tcl_SetHashValue (he, c);
	}
    }
#ifdef TCL_ENV_CMDS
    TclSetupEnv ((Tcl_Interp *) iPtr);
#endif
    return (Tcl_Interp *) iPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DeleteInterp --
 *
 *	Delete an interpreter and free up all of the resources associated
 *	with it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The interpreter is destroyed.  The caller should never again
 *	use the interp token.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DeleteInterp(interp)
    Tcl_Interp *interp;		/* Token for command interpreter (returned
				 * by a previous call to Tcl_CreateInterp). */
{
    Interp *iPtr = (Interp *) interp;
    Tcl_HashEntry *he;
    Tcl_HashSearch search;
    register Command *c;
    int i;

    /*
     * If the interpreter is in use, delay the deletion until later.
     */

    iPtr->flags |= DELETED;
    if (iPtr->numLevels != 0) {
	return;
    }

    /*
     * Free up any remaining resources associated with the
     * interpreter.
     */

    for (he = Tcl_FirstHashEntry(&iPtr->commandTable, &search);
	    he != 0; he = Tcl_NextHashEntry(&search)) {
	c = (Command *) Tcl_GetHashValue(he);
	if (c->deleteProc != 0) {
	    (*c->deleteProc)(c->clientData);
	}
	free (c);
    }
    Tcl_DeleteHashTable(&iPtr->commandTable);
    TclDeleteVars(iPtr, &iPtr->globalTable);
    if (iPtr->events != 0) {
	int i;

	for (i = 0; i < iPtr->numEvents; i++) {
	    free(iPtr->events[i].command);
	}
	free (iPtr->events);
    }
    while (iPtr->revPtr != 0) {
	HistoryRev *nextPtr = iPtr->revPtr->nextPtr;

	free (iPtr->revPtr);
	iPtr->revPtr = nextPtr;
    }
    if (iPtr->appendResult != 0) {
	free(iPtr->appendResult);
    }
#ifdef TCL_FILE_CMDS
    if (iPtr->numFiles > 0) {
	for (i = 0; i < iPtr->numFiles; i++) {
	    OpenFile *filePtr;

	    filePtr = iPtr->filePtrArray[i];
	    if (filePtr == 0) {
		continue;
	    }
	    if (i >= 3) {
		fclose(filePtr->f);
		if (filePtr->f2 != 0) {
		    fclose(filePtr->f2);
		}
		if (filePtr->numPids > 0) {
		    /* Tcl_DetachPids(filePtr->numPids, filePtr->pidPtr); */
		    free (filePtr->pidPtr);
		}
	    }
	    free (filePtr);
	}
	free (iPtr->filePtrArray);
    }
#endif
    for (i = 0; i < NUM_REGEXPS; i++) {
	if (iPtr->patterns[i] == 0) {
	    break;
	}
	free (iPtr->patterns[i]);
	free (iPtr->regexps[i]);
    }
    while (iPtr->tracePtr != 0) {
	Trace *nextPtr = iPtr->tracePtr->nextPtr;

	free (iPtr->tracePtr);
	iPtr->tracePtr = nextPtr;
    }
    free (iPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateCommand --
 *
 *	Define a new command in a command table.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If a command named cmdName already exists for interp, it is
 *	deleted.  In the future, when cmdName is seen as the name of
 *	a command by Tcl_Eval, proc will be called.  When the command
 *	is deleted from the table, deleteProc will be called.  See the
 *	manual entry for details on the calling sequence.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_CreateCommand(interp, cmdName, proc, clientData, deleteProc)
    Tcl_Interp *interp;		/* Token for command interpreter (returned
				 * by a previous call to Tcl_CreateInterp). */
    unsigned char *cmdName;	/* Name of command. */
    Tcl_CmdProc *proc;		/* Command procedure to associate with
				 * cmdName. */
    void *clientData;		/* Arbitrary one-word value to pass to proc. */
    Tcl_CmdDeleteProc *deleteProc;
				/* If not NULL, gives a procedure to call when
				 * this command is deleted. */
{
    Interp *iPtr = (Interp *) interp;
    register Command *c;
    Tcl_HashEntry *he;
    int new;

    he = Tcl_CreateHashEntry(&iPtr->commandTable, cmdName, &new);
    if (!new) {
	/*
	 * Command already exists:  delete the old one.
	 */

	c = (Command *) Tcl_GetHashValue(he);
	if (c->deleteProc != 0) {
	    (*c->deleteProc)(c->clientData);
	}
    } else {
	c = (Command*) malloc (sizeof(Command));
	Tcl_SetHashValue(he, c);
    }
    c->proc = proc;
    c->clientData = clientData;
    c->deleteProc = deleteProc;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DeleteCommand --
 *
 *	Remove the given command from the given interpreter.
 *
 * Results:
 *	0 is returned if the command was deleted successfully.
 *	-1 is returned if there didn't exist a command by that
 *	name.
 *
 * Side effects:
 *	CmdName will no longer be recognized as a valid command for
 *	interp.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_DeleteCommand(interp, cmdName)
    Tcl_Interp *interp;		/* Token for command interpreter (returned
				 * by a previous call to Tcl_CreateInterp). */
    unsigned char *cmdName;	/* Name of command to remove. */
{
    Interp *iPtr = (Interp *) interp;
    Tcl_HashEntry *he;
    Command *c;

    he = Tcl_FindHashEntry(&iPtr->commandTable, cmdName);
    if (he == 0) {
	return -1;
    }
    c = (Command *) Tcl_GetHashValue(he);
    if (c->deleteProc != 0) {
	(*c->deleteProc)(c->clientData);
    }
    free (c);
    Tcl_DeleteHashEntry(he);
    return 0;
}

/*
 *-----------------------------------------------------------------
 *
 * Tcl_Eval --
 *
 *	Parse and execute a command in the Tcl language.
 *
 * Results:
 *	The return value is one of the return codes defined in tcl.hd
 *	(such as TCL_OK), and interp->result contains a string value
 *	to supplement the return code.  The value of interp->result
 *	will persist only until the next call to Tcl_Eval:  copy it or
 *	lose it! *TermPtr is filled in with the character just after
 *	the last one that was part of the command (usually a NULL
 *	character or a closing bracket).
 *
 * Side effects:
 *	Almost certainly;  depends on the command.
 *
 *-----------------------------------------------------------------
 */

int
Tcl_Eval(interp, cmd, flags, termPtr)
    Tcl_Interp *interp;		/* Token for command interpreter (returned
				 * by a previous call to Tcl_CreateInterp). */
    unsigned char *cmd;		/* Pointer to TCL command to interpret. */
    int flags;			/* OR-ed combination of flags like
				 * TCL_BRACKET_TERM and TCL_RECORD_BOUNDS. */
    unsigned char **termPtr;	/* If non-NULL, fill in the address it points
				 * to with the address of the char. just after
				 * the last one that was part of cmd.  See
				 * the man page for details on this. */
{
    /*
     * The storage immediately below is used to generate a copy
     * of the command, after all argument substitutions.  Pv will
     * contain the argv values passed to the command procedure.
     */

#   define NUM_CHARS 200
    unsigned char copyStorage[NUM_CHARS];
    ParseValue pv;
    unsigned char *oldBuffer;

    /*
     * This procedure generates an (argv, argc) array for the command,
     * It starts out with stack-allocated space but uses dynamically-
     * allocated storage to increase it if needed.
     */

#   define NUM_ARGS 10
    unsigned char *(argStorage[NUM_ARGS]);
    unsigned char **argv = argStorage;
    int argc;
    int argSize = NUM_ARGS;

    register unsigned char *src;	/* Points to current character
					 * in cmd. */
    char termChar;			/* Return when this character is found
					 * (either ']' or '\0').  Zero means
					 * that newlines terminate commands. */
    int result;				/* Return value. */
    register Interp *iPtr = (Interp *) interp;
    Tcl_HashEntry *he;
    Command *c;
    unsigned char *dummy;		/* Make termPtr point here if it was
					 * originally NULL. */
    unsigned char *cmdStart;		/* Points to first non-blank char. in
					 * command (used in calling trace
					 * procedures). */
    unsigned char *ellipsis = (unsigned char*) "";
					/* Used in setting errorInfo variable;
					 * set to "..." to indicate that not
					 * all of offending command is included
					 * in errorInfo.  "" means that the
					 * command is all there. */
    register Trace *tracePtr;

    /*
     * Initialize the result to an empty string and clear out any
     * error information.  This makes sure that we return an empty
     * result if there are no commands in the command string.
     */

    Tcl_FreeResult((Tcl_Interp *) iPtr);
    iPtr->result = iPtr->resultSpace;
    iPtr->resultSpace[0] = 0;
    result = TCL_OK;

    /*
     * Check depth of nested calls to Tcl_Eval:  if this gets too large,
     * it's probably because of an infinite loop somewhere.
     */

    iPtr->numLevels++;
    if (iPtr->numLevels > MAX_NESTING_DEPTH) {
	iPtr->numLevels--;
	iPtr->result = (unsigned char*) "too many nested calls to Tcl_Eval (infinite loop?)";
	return TCL_ERROR;
    }

    /*
     * Initialize the area in which command copies will be assembled.
     */

    pv.buffer = copyStorage;
    pv.end = copyStorage + NUM_CHARS - 1;
    pv.expandProc = TclExpandParseValue;
    pv.clientData = (void*) 0;

    src = cmd;
    if (flags & TCL_BRACKET_TERM) {
	termChar = ']';
    } else {
	termChar = 0;
    }
    if (termPtr == 0) {
	termPtr = &dummy;
    }
    *termPtr = src;
    cmdStart = src;

    /*
     * There can be many sub-commands (separated by semi-colons or
     * newlines) in one command string.  This outer loop iterates over
     * individual commands.
     */

    while (*src != termChar) {
	iPtr->flags &= ~(ERR_IN_PROGRESS | ERROR_CODE_SET);

	/*
	 * Skim off leading white space and semi-colons, and skip
	 * comments.
	 */
	while (1) {
	    switch (*src) {
	    case '\t':
	    case '\v':
	    case '\f':
	    case '\r':
	    case '\n':
	    case ' ':
	    case ':':
		++src;
		continue;
	    }
	    break;
	}
	if (*src == '#') {
	    for (src++; *src != 0; src++) {
		if ((*src == '\n') && (src[-1] != '\\')) {
		    src++;
		    break;
		}
	    }
	    continue;
	}
	cmdStart = src;

	/*
	 * Parse the words of the command, generating the argc and
	 * argv for the command procedure.  May have to call
	 * TclParseWords several times, expanding the argv array
	 * between calls.
	 */

	pv.next = oldBuffer = pv.buffer;
	argc = 0;
	while (1) {
	    int newArgs, maxArgs;
	    unsigned char **newArgv;
	    int i;

	    /*
	     * Note:  the "- 2" below guarantees that we won't use the
	     * last two argv slots here.  One is for a NULL pointer to
	     * mark the end of the list, and the other is to leave room
	     * for inserting the command name "unknown" as the first
	     * argument (see below).
	     */

	    maxArgs = argSize - argc - 2;
	    result = TclParseWords((Tcl_Interp *) iPtr, src, flags,
		    maxArgs, termPtr, &newArgs, &argv[argc], &pv);
	    src = *termPtr;
	    if (result != TCL_OK) {
		ellipsis = (unsigned char*) "...";
		goto done;
	    }

	    /*
	     * Careful!  Buffer space may have gotten reallocated while
	     * parsing words.  If this happened, be sure to update all
	     * of the older argv pointers to refer to the new space.
	     */

	    if (oldBuffer != pv.buffer) {
		int i;

		for (i = 0; i < argc; i++) {
		    argv[i] = pv.buffer + (argv[i] - oldBuffer);
		}
		oldBuffer = pv.buffer;
	    }
	    argc += newArgs;
	    if (newArgs < maxArgs) {
		argv[argc] = 0;
		break;
	    }

	    /*
	     * Args didn't all fit in the current array.  Make it bigger.
	     */

	    argSize *= 2;
	    newArgv = (unsigned char**) malloc ((unsigned)
                        argSize * sizeof(char *));
	    for (i = 0; i < argc; i++) {
		newArgv[i] = argv[i];
	    }
	    if (argv != argStorage) {
		free (argv);
	    }
	    argv = newArgv;
	}

	/*
	 * If this is an empty command (or if we're just parsing
	 * commands without evaluating them), then just skip to the
	 * next command.
	 */

	if ((argc == 0) || iPtr->noEval) {
	    continue;
	}
	argv[argc] = 0;

	/*
	 * Save information for the history module, if needed.
	 */

	if (flags & TCL_RECORD_BOUNDS) {
	    iPtr->evalFirst = cmdStart;
	    iPtr->evalLast = src-1;
	}

	/*
	 * Find the procedure to execute this command.  If there isn't
	 * one, then see if there is a command "unknown".  If so,
	 * invoke it instead, passing it the words of the original
	 * command as arguments.
	 */

	he = Tcl_FindHashEntry(&iPtr->commandTable, argv[0]);
	if (he == 0) {
	    int i;

	    he = Tcl_FindHashEntry(&iPtr->commandTable, (unsigned char*) "unknown");
	    if (he == 0) {
		Tcl_ResetResult(interp);
		Tcl_AppendResult(interp, "invalid command name: \"",
			argv[0], "\"", 0);
		result = TCL_ERROR;
		goto done;
	    }
	    for (i = argc; i >= 0; i--) {
		argv[i+1] = argv[i];
	    }
	    argv[0] = (unsigned char*) "unknown";
	    argc++;
	}
	c = (Command *) Tcl_GetHashValue(he);

	/*
	 * Call trace procedures, if any.
	 */

	for (tracePtr = iPtr->tracePtr; tracePtr != 0;
		tracePtr = tracePtr->nextPtr) {
	    char saved;

	    if (tracePtr->level < iPtr->numLevels) {
		continue;
	    }
	    saved = *src;
	    *src = 0;
	    (*tracePtr->proc)(tracePtr->clientData, interp, iPtr->numLevels,
		    cmdStart, c->proc, c->clientData, argc, argv);
	    *src = saved;
	}

	/*
	 * At long last, invoke the command procedure.  Reset the
	 * result to its default empty value first (it could have
	 * gotten changed by earlier commands in the same command
	 * string).
	 */

	iPtr->cmdCount++;
	Tcl_FreeResult ((Tcl_Interp*) iPtr);
	iPtr->result = iPtr->resultSpace;
	iPtr->resultSpace[0] = 0;
	result = (*c->proc)(c->clientData, interp, argc, argv);
	if (result != TCL_OK) {
	    break;
	}
    }

    /*
     * Free up any extra resources that were allocated.
     */

    done:
    if (pv.buffer != copyStorage) {
	free (pv.buffer);
    }
    if (argv != argStorage) {
	free (argv);
    }
    iPtr->numLevels--;
    if (iPtr->numLevels == 0) {
	if (result == TCL_RETURN) {
	    result = TCL_OK;
	}
	if ((result != TCL_OK) && (result != TCL_ERROR)) {
	    Tcl_ResetResult(interp);
	    if (result == TCL_BREAK) {
		iPtr->result = (unsigned char*) "invoked \"break\" outside of a loop";
	    } else if (result == TCL_CONTINUE) {
		iPtr->result = (unsigned char*) "invoked \"continue\" outside of a loop";
	    } else {
		iPtr->result = iPtr->resultSpace;
		sprintf(iPtr->resultSpace, "command returned bad code: %d",
			result);
	    }
	    result = TCL_ERROR;
	}
	if (iPtr->flags & DELETED) {
	    Tcl_DeleteInterp(interp);
	}
    }

    /*
     * If an error occurred, record information about what was being
     * executed when the error occurred.
     */

    if ((result == TCL_ERROR) && !(iPtr->flags & ERR_ALREADY_LOGGED)) {
	int numChars;
	register unsigned char *p;

	/*
	 * Compute the line number where the error occurred.
	 */

	iPtr->errorLine = 1;
	for (p = cmd; p != cmdStart; p++) {
	    if (*p == '\n') {
		iPtr->errorLine++;
	    }
	}
	for ( ; isspace(*p) || (*p == ';'); p++) {
	    if (*p == '\n') {
		iPtr->errorLine++;
	    }
	}

	/*
	 * Figure out how much of the command to print in the error
	 * message (up to a certain number of characters, or up to
	 * the first new-line).
	 */

	numChars = src - cmdStart;
	if (numChars > (NUM_CHARS-50)) {
	    numChars = NUM_CHARS-50;
	    ellipsis = (unsigned char*) " ...";
	}

	if (!(iPtr->flags & ERR_IN_PROGRESS)) {
	    sprintf(copyStorage, "\n    while executing\n\"%.*s%s\"",
		    numChars, cmdStart, ellipsis);
	} else {
	    sprintf(copyStorage, "\n    invoked from within\n\"%.*s%s\"",
		    numChars, cmdStart, ellipsis);
	}
	Tcl_AddErrorInfo(interp, copyStorage);
	iPtr->flags &= ~ERR_ALREADY_LOGGED;
    } else {
	iPtr->flags &= ~ERR_ALREADY_LOGGED;
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_CreateTrace --
 *
 *	Arrange for a procedure to be called to trace command execution.
 *
 * Results:
 *	The return value is a token for the trace, which may be passed
 *	to Tcl_DeleteTrace to eliminate the trace.
 *
 * Side effects:
 *	From now on, proc will be called just before a command procedure
 *	is called to execute a Tcl command.  Calls to proc will have the
 *	following form:
 *
 *	void
 *	proc(clientData, interp, level, command, cmdProc, cmdClientData,
 *		argc, argv)
 *	    void *clientData;
 *	    Tcl_Interp *interp;
 *	    int level;
 *	    unsigned char *command;
 *	    int (*cmdProc)();
 *	    void *cmdClientData;
 *	    int argc;
 *	    unsigned char **argv;
 *	{
 *	}
 *
 *	The clientData and interp arguments to proc will be the same
 *	as the corresponding arguments to this procedure.  Level gives
 *	the nesting level of command interpretation for this interpreter
 *	(0 corresponds to top level).  Command gives the ASCII text of
 *	the raw command, cmdProc and cmdClientData give the procedure that
 *	will be called to process the command and the ClientData value it
 *	will receive, and argc and argv give the arguments to the
 *	command, after any argument parsing and substitution.  Proc
 *	does not return a value.
 *
 *----------------------------------------------------------------------
 */

Tcl_Trace
Tcl_CreateTrace(interp, level, proc, clientData)
    Tcl_Interp *interp;		/* Interpreter in which to create the trace. */
    int level;			/* Only call proc for commands at nesting level
				 * <= level (1 => top level). */
    Tcl_CmdTraceProc *proc;	/* Procedure to call before executing each
				 * command. */
    void *clientData;		/* Arbitrary one-word value to pass to proc. */
{
    register Trace *tracePtr;
    register Interp *iPtr = (Interp *) interp;

    tracePtr = (Trace*) malloc (sizeof(Trace));
    tracePtr->level = level;
    tracePtr->proc = proc;
    tracePtr->clientData = clientData;
    tracePtr->nextPtr = iPtr->tracePtr;
    iPtr->tracePtr = tracePtr;

    return (Tcl_Trace) tracePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DeleteTrace --
 *
 *	Remove a trace.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	From now on there will be no more calls to the procedure given
 *	in trace.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_DeleteTrace(interp, trace)
    Tcl_Interp *interp;		/* Interpreter that contains trace. */
    Tcl_Trace trace;		/* Token for trace (returned previously by
				 * Tcl_CreateTrace). */
{
    register Interp *iPtr = (Interp *) interp;
    register Trace *tracePtr = (Trace *) trace;
    register Trace *tracePtr2;

    if (iPtr->tracePtr == tracePtr) {
	iPtr->tracePtr = tracePtr->nextPtr;
	free (tracePtr);
    } else {
	for (tracePtr2 = iPtr->tracePtr; tracePtr2 != 0;
		tracePtr2 = tracePtr2->nextPtr) {
	    if (tracePtr2->nextPtr == tracePtr) {
		tracePtr2->nextPtr = tracePtr->nextPtr;
		free (tracePtr);
		return;
	    }
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AddErrorInfo --
 *
 *	Add information to a message being accumulated that describes
 *	the current error.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The contents of message are added to the "errorInfo" variable.
 *	If Tcl_Eval has been called since the current value of errorInfo
 *	was set, errorInfo is cleared before adding the new message.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_AddErrorInfo(interp, message)
    Tcl_Interp *interp;		/* Interpreter to which error information
				 * pertains. */
    unsigned char *message;	/* Message to record. */
{
    register Interp *iPtr = (Interp *) interp;

    /*
     * If an error is already being logged, then the new errorInfo
     * is the concatenation of the old info and the new message.
     * If this is the first piece of info for the error, then the
     * new errorInfo is the concatenation of the message in
     * interp->result and the new message.
     */

    if (!(iPtr->flags & ERR_IN_PROGRESS)) {
	Tcl_SetVar2(interp, (unsigned char*) "errorInfo", 0, interp->result,
		TCL_GLOBAL_ONLY);
	iPtr->flags |= ERR_IN_PROGRESS;

	/*
	 * If the errorCode variable wasn't set by the code that generated
	 * the error, set it to "NONE".
	 */

	if (!(iPtr->flags & ERROR_CODE_SET)) {
		Tcl_SetVar2(interp, (unsigned char*) "errorCode", 0,
			(unsigned char*) "NONE", TCL_GLOBAL_ONLY);
	}
    }
    Tcl_SetVar2(interp, (unsigned char*) "errorInfo", 0, message,
	    TCL_GLOBAL_ONLY|TCL_APPEND_VALUE);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_VarEval --
 *
 *	Given a variable number of string arguments, concatenate them
 *	all together and execute the result as a Tcl command.
 *
 * Results:
 *	A standard Tcl return result.  An error message or other
 *	result may be left in interp->result.
 *
 * Side effects:
 *	Depends on what was done by the command.
 *
 *----------------------------------------------------------------------
 */
	/* VARARGS2 */ /* ARGSUSED */
int
Tcl_VarEval (Tcl_Interp *interp,/* Interpreter in which to execute command. */
	...)			/* One or more strings to concatenate,
				 * terminated with a NULL string. */
{
    va_list argList;
#define FIXED_SIZE 200
    unsigned char fixedSpace[FIXED_SIZE+1];
    int spaceAvl, spaceUsed, length;
    unsigned char *string, *cmd;
    int result;

    /*
     * Copy the strings one after the other into a single larger
     * string.  Use stack-allocated space for small commands, but if
     * the commands gets too large than call mem_alloc to create the
     * space.
     */
    va_start(argList, interp);
    spaceAvl = FIXED_SIZE;
    spaceUsed = 0;
    cmd = fixedSpace;
    while (1) {
	string = va_arg(argList, unsigned char *);
	if (string == 0) {
	    break;
	}
	length = strlen(string);
	if ((spaceUsed + length) > spaceAvl) {
	    unsigned char *new;

	    spaceAvl = spaceUsed + length;
	    spaceAvl += spaceAvl/2;
	    new = malloc ((unsigned) spaceAvl);
	    memcpy ((void*) new, (void*) cmd, spaceUsed);
	    if (cmd != fixedSpace) {
		free(cmd);
	    }
	    cmd = new;
	}
	strcpy(cmd + spaceUsed, string);
	spaceUsed += length;
    }
    va_end(argList);
    cmd[spaceUsed] = '\0';

    result = Tcl_Eval(interp, cmd, 0, 0);
    if (cmd != fixedSpace) {
	free(cmd);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GlobalEval --
 *
 *	Evaluate a command at global level in an interpreter.
 *
 * Results:
 *	A standard Tcl result is returned, and interp->result is
 *	modified accordingly.
 *
 * Side effects:
 *	The command string is executed in interp, and the execution
 *	is carried out in the variable context of global level (no
 *	procedures active), just as if an "uplevel #0" command were
 *	being executed.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GlobalEval(interp, command)
    Tcl_Interp *interp;		/* Interpreter in which to evaluate command. */
    unsigned char *command;	/* Command to evaluate. */
{
    register Interp *iPtr = (Interp *) interp;
    int result;
    CallFrame *savedVarFramePtr;

    savedVarFramePtr = iPtr->varFramePtr;
    iPtr->varFramePtr = 0;
    result = Tcl_Eval(interp, command, 0, 0);
    iPtr->varFramePtr = savedVarFramePtr;
    return result;
}
