#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>

#include "libufs.h"

void inodemap(void *buf)
{
    int i;
    struct inode *in;

    for (i = 0; i < INOPB; i++) {
        in = (struct inode *)(buf + (i * sizeof(struct inode)));
        printf("%lu\n", (unsigned long)in->i_number);
    }
}

// Convert an array of direct structures into a filesysem block
// representing a directory structure containing those direct
// structures as the content
int32_t makedir(struct direct *protodir, int entries, void *buf)
{
    char *cp;
    int i, spcleft;

    spcleft = DIRBLKSIZ;
    for (cp = buf, i = 0; i < entries - 1; i++) {
        protodir[i].d_reclen = DIRSIZ(&protodir[i]);
        bcopy(&protodir[i], cp, protodir[i].d_reclen);
        cp += protodir[i].d_reclen;
        spcleft -= protodir[i].d_reclen;
    }
    protodir[i].d_reclen = spcleft;
    bcopy(&protodir[i], cp, DIRSIZ(&protodir[i]));
    for (i=0; i<entries; i++) {
        printf("%d: %s inode %d namlen %d recsize %d\n",
            i, protodir[i].d_name, protodir[i].d_ino, protodir[i].d_namlen, protodir[i].d_reclen);
    }

    return (DIRBLKSIZ);
}

void fssync(struct filesystem *f)
{
    fswriteblock(f, SUPERB, f->fs);
}

u_int32_t fsinodealloc(struct filesystem *f)
{
    u_int32_t ino;
    // are there any known free inodes?
    struct inode *in;
    u_int32_t ninode = 0;

badino:
    if (f->fs->fs_ninode > 0) {
        ino = f->fs->fs_inode[--f->fs->fs_ninode];
        // An inode number below ROOTINO is not valid.
        if (ino <= ROOTINO) {
            goto badino;
        }

        in = malloc(sizeof(struct inode));
        bzero(in, sizeof(struct inode));
        in->i_number = ino;
        fswriteinode(f, in);
        free(in);
        f->fs->fs_tinode--;
        fssync(f);
        return ino;
    }
    // We couldn't get an inode, so we need to scrape the filesystem
    // for more.

    printf("Not able to get an inode - scraping\n");


    // First, let's find a good inode for ourselves...
    for (ino = ROOTINO+1; itod(ino) < f->fs->fs_isize; ino++) {
        in = fsreadinode(f, ino);
        if (in->i_mode == 0) {
            bzero(in, sizeof(struct inode));
            in->i_number = ino;
            fswriteinode(f, in);
            ninode = ino;
            f->fs->fs_tinode--;
            fssync(f);
            free(in);
            break;
        }
        free(in);
    }

    printf("Got inode %lu\n", (unsigned long)ninode);

    // Did it work?
    if (ninode == 0) {
        printf("Error: out of inodes\n");
        return 0;
    }

    // Now to find a free one.
    for (ino = ninode+1; itod(ino) < f->fs->fs_isize; ino++) {
        in = fsreadinode(f, ino);
        if (in->i_mode == 0) {
            f->fs->fs_inode[f->fs->fs_ninode++] = ino;
            if (f->fs->fs_ninode == NICINOD)
                break;
        }
    }

    fssync(f);

    return ninode;
}

int64_t fsblockalloc(struct filesystem *f)
{
    int64_t bno;

    if (f->fs->fs_tfree == 0) {
        printf("disk full\n");
        return 0;
    }
 
    if (f->fs->fs_nfree == 0) {
        printf("no free space and no scraping yet\n");
        return 0;
    }

    f->fs->fs_tfree--;
    bno = f->fs->fs_free[--f->fs->fs_nfree];
    fssync(f);
    return (bno);
}

void *fsreadblock(struct filesystem *f, int64_t bno)
{
    int n;
    int64_t offset;

    char *buf = malloc(DEV_BSIZE);
    if (!buf) {
        printf("Unable to allocate buffer!!!!!\n");
        return NULL;
    }

    offset = (int64_t) bno*DEV_BSIZE;
    if (lseek(f->fd, offset, 0) != offset) {
        printf("lseek read error: %lu\n", (unsigned long)bno);
        free(buf);
        return NULL;
    }
    n = read(f->fd, buf, DEV_BSIZE);
    if (n != DEV_BSIZE) {
        printf("read error: %ld\n", (long)bno);
        free(buf);
        return NULL;
    }
    return buf;
}

int fswriteblock(struct filesystem *f, int64_t bno, void *buf)
{
    int n;
    int64_t offset;

    offset = (int64_t) bno*DEV_BSIZE;
    if (lseek(f->fd, offset, 0) != offset) {
        printf ("lseek failed on block number %lu, offset=%lu\n", (unsigned long)bno, (unsigned long)offset);
        return 0;
    }
    n = write(f->fd, buf, DEV_BSIZE);
    if (n != DEV_BSIZE) {
        printf("write error: %ld\n", (long)bno);
        return 0;
    }
    return 1;
}

struct inode *fsreadinode(struct filesystem *f, int64_t ino)
{
    char *buf;
    struct inode *i;
    int64_t off;
    struct dinode *dp;
    int n;

    buf = fsreadblock(f, itod(ino));
    if (!buf) {
        printf("Failed reading inode block\n");
        return NULL;
    }

    i = malloc(sizeof(struct inode));
    off = itoo(ino);

    dp = (struct dinode *)(buf + (sizeof(struct dinode) * off)); 
    i->i_number = ino;
    i->i_ic1 = dp->di_icom1;
    i->i_flags = dp->di_flags;
    i->i_ic2 = dp->di_icom2;
    for (n = 0; n < NADDR; n++)
        i->i_addr[n] = dp->di_addr[n];
    
    free(buf);

    return i;
}

int fswriteinode(struct filesystem *f, struct inode *ip)
{
    char *buf;
    struct dinode *dp;
    int64_t d;
    int n;

    d = itod(ip->i_number);
    if (d >= f->fs->fs_isize) {
        printf("ilist too small\n");
        return 0;
    }
    buf = fsreadblock(f, d);
    if (!buf) {
        return 0;
    }

    dp = (struct dinode *)(buf + (itoo(ip->i_number) * sizeof(struct dinode)));

    dp->di_icom1 = ip->i_ic1;
    dp->di_flags = ip->i_flags;
    dp->di_icom2 = ip->i_ic2;
    for (n = 0; n < NADDR; n++)
        dp->di_addr[n] = ip->i_addr[n];

    if (!fswriteblock(f, d, buf)) {
        free(buf);
        return 0;
    }
    free(buf);
    return 1;
}


void fsfreeblock(struct filesystem *f, int64_t bno)
{
    if (bno != 0)
        f->fs->fs_tfree++;
    if (f->fs->fs_nfree < NICFREE) {
        f->fs->fs_free[f->fs->fs_nfree++] = bno;
    }
    fssync(f);
}

struct filesystem *fsopen(char *filename)
{
    struct filesystem *f;
    f = malloc(sizeof(struct filesystem));

    f->fd = open(filename, O_RDWR);
    if (!f->fd) {
        free(f);
        return NULL;
    }

    f->fs = (struct fs *)fsreadblock(f, SUPERB);
    if (!f->fs) {
        printf("%s: Bad superblock\n", filename);
        free(f);
        return NULL;
    }
      
    f->root = fsreadinode(f, ROOTINO);
    if (!f->root) {
        printf("%s: Bad root directory\n", filename);
        free(f->fs);
        free(f);
        return NULL;
    }
    return f;
}

void fsclose(struct filesystem *f)
{
    if (!f) 
        return;

    if (f->fd) {
        if (f->fs) {
            fswriteblock(f, SUPERB, f->fs);
        }
        close(f->fd);
    }

    if (f->root)
        free(f->root);

    if (f->fs)
        free(f->fs);
    free(f);
}

void bfree(struct filesystem *f, int64_t bno)
{
    register int i;
    struct fblk *fb = malloc(DEV_BSIZE);

    if (bno != 0)
        f->fs->fs_tfree++;
    if (f->fs->fs_nfree >= NICFREE) {
        fb->df_nfree = f->fs->fs_nfree;
        for (i=0; i<NICFREE; i++)
            fb->df_free[i] = f->fs->fs_free[i];
        fswriteblock(f, bno, fb);
        printf("Writing free list to %d\n", (int)bno);
        f->fs->fs_nfree = 0;
    }
    f->fs->fs_free[f->fs->fs_nfree++] = bno;
}

void bflist(struct filesystem *f)
{
    struct inode in;
    int64_t d;

    bzero(&in, sizeof (in));
    in.i_number = 1;        /* inode 1 is a historical hack */
    in.i_mode = IFREG;
    fswriteinode(f, &in);

    bfree(f, (int64_t)0);
        d = f->fs->fs_fsize;
    while (--d >= f->fs->fs_isize + f->fs->fs_swapsz) {
        bfree(f, d);
    }
    printf("Resultant free list size %d\n", f->fs->fs_nfree);
}

struct filesystem *fsnew(char *filename, unsigned int blocks, unsigned int bpi)
{
    struct filesystem *f;
    unsigned int i;
    char *buffer;

    f = malloc(sizeof(DEV_BSIZE));
    
    f->fd = open(filename, O_RDWR | O_TRUNC | O_CREAT, 0644);
    if (!f->fd) {
        free(f);
        return NULL;
    }

    buffer = malloc(DEV_BSIZE);
    bzero(buffer, DEV_BSIZE);

    for (i=0; i<blocks; i++) {
        fswriteblock(f, i, buffer);
    }

    free(buffer);

    f->fs = malloc(DEV_BSIZE);
    f->fs->fs_fsize = blocks;

    fsformat(f, bpi);
    return f;
}

int fsformat(struct filesystem *f, int bpi)
{
    unsigned int nino;
    char *buffer = malloc(DEV_BSIZE);
    unsigned int i;

    nino = (f->fs->fs_fsize * DEV_BSIZE / bpi) / INOPB;
    if (nino <= 0) {
        nino = 1;
    }

    f->fs->fs_isize = nino+1;
    f->fs->fs_swapsz = 0;
    f->fs->fs_time = time(NULL);
    f->fs->fs_magic1 = FSMAGIC1;
    f->fs->fs_magic2 = FSMAGIC2;
    f->fs->fs_tfree = 0;
    f->fs->fs_tinode = 0;
    f->fs->fs_ninode = 0;
    f->fs->fs_nfree = 0;

    bzero(buffer, DEV_BSIZE);

    for (i = SUPERB+1; i != f->fs->fs_isize; i++) {
        if (!fswriteblock(f, i, buffer)) {
            close(f->fd);
            free(f->fs);
            free(buffer);
            return 0;
        }
        f->fs->fs_tinode += INOPB;
    }

    bflist(f);

    fsinit(f);

    free(buffer);
    return 1;
}

void fsinit(struct filesystem *f)
{
    int i;
    struct inode node;
    char *buf = malloc(DEV_BSIZE);
    time_t utime;

    struct direct lost_found_dir[] = {
        { LOSTFOUNDINO, sizeof(struct direct), 1, "." },
        { ROOTINO,      sizeof(struct direct), 2, ".." },
        { 0,            DIRBLKSIZ,             0, "" },
    };

    struct direct root_dir[] = {
        { ROOTINO,      sizeof(struct direct), 1,  "." },
        { ROOTINO,      sizeof(struct direct), 2,  ".." },
        { LOSTFOUNDINO, sizeof(struct direct), 10, "lost+found" },
        { 0,            DIRBLKSIZ,             0, "" },
    };

    utime = time(NULL);

    node.i_atime = utime;
    node.i_mtime = utime;
    node.i_ctime = utime;

    makedir(lost_found_dir, 2, buf);
    for (i = DIRBLKSIZ; i < DEV_BSIZE; i += DIRBLKSIZ)
        bcopy(&lost_found_dir[2], &buf[i], DIRSIZ(&lost_found_dir[2]));

    node.i_number = LOSTFOUNDINO;
    node.i_mode = IFDIR | 0755;
    node.i_nlink = 2;
    node.i_size = DEV_BSIZE;
    node.i_db[0] = fsblockalloc(f);

    fswriteblock(f, node.i_db[0], buf);
    fswriteinode(f, &node);

    node.i_number = ROOTINO;
    node.i_mode = IFDIR | 0755;
    node.i_nlink = 3;
    node.i_size = makedir(root_dir, 3, buf);
    node.i_db[0] = fsblockalloc(f);

    fswriteblock(f, node.i_db[0], buf);
    fswriteinode(f, &node);

    free(buf);
}
 
