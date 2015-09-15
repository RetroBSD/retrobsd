/*
 * Copyright (c) 1992, 1993
 *      The Regents of the University of California.  All rights reserved.
 * Copyright (c) 2015 Serge Vakulenko
 *
 * This code is derived from software contributed to Berkeley by
 * Ralph Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * This structure is used to encapsulate the routines for a device driver.
 * This allows an "object oriented" approach so a controller device driver
 * can support multiple attached devices or a device can be attached to
 * different types of controllers.
 */
struct driver {
    const char  *d_name;        /* driver name for vmstat and iostat */
    int         (*d_init)();    /* routine to probe & initialize device */
};

/*
 * This structure describes controllers directly connected to CPU
 * and is partially initialized in "ioconf.c" by the 'config' program.
 */
struct conf_ctlr {
    struct driver   *ctlr_driver;   /* controller driver routines */
    int             ctlr_unit;      /* controller number */
    char            *ctlr_addr;     /* address of controller */
    int             ctlr_pri;       /* interrupt priority */
    int             ctlr_flags;     /* flags */

    int             ctlr_alive;     /* true if init routine succeeded */
};

/*
 * This structure describes devices connected to an interface
 * and is partially initialized in "ioconf.c" by the 'config' program.
 */
struct conf_device {
    struct driver   *dev_driver;    /* device driver routines */
    struct driver   *dev_cdriver;   /* interface driver routines */
    int             dev_unit;       /* device unit number */
    int             dev_ctlr;       /* device interface number */
    int             dev_drive;      /* device address number */
    int             dev_flags;      /* flags */

    /* assignment of signals to physical pins */
#define KCONF_MAXPINS 16
    char            dev_pins[KCONF_MAXPINS];

    int             dev_alive;      /* true if init routine succeeded */
};

/*
 * This structure describes optional software services.
 */
struct conf_service {
    void        (*svc_attach)();    /* routine to initialize service */
};

/* Define special unit types used by the config program */
#define QUES    -1      /* -1 means '?' */
#define UNKNOWN -2      /* -2 means not set yet */

#ifdef KERNEL
extern struct conf_ctlr conf_ctlr_init[];
extern struct conf_device conf_device_init[];
extern struct conf_service conf_service_init[];
#endif
