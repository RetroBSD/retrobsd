#ifndef _HX8356_H
#define _HX8357_H

#ifdef KERNEL
extern int hx8357_open(dev_t dev, int flag, int mode);
extern int hx8357_close(dev_t dev, int flag, int mode);
extern int hx8357_read(dev_t dev, struct uio *uio, int flag);
extern int hx8357_write(dev_t dev, struct uio *uio, int flag);
extern int hx8357_ioctl(dev_t dev, u_int cmd, caddr_t addr, int flag);
#endif

#endif
