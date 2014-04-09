/*
 * Compare strings (at most n bytes):  s1>s2: >0  s1==s2: 0  s1<s2: <0
 */
#include <string.h>

int
strncmp (s1, s2, n)
	register const char *s1, *s2;
	register size_t n;
{
        for (;;) {
                if (n-- == 0)
                        return 0;
                if (*s1 != *s2++)
                        return *s1 - *--s2;
		if (*s1++ == '\0')
			return 0;
        }
}
