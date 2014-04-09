#ifndef _LIBUFS_H
#define _LIBUFS_H

#include <sys/types.h>

#define dev_t int
#define ino_t u_int
#define time_t long

#define DEV_BSIZE       1024                // Block size
#define BPI             (16 * DEV_BSIZE)    // Bytes per inode
#define ROOTINO         ((u_int)2)          // i number of all roots
#define LOSTFOUNDINO    (ROOTINO + 1)       // i number of lost+found
#define NICINOD         32      // number of superblock inodes 
#define NICFREE         200     // number of superblock free blocks 
#define MAXMNTLEN   28
#define NSHIFT      8
#define NMASK       0377L   
#define MAXNAMLEN   63
#define MAXSYMLINKS 8
#define DEV_BMASK   (DEV_BSIZE-1)
#define B_CLRBUF    0x01
#define DEV_BSHIFT  10  
#define DIRBLKSIZ   1024
#define NINDIR      (DEV_BSIZE / sizeof(long))
#define INOPB       16
#define MAXPATHLEN  256
#define FSMAGIC1    ('F' | 'S'<<8 | '<'<<16 | '<'<<24)
#define FSMAGIC2    ('>' | '>'<<8 | 'F'<<16 | 'S'<<24)
#define SUPERB      ((long)0)    /* block number of the super block */
#define PINOD       10

#define   dbtofsb(b)  ((long) (b))
#define itod(x)     ((long)((((u_int)(x) + INOPB - 1) / INOPB)))
#define itoo(x)     ((int)(((x) + INOPB - 1) % INOPB))
#define DIRSIZ(dp) \
    ((((sizeof (struct direct) - (MAXNAMLEN+1)) + (dp)->d_namlen+1) + 3) &~ 3)
#define blkoff(loc)     /* calculates (loc % fs->fs_bsize) */ \
    ((loc) & DEV_BMASK)
#define lblkno(loc)     /* calculates (loc / fs->fs_bsize) */ \
    ((unsigned) (loc) >> DEV_BSHIFT)

#define NDINIT(ndp,op,flags,namep) {\
    (ndp)->ni_nameiop = op | flags; \
    (ndp)->ni_dirp = namep; }


#define roundup(x,y)    ((((x)+((y)-1))/(y))*(y))
#define LOOKUP      0   /* perform name lookup only */
#define CREATE      1   /* setup for file creation */
#define DELETE      2   /* setup for file deletion */
#define LOCKPARENT  0x10    /* see the top of namei */
#define NOCACHE     0x20    /* name must not be left in cache */
#define FOLLOW      0x40    /* follow symbolic links */
#define NOFOLLOW    0x0 /* don't follow symbolic links (pseudo) */

#define B_READ      0x00001     /* read when I/O occurs */
#define B_ERROR     0x00004     /* transaction aborted */


struct  fs
{
    u_int   fs_magic1;      /* magic word */
    u_int   fs_isize;       /* first block after i-list */
    u_int   fs_fsize;       /* size in blocks of entire volume */
    u_int   fs_swapsz;      /* size in blocks of swap area */
    int fs_nfree;       /* number of addresses in fs_free */
    daddr_t fs_free [NICFREE];  /* free block list */
    int fs_ninode;      /* number of inodes in fs_inode */
    ino_t   fs_inode [NICINOD]; /* free inode list */
    int     fs_flock;       /* lock during free list manipulation */
    int     fs_fmod;        /* super block modified flag */
    int     fs_ilock;       /* lock during i-list manipulation */
    int     fs_ronly;       /* mounted read-only flag */
    time_t  fs_time;        /* last super block update */
    u_int   fs_tfree;       /* total free blocks */
    ino_t   fs_tinode;      /* total free inodes */
    char    fs_fsmnt [MAXMNTLEN];   /* ordinary file mounted on */
    ino_t   fs_lasti;       /* start place for circular search */
    ino_t   fs_nbehind;     /* est # free inodes before s_lasti */
    u_int   fs_flags;       /* mount time flags */
    u_int   fs_magic2;      /* magic word */
/* actually longer */
};


struct  fblk {
    int df_nfree;       /* number of addresses in df_free */
    daddr_t df_free [NICFREE];  /* free block list */
};

struct  direct {
    ino_t   d_ino;          /* inode number of entry */
    u_short d_reclen;       /* length of this record */
    u_short d_namlen;       /* length of string in d_name */
    char    d_name[MAXNAMLEN+1];    /* name must be no longer than this */
};

#define NDADDR  4           /* direct addresses in inode */
#define NIADDR  3           /* indirect addresses in inode */
#define NADDR   (NDADDR + NIADDR)   /* total addresses in inode */

struct icommon1 {
    u_short ic_mode;        /* mode and type of file */
    u_short ic_nlink;       /* number of links to file */
    uid_t   ic_uid;         /* owner's user id */
    gid_t   ic_gid;         /* owner's group id */
    off_t   ic_size;        /* number of bytes in file */
};

struct icommon2 {
    time_t  ic_atime;       /* time last accessed */
    time_t  ic_mtime;       /* time last modified */
    time_t  ic_ctime;       /* time created */
};

struct inode {
    struct inode    *i_chain[2];    /* must be first */
    u_int       i_flag;
    u_int       i_count;    /* reference count */
    dev_t       i_dev;      /* device where inode resides */
    ino_t       i_number;   /* i number, 1-to-1 with device address */
    u_int       i_id;       /* unique identifier */
    struct fs       *i_fs;      /* file sys associated with this inode */
    union {
        struct {
            u_short I_shlockc;  /* count of shared locks */
            u_short I_exlockc;  /* count of exclusive locks */
        } i_l;
        struct  proc *I_rsel;   /* pipe read select */
    } i_un0;
    union {
        struct  proc *I_wsel;   /* pipe write select */
    } i_un1;
    union {
        daddr_t I_addr[NADDR];      /* normal file/directory */
        struct {
            daddr_t I_db[NDADDR];   /* normal file/directory */
            daddr_t I_ib[NIADDR];
        } i_f;
        struct {
            /*
             * the dummy field is here so that the de/compression
             * part of the iget/iput routines works for special
             * files.
             */
            u_int   I_dummy;
            dev_t   I_rdev;     /* dev type */
        } i_d;
    } i_un2;
    union {
        daddr_t if_lastr;       /* last read (read-ahead) */
        struct  {
            struct inode  *if_freef; /* free list forward */
            struct inode **if_freeb; /* free list back */
        } i_fr;
    } i_un3;
    struct icommon1 i_ic1;
    u_int   i_flags;            /* user changeable flags */
    struct icommon2 i_ic2;
};

struct dinode {
    struct  icommon1 di_icom1;
    daddr_t di_addr[NADDR];     /* 7 block addresses 4 bytes each */
    u_int   di_reserved[1];     /* pad of 4 to make total size 64 */
    u_int   di_flags;
    struct  icommon2 di_icom2;
};

struct nameidata {
    caddr_t ni_dirp;        /* pathname pointer */
    short   ni_nameiop;     /* see below */
    short   ni_error;       /* error return if any */
    off_t   ni_endoff;      /* end of useful stuff in directory */
    struct  inode *ni_pdir;     /* inode of parent directory of dirp */
    struct  inode *ni_ip;       /* inode of dirp */
    off_t   ni_offset;      /* offset in directory */
    u_short ni_count;       /* offset of open slot (off_t?) */
    struct  direct ni_dent;     /* current directory entry */
};

#define i_mode      i_ic1.ic_mode
#define i_nlink     i_ic1.ic_nlink
#define i_uid       i_ic1.ic_uid
#define i_gid       i_ic1.ic_gid
#define i_size      i_ic1.ic_size
#define i_shlockc   i_un0.i_l.I_shlockc
#define i_exlockc   i_un0.i_l.I_exlockc
#define i_rsel      i_un0.I_rsel
#define i_wsel      i_un1.I_wsel
#define i_db        i_un2.i_f.I_db
#define i_ib        i_un2.i_f.I_ib
#define i_atime     i_ic2.ic_atime
#define i_mtime     i_ic2.ic_mtime
#define i_ctime     i_ic2.ic_ctime
#define i_rdev      i_un2.i_d.I_rdev
#define i_addr      i_un2.I_addr
#define i_dummy     i_un2.i_d.I_dummy
#define i_lastr     i_un3.if_lastr
#define i_forw      i_chain[0]
#define i_back      i_chain[1]
#define i_freef     i_un3.i_fr.if_freef
#define i_freeb     i_un3.i_fr.if_freeb

#define di_ic1      di_icom1
#define di_ic2      di_icom2
#define di_mode     di_ic1.ic_mode
#define di_nlink    di_ic1.ic_nlink
#define di_uid      di_ic1.ic_uid
#define di_gid      di_ic1.ic_gid
#define di_size     di_ic1.ic_size
#define di_atime    di_ic2.ic_atime
#define di_mtime    di_ic2.ic_mtime
#define di_ctime    di_ic2.ic_ctime
/* i_flag */
#define ILOCKED     0x1     /* inode is locked */
#define IUPD        0x2     /* file has been modified */
#define IACC        0x4     /* inode access time to be updated */
#define IMOUNT      0x8     /* inode is mounted on */
#define IWANT       0x10        /* some process waiting on lock */
#define ITEXT       0x20        /* inode is pure text prototype */
#define ICHG        0x40        /* inode has been changed */
#define ISHLOCK     0x80        /* file has shared lock */
#define IEXLOCK     0x100       /* file has exclusive lock */
#define ILWAIT      0x200       /* someone waiting on file lock */
#define IMOD        0x400       /* inode has been modified */
#define IRENAME     0x800       /* inode is being renamed */
#define IPIPE       0x1000      /* inode is a pipe */
#define IRCOLL      0x2000      /* read select collision on pipe */
#define IWCOLL      0x4000      /* write select collision on pipe */
#define IXMOD       0x8000      /* inode is text, but impure (XXX) */

/* i_mode */
#define IFMT        0170000     /* type of file */
#define IFCHR       0020000     /* character special */
#define IFDIR       0040000     /* directory */
#define IFBLK       0060000     /* block special */
#define IFREG       0100000     /* regular */
#define IFLNK       0120000     /* symbolic link */
#define IFSOCK      0140000     /* socket */
#define ISUID       04000       /* set user id on execution */
#define ISGID       02000       /* set group id on execution */
#define ISVTX       01000       /* save swapped text even after use */
#define IREAD       0400        /* read, write, execute permissions */
#define IWRITE      0200
#define IEXEC       0100

#define IUPDAT(ip, t1, t2, waitfor) { \
    if (ip->i_flag&(IUPD|IACC|ICHG|IMOD)) \
        iupdat(ip, t1, t2, waitfor); \
}

#define ITIMES(ip, t1, t2) { \
    if ((ip)->i_flag&(IUPD|IACC|ICHG)) { \
        (ip)->i_flag |= IMOD; \
        if ((ip)->i_flag&IACC) \
            (ip)->i_atime = (t1)->tv_sec; \
        if ((ip)->i_flag&IUPD) \
            (ip)->i_mtime = (t2)->tv_sec; \
        if ((ip)->i_flag&ICHG) \
            (ip)->i_ctime = time.tv_sec; \
        (ip)->i_flag &= ~(IACC|IUPD|ICHG); \
    } \
}

struct bufhd
{
    int32_t b_flags;        /* see defines below */
    struct  buf *b_forw, *b_back;   /* fwd/bkwd pointer in chain */
};

struct buf
{
    int32_t b_flags;        /* see defines below */
    struct  buf *b_forw, *b_back;   /* hash chain (2 way street) */
    struct  buf *av_forw, *av_back; /* position on free list if not BUSY */
#define b_actf  av_forw         /* alternate names for driver queue */
#define b_actl  av_back         /*    head - isn't history wonderful */
    u_int32_t   b_bcount;       /* transfer count */
#define b_active b_bcount       /* driver queue head: drive active */
    int32_t b_error;        /* returned after I/O */
    dev_t   b_dev;          /* major+minor device name */
    caddr_t b_addr;         /* core address */
    long b_blkno;        /* block # on device */
    u_int32_t   b_resid;        /* words not transferred after error */
#define b_cylin b_resid         /* disksort */
#define b_errcnt b_resid        /* while i/o in progress: # retries */
};

#define dotdot_ino  dtdt_ino
#define dotdot_reclen   dtdt_rec
#define dotdot_name dtdt_name
struct dirtemplate {
    u_int   dot_ino;
    u_short dot_reclen;
    u_short dot_namlen;
    char    dot_name[4];        /* must be multiple of 4 */
    u_int   dotdot_ino;
    u_short dotdot_reclen;
    u_short dotdot_namlen;
    char    dotdot_name[4];     /* ditto */
};

struct filesystem {
    int32_t fd;
    struct fs *fs;
    struct inode *root;
};


extern int32_t makedir(struct direct *protodir, int32_t entries, void *buf);
extern void *fsreadblock(struct filesystem *f, int64_t bno);
extern int32_t fsformat(struct filesystem *f, int32_t bpi);
extern void fsinit(struct filesystem *f);
extern struct filesystem *fsopen(char *filename);
extern void fsclose(struct filesystem *f);
extern struct filesystem *fsnew(char *filename, u_int32_t blocks, u_int32_t bpi);
extern int32_t fsformat(struct filesystem *f, int32_t bpi);
extern struct inode *fsreadinode(struct filesystem *f, int64_t ino);
extern int32_t fswriteblock(struct filesystem *f, int64_t bno, void *buf);
extern int32_t fswriteinode(struct filesystem *f, struct inode *ip);
extern u_int32_t fsinodealloc(struct filesystem *f);
extern int64_t fsblockalloc(struct filesystem *f);
extern int32_t umkdir(struct filesystem *f, char *path);
extern int uls(struct filesystem *f, char *path);
extern int uchmod(struct filesystem *f, char *path, int mode);
extern int uchown(struct filesystem *f, char *path, int uid);
extern int uchgrp(struct filesystem *f, char *path, int gid);
extern struct inode *inodebypath(struct filesystem *f, char *path);


typedef struct {
    struct filesystem *f;
    struct inode *in;
    off_t readoffset;
    off_t writeoffset;
    int perm;
} UFILE;

extern char *getsetting(char *setting);
extern void storesetting(char *setting, char *value);
extern char **splitpath(char *path, int *ndir);
extern void compresspath(char *path);

extern UFILE *ufopen(struct filesystem *f, char *path, char *mode);
extern void ufclose(UFILE *f);
extern int ufgetc(UFILE *f);

#endif
