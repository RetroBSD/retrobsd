/*
 * Internal definitions for regexp(3) routines.
 */
#define NSUBEXP		10

/*
 * The "internal use only" fields in regexp.h are present to pass info from
 * compile to execute that permits the execute phase to run lots faster on
 * simple cases.  They are:
 *
 * start	char that must begin a match; '\0' if none obvious
 * anchor	is the match anchored (at beginning-of-line only)?
 * must		string (pointer into program) that match must include, or NULL
 * mustlen	length of `must' string
 *
 * `Start' and `anchor' permit very fast decisions on suitable starting points
 * for a match, cutting down the work a lot.  `Must' permits fast rejection
 * of lines that cannot possibly match.  The `must' tests are costly enough
 * that regcomp() supplies a `must' only if the r.e. contains something
 * potentially expensive (at present, the only such thing detected is * or +
 * at the start of the r.e., which can involve a lot of backup).  `Mustlen' is
 * supplied because the test in regexec() needs it and regcomp() is computing
 * it anyway.
 */
struct _regexp_t {
	const unsigned char	*startp [NSUBEXP];
	const unsigned char	*endp [NSUBEXP];
	unsigned char		start;
	unsigned char		anchor;
	unsigned char		*must;
	unsigned short		mustlen;
	unsigned char		program [1];
};

/*
 * Utility definitions.
 */
#define	UCHARAT(p)	(*(unsigned char*)(p))

/*
 * The first byte of the regexp internal "program" is actually this magic
 * number; the start node begins in the second byte.
 */
#define	MAGIC		0234

#ifdef DEBUG_REGEXP
unsigned char regsub_narrate;
void regsub_dump (regexp_t *r);
#endif
