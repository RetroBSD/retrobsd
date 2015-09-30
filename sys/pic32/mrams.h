#ifndef _MRAMS_H
#define _MRAMS_H

#ifdef KERNEL
extern int mrams_open(dev_t dev, int flag, int mode);
extern int mrams_close(dev_t dev, int flag, int mode);
extern daddr_t mrams_size(dev_t dev);
extern void mrams_strategy(struct buf *bp);
extern int mrams_ioctl (dev_t dev, u_int cmd, caddr_t addr, int flag);
#endif

#endif
