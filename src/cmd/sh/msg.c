/*
 * UNIX shell
 *
 * Bell Telephone Laboratories
 */
#include "defs.h"
#include "sym.h"

/*
 * error messages
 */
char    badopt[]        = "bad option(s)";
char    mailmsg[]       = "you have mail\n";
char    nospace[]       = "no space";
char    nostack[]       = "no stack space";
char    synmsg[]        = "syntax error";

char    badnum[]        = "bad number";
char    badparam[]      = "parameter null or not set";
char    unset[]         = "parameter not set";
char    badsub[]        = "bad substitution";
char    badcreate[]     = "cannot create";
char    nofork[]        = "fork failed - too many processes";
char    noswap[]        = "cannot fork: no swap space";
char    restricted[]    = "restricted";
char    piperr[]        = "cannot make pipe";
char    badopen[]       = "cannot open";
char    coredump[]      = " - core dumped";
char    arglist[]       = "arg list too long";
char    txtbsy[]        = "text busy";
char    toobig[]        = "too big";
char    badexec[]       = "cannot execute";
char    notfound[]      = "not found";
char    badfile[]       = "bad file number";
char    badshift[]      = "cannot shift";
char    baddir[]        = "bad directory";
char    badtrap[]       = "bad trap";
char    wtfailed[]      = "is read only";
char    notid[]         = "is not an identifier";
char    badulimit[]     = "Bad ulimit";
char    badreturn[] = "cannot return when not in function";
char    badexport[] = "cannot export functions";
char    badunset[]      = "cannot unset";
char    nohome[]        = "no home directory";
char    badperm[]       = "execute permission denied";
char    longpwd[]       = "sh error: pwd too long";
/*
 * messages for 'builtin' functions
 */
char    btest[]         = "test";
char    badop[]         = "unknown operator ";
/*
 * built in names
 */
char    pathname[]      = "PATH";
char    cdpname[]       = "CDPATH";
char    homename[]      = "HOME";
char    mailname[]      = "MAIL";
char    ifsname[]       = "IFS";
char    ps1name[]       = "PS1";
char    ps2name[]       = "PS2";
char    mchkname[]      = "MAILCHECK";
char    acctname[]      = "SHACCT";
char    mailpname[]     = "MAILPATH";

/*
 * string constants
 */
char    nullstr[]       = "";
char    sptbnl[]        = " \t\n";
char    defpath[]       = ":/bin:/usr/bin:/usr/ucb/bin:/etc";
char    colon[]         = ": ";
char    minus[]         = "-";
char    endoffile[]     = "end of file";
char    unexpected[]    = " unexpected";
char    atline[]        = " at line ";
char    devnull[]       = "/dev/null";
char    execpmsg[]      = "+ ";
char    readmsg[]       = "> ";
char    stdprompt[]     = "$ ";
char    supprompt[]     = "# ";
char    profile[]       = ".profile";
char    sysprofile[]    = "/etc/profile";

/*
 * tables
 */

struct sysnod reserved[] =
{
	{ "case",       CASYM   },
	{ "do",         DOSYM   },
	{ "done",       ODSYM   },
	{ "elif",       EFSYM   },
	{ "else",       ELSYM   },
	{ "esac",       ESSYM   },
	{ "fi",         FISYM   },
	{ "for",        FORSYM  },
	{ "if",         IFSYM   },
	{ "in",         INSYM   },
	{ "then",       THSYM   },
	{ "until",      UNSYM   },
	{ "while",      WHSYM   },
	{ "{",          BRSYM   },
	{ "}",          KTSYM   }
};

int no_reserved = 15;

char    *sysmsg[] =
{
	0,
	"Hangup",
	0,      /* Interrupt */
	"Quit",
	"Illegal instruction",
	"Trace/BPT trap",
	"abort",
	"EMT trap",
	"Floating exception",
	"Killed",
	"Bus error",
	"Memory fault",
	"Bad system call",
	"Broken pipe",
	"Alarm call",
	"Terminated",
	"Urgent cond.",
	"Stopped",
	"Ctrl Z",
	"Continued",
	"Child death",
	"Tty input",
	"Tty output",
	"Keyboard",
	/* "Power Fail" */
};

char    export[] = "export";
char    duperr[] = "cannot dup";
char    readonly[] = "readonly";

struct sysnod commands[] =
{
	{ ".",          SYSDOT  },
	{ ":",          SYSNULL },

#ifndef RES
	{ "[",          SYSTST },
#endif

	{ "break",      SYSBREAK },
	{ "cd",         SYSCD   },
	{ "continue",   SYSCONT },
	{ "echo",       SYSECHO },
	{ "eval",       SYSEVAL },
	{ "exec",       SYSEXEC },
	{ "exit",       SYSEXIT },
	{ "export",     SYSXPORT },
	{ "hash",       SYSHASH },

#ifdef RES
	{ "login",      SYSLOGIN },
	{ "newgrp",     SYSLOGIN },
#else
	{ "login",      SYSLOGIN },
	{ "newgrp",     SYSNEWGRP },
#endif
	{ "pwd",        SYSPWD },
	{ "read",       SYSREAD },
	{ "readonly",   SYSRDONLY },
	{ "return",     SYSRETURN },
	{ "set",	SYSSET	},
	{ "shift",	SYSSHFT	},
	{ "test",	SYSTST },
	{ "times",	SYSTIMES },
	{ "trap",	SYSTRAP	},
	{ "type",	SYSTYPE },

#ifndef RES
	{ "ulimit",	SYSULIMIT },
	{ "umask",	SYSUMASK },
#endif
	{ "unset", 	SYSUNS },
	{ "wait",	SYSWAIT	}
};

#ifdef RES
	int no_commands = 26;
#else
	int no_commands = 28;
#endif
