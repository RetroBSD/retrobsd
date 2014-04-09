/*
 * Web client
 *
 * This sketch connects to a website (http://www.google.com)
 * using an Arduino Wiznet Ethernet shield.
 *
 * 18 Dec 2009 created by David A. Mellis
 * 16 Apr 2012 ported to RetroBSD by Serge Vakulenko
 */
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <wiznet/ethernet.h>

/*
 * IP address of the server.
 */
unsigned char server[4];

client_t client;

int main (int argc, char **argv)
{
    if (argc > 1) {
        /* Command argument: IP address of server. */
        *(int*)server = inet_addr (argv[1]);
    } else {
        /* Google */
        server[0] = 173;
        server[1] = 194;
        server[2] = 33;
        server[3] = 104;
    }

    /* Start the Ethernet connection. */
    ethernet_init ();

    /* Give the Ethernet shield a second to initialize. */
    usleep (1000000);

    /* Initialize the Ethernet client library
     * with the IP address and port of the server
     * that you want to connect to (port 80 is default for HTTP). */
    client_init (&client, server, 80);

    printf("connecting to %u.%u.%u.%u\n",
        server[0], server[1], server[2], server[3]);
    if (! client_connect (&client)) {
        /* If you didn't get a connection to the server. */
        printf ("connection failed\n");
        client_stop (&client);
        return 0;
    }

    /* if you get a connection, report back via serial */
    printf ("connected\n");

    /* Make a HTTP request. */
    client_puts (&client, "GET / HTTP/1.0\r\n\r\n");

    while (client_connected (&client)) {
        /* If there are incoming bytes available
         * from the server, read them and print them. */
        int c = client_getc (&client);
        if (c > 0)
            putchar(c);
    }

    /* If the server's disconnected, stop the client. */
    printf("\ndisconnecting.\n");
    client_stop (&client);
    return 0;
}
