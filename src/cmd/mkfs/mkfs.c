/*
 * Make a file system.  Run by 'newfs' and not directly by users.
 * usage: mkfs -s size -i byte/ino -n num -m num filsys
 *
 * NOTE:  the size is specified in filesystem (1k) blocks.
 */
#include <sys/param.h>

/*
 * Need to do the following to get the larger incore inode structure
 * (the kernel might be built with the inode times external/mapped-out).
 * See /sys/h/localtimes.h and /sys/conf.
 */
#undef EXTERNALITIMES

#include <sys/file.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <sys/fs.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/inode.h>
#include <sys/rdisk.h>
#include <sys/ioctl.h>

#define UMASK	0755
#define MAXFN	750

time_t	utime;

#ifdef CROSS
#define off_t unsigned long long
#endif

int	fsi;
int	fso;
char	buf [DEV_BSIZE];

union {
	struct fblk fb;
	char pad1 [DEV_BSIZE];
} fbuf;

union {
	struct fs fs;
	char pad2 [DEV_BSIZE];
} filsys;

u_int	f_i	= 16 * 1024;		/* bytes/inode default */

/*
 * initialize the file system
 */
struct inode node;

struct direct root_dir[] = {
	{ ROOTINO,      sizeof(struct direct), 1,  "." },
	{ ROOTINO,      sizeof(struct direct), 2,  ".." },
	{ LOSTFOUNDINO, sizeof(struct direct), 10, "lost+found" },
	{ 0,            DIRBLKSIZ,             0, "" },
};

struct direct root_dir_with_swap[] = {
	{ ROOTINO,      sizeof(struct direct), 1,  "." },
	{ ROOTINO,      sizeof(struct direct), 2,  ".." },
	{ LOSTFOUNDINO, sizeof(struct direct), 10, "lost+found" },
	{ ROOTINO+2,    sizeof(struct direct), 4,  "swap" },
	{ 0,            DIRBLKSIZ,             0, "" },
};

struct direct lost_found_dir[] = {
	{ LOSTFOUNDINO, sizeof(struct direct), 1, "." },
	{ ROOTINO,      sizeof(struct direct), 2, ".." },
	{ 0,            DIRBLKSIZ,             0, "" },
};

int get_disk_size(char *fn)
{
        int size;
        int fd;
        struct stat sb;

	printf("Getting disk size for %s\n",fn);

        // Is it a /dev entry?
        if(strncmp(fn,"/dev/",5)==0)
        {
                fd = open(fn,O_RDONLY);
                ioctl(fd,RDGETMEDIASIZE,&size);
                close(fd);
        } else {
                lstat(fn,&sb);
                size = sb.st_size/1024;
        }
        return size;
}

/*
 * construct a set of directory entries in "buf".
 * return size of directory.
 */
int
makedir (protodir, entries)
	register struct direct *protodir;
	int entries;
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
	return (DIRBLKSIZ);
}

void
rdfs (bno, bf)
	daddr_t bno;
	char *bf;
{
	int n;
	off_t offset;

	offset = (off_t) bno*DEV_BSIZE;
	if (lseek(fsi, offset, 0) != offset) {
		printf("lseek read error: %ld\n", bno);
		exit(1);
	}
	n = read(fsi, bf, DEV_BSIZE);
	if (n != DEV_BSIZE) {
		printf("read error: %ld\n", bno);
		exit(1);
	}
}

void
wtfs (bno, bf)
	daddr_t bno;
	char *bf;
{
	int n;
	off_t offset;

	offset = (off_t) bno*DEV_BSIZE;
	if (lseek(fso, offset, 0) != offset) {
		printf ("wtfs: lseek failed on block number %ld, offset=%ld\n", bno, offset);
		exit(1);
	}
	n = write(fso, bf, DEV_BSIZE);
	if (n != DEV_BSIZE) {
		printf("write error: %ld\n", bno);
		exit(1);
	}
}

void
iput (ip)
	register struct inode *ip;
{
	struct	dinode	buf [INOPB];
	register struct dinode *dp;
	register int i;
	daddr_t d;

	filsys.fs.fs_tinode--;
	d = itod(ip->i_number);
	if (d >= filsys.fs.fs_isize) {
		printf("ilist too small\n");
		return;
	}
	rdfs(d, buf);
	dp = (struct dinode *)buf;
	dp += itoo(ip->i_number);

	for (i=0; i<NADDR; i++)
                dp->di_addr[i] = ip->i_addr[i];
	dp->di_ic1 = ip->i_ic1;
	dp->di_ic2 = ip->i_ic2;
	wtfs(d, buf);
}

daddr_t
alloc()
{
	register int i;
	daddr_t bno;

	filsys.fs.fs_tfree--;
	bno = filsys.fs.fs_free[--filsys.fs.fs_nfree];
	if (bno == 0) {
		printf("out of free space\n");
		exit(1);
	}
	if (filsys.fs.fs_nfree <= 0) {
		rdfs(bno, (char *)&fbuf);
		filsys.fs.fs_nfree = fbuf.fb.df_nfree;
		for (i=0; i<NICFREE; i++)
			filsys.fs.fs_free[i] = fbuf.fb.df_free[i];
	}
	return (bno);
}

/*
 * add a block to swap inode
 */
void
add_swap (daddr_t lbn)
{
	unsigned block [DEV_BSIZE / 4];
	unsigned int shift, i, j;
	daddr_t bn, indir, newb;

	/*
	 * Direct blocks.
	 */
	if (lbn < NDADDR) {
		/* small file algorithm */
		node.i_db[lbn] = filsys.fs.fs_isize + lbn;
		return;
	}

	/*
	 * Addresses NDADDR, NDADDR+1, and NDADDR+2
	 * have single, double, triple indirect blocks.
	 * The first step is to determine
	 * how many levels of indirection.
	 */
	shift = 0;
	i = 1;
	bn = lbn - NDADDR;
	for (j=NIADDR; ; j--) {
		if (j == 0) {
                        printf("too large swap size\n");
			exit(1);
                }
		shift += NSHIFT;
		i <<= NSHIFT;
		if (bn < i)
			break;
		bn -= i;
	}

	/*
	 * Fetch the first indirect block.
	 */
	indir = node.i_ib [NIADDR-j];
	if (indir == 0) {
	        indir = alloc();
                bzero (block, DEV_BSIZE);
                wtfs (indir, (char*) block);
		node.i_ib [NIADDR-j] = indir;
	}

	/*
	 * Fetch through the indirect blocks
	 */
	for (; ; j++) {
                rdfs (indir, (char*) block);
		shift -= NSHIFT;
		i = (bn >> shift) & NMASK;
                if (j == NIADDR) {
                        block[i] = filsys.fs.fs_isize + lbn;
                        wtfs (indir, (char*) block);
                        return;
                }
                if (block[i] != 0) {
                       indir = block [i];
                       continue;
                }
                /* Allocate new indirect block. */
	        newb = alloc();
                block[i] = newb;
                wtfs (indir, (char*) block);

                bzero (block, DEV_BSIZE);
                wtfs (newb, (char*) block);
                indir = newb;
	}
}

/*
 * create swap file
 */
void
mkswap ()
{
        daddr_t lbn;

	node.i_atime = utime;
	node.i_mtime = utime;
	node.i_ctime = utime;

	node.i_number = ROOTINO + 2;
	node.i_mode = IFREG | 0400;
	node.i_nlink = 1;
	node.i_size = filsys.fs.fs_swapsz * DEV_BSIZE;
	node.i_flags = UF_NODUMP | UF_IMMUTABLE /*| SF_IMMUTABLE*/;

        for (lbn=0; lbn<filsys.fs.fs_swapsz; lbn++)
                add_swap (lbn);
        iput(&node);
}

void
fsinit()
{
	register int i;

	/*
	 * initialize the node
	 */
	node.i_atime = utime;
	node.i_mtime = utime;
	node.i_ctime = utime;

	/*
	 * create the lost+found directory
	 */
	(void)makedir(lost_found_dir, 2);
	for (i = DIRBLKSIZ; i < DEV_BSIZE; i += DIRBLKSIZ)
		bcopy(&lost_found_dir[2], &buf[i], DIRSIZ(&lost_found_dir[2]));
	node.i_number = LOSTFOUNDINO;
	node.i_mode = IFDIR | UMASK;
	node.i_nlink = 2;
	node.i_size = DEV_BSIZE;
	node.i_db[0] = alloc();
	wtfs(node.i_db[0], buf);
	iput(&node);

	/*
	 * create the root directory
	 */
	node.i_number = ROOTINO;
	node.i_mode = IFDIR | UMASK;
        node.i_nlink = 3;
	if (filsys.fs.fs_swapsz == 0) {
                node.i_size = makedir(root_dir, 3);
        } else {
                node.i_size = makedir(root_dir_with_swap, 4);
        }
	node.i_db[0] = alloc();
	wtfs(node.i_db[0], buf);
	iput(&node);
}

void
bfree (bno)
	daddr_t bno;
{
	register int i;

	if (bno != 0)
		filsys.fs.fs_tfree++;
	if (filsys.fs.fs_nfree >= NICFREE) {
		fbuf.fb.df_nfree = filsys.fs.fs_nfree;
		for (i=0; i<NICFREE; i++)
			fbuf.fb.df_free[i] = filsys.fs.fs_free[i];
		wtfs(bno, (char *)&fbuf);
		filsys.fs.fs_nfree = 0;
	}
	filsys.fs.fs_free[filsys.fs.fs_nfree++] = bno;
}

void
bflist()
{
	struct inode in;
	daddr_t d;

	bzero(&in, sizeof (in));
	in.i_number = 1;		/* inode 1 is a historical hack */
	in.i_mode = IFREG;
	iput(&in);

	bfree((daddr_t)0);
        d = filsys.fs.fs_fsize;
	while (--d >= filsys.fs.fs_isize + filsys.fs.fs_swapsz) {
		bfree(d);
	}
}

void
usage()
{
	printf("usage: mkfs [-i bytes/ino] [-p swapsize] special kbytes\n");
	exit(1);
	/* NOTREACHED */
}

int
main (argc,argv)
	int	argc;
	char	**argv;
{
	register int c;
	unsigned n, kbytes, swapsz = 0;
	char *special;

	while ((c = getopt(argc, argv, "i:s:")) != EOF) {
		switch (c) {
		case 'i':
			f_i = atoi(optarg);
			break;
		case 's':
			swapsz = strtoul(optarg, 0, 0);
			break;
		default:
			usage();
			break;
		}
	}
	argc -= optind;
	argv += optind;
	if ((argc < 1 || argc > 2) || f_i == 0)
		usage();
	special = argv[0];
	if(argc==2)
		kbytes = strtoul(argv[1], 0, 0);
	else
		kbytes = get_disk_size(argv[0]);

	/*
	 * NOTE: this will fail if the device is currently mounted and the system
	 * is at securelevel 1 or higher.
	 *
	 * We do not get the partition information because 'newfs' has already
	 * done so and invoked us.  This program should not be run manually unless
	 * you are absolutely sure you know what you are doing - use 'newfs' instead.
	 */
	fso = creat (special, 0666);
	if (fso < 0)
		err (1, "cannot create %s\n", special);
	fsi = open (special, 0);
	if (fsi < 0)
		err (1, "cannot open %s\n", special);

	printf ("Size: %u kbytes\n", kbytes);
	if (kbytes == 0) {
		printf ("Can't make zero length filesystem\n");
		return -1;
	}

	/* Check media: write zeroes to last block. */
	wtfs (kbytes-1, (char*) &filsys.fs);

	/*
	 * Calculate number of blocks of inodes as follows:
	 *
	 * n * DEV_BSIZE               = # of bytes in the filesystem
	 * n * DEV_BSIZE / f_i         = # of inodes to allocate
	 * n * DEV_BSIZE / f_i / INOPB = # of fs blocks of inodes
	 *
	 * Pretty - isn't it?
	 */
	n = (kbytes * DEV_BSIZE / f_i) / INOPB;
	if (n <= 0)
		n = 1;
	printf ("Inodes: %u\n", n*INOPB);

	filsys.fs.fs_isize = n + 1;
	filsys.fs.fs_fsize = kbytes;
	filsys.fs.fs_swapsz = swapsz;
	if (filsys.fs.fs_isize + filsys.fs.fs_swapsz >= filsys.fs.fs_fsize) {
		printf ("%u/%u: bad ratio\n",
                        filsys.fs.fs_fsize, filsys.fs.fs_isize-2);
		exit (1);
	}
	time (&utime);
	filsys.fs.fs_time = utime;
	filsys.fs.fs_magic1 = FSMAGIC1;
	filsys.fs.fs_magic2 = FSMAGIC2;
	filsys.fs.fs_tfree = 0;
	filsys.fs.fs_tinode = 0;
	bzero (buf, DEV_BSIZE);
	for (n=SUPERB+1; n != filsys.fs.fs_isize; n++) {
		wtfs (n, buf);
		filsys.fs.fs_tinode += INOPB;
	}

	bflist();

	fsinit();

	if (swapsz != 0)
                mkswap();

	wtfs (SUPERB, (char*) &filsys.fs);
	exit(0);
}
