/*
 * Return the ptr in sp at which the character c appears;
 * NULL if not found
 *
 * this routine is just "index" renamed.
 */
char *
strchr (sp, c)
	register const char *sp;
	register int c;
{
	do {
		if (*sp == c)
			return (char*) sp;
	} while (*sp++);
	return 0;
}
