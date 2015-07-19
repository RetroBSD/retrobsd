#include <stdlib.h>
#include <unistd.h>

struct atexit {                 /* entry allocated per atexit() call */
    struct atexit *next;        /* next enty in a list */
    void (*func)(void);         /* callback function */
};

int errno;
struct atexit *__atexit;        /* points to head of LIFO stack */

extern void _cleanup();

void
exit (code)
    int code;
{
    register struct atexit *p;

    for (p = __atexit; p; p = p->next)
        (*p->func)();
    _cleanup();
    _exit (code);
}

/*
 * Register a function to be performed at exit.
 */
int
atexit(fn)
    void (*fn)();
{
    static struct atexit __atexit0; /* one guaranteed table */
    register struct atexit *p;

    p = __atexit;
    if (! p) {
        p = &__atexit0;
    } else {
        p = malloc(sizeof(struct atexit));
        if (! p)
            return -1;
        p->next = __atexit;
    }
    p->func = fn;
    __atexit = p;
    return 0;
}
