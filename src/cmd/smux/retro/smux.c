#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "../common/packet.h"

#define MAXFD 20
#define MAXPTY 10

struct smux_packet rx;
struct smux_packet tx;

#define termios sgttyb
static struct termios term_orig, term_smux;

void send_tx()
{
    write(0, &tx, tx.len + 3);
    fflush(stdout);
}

int allocate_pty()
{
    int i;
    int fd = -1;
    char dev[20];
    struct stat sb;

    for (i = 0; i < MAXPTY; i++) {
        sprintf(dev, "/dev/ptyp%d", i);
        if (stat(dev, &sb) == -1) {
            return -1;
        }
        fd = open(dev, O_RDWR | O_EXCL);
        if (fd > 0) {
            return fd;
        }
    }
    return -1;
}

void close_pty(int fd)
{
    close(fd);
}

void raw()
{
    ioctl(0, TIOCGETP, &term_orig);
    term_smux = term_orig;

    term_smux.sg_flags &= ~(ECHO | CRMOD | XTABS | RAW);
    term_smux.sg_flags |= CBREAK | RAW;

    ioctl(0, TIOCSETP, &term_smux);

    //signal(SIGINT, SIG_IGN);
}

void cooked()
{
    ioctl(0, TIOCSETP, &term_orig);
}

int rx_link_data()
{
    static unsigned char phase = 0;
    static unsigned int pos = 0;
    char inch[2];
    int res;

    res = read(0, inch, 1);

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

int main(int argc, char *argv[])
{
    // Storage for all the FDs for the various streams
    int fds[MAXFD];
    fd_set rfd;
    unsigned int nextping = time(NULL) + 10;
    fds[0] = 0;
    int i;
    struct timeval tv;
    int fd;
    int maxfd;

    tv.tv_sec = 1;
    tv.tv_usec = 0;

    for (i=0; i<MAXFD; i++)
        fds[i] = -1;

    fds[0] = 0;

    raw();

    // Main loop.  Do this forever.
    for (;;) {

        maxfd = 0;
        FD_ZERO(&rfd);
        for (i=0; i<MAXFD; i++) {
            if (fds[i] != -1) {
                FD_SET(fds[i], &rfd);
                if (fds[i] > maxfd)
                    maxfd = fds[i];
            }
        }
        select(maxfd+1, &rfd, NULL, NULL, &tv);

        for (i = 0; i < MAXFD; i++) {
            if (fds[i] != -1) {
                if (FD_ISSET(fds[i], &rfd)) {

                    if (fds[i] == 0) {
                        if (rx_link_data()) {
                            // We have a complete packet

                            // Any packet counts as a ping - reset the
                            // ping time.
                            nextping = time(NULL) + 10;

                            switch (rx.type) {
                            case C_PING:
                                // This was a ping - we should respond to confirm
                                // we got it.
                                tx.type = C_PONG;
                                tx.stream = 0;
                                tx.len = 0;
                                send_tx();
                                break;

                            case C_CONNECT:
                                // We want to create a new channel.  First off, get a ptyp
                                // device.

                                fd = allocate_pty();
                                if (fd < 0) {
                                    tx.type = C_CONNECTFAIL;
                                    tx.stream = 0;
                                    tx.len = 0;
                                    send_tx();
                                    break;
                                }
                                i = rx.stream;
                                if (fds[i] == -1) {
                                    fds[i] = fd;
                                    tx.type = C_CONNECTOK;
                                    tx.stream = i;
                                    tx.len = 0;
                                    send_tx();
                                    break;
                                }
                                if (i == MAXFD) {
                                    close_pty(fd);
                                    tx.type = C_CONNECTFAIL;
                                    tx.stream = rx.stream;
                                    tx.len = 0;
                                    send_tx();
                                }
                                break;

                            case C_DISCONNECT:
                                close_pty(fds[rx.stream]);
                                fds[rx.stream] = -1;
                                break;

                            case C_DATA:
                                if (fds[rx.stream] > -1) {
                                    write(fds[rx.stream], rx.data, rx.len);
                                }
                                break;

                            case C_QUIT:
                                cooked();
                                return(0);

                            }
                        }
                    } else {
                        tx.type = C_DATA;
                        tx.stream = i;
                        tx.len = read(fds[i], tx.data, 255);
                        send_tx();
                    }
                }
            }
        }
        if (time(NULL) > nextping) {
            cooked();
            exit(10);
        }
    }
    return 0;
}
