/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <sys/types.h>

#ifndef	NULL
#define	NULL	0
#endif

char	*strcat (char *, const char *);
char	*strncat (char *, const char *, size_t);
char	*strcpy (char *, const char *);
char	*strncpy (char *, const char *, size_t);

char	*strstr (const char *, const char *);

int	strcmp (const char *, const char *);
int	strncmp (const char *, const char *, size_t);
size_t	strlen (const char *);

int	memcmp (const void *, const void *, size_t);

void    *memmove (void *, const void *, size_t);
void	*memccpy (void *, const void *, int, size_t);
void	*memchr (const void *, int, size_t);
void	*memcpy (void *, const void *, size_t);
void	*memset (void *, int, size_t);
char	*strchr (const char *, int);

char	*strdup (const char *);
char	*strpbrk (const char *, const char *);
char	*strrchr (const char *, int);
char	*strsep (char **, const char *);
char	*strtok (char *, const char *);
char	*strtok_r (char *, const char *, char **);

size_t	strcspn (const char *, const char *);
size_t	strspn (const char *, const char *);

char *strerror (int);
const char *syserrlst (int);
