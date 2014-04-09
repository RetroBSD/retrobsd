#ifndef	_SOCKET_H_
#define	_SOCKET_H_

#include "w5100.h"

extern unsigned _socket_port [MAX_SOCK_NUM];

/*
 * Opens a socket(TCP or UDP or IP_RAW mode)
 */
unsigned socket_init (unsigned sock, unsigned protocol, unsigned port, unsigned flag);

/*
 * Close socket
 */
void socket_close (unsigned sock);

/*
 * Establish TCP connection (Active connection)
 */
unsigned socket_connect (unsigned sock, uint8_t *addr, unsigned port);

/*
 * disconnect the connection
 */
void socket_disconnect (unsigned sock);

/*
 * Establish TCP connection (Passive connection)
 */
unsigned socket_listen (unsigned sock);

/*
 * Send data (TCP)
 */
unsigned socket_send (unsigned sock, const uint8_t *buf, unsigned len);

/*
 * Receive data (TCP)
 */
unsigned socket_recv (unsigned sock, uint8_t *buf, unsigned len);
unsigned socket_peek (unsigned sock);

/*
 * Send data (UDP/IP RAW)
 */
unsigned socket_sendto (unsigned sock, const uint8_t *buf, unsigned len, uint8_t *addr, unsigned port);

/*
 * Receive data (UDP/IP RAW)
 */
unsigned socket_recvfrom (unsigned sock, uint8_t *buf, unsigned len, uint8_t *addr, unsigned *port);

unsigned socket_igmpsend (unsigned sock, const uint8_t *buf, unsigned len);

#endif /* _SOCKET_H_ */
