/*-
 * Copyright (c) 1991, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Modified for 2.11BSD by removing prototypes.  To save time and space
 * functions not returning 'int' and functions not present in the system
 * are not listed.
*/

#ifndef _UNISTD_H_
#define	_UNISTD_H_

#include <sys/types.h>

#define	STDIN_FILENO	0	/* standard input file descriptor */
#define	STDOUT_FILENO	1	/* standard output file descriptor */
#define	STDERR_FILENO	2	/* standard error file descriptor */

#ifndef NULL
#define	NULL		0	/* null pointer constant */
#endif

/* Values for the second argument to access.
   These may be OR'd together.  */
#define	R_OK            4	/* Test for read permission.  */
#define	W_OK            2	/* Test for write permission.  */
#define	X_OK            1	/* Test for execute permission.  */
#define	F_OK            0	/* Test for existence.  */

void	_exit (int);
int	access();
unsigned int alarm();
pid_t	fork();
gid_t	getegid();
uid_t	geteuid();
gid_t	getgid();
char	*getlogin();
pid_t	getpgrp();
pid_t	getpid();
pid_t	getppid();
uid_t	getuid();
off_t	lseek();
ssize_t	read();
unsigned int	sleep();
char	*ttyname();
ssize_t	write (int fd, const void *buf, size_t count);
int     truncate (const char *path, off_t length);
int     ftruncate (int fd, off_t length);

void	*brk (const void *addr);
int	_brk (const void *addr);
char	*crypt();
void	endusershell();
long	gethostid();
char	*getpass();
char	*getusershell();
char	*getwd();
void	psignal();
extern	char 	*sys_siglist[];
char	*re_comp();
void	*sbrk (int incr);
int	sethostid();
void	setusershell();
void	sync();
unsigned int	ualarm();
void	usleep();
int     pause (void);
pid_t	vfork();

int     pipe (int pipefd[2]);
int     close (int fd);
int     dup (int oldfd);
int     dup2 (int oldfd, int newfd);
int     unlink (const char *pathname);
int     link (const char *oldpath, const char *newpath);
ssize_t readlink (const char *path, char *buf, size_t bufsiz);
int     chown (const char *path, uid_t owner, gid_t group);
int     nice (int inc);
int     setuid (uid_t uid);
int     setgid (gid_t gid);
int     seteuid (uid_t euid);
int     setegid (gid_t egid);
int     setreuid (uid_t ruid, uid_t euid);
int     setregid (gid_t rgid, gid_t egid);
int     isatty (int fd);
int     chdir (const char *path);
int     fchdir (int fd);
int     chflags (const char *path, u_long flags);
int     fchflags (int fd, u_long flags);
int     getgroups (int size, gid_t list[]);
int     getdtablesize (void);
int     rmdir (const char *pathname);

struct stat;
int     stat (const char *path, struct stat *buf);
int     fstat (int fd, struct stat *buf);
int     lstat (const char *path, struct stat *buf);

int	execl (const char *path, const char *arg0, ... /* NULL */);
int	execle (const char *path, const char *arg0, ... /* NULL, char *envp[] */);
int	execlp (const char *file, const char *arg0, ... /* NULL */);

int	execv (const char *path, char *const argv[]);
int	execve (const char *path, char *const arg0[], char *const envp[]);
int	execvp (const char *file, char *const argv[]);

extern	char	**environ;		/* Environment, from crt0. */
extern	const char *__progname;		/* Program name, from crt0. */

int	getopt (int argc, char * const argv[], const char *optstring);

extern	char	*optarg;		/* getopt(3) external variables */
extern	int	opterr, optind, optopt;

#ifndef _VA_LIST_
#define va_list		__builtin_va_list	/* For GCC */
#endif

void	err (int eval, const char *fmt, ...);
void	errx (int eval, const char *fmt, ...);
void	warn (const char *fmt, ...);
void	warnx (const char *fmt, ...);
void	verr (int eval, const char *fmt, va_list ap);
void	verrx (int eval, const char *fmt, va_list ap);
void	vwarn (const char *fmt, va_list ap);
void	vwarnx (const char *fmt, va_list ap);

#ifndef _VA_LIST_
#undef va_list
#endif
#endif /* !_UNISTD_H_ */
