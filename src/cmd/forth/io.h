#if 0
/*
 * Use standard I/O library.
 */
#include <stdio.h>
#else
/*
 * Use internal I/O library.
 */
#include <stdarg.h>
#include <fcntl.h>

typedef int FILE;

#define stdin   ((FILE*) 0)
#define stdout  ((FILE*) 1)
#define stderr  ((FILE*) 2)

static inline int fileno (FILE *stream)
{
    return (int) stream;
}

static inline FILE *fopen (const char *path, const char *mode)
{
    int fd, flags;

    if (mode[0] == 'w')
        flags = O_WRONLY;
    else if (mode[0] == 'a')
        flags = O_WRONLY | O_APPEND | O_CREAT;
    else if (mode[0] == 'r' && mode[1] == '+')
        flags = O_RDWR | O_CREAT;
    else
        flags = O_RDONLY;

    fd = open (path, flags, 0664);
    if (fd < 0)
        return 0;
    return (FILE*) fd;
}

static inline FILE *fdopen (int fildes, const char *mode)
{
    if (fildes < 0)
        return 0;
    return (FILE*) fildes;
}

static inline int fclose (FILE *stream)
{
    return close ((int) stream);
}

extern int vsprintf (char *s, const char *format, va_list ap);

static inline int vfprintf (FILE *stream, const char *format, va_list ap)
{
    int len;
    char str[160];

    vsprintf (str, format, ap);
    len = strlen (str);
    if (len > 0) {
        write ((int) stream, str, len);
    }
    return len;
}

static inline int vprintf (const char *format, va_list ap)
{
    return vfprintf (stdout, format, ap);
}

static inline int fprintf (FILE *stream, const char *format, ...)
{
    va_list ap;
    int ret;

    va_start (ap, format);
    ret = vfprintf (stream, format, ap);
    va_end (ap);
    return ret;
}

static inline int printf (const char *format, ...)
{
    va_list ap;
    int ret;

    va_start (ap, format);
    ret = vfprintf (stdout, format, ap);
    va_end (ap);
    return ret;
}

static inline int putc (int c, FILE *stream)
{
    unsigned char sym = c;

    if (write ((int) stream, &sym, 1) < 0)
        return -1;
    return c;
}

static inline int getc (FILE *stream)
{
    unsigned char sym;

    if (read ((int) stream, &sym, 1) != 1)
        return -1;
    return sym;
}
#endif
