/*
 * Mount 2.xBSD filesystem via FUSE interface.
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>

#include "bsdfs.h"

extern int verbose;

/*
 * File descriptor to be used by op_open(), op_create(), op_read(),
 * op_write(), op_release(), op_fgetattr(), op_fsync(), op_ftruncate().
 */
static fs_file_t file;

/*
 * Print a message to log file.
 */
static void printlog(const char *format, ...)
{
    va_list ap;

    if (verbose) {
        va_start(ap, format);
        vfprintf(stderr, format, ap);
        va_end(ap);
        fflush(stderr);
    }
}

static dev_t make_rdev(unsigned raw)
{
    return makedev (raw >> 8, raw & 0xff);
}

/*
 * Copy data to struct stat.
 */
static int getstat (fs_inode_t *inode, struct stat *statbuf)
{
    statbuf->st_mode   = inode->mode & 07777;    /* protection */
    statbuf->st_ino    = inode->number;          /* inode number */
    statbuf->st_nlink  = inode->nlink;           /* number of hard links */
    statbuf->st_uid    = inode->uid;             /* user ID of owner */
    statbuf->st_gid    = inode->gid;             /* group ID of owner */
    statbuf->st_rdev   = 0;                      /* device ID (if special file) */
    statbuf->st_size   = inode->size;            /* total size, in bytes */
    statbuf->st_blocks = inode->size >> 9;       /* number of blocks allocated */
    statbuf->st_atime  = inode->atime;           /* time of last access */
    statbuf->st_mtime  = inode->mtime;           /* time of last modification */
    statbuf->st_ctime  = inode->ctime;           /* time of last status change */

    switch (inode->mode & INODE_MODE_FMT) {      /* type of file */
    case INODE_MODE_FREG:                       /* regular */
        statbuf->st_mode |= S_IFREG;
        break;
    case INODE_MODE_FDIR:                       /* directory */
        statbuf->st_mode |= S_IFDIR;
        break;
    case INODE_MODE_FCHR:                       /* character special */
        statbuf->st_mode |= S_IFCHR;
        statbuf->st_rdev = make_rdev (inode->addr[1]);
        break;
    case INODE_MODE_FBLK:                       /* block special */
        statbuf->st_mode |= S_IFBLK;
        statbuf->st_rdev = make_rdev (inode->addr[1]);
        break;
    case INODE_MODE_FLNK:                       /* symbolic link */
        statbuf->st_mode |= S_IFLNK;
        break;
    case INODE_MODE_FSOCK:                      /* socket */
        statbuf->st_mode |= S_IFSOCK;
        break;
    default:                                    /* cannot happen */
        printlog("--- unknown file type %#x\n", inode->mode & INODE_MODE_FMT);
        return -ENOENT;
    }
    return 0;
}

/*
 * Get file attributes.
 */
int op_getattr(const char *path, struct stat *statbuf)
{
    fs_t *fs = fuse_get_context()->private_data;
    fs_inode_t inode;

    printlog("--- op_getattr(path=\"%s\", statbuf=%p)\n", path, statbuf);

    if (! fs_inode_lookup (fs, &inode, path)) {
        printlog("--- search failed\n");
        return -ENOENT;
    }
    return getstat (&inode, statbuf);
}

/*
 * Get attributes from an open file.
 */
int op_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi)
{
    printlog("--- op_fgetattr(path=\"%s\", statbuf=%p, fi=%p)\n",
        path, statbuf, fi);

    if (strcmp(path, "/") == 0)
	return op_getattr(path, statbuf);

    if (file.inode.mode == 0)
        return -EBADF;

    return getstat (&file.inode, statbuf);
}

/*
 * File open operation.
 */
int op_open(const char *path, struct fuse_file_info *fi)
{
    fs_t *fs = fuse_get_context()->private_data;
    int write_flag = (fi->flags & O_ACCMODE) != O_RDONLY;

    printlog("--- op_open(path=\"%s\", fi=%p) flags=%#x \n",
        path, fi, fi->flags);

    if (! fs_file_open (fs, &file, path, write_flag)) {
        printlog("--- open failed\n");
        return -ENOENT;
    }

    if ((file.inode.mode & INODE_MODE_FMT) != INODE_MODE_FREG) {
        /* Cannot open special files. */
        file.inode.mode = 0;
        return -ENXIO;
    }

    if (fi->flags & O_APPEND) {
        file.offset = file.inode.size;
    }
    return 0;
}

/*
 * Create and open a file.
 */
int op_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    fs_t *fs = fuse_get_context()->private_data;

    printlog("--- op_create(path=\"%s\", mode=0%03o, fi=%p)\n",
        path, mode, fi);

    file.inode.mode = 0;
    if (! fs_file_create (fs, &file, path, mode & 07777)) {
        printlog("--- create failed\n");
	if ((file.inode.mode & INODE_MODE_FMT) == INODE_MODE_FDIR)
            return -EISDIR;
        return -EIO;
    }
    file.inode.mtime = time(0);
    file.inode.dirty = 1;
    fs_file_close (&file);
    return 0;
}

/*
 * Read data from an open file.
 */
int op_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    printlog("--- op_read(path=\"%s\", buf=%p, size=%d, offset=%lld, fi=%p)\n",
        path, buf, size, offset, fi);

    if (offset >= file.inode.size)
        return 0;

    file.offset = offset;
    if (size > file.inode.size - offset)
        size = file.inode.size - offset;

    if (! fs_file_read (&file, (unsigned char*) buf, size)) {
        printlog("--- read failed\n");
        return -EIO;
    }
    printlog("--- read returned %u\n", size);
    return size;
}

/*
 * Write data to an open file.
 */
int op_write(const char *path, const char *buf, size_t size, off_t offset,
	     struct fuse_file_info *fi)
{
    printlog("--- op_write(path=\"%s\", buf=%p, size=%d, offset=%lld, fi=%p)\n",
        path, buf, size, offset, fi);

    file.offset = offset;
    if (! fs_file_write (&file, (unsigned char*) buf, size)) {
        printlog("--- read failed\n");
        return -EIO;
    }
    file.inode.mtime = time(0);
    file.inode.dirty = 1;
    return size;
}

/*
 * Release an open file.
 */
int op_release(const char *path, struct fuse_file_info *fi)
{
    printlog("--- op_release(path=\"%s\", fi=%p)\n", path, fi);

    if (file.inode.mode == 0)
        return -EBADF;

    fs_file_close (&file);
    file.inode.mode = 0;
    return 0;
}

/*
 * Change the size of a file
 */
int op_truncate(const char *path, off_t newsize)
{
    fs_t *fs = fuse_get_context()->private_data;
    fs_file_t f;

    printlog("--- op_truncate(path=\"%s\", newsize=%lld)\n", path, newsize);

    if (! fs_file_open (fs, &f, path, 1)) {
        printlog("--- open failed\n");
        return -ENOENT;
    }

    if ((f.inode.mode & INODE_MODE_FMT) != INODE_MODE_FREG) {
        /* Cannot truncate special files. */
        return -EINVAL;
    }
    fs_inode_truncate (&f.inode, newsize);
    f.inode.mtime = time(0);
    file.inode.dirty = 1;
    fs_file_close (&f);
    return 0;
}

/*
 * Change the size of an open file.
 */
int op_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
    printlog("--- op_ftruncate(path=\"%s\", offset=%lld, fi=%p)\n",
        path, offset, fi);

    if (! file.writable)
        return -EACCES;

    if ((file.inode.mode & INODE_MODE_FMT) != INODE_MODE_FREG) {
        /* Cannot truncate special files. */
        return -EINVAL;
    }
    fs_inode_truncate (&file.inode, offset);
    file.inode.mtime = time(0);
    file.inode.dirty = 1;
    fs_file_close (&file);
    return 0;
}

/*
 * Remove a file.
 */
int op_unlink(const char *path)
{
    fs_t *fs = fuse_get_context()->private_data;
    fs_inode_t inode;

    printlog("--- op_unlink(path=\"%s\")\n", path);

    /* Get the file type. */
    if (! fs_inode_lookup (fs, &inode, path)) {
        printlog("--- search failed\n");
        return -ENOENT;
    }
    if ((file.inode.mode & INODE_MODE_FMT) == INODE_MODE_FDIR) {
        /* Cannot unlink directories. */
        return -EISDIR;
    }

    /* Delete file. */
    if (! fs_inode_delete (fs, &inode, path)) {
        printlog("--- delete failed\n");
        return -EIO;
    }
    fs_inode_save (&inode, 1);
    return 0;
}

/*
 * Remove a directory.
 */
int op_rmdir(const char *path)
{
    fs_t *fs = fuse_get_context()->private_data;
    fs_inode_t inode, parent;
    char buf [BSDFS_BSIZE], *p;

    printlog("--- op_rmdir(path=\"%s\")\n", path);

    /* Get the file type. */
    if (! fs_inode_lookup (fs, &inode, path)) {
        printlog("--- search failed\n");
        return -ENOENT;
    }
    if ((inode.mode & INODE_MODE_FMT) != INODE_MODE_FDIR) {
        /* Cannot remove files. */
        return -ENOTDIR;
    }
    if (inode.nlink > 2) {
        /* Cannot remove non-empty directories. */
        return -ENOTEMPTY;
    }

    /* Open parent directory. */
    strcpy (buf, path);
    p = strrchr (buf, '/');
    if (p)
        *p = 0;
    else
        *buf = 0;
    if (! fs_inode_lookup (fs, &parent, buf)) {
        printlog("--- parent not found\n");
        return -ENOENT;
    }

    /* Delete directory.
     * Need to decrease a link count first. */
    if (! fs_inode_lookup (fs, &inode, path)) {
        printlog("--- directory not found\n");
        return -ENOENT;
    }
    --inode.nlink;
    fs_inode_save (&inode, 1);
    if (! fs_inode_delete (fs, &inode, path)) {
        printlog("--- delete failed\n");
        return -EIO;
    }
    fs_inode_save (&inode, 1);

    /* Decrease a parent's link counter. */
    if (! fs_inode_get (fs, &parent, parent.number)) {
        printlog("--- cannot reopen parent\n");
        return -EIO;
    }
    --parent.nlink;
    fs_inode_save (&parent, 1);
    return 0;
}

/*
 * Create a directory.
 */
int op_mkdir(const char *path, mode_t mode)
{
    fs_t *fs = fuse_get_context()->private_data;
    fs_inode_t dir, parent;
    char buf [BSDFS_BSIZE], *p;

    printlog("--- op_mkdir(path=\"%s\", mode=0%3o)\n", path, mode);

    /* Open parent directory. */
    strcpy (buf, path);
    p = strrchr (buf, '/');
    if (p)
        *p = 0;
    else
        *buf = 0;
    if (! fs_inode_lookup (fs, &parent, buf)) {
        printlog("--- parent not found\n");
        return -ENOENT;
    }

    /* Create directory. */
    mode &= 07777;
    mode |= INODE_MODE_FDIR;
    int done = fs_inode_create (fs, &dir, path, mode);
    if (! done) {
        printlog("--- cannot create dir inode\n");
        return -ENOENT;
    }
    if (done == 1) {
        /* The directory already existed. */
        return -EEXIST;
    }
    fs_inode_save (&dir, 0);

    /* Make parent link '..' */
    strcpy (buf, path);
    strcat (buf, "/..");
    if (! fs_inode_link (fs, &dir, buf, parent.number)) {
        printlog("--- dotdot link failed\n");
        return -EIO;
    }
    if (! fs_inode_get (fs, &parent, parent.number)) {
        printlog("--- cannot reopen parent\n");
        return -EIO;
    }
    ++parent.nlink;
    fs_inode_save (&parent, 1);
    return 0;
}

/*
 * Create a hard link to a file.
 */
int op_link(const char *path, const char *newpath)
{
    fs_t *fs = fuse_get_context()->private_data;
    fs_inode_t source, target;

    printlog("--- op_link(path=\"%s\", newpath=\"%s\")\n", path, newpath);

    /* Find source. */
    if (! fs_inode_lookup (fs, &source, path)) {
        printlog("--- source not found\n");
        return -ENOENT;
    }

    if ((source.mode & INODE_MODE_FMT) == INODE_MODE_FDIR) {
        /* Cannot link directories. */
        return -EPERM;
    }

    /* Create target link. */
    if (! fs_inode_link (fs, &target, newpath, source.number)) {
        printlog("--- link failed\n");
        return -EIO;
    }
    source.nlink++;
    fs_inode_save (&source, 1);
    return 0;
}

/*
 * Rename a file.
 */
int op_rename(const char *path, const char *newpath)
{
    fs_t *fs = fuse_get_context()->private_data;
    fs_inode_t source, target;

    printlog("--- op_rename(path=\"%s\", newpath=\"%s\")\n", path, newpath);

    /* Find source and increase the link count. */
    if (! fs_inode_lookup (fs, &source, path)) {
        printlog("--- source not found\n");
        return -ENOENT;
    }
    source.nlink++;
    fs_inode_save (&source, 1);

    /* Create target link. */
    if (! fs_inode_link (fs, &target, newpath, source.number)) {
        printlog("--- link failed\n");
        return -EIO;
    }

    /* Delete the source. */
    if (! fs_inode_delete (fs, &source, path)) {
        printlog("--- delete failed\n");
        return -EIO;
    }
    fs_inode_save (&source, 1);
    return 0;
}

/*
 * Create a file node.
 */
int op_mknod(const char *path, mode_t mode, dev_t dev)
{
    fs_t *fs = fuse_get_context()->private_data;
    fs_inode_t inode;

    printlog("--- op_mknod(path=\"%s\", mode=0%3o, dev=%lld)\n",
        path, mode, dev);

    /* Check if the file already exists. */
    if (fs_inode_lookup (fs, &inode, path)) {
        printlog("--- already exists\n");
        return -EEXIST;
    }

    /* Encode a mode bitmask. */
    if (S_ISREG(mode)) {
        mode = (mode & 07777) | INODE_MODE_FREG;
    } else if (S_ISCHR(mode)) {
        mode = (mode & 07777) | INODE_MODE_FCHR;
    } else if (S_ISBLK(mode)) {
        mode = (mode & 07777) | INODE_MODE_FBLK;
    } else
        return -EINVAL;

    /* Create the file. */
    if (! fs_inode_create (fs, &inode, path, mode)) {
        printlog("--- create failed\n");
        return -EIO;
    }
    if (S_ISCHR(mode) || S_ISBLK(mode)) {
        inode.addr[1] = major(dev) << 8 | minor(dev);
    }
    inode.mtime = time(0);
    inode.dirty = 1;
    if (! fs_inode_save (&inode, 0)) {
        printlog("--- create failed\n");
        return -EIO;
    }
    return 0;
}

/*
 * Read the target of a symbolic link.
 */
int op_readlink(const char *path, char *link, size_t size)
{
    fs_t *fs = fuse_get_context()->private_data;
    fs_inode_t inode;

    printlog("--- op_readlink(path=\"%s\", link=\"%s\", size=%d)\n",
        path, link, size);

    /* Open the file. */
    if (! fs_inode_lookup (fs, &inode, path)) {
        printlog("--- file not found\n");
        return -ENOENT;
    }

    if ((inode.mode & INODE_MODE_FMT) != INODE_MODE_FLNK)
        return -EINVAL;

    /* Leave one byte for the terminating null. */
    if (size > inode.size + 1)
        size = inode.size + 1;
    if (! fs_inode_read (&inode, 0, (unsigned char*)link, size-1)) {
        printlog("--- read failed\n");
        return -EIO;
    }
    link[size-1] = 0;
    return 0;
}

/*
 * Create a symbolic link.
 */
int op_symlink(const char *path, const char *newpath)
{
    fs_t *fs = fuse_get_context()->private_data;
    fs_inode_t inode;
    int len, mode;

    printlog("--- op_symlink(path=\"%s\", newpath=\"%s\")\n", path, newpath);

    /* Check if the file already exists. */
    if (fs_inode_lookup (fs, &inode, newpath)) {
        printlog("--- already exists\n");
        return -EEXIST;
    }

    /* Create symlink. */
    mode = 0777 | INODE_MODE_FLNK;
    if (! fs_inode_create (fs, &inode, newpath, mode)) {
        printlog("--- create failed\n");
        return -EIO;
    }
    fs_inode_save (&inode, 0);

    len = strlen (path);
    if (! fs_inode_write (&inode, 0, (unsigned char*)path, len)) {
        printlog("--- write failed\n");
        return -EIO;
    }
    inode.mtime = time(0);
    inode.dirty = 1;
    if (! fs_inode_save (&inode, 0)) {
        printlog("--- create failed\n");
        return -EIO;
    }
    return 0;
}

/*
 * Change the permission bits of a file.
 */
int op_chmod(const char *path, mode_t mode)
{
    fs_t *fs = fuse_get_context()->private_data;
    fs_inode_t inode;

    printlog("--- op_chmod(path=\"%s\", mode=0%03o)\n", path, mode);

    /* Open the file. */
    if (! fs_inode_lookup (fs, &inode, path)) {
        printlog("--- file not found\n");
        return -ENOENT;
    }

    /* Modify the access mode. */
    inode.mode &= ~07777;
    inode.mode |= mode;
    inode.dirty = 1;
    fs_inode_save (&inode, 0);
    return 0;
}

/*
 * Change the owner and group of a file.
 */
int op_chown(const char *path, uid_t uid, gid_t gid)
{
    fs_t *fs = fuse_get_context()->private_data;
    fs_inode_t inode;

    printlog("--- op_chown(path=\"%s\", uid=%d, gid=%d)\n", path, uid, gid);

    /* Open the file. */
    if (! fs_inode_lookup (fs, &inode, path)) {
        printlog("--- file not found\n");
        return -ENOENT;
    }

    /* Modify the owner and group. */
    inode.uid = uid;
    inode.gid = gid;
    inode.dirty = 1;
    fs_inode_save (&inode, 0);
    return 0;
}

/*
 * Change the access and/or modification times of a file.
 */
int op_utime(const char *path, struct utimbuf *ubuf)
{
    fs_t *fs = fuse_get_context()->private_data;
    fs_inode_t inode;

    printlog("--- op_utime(path=\"%s\", ubuf=%p)\n", path, ubuf);

    /* Open the file. */
    if (! fs_inode_lookup (fs, &inode, path)) {
        printlog("--- file not found\n");
        return -ENOENT;
    }

    /* Modify the access and modification times. */
    inode.atime = ubuf->actime;
    inode.mtime = ubuf->modtime;
    inode.dirty = 1;
    fs_inode_save (&inode, 0);
    return 0;
}

/*
 * Get file system statistics.
 *
 * The 'f_frsize', 'f_favail', 'f_fsid' and 'f_flag' fields are ignored
 */
int op_statfs(const char *path, struct statvfs *statv)
{
    fs_dirent_t dirent;

    printlog("--- op_statfs(path=\"%s\", statv=%p)\n", path, statv);

    /* The maximum length in bytes of a file name on this file system. */
    statv->f_namemax = sizeof(dirent.name) - 1;

    /* The preferred length of I/O requests for files on this file system. */
    statv->f_bsize = BSDFS_BSIZE;
    return 0;
}

/*
 * Possibly flush cached data.
 */
int op_flush(const char *path, struct fuse_file_info *fi)
{
    printlog("--- op_flush(path=\"%s\", fi=%p)\n", path, fi);
    return 0;
}

/*
 * Synchronize file contents.
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data.
 */
int op_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
    printlog("--- op_fsync(path=\"%s\", datasync=%d, fi=%p)\n",
        path, datasync, fi);

    if (datasync == 0 && file.writable) {
        if (! fs_inode_save (&file.inode, 0))
            return -EIO;
    }
    return 0;
}

/*
 * Open directory.
 */
int op_opendir(const char *path, struct fuse_file_info *fi)
{
    printlog("--- op_opendir(path=\"%s\", fi=%p)\n", path, fi);
    return 0;
}

/*
 * Read directory.
 */
int op_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
	       struct fuse_file_info *fi)
{
    fs_t *fs = fuse_get_context()->private_data;
    fs_inode_t dir;
    char name [BSDFS_BSIZE - 12];
    struct {
        unsigned int inum;
        unsigned short reclen;
        unsigned short namlen;
    } dirent;

    printlog("--- op_readdir(path=\"%s\", buf=%p, filler=%p, offset=%lld, fi=%p)\n",
        path, buf, filler, offset, fi);

    if (! fs_inode_lookup (fs, &dir, path)) {
        printlog("--- cannot find path %s\n", path);
        return -ENOENT;
    }

    /* Copy the entire directory into the buffer. */
    for (offset = 0; offset < dir.size; offset += dirent.reclen) {
        if (! fs_inode_read (&dir, offset, (unsigned char*) &dirent, sizeof(dirent))) {
            printlog("--- read error at offset %ld\n", offset);
            return -EIO;
        }
        //printlog("--- readdir offset %lu: inum=%u, reclen=%u, namlen=%u\n", offset, dirent.inum, dirent.reclen, dirent.namlen);

        if (! fs_inode_read (&dir, offset+sizeof(dirent),
            (unsigned char*)name, (dirent.namlen + 4) / 4 * 4))
        {
            printlog("--- name read error at offset %ld\n", offset);
            return -EIO;
        }
        //printlog("--- readdir offset %lu: name='%s'\n", offset, name);

        if (dirent.inum != 0) {
            //printlog("calling filler with name %s\n", name);
            name[dirent.namlen] = 0;
            if (filler(buf, name, NULL, 0) != 0) {
                printlog("    ERROR op_readdir filler: buffer full");
                return -ENOMEM;
            }
        }
    }
    return 0;
}

/*
 * Release directory.
 */
int op_releasedir(const char *path, struct fuse_file_info *fi)
{
    printlog("--- op_releasedir(path=\"%s\", fi=%p)\n", path, fi);
    return 0;
}

/*
 * Clean up filesystem.
 * Called on filesystem exit.
 */
void op_destroy(void *userdata)
{
    printlog("--- op_destroy(userdata=%p)\n", userdata);
}

/*
 * Check file access permissions.
 */
int op_access(const char *path, int mask)
{
    printlog("--- op_access(path=\"%s\", mask=0%o)\n", path, mask);

    /* Always permitted. */
    return 0;
}

static struct fuse_operations mount_ops = {
    .access     = op_access,
    .chmod      = op_chmod,
    .chown      = op_chown,
    .create     = op_create,
    .destroy    = op_destroy,
    .fgetattr   = op_fgetattr,
    .flush      = op_flush,
    .fsync      = op_fsync,
    .ftruncate  = op_ftruncate,
    .getattr    = op_getattr,
    .link       = op_link,
    .mkdir      = op_mkdir,
    .mknod      = op_mknod,
    .open       = op_open,
    .opendir    = op_opendir,
    .readdir    = op_readdir,
    .readlink   = op_readlink,
    .read       = op_read,
    .release    = op_release,
    .releasedir = op_releasedir,
    .rename     = op_rename,
    .rmdir      = op_rmdir,
    .statfs     = op_statfs,
    .symlink    = op_symlink,
    .truncate   = op_truncate,
    .unlink     = op_unlink,
    .utime      = op_utime,
    .write      = op_write,
};

int fs_mount(fs_t *fs, char *dirname)
{
    char *av[8];
    int ret, ac;

    printf ("Filesystem mounted as %s\n", dirname);
    printf ("Press ^C to unmount\n");

    /* Invoke FUSE to mount the filesystem. */
    ac = 0;
    av[ac++] = "fsutil";
    av[ac++] = "-f";                    /* foreground */
    av[ac++] = "-s";                    /* single-threaded */
    if (verbose > 1)
        av[ac++] = "-d";                /* debug */
    av[ac++] = dirname;
    av[ac] = 0;
    ret = fuse_main(ac, av, &mount_ops, fs);
    if (ret != 0) {
        perror ("fuse_main failed");
        return -1;
    }
    fs_sync (fs, 0);
    fs_close (fs);
    printf ("\nFilesystem %s unmounted\n", dirname);
    return ret;
}
