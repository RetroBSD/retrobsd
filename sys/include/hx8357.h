#ifndef _HX8356_H
#define _HX8357_H

#include "tty.h"

extern const struct devspec hx8357devs[];
extern int hx8357_open(dev_t dev, int flag, int mode);
extern int hx8357_close(dev_t dev, int flag, int mode);
extern int hx8357_read(dev_t dev, struct uio *uio, int flag);
extern int hx8357_write(dev_t dev, struct uio *uio, int flag);
extern int hx8357_ioctl(dev_t dev, register u_int cmd, caddr_t addr, int flag);
extern int hx8357_select(dev_t dev, int rw);
extern void hx8357_putc(dev_t dev, char c);
extern char hx8357_getc(dev_t dev);
extern void hx8357_init();


extern struct tty hx8357_ttys[1];


#endif
