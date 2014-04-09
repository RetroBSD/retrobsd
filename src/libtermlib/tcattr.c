//
// TODO: termios support
//
#if 0
#include <errno.h>
#include <termios.h>

int tcgetattr(int fd, struct termios *t)
{

        return (ioctl(fd, TIOCGETA, t));
}

int tcsetattr(int fd, int opt, struct termios *t)
{
        struct termios localterm;

        if (opt & TCSASOFT) {
                localterm = *t;
                localterm.c_cflag |= CIGNORE;
                t = &localterm;
        }
        switch (opt & ~TCSASOFT) {
        case TCSANOW:
                return (ioctl(fd, TIOCSETA, t));
        case TCSADRAIN:
                return (ioctl(fd, TIOCSETAW, t));
        case TCSAFLUSH:
                return (ioctl(fd, TIOCSETAF, t));
        default:
                errno = EINVAL;
                return (-1);
        }
}
#endif
