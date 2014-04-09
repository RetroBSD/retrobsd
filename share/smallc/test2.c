#include <fcntl.h>

int aaa;
int bbb;
int ccc;
char gc;
char gbuffer[3];
int gibuffer[4];

extern errno;

main()
{
    char b;
    int la;
    unsigned int u1, u2;
    int s1, s2;
    unsigned char uc1, uc2;
    char sc1, sc2;
    int fd;
    char buffer[6];
    int ibuffer[7];

    printf("                sizeof(uc1): %d 1\n",  sizeof(uc1));
    printf("                sizeof(sc1): %d 1\n",  sizeof(sc1));
    printf("                 sizeof(u1): %d 4\n",  sizeof(u1));
    printf("                 sizeof(s1): %d 4\n",  sizeof(s1));
    printf("                sizeof(aaa): %d 4\n",  sizeof(aaa));
    printf("                sizeof(bbb): %d 4\n",  sizeof(bbb));
    printf("                 sizeof(gc): %d 1\n",  sizeof(gc));
    printf("             sizeof(buffer): %d 6\n",  sizeof(buffer));
    printf("            sizeof(ibuffer): %d 28\n", sizeof(ibuffer));
    printf("               sizeof(char): %d 1\n",  sizeof(char));
    printf("                sizeof(int): %d 4\n",  sizeof(int));
    printf("            sizeof(gbuffer): %d 3\n",  sizeof(gbuffer));
    printf("           sizeof(gibuffer): %d 16\n", sizeof(gibuffer));
    // sizeof(ibuffer[0]) is not supported, so the following can be used...
    printf("sizeof(ibuffer)/sizeof(int): %d 7\n",  sizeof(ibuffer)/sizeof(int));

    aaa = 1;
    bbb = 2;
    la = 4;
    printf("%d 1\n", aaa);
    printf("%d 2\n", bbb);
    printf("%d 4\n", la);

    uc1 = 0x80;
    sc1 = 0x80;
    s1 = uc1;
    s2 = sc1;
    printf("unsigned char (0x80) -> int: %d 128\n",  s1);
    printf("  signed char (0x80) -> int: %d -128\n", s2);

    u1 = uc1;
    u2 = sc1;
    printf("unsigned char (0x80) -> unsigned: %d 128\n", u1);
    printf("  signed char (0x80) -> unsigned: %d -128\n", u2);

    la = errno;
    printf("errno: %d 0\n", la);

    write(1, "abcd ", 5);
    la = errno;
    printf("errno after good write call: %d 0\n", la);

    write(10, "abcde", 5);
    la = errno;
    printf("errno after bad write call: %d 9\n", la);

    write(1, "abcd ", 5);
    la = errno;
    printf("good write after failed should not overwrite errno: %d 9\n", la);

    errno = 0;
    write(1, "abcd ", 5);
    la = errno;
    printf("good write after errno set to zero: %d 0\n", la);

    la = write(1, "abcd ", 5);
    printf("write() return: %d 5\n", la);

    la = write(10, "abcd ", 5);
    printf("write(bad fd) return: %d -1\n", la);

    fd = open("/a.txt", O_WRONLY | O_CREAT, 0666);
    if (fd != -1) {
        printf("open success\n");
        la = write(fd, "abcd\n", 5);
        if (la == 5) printf("write success\n"); else  printf("write failed\n");
        la = close(fd);
        if (la != -1) printf("close success\n"); else printf("close failed\n");
    } else {
        printf("open failed\n");
    }

    buffer[0] = 0;
    buffer[1] = 0;
    buffer[2] = 0;
    buffer[3] = 0;
    buffer[4] = 0;
    buffer[5] = 0;

    fd = open("/a.txt", O_RDONLY, 0666);
    if (fd != -1) {
        printf("open success\n");
        la = read(fd, buffer, 5);
        printf(buffer);
        if (la == 5) printf("read success\n"); else  printf("read failed\n");
        la = close(fd);
        if (la != -1) printf("close success\n"); else printf("close failed\n");
    } else {
        printf("open failed\n");
    }

    if (buffer[0] != 'a')  printf("data0 readback from file MISMATCH\n");
    if (buffer[1] != 'b')  printf("data1 readback from file MISMATCH\n");
    if (buffer[2] != 'c')  printf("data2 readback from file MISMATCH\n");
    if (buffer[3] != 'd')  printf("data3 readback from file MISMATCH\n");
    if (buffer[4] != '\n') printf("data4 readback from file MISMATCH\n");

    if (buffer[0] != 'a' || buffer[1] != 'b' || buffer[2] != 'c' ||
        buffer[3] != 'd' || buffer[4] != '\n') {
        printf("data readback from file MISMATCH\n");
    } else {
        printf("data readback from file OK\n");
    }
}
