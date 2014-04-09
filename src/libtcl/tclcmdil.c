/*
 * tclCmdIL.c --
 *
 *	This file contains the top-level command routines for most of
 *	the Tcl built-in commands whose names begin with the letters
 *	I through L.  It contains only commands in the generic core
 *	(i.e. those that don't depend much upon UNIX facilities).
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
#include "internal.h"

/*
 * Forward declarations for procedures defined in this file:
 */

static int		SortCompareProc (const void *first, const void *second);

/*
 *----------------------------------------------------------------------
 *
 * Tcl_IfCmd --
 *
 *	This procedure is invoked to process the "if" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_IfCmd(dummy, interp, argc, argv)
    void *dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    unsigned char **argv;		/* Argument strings. */
{
    int i, result, value;

    i = 1;
    while (1) {
	/*
	 * At this point in the loop, argv and argc refer to an expression
	 * to test, either for the main expression or an expression
	 * following an "elseif".  The arguments after the expression must
	 * be "then" (optional) and a script to execute if the expression is
	 * true.
	 */

	if (i >= argc) {
	    Tcl_AppendResult(interp, "wrong # args: no expression after \"",
		    argv[i-1], "\" argument", 0);
	    return TCL_ERROR;
	}
	result = Tcl_ExprBoolean(interp, argv[i], &value);
	if (result != TCL_OK) {
	    return result;
	}
	i++;
	if ((i < argc) && (strcmp(argv[i], (unsigned char*) "then") == 0)) {
	    i++;
	}
	if (i >= argc) {
	    Tcl_AppendResult(interp, "wrong # args: no script following \"",
		    argv[i-1], "\" argument", 0);
	    return TCL_ERROR;
	}
	if (value) {
	    return Tcl_Eval(interp, argv[i], 0, 0);
	}

	/*
	 * The expression evaluated to false.  Skip the command, then
	 * see if there is an "else" or "elseif" clause.
	 */

	i++;
	if (i >= argc) {
	    return TCL_OK;
	}
	if ((argv[i][0] == 'e') && (strcmp(argv[i], (unsigned char*) "elseif") == 0)) {
	    i++;
	    continue;
	}
	break;
    }

    /*
     * Couldn't find a "then" or "elseif" clause to execute.  Check now
     * for an "else" clause.  We know that there's at least one more
     * argument when we get here.
     */

    if (strcmp(argv[i], (unsigned char*) "else") == 0) {
	i++;
	if (i >= argc) {
	    Tcl_AppendResult(interp,
		    "wrong # args: no script following \"else\" argument", 0);
	    return TCL_ERROR;
	}
    }
    return Tcl_Eval(interp, argv[i], 0, 0);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_IncrCmd --
 *
 *	This procedure is invoked to process the "incr" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

    /* ARGSUSED */
int
Tcl_IncrCmd(dummy, interp, argc, argv)
    void *dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    unsigned char **argv;		/* Argument strings. */
{
    int value;
    unsigned char *oldString, *result;
    unsigned char newString[30];

    if ((argc != 2) && (argc != 3)) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" varName ?increment?\"", 0);
	return TCL_ERROR;
    }

    oldString = Tcl_GetVar(interp, argv[1], TCL_LEAVE_ERR_MSG);
    if (oldString == 0) {
	return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, oldString, &value) != TCL_OK) {
	Tcl_AddErrorInfo(interp, (unsigned char*)
		"\n    (reading value of variable to increment)");
	return TCL_ERROR;
    }
    if (argc == 2) {
	value += 1;
    } else {
	int increment;

	if (Tcl_GetInt(interp, argv[2], &increment) != TCL_OK) {
	    Tcl_AddErrorInfo(interp, (unsigned char*)
		    "\n    (reading increment)");
	    return TCL_ERROR;
	}
	value += increment;
    }
    sprintf(newString, "%d", value);
    result = Tcl_SetVar(interp, argv[1], newString, TCL_LEAVE_ERR_MSG);
    if (result == 0) {
	return TCL_ERROR;
    }
    interp->result = result;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_InfoCmd --
 *
 *	This procedure is invoked to process the "info" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_InfoCmd(dummy, interp, argc, argv)
    void *dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    unsigned char **argv;		/* Argument strings. */
{
    register Interp *iPtr = (Interp *) interp;
    int length;
    char c;
    Arg *argPtr;
    Proc *procPtr;
    Var *varPtr;
    Command *cmdPtr;
    Tcl_HashEntry *hPtr;
    Tcl_HashSearch search;

    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" option ?arg arg ...?\"", 0);
	return TCL_ERROR;
    }
    c = argv[1][0];
    length = strlen(argv[1]);
    if ((c == 'a') && (strncmp(argv[1], (unsigned char*) "args", length)) == 0) {
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " args procname\"", 0);
	    return TCL_ERROR;
	}
	procPtr = TclFindProc(iPtr, argv[2]);
	if (procPtr == 0) {
	    infoNoSuchProc:
	    Tcl_AppendResult(interp, "\"", argv[2],
		    "\" isn't a procedure", 0);
	    return TCL_ERROR;
	}
	for (argPtr = procPtr->argPtr; argPtr != 0;
		argPtr = argPtr->nextPtr) {
	    Tcl_AppendElement(interp, argPtr->name, 0);
	}
	return TCL_OK;
    } else if ((c == 'b') && (strncmp(argv[1], (unsigned char*) "body", length)) == 0) {
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		    " body procname\"", 0);
	    return TCL_ERROR;
	}
	procPtr = TclFindProc(iPtr, argv[2]);
	if (procPtr == 0) {
	    goto infoNoSuchProc;
	}
	iPtr->result = procPtr->command;
	return TCL_OK;
    } else if ((c == 'c') && (strncmp(argv[1], (unsigned char*) "cmdcount", length) == 0)
	    && (length >= 2)) {
	if (argc != 2) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		    " cmdcount\"", 0);
	    return TCL_ERROR;
	}
	sprintf(iPtr->result, "%ld", iPtr->cmdCount);
	return TCL_OK;
    } else if ((c == 'c') && (strncmp(argv[1], (unsigned char*) "commands", length) == 0)
	    && (length >= 4)) {
	if (argc > 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		    " commands [pattern]\"", 0);
	    return TCL_ERROR;
	}
	for (hPtr = Tcl_FirstHashEntry(&iPtr->commandTable, &search);
		hPtr != 0; hPtr = Tcl_NextHashEntry(&search)) {
	    unsigned char *name = Tcl_GetHashKey(&iPtr->commandTable, hPtr);
	    if ((argc == 3) && !Tcl_StringMatch(name, argv[2])) {
		continue;
	    }
	    Tcl_AppendElement(interp, name, 0);
	}
	return TCL_OK;
    } else if ((c == 'c') && (strncmp(argv[1], (unsigned char*) "complete", length) == 0)
	    && (length >= 4)) {
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		    " complete command\"", 0);
	    return TCL_ERROR;
	}
	if (Tcl_CommandComplete(argv[2])) {
	    interp->result = (unsigned char*) "1";
	} else {
	    interp->result = (unsigned char*) "0";
	}
	return TCL_OK;
    } else if ((c == 'd') && (strncmp(argv[1], (unsigned char*) "default", length)) == 0) {
	if (argc != 5) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " default procname arg varname\"", 0);
	    return TCL_ERROR;
	}
	procPtr = TclFindProc(iPtr, argv[2]);
	if (procPtr == 0) {
	    goto infoNoSuchProc;
	}
	for (argPtr = procPtr->argPtr; ; argPtr = argPtr->nextPtr) {
	    if (argPtr == 0) {
		Tcl_AppendResult(interp, "procedure \"", argv[2],
			"\" doesn't have an argument \"", argv[3],
			"\"", 0);
		return TCL_ERROR;
	    }
	    if (strcmp(argv[3], argPtr->name) == 0) {
		if (argPtr->defValue != 0) {
		    if (Tcl_SetVar((Tcl_Interp *) iPtr, argv[4],
			    argPtr->defValue, 0) == 0) {
			defStoreError:
			Tcl_AppendResult(interp,
				"couldn't store default value in variable \"",
				argv[4], "\"", 0);
			return TCL_ERROR;
		    }
		    iPtr->result = (unsigned char*) "1";
		} else {
		    if (Tcl_SetVar((Tcl_Interp *) iPtr, argv[4], (unsigned char*) "", 0)
			    == 0) {
			goto defStoreError;
		    }
		    iPtr->result = (unsigned char*) "0";
		}
		return TCL_OK;
	    }
	}
    } else if ((c == 'e') && (strncmp(argv[1], (unsigned char*) "exists", length) == 0)) {
	unsigned char *p;
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		    " exists varName\"", 0);
	    return TCL_ERROR;
	}
	p = Tcl_GetVar((Tcl_Interp *) iPtr, argv[2], 0);

	/*
	 * The code below handles the special case where the name is for
	 * an array:  Tcl_GetVar will reject this since you can't read
	 * an array variable without an index.
	 */

	if (p == 0) {
	    Tcl_HashEntry *hPtr;
	    Var *varPtr;

	    if (strchr(argv[2], '(') != 0) {
		noVar:
		iPtr->result = (unsigned char*) "0";
		return TCL_OK;
	    }
	    if (iPtr->varFramePtr == 0) {
		hPtr = Tcl_FindHashEntry(&iPtr->globalTable, argv[2]);
	    } else {
		hPtr = Tcl_FindHashEntry(&iPtr->varFramePtr->varTable, argv[2]);
	    }
	    if (hPtr == 0) {
		goto noVar;
	    }
	    varPtr = (Var *) Tcl_GetHashValue(hPtr);
	    if (varPtr->flags & VAR_UPVAR) {
		varPtr = (Var *) Tcl_GetHashValue(varPtr->value.upvarPtr);
	    }
	    if (!(varPtr->flags & VAR_ARRAY)) {
		goto noVar;
	    }
	}
	iPtr->result = (unsigned char*) "1";
	return TCL_OK;
    } else if ((c == 'g') && (strncmp(argv[1], (unsigned char*) "globals", length) == 0)) {
	unsigned char *name;

	if (argc > 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		    " globals [pattern]\"", 0);
	    return TCL_ERROR;
	}
	for (hPtr = Tcl_FirstHashEntry(&iPtr->globalTable, &search);
		hPtr != 0; hPtr = Tcl_NextHashEntry(&search)) {
	    varPtr = (Var *) Tcl_GetHashValue(hPtr);
	    if (varPtr->flags & VAR_UNDEFINED) {
		continue;
	    }
	    name = Tcl_GetHashKey(&iPtr->globalTable, hPtr);
	    if ((argc == 3) && !Tcl_StringMatch(name, argv[2])) {
		continue;
	    }
	    Tcl_AppendElement(interp, name, 0);
	}
	return TCL_OK;
    } else if ((c == 'l') && (strncmp(argv[1], (unsigned char*) "level", length) == 0)
	    && (length >= 2)) {
	if (argc == 2) {
	    if (iPtr->varFramePtr == 0) {
		iPtr->result = (unsigned char*) "0";
	    } else {
		sprintf(iPtr->result, "%d", iPtr->varFramePtr->level);
	    }
	    return TCL_OK;
	} else if (argc == 3) {
	    int level;
	    CallFrame *framePtr;

	    if (Tcl_GetInt(interp, argv[2], &level) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (level <= 0) {
		if (iPtr->varFramePtr == 0) {
		    levelError:
		    Tcl_AppendResult(interp, "bad level \"", argv[2],
			    "\"", 0);
		    return TCL_ERROR;
		}
		level += iPtr->varFramePtr->level;
	    }
	    for (framePtr = iPtr->varFramePtr; framePtr != 0;
		    framePtr = framePtr->callerVarPtr) {
		if (framePtr->level == level) {
		    break;
		}
	    }
	    if (framePtr == 0) {
		goto levelError;
	    }
	    iPtr->result = Tcl_Merge (framePtr->argc, framePtr->argv);
	    iPtr->freeProc = (Tcl_FreeProc *) free;
	    return TCL_OK;
	}
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" level [number]\"", 0);
	return TCL_ERROR;
#ifdef TCL_FILE_CMDS
    } else if ((c == 'l') && (strncmp(argv[1], "library", length) == 0)
	    && (length >= 2)) {
	if (argc != 2) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		    " library\"", 0);
	    return TCL_ERROR;
	}
	interp->result = getenv("TCL_LIBRARY");
	if (interp->result == 0) {
#ifdef TCL_LIBRARY
	    interp->result = TCL_LIBRARY;
#else
	    interp->result = "there is no Tcl library at this installation";
	    return TCL_ERROR;
#endif
	}
	return TCL_OK;
#endif
    } else if ((c == 'l') && (strncmp(argv[1], (unsigned char*) "locals", length) == 0)
	    && (length >= 2)) {
	unsigned char *name;

	if (argc > 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		    " locals [pattern]\"", 0);
	    return TCL_ERROR;
	}
	if (iPtr->varFramePtr == 0) {
	    return TCL_OK;
	}
	for (hPtr = Tcl_FirstHashEntry(&iPtr->varFramePtr->varTable, &search);
		hPtr != 0; hPtr = Tcl_NextHashEntry(&search)) {
	    varPtr = (Var *) Tcl_GetHashValue(hPtr);
	    if (varPtr->flags & (VAR_UNDEFINED|VAR_UPVAR)) {
		continue;
	    }
	    name = Tcl_GetHashKey(&iPtr->varFramePtr->varTable, hPtr);
	    if ((argc == 3) && !Tcl_StringMatch(name, argv[2])) {
		continue;
	    }
	    Tcl_AppendElement(interp, name, 0);
	}
	return TCL_OK;
    } else if ((c == 'p') && (strncmp(argv[1], (unsigned char*) "procs", length)) == 0) {
	if (argc > 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		    " procs [pattern]\"", 0);
	    return TCL_ERROR;
	}
	for (hPtr = Tcl_FirstHashEntry(&iPtr->commandTable, &search);
		hPtr != 0; hPtr = Tcl_NextHashEntry(&search)) {
	    unsigned char *name = Tcl_GetHashKey(&iPtr->commandTable, hPtr);

	    cmdPtr = (Command *) Tcl_GetHashValue(hPtr);
	    if (!TclIsProc(cmdPtr)) {
		continue;
	    }
	    if ((argc == 3) && !Tcl_StringMatch(name, argv[2])) {
		continue;
	    }
	    Tcl_AppendElement(interp, name, 0);
	}
	return TCL_OK;
    } else if ((c == 's') && (strncmp(argv[1], (unsigned char*) "script", length) == 0)) {
	if (argc != 2) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " script\"", 0);
	    return TCL_ERROR;
	}
	if (iPtr->scriptFile != 0) {
	    interp->result = iPtr->scriptFile;
	}
	return TCL_OK;
    } else if ((c == 't') && (strncmp(argv[1], (unsigned char*) "tclversion", length) == 0)) {
	if (argc != 2) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " tclversion\"", 0);
	    return TCL_ERROR;
	}

	/*
	 * Note:  TCL_VERSION below is expected to be set with a "-D"
	 * switch in the Makefile.
	 */

	strcpy(iPtr->result, (unsigned char*) TCL_VERSION);
	return TCL_OK;
    } else if ((c == 'v') && (strncmp(argv[1], (unsigned char*) "vars", length)) == 0) {
	Tcl_HashTable *tablePtr;
	unsigned char *name;

	if (argc > 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " vars [pattern]\"", 0);
	    return TCL_ERROR;
	}
	if (iPtr->varFramePtr == 0) {
	    tablePtr = &iPtr->globalTable;
	} else {
	    tablePtr = &iPtr->varFramePtr->varTable;
	}
	for (hPtr = Tcl_FirstHashEntry(tablePtr, &search);
		hPtr != 0; hPtr = Tcl_NextHashEntry(&search)) {
	    varPtr = (Var *) Tcl_GetHashValue(hPtr);
	    if (varPtr->flags & VAR_UNDEFINED) {
		continue;
	    }
	    name = Tcl_GetHashKey(tablePtr, hPtr);
	    if ((argc == 3) && !Tcl_StringMatch(name, argv[2])) {
		continue;
	    }
	    Tcl_AppendElement(interp, name, 0);
	}
	return TCL_OK;
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\": should be args, body, cmdcount, commands, ",
		"complete, default, ",
		"exists, globals, level, library, locals, procs, ",
		"script, tclversion, or vars", 0);
	return TCL_ERROR;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_JoinCmd --
 *
 *	This procedure is invoked to process the "join" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_JoinCmd(dummy, interp, argc, argv)
    void *dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    unsigned char **argv;		/* Argument strings. */
{
    unsigned char *joinString;
    unsigned char **listArgv;
    int listArgc, i;

    if (argc == 2) {
	joinString = (unsigned char*) " ";
    } else if (argc == 3) {
	joinString = argv[2];
    } else {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" list ?joinString?\"", 0);
	return TCL_ERROR;
    }

    if (Tcl_SplitList(interp, argv[1], &listArgc, &listArgv) != TCL_OK) {
	return TCL_ERROR;
    }
    for (i = 0; i < listArgc; i++) {
	if (i == 0) {
	    Tcl_AppendResult(interp, listArgv[0], 0);
	} else  {
	    Tcl_AppendResult(interp, joinString, listArgv[i], 0);
	}
    }
    free (listArgv);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_LindexCmd --
 *
 *	This procedure is invoked to process the "lindex" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

    /* ARGSUSED */
int
Tcl_LindexCmd(dummy, interp, argc, argv)
    void *dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    unsigned char **argv;		/* Argument strings. */
{
    unsigned char *p, *element;
    int index, size, parenthesized, result;

    if (argc != 3) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" list index\"", 0);
	return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[2], &index) != TCL_OK) {
	return TCL_ERROR;
    }
    if (index < 0) {
	return TCL_OK;
    }
    for (p = argv[1] ; index >= 0; index--) {
	result = TclFindElement(interp, p, &element, &p, &size,
		&parenthesized);
	if (result != TCL_OK) {
	    return result;
	}
    }
    if (size == 0) {
	return TCL_OK;
    }
    if (size >= TCL_RESULT_SIZE) {
	interp->result = malloc ((unsigned) size + 1);
	interp->freeProc = (Tcl_FreeProc *) free;
    }
    if (parenthesized) {
	memcpy((void *) interp->result, (void *) element, size);
	interp->result[size] = 0;
    } else {
	TclCopyAndCollapse(size, element, interp->result);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_LinsertCmd --
 *
 *	This procedure is invoked to process the "linsert" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_LinsertCmd(dummy, interp, argc, argv)
    void *dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    unsigned char **argv;		/* Argument strings. */
{
    unsigned char *p, *element, savedChar;
    int i, index, count, result, size;

    if (argc < 4) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" list index element ?element ...?\"", 0);
	return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[2], &index) != TCL_OK) {
	return TCL_ERROR;
    }

    /*
     * Skip over the first "index" elements of the list, then add
     * all of those elements to the result.
     */

    size = 0;
    element = argv[1];
    for (count = 0, p = argv[1]; (count < index) && (*p != 0); count++) {
	result = TclFindElement(interp, p, &element, &p, &size, (int *) 0);
	if (result != TCL_OK) {
	    return result;
	}
    }
    if (*p == 0) {
	Tcl_AppendResult(interp, argv[1], 0);
    } else {
	unsigned char *end;

	end = element+size;
	if (element != argv[1]) {
	    while ((*end != 0) && !isspace(*end)) {
		end++;
	    }
	}
	savedChar = *end;
	*end = 0;
	Tcl_AppendResult(interp, argv[1], 0);
	*end = savedChar;
    }

    /*
     * Add the new list elements.
     */

    for (i = 3; i < argc; i++) {
	Tcl_AppendElement(interp, argv[i], 0);
    }

    /*
     * Append the remainder of the original list.
     */

    if (*p != 0) {
	Tcl_AppendResult(interp, " ", p, 0);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ListCmd --
 *
 *	This procedure is invoked to process the "list" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_ListCmd(dummy, interp, argc, argv)
    void *dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    unsigned char **argv;		/* Argument strings. */
{
    if (argc < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" arg ?arg ...?\"", 0);
	return TCL_ERROR;
    }
    interp->result = Tcl_Merge (argc-1, argv+1);
    interp->freeProc = (Tcl_FreeProc *) free;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_LlengthCmd --
 *
 *	This procedure is invoked to process the "llength" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_LlengthCmd(dummy, interp, argc, argv)
    void *dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    unsigned char **argv;		/* Argument strings. */
{
    int count, result;
    unsigned char *element, *p;

    if (argc != 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" list\"", 0);
	return TCL_ERROR;
    }
    for (count = 0, p = argv[1]; *p != 0 ; count++) {
	result = TclFindElement(interp, p, &element, &p, (int *) 0,
		(int *) 0);
	if (result != TCL_OK) {
	    return result;
	}
	if (*element == 0) {
	    break;
	}
    }
    sprintf(interp->result, "%d", count);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_LrangeCmd --
 *
 *	This procedure is invoked to process the "lrange" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_LrangeCmd(notUsed, interp, argc, argv)
    void *notUsed;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    unsigned char **argv;		/* Argument strings. */
{
    int first, last, result;
    unsigned char *begin, *end, c, *dummy;
    int count;

    if (argc != 4) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" list first last\"", 0);
	return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[2], &first) != TCL_OK) {
	return TCL_ERROR;
    }
    if (first < 0) {
	first = 0;
    }
    if ((*argv[3] == 'e') && (strncmp(argv[3], (unsigned char*) "end", strlen(argv[3])) == 0)) {
	last = 30000;
    } else {
	if (Tcl_GetInt(interp, argv[3], &last) != TCL_OK) {
	    Tcl_ResetResult(interp);
	    Tcl_AppendResult(interp,
		    "expected integer or \"end\" but got \"",
		    argv[3], "\"", 0);
	    return TCL_ERROR;
	}
    }
    if (first > last) {
	return TCL_OK;
    }

    /*
     * Extract a range of fields.
     */

    for (count = 0, begin = argv[1]; count < first; count++) {
	result = TclFindElement(interp, begin, &dummy, &begin, (int *) 0,
		(int *) 0);
	if (result != TCL_OK) {
	    return result;
	}
	if (*begin == 0) {
	    break;
	}
    }
    for (count = first, end = begin; (count <= last) && (*end != 0);
	    count++) {
	result = TclFindElement(interp, end, &dummy, &end, (int *) 0,
		(int *) 0);
	if (result != TCL_OK) {
	    return result;
	}
    }

    /*
     * Chop off trailing spaces.
     */

    while (isspace(end[-1])) {
	end--;
    }
    c = *end;
    *end = 0;
    Tcl_SetResult(interp, begin, TCL_VOLATILE);
    *end = c;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_LreplaceCmd --
 *
 *	This procedure is invoked to process the "lreplace" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_LreplaceCmd(notUsed, interp, argc, argv)
    void *notUsed;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    unsigned char **argv;		/* Argument strings. */
{
    unsigned char *p1, *p2, *element, savedChar, *dummy;
    int i, first, last, count, result, size;

    if (argc < 4) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" list first last ?element element ...?\"", 0);
	return TCL_ERROR;
    }
    if (Tcl_GetInt(interp, argv[2], &first) != TCL_OK) {
	return TCL_ERROR;
    }
    if (TclGetListIndex(interp, argv[3], &last) != TCL_OK) {
	return TCL_ERROR;
    }
    if (first < 0) {
	first = 0;
    }
    if (last < 0) {
	last = 0;
    }
    if (first > last) {
	Tcl_AppendResult(interp, "first index must not be greater than second", 0);
	return TCL_ERROR;
    }

    /*
     * Skip over the elements of the list before "first".
     */

    size = 0;
    element = argv[1];
    for (count = 0, p1 = argv[1]; (count < first) && (*p1 != 0); count++) {
	result = TclFindElement(interp, p1, &element, &p1, &size,
		(int *) 0);
	if (result != TCL_OK) {
	    return result;
	}
    }
    if (*p1 == 0) {
	Tcl_AppendResult(interp, "list doesn't contain element ",
		argv[2], 0);
	return TCL_ERROR;
    }

    /*
     * Skip over the elements of the list up through "last".
     */

    for (p2 = p1 ; (count <= last) && (*p2 != 0); count++) {
	result = TclFindElement(interp, p2, &dummy, &p2, (int *) 0,
		(int *) 0);
	if (result != TCL_OK) {
	    return result;
	}
    }

    /*
     * Add the elements before "first" to the result.  Be sure to
     * include quote or brace characters that might terminate the
     * last of these elements.
     */

    p1 = element+size;
    if (element != argv[1]) {
	while ((*p1 != 0) && !isspace(*p1)) {
	    p1++;
	}
    }
    savedChar = *p1;
    *p1 = 0;
    Tcl_AppendResult(interp, argv[1], 0);
    *p1 = savedChar;

    /*
     * Add the new list elements.
     */

    for (i = 4; i < argc; i++) {
	Tcl_AppendElement(interp, argv[i], 0);
    }

    /*
     * Append the remainder of the original list.
     */

    if (*p2 != 0) {
	if (*interp->result == 0) {
	    Tcl_SetResult(interp, p2, TCL_VOLATILE);
	} else {
	    Tcl_AppendResult(interp, " ", p2, 0);
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_LsearchCmd --
 *
 *	This procedure is invoked to process the "lsearch" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_LsearchCmd(notUsed, interp, argc, argv)
    void *notUsed;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    unsigned char **argv;		/* Argument strings. */
{
    int listArgc;
    unsigned char **listArgv;
    int i, match;

    if (argc != 3) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" list pattern\"", 0);
	return TCL_ERROR;
    }
    if (Tcl_SplitList(interp, argv[1], &listArgc, &listArgv) != TCL_OK) {
	return TCL_ERROR;
    }
    match = -1;
    for (i = 0; i < listArgc; i++) {
	if (Tcl_StringMatch(listArgv[i], argv[2])) {
	    match = i;
	    break;
	}
    }
    sprintf (interp->result, "%d", match);
    free (listArgv);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_LsortCmd --
 *
 *	This procedure is invoked to process the "lsort" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
Tcl_LsortCmd(notUsed, interp, argc, argv)
    void *notUsed;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int argc;				/* Number of arguments. */
    unsigned char **argv;		/* Argument strings. */
{
    int listArgc;
    unsigned char **listArgv;

    if (argc != 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		" list\"", 0);
	return TCL_ERROR;
    }
    if (Tcl_SplitList(interp, argv[1], &listArgc, &listArgv) != TCL_OK) {
	return TCL_ERROR;
    }
    qsort((void *) listArgv, listArgc, sizeof (char*), SortCompareProc);
    interp->result = Tcl_Merge (listArgc, listArgv);
    interp->freeProc = (Tcl_FreeProc *) free;
    free (listArgv);
    return TCL_OK;
}

/*
 * The procedure below is called back by qsort to determine
 * the proper ordering between two elements.
 */

static int
SortCompareProc(first, second)
    const void *first, *second;		/* Elements to be compared. */
{
    return strcmp(*((unsigned char **) first), *((unsigned char **) second));
}
