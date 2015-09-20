%option noinput
%option nounput
%option noyywrap
%{
/*-
 * Copyright (c) 1980, 1993
 *  The Regents of the University of California.  All rights reserved.
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
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
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
 *
 *  @(#)lang.l  8.1 (Berkeley) 6/6/93
 */
#include <ctype.h>
#include "y.tab.h"
#include "config.h"

#define tprintf if (do_trace) printf

/*
 * Key word table
 */
struct kt {
    char *kt_name;
    int kt_val;
};

static struct kt key_words[] = {
    { "and",            AND },
    { "architecture",   ARCHITECTURE },
    { "at",             AT },
    { "board",          BOARD },
    { "config",         CONFIG },
    { "controller",     CONTROLLER },
    { "cpu",            CPU },
    { "csr",            CSR },
    { "device",         DEVICE },
    { "drive",          DRIVE },
    { "dst",            DST },
    { "dumps",          DUMPS },
    { "flags",          FLAGS },
    { "invert",         INVERT },
    { "ldscript",       LDSCRIPT },
    { "major",          MAJOR },
    { "makeoptions",    MAKEOPTIONS },
    { "maxusers",       MAXUSERS },
    { "minor",          MINOR },
    { "on",             ON },
    { "options",        OPTIONS },
    { "pin",            PINS },
    { "pins",           PINS },
    { "priority",       PRIORITY },
    { "root",           ROOT },
    { "sequential",     SEQUENTIAL },
    { "service",        SERVICE },
    { "signal",         SIGNAL },
    { "size",           SIZE },
    { "swap",           SWAP },
    { "tape",           DEVICE },
    { "timezone",       TIMEZONE },
    { "trace",          TRACE },
    { "vector",         VECTOR },
    { 0, 0 },
};

/*
 * Look up a string in the keyword table.  Returns a -1 if the
 * string is not a keyword otherwise it returns the keyword number
 */
static int
kw_lookup(word)
    register char *word;
{
    register struct kt *kp;

    for (kp = key_words; kp->kt_name != 0; kp++)
        if (eq(word, kp->kt_name))
            return kp->kt_val;
    return -1;
}
%}
WORD            [A-Za-z_][-A-Za-z_]*
PIN             R[A-Z][0-9][0-9]*
%%
{PIN}           {
                    /* Pin name from RA0 to RZ255. */
                    yylval.val = atoi(yytext + 2) & 0xff;
                    yylval.val |= (yytext[1] - 'A' + 1) << 8;
                    return PIN;
                }
{WORD}          {
                    int i;

                    if ((i = kw_lookup(yytext)) == -1)
                    {
                        yylval.str = yytext;
                        tprintf("id(%s) ", yytext);
                        return ID;
                    }
                    tprintf("(%s) ", yytext);
                    return i;
                }
\"[^"]+\"       {
                    yytext[strlen(yytext)-1] = '\0';
                    yylval.str = yytext + 1;
                    return ID;
                }
0[0-7]*         {
                    yylval.val = strtoul(yytext, 0, 8);
                    tprintf("#O:%o ", yylval.val);
                    return NUMBER;
                }
0x[0-9a-fA-F]+  {
                    yylval.val = strtoul(yytext, 0, 16);
                    tprintf("#X:%x ", yylval.val);
                    return NUMBER;
                }
[1-9][0-9]*     {
                    yylval.val = atoi(yytext);
                    tprintf("#D:%d ", yylval.val);
                    return NUMBER;
                }
[0-9]"."[0-9]*  {
                    yylval.val = (int) (60 * atof(yytext) + 0.5);
                    return FPNUMBER;
                }
"-"             {
                    return MINUS;
                }
"?"             {
                    yylval.val = -1;
                    tprintf("? ");
                    return NUMBER;
                }
\n/[ \t]        {
                    yyline++;
                    tprintf("\n... ");
                }
\n              {
                    yyline++;
                    tprintf("\n");
                    return SEMICOLON;
                }
#.*             {   /* Ignored (comment) */; }
[ \t]*          {   /* Ignored (white space) */; }
";"             {   return SEMICOLON;   }
","             {   return COMMA;       }
"="             {   return EQUALS;      }
"@"             {   return AT;          }
.               {   return yytext[0];   }

%%
