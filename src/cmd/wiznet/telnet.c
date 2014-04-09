/*
 * Telnet client
 *
 * This sketch connects to a a telnet server (http://www.google.com)
 * using an Arduino Wiznet Ethernet shield.  You'll need a telnet server
 * to test this with.
 * Processing's ChatServer example (part of the network library) works well,
 * running on port 10002. It can be found as part of the examples
 * in the Processing application, available at http://processing.org/
 *
 * Circuit:
 * - Ethernet shield attached to pins 10, 11, 12, 13
 *
 * 14 Sep 2010 created by Tom Igoe
 * 04 Jun 2012 telnet protocol by Serge Vakulenko
 */
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <wiznet/ethernet.h>

/*
 * Definitions for the TELNET protocol.
 */
#define	IAC	255		/* interpret as command: */
#define	DONT	254		/* you are not to use option */
#define	DO	253		/* please, you use option */
#define	WONT	252		/* I won't use option */
#define	WILL	251		/* I will use option */
#define	SB	250		/* interpret as subnegotiation */
#define	AYT	246		/* are you there */
#define	NOP	241		/* nop */

/*
 * Telnet options
 */
#define TELOPT_BINARY	0	/* 8-bit data path */
#define TELOPT_ECHO	1	/* echo */
#define	TELOPT_SGA	3	/* suppress go ahead */
#define	TELOPT_TM	6	/* timing mark */

/*
 * IP address of the server to connect to.
 */
unsigned char server[4];

client_t client;

/*
 * Telnet options.
 */
unsigned char local_option [256/8];
unsigned char remote_option [256/8];

/*
 * Get local and remote options.
 */
static int
get_local_option (int opt)
{
    return (local_option [opt >> 3] >> (opt & 7)) & 1;
}

static int
get_remote_option (int opt)
{
    return (remote_option [opt >> 3] >> (opt & 7)) & 1;
}

/*
* Send reply to remote host
*/
static void
reply (int cmd, int opt)
{
    client_putc (&client, IAC);
    client_putc (&client, cmd);
    client_putc (&client, opt);
}

static void
will_option (int opt)
{
    int cmd;

    switch (opt) {
    case TELOPT_BINARY:
    case TELOPT_ECHO:
    case TELOPT_SGA:
        /* Set remote option. */
        remote_option [opt >> 3] |= 1 << (opt & 7);
        cmd = DO;
        break;

    case TELOPT_TM:
    default:
        cmd = DONT;
        break;
    }
    reply (cmd, opt);
}

static void
wont_option (int opt)
{
    switch (opt) {
    case TELOPT_ECHO:
    case TELOPT_BINARY:
    case TELOPT_SGA:
        /* Clear remote option. */
        remote_option [opt >> 3] &= ~(1 << (opt & 7));
        break;
    }
    reply (DONT, opt);
}

static void
do_option (int opt)
{
    int cmd;

    switch (opt) {
    case TELOPT_ECHO:
    case TELOPT_BINARY:
    case TELOPT_SGA:
        /* Set local option. */
        local_option [opt >> 3] |= 1 << (opt & 7);
        cmd = WILL;
        break;

    case TELOPT_TM:
    default:
        cmd = WONT;
        break;
    }
    reply (cmd, opt);
}

static void
dont_option (int opt)
{
    switch (opt) {
    case TELOPT_ECHO:
    case TELOPT_BINARY:
    case TELOPT_SGA:
        /* Clear local option. */
        local_option [opt >> 3] &= ~(1 << (opt & 7));
        break;
    }
    reply (WONT, opt);
}

static void
telnet_command (int c)
{
    int i;

    switch (c) {
    case AYT:
        /* Send reply to AYT ("Are You There") request */
        client_putc (&client, IAC);
        client_putc (&client, NOP);
        break;
    case WILL:
        /* IAC WILL received (get next character) */
        c = client_getc (&client);
        if (c >= 0 && c < 256 && ! get_remote_option (c))
            will_option (c);
        break;
    case WONT:
        /* IAC WONT received (get next character) */
        c = client_getc (&client);
        if (c >= 0 && c < 256 && get_remote_option (c))
            wont_option (c);
        break;
    case DO:
        /* IAC DO received (get next character) */
        c = client_getc (&client);
        if (c >= 0 && c < 256 && ! get_local_option (c))
            do_option (c);
        break;
    case DONT:
        /* IAC DONT received (get next character) */
        c = client_getc (&client);
        if (c >= 0 && c < 256 && get_local_option (c))
            dont_option (c);
        break;
    case SB:
        for (i=0; i<128; i++) {
            c = client_getc (&client);
            if (c == IAC) {
                c = client_getc (&client);
                break;
            }
        }
        break;
    }
}

static int
telnet_getc ()
{
    int c;

    for (;;) {
        c = client_getc (&client);
        if (c != IAC)
            return c;

        c = client_getc (&client);
        if (c < 0 || c == IAC)
            return c;

        telnet_command (c);
    }
}

int main (int argc, char **argv)
{
    /* Command argument: IP address of server. */
    if (argc != 2) {
        printf ("Usage: %s <ip-address>\n", argv[0]);
        return -1;
    }
    *(int*)server = inet_addr (argv[1]);

    /* Start the Ethernet connection. */
    ethernet_init ();

    /* Give the Ethernet shield a second to initialize. */
    usleep (1000000);

    /* Initialize the Ethernet client library
     * with the IP address and port of the server
     * that you want to connect to (port 23 is default for telnet). */
    client_init (&client, server, 23);
    printf("connecting to %u.%u.%u.%u\n",
        server[0], server[1], server[2], server[3]);

    /* If you get a connection, report back via console. */
    if (! client_connect (&client)) {
        /* If you didn't get a connection to the server. */
        printf("connection failed\n");
        client_stop (&client);
        return 0;
    }
    printf("connected\n");

    /* Setup telnet options */
    do_option (TELOPT_ECHO);
    do_option (TELOPT_SGA);
    will_option (TELOPT_SGA);

    while (client_connected (&client)) {

        /* If there are incoming bytes available
         * from the server, read them and print them. */
        if (client_available (&client)) {
            int c = telnet_getc();
            if (c >= 0)
                putchar(c);
        }
        fflush (stdout);

        /* As long as there are bytes in the serial queue,
         * read them and send them out the socket if it's open. */
        for (;;) {
            off_t nread;
            char c;

            if (ioctl (0, FIONREAD, &nread) < 0 || nread == 0)
                break;

            read(0, &c, 1);
            if (client_connected (&client)) {
                client_putc (&client, c);

                /* IAC -> IAC IAC */
                if (c == (char) IAC)
                    client_putc (&client, c);
            }
        }
    }

    /* If the server's disconnected, stop the client. */
    printf("\ndisconnecting.\n");
    client_stop (&client);
    return 0;
}
