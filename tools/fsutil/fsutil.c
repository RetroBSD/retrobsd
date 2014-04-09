/*
 * Utility for dealing with 2.xBSD filesystem images.
 *
 * Copyright (C) 2006-2011 Serge Vakulenko, <serge@vak.ru>
 *
 * This file is part of RetroBSD project, which is distributed
 * under the terms of the GNU General Public License (GPL).
 * See the accompanying file "COPYING" for more details.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <getopt.h>
#include "bsdfs.h"

int verbose;
int extract;
int add;
int newfs;
int check;
int fix;
unsigned kbytes;
unsigned swap_kbytes;

static const char *program_version =
	"BSD 2.x file system utility, version 1.0\n"
	"Copyright (C) 2011 Serge Vakulenko";

static const char *program_bug_address = "<serge@vak.ru>";

static struct option program_options[] = {
	{ "help",	no_argument,		0,	'h' },
	{ "version",	no_argument,		0,	'V' },
	{ "verbose",	no_argument,		0,	'v' },
	{ "add",	no_argument,		0,	'a' },
	{ "extract",	no_argument,		0,	'x' },
	{ "check",	no_argument,		0,	'c' },
	{ "fix",	no_argument,		0,	'f' },
	{ "new",	required_argument,	0,	'n' },
	{ "swap",	required_argument,	0,	's' },
	{ 0 }
};

static void print_help (char *progname)
{
	char *p = strrchr (progname, '/');
	if (p)
		progname = p+1;

	printf ("%s\n", program_version);
	printf ("This program is free software; it comes with ABSOLUTELY NO WARRANTY;\n"
		"see the GNU General Public License for more details.\n");
	printf ("\n");
	printf ("Usage:\n");
	printf ("  %s [--verbose] filesys.bin\n", progname);
	printf ("  %s --add filesys.bin files...\n", progname);
	printf ("  %s --extract filesys.bin\n", progname);
	printf ("  %s --check [--fix] filesys.bin\n", progname);
	printf ("  %s --new=kbytes [--swap=kbytes] filesys.bin\n", progname);
	printf ("\n");
	printf ("Options:\n");
	printf ("  -a, --add          Add files to filesystem.\n");
	printf ("  -x, --extract      Extract all files.\n");
	printf ("  -c, --check        Check filesystem, use -c -f to fix.\n");
	printf ("  -f, --fix          Fix bugs in filesystem.\n");
	printf ("  -n NUM, --new=NUM  Create new filesystem, size in kbytes.\n");
	printf ("  -s NUM, --swap=NUM Size of swap area in kbytes.\n");
	printf ("  -v, --verbose      Print verbose information.\n");
	printf ("  -V, --version      Print version information and then exit.\n");
	printf ("  -h, --help         Print this message.\n");
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
void add_directory (fs_t *fs, char *name)
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
	if (! fs_inode_by_name (fs, &parent, buf, 0, 0)) {
		fprintf (stderr, "%s: cannot open directory\n", buf);
		return;
	}

	/* Create directory. */
	int done = fs_inode_by_name (fs, &dir, name, 1, INODE_MODE_FDIR | 0777);
	if (! done) {
		fprintf (stderr, "%s: directory inode create failed\n", name);
		return;
	}
	if (done == 1) {
                /* The directory already existed. */
                return;
	}
	fs_inode_save (&dir, 0);

	/* Make parent link '..' */
	strcpy (buf, name);
	strcat (buf, "/..");
	if (! fs_inode_by_name (fs, &dir, buf, 3, parent.number)) {
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
void add_device (fs_t *fs, char *name, char *spec)
{
	fs_inode_t dev;
	int majr, minr;
	char type;

	if (sscanf (spec, "%c%d:%d", &type, &majr, &minr) != 3 ||
	    (type != 'c' && type != 'b') ||
	    majr < 0 || majr > 255 || minr < 0 || minr > 255) {
		fprintf (stderr, "%s: invalid device specification\n", spec);
		fprintf (stderr, "expected c<major>:<minor> or b<major>:<minor>\n");
		return;
	}
	if (! fs_inode_by_name (fs, &dev, name, 1, 0666 |
	    ((type == 'b') ? INODE_MODE_FBLK : INODE_MODE_FCHR))) {
		fprintf (stderr, "%s: device inode create failed\n", name);
		return;
	}
	dev.addr[1] = majr << 8 | minr;
	fs_inode_save (&dev, 1);
}

/*
 * Copy file to filesystem.
 * When name is ended by slash as "name/", directory is created.
 */
void add_file (fs_t *fs, char *name)
{
	fs_file_t file;
	FILE *fd;
	unsigned char data [BSDFS_BSIZE];
	struct stat st;
	char *p;
	int len;

	if (verbose) {
		printf ("%s\n", name);
	}
	p = strrchr (name, '/');
	if (p && p[1] == 0) {
		*p = 0;
		add_directory (fs, name);
		return;
	}
	p = strrchr (name, '!');
	if (p) {
		*p++ = 0;
		add_device (fs, name, p);
		return;
	}
	fd = fopen (name, "r");
	if (! fd) {
		perror (name);
		return;
	}
	stat (name, &st);
	if (! fs_file_create (fs, &file, name, st.st_mode)) {
		fprintf (stderr, "%s: cannot create\n", name);
		return;
	}
	for (;;) {
		len = fread (data, 1, sizeof (data), fd);
/*		printf ("read %d bytes from %s\n", len, name);*/
		if (len < 0)
			perror (name);
		if (len <= 0)
			break;
		if (! fs_file_write (&file, data, len)) {
			fprintf (stderr, "%s: write error\n", name);
			break;
		}
	}
        file.inode.mtime = st.st_mtime;
	fs_file_close (&file);
	fclose (fd);
}

int main (int argc, char **argv)
{
	int i, key;
	fs_t fs;
	fs_inode_t inode;

	for (;;) {
		key = getopt_long (argc, argv, "vaxn:cfs:",
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
		case 's':
			swap_kbytes = strtol (optarg, 0, 0);
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
	if ((! add && i != argc-1) || (add && i >= argc) ||
	    (extract + newfs + check + add > 1) ||
	    (newfs && kbytes < BSDFS_BSIZE * 10 / 1024)) {
		print_help (argv[0]);
		return -1;
	}

	if (newfs) {
		/* Create new filesystem. */
		if (! fs_create (&fs, argv[i], kbytes, swap_kbytes)) {
			fprintf (stderr, "%s: cannot create filesystem\n", argv[i]);
			return -1;
		}
		printf ("Created filesystem %s - %u kbytes\n", argv[i], kbytes);
		fs_close (&fs);
		return 0;
	}

	if (check) {
		/* Check filesystem for errors, and optionally fix them. */
		if (! fs_open (&fs, argv[i], fix)) {
			fprintf (stderr, "%s: cannot open\n", argv[i]);
			return -1;
		}
		fs_check (&fs);
		fs_close (&fs);
		return 0;
	}

	/* Add or extract or info. */
	if (! fs_open (&fs, argv[i], (add != 0))) {
		fprintf (stderr, "%s: cannot open\n", argv[i]);
		return -1;
	}

	if (extract) {
		/* Extract all files to current directory. */
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
		while (++i < argc)
			add_file (&fs, argv[i]);
		fs_sync (&fs, 0);
		fs_close (&fs);
		return 0;
	}

	/* Print the structure of flesystem. */
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
