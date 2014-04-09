#ifndef _TERMIOS_H
#define _TERMIOS_H

#include <sys/ioctl.h>
#include <stdint.h>

#define E_TERMCAP       "/etc/termcap"

typedef unsigned char   cc_t;
typedef unsigned int    speed_t;
typedef unsigned int    tcflag_t;

#define NCCS 32
struct termios
  {
    tcflag_t c_iflag;           /* input mode flags */
    tcflag_t c_oflag;           /* output mode flags */
    tcflag_t c_cflag;           /* control mode flags */
    tcflag_t c_lflag;           /* local mode flags */
    cc_t c_line;                        /* line discipline */
    cc_t c_cc[NCCS];            /* control characters */
    speed_t c_ispeed;           /* input speed */
    speed_t c_ospeed;           /* output speed */
#define _HAVE_STRUCT_TERMIOS_C_ISPEED 1
#define _HAVE_STRUCT_TERMIOS_C_OSPEED 1
  };

extern int tcgetattr(int, struct termios *);
extern int tcsetattr(int, int, struct termios *);

/* c_cc characters */
#define VINTR 0
#define VQUIT 1
#define VERASE 2
#define VKILL 3
#define VEOF 4
#define VTIME 5
#define VMIN 6
#define VSWTC 7
#define VSTART 8
#define VSTOP 9
#define VSUSP 10
#define VEOL 11
#define VREPRINT 12
#define VDISCARD 13
#define VWERASE 14
#define VLNEXT 15
#define VEOL2 16

/* c_iflag bits */
#define IGNBRK  0000001
#define BRKINT  0000002
#define IGNPAR  0000004
#define PARMRK  0000010
#define INPCK   0000020
#define ISTRIP  0000040
#define INLCR   0000100
#define IGNCR   0000200
#define ICRNL   0000400
#define IUCLC   0001000
#define IXON    0002000
#define IXANY   0004000
#define IXOFF   0010000
#define IMAXBEL 0020000
#define IUTF8   0040000

/* c_oflag bits */
#define OPOST   0000001
#define OLCUC   0000002
#define ONLCR   0000004
#define OCRNL   0000010
#define ONOCR   0000020
#define ONLRET  0000040
#define OFILL   0000100
#define OFDEL   0000200
#if defined __USE_MISC || defined __USE_XOPEN
# define NLDLY  0000400
# define   NL0  0000000
# define   NL1  0000400
# define CRDLY  0003000
# define   CR0  0000000
# define   CR1  0001000
# define   CR2  0002000
# define   CR3  0003000
# define TABDLY 0014000
# define   TAB0 0000000
# define   TAB1 0004000
# define   TAB2 0010000
# define   TAB3 0014000
# define BSDLY  0020000
# define   BS0  0000000
# define   BS1  0020000
# define FFDLY  0100000
# define   FF0  0000000
# define   FF1  0100000
#endif

#define VTDLY   0040000
#define   VT0   0000000
#define   VT1   0040000

#ifdef __USE_MISC
# define XTABS  0014000
#endif

/* c_lflag bits */
#define ISIG    0000001
#define ICANON  0000002

#define CSIZE   0000060
#define   CS5   0000000
#define   CS6   0000020
#define   CS7   0000040
#define   CS8   0000060
#define CSTOPB  0000100
#define CREAD   0000200
#define PARENB  0000400
#define PARODD  0001000
#define HUPCL   0002000
#define CLOCAL  0004000

#define IEXTEN  0100000

/* tcflow() and TCXONC use these */
#define TCOOFF          0
#define TCOON           1
#define TCIOFF          2
#define TCION           3

/* tcflush() and TCFLSH use these */
#define TCIFLUSH        0
#define TCOFLUSH        1
#define TCIOFLUSH       2

/* tcsetattr uses these */
#define TCSANOW         0
#define TCSADRAIN       1
#define TCSAFLUSH       2

#define TCSASOFT        0x10
#define CIGNORE         0x00000001

#define TIOCGETA    _IOR(i, 92, struct termios)
#define TIOCSETA    _IOW(i, 92, struct termios)
#define TIOCSETAW   _IOW(i, 92, struct termios)
#define TIOCSETAF   _IOW(i, 92, struct termios)


#endif
