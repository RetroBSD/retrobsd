#include <stdio.h>
#include <stdarg.h>

void print(char *fmt, ...)
{
    va_list ap;
    int d;
    char c, *s;

    va_start(ap, fmt);
    while (*fmt) {
        switch (*fmt++) {
        case 's':              /* string */
            s = va_arg(ap, char*);
            printf("string %s\n", s);
            break;
        case 'd':              /* int */
            d = va_arg(ap, int);
            printf("int %d\n", d);
            break;
        case 'c':              /* char */
            c = va_arg(ap, int);
            printf("char %c\n", c);
            break;
        }
    }
    va_end(ap);
}

int main()
{
    print("sdcsdc", "abracadabra", 12345, 'Z', "foo", 365, '%');
    return 0;
}
