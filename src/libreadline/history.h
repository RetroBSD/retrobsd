/*
 * Guerrilla line editing library against the idea that a line editing lib
 * needs to be 20,000 lines of C code.
 *
 * Based on linenoise.c with API modified for compatibility with
 * traditional readline library.
 *
 * ------------------------------------------------------------------------
 *
 * Copyright (c) 2010-2014, Salvatore Sanfilippo <antirez at gmail dot com>
 * Copyright (c) 2010-2013, Pieter Noordhuis <pcnoordhuis at gmail dot com>
 * Copyright (c) 2015, Serge Vakulenko <serge at vak dot ru>
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *  *  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *  *  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __HISTORY_H
#define __HISTORY_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Place STRING at the end of the history list.
 * The associated data field (if any) is set to NULL.
 */
void add_history(const char *line);

/*
 * Set the maximum length of the current history array.
 */
int history_set_length(int len);

/*
 * Add the contents of FILENAME to the history list, a line at a time.
 * If FILENAME is NULL, then read from ~/.history.  Returns 0 if
 * successful, or errno if not.
 */
int read_history(const char *filename);

/*
 * Write the current history to FILENAME.  If FILENAME is NULL,
 * then write the history list to ~/.history.  Values returned
 * are as in read_history ().
 */
int write_history(const char *filename);

#ifdef __cplusplus
}
#endif

#endif /* __HISTORY_H */
