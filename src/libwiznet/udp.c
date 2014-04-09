/*
 * Udp.cpp: Library to send/receive UDP packets with the Arduino ethernet shield.
 * This version only offers minimal wrapping of socket.c/socket.h
 *
 * MIT License:
 * Copyright (c) 2008 Bjoern Hartmann
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * bjoern@cs.stanford.edu 12/30/2008
 */
#include <string.h>
#include <wiznet/socket.h>
#include <wiznet/ethernet.h>
#include <wiznet/udp.h>

/*
 * Start UDP socket, listening at local port PORT
 */
int udp_init (udp_t *u, unsigned port)
{
    int i;

    for (i = 0; i < MAX_SOCK_NUM; i++) {
        uint8_t s = w5100_readSnSR(i);
        if (s == SnSR_CLOSED || s == SnSR_FIN_WAIT) {
            u->sock = i;
            break;
        }
    }

    if (u->sock == MAX_SOCK_NUM)
        return 0;

    u->port = port;
    socket_init (u->sock, SnMR_UDP, u->port, 0);
    return 1;
}

/*
 * Is data available in rx buffer?
 * Returns 0 if no, number of available bytes if yes.
 * returned value includes 8 byte UDP header!
 */
unsigned udp_available (udp_t *u)
{
    return w5100_getRXReceivedSize (u->sock);
}

/*
 * Release any resources being used by this UDP instance.
 */
void udp_stop (udp_t *u)
{
    if (u->sock == MAX_SOCK_NUM)
        return;

    socket_close (u->sock);

    _socket_port[u->sock] = 0;
    u->sock = MAX_SOCK_NUM;
}

/*
 * Send packet contained in buf of length len to peer at specified ip, and port.
 * Use this function to transmit binary data that might contain 0x00 bytes.
 * This function returns sent data size for success else -1.
 */
unsigned udp_send_packet (udp_t *u, const uint8_t *buf, unsigned len,
                          uint8_t *ip, unsigned port)
{
    return socket_sendto (u->sock, buf, len, ip, port);
}

/*
 * Send zero-terminated string str as packet to peer at specified ip, and port.
 * This function returns sent data size for success else -1.
 */
unsigned udp_send_string (udp_t *u, const char *str,
                          uint8_t *ip, unsigned port)
{
    unsigned len = strlen (str);

    return socket_sendto (u->sock, (const uint8_t *) str, len, ip, port);
}

/*
 * Read a received packet into buffer buf (which is of maximum length len);
 * store calling ip and port as well. Call available() to make sure data is
 * ready first.
 * NOTE: I don't believe len is ever checked in implementation of recvfrom(),
 *       so it's easy to overflow buffer. so we check and truncate.
 * Returns number of bytes read, or negative number of bytes we would have
 * needed if we truncated.
 */
int udp_read_packet (udp_t *u, uint8_t *buf, unsigned len,
                     uint8_t *ip, unsigned *port)
{
    int nbytes = udp_available (u) - 8;     /* skip UDP header */
    if (nbytes < 0) {
        /* No real data here. */
        return 0;
    }

    if (nbytes > (int)len) {
        /* Packet is too large - truncate.
         * HACK: hand-parse the UDP packet using TCP recv method. */
        uint8_t tmpBuf[8];
        int i;

        /* Read 8 header bytes and get IP and port from it. */
        socket_recv (u->sock, tmpBuf, 8);
        if (ip != 0) {
            ip[0] = tmpBuf[0];
            ip[1] = tmpBuf[1];
            ip[2] = tmpBuf[2];
            ip[3] = tmpBuf[3];
        }
        if (port != 0)
            *port = (tmpBuf[4] << 8) + tmpBuf[5];

        /* Now copy first (len) bytes into buf. */
        for (i=0; i<(int)len; i++) {
            socket_recv (u->sock, tmpBuf, 1);
            buf[i] = tmpBuf[0];
        }

        /* And just read the rest byte by byte and throw it away. */
        while (udp_available (u)) {
            socket_recv (u->sock, tmpBuf, 1);
        }
        return -nbytes;
    }
    return socket_recvfrom (u->sock, buf, len, ip, port);
}
