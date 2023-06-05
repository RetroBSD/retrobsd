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
 */

/*
 * Build a swap configuration file.
 */
#include "config.h"

#include <unistd.h>
#include <ctype.h>

void swapconf()
{
    register struct file_list *fl;
    struct file_list *do_swap();

    fl = conf_list;
    while (fl) {
        if (fl->f_type != SYSTEMSPEC) {
            fl = fl->f_next;
            continue;
        }
        fl = do_swap(fl);
    }
}

struct file_list *
do_swap(fl)
    register struct file_list *fl;
{
    FILE *fp;
    char  swapname[80];
    register struct file_list *swap;

    if (eq(fl->f_fn, "generic")) {
        fl = fl->f_next;
        return (fl->f_next);
    }
    (void) sprintf(swapname, "swap%s.c", fl->f_fn);
    fp = fopen(swapname, "w");
    if (fp == 0) {
        perror(swapname);
        exit(1);
    }
    fprintf(fp, "#include \"sys/param.h\"\n");
    fprintf(fp, "#include \"sys/conf.h\"\n");
    fprintf(fp, "\n");
    /*
     * If there aren't any swap devices
     * specified, just return, the error
     * has already been noted.
     */
    swap = fl->f_next;
    if (swap == 0 || swap->f_type != SWAPSPEC) {
        (void) unlink(swapname);
        fclose(fp);
        return (swap);
    }
    fprintf(fp, "dev_t\trootdev = makedev(%d, %d);\t/* %s */\n",
        major(fl->f_rootdev), minor(fl->f_rootdev),
        devtoname(fl->f_rootdev));
    fprintf(fp, "dev_t\tdumpdev = makedev(%d, %d);\t/* %s */\n",
        major(fl->f_dumpdev), minor(fl->f_dumpdev),
        devtoname(fl->f_dumpdev));
#if 1
    /* Only one swap device is supported. */
    fprintf(fp, "dev_t\tswapdev = makedev(%d, %d);\t/* %s */\n",
        major(swap->f_swapdev), minor(swap->f_swapdev),
        devtoname(swap->f_swapdev));
#else
    fprintf(fp, "\n");
    fprintf(fp, "struct\tswdevt swdevt[] = {\n");
    do {
        int dev = swap->f_swapdev;
        fprintf(fp, "\t{ makedev(%d, %d), %d, %d },\t/* %s */\n",
            major(dev), minor(dev), swap->f_swapflag,
            swap->f_swapsize, swap->f_fn);
        swap = swap->f_next;
    } while (swap && swap->f_type == SWAPSPEC);
    fprintf(fp, "\t{ NODEV, 0, 0 }\n");
    fprintf(fp, "};\n");
#endif
    fclose(fp);
    return (swap);
}

static  int devtablenotread = 1;

static  struct devdescription {
    char    *dev_name;
    int     dev_major;
    struct  devdescription *dev_next;
} *devtable;

void initdevtable()
{
    char buf[BUFSIZ], name[BUFSIZ], *p;
    int maj;
    register struct devdescription **dp = &devtable;
    FILE *fp;

    (void) sprintf(buf, "../devices.kconf");
    fp = fopen(buf, "r");
    if (fp == NULL) {
        fprintf(stderr, "config: can't open %s\n", buf);
        exit(1);
    }
    while (fgets(buf, sizeof(buf), fp)) {
        for (p=buf; *p; p++)
            if (*p != ' ' && *p != '\t')
                break;
        if (*p == '#' || *p == '\n' || *p == '\r')
            continue;
        if (sscanf(p, "%s %d", name, &maj) != 2) {
            fprintf(stderr, "../devices.kconf: unrecognized line %s\n", buf);
            exit(1);
        }
        *dp = (struct devdescription *)malloc(sizeof (**dp));
        (*dp)->dev_name = strdup(name);
        (*dp)->dev_major = maj;
        dp = &(*dp)->dev_next;
    }
    *dp = 0;
    fclose(fp);
    devtablenotread = 0;
}

/*
 * Given a device name specification figure out:
 *  major device number
 *  partition
 *  device name
 *  unit number
 * This is a hack, but the system still thinks in
 * terms of major/minor instead of string names.
 */
dev_t
nametodev(name, defunit)
    char *name;
    int defunit;
{
    char *cp, partition;
    int unit;
    register struct devdescription *dp;

    cp = name;
    if (cp == 0) {
        fprintf(stderr, "config: internal error, nametodev\n");
        exit(1);
    }
    while (*cp && !isdigit(*cp))
        cp++;
    unit = *cp ? atoi(cp) : defunit;
    if (unit < 0 || unit > 31) {
        fprintf(stderr,
            "config: %s: invalid device specification, unit out of range\n",
            name);
        unit = defunit;         /* carry on more checking */
    }
    if (*cp) {
        *cp++ = '\0';
        while (*cp && isdigit(*cp))
            cp++;
    }
    partition = *cp ? *cp : '`';
    if (partition < '`' || partition > 'd') {
        fprintf(stderr,
            "config: %c: invalid device specification, bad partition\n", *cp);
        partition = 'a';   /* carry on */
    }
    if (devtablenotread)
        initdevtable();
    for (dp = devtable; dp; dp = dp->dev_next)
        if (eq(name, dp->dev_name))
            break;
    if (dp == 0) {
        fprintf(stderr, "config: %s: unknown device\n", name);
        return (NODEV);
    }
    return (makedev(dp->dev_major, (unit << 3) + (partition - '`')));
}

char *
devtoname(dev)
    dev_t dev;
{
    char buf[80];
    register struct devdescription *dp;

    if (devtablenotread)
        initdevtable();
    for (dp = devtable; dp; dp = dp->dev_next)
        if (major(dev) == dp->dev_major)
            break;
    if (dp == 0)
        dp = devtable;

    if (minor(dev) == 0)
        sprintf(buf, "%s%d", dp->dev_name, minor(dev) >> 3);
    else
        sprintf(buf, "%s%d%c", dp->dev_name, minor(dev) >> 3,
            (minor(dev) & 07) + 'a' - 1);
    return strdup(buf);
}
