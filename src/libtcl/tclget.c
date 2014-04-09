/*
 * tclGet.c --
 *
 *	This file contains procedures to convert strings into
 *	other forms, like integers or floating-point numbers or
 *	booleans, doing syntax checking along the way.
 *
 * Copyright 1990-1991 Regents of the University of California
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
 *----------------------------------------------------------------------
 *
 * Tcl_GetInt --
 *
 *	Given a string, produce the corresponding integer value.
 *
 * Results:
 *	The return value is normally TCL_OK;  in this case *intPtr
 *	will be set to the integer value equivalent to string.  If
 *	string is improperly formed then TCL_ERROR is returned and
 *	an error message will be left in interp->result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetInt(interp, string, intPtr)
    Tcl_Interp *interp;		/* Interpreter to use for error reporting. */
    unsigned char *string;	/* String containing a (possibly signed)
				 * integer in a form acceptable to strtol. */
    int *intPtr;		/* Place to store converted result. */
{
    unsigned char *end;
    int i;

    i = strtol(string, &end, 0);
    while ((*end != '\0') && isspace(*end)) {
	end++;
    }
    if ((end == string) || (*end != 0)) {
	Tcl_AppendResult(interp, "expected integer but got \"", string,
		"\"", (char *) 0);
	return TCL_ERROR;
    }
    *intPtr = i;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetBoolean --
 *
 *	Given a string, return a 0/1 boolean value corresponding
 *	to the string.
 *
 * Results:
 *	The return value is normally TCL_OK;  in this case *boolPtr
 *	will be set to the 0/1 value equivalent to string.  If
 *	string is improperly formed then TCL_ERROR is returned and
 *	an error message will be left in interp->result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetBoolean(interp, string, boolPtr)
    Tcl_Interp *interp;		/* Interpreter to use for error reporting. */
    unsigned char *string;	/* String containing a boolean number
				 * specified either as 1/0 or true/false or
				 * yes/no. */
    int *boolPtr;		/* Place to store converted result, which
				 * will be 0 or 1. */
{
    char c;
    unsigned char lowerCase[10];
    int i, length;

    /*
     * Convert the input string to all lower-case.
     */

    for (i = 0; i < 9; i++) {
	c = string[i];
	if (c == 0) {
	    break;
	}
	if ((c >= 'A') && (c <= 'Z')) {
	    c += 'a' - 'A';
	}
	lowerCase[i] = c;
    }
    lowerCase[i] = 0;

    length = strlen(lowerCase);
    c = lowerCase[0];
    if ((c == '0') && (lowerCase[1] == '\0')) {
	*boolPtr = 0;
    } else if ((c == '1') && (lowerCase[1] == '\0')) {
	*boolPtr = 1;
    } else if ((c == 'y') && (strncmp(lowerCase, (unsigned char*) "yes", length) == 0)) {
	*boolPtr = 1;
    } else if ((c == 'n') && (strncmp(lowerCase, (unsigned char*) "no", length) == 0)) {
	*boolPtr = 0;
    } else if ((c == 't') && (strncmp(lowerCase, (unsigned char*) "true", length) == 0)) {
	*boolPtr = 1;
    } else if ((c == 'f') && (strncmp(lowerCase, (unsigned char*) "false", length) == 0)) {
	*boolPtr = 0;
    } else if ((c == 'o') && (length >= 2)) {
	if (strncmp(lowerCase, (unsigned char*) "on", length) == 0) {
	    *boolPtr = 1;
	} else if (strncmp(lowerCase, (unsigned char*) "off", length) == 0) {
	    *boolPtr = 0;
	}
    } else {
	Tcl_AppendResult(interp, "expected boolean value but got \"",
		string, "\"", (char *) 0);
	return TCL_ERROR;
    }
    return TCL_OK;
}
