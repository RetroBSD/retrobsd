/*******************************************************************************
 *  The BYTE UNIX Benchmarks - Release 3
 *	Module: fstime.c   SID: 3.5 5/15/91 19:30:19
 *
 *******************************************************************************
 * Bug reports, patches, comments, suggestions should be sent to:
 *
 *	Ben Smith, Rick Grehan or Tom Yager
 *	ben@bytepb.byte.com   rick_g@bytepb.byte.com   tyager@bytepb.byte.com
 *
 *******************************************************************************
 *  Modification Log:
 * $Header: fstime.c,v 3.4 87/06/22 14:23:05 kjmcdonell Beta $
 * 10/19/89 - rewrote timing calcs and added clock check (Ben Smith)
 * 10/26/90 - simplify timing, change defaults (Tom Yager)
 * 11/16/90 - added better error handling and changed output format (Ben Smith)
 * 11/17/90 - changed the whole thing around (Ben Smith)
 * 2/22/91 - change a few style elements and improved error handling (Ben Smith)
 * 4/17/91 - incorporated suggestions from Seckin Unlu (seckin@sumac.intel.com)
 * 4/17/91 - limited size of file, will rewind when reaches end of file
 * 7/95    - fixed mishandling of read() and write() return codes
 *	     Carl Emilio Prelz <fluido@telepac.pt>
 * 12/95   - Massive changes.  Made sleep time proportional increase with run
 *	     time; added fsbuffer and fsdisk variants; added partial counting
 *	     of partial reads/writes (was *full* credit); added dual syncs.
 *	     David C Niemi <niemi@tux.org>
 * 10/22/97 - code cleanup to remove ANSI C compiler warnings
 *           Andy Kahn <kahn@zk3.dec.com>
 ******************************************************************************/
char SCCSid[] = "@(#) @(#)fstime.c:3.5 -- 5/15/91 19:30:19";

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#ifndef SECONDS
#define SECONDS 10
#endif

#ifdef FSDISK
#define BUFSIZE 4096
#define MAX_BLOCKS 8000
#endif

#ifdef FSBUFFER
#define BUFSIZE 256
#define MAX_BLOCKS 500
#endif

#ifndef BUFSIZE
#define BUFSIZE 1024
#endif

/* This must be set to the smallest BUFSIZE or 1024, whichever is smaller */
#define COUNTSIZE 256
#define HALFCOUNT (COUNTSIZE/2)		/* Half of COUNTSIZE */
#define COUNTPERK (1024/COUNTSIZE)	/* Countable units per 1024 bytes */

#define COUNTPERBUF (BUFSIZE/COUNTSIZE)	/* Countable units per BUFSIZE */


#ifndef MAX_BLOCKS
#define MAX_BLOCKS 2000
#endif
		/* max number of BUFSIZE blocks in file */
		/* Don't limit it much, so that memory buffering
		 * can be overcome
		 */
#define _MAXBLOCKS MAX_BLOCKS*1024/BUFSIZE

#define FNAME0	"dummy0"
#define FNAME1	"dummy1"

int w_test(void);
int r_test(void);
int c_test(void);

long read_score = 1, write_score = 1;

/****************** GLOBALS ***************************/
char buf[BUFSIZE];
int			seconds = SECONDS;
int			f;
int			g;
int			i;
void			stop_count();
void			clean_up();
int			sigalarm = 0;

/******************** MAIN ****************************/

int main(argc, argv)
int	argc;
char	*argv[];
{

	/**** initialize ****/
	if (argc > 1)
		seconds = atoi(argv[1]);
	if (argc == 3 && chdir(argv[2]) == -1) {
		perror("fstime: chdir");
		exit(1);
	}


	if((f = creat(FNAME0, 0600)) == -1) {
		perror("fstime: creat");
		exit(1);
	}
	close(f);

	if((g = creat(FNAME1, 0600)) == -1) {
		perror("fstime: creat");
		exit(1);
	}
	close(g);

	if( (f = open(FNAME0, 2)) == -1) {
		perror("fstime: open");
		exit(1);
	}
	if( ( g = open(FNAME1, 2)) == -1 ) {
		perror("fstime: open");
		exit(1);
	}

	/* fill buffer */
	for (i=0; i < BUFSIZE; ++i)
		buf[i] = i & 0xff;

	/*** run the tests ****/
	signal(SIGKILL,clean_up);
	if (w_test() || r_test() || c_test()) {
		clean_up();
		exit(1);
	}

	/* else */
	clean_up();
	exit(0);
}

/* write test */
int w_test(void)
{
	unsigned long counted = 0L;
	unsigned long tmp;
	long f_blocks;
	extern int sigalarm;

	/* Sync and let it settle */
	sync();
	sleep(2);
	sync();
	sleep(1);

	signal(SIGALRM,stop_count);
	sigalarm = 0; /* reset alarm flag */
	alarm(seconds);

	while(!sigalarm) {
		for(f_blocks=0; f_blocks < _MAXBLOCKS; ++f_blocks) {
			if ((tmp=write(f, buf, BUFSIZE)) != BUFSIZE) {
				if (errno != EINTR) {
					perror("fstime: write");
					return(-1);
				}
				stop_count();
				counted += ((tmp+HALFCOUNT)/COUNTSIZE);
			} else
				counted += COUNTPERBUF;
		}
		lseek(f, 0L, 0); /* rewind */
	}

	/* stop clock */
	fprintf(stderr, "%d second sample\n", seconds);
	write_score = counted/((long)seconds * COUNTPERK);
	fprintf(stderr,
		"%ld Kbytes/sec write %d bufsize %d max blocks\n",
		write_score, BUFSIZE, _MAXBLOCKS);

	return(0);
}

/* read test */
int r_test(void)
{
	unsigned long counted = 0L;
	unsigned long tmp;
	extern int sigalarm;
	extern int errno;

	/* Sync and let it settle */
	sync();
	sleep(2 + seconds/4);
	sync();
	sleep(1 + seconds/4);

	/* rewind */
	errno = 0;
	lseek(f, 0L, 0);

	signal(SIGALRM,stop_count);
	sigalarm = 0; /* reset alarm flag */
	alarm(seconds);
	while(!sigalarm) {
		/* read while checking for an error */
		if ((tmp=read(f, buf, BUFSIZE)) != BUFSIZE) {
			switch(errno) {
			case 0:
			case EINVAL:
				lseek(f, 0L, 0);  /* rewind at end of file */
				counted += (tmp+HALFCOUNT)/COUNTSIZE;
				continue;
			case EINTR:
				stop_count();
				counted += (tmp+HALFCOUNT)/COUNTSIZE;
				break;
			default:
				perror("fstime: read");
				return(-1);
				break;
			}
		} else
			counted += COUNTPERBUF;
	}

	/* stop clock */
	fprintf(stderr, "%d second sample\n", seconds);
	read_score = counted / ((long)seconds * COUNTPERK);
	fprintf(stderr,
		"%ld Kbytes/sec read %d bufsize %d max blocks \n",
		read_score, BUFSIZE, _MAXBLOCKS);
	return(0);
}


/* copy test */
int c_test(void)
{
	unsigned long counted = 0L;
	unsigned long tmp;
	extern int sigalarm;

	sync();
	sleep(2 + seconds/4);
	sync();
	sleep(1 + seconds/8);

	/* rewind */
	errno = 0;
	lseek(f, 0L, 0);

	signal(SIGALRM,stop_count);
	sigalarm = 0; /* reset alarm flag */
	alarm(seconds);

	while (! sigalarm) {
		if ((tmp=read(f, buf, BUFSIZE)) != BUFSIZE) {
			switch(errno) {
			case 0:
			case EINVAL:
				lseek(f, 0L, 0);  /* rewind at end of file */
				lseek(g, 0L, 0);  /* rewind the output too */
				continue;
			case EINTR:
				/* part credit for leftover bytes read */
				counted += ( (tmp * write_score) /
					(read_score + write_score)
					+ HALFCOUNT) / COUNTSIZE;
				stop_count();
				break;
			default:
				perror("fstime: copy read");
				return(-1);
				break;
			}
		} else  {
			if ((tmp=write(g, buf, BUFSIZE)) != BUFSIZE) {
				if (errno != EINTR) {
					perror("fstime: copy write");
					return(-1);
				}
				counted += (
				 /* Full credit for part of buffer written */
					tmp +

				 /* Plus part credit having read full buffer */
					( ((BUFSIZE - tmp) * write_score) /
					(read_score + write_score) )
					+ HALFCOUNT) / COUNTSIZE;
				stop_count();
			} else
				counted += COUNTPERBUF;
		}
	}
	/* stop clock */

	fprintf(stderr, "%d second sample\n", seconds);
	fprintf(stderr,
		"%ld Kbytes/sec copy %d bufsize %d max blocks \n",
		counted / ((long)seconds * COUNTPERK), BUFSIZE, _MAXBLOCKS);
	return(0);
}

void stop_count(void)
{
	extern int sigalarm;
	sigalarm = 1;
}

void clean_up(void)
{
	unlink(FNAME0);
	unlink(FNAME1);
}
