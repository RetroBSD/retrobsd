/*
 * Compute the sum of prime numbers up to 10000.
 */
main()
{
    int sum, n;

    sum = 0;
    for (n=2; n<10000; ++n) {
        if (isprime(n)) {
            sum += n;
        }
    }
    printf("Sum of primes less than 10000: %d\n", sum);
}

isprime(n)
    int n;
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
