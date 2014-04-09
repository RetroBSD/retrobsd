#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <wiznet/ethernet.h>
#include <wiznet/w5100.h>

unsigned char ethernet_mac [6];
unsigned char ethernet_ip [4];
unsigned char ethernet_gateway [4];
unsigned char ethernet_netmask [4];

/*
 * Get MAC address from string to byte array.
 */
static void parse_mac (unsigned char *macp, char *str)
{
    unsigned char *limit = macp + 6;
    register unsigned c, val;

    do {
        /* Collect number up to ":". */
        val = 0;
        while ((c = (unsigned char) *str)) {
            if (c >= '0' && c <= '9')
                val = (val << 4) + (c - '0');
            else if (c >= 'a' && c <= 'f')
                val = (val << 4) + (c - 'a' + 10);
            else if (c >= 'A' && c <= 'F')
                val = (val << 4) + (c - 'A' + 10);
            else
                break;
            str++;
        }
        *macp++ = val;
    } while (*str++ == ':' && macp < limit);
}

/*
 * Get IP address from string to byte array.
 */
static void parse_ip (unsigned char *val, char *str)
{
    unsigned long addr;

    if (! str)
        return;
    addr = inet_addr (str);
    val[0] = addr >> 0;
    val[1] = addr >> 8;
    val[2] = addr >> 16;
    val[3] = addr >> 24;
}

/*
 * Initialize Ethernet controller.
 * Get parameters from environment:
 *  MAC=aa:bb:cc:dd:ee:ff   - unique MAC address
 *  IP=12.34.56.78          - IP address
 *  GATEWAY=12.34.56.1      - gateway to Internet (optional)
 *  NETMASK=255.255.255.0   - netmask of local Ethernet network (optional)
 */
void ethernet_init ()
{
    char *mac, *ip, *gateway, *netmask;

    mac = getenv("MAC");
    if (! mac) {
        fprintf (stderr, "Please set MAC environment variable\n");
        exit (-1);
    }
    parse_mac (ethernet_mac, mac);

    ip = getenv("IP");
    if (! ip) {
        fprintf (stderr, "Please set IP environment variable\n");
        exit (-1);
    }
    parse_ip (ethernet_ip, ip);

    gateway = getenv("GATEWAY");
    if (gateway != 0) {
        parse_ip (ethernet_gateway, gateway);
    } else {
        ethernet_gateway[0] = ethernet_ip[0];
        ethernet_gateway[1] = ethernet_ip[1];
        ethernet_gateway[2] = ethernet_ip[2];
        ethernet_gateway[3] = 1;
    }

    netmask = getenv("NETMASK");
    if (netmask != 0) {
        parse_ip (ethernet_netmask, netmask);
    } else {
        ethernet_netmask[0] = 255;
        ethernet_netmask[1] = 255;
        ethernet_netmask[2] = 255;
        ethernet_netmask[3] = 0;
    }
    printf("local MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",
        ethernet_mac[0], ethernet_mac[1], ethernet_mac[2],
        ethernet_mac[3], ethernet_mac[4], ethernet_mac[5]);
    printf("local IP address %u.%u.%u.%u\n",
        ethernet_ip[0], ethernet_ip[1],
        ethernet_ip[2], ethernet_ip[3]);
    printf("gateway %u.%u.%u.%u\n",
        ethernet_gateway[0], ethernet_gateway[1],
        ethernet_gateway[2], ethernet_gateway[3]);
    printf("netmask %u.%u.%u.%u\n",
        ethernet_netmask[0], ethernet_netmask[1],
        ethernet_netmask[2], ethernet_netmask[3]);

    w5100_init();

    /* Set Ethernet MAC address. */
    w5100_writeSHAR (ethernet_mac);

    /* Set local IP address. */
    w5100_writeSIPR (ethernet_ip);

    /* Set gateway IP address. */
    w5100_writeGAR (ethernet_gateway);

    /* Set subnet mask. */
    w5100_writeSUBR (ethernet_netmask);

    /* Set retransmission timeout to 200 msec. */
    w5100_writeRTR (200*10);

    /* Set retransmission count. */
    //w5100_writeRCR (3);
}
