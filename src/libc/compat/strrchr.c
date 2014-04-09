/*
 * Return the ptr in sp at which the character c last
 * appears; NULL if not found
 *
 * This routine is just "rindex" renamed.
 */
char *
strrchr(sp, c)
	register const char *sp;
	register int c;
{
	register char *r;

	r = 0;
	do {
		if (*sp == c)
			r = (char*) sp;
	} while (*sp++);
	return(r);
}
