#ifndef server_h
#define server_h

#include "client.h"

extern unsigned _server_port;

void server_init (unsigned port);

int server_available (client_t *);
void server_accept (void);
void server_putc (uint8_t byte);
void server_puts (const char *str);
void server_write (const uint8_t *buf, unsigned size);

#endif
