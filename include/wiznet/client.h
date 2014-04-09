#ifndef client_h
#define client_h

struct _client_t {
    unsigned sock;
    uint8_t *ip;
    unsigned port;
};
typedef struct _client_t client_t;

extern unsigned _client_srcport;

void client_init (client_t *c, uint8_t *ip, unsigned port);
void client_init_sock (client_t *c, unsigned sock);

unsigned client_status (client_t *);
int client_connect (client_t *);
void client_putc (client_t *, uint8_t);
void client_puts (client_t *c, const char *str);
void client_write (client_t *c, const uint8_t *buf, unsigned size);
int client_available (client_t *);
int client_getc (client_t *);
int client_read (client_t *c, uint8_t *buf, unsigned size);
int client_peek (client_t *);
void client_flush (client_t *);
void client_stop (client_t *);
int client_connected (client_t *);

#endif
