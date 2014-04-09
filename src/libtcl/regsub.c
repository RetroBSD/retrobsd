/*
 *	Copyright (c) 1986 by University of Toronto.
 *	Written by Henry Spencer.  Not derived from licensed software.
 *
 *	Permission is granted to anyone to use this software for any
 *	purpose on any computer system, and to redistribute it freely,
 *	subject to the following restrictions:
 *
 *	1. The author is not responsible for the consequences of use of
 *		this software, no matter how awful, even if they arise
 *		from defects in it.
 *
 *	2. The origin of this software must not be misrepresented, either
 *		by explicit claim or by omission.
 *
 *	3. Altered versions must be plainly marked as such, and must not
 *		be misrepresented as being the original software.
 */
#include <stdlib.h>
#include <string.h>
#include "regexp.h"
#include "regpriv.h"

/*
 * Perform substitutions after a regexp match.
 * Returns 1 on success, or 0 on failure.
 */
bool_t
regexp_substitute (const regexp_t *prog, const unsigned char *src, unsigned char *dst)
{
	unsigned char c;
	unsigned char no;
	unsigned short len;

	if (! prog || ! src || ! dst) {
		/* regerror("NULL parm to regsub"); */
		return 0;
	}
	if (UCHARAT(prog->program) != MAGIC) {
		/* regerror("damaged regexp fed to regsub"); */
		return 0;
	}

	while ((c = *src++) != '\0') {
		if (c == '&')
			no = 0;
		else if (c == '\\' && '0' <= *src && *src <= '9')
			no = *src++ - '0';
		else
			no = 10;
 		if (no > 9) {
			/* Ordinary character. */
 			if (c == '\\' && (*src == '\\' || *src == '&'))
 				c = *src++;
 			*dst++ = c;
 		} else if (prog->startp[no] && prog->endp[no]) {
			len = prog->endp[no] - prog->startp[no];
			strncpy (dst, prog->startp[no], len);
			dst += len;
			if (len != 0 && dst[-1] == '\0') {
				/* strncpy hit NUL. */
				/* regerror("damaged match string"); */
				return 0;
			}
		}
	}
	*dst = '\0';
	return 1;
}
