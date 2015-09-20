/*
 * Copyright (c) 1980, 1993
 *  The Regents of the University of California.  All rights reserved.
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
 *  This product includes software developed by the University of
 *  California, Berkeley and its contributors.
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
 *
 *  @(#)config.h    8.1 (Berkeley) 6/6/93
 */

/*
 * Config.
 */
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define NODEV   ((dev_t)-1)

struct file_list {
    struct  file_list *f_next;
    char    *f_fn;          /* the name */
    u_char  f_type;         /* see below */
    u_char  f_flags;        /* see below */
    char    *f_special;     /* special make rule if present */
    char    *f_needs;
    /*
     * Random values:
     *  swap space parameters for swap areas
     *  root device, etc. for system specifications
     */
    union {
        struct {            /* when swap specification */
            dev_t   fuw_swapdev;
            int     fuw_swapsize;
            int     fuw_swapflag;
        } fuw;
        struct {            /* when system specification */
            dev_t   fus_rootdev;
            dev_t   fus_dumpdev;
        } fus;
        struct {            /* when component dev specification */
            dev_t   fup_compdev;
            int     fup_compinfo;
        } fup;
    } fun;
#define f_swapdev   fun.fuw.fuw_swapdev
#define f_swapsize  fun.fuw.fuw_swapsize
#define f_swapflag  fun.fuw.fuw_swapflag
#define f_rootdev   fun.fus.fus_rootdev
#define f_dumpdev   fun.fus.fus_dumpdev
#define f_compdev   fun.fup.fup_compdev
#define f_compinfo  fun.fup.fup_compinfo
};

/*
 * Types.
 */
#define NORMAL      1
#define INVISIBLE   2
#define SYSTEMSPEC  3
#define SWAPSPEC    4

struct  idlst {
    char    *id;
    struct  idlst *id_next;
};

struct device {
    int     d_type;             /* CONTROLLER, DEVICE, bus adaptor */
    struct  device *d_conn;     /* what it is connected to */
    char    *d_name;            /* name of device (e.g. rk11) */
    struct  idlst *d_vec;       /* interrupt vectors */
    int     d_pri;              /* interrupt priority */
    int     d_addr;             /* address of csr */
    int     d_unit;             /* unit number */
    int     d_drive;            /* drive number */
    int     d_slave;            /* slave number */
#define QUES    -1              /* -1 means '?' */
#define UNKNOWN -2              /* -2 means not set yet */
    int     d_flags;            /* flags for device init */
    char    *d_port;            /* io port base manifest constant */
    char    *d_mask;            /* interrupt mask */
    int     d_maddr;            /* io memory base */
    int     d_msize;            /* io memory size */
    int     d_drq;              /* DMA request  */
    int     d_irq;              /* interrupt request  */
    struct  device *d_next;     /* Next one in list */
#define MAXPINS 32              /* max number of pins */
    short   d_pins[MAXPINS];    /* pins assigned */
    int     d_npins;            /* pin count */
};

struct config {
    char    *c_dev;
    char    *s_sysname;
};

/*
 * Config has a global notion of which architecture is being used.
 */
int     arch;
char    *archname;
#define ARCH_PIC32      1

/*
 * For each architecture, a set of CPU's may be specified as supported.
 * These and the options (below) are put in the C flags in the makefile.
 */
struct cputype {
    char    *cpu_name;
    struct  cputype *cpu_next;
} *cputype;

/*
 * A set of options may also be specified which are like CPU types,
 * but which may also specify values for the options.
 * A separate set of options may be defined for make-style options.
 */
struct opt {
    char    *op_name;
    char    *op_value;
    struct  opt *op_next;
} *opt, *mkopt;

/*
 * Mapping of signal names to pins.
 */
struct signal {
    char    *sig_name;
    int     sig_pin;
    int     sig_invert;
    struct  signal *sig_next;
} *siglist;

char    *board;
char    *ldscript;

int do_trace;

struct  device *dtab;

char    errbuf[80];
int     yyline;

struct  file_list *ftab, *conf_list, **confp, *comp_list, **compp;

int     zone, hadtz;
int     dst;
int     debugging;

int     maxusers;

#define eq(a,b) (!strcmp(a,b))

char    *get_word(FILE *);
char    *get_quoted_word(FILE *);
char    *raise(char *);
dev_t   nametodev(char *, int);
char    *devtoname(dev_t);
void    init_dev(struct device *);
int     yyparse(void);
void    pic32_ioconf(void);
void    makefile(void);
void    headers(void);
void    swapconf(void);
