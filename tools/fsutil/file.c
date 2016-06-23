/*
 * File i/o routines for 2.xBSD filesystem.
 *
 * Copyright (C) 2006-2014 Serge Vakulenko, <serge@vak.ru>
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "bsdfs.h"

extern int verbose;

int fs_file_create (fs_t *fs, fs_file_t *file, const char *name, int mode)
{
    if (! fs_inode_create (fs, &file->inode, name, mode)) {
        fprintf (stderr, "%s: inode open failed\n", name);
        return 0;
    }
    if ((file->inode.mode & INODE_MODE_FMT) == INODE_MODE_FDIR) {
        /* Cannot open directory on write. */
        return 0;
    }
    fs_inode_truncate (&file->inode, 0);
    fs_inode_save (&file->inode, 0);
    file->writable = 1;
    file->offset = 0;
    return 1;
}

int fs_file_open (fs_t *fs, fs_file_t *file, const char *name, int wflag)
{
    if (! fs_inode_lookup (fs, &file->inode, name)) {
        fprintf (stderr, "%s: inode open failed\n", name);
        return 0;
    }
    if (wflag && (file->inode.mode & INODE_MODE_FMT) == INODE_MODE_FDIR) {
        /* Cannot open directory on write. */
        return 0;
    }
    file->writable = wflag;
    file->offset = 0;
    return 1;
}

int fs_file_read (fs_file_t *file, unsigned char *data, unsigned long bytes)
{
    if (! fs_inode_read (&file->inode, file->offset, data, bytes)) {
        fprintf (stderr, "inode %d: file read failed, %lu bytes at offset %lu\n",
            file->inode.number, bytes, file->offset);
        return 0;
    }
    file->offset += bytes;
    return 1;
}

int fs_file_write (fs_file_t *file, unsigned char *data, unsigned long bytes)
{
    if (! file->writable)
        return 0;
    if (! fs_inode_write (&file->inode, file->offset, data, bytes)) {
        fprintf (stderr, "inode %d: file write failed, %lu bytes at offset %lu\n",
            file->inode.number, bytes, file->offset);
        return 0;
    }
    file->offset += bytes;
    return 1;
}

int fs_file_close (fs_file_t *file)
{
    if (file->writable) {
        if (! fs_inode_save (&file->inode, 0)) {
            fprintf (stderr, "inode %d: file close failed\n",
                file->inode.number);
            return 0;
        }
    }
    return 1;
}
