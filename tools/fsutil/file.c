/*
 * File i/o routines for 2.xBSD filesystem.
 *
 * Copyright (C) 2006-2011 Serge Vakulenko, <serge@vak.ru>
 *
 * This file is part of RetroBSD project, which is distributed
 * under the terms of the GNU General Public License (GPL).
 * See the accompanying file "COPYING" for more details.
 */
#include <stdio.h>
#include <string.h>
#include "bsdfs.h"

extern int verbose;

int fs_file_create (fs_t *fs, fs_file_t *file, char *name, int mode)
{
	if (! fs_inode_by_name (fs, &file->inode, name, 1, mode)) {
		fprintf (stderr, "%s: inode open failed\n", name);
		return 0;
	}
	if ((file->inode.mode & INODE_MODE_FMT) == INODE_MODE_FDIR) {
		/* Cannot open directory on write. */
		return 0;
	}
	fs_inode_truncate (&file->inode);
	fs_inode_save (&file->inode, 0);
	file->writable = 1;
	file->offset = 0;
	return 1;
}

int fs_file_open (fs_t *fs, fs_file_t *file, char *name, int wflag)
{
	if (! fs_inode_by_name (fs, &file->inode, name, 0, 0)) {
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
		fprintf (stderr, "inode %d: file write failed\n",
			file->inode.number);
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
		fprintf (stderr, "inode %d: error writing %lu bytes at offset %lu\n",
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
