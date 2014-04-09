#include <string.h>
#include <wiznet/socket.h>
#include <wiznet/server.h>

unsigned _server_port;

void server_init (unsigned port)
{
    unsigned sock;
    client_t client;

    _server_port = port;
    for (sock = 0; sock < MAX_SOCK_NUM; sock++) {
        client_init_sock (&client, sock);

        if (client_status (&client) == SnSR_CLOSED) {
            socket_init (sock, SnMR_TCP, port, 0);
            socket_listen (sock);
            _socket_port[sock] = port;
            break;
        }
    }
}

void server_accept()
{
    unsigned sock;
    client_t client;
    int listening = 0;

    for (sock = 0; sock < MAX_SOCK_NUM; sock++) {
        client_init_sock (&client, sock);

        if (_socket_port[sock] == _server_port) {
            if (client_status (&client) == SnSR_LISTEN) {
                listening = 1;
            }
            else if (client_status (&client) == SnSR_CLOSE_WAIT &&
                     ! client_available (&client)) {
                client_stop (&client);
            }
        }
    }

    if (! listening) {
        server_init (_server_port);
    }
}

int server_available (client_t *client)
{
    unsigned sock;

    server_accept();

    for (sock = 0; sock < MAX_SOCK_NUM; sock++) {
        client_init_sock (client, sock);

        if (_socket_port[sock] == _server_port &&
              (client_status (client) == SnSR_ESTABLISHED ||
               client_status (client) == SnSR_CLOSE_WAIT)) {
            if (client_available (client)) {
                // XXX: don't always pick the lowest numbered socket.
                return 1;
            }
        }
    }
    return 0;
}

void server_putc (uint8_t b)
{
    server_write (&b, 1);
}

void server_puts (const char *str)
{
    server_write ((const uint8_t *)str, strlen(str));
}

void server_write (const uint8_t *buffer, size_t size)
{
    unsigned sock;
    client_t client;

    server_accept();

    for (sock = 0; sock < MAX_SOCK_NUM; sock++) {
        client_init_sock (&client, sock);

        if (_socket_port[sock] == _server_port &&
          client_status (&client) == SnSR_ESTABLISHED) {
            client_write (&client, buffer, size);
        }
    }
}
