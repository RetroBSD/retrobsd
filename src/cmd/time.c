/*
 * time
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>

void printt(char *s, struct timeval *tv)
{
    fprintf(stderr, "%9ld.%01ld %s ", tv->tv_sec, tv->tv_usec / 100000, s);
}

int main(int argc, char **argv)
{
    int status;
    int p;
    struct timeval before, after;
    struct rusage ru;

    if (argc <= 1)
        exit(0);
    gettimeofday(&before, 0);
    p = fork();
    if (p < 0) {
        perror("time");
        exit(1);
    }
    if (p == 0) {
        execvp(argv[1], &argv[1]);
        perror(argv[1]);
        exit(1);
    }
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    while (wait3(&status, 0, &ru) != p)
        ;
    gettimeofday(&after, 0);
    if ((status & 0377) != 0)
        fprintf(stderr, "Command terminated abnormally.\n");
    after.tv_sec -= before.tv_sec;
    after.tv_usec -= before.tv_usec;
    if (after.tv_usec < 0)
        after.tv_sec--, after.tv_usec += 1000000;
    printt("real", &after);
    printt("user", &ru.ru_utime);
    printt("sys ", &ru.ru_stime);
    fprintf(stderr, "\n");
    exit(status >> 8);
}
