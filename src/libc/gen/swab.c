/*
 * Swab bytes
 * Jeffrey Mogul, Stanford
 */
void
swab (from, to, n)
	register char *from, *to;
	register int n;
{
#ifdef pdp11
	register int temp;
#else
	register unsigned long temp;
#endif

	n >>= 1; n++;
#define	STEP	temp = *from++,*to++ = *from++,*to++ = temp
	/* round to multiple of 8 */
	while ((--n) & 07)
		STEP;
	n >>= 3;
	while (--n >= 0) {
		STEP; STEP; STEP; STEP;
		STEP; STEP; STEP; STEP;
	}
}
