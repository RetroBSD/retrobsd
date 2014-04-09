/*
 * Udp.cpp: Library to send/receive UDP packets with the Arduino ethernet shield.
 * This version only offers minimal wrapping of socket.c/socket.h
 *
 * NOTE: UDP is fast, but has some important limitations (thanks to Warren Gray for mentioning these)
 * 1) UDP does not guarantee the order in which assembled UDP packets are received. This
 * might not happen often in practice, but in larger network topologies, a UDP
 * packet can be received out of sequence.
 * 2) UDP does not guard against lost packets - so packets *can* disappear without the sender being
 * aware of it. Again, this may not be a concern in practice on small local networks.
 * For more information, see http://www.cafeaulait.org/course/week12/35.html
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
#ifndef udp_h
#define udp_h

#define UDP_TX_PACKET_MAX_SIZE 24

struct _udp_t {
    unsigned port;      // local port to listen on
    unsigned sock;      // socket ID for Wiz5100
};
typedef struct _udp_t udp_t;

/*
 * Initialize, start listening on specified port.
 * Returns 1 if successful, 0 if there are no sockets available to use.
 */
int udp_init (udp_t *u, unsigned port);

/*
 * Has data been received?
 */
unsigned udp_available (udp_t *u);

/*
 * Finish with the UDP socket.
 */
void udp_stop (udp_t *u);

/*
 * Send a packet to specified peer.
 */
unsigned udp_send_packet (udp_t *u, const uint8_t *data, unsigned len,
                          uint8_t *ip, unsigned port);

/*
 * Send a zero-terminated string to specified peer.
 */
unsigned udp_send_string (udp_t *u, const char *data,
                          uint8_t *ip, unsigned port);

/*
 * Read a received packet, also return sender's ip and port.
 */
int udp_read_packet (udp_t *u, uint8_t *buf, unsigned len, uint8_t *ip, unsigned *port);

#endif
