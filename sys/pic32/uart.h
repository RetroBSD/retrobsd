#ifndef _UART_H
#define _UART_H

#define NUART 6

#ifdef KERNEL

void uartinit(int unit);
int uartopen(dev_t dev, int flag, int mode);
int uartclose(dev_t dev, int flag, int mode);
int uartread(dev_t dev, struct uio * uio, int flag);
int uartwrite(dev_t dev, struct uio *uio, int flag);
int uartselect(dev_t dev, int rw);
int uartioctl(dev_t dev, u_int cmd, caddr_t addr, int flag);
void uartintr(dev_t dev);
void uartstart(struct tty *tp);
void uartputc(dev_t dev, char c);
char uartgetc(dev_t dev);

extern struct tty uartttys[NUART];

#endif

#endif
