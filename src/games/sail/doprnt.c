/*
Copyright (c) 2013, Alexey Frunze
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the FreeBSD Project.
*/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

int _doprnt (char const *fmt, va_list pp, FILE *stream)
{
    int cnt = 0;
    const char* p;
    const char* phex = "0123456789abcdef";
    char s[1/*sign*/+10/*magnitude*/+1/*\0*/]; // up to 11 octal digits in 32-bit numbers
    char* pc;
    int n, sign, msign;
    int minlen = 0, len;
    int leadchar;

    for (p = fmt; *p != '\0'; p++)
    {
        if (*p != '%' || p[1] == '%')
        {
            fputc(*p, stream);
            p = p + (*p == '%');
            cnt++;
            continue;
        }
        p++;
        minlen = 0;
        msign = 0;
        if (*p == '+') { msign = 1; p++; }
        else if (*p == '-') { msign = -1; p++; }
        leadchar = ' ';
        if (*p >= '0' && *p <= '9')
        {
            if (*p == '0')
                leadchar = '0';
            while (*p >= '0' && *p <= '9')
                minlen = minlen * 10 + *p++ - '0';
            if (msign < 0)
                minlen = -minlen;
            msign = 0;
        }
        if (!msign)
        {
            if (*p == '+') { msign = 1; p++; }
            else if (*p == '-') { msign = -1; p++; }
        }
        switch (*p)
        {
        case 'c':
            while (minlen > 1) { fputc(' ', stream); cnt++; minlen--; }
            fputc(va_arg(pp, int), stream);
            while (-minlen > 1) { fputc(' ', stream); cnt++; minlen++; }
            cnt++;
            break;
        case 's':
            pc = va_arg(pp, char*);
            len = 0;
            if (pc)
                len = strlen(pc);
            while (minlen > len) { fputc(' ', stream); cnt++; minlen--; }
            if (len)
                while (*pc != '\0')
                {
                    fputc(*pc++, stream);
                    cnt++;
                }
            while (-minlen > len) { fputc(' ', stream); cnt++; minlen++; }
            break;
        case 'i':
        case 'd':
            pc = &s[sizeof s - 1];
            *pc = '\0';
            len = 0;
            n = va_arg(pp, int);
            sign = 1 - 2 * (n < 0);
            do
            {
                *--pc = '0' + (n - n / 10 * 10) * sign;
                n = n / 10;
                len++;
            } while (n);
            if (sign < 0)
            {
                *--pc = '-';
                len++;
            }
            else if (msign > 0)
            {
                *--pc = '+';
                len++;
                msign = 0;
            }
            while (minlen > len) { fputc(leadchar, stream); cnt++; minlen--; }
            while (*pc != '\0')
            {
                fputc(*pc++, stream);
                cnt++;
            }
            while (-minlen > len) { fputc(' ', stream); cnt++; minlen++; }
            break;
        case 'u':
            pc = &s[sizeof s - 1];
            *pc = '\0';
            len = 0;
            n = va_arg(pp, int);
            do
            {
                unsigned nn = n;
                *--pc = '0' + nn % 10;
                n = nn / 10;
                len++;
            } while (n);
            if (msign > 0)
            {
                *--pc = '+';
                len++;
                msign = 0;
            }
            while (minlen > len) { fputc(leadchar, stream); cnt++; minlen--; }
            while (*pc != '\0')
            {
                fputc(*pc++, stream);
                cnt++;
            }
            while (-minlen > len) { fputc(' ', stream); cnt++; minlen++; }
            break;
        case 'X':
            phex = "0123456789ABCDEF";
            // fallthrough
        case 'p':
        case 'x':
            pc = &s[sizeof s - 1];
            *pc = '\0';
            len = 0;
            n = va_arg(pp, int);
            do
            {
                *--pc = phex[n & 0xF];
                n = (n >> 4) & ((1 << (8 * sizeof n - 4)) - 1); // drop sign-extended bits
                len++;
            } while (n);
            while (minlen > len) { fputc(leadchar, stream); cnt++; minlen--; }
            while (*pc != '\0')
            {
                fputc(*pc++, stream);
                cnt++;
            }
            while (-minlen > len) { fputc(' ', stream); cnt++; minlen++; }
            break;
        case 'o':
            pc = &s[sizeof s - 1];
            *pc = '\0';
            len = 0;
            n = va_arg(pp, int);
            do
            {
                *--pc = '0' + (n & 7);
                n = (n >> 3) & ((1 << (8 * sizeof n - 3)) - 1); // drop sign-extended bits
                len++;
            } while (n);
            while (minlen > len) { fputc(leadchar, stream); cnt++; minlen--; }
            while (*pc != '\0')
            {
                fputc(*pc++, stream);
                cnt++;
            }
            while (-minlen > len) { fputc(' ', stream); cnt++; minlen++; }
            break;
        default:
            return -1;
        }
    }

    return cnt;
}
