/*
 * Parsing INI-style configuration files. The routines are taken and
 * modified from SMB source code (http://samba.anu.edu.au/cifs).
 *
 * Copyright (C) 2009-2012 Serge Vakulenko <serge@vak.ru>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *   3. The name of the author may not be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "config.h"

static const char *confname;
static char *bufr;
static int bsize;
static char *cursec;

/*
 * Scan to the end of a comment.
 */
static int eat_comment (FILE *fp)
{
    int c;

    c = getc (fp);
    while (c > 0 && c != '\n')
        c = getc (fp);
    return c;
}

/*
 * Skip whitespaces to end of line.
 */
static int eat_whitespace (FILE *fp)
{
    int c;

    c = getc (fp);
    while (isspace(c) && c != '\n')
        c = getc (fp);
    return c;
}

/*
 * Search for continuation backshash, starting from line end,
 * When found, return it's index.
 * When no continuation, return -1.
 */
static int find_continuation (char *line, int pos)
{
    pos--;
    while (pos >= 0 && isspace (line [pos]))
        pos--;
    if (pos >= 0 && line[pos] == '\\')
        return pos;
    /* No continuation. */
    return -1;
}

/*
 * Scan a parameter name (or name and value pair) and pass the value (or
 * values) to function pfunc().
 */
static void parse_parameter (FILE *fp,
    void (*pfunc) (void*, char*, char*, char*),
    void *arg,
    int c)
{
    int i = 0;      /* position withing bufr */
    int end = 0;        /* bufr[end] is current end-of-string */
    int vstart = 0;     /* starting position of the parameter */

    /* Loop until we found the start of the value */
    while (vstart == 0) {
        /* Ensure there's space for next char */
        if (i > (bsize-2)) {
            bsize += 1024;
            bufr = realloc (bufr, bsize);
            if (! bufr) {
                fprintf (stderr, "%s: malloc failed\n", confname);
                exit (-1);
            }
        }
        switch (c) {
        case '=':
            if (end == 0) {
                fprintf (stderr, "%s: invalid parameter name\n", confname);
                exit (-1);
            }
            bufr[end++] = '\0';
            i = end;
            vstart = end;
            bufr[i] = '\0';
            break;

        case ';':           /* comment line */
        case '#':
            c = eat_comment (fp);
        case '\n':
            i = find_continuation (bufr, i);
            if (i < 0) {
                /* End of line, but no assignment symbol. */
                bufr[end]='\0';
                fprintf (stderr, "%s: bad line, ignored: `%s'\n",
                    confname, bufr);
                return;
            }
            end = ((i > 0) && (bufr[i-1] == ' ')) ? (i-1) : (i);
            c = getc (fp);
            break;

        case '\0':
        case EOF:
            bufr[i] = '\0';
            fprintf (stderr, "%s: unexpected end-of-file at %s: func\n",
                confname, bufr);
            exit (-1);

        default:
            if (isspace (c)) {
                bufr[end] = ' ';
                i = end + 1;
                c = eat_whitespace (fp);
            } else {
                bufr[i++] = c;
                end = i;
                c = getc (fp);
            }
            break;
        }
    }

    /* Now parse the value */
    c = eat_whitespace (fp);
    while (c > 0) {
        if (i > (bsize-2)) {
            bsize += 1024;
            bufr = realloc (bufr, bsize);
            if (! bufr) {
                fprintf (stderr, "%s: malloc failed\n", confname);
                exit (-1);
            }
        }
        switch(c) {
        case '\r':
            c = getc (fp);
            break;

        case ';':           /* comment line */
        case '#':
            c = eat_comment (fp);
        case '\n':
            i = find_continuation (bufr, i);
            if (i < 0)
                c = 0;
            else {
                for (end=i; (end >= 0) && isspace (bufr[end]); end--)
                    ;
                c = getc (fp);
            }
            break;

        default:
            bufr[i++] = c;
            if (! isspace (c))
                end = i;
            c = getc (fp);
            break;
        }
    }
    bufr[end] = '\0';
    pfunc (arg, cursec, bufr, &bufr [vstart]);
}

/*
 * Scan a section name and remember it in `cursec'.
 */
static void parse_section (FILE *fp)
{
    int c, i, end;

    /* We've already got the '['. Scan past initial white space. */
    c = eat_whitespace (fp);
    i = 0;
    end = 0;
    while (c > 0) {
        if (i > (bsize-2)) {
            bsize += 1024;
            bufr = realloc (bufr, bsize);
            if (! bufr) {
                fprintf (stderr, "%s: malloc failed\n", confname);
                exit (-1);
            }
        }
        switch (c) {
        case ']':       /* found the closing bracked */
            bufr[end] = '\0';
            if (end == 0) {
                fprintf (stderr, "%s: empty section name\n", confname);
                exit (-1);
            }
            /* Register a section. */
            if (cursec)
                free (cursec);
            cursec = strdup (bufr);

            eat_comment (fp);
            return;

        case '\n':
            i = find_continuation (bufr, i);
            if (i < 0) {
                bufr [end] = 0;
                fprintf (stderr, "%s: invalid line: '%s'\n",
                    confname, bufr);
                exit (-1);
            }
            end = ((i > 0) && (bufr[i-1] == ' ')) ? (i-1) : (i);
            c = getc (fp);
            break;

        default:
            if (isspace (c)) {
                bufr[end] = ' ';
                i = end + 1;
                c = eat_whitespace (fp);
            } else {
                bufr[i++] = c;
                end = i;
                c = getc (fp);
            }
            break;
        }
    }
}

/*
 * Process the named parameter file
 */
void conf_parse (const char *filename,
    void (*pfunc) (void*, char*, char*, char*),
    void *arg)
{
    FILE *fp;
    int c;

    confname = filename;
    fp = fopen (filename, "r");
    if (! fp) {
        fprintf (stderr, "%s: unable to open config file\n", filename);
        exit (-1);
    }
    bsize = 1024;
    bufr = (char*) malloc (bsize);
    if (! bufr) {
        fprintf (stderr, "%s: malloc failed\n", confname);
        fclose (fp);
        exit (-1);
    }

    /* Parse file. */
    c = eat_whitespace (fp);
    while (c > 0) {
        switch (c) {
        case '\n':          /* blank line */
            c = eat_whitespace (fp);
            break;
        case ';':           /* comment line */
        case '#':
            c = eat_comment (fp);
            break;
        case '[':           /* section header */
            parse_section (fp);
            c = eat_whitespace (fp);
            break;
        case '\\':          /* bogus backslash */
            c = eat_whitespace (fp);
            break;
        default:            /* parameter line */
            parse_parameter (fp, pfunc, arg, c);
            c = eat_whitespace (fp);
            break;
        }
    }
    fclose (fp);
    if (cursec) {
        free (cursec);
        cursec = 0;
    }
    free (bufr);
    bufr = 0;
    bsize = 0;
}
