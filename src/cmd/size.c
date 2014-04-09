/*
 * size
 */
#ifdef CROSS
#   include </usr/include/stdio.h>
#else
#   include <stdio.h>
#endif
#include <stdlib.h>
#include <unistd.h>
#include <a.out.h>

int header;

int
main(argc, argv)
char **argv;
{
    struct exec buf;
    long sum;
    int nfiles, ch, err = 0;
    FILE *f;

    while ((ch = getopt(argc, argv, "h")) != EOF) {
        switch(ch) {
        case 'h':
        default:
            fprintf(stderr, "Usage:\n");
            fprintf(stderr, "  size file...\n");
            return(1);
        }
    }
    argc -= optind;
    argv += optind;

    if (argc == 0) {
        *argv = "a.out";
        argc++;
    }

    for (nfiles=argc; argc--; argv++) {
        if ((f = fopen(*argv, "r"))==NULL) {
            printf("size: %s not found\n", *argv);
            err++;
            continue;
        }
        if (fread((char *)&buf, sizeof(buf), 1, f) != 1 ||
            N_BADMAG(buf)) {
            printf("size: %s not an object file\n", *argv);
            fclose(f);
            err++;
            continue;
        }
        if (header == 0) {
            printf("text\tdata\tbss\tdec\thex\n");
            header = 1;
        }
        printf("%u\t%u\t%u\t", buf.a_text,buf.a_data,buf.a_bss);
        sum = (long) buf.a_text + (long) buf.a_data + (long) buf.a_bss;
        printf("%ld\t%lx", sum, sum);
        if (nfiles > 1)
            printf("\t%s", *argv);
        printf("\n");
        fclose(f);
    }
    exit(err);
}
