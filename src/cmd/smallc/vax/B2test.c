#include <stdio.h>

main () {
        FILE *infile; FILE *outfile;
        int c;
        puts("Starting input only");
        if ((infile = fopen("b2test.dat","r")) == NULL ) {
                puts("could not open input file");
                exit(1);
        }
        while (putchar(fgetc(infile)) != EOF);
        puts("end of input file");
        fclose(infile);
        puts("starting copy");
        if ((infile = fopen("b2test.dat","r")) == NULL) {
                puts("could not open input file for copy");
                exit(1);
        }
        if ((outfile = fopen("b2test.out","w")) == NULL) {
                puts("could not open output file");
                exit(1);
        }
        while ((c = fgetc(infile)) != EOF) {
                fputc(c, outfile);
        }
        puts("finished output file");
        fclose(infile);
        fclose(outfile);

}

