#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include "libufs.h"

// Split a path into an array of char pointers, each
// one containing an element of the path,
char **splitpath(char *path, int *ndir)
{
    char *p,*q;
    int dn = 0;
    char **dirs;
    *ndir = 0;

    p = path;
    while (*p == '/')
        p++;

    while (*(p+strlen(p)-1) == '/') {
        *(p+strlen(p)-1) = '\0';
    }

    // First count the number of entries we have
    q = p;
    for ( ; *p; p++) {
        if (*p == '/') {
            (*ndir)++;
        }
    }

    if(*q)
        (*ndir)++;
    p = q;

    // Then allocate enough space for them
    dirs = malloc(*ndir * sizeof(char *));

    // Now do the actual splitting.
    for ( ; *p; p++) {
        if ((*p) == '/') {
            *p = 0;
            dirs[dn] = q;
            p++;
            dn++;
            q = p;
        }
    }

    if (*q) {
        dirs[dn] = q;
        dn++;
    }
    if (dn != *ndir) {
        printf("Something odd happened to the path\n");
    }
    
    return dirs;
}

unsigned int getfileinode(struct filesystem *f, struct inode *in, char *fn)
{
    unsigned int ino = 0;
    char *buf;
    char *d;
    struct direct *de;

    buf = fsreadblock(f, in->i_db[0]);
    if (!buf) {
        printf("Bad read\n");
        return 0;
    } 

    d = buf;
    de = (struct direct *)d;
    while ((de < (struct direct *)(buf+DEV_BSIZE)) && (de->d_ino != 0)) {
        if (!strcmp(de->d_name, fn)) {
            ino = de->d_ino;
            break;
        }
        d += de->d_reclen;
        de = (struct direct *)d;
    }

    free(buf);
    
    return ino;
}

struct inode *inodebypath(struct filesystem *f, char *path)
{
    char **dirs = NULL;
    int ndir = 0;
    int i;
    struct inode *in = f->root;
    char *p = strdup(path);
    int ino __attribute__((unused));

    dirs = splitpath(p, &ndir);

    if (ndir == 0) {
        free(dirs);
        free(p); 
        return f->root;
    }

    for (i = 0; i < ndir; i++) {
        ino = getfileinode(f, in, dirs[i]);
        if (ino != 0) {
            if (in != f->root) {
                free(in);
            }
            in = fsreadinode(f, ino);
        } else {
            free(p);
            free(dirs);
            if (in != f->root) {
                free(in);
            }
            return 0;
        }
    }
    free(p);
    free(dirs);
    return in;
}

struct inode *inodebypartpath(struct filesystem *f, char *path, int *depth)
{
    char **dirs = NULL;
    int ndir = 0;
    int i;
    struct inode *in = f->root;
    char *p = strdup(path);
    int ino __attribute__((unused));

    if(depth)
        *depth = 0;

    dirs = splitpath(p, &ndir);

    for (i = 0; i < ndir; i++) {
        ino = getfileinode(f, in, dirs[i]);
        if (ino != 0) {
            if (in != f->root) {
                free(in);
            }
            in = fsreadinode(f, ino);
            if(depth)
                (*depth)++;
        } else {
            printf("Not found\n");
            free(p);
            free(dirs);
            return in;
        }
    }
    free(p);
    free(dirs);
    return in;
}

void listdir(struct filesystem *f, ino_t ino)
{
    struct inode *in;
    struct inode *ip;
    char *buf;
    int i;
    char c;
    char *d;
    struct direct *dp;
    struct tm *tmp;
    char tbuf[22];

    in = fsreadinode(f, ino);

    for (i = 0; i < NADDR; i++) {
        if (in->i_db[i] > 0) {
            buf = fsreadblock(f, in->i_db[i]);

            d = buf;
            while ((d - buf) < DEV_BSIZE) {
                dp = (struct direct *)d;
                ip = fsreadinode(f, dp->d_ino);

                switch (ip->i_mode & IFMT) {
                case IFBLK:
                    printf("b");
                    break;
                case IFCHR:
                    printf("c");
                    break;
                case IFDIR:
                    printf("d");
                    break;
                case IFLNK:
                    printf("l");
                    break;
                default:
                    printf("-");
                    break;
                }

                c = '-';
                if (ip->i_mode & 0400)
                    c = 'r';
                printf("%c", c);

                c = '-';
                if (ip->i_mode & 0200)
                    c = 'w';
                printf("%c", c);

                c = '-';
                if (ip->i_mode & 0100)
                    c = 'x';
                if (ip->i_mode & 04000)
                    c = 's';
                printf("%c", c);

                c = '-';
                if (ip->i_mode & 0040)
                    c = 'r';
                printf("%c", c);

                c = '-';
                if (ip->i_mode & 0020)
                    c = 'w';
                printf("%c", c);

                c = '-';
                if (ip->i_mode & 0010)
                    c = 'x';
                if (ip->i_mode & 02000)
                    c = 's';
                printf("%c", c);

                c = '-';
                if (ip->i_mode & 0004)
                    c = 'r';
                printf("%c", c);

                c = '-';
                if (ip->i_mode & 0002)
                    c = 'w';
                printf("%c", c);

                c = '-';
                if (ip->i_mode & 0001)
                    c = 'x';
                if (ip->i_mode & 01000)
                    c = 't';
                printf("%c", c);

                printf(" %5u", ip->i_number);
                printf(" %8lu", ip->i_size);

                tmp = localtime(&ip->i_mtime);
                strftime(tbuf, 20, "%Y-%m-%d %H:%M", tmp);

                printf(" %s", tbuf);
                printf(" %s", dp->d_name);
#if 0
                printf("\n    ");

                for (i=0; i<NADDR; i++) {
                    printf("%d ", ip->i_addr[i]);
                }
#endif
                printf("\n");


                d += dp->d_reclen;
                free(ip);
            }
            free(buf);
        }
    }
    free(in);
}

struct inode *adddirectory(struct filesystem *f, struct inode *in, char *name)
{
    char *buf;
    char *d;
    struct direct *dp;
    int bno;
    struct inode *ip;
    ino_t ino = fsinodealloc(f);
    int rem;

    int i = 0;

    if (!(in->i_mode & IFDIR)) {
        errno = ENOTDIR;
        return NULL;
    }

    if (ino == 0) {
        printf("Could not allocate inode\n");
        errno = ENOSPC;
        return NULL;
    }

    // Create the new directory

    bno = fsblockalloc(f);

    if (bno == 0) {
        errno = ENOSPC;
        return NULL;
    }

    rem = DEV_BSIZE;
    buf = malloc(DEV_BSIZE);
    d = buf;

    dp = (struct direct *)d;
    dp->d_ino = ino;
    dp->d_namlen = 1;
    dp->d_name[0] = '.'; dp->d_name[1] = '\0';
    dp->d_reclen = DIRSIZ(dp);
    rem -= dp->d_reclen;
    d += dp->d_reclen;
    dp = (struct direct *)d;
    dp->d_ino = in->i_number;
    dp->d_namlen = 2;
    dp->d_name[0] = '.'; dp->d_name[1] = '.'; dp->d_name[2] = '\0';
    dp->d_reclen = rem;

    fswriteblock(f, bno, buf);
    free(buf);

    ip = malloc(sizeof(struct inode));

    for (i = 0; i < NADDR; i++)
        ip->i_db[i] = 0;

    ip->i_number = ino;
    ip->i_db[0] = bno;
    ip->i_mode = IFDIR | 0755;
    ip->i_uid = 0;
    ip->i_gid = 0;
    ip->i_nlink = 2;
    ip->i_size = DEV_BSIZE;
    ip->i_atime = time(NULL);
    ip->i_ctime = time(NULL);
    ip->i_mtime = time(NULL);
    fswriteinode(f, ip);

    // insert the new directory into the parent

    buf = fsreadblock(f, in->i_db[0]);
    if (!buf) {
        printf("Bad read\n");
        errno = EIO;
        return NULL;
    } 
    
    d = buf;
    rem = DEV_BSIZE;
    while ((d - buf) < DEV_BSIZE) {
        dp = (struct direct *)d;
        printf("inode %d (%d %d)\n", (int)dp->d_ino, (int)dp->d_reclen, sizeof(struct direct));
        if (dp->d_reclen != DIRSIZ(dp)) {
            if (dp->d_ino != 0) {
                dp->d_reclen = DIRSIZ(dp);
                rem -= dp->d_reclen;
                d += dp->d_reclen;
                dp = (struct direct *)d;
            }
            dp->d_ino = ino;
            dp->d_namlen = strlen(name);
            bcopy(name, dp->d_name, strlen(name));
            dp->d_reclen = rem;
            if (!fswriteblock(f, in->i_db[0], buf)) {
                errno = EIO;
                free(ip);
                free(buf);
                return NULL;
            }
            in->i_nlink++;
            fswriteinode(f, in);
            free(buf);
            return ip;
        }
        i++;
        rem -= dp->d_reclen;
        d += dp->d_reclen;
    }
    free(ip);
    free(buf);
    errno = ENOSPC;
    return NULL;
}

int umkdir(struct filesystem *f, char *path)
{
    struct inode *in;
    struct inode *ip;
    int sd;
    int i;
    char *p = strdup(path);
    int ndir;
    char **dirs = splitpath(p, &ndir);

    in = inodebypartpath(f, path, &sd);

    if (sd == ndir) {
        if (in != f->root) {
            free(in);
        }
        free(p);
        free(dirs);
        errno = EEXIST;
        return 0;
    }
        
    for (i = sd; i < ndir; i++)
    {
        ip = adddirectory(f, in, dirs[i]);
        if (in != f->root) {
            free(in);
        }
        if (!ip) {
            free(p);
            free(dirs);
            return 0;
        }
        in = ip;
    }
    if (in != f->root) {
        free(in);
    }
    free(p);
    free(dirs);

    return 1;
}

int uls(struct filesystem *f, char *path)
{
    struct inode *in;

    in = inodebypath(f, path);
    if (!in) {
        errno = ENOENT;
        return 0;
    }
    listdir(f, in->i_number);
    if (in != f->root)
        free(in);
    return 1;
}

int uchmod(struct filesystem *f, char *path, int mode)
{
    struct inode *in;
    char *p = strdup(path);
    int ndir;
    char **dirs = splitpath(p, &ndir);
    free(p);
    free(dirs);

    in = inodebypath(f, path);

    if (!in) {
        errno = ENOENT;
        return 0;
    }

    in->i_mode &= ~07777;
    in->i_mode |= (mode & 07777);
    fswriteinode(f, in);
    free(in);
    return 1;
}
        
int uchown(struct filesystem *f, char *path, int uid)
{
    struct inode *in;
    char *p = strdup(path);
    int ndir;
    char **dirs = splitpath(p, &ndir);
    free(p);
    free(dirs);

    in = inodebypath(f, path);

    if (!in) {
        errno = ENOENT;
        return 0;
    }

    in->i_uid = uid;
    fswriteinode(f, in);
    free(in);
    return 1;
}
        
int uchgrp(struct filesystem *f, char *path, int gid)
{
    struct inode *in;
    char *p = strdup(path);
    int ndir;
    char **dirs = splitpath(p, &ndir);
    free(p);
    free(dirs);

    in = inodebypath(f, path);

    if (!in) {
        errno = ENOENT;
        return 0;
    }

    in->i_mode = gid;
    fswriteinode(f, in);
    free(in);
    return 1;
}

int uftruncate(UFILE *file)
{
    return 1;
}

struct inode *fscreatefile(struct filesystem *f, char *path)
{
    return NULL;
}
        
UFILE *ufopen(struct filesystem *f, char *path, char *mode)
{
    UFILE *file = malloc(sizeof(UFILE));
    file->f = f;
    file->in = inodebypath(f, path);    
    file->readoffset = 0;
    file->writeoffset = 0;
    file->perm = 0;

    if (!strcmp(mode, "r")) {
        if (!file->in) {
            return 0;
        }
        file->readoffset = 0;
        file->writeoffset = 0;
        file->perm = O_RDONLY;
    }

    if (!strcmp(mode, "r+")) {
        if (!file->in) {
            return 0;
        }
        file->readoffset = 0;
        file->writeoffset = 0;
        file->perm = O_RDWR;
    }

    if (!strcmp(mode, "w")) {
        if (!file->in) {
            return 0;
        }
        uftruncate(file);
        file->readoffset = 0;
        file->writeoffset = 0;
        file->perm = O_WRONLY;
    }

    if (!strcmp(mode, "w+")) {
        if (!file->in) {
            file->in = fscreatefile(f, path);
        } else {
            uftruncate(file);
        }
        file->readoffset = 0;
        file->writeoffset = 0;
        file->perm = O_RDWR;
    }

    if (!strcmp(mode, "a")) {
        if (!file->in) {
            file->in = fscreatefile(f, path);
        }
        file->readoffset = file->in->i_size;
        file->writeoffset = file->in->i_size;
        file->perm = O_WRONLY;
    }

    if (!strcmp(mode, "a+")) {
        if (!file->in) {
            file->in = fscreatefile(f, path);
        }
        file->readoffset = 0;
        file->writeoffset = file->in->i_size;
        file->perm = O_RDWR;
    }

    return file;
}

void ufclose(UFILE *f)
{
    if (f->in) {
        free(f->in);
    }
    free(f);
}

int ufgetc(UFILE *f)
{
    unsigned char *buf;
    int bno;
    int offset;
    unsigned int addr;
    int c;
    int *ib;

    if (f->readoffset >= f->in->i_size) {
        return EOF;
    }
    bno = f->readoffset / DEV_BSIZE;
    offset = f->readoffset - (bno * DEV_BSIZE); 

    
    if (bno < NADDR-3) {
        addr = f->in->i_db[bno];
    } else {
        bno -= NADDR-3;
        // First level indirect
        if (bno < DEV_BSIZE / 4) {
            buf = fsreadblock(f->f, f->in->i_db[NADDR-3]);
            ib = (daddr_t *)buf;
            addr = ib[bno];
            free(buf);
        } else {
            bno -= (DEV_BSIZE / 4);
            // Double indirect
            if (bno < ((DEV_BSIZE / 4) * (DEV_BSIZE / 4))) {
                buf = fsreadblock(f->f, f->in->i_db[NADDR-2]);
                ib = (daddr_t *)buf;
                addr = ib[bno / (DEV_BSIZE / 4)];
                free(buf);
                buf = fsreadblock(f->f, addr);
                ib = (daddr_t *)buf;
                addr = ib[bno - ((bno / (DEV_BSIZE / 4)) * (DEV_BSIZE / 4))];
                free(buf);
            } else {
                // It gets horrible now - triple indirect.
            }
        }
    }

    buf = fsreadblock(f->f, addr);
    if (!buf) {
        return -EIO;
    }
    f->readoffset++;
    c = buf[offset];
    free(buf);
    return c;
}
