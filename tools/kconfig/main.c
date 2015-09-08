/*
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
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <ctype.h>
#include "y.tab.h"
#include "config.h"

/*
 * Config builds a set of files for building a UNIX
 * system given a description of the desired system.
 */
int main(argc, argv)
    int argc;
    char **argv;
{
    int ch;

    while ((ch = getopt(argc, argv, "g")) != EOF)
        switch (ch) {
        case 'g':
            debugging++;
            break;
        case '?':
        default:
            goto usage;
        }
    argc -= optind;
    argv += optind;

    if (argc != 1) {
usage:  fputs("usage: kconfig [-gp] sysname\n", stderr);
        exit(1);
    }

    if (! freopen(*argv, "r", stdin)) {
        perror(*argv);
        exit(2);
    }

    dtab = NULL;
    confp = &conf_list;
    compp = &comp_list;
    if (yyparse())
        exit(3);

    switch (arch) {

    case ARCH_PIC32:
        pic32_ioconf();
        break;

    default:
        printf("Specify architecture, e.g. ``architecture pic32''\n");
        exit(1);
    }
    makefile();             /* build Makefile */
    swapconf();             /* swap config files */
    exit(0);
}

/*
 * get_word
 *  returns EOF on end of file
 *  NULL on end of line
 *  pointer to the word otherwise
 */
char *
get_word(fp)
    register FILE *fp;
{
    static char line[80];
    register int ch;
    register char *cp;

    while ((ch = getc(fp)) != EOF)
        if (ch != ' ' && ch != '\t')
            break;
    if (ch == EOF)
        return ((char *)EOF);
    if (ch == '\n')
        return (NULL);
    cp = line;
    *cp++ = ch;
    while ((ch = getc(fp)) != EOF) {
        if (isspace(ch))
            break;
        *cp++ = ch;
    }
    *cp = 0;
    if (ch == EOF)
        return ((char *)EOF);
    (void) ungetc(ch, fp);
    return (line);
}

/*
 * get_quoted_word
 *  like get_word but will accept something in double or single quotes
 *  (to allow embedded spaces).
 */
char *
get_quoted_word(fp)
    register FILE *fp;
{
    static char line[256];
    register int ch;
    register char *cp;

    while ((ch = getc(fp)) != EOF)
        if (ch != ' ' && ch != '\t')
            break;
    if (ch == EOF)
        return ((char *)EOF);
    if (ch == '\n')
        return (NULL);
    cp = line;
    if (ch == '"' || ch == '\'') {
        register int quote = ch;

        while ((ch = getc(fp)) != EOF) {
            if (ch == quote)
                break;
            if (ch == '\n') {
                *cp = 0;
                printf("config: missing quote reading `%s'\n",
                    line);
                exit(2);
            }
            *cp++ = ch;
        }
    } else {
        *cp++ = ch;
        while ((ch = getc(fp)) != EOF) {
            if (isspace(ch))
                break;
            *cp++ = ch;
        }
        if (ch != EOF)
            (void) ungetc(ch, fp);
    }
    *cp = 0;
    if (ch == EOF)
        return ((char *)EOF);
    return (line);
}
