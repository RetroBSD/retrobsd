#include <stdio.h>
#include <unistd.h>
#include <wiznet/socket.h>

unsigned _socket_port [MAX_SOCK_NUM] = { 0 };

static unsigned local_port;

/*
 * This Socket function initialize the channel in perticular mode,
 * and set the port and wait for W5100 done it.
 * Return 1 for success else 0.
 */
unsigned socket_init (unsigned sock, unsigned protocol, unsigned port, unsigned flag)
{
    if ((protocol == SnMR_TCP) || (protocol == SnMR_UDP) ||
        (protocol == SnMR_IPRAW) || (protocol == SnMR_MACRAW) ||
        (protocol == SnMR_PPPOE))
    {
        socket_close (sock);
        w5100_writeSnMR (sock, protocol | flag);
        if (port != 0) {
            w5100_writeSnPORT (sock, port);
        } else {
            /* if don't set the source port, set local_port number. */
            local_port++;
            w5100_writeSnPORT (sock, local_port);
        }

        w5100_socket_cmd (sock, Sock_OPEN);
        return 1;
    }
    return 0;
}

/*
 * This function close the socket and parameter is "sock" which represent
 * the socket number.
 */
void socket_close (unsigned sock)
{
    w5100_socket_cmd (sock, Sock_CLOSE);
    w5100_writeSnIR (sock, 0xFF);
}

/*
 * This function established the connection for the channel in passive
 * (server) mode. This function waits for the request from the peer.
 * Return 1 for success else 0.
 */
unsigned socket_listen (unsigned sock)
{
    if (w5100_readSnSR (sock) != SnSR_INIT)
        return 0;
    w5100_socket_cmd (sock, Sock_LISTEN);
    return 1;
}

/*
 * This function established  the connection for the channel in Active
 * (client) mode.  This function waits for the untill the connection
 * is established.
 * Return 1 for success else 0.
 */
unsigned socket_connect (unsigned sock, uint8_t * addr, unsigned port)
{
    if (((addr[0] == 0xFF) && (addr[1] == 0xFF) &&
         (addr[2] == 0xFF) && (addr[3] == 0xFF)) ||
        ((addr[0] == 0x00) && (addr[1] == 0x00) &&
         (addr[2] == 0x00) && (addr[3] == 0x00)) ||
        (port == 0))
      return 0;

    /* set destination IP */
    w5100_writeSnDIPR (sock, addr);
    w5100_writeSnDPORT (sock, port);
    w5100_socket_cmd (sock, Sock_CONNECT);
    return 1;
}

/*
 * This function used for disconnect the socket and parameter is "sock"
 * which represent the socket number
 * Return 1 for success else 0.
 */
void socket_disconnect (unsigned sock)
{
    w5100_socket_cmd (sock, Sock_DISCON);
}

/*
 * This function used to send the data in TCP mode
 * Return 1 for success else 0.
 */
unsigned socket_send (unsigned sock, const uint8_t * buf, unsigned nbytes)
{
    unsigned status = 0;
    unsigned freesize = 0;

    /* check size not to exceed MAX size. */
    if (nbytes > TXBUF_SIZE)
        nbytes = TXBUF_SIZE;

    /* if freebuf is available, start. */
    do {
        freesize = w5100_getTXFreeSize (sock);
        status = w5100_readSnSR (sock);
        if ((status != SnSR_ESTABLISHED) &&
            (status != SnSR_CLOSE_WAIT)) {
            return 0;
        }
    } while (freesize < nbytes);

    /* copy data */
    w5100_send_chunk (sock, buf, nbytes);
    w5100_socket_cmd (sock, Sock_SEND);

    /* +2008.01 bj */
    while ((w5100_readSnIR (sock) & SnIR_SEND_OK) != SnIR_SEND_OK) {
        /* m2008.01 [bj] : reduce code */
        if (w5100_readSnSR (sock) == SnSR_CLOSED) {
          socket_close (sock);
          return 0;
        }
    }
    /* +2008.01 bj */
    w5100_writeSnIR (sock, SnIR_SEND_OK);
    return nbytes;
}

/*
 * This function is an application I/F function which is used to receive
 * the data in TCP mode.  It continues to wait for data as much as
 * the application wants to receive.
 * Return received data size for success else -1.
 */
unsigned socket_recv (unsigned sock, uint8_t *buf, unsigned len)
{
    /* Check how much data is available */
    unsigned navail = w5100_getRXReceivedSize (sock);
    if (navail == 0) {
        /* No data available. */
        w5100_readSnSR (sock);

        if (sock == SnSR_LISTEN ||
            sock == SnSR_CLOSED ||
            sock == SnSR_CLOSE_WAIT)
        {
            /* The remote end has closed its side of the connection,
             * so this is the eof state */
            return 0;
        }
        /* The connection is still up, but there's no data waiting
         * to be read */
        return -1;
    }

    if (navail > len) {
        navail = len;
    }

    w5100_recv_chunk (sock, buf, navail);
    w5100_socket_cmd (sock, Sock_RECV);
    return navail;
}

/*
 * Returns the first byte in the receive queue (no checking)
 */
unsigned socket_peek (unsigned sock)
{
    return w5100_recv_peek (sock);
}

/*
 * This function is an application I/F function which is used to send
 * the data for other then TCP mode.  Unlike TCP transmission, the peer's
 * destination address and the port is needed.
 *
 * This function return send data size for success else -1.
 */
unsigned socket_sendto (unsigned sock, const uint8_t *buf, unsigned len, uint8_t *addr, unsigned port)
{
    unsigned ret = 0;

    /* check size not to exceed MAX size. */
    if (len > TXBUF_SIZE)
        ret = TXBUF_SIZE;
    else
        ret = len;

    if (((addr[0] == 0x00) && (addr[1] == 0x00) &&
         (addr[2] == 0x00) && (addr[3] == 0x00)) ||
        (port == 0) || (ret == 0)) {
        /* +2008.01 [bj] : added return value */
        ret = 0;
    } else {
        w5100_writeSnDIPR (sock, addr);
        w5100_writeSnDPORT (sock, port);

        /* copy data */
        w5100_send_chunk (sock, buf, ret);
        w5100_socket_cmd (sock, Sock_SEND);

        /* +2008.01 bj */
        while ((w5100_readSnIR (sock) & SnIR_SEND_OK) != SnIR_SEND_OK) {
            if (w5100_readSnIR (sock) & SnIR_TIMEOUT) {
                /* +2008.01 [bj]: clear interrupt */
                /* clear SEND_OK & TIMEOUT */
                w5100_writeSnIR (sock, SnIR_SEND_OK | SnIR_TIMEOUT);
                return 0;
            }
        }

        /* +2008.01 bj */
        w5100_writeSnIR (sock, SnIR_SEND_OK);
    }
    return ret;
}

/*
 * This function is an application I/F function which is used to receive
 * the data in other then TCP mode.  This function is used to receive UDP,
 * IP_RAW and MAC_RAW mode, and handle the header as well.
 *
 * This function return received data size for success else -1.
 */
unsigned socket_recvfrom (unsigned sock, uint8_t *buf, unsigned len,
    uint8_t *addr, unsigned *port)
{
    uint8_t head[8];
    unsigned nreceived = 0;
    unsigned ptr, mode;

    if (len <= 0)
        return 0;

    ptr = w5100_readSnRX_RD (sock);
    mode = w5100_readSnMR (sock);
    W5100_DEBUG ("socket_recvfrom: mode %02x, RX pointer = %04x\n", mode, ptr);

    switch (mode & 0x07) {
    case SnMR_UDP:
        w5100_read_data (sock, ptr, head, 8);
        ptr += 8;

        /* read peer's IP address, port number. */
        W5100_DEBUG ("UDP peer %u.%u.%u.%u, port %u\n",
            head[0], head[1], head[2], head[3], (head[4] << 8) | head[5]);
        if (addr) {
            addr[0] = head[0];
            addr[1] = head[1];
            addr[2] = head[2];
            addr[3] = head[3];
        }
        if (port)
            *port = (head[4] << 8) | head[5];

        nreceived = (head[6] << 8) + head[7];
        break;

    case SnMR_IPRAW:
        w5100_read_data (sock, ptr, head, 6);
        ptr += 6;

        /* read peer's IP address. */
        W5100_DEBUG ("IPRAW peer %u.%u.%u.%u\n",
            head[0], head[1], head[2], head[3]);
        if (addr) {
            addr[0] = head[0];
            addr[1] = head[1];
            addr[2] = head[2];
            addr[3] = head[3];
        }
        nreceived = (head[4] << 8) + head[5];
        break;

    case SnMR_MACRAW:
        w5100_read_data (sock, ptr, head, 2);
        ptr += 2;

        nreceived = (head[0] << 8) + head[1] - 2;
        break;

    default:
        W5100_DEBUG ("unknown mode %02x\n", mode);
        return 0;
    }
    W5100_DEBUG ("received %u bytes\n", nreceived);

    /* data copy. */
    w5100_read_data (sock, ptr, buf, nreceived);
    ptr += nreceived;

    w5100_writeSnRX_RD (sock, ptr);
    W5100_DEBUG ("set RX pointer = %04x\n", ptr);

    w5100_socket_cmd (sock, Sock_RECV);
    return nreceived;
}

unsigned socket_igmpsend (unsigned sock, const uint8_t * buf, unsigned len)
{
    unsigned ret = 0;

    if (len > TXBUF_SIZE)
        ret = TXBUF_SIZE; /* check size not to exceed MAX size. */
    else
        ret = len;

    if (ret == 0)
        return 0;

    w5100_send_chunk (sock, buf, ret);
    w5100_socket_cmd (sock, Sock_SEND);

    while ((w5100_readSnIR (sock) & SnIR_SEND_OK) != SnIR_SEND_OK) {
        w5100_readSnSR (sock);
        if (w5100_readSnIR (sock) & SnIR_TIMEOUT) {
            /* in case of igmp, if send fails, then socket closed */
            /* if you want change, remove this code. */
            socket_close (sock);
            return 0;
        }
    }

    w5100_writeSnIR (sock, SnIR_SEND_OK);
    return ret;
}
