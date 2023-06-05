%union {
    char    *str;
    int     val;
    struct  file_list *file;
    struct  idlst *lst;
}

%token  AND
%token  ANY
%token  ARCHITECTURE
%token  AT
%token  BOARD
%token  COMMA
%token  CONFIG
%token  CONTROLLER
%token  CPU
%token  CSR
%token  DEVICE
%token  DRIVE
%token  DST
%token  DUMPS
%token  EQUALS
%token  FLAGS
%token  HZ
%token  INVERT
%token  LDSCRIPT
%token  MAJOR
%token  MAXUSERS
%token  MINOR
%token  MINUS
%token  ON
%token  OPTIONS
%token  MAKEOPTIONS
%token  PINS
%token  PRIORITY
%token  SERVICE
%token  SIGNAL
%token  ROOT
%token  SEMICOLON
%token  SEQUENTIAL
%token  SIZE
%token  SWAP
%token  TIMEZONE
%token  TRACE
%token  VECTOR

%token  <str>   ID
%token  <val>   NUMBER
%token  <val>   FPNUMBER
%token  <val>   PIN

%type   <str>   Save_id
%type   <str>   Opt_value
%type   <str>   Dev
%type   <lst>   Id_list
%type   <val>   optional_size
%type   <val>   optional_sflag
%type   <val>   optional_invert
%type   <str>   device_name
%type   <val>   major_minor
%type   <val>   root_device_spec
%type   <val>   dump_device_spec
%type   <file>  swap_device_spec

%{

/*
 * Copyright (c) 1988, 1993
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
 *  @(#)config.y    8.1 (Berkeley) 6/6/93
 */

#include "config.h"
#include <ctype.h>
#include <stdio.h>

struct  device cur;
struct  device *curp = 0;
char    *temp_id;
char    *val_id;

int yylex(void);
int finddev(dev_t dev);
void deverror(char *systemname, char *devtype);
int alreadychecked(dev_t dev, dev_t list[], dev_t *last);

%}
%%
Configuration:
    Many_specs
        = { verifysystemspecs(); }
    ;

Many_specs:
    Many_specs Spec
        |
    /* lambda */
    ;

Spec:
    Device_spec SEMICOLON
        = { newdev(&cur); }
        |
    Config_spec SEMICOLON
        |
    TRACE SEMICOLON
        = { do_trace = !do_trace; }
        |
    SEMICOLON
        |
    error SEMICOLON
    ;

Config_spec:
    ARCHITECTURE Save_id
        = {
            if (strcmp($2, "pic32") == 0) {
                arch = ARCH_PIC32;
                archname = "pic32";
            } else
                yyerror("Unknown architecture");
        }
        |
    CPU Save_id
        = {
            struct cputype *cp =
                (struct cputype *)malloc(sizeof (struct cputype));
            cp->cpu_name = strdup($2);
            cp->cpu_next = cputype;
            cputype = cp;
            free(temp_id);
        }
        |
    OPTIONS Opt_list
        |
    MAKEOPTIONS Mkopt_list
        |
    BOARD ID
        = { board = strdup($2); }
        |
    LDSCRIPT ID
        = { ldscript = strdup($2); }
        |
    SIGNAL ID PINS PIN optional_invert
        = {
            struct signal *s = (struct signal*) malloc(sizeof(struct signal));
            s->sig_name = strdup($2);
            s->sig_next = siglist;
            s->sig_pin = $4;
            s->sig_invert = $5;
            siglist = s;
        }
        |
    System_spec
        |
    TIMEZONE NUMBER
        = { zone = 60 * $2; check_tz(); }
        |
    TIMEZONE NUMBER DST NUMBER
        = { zone = 60 * $2; dst = $4; check_tz(); }
        |
    TIMEZONE NUMBER DST
        = { zone = 60 * $2; dst = 1; check_tz(); }
        |
    TIMEZONE FPNUMBER
        = { zone = $2; check_tz(); }
        |
    TIMEZONE FPNUMBER DST NUMBER
        = { zone = $2; dst = $4; check_tz(); }
        |
    TIMEZONE FPNUMBER DST
        = { zone = $2; dst = 1; check_tz(); }
        |
    TIMEZONE MINUS NUMBER
        = { zone = -60 * $3; check_tz(); }
        |
    TIMEZONE MINUS NUMBER DST NUMBER
        = { zone = -60 * $3; dst = $5; check_tz(); }
        |
    TIMEZONE MINUS NUMBER DST
        = { zone = -60 * $3; dst = 1; check_tz(); }
        |
    TIMEZONE MINUS FPNUMBER
        = { zone = -$3; check_tz(); }
        |
    TIMEZONE MINUS FPNUMBER DST NUMBER
        = { zone = -$3; dst = $5; check_tz(); }
        |
    TIMEZONE MINUS FPNUMBER DST
        = { zone = -$3; dst = 1; check_tz(); }
        |
    MAXUSERS NUMBER
        = { maxusers = $2; }
    ;

System_spec:
    System_id System_parameter_list
        = { checksystemspec(*confp); }
    ;

System_id:
    CONFIG Save_id
        = { mkconf($2); }
    ;

System_parameter_list:
    System_parameter_list System_parameter
        |
    System_parameter
    ;

System_parameter:
    swap_spec
        |
    root_spec
        |
    dump_spec
    ;

swap_spec:
    SWAP optional_on swap_device_list
    ;

swap_device_list:
    swap_device_list AND swap_device
        |
    swap_device
    ;

swap_device:
    swap_device_spec optional_size optional_sflag
        = { mkswap(*confp, $1, $2, $3); }
    ;

swap_device_spec:
    device_name
        = {
            struct file_list *fl = newflist(SWAPSPEC);

            if (eq($1, "generic"))
                fl->f_fn = $1;
            else {
                fl->f_swapdev = nametodev($1, 0);
                fl->f_fn = devtoname(fl->f_swapdev);
            }
            $$ = fl;
        }
        |
    major_minor
        = {
            struct file_list *fl = newflist(SWAPSPEC);

            fl->f_swapdev = $1;
            fl->f_fn = devtoname($1);
            $$ = fl;
        }
    ;

root_spec:
    ROOT optional_on root_device_spec
        = {
            struct file_list *fl = *confp;

            if (fl && fl->f_rootdev != NODEV)
                yyerror("extraneous root device specification");
            else
                fl->f_rootdev = $3;
        }
    ;

root_device_spec:
    device_name
        = { $$ = nametodev($1, 0); }
        |
    major_minor
    ;

dump_spec:
    DUMPS optional_on dump_device_spec
        = {
            struct file_list *fl = *confp;

            if (fl && fl->f_dumpdev != NODEV)
                yyerror("extraneous dump device specification");
            else
                fl->f_dumpdev = $3;
        }
    ;

dump_device_spec:
    device_name
        = { $$ = nametodev($1, 0); }
        |
    major_minor
    ;

major_minor:
    MAJOR NUMBER MINOR NUMBER
        = { $$ = makedev($2, $4); }
    ;

optional_on:
    ON
        |
    /* empty */
    ;

optional_size:
    SIZE NUMBER
        = { $$ = $2; }
        |
    /* empty */
        = { $$ = 0; }
    ;

optional_sflag:
    SEQUENTIAL
        = { $$ = 2; }
        |
    /* empty */
        = { $$ = 0; }
    ;

optional_invert:
    INVERT
        = { $$ = 1; }
        |
    /* empty */
        = { $$ = 0; }
    ;

device_name:
    Save_id
        = { $$ = $1; }
        |
    Save_id NUMBER
        = {
            char buf[80];

            (void) sprintf(buf, "%s%d", $1, $2);
            $$ = strdup(buf);
            free($1);
        }
        |
    Save_id NUMBER ID
        = {
            char buf[80];

            (void) sprintf(buf, "%s%d%s", $1, $2, $3);
            $$ = strdup(buf);
            free($1);
        }
    ;

Opt_list:
    Opt_list COMMA Option
        |
    Option
    ;

Option:
    Save_id
        = {
            struct opt *op = (struct opt *)malloc(sizeof (struct opt));
            op->op_name = strdup($1);
            op->op_next = opt;
            op->op_value = 0;
            opt = op;
            free(temp_id);
        }
        |
    Save_id EQUALS Opt_value
        = {
            struct opt *op = (struct opt *)malloc(sizeof (struct opt));
            op->op_name = strdup($1);
            op->op_next = opt;
            op->op_value = strdup($3);
            opt = op;
            free(temp_id);
            free(val_id);
        }
    ;

Opt_value:
    ID
        = { $$ = val_id = strdup($1); }
        |
    NUMBER
        = {
            char nb[16];
            (void) sprintf(nb, "%d", $1);
            $$ = val_id = strdup(nb);
        }
    ;

Save_id:
    ID
        = { $$ = temp_id = strdup($1); }
    ;

Mkopt_list:
    Mkopt_list COMMA Mkoption
        |
    Mkoption
    ;

Mkoption:
    Save_id EQUALS Opt_value
        = {
            struct opt *op = (struct opt *)malloc(sizeof (struct opt));
            op->op_name = strdup($1);
            op->op_next = mkopt;
            op->op_value = strdup($3);
            mkopt = op;
            free(temp_id);
            free(val_id);
        }
    ;

Dev:
    ID
        = { $$ = strdup($1); }
    ;

Device_spec:
    DEVICE Dev_name Dev_info Int_spec
        = { cur.d_type = DEVICE; }
        |
    CONTROLLER Dev_name Dev_info Int_spec
        = { cur.d_type = CONTROLLER; }
        |
    SERVICE Init_dev Dev
        = {
            cur.d_name = $3;
            cur.d_type = SERVICE;
        }
        |
    SERVICE Init_dev Dev NUMBER
        = {
            cur.d_name = $3;
            cur.d_type = SERVICE;
            cur.d_slave = $4;
        }
    ;

Dev_name:
    Init_dev Dev NUMBER
        = {
            cur.d_name = $2;
            cur.d_unit = $3;
        }
        |
    Init_dev Dev
        = {
            cur.d_name = $2;
        }
    ;

Init_dev:
    /* lambda */
        = { init_dev(&cur); }
    ;

Dev_info:
    Con_info Info_list
        |
    Info_list
        |
    /* lambda */
    ;

Con_info:
    AT Dev NUMBER
        = { cur.d_conn = connect($2, $3); }
    ;

Info_list:
    Info_list Info
        |
    /* lambda */
        ;

Info:
    CSR NUMBER
        = { cur.d_addr = $2; }
        |
    DRIVE NUMBER
        = { cur.d_drive = $2; }
        |
    FLAGS NUMBER
        = { cur.d_flags = $2; }
        |
    PINS Pin_list
    ;

Int_spec:
    VECTOR Id_list
        = { cur.d_vec = $2; }
        |
    PRIORITY NUMBER
        = { cur.d_pri = $2; }
        |
    /* lambda */
    ;

Id_list:
    Save_id
        = {
            struct idlst *a = (struct idlst *)malloc(sizeof(struct idlst));
            a->id = $1; a->id_next = 0; $$ = a;
        }
        |
    Save_id Id_list =
        {
            struct idlst *a = (struct idlst *)malloc(sizeof(struct idlst));
            a->id = $1; a->id_next = $2; $$ = a;
        }
    ;

Pin_list:
    PIN
        = { cur.d_pins[cur.d_npins++] = $1; }
        |
    PIN COMMA Pin_list
        = { cur.d_pins[cur.d_npins++] = $1; }
    ;
%%

void yyerror(s)
    char *s;
{
    fprintf(stderr, "config: line %d: %s\n", yyline + 1, s);
}

/*
 * add a device to the list of devices
 */
void newdev(dp)
    register struct device *dp;
{
    register struct device *np;

    np = (struct device *) malloc(sizeof *np);
    *np = *dp;
    np->d_next = 0;
    if (curp == 0)
        dtab = np;
    else
        curp->d_next = np;
    curp = np;
}

/*
 * note that a configuration should be made
 */
void mkconf(sysname)
    char *sysname;
{
    register struct file_list *fl, **flp;

    fl = (struct file_list *) malloc(sizeof *fl);
    fl->f_type = SYSTEMSPEC;
    fl->f_needs = sysname;
    fl->f_rootdev = NODEV;
    fl->f_dumpdev = NODEV;
    fl->f_fn = 0;
    fl->f_next = 0;
    for (flp = confp; *flp; flp = &(*flp)->f_next)
        ;
    *flp = fl;
    confp = flp;
}

struct file_list *
newflist(ftype)
    u_char ftype;
{
    struct file_list *fl = (struct file_list *)malloc(sizeof (*fl));

    fl->f_type = ftype;
    fl->f_next = 0;
    fl->f_swapdev = NODEV;
    fl->f_swapsize = 0;
    fl->f_needs = 0;
    fl->f_fn = 0;
    return (fl);
}

/*
 * Add a swap device to the system's configuration
 */
void mkswap(system, fl, size, flag)
    struct file_list *system, *fl;
    int size, flag;
{
    register struct file_list **flp;

    if (system == 0 || system->f_type != SYSTEMSPEC) {
        yyerror("\"swap\" spec precedes \"config\" specification");
        return;
    }
    if (size < 0) {
        yyerror("illegal swap partition size");
        return;
    }
    /*
     * Append swap description to the end of the list.
     */
    flp = &system->f_next;
    for (; *flp && (*flp)->f_type == SWAPSPEC; flp = &(*flp)->f_next)
        ;
    fl->f_next = *flp;
    *flp = fl;
    fl->f_swapsize = size;
    fl->f_swapflag = flag;

    /*
     * If first swap device for this system,
     * set up f_fn field to insure swap
     * files are created with unique names.
     */
    if (system->f_fn)
        return;

    if (eq(fl->f_fn, "generic"))
        system->f_fn = strdup(fl->f_fn);
    else
        system->f_fn = strdup(system->f_needs);
}

/*
 * find the pointer to connect to the given device and number.
 * returns 0 if no such device and prints an error message
 */
struct device *
connect(dev, num)
    register char *dev;
    register int num;
{
    register struct device *dp;
    struct device *huhcon();

    if (num == QUES)
        return (huhcon(dev));
    for (dp = dtab; dp != 0; dp = dp->d_next) {
        if ((num != dp->d_unit) || !eq(dev, dp->d_name))
            continue;
        if (dp->d_type != CONTROLLER) {
            (void) sprintf(errbuf,
                "%s connected to non-controller", dev);
            yyerror(errbuf);
            return (0);
        }
        return (dp);
    }
    (void) sprintf(errbuf, "%s %d not defined", dev, num);
    yyerror(errbuf);
    return (0);
}

/*
 * connect to an unspecific thing
 */
struct device *
huhcon(dev)
    register char *dev;
{
    register struct device *dp, *dcp;
    struct device rdev;
    int oldtype;

    /*
     * First make certain that there are some of these to wildcard on
     */
    for (dp = dtab; dp != 0; dp = dp->d_next)
        if (eq(dp->d_name, dev))
            break;
    if (dp == 0) {
        (void) sprintf(errbuf, "no %s's to wildcard", dev);
        yyerror(errbuf);
        return (0);
    }
    oldtype = dp->d_type;
    dcp = dp->d_conn;

    /*
     * Now see if there is already a wildcard entry for this device
     * (e.g. Search for a "uba ?")
     */
    for (; dp != 0; dp = dp->d_next)
        if (eq(dev, dp->d_name) && dp->d_unit == -1)
            break;
    /*
     * If there isn't, make one because everything needs to be connected
     * to something.
     */
    if (dp == 0) {
        dp = &rdev;
        init_dev(dp);
        dp->d_unit = QUES;
        dp->d_name = strdup(dev);
        dp->d_type = oldtype;
        newdev(dp);
        dp = curp;

        /*
         * Connect it to the same thing that other similar things are
         * connected to, but make sure it is a wildcard unit
         * (e.g. up connected to sc ?, here we make connect sc? to a
         * uba?).  If other things like this
         * aren't connected to anything, then make the same
         * connection, else call ourself to connect to another
         * unspecific device.
         */
        if (dcp == 0)
            dp->d_conn = dcp;
        else
            dp->d_conn = connect(dcp->d_name, QUES);
    }
    return (dp);
}

void init_dev(dp)
    register struct device *dp;
{
    dp->d_name = "OHNO!!!";
    dp->d_type = DEVICE;
    dp->d_conn = 0;
    dp->d_vec = 0;
    dp->d_addr = dp->d_flags;
    dp->d_pri = -1;
    dp->d_slave = dp->d_drive = dp->d_unit = UNKNOWN;
    dp->d_port = (char *)0;
    dp->d_irq = -1;
    dp->d_drq = -1;
    dp->d_maddr = 0;
    dp->d_msize = 0;
    dp->d_npins = 0;
    dp->d_flags = 0;
    dp->d_mask = "null";
}

/*
 * make certain that this is a reasonable type of thing to connect to a nexus
 */
void check_nexus(dev, num)
    register struct device *dev;
    int num;
{
    switch (arch) {

    case ARCH_PIC32:
        break;
    }
}

/*
 * Check the timezone to make certain it is sensible
 */
void check_tz()
{
    if (abs(zone) > 12 * 60)
        yyerror("timezone is unreasonable");
    else
        hadtz = 1;
}

/*
 * Check system specification and apply defaulting
 * rules on root, argument, dump, and swap devices.
 */
void checksystemspec(fl)
    register struct file_list *fl;
{
    char buf[BUFSIZ];
    register struct file_list *swap;
    int generic;

    if (fl == 0 || fl->f_type != SYSTEMSPEC) {
        yyerror("internal error, bad system specification");
        exit(1);
    }
    swap = fl->f_next;
    generic = swap && swap->f_type == SWAPSPEC && eq(swap->f_fn, "generic");
    if (fl->f_rootdev == NODEV && !generic) {
        yyerror("no root device specified");
        exit(1);
    }

    /*
     * Default swap area to be in 'b' partition of root's
     * device.  If root specified to be other than on 'a'
     * partition, give warning, something probably amiss.
     */
    if (swap == 0 || swap->f_type != SWAPSPEC) {
        dev_t dev;

        swap = newflist(SWAPSPEC);
        dev = fl->f_rootdev;
        if (minor(dev) & 07) {
            sprintf(buf,
                "Warning, swap defaulted to 'b' partition with root on '%c' partition",
                (minor(dev) & 07) + 'a' - 1);
            yyerror(buf);
        }
        swap->f_swapdev =
           makedev(major(dev), (minor(dev) &~ 07) | ('b' - 'a' + 1));
        swap->f_fn = devtoname(swap->f_swapdev);
        mkswap(fl, swap, 0, 0);
    }

    /*
     * Make sure a generic swap isn't specified, along with
     * other stuff (user must really be confused).
     */
    if (generic) {
        if (fl->f_rootdev != NODEV)
            yyerror("root device specified with generic swap");
        if (fl->f_dumpdev != NODEV)
            yyerror("dump device specified with generic swap");
        return;
    }

    /*
     * Default dump device and warn if place is not a
     * swap area.
     */
    if (fl->f_dumpdev == NODEV)
        fl->f_dumpdev = swap->f_swapdev;
    if (fl->f_dumpdev != swap->f_swapdev) {
        struct file_list *p = swap->f_next;

        for (; p && p->f_type == SWAPSPEC; p = p->f_next)
            if (fl->f_dumpdev == p->f_swapdev)
                return;
        (void) sprintf(buf,
            "Warning: dump device is not a swap partition");
        yyerror(buf);
    }
}

/*
 * Verify all devices specified in the system specification
 * are present in the device specifications.
 */
void verifysystemspecs()
{
    register struct file_list *fl;
    dev_t checked[50], *verifyswap();
    register dev_t *pchecked = checked;

    for (fl = conf_list; fl; fl = fl->f_next) {
        if (fl->f_type != SYSTEMSPEC)
            continue;
        if (!finddev(fl->f_rootdev))
            deverror(fl->f_needs, "root");
        *pchecked++ = fl->f_rootdev;
        pchecked = verifyswap(fl->f_next, checked, pchecked);

#define samedev(dev1, dev2) \
    ((minor(dev1) &~ 07) != (minor(dev2) &~ 07))

        if (!alreadychecked(fl->f_dumpdev, checked, pchecked)) {
            if (!finddev(fl->f_dumpdev))
                deverror(fl->f_needs, "dump");
            *pchecked++ = fl->f_dumpdev;
        }
    }
}

/*
 * Do as above, but for swap devices.
 */
dev_t *
verifyswap(fl, checked, pchecked)
    register struct file_list *fl;
    dev_t checked[];
    register dev_t *pchecked;
{
    for (;fl && fl->f_type == SWAPSPEC; fl = fl->f_next) {
        if (eq(fl->f_fn, "generic"))
            continue;
        if (alreadychecked(fl->f_swapdev, checked, pchecked))
            continue;
        if (!finddev(fl->f_swapdev))
            fprintf(stderr,
               "config: swap device %s not configured", fl->f_fn);
        *pchecked++ = fl->f_swapdev;
    }
    return (pchecked);
}

/*
 * Has a device already been checked
 * for it's existence in the configuration?
 */
int alreadychecked(dev, list, last)
    dev_t dev, list[];
    register dev_t *last;
{
    register dev_t *p;

    for (p = list; p < last; p++)
        if (samedev(*p, dev))
            return (1);
    return (0);
}

void deverror(systemname, devtype)
    char *systemname, *devtype;
{

    fprintf(stderr, "config: %s: %s device not configured\n",
        systemname, devtype);
}

/*
 * Look for the device in the list of
 * configured hardware devices.  Must
 * take into account stuff wildcarded.
 */
/*ARGSUSED*/
int finddev(dev)
    dev_t dev;
{
    /* punt on this right now */
    return (1);
}
