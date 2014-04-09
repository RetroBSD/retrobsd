/*
 * Definitions etc. for regexp(3) routines.
 */
typedef struct _regexp_t regexp_t;

/*
 * Determine the required size.
 * On failure, returns 0.
 */
unsigned regexp_size (const unsigned char *pattern);

/*
 * Compile a regular expression into internal code.
 * Returns 1 on success, or 0 on failure.
 */
bool_t regexp_compile (regexp_t *re, const unsigned char *pattern);

/*
 * Match a regular expression against a string.
 * Returns 1 on success, or 0 on failure.
 */
bool_t regexp_execute (regexp_t *re, const unsigned char *str);

/*
 * Perform substitutions after a regexp match.
 * Returns 1 on success, or 0 on failure.
 */
bool_t regexp_substitute (const regexp_t *re,
	const unsigned char *src, unsigned char *dst);
