/*
 * SmallC: interface to wiznet library.
 */
#define MAX_SOCK_NUM            4   /* Max number of sockets per chip */
#define CLIENT_SIZE             3   /* Size of client structure in words */
#define UDP_SIZE                2   /* Size of UDP structure in words */

extern unsigned _socket_port[];

extern unsigned _client_srcport;

extern unsigned _server_port;
