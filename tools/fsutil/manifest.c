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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fts.h>
#include <sys/stat.h>
#include "bsdfs.h"
#include "manifest.h"

/*
 * Manifest entry.
 */
struct _entry_t {
    entry_t *next;
    int     type;               /* d, f, l, s, b or c */
    int     mode;
    int     owner;
    int     group;
    int     major;
    int     minor;
    char    *link;              /* Target for link or symlink */
    char    path[1];
};

/*
 * Linked list of information for finding hard links.
 */
typedef struct _link_info_t link_info_t;
struct _link_info_t {
    link_info_t *next;
    dev_t       dev;
    ino_t       ino;
    char        path[1];
};

static link_info_t *link_list;  /* List of hard links. */

/*
 * Store information about the hard link: dev, inode and path.
 */
static void keep_link (dev_t dev, ino_t ino, char *path)
{
    link_info_t *info;

    info = (link_info_t*) malloc (strlen (path) + sizeof (link_info_t));
    if (! info) {
        fprintf (stderr, "%s: no memory for link info\n", path);
        return;
    }
    info->dev = dev;
    info->ino = ino;
    strcpy (info->path, path);

    /* Insert into the list. */
    info->next = link_list;
    link_list = info;
}

/*
 * Find a hard link by dev and inode number.
 * Return a path.
 */
static char *find_link (dev_t dev, ino_t ino)
{
    link_info_t *info = link_list;

    for (info=link_list; info; info=info->next) {
        if (info->dev == dev && info->ino == ino)
            return info->path;
    }
    return 0;
}

/*
 * Add new entry to the manifest.
 */
static void add_entry (manifest_t *m, int filetype, char *path, char *link,
    int mode, int owner, int group, int majr, int minr)
{
    entry_t *e;

    e = malloc (sizeof(entry_t) + strlen (path));
    if (! e) {
        fprintf (stderr, "%s: no memory for entry\n", path);
        return;
    }

    e->next = 0;
    e->type = filetype;
    e->mode = mode;
    e->owner = owner;
    e->group = group;
    e->major = majr;
    e->minor = minr;
    e->link = 0;
    e->link = link ? strdup (link) : 0;
    strcpy (e->path, path);

    /* Append to the tail of the list. */
    if (m->first == 0) {
        m->first = e;
    } else {
        m->last->next = e;
    }
    m->last = e;
}

/*
 * Compare two entries of file traverse scan.
 */
static int ftsent_compare (const FTSENT **a, const FTSENT **b)
{
    return strcmp((*a)->fts_name, (*b)->fts_name);
}

/*
 * Scan the directory and create a manifest from it's contents.
 * Return 0 on error.
 */
int manifest_scan (manifest_t *m, const char *dirname)
{
    FTS *dir;
    FTSENT *node;
    char *argv[2], *path, *target, buf[BSDFS_BSIZE];
    struct stat st;
    int prefix_len, mode, len;

    /* Clear manifest header. */
    m->first = 0;
    m->last = 0;
    m->filemode = 0664;
    m->dirmode = 0775;
    m->owner = 0;
    m->group = 0;

    /* Open directory. */
    argv[0] = (char*) dirname;
    argv[1] = 0;
    dir = fts_open (argv, FTS_PHYSICAL | FTS_NOCHDIR, &ftsent_compare);
    if (! dir) {
        fprintf (stderr, "%s: cannot open\n", dirname);
        return 0;
    }
    prefix_len = strlen (dirname);

    printf ("# Manifest for directory %s\n", dirname);
    for (;;) {
        /* Read next directory entry. */
        node = fts_read(dir);
        if (! node)
            break;

        path = node->fts_path + prefix_len;
        if (path[0] == 0)
            continue;

        st = *node->fts_statp;
        mode = st.st_mode & 07777;

        switch (node->fts_info) {
        case FTS_D:
            /* Directory. */
            add_entry (m, 'd', path, 0, mode, st.st_uid, st.st_gid, 0, 0);
            break;

        case FTS_F:
            /* Regular file. */
            if (st.st_nlink > 1) {
                /* Hard link to file. */
                target = find_link (st.st_dev, st.st_ino);
                if (target) {
                    add_entry (m, 'l', path, target, mode, st.st_uid, st.st_gid, 0, 0);
                    break;
                }
                keep_link (st.st_dev, st.st_ino, path);
            }
            add_entry (m, 'f', path, 0, mode, st.st_uid, st.st_gid, 0, 0);
            break;

        case FTS_SL:
            /* Symlink. */
            if (st.st_nlink > 1) {
                /* Hard link to symlink. */
                target = find_link (st.st_dev, st.st_ino);
                if (target) {
                    add_entry (m, 'l', path, target, mode, st.st_uid, st.st_gid, 0, 0);
                    break;
                }
                keep_link (st.st_dev, st.st_ino, path);
            }
            /* Get the target of symlink. */
            len = readlink (node->fts_accpath, buf, sizeof(buf) - 1);
            if (len < 0) {
                fprintf (stderr, "%s: cannot read\n", node->fts_accpath);
                break;
            }
            buf[len] = 0;
            add_entry (m, 's', path, buf, mode, st.st_uid, st.st_gid, 0, 0);
            break;

        default:
            /* Ignore all other variants. */
            break;
        }
    }
    fts_close (dir);
    return 1;
}

/*
 * Dump the manifest to a text file.
 */
void manifest_print (manifest_t *m)
{
    void *cursor;
    char *path, *link;
    int filetype, mode, owner, group, major, minor;

    cursor = 0;
    while ((filetype = manifest_iterate (m, &cursor, &path, &link, &mode,
        &owner, &group, &major, &minor)) != 0)
    {
        switch (filetype) {
        case 'd':
            /* Directory. */
            printf ("\ndir %s\n", path);
            break;
        case 'f':
            /* Regular file. */
            printf ("\nfile %s\n", path);
            break;
        case 'l':
            /* Hard link to file. */
            printf ("\nlink %s\n", path);
            printf ("target %s\n", link);
            continue;
        case 's':
            /* Symlink. */
            printf ("\nsymlink %s\n", path);
            printf ("target %s\n", link);
            break;
        case 'b':
            /* Block device. */
            printf ("\nbdev %s\n", path);
            printf ("major %u\n", major);
            printf ("minor %u\n", minor);
            break;
        case 'c':
            /* Character device. */
            printf ("\ncdev %s\n", path);
            printf ("major %u\n", major);
            printf ("minor %u\n", minor);
            break;
        default:
            /* Ignore all other variants. */
            continue;
        }
        printf ("mode %#o\n", mode);
        printf ("owner %u\n", owner);
        printf ("group %u\n", group);
    }
}

/*
 * Load a manifest from the text file.
 * Return 0 on error.
 */
int manifest_load (manifest_t *m, const char *filename)
{
    FILE *fd;
    char line [1024], *p, *cmd = 0, *arg = 0, *path = 0, *target = 0;
    int type = 0, default_dirmode = 0775, default_filemode = 0664;
    int default_owner = 0, default_group = 0;
    int mode = -1, owner = -1, group = -1, majr = -1, minr = -1;

    /* Clear manifest header. */
    m->first = 0;
    m->last = 0;
    m->filemode = 0664;
    m->dirmode = 0775;
    m->owner = 0;
    m->group = 0;

    /* Open file. */
    fd = fopen (filename, "r");
    if (! fd) {
        perror (filename);
        return 0;
    }

    /* Parse the manifest file. */
    while (fgets (line, sizeof(line), fd)) {
        /* Remove trailing spaces. */
        p = line + strlen(line);
        while (p-- >line && (*p==' ' || *p == '\t' || *p == '\r' || *p == '\n'))
            *p = 0;

        /* Remove spaces at line beginning. */
        p = line;
        while (*p==' ' || *p == '\t' || *p == '\r' || *p == '\n')
            ++p;
        if (*p == 0)
            continue;

        /* Skip comments. */
        if (*p == '#')
            continue;

        /* Split into command and agrument. */
        cmd = p;
        p = strchr (p, ' ');
        if (p) {
            *p = 0;
            arg = p+1;
        } else {
            arg = "";
        }

        /*
         * Process options.
         */
        if (strcmp ("dirmode", cmd) == 0) {
            if (type != 0) {
notdef:         fprintf (stderr, "%s: command '%s' allowed only in default section\n", filename, cmd);
                fclose (fd);
                return 0;
            }
            default_dirmode = strtoul (arg, 0, 0);
            continue;
        }
        if (strcmp ("filemode", cmd) == 0) {
            if (type != 0)
                goto notdef;
            default_filemode = strtoul (arg, 0, 0);
            continue;
        }
        if (strcmp ("owner", cmd) == 0) {
            if (type != 0)
                owner = strtoul (arg, 0, 0);
            else
                default_owner = strtoul (arg, 0, 0);
            continue;
        }
        if (strcmp ("group", cmd) == 0) {
            if (type != 0)
                group = strtoul (arg, 0, 0);
            else
                default_group = strtoul (arg, 0, 0);
            continue;
        }
        if (strcmp ("mode", cmd) == 0) {
            if (type == 0) {
baddef:         fprintf (stderr, "%s: command '%s' not allowed in default section\n", filename, cmd);
                fclose (fd);
                return 0;
            }
            mode = strtoul (arg, 0, 0);
            continue;
        }
        if (strcmp ("major", cmd) == 0) {
            if (type == 0)
                goto baddef;
            majr = strtoul (arg, 0, 0);
            continue;
        }
        if (strcmp ("minor", cmd) == 0) {
            if (type == 0)
                goto baddef;
            minr = strtoul (arg, 0, 0);
            continue;
        }
        if (strcmp ("target", cmd) == 0) {
            if (type != 'l' && type != 's') {
                fprintf (stderr, "%s: command '%s' allowed only for links\n", filename, cmd);
                fclose (fd);
                return 0;
            }
            target = strdup (arg);
            continue;
        }

        /*
         * End of section: add new object.
         */
        if (type != 0) {
newobj:     /* Check parameters. */
            if ((type == 'l' || type == 's') && ! target) {
                fprintf (stderr, "%s: target missing for link '%s'\n", filename, path);
                fclose (fd);
                return 0;
            }
            if ((type == 'b' || type == 'c') && majr == -1) {
                fprintf (stderr, "%s: major index missing for device '%s'\n", filename, path);
                fclose (fd);
                return 0;
            }
            if ((type == 'b' || type == 'c') && minr == -1) {
                fprintf (stderr, "%s: minor index missing for device '%s'\n", filename, path);
                fclose (fd);
                return 0;
            }
            if (mode == -1)
                mode = (type == 'd') ? default_dirmode : default_filemode;
            if (owner == -1)
                owner = default_owner;
            if (group == -1)
                group = default_group;

            /* Create new object. */
            add_entry (m, type, path, target, mode, owner, group, majr, minr);

            /* Set parameters to defaults. */
            type = 0;
            free (path);
            path = 0;
            if (target) {
                free (target);
                target = 0;
            }
            mode = -1;
            owner = -1;
            group = -1;
            majr = -1;
            minr = -1;
            if (! fd) {
                /* Last object done. */
                return 1;
            }
        }

        /*
         * Start new section.
         */
        if (strcmp ("default", cmd) == 0) {
            type = 0;
        }
        else if (strcmp ("dir", cmd) == 0) {
            type = 'd';
            path = strdup (arg);
        }
        else if (strcmp ("file", cmd) == 0) {
            type = 'f';
            path = strdup (arg);
        }
        else if (strcmp ("link", cmd) == 0) {
            type = 'l';
            path = strdup (arg);
        }
        else if (strcmp ("symlink", cmd) == 0) {
            type = 's';
            path = strdup (arg);
        }
        else if (strcmp ("bdev", cmd) == 0) {
            type = 'b';
            path = strdup (arg);
        }
        else if (strcmp ("cdev", cmd) == 0) {
            type = 'c';
            path = strdup (arg);
        }
        else {
            fprintf (stderr, "%s: unknown command '%s'\n", filename, cmd);
            fclose (fd);
            return 0;
        }
    }

    /* Done. */
    fclose (fd);
    if (type != 0) {
        /* Create last object. */
        fd = 0;
        goto newobj;
    }
    return 1;
}

/*
 * Iterate through the manifest.
 */
int manifest_iterate (manifest_t *m, void **last, char **path, char **link,
    int *mode, int *owner, int *group, int *major, int *minor)
{
    /* Get the next entry. */
    entry_t *e = *last ? ((entry_t*)*last)->next : m->first;

    if (! e)
        return 0;

    /* Fetch information about this entry. */
    *last = (void*) e;  /* Pointer to a last processed entry. */
    *path = e->path;
    *link = e->link;
    *mode = (e->mode != -1) ? e->mode : (e->type == 'd') ? m->dirmode : m->filemode;
    *owner = (e->owner != -1) ? e->owner : m->owner;
    *group = (e->group != -1) ? e->group : m->group;
    *major = e->major;
    *minor = e->minor;
    return e->type;
}
