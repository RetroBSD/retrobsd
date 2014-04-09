/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#ifndef	_STAT_H_
#define	_STAT_H_

struct	stat
{
	dev_t	st_dev;
	ino_t	st_ino;
	u_int	st_mode;
	int	st_nlink;
	uid_t	st_uid;
	gid_t	st_gid;
	dev_t	st_rdev;
	off_t	st_size;
	time_t	st_atime;
	time_t	st_mtime;
	time_t	st_ctime;
	long	st_blksize;
	long	st_blocks;
	u_int	st_flags;
};

#define	S_IFMT	0170000		/* type of file */
#define		S_IFDIR	0040000	/* directory */
#define		S_IFCHR	0020000	/* character special */
#define		S_IFBLK	0060000	/* block special */
#define		S_IFREG	0100000	/* regular */
#define		S_IFLNK	0120000	/* symbolic link */
#define		S_IFSOCK 0140000/* socket */
#define	S_ISUID	0004000		/* set user id on execution */
#define	S_ISGID	0002000		/* set group id on execution */
#define	S_ISVTX	0001000		/* save swapped text even after use */
#define	S_IREAD	0000400		/* read permission, owner */
#define	S_IWRITE 0000200	/* write permission, owner */
#define	S_IEXEC	0000100		/* execute/search permission, owner */

/*
 * Definitions of flags in mode that are 4.4 compatible.
 */

#define S_IFIFO 0010000		/* named pipe (fifo) - Not used by 2.11BSD */

#define S_IRWXU 0000700		/* RWX mask for owner */
#define S_IRUSR 0000400		/* R for owner */
#define S_IWUSR 0000200		/* W for owner */
#define S_IXUSR 0000100		/* X for owner */

#define S_IRWXG 0000070		/* RWX mask for group */
#define S_IRGRP 0000040		/* R for group */
#define S_IWGRP 0000020		/* W for group */
#define S_IXGRP 0000010		/* X for group */

#define S_IRWXO 0000007		/* RWX mask for other */
#define S_IROTH 0000004		/* R for other */
#define S_IWOTH 0000002		/* W for other */
#define S_IXOTH 0000001		/* X for other */

#define	S_ISDIR(m)	((m & S_IFMT) == S_IFDIR)	/* directory */
#define	S_ISCHR(m)	((m & S_IFMT) == S_IFCHR)	/* character special */
#define	S_ISBLK(m)	((m & S_IFMT) == S_IFBLK)	/* block special */
#define	S_ISREG(m)	((m & S_IFMT) == S_IFREG)	/* regular */
#define	S_ISLNK(m)	((m & S_IFMT) == S_IFLNK)	/* symbolic link */
#define	S_ISSOCK(m)	((m & S_IFMT) == S_IFSOCK)      /* socket */

/*
 * Definitions of flags stored in file flags word.  Different from 4.4 because
 * 2.11BSD only could afford a u_short for the flags.  It is not a great
 * inconvenience since there are still 5 bits in each byte available for
 * future use.
 *
 * Super-user and owner changeable flags.
 */
#define	UF_SETTABLE	0x00ff		/* mask of owner changeable flags */
#define	UF_NODUMP	0x0001		/* do not dump file */
#define	UF_IMMUTABLE	0x0002		/* file may not be changed */
#define	UF_APPEND	0x0004		/* writes to file may only append */
/*
 * Super-user changeable flags.
 */
#define	SF_SETTABLE	0xff00		/* mask of superuser changeable flags */
#define	SF_ARCHIVED	0x0100		/* file is archived */
#define	SF_IMMUTABLE	0x0200		/* file may not be changed */
#define	SF_APPEND	0x0400		/* writes to file may only append */

#ifdef KERNEL
/*
 * Shorthand abbreviations of above.
 */
#define	APPEND		(UF_APPEND | SF_APPEND)
#define	IMMUTABLE	(UF_IMMUTABLE | SF_IMMUTABLE)
#else

int     chmod (const char *path, mode_t mode);
int     fchmod (int fd, mode_t mode);
mode_t  umask (mode_t cmask);

#endif

#endif /* !_STAT_H_ */
