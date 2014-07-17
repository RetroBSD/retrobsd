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

/*
 * Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int op_getattr(const char *path, struct stat *statbuf)
{
    fs_t *fs = fuse_get_context()->private_data;
    fs_inode_t dir;

    printlog("--- op_getattr(path=\"%s\", statbuf=%p)\n",
	  path, statbuf);

    if (! fs_inode_by_name (fs, &dir, path, 0, 0)) {
        printlog("--- cannot find path %s\n", path);
        return -ENOENT;
    }

    switch (dir.mode & INODE_MODE_FMT) {    /* type of file */
    case INODE_MODE_FREG:                   /* regular */
        statbuf->st_mode = S_IFREG;
        break;
    case INODE_MODE_FDIR:                   /* directory */
        statbuf->st_mode = S_IFDIR;
        break;
    case INODE_MODE_FCHR:                   /* character special */
        statbuf->st_mode = S_IFCHR;
        break;
    case INODE_MODE_FBLK:                   /* block special */
        statbuf->st_mode = S_IFBLK;
        break;
    case INODE_MODE_FLNK:                   /* symbolic link */
        statbuf->st_mode = S_IFLNK;
        break;
    case INODE_MODE_FSOCK:                  /* socket */
        statbuf->st_mode = S_IFSOCK;
        break;
    default:                                /* cannot happen */
        printlog("--- unknown file type %#x\n", dir.mode & INODE_MODE_FMT);
        return -ENOENT;
    }
    statbuf->st_mode  |= dir.mode & 07777;  /* protection */
    statbuf->st_ino    = dir.number;        /* inode number */
    statbuf->st_nlink  = dir.nlink;         /* number of hard links */
    statbuf->st_uid    = dir.uid;           /* user ID of owner */
    statbuf->st_gid    = dir.gid;           /* group ID of owner */
    statbuf->st_rdev   = dir.addr[1];       /* device ID (if special file) */
    statbuf->st_size   = dir.size;          /* total size, in bytes */
    statbuf->st_blocks = dir.size >> 9;     /* number of 512B blocks allocated */
    statbuf->st_atime  = dir.atime;         /* time of last access */
    statbuf->st_mtime  = dir.mtime;         /* time of last modification */
    statbuf->st_ctime  = dir.ctime;         /* time of last status change */
    return 0;
}

/*
 * Get attributes from an open file
 *
 * This method is called instead of the getattr() method if the
 * file information is available.
 *
 * Currently this is only called after the create() method if that
 * is implemented (see above).  Later it may be called for
 * invocations of fstat() too.
 */
int op_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi)
{
    printlog("--- op_fgetattr(path=\"%s\", statbuf=%p, fi=%p)\n",
	    path, statbuf, fi);

    // On FreeBSD, trying to do anything with the mountpoint ends up
    // opening it, and then using the FD for an fgetattr.  So in the
    // special case of a path of "/", I need to do a getattr on the
    // underlying root directory instead of doing the fgetattr().
    if (strcmp(path, "/") == 0)
	return op_getattr(path, statbuf);

    //TODO
    //retstat = fstat(fi->fh, statbuf);
    //if (retstat < 0)
    //    retstat = print_errno("op_fgetattr fstat");

    return 0;
}

/*
 * Read the target of a symbolic link
 *
 * The buffer should be filled with a null terminated string.  The
 * buffer size argument includes the space for the terminating
 * null character.  If the linkname is too long to fit in the
 * buffer, it should be truncated.  The return value should be 0
 * for success.
 */
// Note the system readlink() will truncate and lose the terminating
// null.  So, the size passed to to the system readlink() must be one
// less than the size passed to op_readlink()
// op_readlink() code by Bernardo F Costa (thanks!)
int op_readlink(const char *path, char *link, size_t size)
{
    printlog("op_readlink(path=\"%s\", link=\"%s\", size=%d)\n",
	  path, link, size);

    //retstat = readlink(path, link, size - 1);
    //if (retstat < 0)
    //    retstat = print_errno("op_readlink readlink");
    //else {
    //    link[retstat] = '\0';
    //    retstat = 0;
    //}

    return 0;
}

/*
 * Create a file node
 *
 * There is no create() operation, mknod() will be called for
 * creation of all non-directory, non-symlink nodes.
 */
int op_mknod(const char *path, mode_t mode, dev_t dev)
{
    printlog("--- op_mknod(path=\"%s\", mode=0%3o, dev=%lld)\n",
	  path, mode, dev);

    //TODO
    //if (S_ISREG(mode)) {
    //    retstat = open(path, O_CREAT | O_EXCL | O_WRONLY, mode);
    //    if (retstat < 0)
    //        retstat = print_errno("op_mknod open");
    //    else {
    //        retstat = close(retstat);
    //        if (retstat < 0)
    //            retstat = print_errno("op_mknod close");
    //    }
    //} else if (S_ISFIFO(mode)) {
    //    retstat = mkfifo(path, mode);
    //    if (retstat < 0)
    //        retstat = print_errno("op_mknod mkfifo");
    //} else {
    //    retstat = mknod(path, mode, dev);
    //    if (retstat < 0)
    //        retstat = print_errno("op_mknod mknod");
    //}
    return 0;
}

/*
 * Create a directory
 */
int op_mkdir(const char *path, mode_t mode)
{
    printlog("--- op_mkdir(path=\"%s\", mode=0%3o)\n",
	    path, mode);

    //TODO
    //retstat = mkdir(path, mode);
    //if (retstat < 0)
    //    retstat = print_errno("op_mkdir mkdir");

    return 0;
}

/*
 * Remove a file
 */
int op_unlink(const char *path)
{
    printlog("op_unlink(path=\"%s\")\n",
	    path);

    //TODO
    //retstat = unlink(path);
    //if (retstat < 0)
    //    retstat = print_errno("op_unlink unlink");

    return 0;
}

/*
 * Remove a directory
 */
int op_rmdir(const char *path)
{
    printlog("op_rmdir(path=\"%s\")\n",
	    path);

    //TODO
    //retstat = rmdir(path);
    //if (retstat < 0)
    //    retstat = print_errno("op_rmdir rmdir");

    return 0;
}

/*
 * Create a symbolic link
 * The parameters here are a little bit confusing, but do correspond
 * to the symlink() system call.  The 'path' is where the link points,
 * while the 'link' is the link itself.  So we need to leave the path
 * unaltered, but insert the link into the mounted directory.
 */
int op_symlink(const char *path, const char *link)
{
    printlog("--- op_symlink(path=\"%s\", link=\"%s\")\n",
	    path, link);

    //TODO
    //retstat = symlink(path, link);
    //if (retstat < 0)
    //    retstat = print_errno("op_symlink symlink");

    return 0;
}

/*
 * Rename a file
 *
 * Both path and newpath are fs-relative.
 */
int op_rename(const char *path, const char *newpath)
{
    printlog("--- op_rename(path=\"%s\", newpath=\"%s\")\n",
	    path, newpath);

    //TODO
    //retstat = rename(path, newpath);
    //if (retstat < 0)
    //    retstat = print_errno("op_rename rename");

    return 0;
}

/*
 * Create a hard link to a file
 */
int op_link(const char *path, const char *newpath)
{
    printlog("--- op_link(path=\"%s\", newpath=\"%s\")\n",
	    path, newpath);

    //TODO
    //retstat = link(path, newpath);
    //if (retstat < 0)
    //    retstat = print_errno("op_link link");

    return 0;
}

/*
 * Change the permission bits of a file
 */
int op_chmod(const char *path, mode_t mode)
{
    printlog("--- op_chmod(path=\"%s\", mode=0%03o)\n",
	    path, mode);

    //TODO
    //retstat = chmod(path, mode);
    //if (retstat < 0)
    //    retstat = print_errno("op_chmod chmod");

    return 0;
}

/*
 * Change the owner and group of a file
 */
int op_chown(const char *path, uid_t uid, gid_t gid)

{
    printlog("--- op_chown(path=\"%s\", uid=%d, gid=%d)\n",
	    path, uid, gid);

    //TODO
    //retstat = chown(path, uid, gid);
    //if (retstat < 0)
    //    retstat = print_errno("op_chown chown");

    return 0;
}

/*
 * Change the size of a file
 */
int op_truncate(const char *path, off_t newsize)
{
    printlog("--- op_truncate(path=\"%s\", newsize=%lld)\n",
	    path, newsize);

    //TODO
    //retstat = truncate(path, newsize);
    //if (retstat < 0)
    //    print_errno("op_truncate truncate");

    return 0;
}

/*
 * Change the access and/or modification times of a file
 */
int op_utime(const char *path, struct utimbuf *ubuf)
{
    printlog("--- op_utime(path=\"%s\", ubuf=%p)\n",
	    path, ubuf);

    //TODO
    //retstat = utime(path, ubuf);
    //if (retstat < 0)
    //    retstat = print_errno("op_utime utime");

    return 0;
}

/*
 * File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * return an arbitrary filehandle in the fuse_file_info structure,
 * which will be passed to all file operations.
 */
int op_open(const char *path, struct fuse_file_info *fi)
{
    int fd = 0;

    printlog("--- op_open(path\"%s\", fi=%p)\n",
	    path, fi);

    //TODO
    //fd = open(path, fi->flags);
    //if (fd < 0)
    //    retstat = print_errno("op_open open");

    fi->fh = fd;

    return 0;
}

/*
 * Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 */
int op_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
    printlog("--- op_read(path=\"%s\", buf=%p, size=%d, offset=%lld, fi=%p)\n",
	    path, buf, size, offset, fi);

    //TODO
    //retstat = pread(fi->fh, buf, size, offset);
    //if (retstat < 0)
    //    retstat = print_errno("op_read read");

    return 0;
}

/*
 * Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 */
int op_write(const char *path, const char *buf, size_t size, off_t offset,
	     struct fuse_file_info *fi)
{
    printlog("--- op_write(path=\"%s\", buf=%p, size=%d, offset=%lld, fi=%p)\n",
	    path, buf, size, offset, fi);

    //TODO
    //retstat = pwrite(fi->fh, buf, size, offset);
    //if (retstat < 0)
    //    retstat = print_errno("op_write pwrite");

    return 0;
}

/*
 * Get file system statistics
 *
 * The 'f_frsize', 'f_favail', 'f_fsid' and 'f_flag' fields are ignored
 */
int op_statfs(const char *path, struct statvfs *statv)
{
    printlog("--- op_statfs(path=\"%s\", statv=%p)\n",
	    path, statv);

    // get stats for underlying filesystem
    //TODO
    //retstat = statvfs(path, statv);
    //if (retstat < 0)
    //    retstat = print_errno("op_statfs statvfs");

    return 0;
}

/*
 * Possibly flush cached data
 *
 * BIG NOTE: This is not equivalent to fsync().  It's not a
 * request to sync dirty data.
 *
 * Flush is called on each close() of a file descriptor.  So if a
 * filesystem wants to return write errors in close() and the file
 * has cached dirty data, this is a good place to write back data
 * and return any errors.  Since many applications ignore close()
 * errors this is not always useful.
 *
 * NOTE: The flush() method may be called more than once for each
 * open().  This happens if more than one file descriptor refers
 * to an opened file due to dup(), dup2() or fork() calls.  It is
 * not possible to determine if a flush is final, so each flush
 * should be treated equally.  Multiple write-flush sequences are
 * relatively rare, so this shouldn't be a problem.
 *
 * Filesystems shouldn't assume that flush will always be called
 * after some writes, or that if will be called at all.
 */
int op_flush(const char *path, struct fuse_file_info *fi)
{
    printlog("--- op_flush(path=\"%s\", fi=%p)\n", path, fi);
    return 0;
}

/*
 * Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 */
int op_release(const char *path, struct fuse_file_info *fi)
{
    printlog("--- op_release(path=\"%s\", fi=%p)\n",
	  path, fi);

    //TODO
    // We need to close the file.  Had we allocated any resources
    // (buffers etc) we'd need to free them here as well.
    //retstat = close(fi->fh);

    return 0;
}

/*
 * Synchronize file contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data.
 */
int op_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
    printlog("--- op_fsync(path=\"%s\", datasync=%d, fi=%p)\n",
	    path, datasync, fi);

    //TODO
    //retstat = fsync(fi->fh);
    //if (retstat < 0)
    //    print_errno("op_fsync fsync");

    return 0;
}

/*
 * Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 */
int op_opendir(const char *path, struct fuse_file_info *fi)
{
    printlog("--- op_opendir(path=\"%s\", fi=%p)\n",
	  path, fi);
    return 0;
}

/*
 * Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
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

    if (! fs_inode_by_name (fs, &dir, path, 0, 0)) {
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
            if (filler(buf, name, NULL, 0) != 0) {
                printlog("    ERROR op_readdir filler: buffer full");
                return -ENOMEM;
            }
        }
    }
    return 0;
}

/*
 * Release directory
 */
int op_releasedir(const char *path, struct fuse_file_info *fi)
{
    printlog("--- op_releasedir(path=\"%s\", fi=%p)\n",
	    path, fi);
    return 0;
}

/*
 * Clean up filesystem
 *
 * Called on filesystem exit.
 */
void op_destroy(void *userdata)
{
    printlog("--- op_destroy(userdata=%p)\n", userdata);
}

/*
 * Check file access permissions
 *
 * This will be called for the access() system call.  If the
 * 'default_permissions' mount option is given, this method is not
 * called.
 */
int op_access(const char *path, int mask)
{
    printlog("--- op_access(path=\"%s\", mask=0%o)\n",
	    path, mask);

    /* Always permitted. */
    return 0;
}

/*
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 */
int op_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    int fd = 0;

    printlog("--- op_create(path=\"%s\", mode=0%03o, fi=%p)\n",
	    path, mode, fi);

    //TODO
    //fd = creat(path, mode);
    //if (fd < 0)
    //    retstat = print_errno("op_create creat");

    fi->fh = fd;

    return 0;
}

/*
 * Change the size of an open file
 *
 * This method is called instead of the truncate() method if the
 * truncation was invoked from an ftruncate() system call.
 */
int op_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
    printlog("--- op_ftruncate(path=\"%s\", offset=%lld, fi=%p)\n",
	    path, offset, fi);

    //TODO
    //retstat = ftruncate(fi->fh, offset);
    //if (retstat < 0)
    //    retstat = print_errno("op_ftruncate ftruncate");

    return 0;
}

static struct fuse_operations mount_ops = {
    .access = op_access,
    .chmod = op_chmod,
    .chown = op_chown,
    .create = op_create,              //
    .destroy = op_destroy,            //
    .fgetattr = op_fgetattr,          //
    .flush = op_flush,                //
    .fsync = op_fsync,
    .ftruncate = op_ftruncate,        //
    .getattr = op_getattr,
    .link = op_link,
    .mkdir = op_mkdir,
    .mknod = op_mknod,
    .open = op_open,
    .opendir = op_opendir,            //
    .readdir = op_readdir,
    .readlink = op_readlink,
    .read = op_read,
    .release = op_release,
    .releasedir = op_releasedir,      //
    .rename = op_rename,
    .rmdir = op_rmdir,
    .statfs = op_statfs,
    .symlink = op_symlink,
    .truncate = op_truncate,
    .unlink = op_unlink,
    .utime = op_utime,                //
    .write = op_write,
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
    av[ac++] = "-f";        // foreground
    av[ac++] = "-s";        // single-threaded
    if (verbose > 1)
        av[ac++] = "-d";    // debug
    av[ac++] = dirname;
    av[ac] = 0;
    ret = fuse_main(ac, av, &mount_ops, fs);
    if (ret != 0) {
        perror ("fuse_main failed");
        return -1;
    }
    printf ("\nFilesystem %s unmounted\n", dirname);
    return ret;
}
