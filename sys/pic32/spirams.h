#ifndef _SPIRAMS_H
#define _SPIRAMS_H

#ifdef KERNEL
extern int spirams_open(dev_t dev, int flag, int mode);
extern int spirams_close(dev_t dev, int flag, int mode);
extern daddr_t spirams_size(dev_t dev);
extern void spirams_strategy(struct buf *bp);
extern int spirams_ioctl (dev_t dev, u_int cmd, caddr_t addr, int flag);
#endif

#endif
