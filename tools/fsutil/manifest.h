/*
 * Routines to handle manifest files.
 *
 * Copyright (C) 2014 Serge Vakulenko, <serge@vak.ru>
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

/*
 * Manifest header holds a linked list of file entries.
 */
typedef struct _manifest_t manifest_t;
typedef struct _entry_t entry_t;
struct _manifest_t {
    entry_t *first;
    entry_t *last;
    int     filemode;           /* Default mode for files */
    int     dirmode;            /* Default mode for directories */
    int     owner;              /* Default owner */
    int     group;              /* Default group */
};

/*
 * Manifest entry.
 */

/*
 * Scan the directory and create a manifest from it's contents.
 * Return 0 on error.
 */
int manifest_scan (manifest_t *m, const char *dirname);

/*
 * Load a manifest from the text file.
 * Return 0 on error.
 */
int manifest_load (manifest_t *m, const char *filename);

/*
 * Dump the manifest to a text file.
 */
void manifest_print (manifest_t *m);

/*
 * Iterate through the manifest.
 * Example:
 *      void *cursor = 0;
 *      char *path, *link;
 *      int filetype, mode, owner, group, major, minor;
 *
 *      while ((filetype = manifest_iterate (m, &cursor, &path, &link,
 *          &mode, &owner, &group, &major, &minor)) != 0)
 *      {
 *          switch (filetype) {
 *          case 'd': // directory
 *          case 'f': // regular file
 *          case 'l': // hard link
 *          case 's': // symlink
 *          case 'b': // block device
 *          case 'c': // char device
 *          }
 *      }
 */
int manifest_iterate (manifest_t *m, void **cursor, char **path, char **link,
    int *mode, int *owner, int *group, int *major, int *minor);
