#include <stdio.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/termios.h>
#include <strings.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#define TELOPTS
#define TELCMDS

#include <arpa/telnet.h>
#include <signal.h>

#include "../common/packet.h"

struct smux_packet rx;
struct smux_packet tx;

int pipe_fd;

int string_contains(char *haystack, char *needle, int len)
{
    char *p;
    for (p = haystack; p < (haystack + len) - strlen(needle); p++) {
        if(!strncmp(p, needle, strlen(needle))) {
            return 1;
        }
    }
    return 0;
}

void send_tx()
{
    int rv __attribute__((unused));
    rv = write(pipe_fd, &tx, tx.len + 3);
    fsync(pipe_fd);
}

int fdgetline(int fd, char *buffer, int maxlen)
{
    fd_set rfd;
    struct timeval tv;
    int nr = 0;
    int rc = 0;
    char inch[2];
    int rv __attribute__((unused));

    for (;;) {
        tv.tv_sec = 0;
        tv.tv_usec = 1000;
        FD_ZERO(&rfd);
        FD_SET(fd, &rfd);
        rc = select(fd+1, &rfd, NULL, NULL, &tv);
        if (rc <= 0) {
            return nr;
        }

        if (FD_ISSET(fd, &rfd)) {
            rv = read(fd, inch, 1);
            if (inch[0] == '\n') {
                return nr;
            }
            if (inch[0] != '\r') {
                buffer[nr++] = inch[0];
                buffer[nr] = 0;
            }
            if (nr == maxlen) {
                return maxlen;
            }
        }
    }
}

int read_chunk(int fd, void *buf, int ml)
{
    fd_set rfd;
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    int rv __attribute__((unused));

    FD_ZERO(&rfd);
    FD_SET(fd, &rfd);

    rv = select(9999, &rfd, NULL, NULL, &tv);

    if (FD_ISSET(fd, &rfd)) {
        return read(fd, buf, ml);
    }
    return -1;
}

int rx_link_data()
{
    static unsigned char phase = 0;
    static unsigned int pos = 0;
    char inch[2];
    int res;

    res = read(pipe_fd, inch, 1);

    if (res<0)
        return 0;

    switch(phase) {
    case 0:
        if (inch[0] > C_MAX)
            return 0;
        rx.type = inch[0];
        phase++;
        break;
    case 1:
        rx.stream = inch[0];
        phase++;
        break;
    case 2:
        rx.len = inch[0];
        phase++;
        pos = 0;
        if (rx.len == 0) {
            phase = 0;
            return 1;
        }
        break;
    case 3:
        rx.data[pos++] = inch[0];
        if (pos == rx.len) {
            phase = 0;
            return 1;
        }
        break;
    }

    return 0;
}


#define MAXFD 10

#define M_NORM 0
#define M_IAC 1
#define M_SUB 2

int main(int argc, char *argv[])
{
    char *username = "smux";
    char *password = "";
    char *device = "/dev/ttyACM0";
    int nr;
    char buffer[300];
    struct termios tty;
    int i;
    int maxfd;
    fd_set rfd;
    fd_set efd;
    struct timeval tv;
    unsigned int doping = time(NULL) + 5;
    unsigned int nextping = time(NULL) + 10;
    int sockfd;
    struct sockaddr_in sa;
    int infd;
    int stream;
    int rv __attribute__((unused));

    int ready[MAXFD];
    int fds[MAXFD];

    int opt;

    signal(SIGPIPE, SIG_IGN);


    for (i=0; i<MAXFD; i++) {
        fds[i] = -1;
        ready[i] = 0;
    }

    while ((opt = getopt(argc, argv, "hd:u:p:")) != -1) {
        switch (opt) {
        case 'd':
            device = optarg;
            break;
        case 'u':
            username = optarg;
            break;
        case 'p':
            password = optarg;
            break;
        case 'h':
            printf("Usage: %s [-d device] [-u username] [-p password]\n", argv[0]);
            return(0);
        default:
            printf("Unknown option: %c\n", opt);
            return(10);
        }
    }

    pipe_fd = open(device, O_RDWR | O_EXCL);
    if (pipe_fd < 0) {
        printf("Unable to open %s\n", device);
        return 10;
    }

    tcgetattr(pipe_fd, &tty);
    cfsetospeed(&tty, B115200);
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag |= CS8 | CLOCAL;
    tcsetattr(pipe_fd, TCSANOW, &tty);
    tcflush(pipe_fd, TCOFLUSH);

    fds[0] = pipe_fd;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (!sockfd) {
        printf("Unable to open socket\n");
        return(10);
    }


    bzero(&sa, sizeof(sa));
    sa.sin_family = AF_INET;
    sa.sin_port = htons(3333);

    i = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(int));

    if (bind(sockfd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        printf("Unable to bind socket\n");
        return(10);
    }

    listen(sockfd, 10);

    fds[1] = sockfd;

    rv = write(pipe_fd, "\r\r\r", 1);
    bzero(buffer,256);
    for (;;) {
        if ((nr = fdgetline(pipe_fd, buffer, 255)) > 0) {
            printf("%s\n", buffer);
            if(string_contains(buffer, "ogin:", nr)) {
                sprintf(buffer, "%s\r", username);
                rv = write(pipe_fd, buffer, strlen(buffer));
            }
            if(string_contains(buffer, "assword:", nr)) {
                sprintf(buffer, "%s\r", password);
                rv = write(pipe_fd, buffer, strlen(buffer));
            }
            if(string_contains(buffer, "Welcome to RetroBSD", nr)) {
                break;
            }
        }
    }

    printf("Logged in - starting main loop\n");

    doping = time(NULL) + 5;
    nextping = time(NULL) + 10;

    // Main loop.  Do this forever.
    for (;;) {


        maxfd = 0;
        FD_ZERO(&rfd);
        for (i=0; i<MAXFD; i++) {
            if (fds[i] != -1) {
                FD_SET(fds[i], &rfd);
                FD_SET(fds[i], &efd);
                if (fds[i] > maxfd)
                    maxfd = fds[i];
            }
        }
        tv.tv_sec = 0;
        tv.tv_usec = 1000;
        select(maxfd+1, &rfd, NULL, &efd, &tv);



        for (stream = 0; stream < MAXFD; stream++) {
            if (ready[stream] == 1) {
                tx.type = C_CONNECT;
                tx.stream = stream;
                tx.len = 0;
                send_tx();
                ready[stream] = 0;
		printf("Attempting to connect stream %d\n", stream);
            }
            if (fds[stream] != -1) {

                if (FD_ISSET(fds[stream], &efd)) {
                    if (fds[stream] == sockfd) {
                        printf("Socket died\n");
                        return 10;
                    }
                    if (fds[stream] == pipe_fd) {
                        printf("Pipe died\n");
                        return 10;
                    }
                    close(fds[stream]);
                    FD_CLR(fds[stream], &rfd);
                    printf("Connection closed on stream %d\n", i);
                    fds[stream] = -1;
                }

                if (FD_ISSET(fds[stream], &rfd)) {
                    if (fds[stream] == sockfd) {
                        if ((infd = accept(sockfd,NULL,NULL)) >= 0) {
                            for (i=0; i<MAXFD; i++) {
                                if (fds[i] == -1) {
                                    fds[i] = infd;
					printf("Found spare FD in stream %d\n", i);
                                    break;
                                }
                            }
                            if (i == MAXFD) {
                                printf("Out of FDs\n");
                                close(infd);
                            } else {
                                printf("New connection stream %d\n", i);
                                sprintf(buffer, "%c%c%c%c%c%c", IAC, DO, TELOPT_LINEMODE, IAC, WILL, TELOPT_ECHO);
                                //printf("> IAC DO LINEMODE IAC WILL ECHO\n");
                                rv = write(infd, buffer, 6);
                                fsync(infd);
                                ready[i] = 1;
                            }
                        }
                    } else if (fds[stream] == pipe_fd) {
                        if (rx_link_data()) {
                            // We have a complete packet
                            switch (rx.type) {
                            case C_PONG:
                                nextping = time(NULL) + 10;
                                break;
                            case C_DATA:
                                nextping = time(NULL) + 10;
                                rv = write(fds[rx.stream], rx.data, rx.len);
                                //printf("%d>%s", rx.stream, rx.data);
                                break;
                            case C_CONNECTOK:
                                printf("Connected OK, stream ID is %d\n", rx.stream);
                                tx.type = C_DATA;
                                tx.len = 1;
                                tx.stream = stream;
                                sprintf(tx.data, "\r");
                                send_tx();
                                break;
                            case C_CONNECTFAIL:
                                //printf("Connect failed on stream %d\n", rx.stream);
                                break;
                            }
                        }
                    } else {
                        tx.type = C_DATA;
                        tx.stream = stream;
                        nr = read(fds[stream], buffer, 255);

                        if (nr <= 0) {
                            tx.type = C_DISCONNECT;
                            tx.stream = stream;
                            tx.len = 0;
                            send_tx();
                            close(fds[stream]);
                            fds[stream] = -1;
                            printf("Connection %d closed\n", stream);
                        } else {
                            buffer[nr] = 0;
                            //printf("Got %d bytes\n", nr);
                            int j = 0;
                            char iac[100];
                            for (i=0; i<nr; i++) {
                                switch ((unsigned char)buffer[i]) {
                                case '\r':
                                    tx.data[j++] = buffer[i];
                                    i++;
                                    break;
                                case IAC:
                                    i++;
                                    switch ((unsigned char)buffer[i]) {
                                    case WILL:
                                        i++;
                                        sprintf(iac, "%c%c%c", IAC, DONT, buffer[i]);
                                        rv = write(fds[stream], iac, 3);
                                        fsync(fds[stream]);
                                        break;
                                    case WONT:
                                    case DO:
                                        i++;
                                        switch ((unsigned char)buffer[i]) {
                                        case TELOPT_SGA:
                                            sprintf(iac, "%c%c%c", IAC, WILL, TELOPT_SGA);
                                            rv = write(fds[stream], iac, 3);
                                            break;
                                        case TELOPT_ECHO:
                                            break;
                                        case TELOPT_LINEMODE:
                                            sprintf(iac, "%c%c%c%c%c%c%c", IAC, SB, 34, 1, 0, IAC, SE);
                                            rv = write(fds[stream], iac, 7);
                                            break;
                                        default:
                                            sprintf(iac, "%c%c%c", IAC, WONT, buffer[i]);
                                            rv = write(fds[stream], iac, 3);
                                            break;
                                        }
                                        break;
                                    case DONT:
                                        i++;
                                        break;
                                    case SB:
                                        i++;
                                        if (buffer[i] == 34) {
                                            sprintf(iac, "%c%c%c%c%c%c%c", IAC, SB, 34, 1, 4, IAC, SE);
                                            rv = write(fds[stream], iac, 7);
                                        }
                                        while ((buffer[i] != (char)SE) && (buffer[i-1] != (char)SE)) {
                                            i++;
                                        }
                                        i++;
                                        break;
                                    case SE:
                                        break;
                                    case IAC:
                                        tx.data[j++] = buffer[i];
                                        break;
                                    default:
                                        printf("Unhandled IAC: %d\n", (unsigned char)buffer[i]);
                                        i++;
                                    }
                                default:
                                    tx.data[j++] = buffer[i];
                                }
                            }
                            if (j > 0) {
                                tx.stream = stream;
                                tx.len = j;
                                tx.data[j] = 0;
                                send_tx();
                            }
                        }
                    }
                }
            }
        }

        if (doping < time(NULL)) {
            doping = time(NULL) + 5;
            tx.type = C_PING;
            tx.stream = 0;
            tx.len = 0;
            send_tx();
        }

        if (nextping < time(NULL)) {
            close(pipe_fd);
            printf("Timeout\n");
            return(10);
        }
    }

    close(pipe_fd);
}
