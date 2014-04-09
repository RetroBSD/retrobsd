/*
 * tcl.h --
 *
 *	This header file describes the externally-visible facilities
 *	of the Tcl interpreter.
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
#ifndef _TCL
#define _TCL

#define TCL_VERSION		"6.7"
#define TCL_MAJOR_VERSION	6
#define TCL_MINOR_VERSION	7

/*
 * Data structures defined opaquely in this module.  The definitions
 * below just provide dummy types.  A few fields are made visible in
 * Tcl_Interp structures, namely those for returning string values.
 * Note:  any change to the Tcl_Interp definition below must be mirrored
 * in the "real" definition in tclInt.h.
 */
typedef struct Tcl_Interp {
    unsigned char *result;	/* Points to result string returned by last
				 * command. */
    void (*freeProc) (unsigned char *blockPtr);
				/* Zero means result is statically allocated.
				 * If non-zero, gives address of procedure
				 * to invoke to free the result.  Must be
				 * freed by Tcl_Eval before executing next
				 * command. */
    unsigned short errorLine;	/* When TCL_ERROR is returned, this gives
				 * the line number within the command where
				 * the error occurred (1 means first line). */
} Tcl_Interp;

typedef void *Tcl_Trace;
typedef void *Tcl_CmdBuf;

/*
 * When a TCL command returns, the string pointer interp->result points to
 * a string containing return information from the command.  In addition,
 * the command procedure returns an integer value, which is one of the
 * following:
 *
 * TCL_OK		Command completed normally;  interp->result contains
 *			the command's result.
 * TCL_ERROR		The command couldn't be completed successfully;
 *			interp->result describes what went wrong.
 * TCL_RETURN		The command requests that the current procedure
 *			return;  interp->result contains the procedure's
 *			return value.
 * TCL_BREAK		The command requests that the innermost loop
 *			be exited;  interp->result is meaningless.
 * TCL_CONTINUE		Go on to the next iteration of the current loop;
 *			interp->result is meaninless.
 */
#define TCL_OK		0
#define TCL_ERROR	1
#define TCL_RETURN	2
#define TCL_BREAK	3
#define TCL_CONTINUE	4

#define TCL_RESULT_SIZE 199

/*
 * Procedure types defined by Tcl:
 */
typedef void (Tcl_CmdDeleteProc) (void *clientData);
typedef int (Tcl_CmdProc) (void *clientData,
	Tcl_Interp *interp, int argc, unsigned char *argv[]);
typedef void (Tcl_CmdTraceProc) (void *clientData,
	Tcl_Interp *interp, int level, unsigned char *command, Tcl_CmdProc *proc,
	void *cmdClientData, int argc, unsigned char *argv[]);
typedef void (Tcl_FreeProc) (unsigned char *blockPtr);
typedef unsigned char *(Tcl_VarTraceProc) (void *clientData,
	Tcl_Interp *interp, unsigned char *part1, unsigned char *part2, int flags);

/*
 * Flag values passed to Tcl_Eval (see the man page for details;  also
 * see tclInt.h for additional flags that are only used internally by
 * Tcl):
 */
#define TCL_BRACKET_TERM	1

/*
 * Flag that may be passed to Tcl_ConvertElement to force it not to
 * output braces (careful!  if you change this flag be sure to change
 * the definitions at the front of tclUtil.c).
 */
#define TCL_DONT_USE_BRACES	1

/*
 * Flag value passed to Tcl_RecordAndEval to request no evaluation
 * (record only).
 */
#define TCL_NO_EVAL		-1

/*
 * Specil freeProc values that may be passed to Tcl_SetResult (see
 * the man page for details):
 */
#define TCL_STATIC	((Tcl_FreeProc *) 0)
#define TCL_VOLATILE	((Tcl_FreeProc *) -1)
#define TCL_DYNAMIC	((Tcl_FreeProc *) -2)

/*
 * Flag values passed to variable-related procedures.
 */
#define TCL_GLOBAL_ONLY		1
#define TCL_APPEND_VALUE	2
#define TCL_LIST_ELEMENT	4
#define TCL_NO_SPACE		8
#define TCL_TRACE_READS		0x10
#define TCL_TRACE_WRITES	0x20
#define TCL_TRACE_UNSETS	0x40
#define TCL_TRACE_DESTROYED	0x80
#define TCL_INTERP_DESTROYED	0x100
#define TCL_LEAVE_ERR_MSG	0x200

/*
 * Additional flag passed back to variable watchers.  This flag must
 * not overlap any of the TCL_TRACE_* flags defined above or the
 * TRACE_* flags defined in tclInt.h.
 */
#define TCL_VARIABLE_UNDEFINED	8

/*
 * Exported Tcl procedures:
 */
extern void		Tcl_AppendElement (Tcl_Interp *interp, unsigned char *string,
				int noSep);
extern void		Tcl_AppendResult (Tcl_Interp *interp, ...);
extern unsigned char *	Tcl_AssembleCmd (Tcl_CmdBuf buffer, unsigned char *string);
extern void		Tcl_AddErrorInfo (Tcl_Interp *interp, unsigned char *message);
extern char		Tcl_Backslash (unsigned char *src, int *readPtr);
extern int		Tcl_CommandComplete (unsigned char *cmd);
extern unsigned char *	Tcl_Concat (int argc, unsigned char **argv);
extern int		Tcl_ConvertElement (unsigned char *src, unsigned char *dst, int flags);
extern Tcl_CmdBuf	Tcl_CreateCmdBuf (void);
extern void		Tcl_CreateCommand (Tcl_Interp *interp, unsigned char *cmdName,
				Tcl_CmdProc *proc, void *clientData,
				Tcl_CmdDeleteProc *deleteProc);
extern Tcl_Interp *	Tcl_CreateInterp (void);
extern int		Tcl_CreatePipeline (Tcl_Interp *interp, int argc,
				unsigned char **argv, int **pidArrayPtr,
				int *inPipePtr, int *outPipePtr,
				int *errFilePtr);
extern Tcl_Trace	Tcl_CreateTrace (Tcl_Interp *interp,
			    int level, Tcl_CmdTraceProc *proc,
			    void *clientData);
extern void		Tcl_DeleteCmdBuf (Tcl_CmdBuf buffer);
extern int		Tcl_DeleteCommand (Tcl_Interp *interp,
			    unsigned char *cmdName);
extern void		Tcl_DeleteInterp (Tcl_Interp *interp);
extern void		Tcl_DeleteTrace (Tcl_Interp *interp,
			    Tcl_Trace trace);
extern void		Tcl_DetachPids (int numPids, int *pidPtr);
extern unsigned char *	Tcl_ErrnoId (void);
extern int		Tcl_Eval (Tcl_Interp *interp, unsigned char *cmd,
			    int flags, unsigned char **termPtr);
extern int		Tcl_EvalFile (Tcl_Interp *interp,
			    unsigned char *fileName);
extern int		Tcl_ExprBoolean (Tcl_Interp *interp, unsigned char *string,
				int *ptr);
extern int		Tcl_ExprLong (Tcl_Interp *interp, unsigned char *string,
				long *ptr);
extern int		Tcl_ExprString (Tcl_Interp *interp, unsigned char *string);
extern int		Tcl_Fork (void);
extern void		Tcl_FreeResult (Tcl_Interp *interp);
extern int		Tcl_GetBoolean (Tcl_Interp *interp,
			    unsigned char *string, int *boolPtr);
extern int		Tcl_GetInt (Tcl_Interp *interp,
			    unsigned char *string, int *intPtr);
extern unsigned char *	Tcl_GetVar (Tcl_Interp *interp,
			    unsigned char *varName, int flags);
extern unsigned char *	Tcl_GetVar2 (Tcl_Interp *interp,
			    unsigned char *part1, unsigned char *part2, int flags);
extern int		Tcl_GlobalEval (Tcl_Interp *interp,
			    unsigned char *command);
extern void		Tcl_InitHistory (Tcl_Interp *interp);
extern void		Tcl_InitMemory (Tcl_Interp *interp);
extern unsigned char *	Tcl_Merge (int argc, unsigned char **argv);
extern unsigned char *	Tcl_ParseVar (Tcl_Interp *interp,
			    unsigned char *string, unsigned char **termPtr);
extern int		Tcl_RecordAndEval (Tcl_Interp *interp,
			    unsigned char *cmd, int flags);
extern void		Tcl_ResetResult (Tcl_Interp *interp);
extern int		Tcl_ScanElement (unsigned char *string,
			    int *flagPtr);
extern void		Tcl_SetErrorCode (Tcl_Interp *interp, ...);
extern void		Tcl_SetResult (Tcl_Interp *interp,
			    unsigned char *string, Tcl_FreeProc *freeProc);
extern unsigned char *	Tcl_SetVar (Tcl_Interp *interp,
			    unsigned char *varName, unsigned char *newValue, int flags);
extern unsigned char *	Tcl_SetVar2 (Tcl_Interp *interp,
			    unsigned char *part1, unsigned char *part2,
			    unsigned char *newValue, int flags);
extern unsigned char *	Tcl_SignalId (int sig);
extern unsigned char *	Tcl_SignalMsg (int sig);
extern int		Tcl_SplitList (Tcl_Interp *interp,
			    unsigned char *list, int *argcPtr, unsigned char ***argvPtr);
extern int		Tcl_StringMatch (unsigned char *string,
			    unsigned char *pattern);
extern unsigned char *	Tcl_TildeSubst (Tcl_Interp *interp,
			    unsigned char *name);
extern int		Tcl_TraceVar (Tcl_Interp *interp,
			    unsigned char *varName, int flags, Tcl_VarTraceProc *proc,
			    void *clientData);
extern int		Tcl_TraceVar2 (Tcl_Interp *interp,
			    unsigned char *part1, unsigned char *part2, int flags,
			    Tcl_VarTraceProc *proc, void *clientData);
extern int		Tcl_UnsetVar (Tcl_Interp *interp,
			    unsigned char *varName, int flags);
extern int		Tcl_UnsetVar2 (Tcl_Interp *interp,
			    unsigned char *part1, unsigned char *part2, int flags);
extern void		Tcl_UntraceVar (Tcl_Interp *interp,
			    unsigned char *varName, int flags, Tcl_VarTraceProc *proc,
			    void *clientData);
extern void		Tcl_UntraceVar2 (Tcl_Interp *interp,
			    unsigned char *part1, unsigned char *part2, int flags,
			    Tcl_VarTraceProc *proc, void *clientData);
extern int		Tcl_VarEval (Tcl_Interp *interp, ...);
extern void *		Tcl_VarTraceInfo (Tcl_Interp *interp,
			    unsigned char *varName, int flags,
			    Tcl_VarTraceProc *procPtr,
			    void *prevClientData);
extern void *		Tcl_VarTraceInfo2 (Tcl_Interp *interp,
			    unsigned char *part1, unsigned char *part2, int flags,
			    Tcl_VarTraceProc *procPtr,
			    void *prevClientData);
extern int		Tcl_WaitPids (int numPids, int *pidPtr,
			    int *statusPtr);

#endif /* _TCL */
