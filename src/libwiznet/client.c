#include <string.h>
#include <unistd.h>
#include <wiznet/ethernet.h>
#include <wiznet/socket.h>

unsigned _client_srcport = 1024;

void client_init (client_t *c, uint8_t *ip, unsigned port)
{
    c->ip = ip;
    c->port = port;
    c->sock = MAX_SOCK_NUM;
}

void client_init_sock (client_t *c, unsigned sock)
{
    c->sock = sock;
}

int client_connect (client_t *c)
{
    int i;

    if (c->sock != MAX_SOCK_NUM)
        return 0;

    for (i = 0; i < MAX_SOCK_NUM; i++) {
        unsigned s = w5100_readSnSR (i);
        if (s == SnSR_CLOSED || s == SnSR_FIN_WAIT) {
            c->sock = i;
            break;
        }
    }

    if (c->sock == MAX_SOCK_NUM)
        return 0;

    _client_srcport++;
    if (_client_srcport == 0)
        _client_srcport = 1024;
    socket_init (c->sock, SnMR_TCP, _client_srcport, 0);

    if (! socket_connect (c->sock, c->ip, c->port)) {
        c->sock = MAX_SOCK_NUM;
        return 0;
    }

    while (client_status(c) != SnSR_ESTABLISHED) {
        usleep (10000);
        if (client_status(c) == SnSR_CLOSED) {
            c->sock = MAX_SOCK_NUM;
            return 0;
        }
    }
    return 1;
}

void client_putc (client_t *c, uint8_t b)
{
    if (c->sock != MAX_SOCK_NUM)
        socket_send (c->sock, &b, 1);
}

void client_puts (client_t *c, const char *str)
{
    if (c->sock != MAX_SOCK_NUM)
        socket_send (c->sock, (const uint8_t *)str, strlen(str));
}

void client_write (client_t *c, const uint8_t *buf, size_t size)
{
    if (c->sock != MAX_SOCK_NUM)
        socket_send (c->sock, buf, size);
}

int client_available (client_t *c)
{
    if (c->sock != MAX_SOCK_NUM)
        return w5100_getRXReceivedSize (c->sock);
    return 0;
}

int client_getc (client_t *c)
{
    uint8_t b;

    if (socket_recv (c->sock, &b, 1) <= 0) {
        // No data available
        return -1;
    }
    return b;
}

int client_read (client_t *c, uint8_t *buf, size_t size)
{
    return socket_recv (c->sock, buf, size);
}

int client_peek (client_t *c)
{
    uint8_t b;

    // Unlike recv, peek doesn't check to see if there's any data available, so we must
    if (! client_available (c))
      return -1;

    b = socket_peek (c->sock);
    return b;
}

void client_flush (client_t *c)
{
    while (client_available (c))
        client_getc (c);
}

void client_stop (client_t *c)
{
    int i;

    if (c->sock == MAX_SOCK_NUM)
        return;

    // attempt to close the connection gracefully (send a FIN to other side)
    socket_disconnect (c->sock);

    // wait a second for the connection to close
    for (i=0; i<100; i++) {
        if (client_status (c) == SnSR_CLOSED)
            break;
        usleep (10000);
    }

    // if it hasn't closed, close it forcefully
    if (client_status(c) != SnSR_CLOSED)
        socket_close (c->sock);

    _socket_port[c->sock] = 0;
    c->sock = MAX_SOCK_NUM;
}

int client_connected (client_t *c)
{
    if (c->sock == MAX_SOCK_NUM)
        return 0;

    unsigned s = client_status (c);
    return ! (s == SnSR_LISTEN || s == SnSR_CLOSED || s == SnSR_FIN_WAIT ||
        (s == SnSR_CLOSE_WAIT && ! client_available (c)));
}

unsigned client_status (client_t *c)
{
    if (c->sock == MAX_SOCK_NUM)
        return SnSR_CLOSED;
    return w5100_readSnSR (c->sock);
}
