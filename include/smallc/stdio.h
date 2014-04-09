/*
 * SmallC: interface to stdio library.
 */
#define	BUFSIZ	1024

#ifndef	NULL
#define	NULL	0
#endif

#define	FILE	int
#define	EOF	(-1)

extern int _iob[];

#define	stdin	(&_iob[0])
#define	stdout	(&_iob[5])
#define	stderr	(&_iob[10])

#define	SEEK_SET 0	/* set file offset to offset */
#define	SEEK_CUR 1	/* set file offset to current plus offset */
#define	SEEK_END 2	/* set file offset to EOF plus offset */

#define getc fgetc
#define putc fputc
