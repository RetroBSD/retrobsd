/*
 * Control general purpose i/o pins.
 *
 * Copyright (C) 2012 Serge Vakulenko, <serge@vak.ru>
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/gpio.h>

enum {
    OP_ASSIGN,
    OP_CONF_INPUT,
    OP_CONF_OUTPUT,
    OP_CONF_OPENDRAIN,
    OP_DECONF,
    OP_SET,
    OP_CLEAR,
    OP_INVERT,
    OP_POLL,
};

const char *opname[] = {
    "ASSIGN",
    "CONF_INPUT",
    "CONF_OUTPUT",
    "CONF_OPENDRAIN",
    "DECONF",
    "SET",
    "CLEAR",
    "INVERT",
    "POLL",
};

char *progname;
int verbose;
int fd;

/*
 * Parse port name.
 * Split it into port number and pin mask.
 * Return nonzero on success.
 * Examples:
 *      a0          single pin
 *      b3-7,11     list of pins
 *      c           same as c0-15 (zero mask returned)
 */
int parse_port (char *arg, int *pnum, int *pmask)
{
    char *arg0 = arg;

    if (*arg >= 'a' && *arg <='z')
        *pnum = *arg - 'a';
    else if (*arg >= 'A' && *arg <='Z')
        *pnum = *arg - 'A';
    else {
fatal:  fprintf (stderr, "%s: incorrect port '%s'\n", progname, arg0);
        return 0;
    }
    *pmask = 0;
    while (*++arg) {
        char *ep;
        unsigned from = strtoul (arg, &ep, 10);
        if (ep == arg)
            goto fatal;
        arg = ep + 1;
        if (*ep == '-') {
            unsigned to = strtoul (arg, &ep, 10);
            if (ep == arg)
                goto fatal;
            arg = ep + 1;
            if (from > to) {
                unsigned v = from;
                from = to;
                to = v;
            }
            *pmask |= (0xffff >> (15 - to + from)) << from;
        } else {
            *pmask |= 1 << from;
        }
        if (*ep == 0)
            return 1;
        if (*ep != ',')
            goto fatal;
    }
    return 1;
}

void do_op (int op, char *port_arg, char *value_arg)
{
    int pnum, pmask, i, value = 0;

    if (! parse_port (port_arg, &pnum, &pmask))
        return;
    if (pmask == 0) {
        /* No pin numbers - use all pins. */
        pmask = 0xffff;
    }

    if (value_arg) {
        if (value_arg[0] == '0' && value_arg[1] == 'b') {
            /* Binary value 0bxxx */
            value = strtoul (value_arg + 2, 0, 2);
        } else
            value = strtoul (value_arg, 0, 0);
        if (verbose)
            printf ("%s port %c mask %04x value %04x\n",
                opname[op], pnum + 'A', pmask, value);
    } else if (verbose) {
        printf ("%s port %c mask %04x\n",
            opname[op], pnum + 'A', pmask);
    }

    switch (op) {
    case OP_ASSIGN:
        if (pmask == 0xffff) {                  /* All pins */
            if (ioctl (fd, GPIO_STORE | GPIO_PORT (pnum), value) < 0)
                perror ("GPIO_STORE");

        } else if (pmask & (pmask-1)) {         /* Several pins */
            for (i=1; ! (pmask & i); i<<=1)
                value <<= 1;
            if (ioctl (fd, GPIO_STORE | GPIO_PORT (pnum), value) < 0)
                perror ("GPIO_STORE");

        } else if (value & 1) {                 /* Set single pin */
            if (ioctl (fd, GPIO_SET | GPIO_PORT (pnum), pmask) < 0)
                perror ("GPIO_SET");
        } else {                                /* Clear single pin */
            if (ioctl (fd, GPIO_CLEAR | GPIO_PORT (pnum), pmask) < 0)
                perror ("GPIO_SET");
        }
        break;
    case OP_CONF_INPUT:
        if (ioctl (fd, GPIO_CONFIN | GPIO_PORT (pnum), pmask) < 0)
            perror ("GPIO_CONFIN");
        break;
    case OP_CONF_OUTPUT:
        if (ioctl (fd, GPIO_CONFOUT | GPIO_PORT (pnum), pmask) < 0)
            perror ("GPIO_CONFOUT");
        break;
    case OP_CONF_OPENDRAIN:
        if (ioctl (fd, GPIO_CONFOD | GPIO_PORT (pnum), pmask) < 0)
            perror ("GPIO_CONFOD");
        break;
    case OP_DECONF:
        if (ioctl (fd, GPIO_DECONF | GPIO_PORT (pnum), pmask) < 0)
            perror ("GPIO_DECONF");
        break;
    case OP_SET:
        if (ioctl (fd, GPIO_SET | GPIO_PORT (pnum), pmask) < 0)
            perror ("GPIO_SET");
        break;
    case OP_CLEAR:
        if (ioctl (fd, GPIO_CLEAR | GPIO_PORT (pnum), pmask) < 0)
            perror ("GPIO_CLEAR");
        break;
    case OP_INVERT:
        if (ioctl (fd, GPIO_INVERT | GPIO_PORT (pnum), pmask) < 0)
            perror ("GPIO_INVERT");
        break;
    case OP_POLL:
        value = ioctl (fd, GPIO_POLL | GPIO_PORT (pnum), 0);
        if (value < 0)
            perror ("GPIO_POLL");
        if (pmask == 0xffff) {                  /* All pins */
            printf ("0x%04x\n", value);

        } else {                                /* Some pins */
            value &= pmask;
            for (i=1; ! (pmask & i); i<<=1)
                value >>= 1;
            printf ("%#x\n", value);
        }
        break;
    }
}

void usage ()
{
    fprintf (stderr, "Control gpio pins.\n");
    fprintf (stderr, "Usage:\n");
    fprintf (stderr, "    %s option...\n", progname);
    fprintf (stderr, "Options:\n");
    fprintf (stderr, "    -i ports        configure ports as input\n");
    fprintf (stderr, "    -o ports        configure ports as output\n");
    fprintf (stderr, "    -d ports        configure ports as open-drain output\n");
    fprintf (stderr, "    -x ports        deconfigure ports\n");
    fprintf (stderr, "    -a port value   assign port value\n");
    fprintf (stderr, "    -s ports        set ports to 1\n");
    fprintf (stderr, "    -c ports        clear ports (set to 0)\n");
    fprintf (stderr, "    -r ports        reverse ports (invert)\n");
    fprintf (stderr, "    -g ports        get port values\n");
    fprintf (stderr, "    -m msec         delay in milliseconds\n");
    fprintf (stderr, "    -v              verbose mode\n");
    fprintf (stderr, "Ports:\n");
    fprintf (stderr, "    a0              single pin\n");
    fprintf (stderr, "    b3-7,11         list of pins\n");
    fprintf (stderr, "    c               same as c0-15\n");
    exit (-1);
}

int main (int argc, char **argv)
{
    register int op, msec;
    register char *arg;
    const char *devname = "/dev/porta";

    progname = *argv++;
    if (argc <= 1)
        usage();

    fd = open (devname, 1);
    if (fd < 0) {
        perror (devname);
        return -1;
    }

    /* Default operation is poll. */
    op = OP_POLL;
    while (--argc > 0) {
        arg = *argv++;
        if (arg[0] != '-') {
            if (op == OP_ASSIGN) {
                if (--argc <= 0 || **argv=='-') {
                    fprintf (stderr, "%s: option -a: second argument missed\n", progname);
                    return -1;
                }
                do_op (op, arg, *argv++);
            } else {
                do_op (op, arg, 0);
            }
            continue;
        }
        if (arg[1] == 0 || arg[2] != 0) {
badop:      fprintf (stderr, "%s: unknown option `%s'\n", progname, arg);
            return -1;
        }
        switch (arg[1]) {
        case 'i':               /* configure ports as input */
            op = OP_CONF_INPUT;
            break;
        case 'o':               /* configure ports as output */
            op = OP_CONF_OUTPUT;
            break;
        case 'd':               /* configure ports as open-drain output */
            op = OP_CONF_OPENDRAIN;
            break;
        case 'x':               /* deconfigure ports */
            op = OP_DECONF;
            break;
        case 'a':               /* set a value of port */
            op = OP_ASSIGN;
            break;
        case 's':               /* set ports to 1 */
            op = OP_SET;
            break;
        case 'c':               /* clear ports */
            op = OP_CLEAR;
            break;
        case 'r':               /* reverse ports */
            op = OP_INVERT;
            break;
        case 'g':               /* get port values */
            op = OP_POLL;
            break;
        case 'v':               /* verbose mode */
            verbose++;
            break;
        case 'm':               /* delay in milliseconds */
            if (--argc <= 0 || **argv=='-') {
                fprintf (stderr, "%s: option -m: argument missed\n", progname);
                return -1;
            }
            msec = strtol (*argv++, 0, 0);
            usleep (msec * 1000);
            break;
        case 'h':               /* help */
            usage();
        default:
            goto badop;
        }
    }
    return 0;
}
