/*
 * Utility for dealing with 2.xBSD filesystem images.
 *
 * Copyright (C) 2006-2011 Serge Vakulenko, <serge@vak.ru>
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
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include <getopt.h>
#include <fts.h>
#include "bsdfs.h"
#include "manifest.h"

int verbose;
int extract;
int add;
int newfs;
int check;
int fix;
int mount;
int scan;
unsigned kbytes;
unsigned swap_kbytes;

static const char *program_version =
    "BSD 2.x file system utility, version 1.1\n"
    "Copyright (C) 2011-2014 Serge Vakulenko";

static const char *program_bug_address = "<serge@vak.ru>";

static struct option program_options[] = {
    { "help",       no_argument,        0,  'h' },
    { "version",    no_argument,        0,  'V' },
    { "verbose",    no_argument,        0,  'v' },
    { "add",        no_argument,        0,  'a' },
    { "extract",    no_argument,        0,  'x' },
    { "check",      no_argument,        0,  'c' },
    { "fix",        no_argument,        0,  'f' },
    { "mount",      no_argument,        0,  'm' },
    { "scan",       no_argument,        0,  'S' },
    { "new",        required_argument,  0,  'n' },
    { "swap",       required_argument,  0,  's' },
    { "manifest",   required_argument,  0,  'M' },
    { 0 }
};

static void print_help (char *progname)
{
    char *p = strrchr (progname, '/');
    if (p)
        progname = p+1;

    printf ("%s\n", program_version);
    printf ("This program is free software; it comes with ABSOLUTELY NO WARRANTY;\n"
            "see the BSD 3-Clause License for more details.\n");
    printf ("\n");
    printf ("Usage:\n");
    printf ("  %s [--verbose] filesys.img\n", progname);
    printf ("  %s --check [--fix] filesys.img\n", progname);
    printf ("  %s --new=kbytes [--swap=kbytes] [--manifest=file] filesys.img [dir]\n", progname);
    printf ("  %s --mount filesys.img dir\n", progname);
    printf ("  %s --add filesys.img files...\n", progname);
    printf ("  %s --extract filesys.img\n", progname);
    printf ("  %s --scan dir > file\n", progname);
    printf ("\n");
    printf ("Options:\n");
    printf ("  -c, --check         Check filesystem, use -c -f to fix.\n");
    printf ("  -f, --fix           Fix bugs in filesystem.\n");
    printf ("  -n NUM, --new=NUM   Create new filesystem, size in kbytes.\n");
    printf ("                      Add files from specified directory (optional)\n");
    printf ("  -s NUM, --swap=NUM  Size of swap area in kbytes.\n");
    printf ("  -M file, --manifest=file  List of files and attributes to create.\n");
    printf ("  -m, --mount         Mount the filesystem.\n");
    printf ("  -a, --add           Add files to filesystem.\n");
    printf ("  -x, --extract       Extract all files.\n");
    printf ("  -S, --scan          Create a manifest from directory contents.\n");
    printf ("  -v, --verbose       Be verbose.\n");
    printf ("  -V, --version       Print version information and then exit.\n");
    printf ("  -h, --help          Print this message.\n");
    printf ("\n");
    printf ("Report bugs to \"%s\".\n", program_bug_address);
}

void print_inode (fs_inode_t *inode,
    char *dirname, char *filename, FILE *out)
{
    fprintf (out, "%s/%s", dirname, filename);
    switch (inode->mode & INODE_MODE_FMT) {
    case INODE_MODE_FDIR:
        if (filename[0] != 0)
            fprintf (out, "/");
        break;
    case INODE_MODE_FCHR:
        fprintf (out, " - char %d %d",
            inode->addr[1] >> 8, inode->addr[1] & 0xff);
        break;
    case INODE_MODE_FBLK:
        fprintf (out, " - block %d %d",
            inode->addr[1] >> 8, inode->addr[1] & 0xff);
        break;
    default:
        fprintf (out, " - %lu bytes", inode->size);
        break;
    }
    fprintf (out, "\n");
}

void print_indirect_block (fs_t *fs, unsigned int bno, FILE *out)
{
    unsigned short nb;
    unsigned char data [BSDFS_BSIZE];
    int i;

    fprintf (out, " [%d]", bno);
    if (! fs_read_block (fs, bno, data)) {
        fprintf (stderr, "read error at block %d\n", bno);
        return;
    }
    for (i=0; i<BSDFS_BSIZE-2; i+=2) {
        nb = data [i+1] << 8 | data [i];
        if (nb)
            fprintf (out, " %d", nb);
    }
}

void print_double_indirect_block (fs_t *fs, unsigned int bno, FILE *out)
{
    unsigned short nb;
    unsigned char data [BSDFS_BSIZE];
    int i;

    fprintf (out, " [%d]", bno);
    if (! fs_read_block (fs, bno, data)) {
        fprintf (stderr, "read error at block %d\n", bno);
        return;
    }
    for (i=0; i<BSDFS_BSIZE-2; i+=2) {
        nb = data [i+1] << 8 | data [i];
        if (nb)
            print_indirect_block (fs, nb, out);
    }
}

void print_triple_indirect_block (fs_t *fs, unsigned int bno, FILE *out)
{
    unsigned short nb;
    unsigned char data [BSDFS_BSIZE];
    int i;

    fprintf (out, " [%d]", bno);
    if (! fs_read_block (fs, bno, data)) {
        fprintf (stderr, "read error at block %d\n", bno);
        return;
    }
    for (i=0; i<BSDFS_BSIZE-2; i+=2) {
        nb = data [i+1] << 8 | data [i];
        if (nb)
            print_indirect_block (fs, nb, out);
    }
}

void print_inode_blocks (fs_inode_t *inode, FILE *out)
{
    int i;

    if ((inode->mode & INODE_MODE_FMT) == INODE_MODE_FCHR ||
        (inode->mode & INODE_MODE_FMT) == INODE_MODE_FBLK)
        return;

    fprintf (out, "    ");
    for (i=0; i<NDADDR; ++i) {
        if (inode->addr[i] == 0)
            continue;
        fprintf (out, " %d", inode->addr[i]);
    }
    if (inode->addr[NDADDR] != 0)
        print_indirect_block (inode->fs, inode->addr[NDADDR], out);
    if (inode->addr[NDADDR+1] != 0)
        print_double_indirect_block (inode->fs,
            inode->addr[NDADDR+1], out);
    if (inode->addr[NDADDR+2] != 0)
        print_triple_indirect_block (inode->fs,
            inode->addr[NDADDR+2], out);
    fprintf (out, "\n");
}

void extract_inode (fs_inode_t *inode, char *path)
{
    int fd, n, mode;
    unsigned long offset;
    unsigned char data [BSDFS_BSIZE];

    /* Allow read/write for user. */
    mode = (inode->mode & 0777) | 0600;
    fd = open (path, O_CREAT | O_RDWR, mode);
    if (fd < 0) {
        perror (path);
        return;
    }
    for (offset = 0; offset < inode->size; offset += BSDFS_BSIZE) {
        n = inode->size - offset;
        if (n > BSDFS_BSIZE)
            n = BSDFS_BSIZE;
        if (! fs_inode_read (inode, offset, data, n)) {
            fprintf (stderr, "%s: read error at offset %ld\n",
                path, offset);
            break;
        }
        if (write (fd, data, n) != n) {
            fprintf (stderr, "%s: write error\n", path);
            break;
        }
    }
    close (fd);
}

void extractor (fs_inode_t *dir, fs_inode_t *inode,
    char *dirname, char *filename, void *arg)
{
    FILE *out = arg;
    char *path, *relpath;

    if (verbose)
        print_inode (inode, dirname, filename, out);

    if ((inode->mode & INODE_MODE_FMT) != INODE_MODE_FDIR &&
        (inode->mode & INODE_MODE_FMT) != INODE_MODE_FREG)
        return;

    path = alloca (strlen (dirname) + strlen (filename) + 2);
    strcpy (path, dirname);
    strcat (path, "/");
    strcat (path, filename);
    for (relpath=path; *relpath == '/'; relpath++)
        continue;

    if ((inode->mode & INODE_MODE_FMT) == INODE_MODE_FDIR) {
        if (mkdir (relpath, 0775) < 0 && errno != EEXIST)
            perror (relpath);
        /* Scan subdirectory. */
        fs_directory_scan (inode, path, extractor, arg);
    } else {
        extract_inode (inode, relpath);
    }
}

void scanner (fs_inode_t *dir, fs_inode_t *inode,
    char *dirname, char *filename, void *arg)
{
    FILE *out = arg;
    char *path;

    print_inode (inode, dirname, filename, out);

    if (verbose > 1) {
        /* Print a list of blocks. */
        print_inode_blocks (inode, out);
        if (verbose > 2) {
            fs_inode_print (inode, out);
            printf ("--------\n");
        }
    }
    if ((inode->mode & INODE_MODE_FMT) == INODE_MODE_FDIR &&
        inode->number != BSDFS_ROOT_INODE) {
        /* Scan subdirectory. */
        path = alloca (strlen (dirname) + strlen (filename) + 2);
        strcpy (path, dirname);
        strcat (path, "/");
        strcat (path, filename);
        fs_directory_scan (inode, path, scanner, arg);
    }
}

/*
 * Create a directory.
 */
void add_directory (fs_t *fs, char *name, int mode, int owner, int group)
{
    fs_inode_t dir, parent;
    char buf [BSDFS_BSIZE], *p;

    /* Open parent directory. */
    strcpy (buf, name);
    p = strrchr (buf, '/');
    if (p)
        *p = 0;
    else
        *buf = 0;
    if (! fs_inode_lookup (fs, &parent, buf)) {
        fprintf (stderr, "%s: cannot open directory\n", buf);
        return;
    }

    /* Create directory. */
    mode &= 07777;
    mode |= INODE_MODE_FDIR;
    int done = fs_inode_create (fs, &dir, name, mode);
    if (! done) {
        fprintf (stderr, "%s: directory inode create failed\n", name);
        return;
    }
    if (done == 1) {
        /* The directory already existed. */
        return;
    }
    dir.uid = owner;
    dir.gid = group;
    fs_inode_save (&dir, 1);

    /* Make parent link '..' */
    strcpy (buf, name);
    strcat (buf, "/..");
    if (! fs_inode_link (fs, &dir, buf, parent.number)) {
        fprintf (stderr, "%s: dotdot link failed\n", name);
        return;
    }
    if (! fs_inode_get (fs, &parent, parent.number)) {
        fprintf (stderr, "inode %d: cannot open parent\n", parent.number);
        return;
    }
    ++parent.nlink;
    fs_inode_save (&parent, 1);
/*printf ("*** inode %d: increment link counter to %d\n", parent.number, parent.nlink);*/
}

/*
 * Create a device node.
 */
void add_device (fs_t *fs, char *name, int mode, int owner, int group,
    int type, int majr, int minr)
{
    fs_inode_t dev;

    mode &= 07777;
    mode |= (type == 'b') ? INODE_MODE_FBLK : INODE_MODE_FCHR;
    if (! fs_inode_create (fs, &dev, name, mode)) {
        fprintf (stderr, "%s: device inode create failed\n", name);
        return;
    }
    dev.addr[1] = majr << 8 | minr;
    dev.uid = owner;
    dev.gid = group;
    time (&dev.mtime);
    fs_inode_save (&dev, 1);
}

/*
 * Copy regular file to filesystem.
 */
void add_file (fs_t *fs, const char *path, const char *dirname,
    int mode, int owner, int group)
{
    fs_file_t file;
    FILE *fd;
    char accpath [BSDFS_BSIZE];
    unsigned char data [BSDFS_BSIZE];
    struct stat st;
    int len;

    if (dirname && *dirname) {
        /* Concatenate directory name and file name. */
        strcpy (accpath, dirname);
        len = strlen (accpath);
        if (accpath[len-1] != '/' && path[0] != '/')
            strcat (accpath, "/");
        strcat (accpath, path);
    } else {
        /* Use filename relative to current directory. */
        strcpy (accpath, path);
    }
    fd = fopen (accpath, "r");
    if (! fd) {
        perror (accpath);
        return;
    }
    fstat (fileno(fd), &st);
    if (mode == -1)
        mode = st.st_mode;
    mode &= 07777;
    mode |= INODE_MODE_FREG;
    if (! fs_file_create (fs, &file, path, mode)) {
        fprintf (stderr, "%s: cannot create\n", path);
        return;
    }
    for (;;) {
        len = fread (data, 1, sizeof (data), fd);
/*      printf ("read %d bytes from %s\n", len, accpath);*/
        if (len < 0)
            perror (accpath);
        if (len <= 0)
            break;
        if (! fs_file_write (&file, data, len)) {
            fprintf (stderr, "%s: write error\n", path);
            break;
        }
    }
    file.inode.uid = owner;
    file.inode.gid = group;
    file.inode.mtime = st.st_mtime;
    file.inode.dirty = 1;
    fs_file_close (&file);
    fclose (fd);
}

/*
 * Create a symlink.
 */
void add_symlink (fs_t *fs, const char *path, const char *link,
    int mode, int owner, int group)
{
    fs_file_t file;
    int len;

    mode &= 07777;
    mode |= INODE_MODE_FLNK;
    if (! fs_file_create (fs, &file, path, mode)) {
        fprintf (stderr, "%s: cannot create\n", path);
        return;
    }
    len = strlen (link);
    if (! fs_file_write (&file, (unsigned char*) link, len)) {
        fprintf (stderr, "%s: write error\n", path);
        return;
    }
    file.inode.uid = owner;
    file.inode.gid = group;
    time (&file.inode.mtime);
    file.inode.dirty = 1;
    fs_file_close (&file);
}

/*
 * Create a hard link.
 */
void add_hardlink (fs_t *fs, const char *path, const char *link)
{
    fs_inode_t source, target;

    /* Find source. */
    if (! fs_inode_lookup (fs, &source, link)) {
        fprintf (stderr, "%s: link source not found\n", link);
        return;
    }
    if ((source.mode & INODE_MODE_FMT) == INODE_MODE_FDIR) {
        fprintf (stderr, "%s: cannot link directories\n", link);
        return;
    }

    /* Create target link. */
    if (! fs_inode_link (fs, &target, path, source.number)) {
        fprintf (stderr, "%s: link failed\n", path);
        return;
    }
    source.nlink++;
    fs_inode_save (&source, 1);
}

/*
 * Create a file/device/directory in the filesystem.
 * When name is ended by slash as "name/", directory is created.
 */
void add_object (fs_t *fs, char *name)
{
    int majr, minr;
    char type;
    char *p;

    if (verbose) {
        printf ("%s\n", name);
    }
    p = strrchr (name, '/');
    if (p && p[1] == 0) {
        *p = 0;
        add_directory (fs, name, 0777, 0, 0);
        return;
    }
    p = strrchr (name, '!');
    if (p) {
        *p++ = 0;
        if (sscanf (p, "%c%d:%d", &type, &majr, &minr) != 3 ||
            (type != 'c' && type != 'b') ||
            majr < 0 || majr > 255 || minr < 0 || minr > 255) {
            fprintf (stderr, "%s: invalid device specification\n", p);
            fprintf (stderr, "expected c<major>:<minor> or b<major>:<minor>\n");
            return;
        }
        add_device (fs, name, 0666, 0, 0, type, majr, minr);
        return;
    }
    add_file (fs, name, 0, -1, 0, 0);
}

/*
 * Add the contents from the specified directory.
 * Use the optional manifest file.
 */
void add_contents (fs_t *fs, const char *dirname, const char *manifest)
{
    manifest_t m;
    void *cursor;
    char *path, *link;
    int filetype, mode, owner, group, majr, minr;
    int ndirs = 0, nfiles = 0, nlinks = 0, nsymlinks = 0, ndevs = 0;

    if (manifest) {
        /* Load manifest from file. */
        if (! manifest_load (&m, manifest)) {
            fprintf (stderr, "%s: cannot read\n", manifest);
            return;
        }
    } else {
        /* Create manifest from directory contents. */
        if (! manifest_scan (&m, dirname)) {
            fprintf (stderr, "%s: cannot read\n", dirname);
            return;
        }
    }

    /* For every file in the manifest,
     * add it to the target filesystem. */
    cursor = 0;
    while ((filetype = manifest_iterate (&m, &cursor, &path, &link, &mode,
        &owner, &group, &majr, &minr)) != 0)
    {
        switch (filetype) {
        case 'd':
            add_directory (fs, path, mode, owner, group);
            ndirs++;
            break;
        case 'f':
            add_file (fs, path, dirname, mode, owner, group);
            nfiles++;
            break;
        case 'l':
            add_hardlink (fs, path, link);
            nlinks++;
            break;
        case 's':
            add_symlink (fs, path, link, mode, owner, group);
            nsymlinks++;
            break;
        case 'b':
            add_device (fs, path, mode, owner, group, 'b', majr, minr);
            ndevs++;
            break;
        case 'c':
            add_device (fs, path, mode, owner, group, 'c', majr, minr);
            ndevs++;
            break;
        }
    }
    fs_sync (fs, 0);
    fs_close (fs);
    printf ("Installed %u directories, %u files, %u devices, %u links, %u symlinks\n",
        ndirs, nfiles, ndevs, nlinks, nsymlinks);
}

int main (int argc, char **argv)
{
    int i, key;
    fs_t fs;
    fs_inode_t inode;
    manifest_t m;
    const char *manifest = 0;

    for (;;) {
        key = getopt_long (argc, argv, "vaxmSn:cfs:M:",
            program_options, 0);
        if (key == -1)
            break;
        switch (key) {
        case 'v':
            ++verbose;
            break;
        case 'a':
            ++add;
            break;
        case 'x':
            ++extract;
            break;
        case 'n':
            ++newfs;
            kbytes = strtol (optarg, 0, 0);
            break;
        case 'c':
            ++check;
            break;
        case 'f':
            ++fix;
            break;
        case 'm':
            ++mount;
            break;
        case 'S':
            ++scan;
            break;
        case 's':
            swap_kbytes = strtol (optarg, 0, 0);
            break;
        case 'M':
            manifest = optarg;
            break;
        case 'V':
            printf ("%s\n", program_version);
            return 0;
        case 'h':
            print_help (argv[0]);
            return 0;
        default:
            print_help (argv[0]);
            return -1;
        }
    }
    i = optind;
    if (extract + newfs + check + add + mount + scan > 1) {
        print_help (argv[0]);
        return -1;
    }

    if (newfs) {
        /* Create new filesystem. */
        if (i != argc-1 && i != argc-2) {
            print_help (argv[0]);
            return -1;
        }
        if (kbytes < BSDFS_BSIZE * 10 / 1024) {
            /* Need at least 10 blocks. */
            fprintf (stderr, "%s: too small\n", argv[i]);
            return -1;
        }

        if (! fs_create (&fs, argv[i], kbytes, swap_kbytes)) {
            fprintf (stderr, "%s: cannot create filesystem\n", argv[i]);
            return -1;
        }
        printf ("Created filesystem %s - %u kbytes\n", argv[i], kbytes);

        if (i == argc-2) {
            /* Add the contents from the specified directory.
             * Use the optional manifest file. */
            add_contents (&fs, argv[i+1], manifest);
        }
        fs_close (&fs);
        return 0;
    }

    if (check) {
        /* Check filesystem for errors, and optionally fix them. */
        if (i != argc-1) {
            print_help (argv[0]);
            return -1;
        }
        if (! fs_open (&fs, argv[i], fix)) {
            fprintf (stderr, "%s: cannot open\n", argv[i]);
            return -1;
        }
        fs_check (&fs);
        fs_close (&fs);
        return 0;
    }

    if (scan) {
        /* Create a manifest from directory contents. */
        if (i != argc-1) {
            print_help (argv[0]);
            return -1;
        }
        if (! manifest_scan (&m, argv[i])) {
            fprintf (stderr, "%s: cannot read\n", argv[i]);
            return -1;
        }
        manifest_print (&m);
        return 0;
    }

    /* Add or extract or info. */
    if (i >= argc) {
        print_help (argv[0]);
        return -1;
    }
    if (! fs_open (&fs, argv[i], (add + mount != 0))) {
        fprintf (stderr, "%s: cannot open\n", argv[i]);
        return -1;
    }

    if (extract) {
        /* Extract all files to current directory. */
        if (i != argc-1) {
            print_help (argv[0]);
            return -1;
        }
        if (! fs_inode_get (&fs, &inode, BSDFS_ROOT_INODE)) {
            fprintf (stderr, "%s: cannot get inode 1\n", argv[i]);
            return -1;
        }
        fs_directory_scan (&inode, "", extractor, (void*) stdout);
        fs_close (&fs);
        return 0;
    }

    if (add) {
        /* Add files i+1..argc-1 to filesystem. */
        if (i >= argc) {
            print_help (argv[0]);
            return -1;
        }
        while (++i < argc)
            add_object (&fs, argv[i]);
        fs_sync (&fs, 0);
        fs_close (&fs);
        return 0;
    }

    if (mount) {
        /* Mount the filesystem. */
        if (i != argc-2) {
            print_help (argv[0]);
            return -1;
        }
        return fs_mount(&fs, argv[i+1]);
    }

    /* Print the structure of flesystem. */
    if (i != argc-1) {
        print_help (argv[0]);
        return -1;
    }
    fs_print (&fs, stdout);
    if (verbose) {
        printf ("--------\n");
        if (! fs_inode_get (&fs, &inode, BSDFS_ROOT_INODE)) {
            fprintf (stderr, "%s: cannot get inode 1\n", argv[i]);
            return -1;
        }
        printf ("/\n");
        if (verbose > 1) {
            /* Print a list of blocks. */
            print_inode_blocks (&inode, stdout);
            if (verbose > 2) {
                fs_inode_print (&inode, stdout);
                printf ("--------\n");
            }
        }
        fs_directory_scan (&inode, "", scanner, (void*) stdout);
    }
    fs_close (&fs);
    return 0;
}
