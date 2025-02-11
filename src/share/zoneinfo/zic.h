#ifndef BUFSIZ
#define BUFSIZ 1024
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

extern char *icpyalloc(char *string);
extern char *imalloc(int);
extern char *irealloc(char *pointer, int size);
extern char *scheck(char *string, char *format);

extern char **environ;
