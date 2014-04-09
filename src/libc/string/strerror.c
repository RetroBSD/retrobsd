/*
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <string.h>
#include <sys/sysctl.h>
#include <machine/cpu.h>

char *
strerror(errnum)
	register int errnum;
{
        static char msgstr[64];
	int mib[3];
	size_t size;

        /* Read an error message from kernel to a static buffer. */
	mib[0] = CTL_MACHDEP;
	mib[1] = CPU_ERRMSG;
	mib[2] = errnum;
	size = sizeof (msgstr);
	if (sysctl(mib, 3, msgstr, &size, NULL, 0) == -1) {
                /* Do this by hand, so we don't include stdio(3). */
                static const char unknown[] = "Unknown error: ";
                register char *p, *t;
                const char *q;
                char tmp[20];

                t = tmp;
                do {
                        *t++ = '0' + ((unsigned)errnum % 10);
                        errnum = (unsigned)errnum / 10;
                } while (errnum != 0);

                p = msgstr;
                for (q=unknown; *q; q++) {
                        *p++ = *q;
                }
                do {
                        *p++ = *--t;
                } while (t > tmp);
                *p = 0;
        }
	return msgstr;
}
