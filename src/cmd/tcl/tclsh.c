/*
 * TCL shell for RetroBSD.
 * Copyright (C) 2011 Serge Vakulenko, <vak@cronyx.ru>
 */
#ifdef CROSS
#   include </usr/include/stdio.h>
#else
#   include <stdio.h>
#endif
#include <stdlib.h>
#include <tcl/tcl.h>

/*
 * Implement the TCL loop command:
 *	loop var start end [increment] command
 */
static int
loop_cmd (void *arg, Tcl_Interp *interp, int argc, unsigned char **argv)
{
	int result = TCL_OK;
	int i, first, limit, incr = 1;
	unsigned char *command;
	unsigned char itxt [12];

	if ((argc < 5) || (argc > 6)) {
		Tcl_AppendResult (interp, "bad # args: ", argv [0],
			" var first limit [incr] command", 0);
		return TCL_ERROR;
	}

	if (Tcl_GetInt (interp, argv[2], &first) != TCL_OK)
		return TCL_ERROR;

	if (Tcl_GetInt (interp, argv[3], &limit) != TCL_OK)
		return TCL_ERROR;

	if (argc == 5)
		command = argv[4];
	else {
		if (Tcl_GetInt (interp, argv[4], &incr) != TCL_OK)
			return TCL_ERROR;
		command = argv[5];
	}

	for (i = first;
	    (((i < limit) && (incr > 0)) || ((i > limit) && (incr < 0)));
	    i += incr) {
		sprintf (itxt, "%d", i);
		if (! Tcl_SetVar (interp, argv [1], itxt, TCL_LEAVE_ERR_MSG))
			return TCL_ERROR;

		result = Tcl_Eval (interp, command, 0, 0);
		if (result != TCL_OK) {
			if (result == TCL_CONTINUE) {
				result = TCL_OK;
			} else if (result == TCL_BREAK) {
				result = TCL_OK;
				break;
			} else if (result == TCL_ERROR) {
				unsigned char buf [64];

				sprintf (buf, "\n    (\"loop\" body line %d)",
					interp->errorLine);
				Tcl_AddErrorInfo (interp, buf);
				break;
			} else {
				break;
			}
		}
	}

	/*
	 * Set variable to its final value.
	 */
	sprintf (itxt, "%d", i);
	if (! Tcl_SetVar (interp, argv [1], itxt, TCL_LEAVE_ERR_MSG))
		return TCL_ERROR;

	return result;
}

/*
 * Implement the TCL echo command:
 *	echo arg ...
 */
static int
echo_cmd (void *arg, Tcl_Interp *interp, int argc, unsigned char **argv)
{
	FILE *output = arg;
	int i;

	for (i=1; ; i++) {
		if (! argv[i]) {
			if (i != argc)
echoError:			sprintf (interp->result,
					"argument list wasn't properly NULL-terminated in \"%s\" command",
					argv[0]);
			break;
		}
		if (i >= argc)
			goto echoError;

		if (i > 1)
			putc (' ', output);
		fputs ((char*) argv[i], output);
	}
	putc ('\n', output);
	return TCL_OK;
}

static int
help_cmd (void *arg, Tcl_Interp *interp, int argc, unsigned char **argv)
{
	FILE *output = arg;

	fputs ("Available commands:\n", output);
	fputs ("    loop var first limit [incr] command\n", output);
	fputs ("    echo [param...]\n", output);
	return TCL_OK;
}

/*
 * Read a newline-terminated string from stream.
 */
static unsigned char *
getlin (FILE *input, FILE *output, unsigned char *buf, int len)
{
	int c;
	unsigned char *s;

	s = buf;
        while (--len > 0) {
                fflush (output);
		c = getc (input);
		if (feof (input))
			return 0;
		if (c == '\b') {
			if (s > buf) {
				--s;
				fputs ("\b \b", output);
			}
			continue;
		}
		if (c == '\r')
			c = '\n';
		putc (c, output);
		*s++ = c;
		if (c == '\n')
			break;
	}
	*s = '\0';
	return buf;
}

void tcl_main ()
{
	FILE *input = stdin;
	FILE *output = stdout;
	Tcl_Interp *interp;
	Tcl_CmdBuf buffer;
	unsigned char line [200], *cmd;
	int result, got_partial, quit_flag;

	fputs ("\n\nTCL Shell\n", stdout);
	fputs ("~~~~~~~~~\n", stdout);
	fputs ("\nEnter \"help\" for a list of commands\n\n", stdout);

	interp = Tcl_CreateInterp ();
	Tcl_CreateCommand (interp, (unsigned char*) "loop", loop_cmd, output, 0);
	Tcl_CreateCommand (interp, (unsigned char*) "echo", echo_cmd, output, 0);
	Tcl_CreateCommand (interp, (unsigned char*) "help", help_cmd, output, 0);

	buffer = Tcl_CreateCmdBuf ();
	got_partial = 0;
	quit_flag = 0;
	while (! quit_flag) {
		/*clearerr (input);*/
		if (! got_partial) {
			fputs ("% ", output);
		}
		if (! getlin (input, output, line, sizeof (line))) {
			if (! got_partial)
				break;

			line[0] = 0;
		}
		cmd = Tcl_AssembleCmd (buffer, line);
		if (! cmd) {
			got_partial = 1;
			continue;
		}

		got_partial = 0;
		result = Tcl_Eval (interp, cmd, 0, 0);

		if (result != TCL_OK) {
			fputs ("Error", output);

			if (result != TCL_ERROR)
				fprintf (output, " %d", result);

			if (*interp->result != 0)
				fprintf (output, ": %s", interp->result);

			putc ('\n', output);
			continue;
		}

		if (*interp->result != 0)
			fprintf (output, "%s\n", interp->result);
	}
        fflush (output);
	Tcl_DeleteInterp (interp);
	Tcl_DeleteCmdBuf (buffer);
}

int main (void)
{
	for (;;)
		tcl_main ();
}
