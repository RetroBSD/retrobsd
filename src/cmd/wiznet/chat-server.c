/*
 * Chat Server
 *
 * A simple server that distributes any incoming messages to all
 * connected clients.  To use telnet to your device's IP address and type.
 * Using an Arduino Wiznet Ethernet shield.
 *
 * 18 Dec 2009 created by David A. Mellis
 * 10 Aug 2010 modified by Tom Igoe
 * 16 Apr 2012 ported to RetroBSD by Serge Vakulenko
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <wiznet/ethernet.h>

int got_a_message = 0;  /* whether you got a message from the client yet */

int main()
{
    /* Initialize the ethernet device */
    ethernet_init ();

    /* Start listening for clients.
     * Telnet defaults to port 23. */
    server_init (23);

    for (;;) {
        /* Wait for a new client. */
        client_t client;
        if (! server_available (&client))
            continue;

        /* When the client sends the first byte, say hello. */
        if (! got_a_message) {
            printf ("We have a new client\n");
            client_puts (&client, "Hello, client!\n");
            got_a_message = 1;
        }

        /* Read the bytes incoming from the client.
         * Echo the bytes back to the client. */
        char c = client_getc (&client);
        server_putc (c);

        /* Echo the bytes to the server as well. */
        putchar (c);
        fflush (stdout);
    }
}
