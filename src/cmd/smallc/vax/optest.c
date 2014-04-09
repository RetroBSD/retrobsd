#include <stdio.h>
#define chkstk  1
#define NOSUP   1
int address;
int ret;
int locaddr;
int i;
int *temp;
#ifdef  vax
#define INTSIZE 4
#else
#define INTSIZE 2
#endif
int     fred[30];
main(){
        int x;
        puts("Starting test");
        i = 1;
        address = &x;
        locaddr = 0;
        address = address + INTSIZE;
        temp = address;
        ret = *temp;
        fred[3] = 3;
        test(fred[3], 3, "fred[3] = 3");
        test(INTSIZE, sizeof(int), "INTSIZE");
        test(sizeof(char), 1, "sizeof char");
        test(1 + 4, 1,  "(should fail) 1+4 <tel:+4>");
        test(1022 + 5, 1027, "1022 + 5");
        test(4 + 5, 9, "4 + 5");
        test(1022 * 3, 3066, "1022 * 3");
        test(4 * - 1, -4, "4 * - 1");
        test(4 * 5, 20, "4 * 5");
        test(1000 - 999, 1, "1000 - 999");
        test(1000 - 1200, -200, "1000 - 1200");
        test(-1 - -1, 0, "-1 - -1");
        test(4 >> 2, 1, "4 >> 2");
        test(1234 >> 1, 617, "1234 >> 1");
        test(4 << 2, 16, "4 << 2");
        test(1000 << 1, 2000, "1000 << 1");
        test(1001 % 10, 1, "1001 % 10");
        test(3 % 10, 3, "3 % 10");
        test(10 % 4, 2, "10 % 4");
        test(1000 / 5, 200, "1000 / 5");
        test(3 / 10, 0, "3 / 10");
        test(10 / 3, 3, "10 / 3");
        test(1000 == 32767, 0, "1000 == 32767");
        test(1000 == 1000, 1, "1000 == 1000");
        test(1 != 0, 1, "1 != 0");
        test(1 < -1, 0, "1 < -1");
        test(1 < 2, 1, "1 < 2");
        test(1 != 1, 0, "1 != 1");
        test(2 && 1, 1, "2 && 1");
        test(0 && 1, 0, "0 && 1");
        test(1 && 0, 0, "1 && 0");
        test(0 && 0, 0, "0 && 0");
        test(1000 || 1, 1, "1000 || 1");
        test(1000 || 0, 1, "1000 || 0");
        test(0 || 1, 1, "0 || 1");
        test(0 || 0, 0, "0 || 0");
        test(!2, 0, "!2");
        test(!0, 1, "!0");
        test(~1, -2, "~1");
        test(2 ^ 1, 3, "2 ^ 1");
        test(0 ^ 0, 0, "0 ^ 0");
        test(1 ^ 1, 0, "1 ^ 1");
        test(5 ^ 6, 3, "5 ^ 6");
        test((0 < 1) ? 1 : 0, 1, "(0 < 1) ? 1 : 0");
        test((1000 > 1000) ? 0: 1, 1, "(1000 > 1000) ? 0 : 1");
        puts("ending test");
        }
test(t, real, testn) int t; char *testn; int real;{
        if (t != real) {
                fputs(testn, stdout);
                fputs(" failed\n", stdout);
                fputs("Should be: ", stdout);
                printn(real, 10, stdout);
                fputs(" was: ", stdout);
                printn(t, 10, stdout);
                putchar('\n');
                prompt();
                }
        if (*temp != ret) {
                puts("retst");
                prompt();
        }
#ifdef  chkstk
        if (locaddr == 0) locaddr = &t;
        else if (locaddr != &t) {
                puts("locst during");
                puts(testn);
                prompt();
        }
#endif

}

prompt() {
        puts("hit any key to continue");
        getchar();

}

