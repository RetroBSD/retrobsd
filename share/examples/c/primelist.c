/*
 * Print the list of prime numbers up to 100.
 */
#include <stdio.h>

int isprime(int);

int main(void)
{
    int n;

    for (n=2; n<100; ++n) {
        if (isprime(n)) {
            printf("%d ", n);
        }
    }
    printf("\n");
}

int isprime(int n)
{
    int j;

    if (n == 2)
        return 1;

    if (n % 2 == 0)
        return 0;

    for (j=3; j*j<=n; j+=2)
        if (n % j == 0)
            return 0;
    return 1;
}

