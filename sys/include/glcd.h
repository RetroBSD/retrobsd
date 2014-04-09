#ifndef _GLCD_H
#define _GLCD_H

#include <sys/ioctl.h>
#include <sys/uio.h>

//G13
#define TRIS_DI         TRISG
#define LAT_DI          LATG
#define PORT_DI         PORTG
#define PIN_DI          13

//G12
#define TRIS_RW         TRISG
#define LAT_RW          LATG
#define PORT_RW         PORTG
#define PIN_RW          12

//G14
#define TRIS_E          TRISG
#define LAT_E           LATG
#define PORT_E          PORTG
#define PIN_E           14

//E1
#define TRIS_DB0        TRISE
#define LAT_DB0         LATE
#define PORT_DB0        PORTE
#define PIN_DB0         1

//E0
#define TRIS_DB1        TRISE
#define LAT_DB1         LATE
#define PORT_DB1        PORTE
#define PIN_DB1         0

//A7
#define TRIS_DB2        TRISA
#define LAT_DB2         LATA
#define PORT_DB2        PORTA
#define PIN_DB2         7

//A6
#define TRIS_DB3        TRISA
#define LAT_DB3         LATA
#define PORT_DB3        PORTA
#define PIN_DB3         6

//G0
#define TRIS_DB4        TRISG
#define LAT_DB4         LATG
#define PORT_DB4        PORTG
#define PIN_DB4         0

//G1
#define TRIS_DB5        TRISG
#define LAT_DB5         LATG
#define PORT_DB5        PORTG
#define PIN_DB5         1

//F1
#define TRIS_DB6        TRISF
#define LAT_DB6         LATF
#define PORT_DB6        PORTF
#define PIN_DB6         1

//F0
#define TRIS_DB7        TRISF
#define LAT_DB7         LATF
#define PORT_DB7        PORTF
#define PIN_DB7         0

//D7
#define TRIS_CS1        TRISD
#define LAT_CS1         LATD
#define PORT_CS1        PORTD
#define PIN_CS1         7

//D6
#define TRIS_CS2        TRISD
#define LAT_CS2         LATD
#define PORT_CS2        PORTD
#define PIN_CS2         6

//D5
#define TRIS_RES        TRISD
#define LAT_RES         LATD
#define PORT_RES        PORTD
#define PIN_RES         5

#define GLCD_CMD_OFF         0b00111110
#define GLCD_CMD_ON          0b00111111
#define GLCD_CMD_SET_Y       0b01000000
#define GLCD_CMD_SET_PAGE    0b10111000
#define GLCD_CMD_START       0b11000000

#define INPUT 1
#define OUTPUT 0

#define HIGH 1
#define LOW 0

#define DATA 1
#define INSTRUCTION 0

#define READ 1
#define WRITE 0

#define ENABLE 1
#define DISABLE 0

#define GLCD_STAT_BUSY   0b10000000
#define GLCD_STAT_ONOFF  0b00100000
#define GLCD_STAT_RESET  0b00010000

/* glcd interface */

struct glcd_command {
    unsigned char x1;
    unsigned char y1;
    unsigned char x2;
    unsigned char y2;
    unsigned char ink;
};

#define GLCD_RESET       _IOW('i', 1, struct glcd_command)
#define GLCD_CLS         _IOW('i', 2, struct glcd_command)
#define GLCD_LOAD_PAGE   _IOW('i', 3, struct glcd_command)
#define GLCD_UPDATE      _IOW('i', 4, struct glcd_command)
#define GLCD_SET_PIXEL   _IOW('i', 5, struct glcd_command)
#define GLCD_CLEAR_PIXEL _IOW('i', 6, struct glcd_command)
#define GLCD_LINE        _IOW('i', 7, struct glcd_command)
#define GLCD_BOX         _IOW('i', 8, struct glcd_command)
#define GLCD_FILLED_BOX  _IOW('i', 9, struct glcd_command)
#define GLCD_GOTO_XY     _IOW('i', 10, struct glcd_command)

#ifdef KERNEL
#include "conf.h"

extern const struct devspec glcddevs[];

extern int glcd_open (dev_t dev, int flag, int mode);
extern int glcd_close (dev_t dev, int flag, int mode);
extern int glcd_read (dev_t dev, struct uio *uio, int flag);
extern int glcd_write (dev_t dev, struct uio *uio, int flag);
extern int glcd_ioctl (dev_t dev, u_int cmd, caddr_t addr, int flag);
#endif

#endif
