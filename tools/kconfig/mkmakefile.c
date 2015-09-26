/*
 * Copyright (c) 1993, 19801990
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
 * Build the makefile for the system, from
 * the information in the files files and the
 * additional files for the architecture being compiled to.
 */
#include <ctype.h>
#include "y.tab.h"
#include "config.h"

#define next_word(fp, wd) \
    { register char *word = get_word(fp); \
      if (word == (char *)EOF) \
        return; \
      else \
        wd = word; \
    }
#define next_quoted_word(fp, wd) \
    { register char *word = get_quoted_word(fp); \
      if (word == (char *)EOF) \
        return; \
      else \
        wd = word; \
    }

static  struct file_list *fcur;
char *tail();

/*
 * Lookup a file, by name.
 */
struct file_list *
fl_lookup(file)
    register char *file;
{
    register struct file_list *fp;

    for (fp = ftab ; fp != 0; fp = fp->f_next) {
        if (eq(fp->f_fn, file))
            return (fp);
    }
    return (0);
}

/*
 * Lookup a file, by final component name.
 */
struct file_list *
fltail_lookup(file)
    register char *file;
{
    register struct file_list *fp;

    for (fp = ftab ; fp != 0; fp = fp->f_next) {
        if (eq(tail(fp->f_fn), tail(file)))
            return (fp);
    }
    return (0);
}

/*
 * Make a new file list entry
 */
struct file_list *
new_fent()
{
    register struct file_list *fp;

    fp = (struct file_list *) malloc(sizeof *fp);
    bzero(fp, sizeof *fp);
    if (fcur == 0)
        fcur = ftab = fp;
    else
        fcur->f_next = fp;
    fcur = fp;
    return (fp);
}

int opteq(cp, dp)
    char *cp, *dp;
{
    char c, d;

    for (; ; cp++, dp++) {
        if (*cp != *dp) {
            c = isupper(*cp) ? tolower(*cp) : *cp;
            d = isupper(*dp) ? tolower(*dp) : *dp;
            if (c != d)
                return (0);
        }
        if (*cp == 0)
            return (1);
    }
}

/*
 * Read in the information about files used in making the system.
 * Store it in the ftab linked list.
 */
void read_files()
{
    FILE *fp;
    register struct file_list *tp, *pf;
    register struct device *dp;
    struct device *save_dp = 0;
    register struct opt *op;
    char *wd, *this, *needs, *special;
    char fname[32];
    int nreqs, first = 1, isdup, std, filetype;

    ftab = 0;
    (void) strcpy(fname, "../files.kconf");
    fp = fopen(fname, "r");
    if (fp == 0) {
        perror(fname);
        exit(1);
    }
next:
    /*
     * filename [ standard | optional ] [ dev* ] [ compile-with "compile rule" ]
     */
    wd = get_word(fp);
    if (wd == (char *)EOF) {
eof:    (void) fclose(fp);
        if (first == 1) {
            (void) sprintf(fname, "files.%s", raise(board));
            first++;
            fp = fopen(fname, "r");
            if (fp != 0)
                goto next;
        }
        return;
    }
    if (wd == 0)
        goto next;
    if (*wd == '#') {
        /* Skip comment. */
        for (;;) {
            wd = get_word(fp);
            if (wd == 0)
                goto next;
            if (wd == (char *)EOF)
                goto eof;
        }
    }
    this = strdup(wd);
    next_word(fp, wd);
    if (wd == 0) {
        printf("%s: No type for %s.\n",
            fname, this);
        exit(1);
    }
    if ((pf = fl_lookup(this)) && (pf->f_type != INVISIBLE || pf->f_flags))
        isdup = 1;
    else
        isdup = 0;
    tp = 0;
    if (first == 3 && (tp = fltail_lookup(this)) != 0)
        printf("%s: Local file %s overrides %s.\n",
            fname, this, tp->f_fn);
    nreqs = 0;
    special = 0;
    needs = 0;
    std = 0;
    filetype = NORMAL;
    if (eq(wd, "standard"))
        std = 1;
    else if (!eq(wd, "optional")) {
        printf("%s: %s must be optional or standard\n", fname, this);
        exit(1);
    }
nextparam:
    next_word(fp, wd);
    if (wd == 0)
        goto doneparam;
    if (eq(wd, "compile-with")) {
        next_quoted_word(fp, wd);
        if (wd == 0) {
            printf("%s: missing compile command string.\n",
                   fname);
            exit(1);
        }
        special = strdup(wd);
        goto nextparam;
    }
    nreqs++;
    if (needs == 0 && nreqs == 1)
        needs = strdup(wd);
    if (isdup)
        goto invis;
    for (dp = dtab; dp != 0; save_dp = dp, dp = dp->d_next)
        if (eq(dp->d_name, wd)) {
            if (std && dp->d_type == SERVICE &&
                dp->d_slave <= 0)
                dp->d_slave = 1;
            goto nextparam;
        }
    if (std) {
        dp = (struct device *) malloc(sizeof *dp);
        init_dev(dp);
        dp->d_name = strdup(wd);
        dp->d_type = SERVICE;
        dp->d_slave = 1;
        if (save_dp)
            save_dp->d_next = dp;
        goto nextparam;
    }
    for (op = opt; op != 0; op = op->op_next)
        if (op->op_value == 0 && opteq(op->op_name, wd)) {
            if (nreqs == 1) {
                free(needs);
                needs = 0;
            }
            goto nextparam;
        }
invis:
    while ((wd = get_word(fp)) != 0)
        ;
    if (tp == 0)
        tp = new_fent();
    tp->f_fn = this;
    tp->f_type = INVISIBLE;
    tp->f_needs = needs;
    tp->f_flags = isdup;
    tp->f_special = special;
    goto next;

doneparam:
    if (std == 0 && nreqs == 0) {
        printf("%s: what is %s optional on?\n",
            fname, this);
        exit(1);
    }

    if (wd) {
        printf("%s: syntax error describing %s\n",
            fname, this);
        exit(1);
    }
    if (tp == 0)
        tp = new_fent();
    tp->f_fn = this;
    tp->f_type = filetype;
    tp->f_flags = 0;
    tp->f_needs = needs;
    tp->f_special = special;
    if (pf && pf->f_type == INVISIBLE)
        pf->f_flags = 1;        /* mark as duplicate */
    goto next;
}

void do_objs(fp)
    FILE *fp;
{
    register struct file_list *tp, *fl;
    register int lpos, len;
    register char *cp, och, *sp;
    char swapname[32];

    fprintf(fp, "OBJS = ");
    lpos = 6;
    for (tp = ftab; tp != 0; tp = tp->f_next) {
        if (tp->f_type == INVISIBLE)
            continue;
        sp = tail(tp->f_fn);
        for (fl = conf_list; fl; fl = fl->f_next) {
            if (fl->f_type != SWAPSPEC)
                continue;
            (void) sprintf(swapname, "swap%s.c", fl->f_fn);
            if (eq(sp, swapname))
                goto cont;
        }
        cp = sp + (len = strlen(sp)) - 1;
        och = *cp;
        *cp = 'o';
        if (len + lpos > 72) {
            lpos = 8;
            fprintf(fp, "\\\n\t");
        }
        fprintf(fp, "%s ", sp);
        lpos += len + 1;
        *cp = och;
cont:
        ;
    }
    if (lpos != 8)
        putc('\n', fp);
}

void do_cfiles(fp)
    FILE *fp;
{
    register struct file_list *tp, *fl;
    register int lpos, len;
    char swapname[32];

    fputs("CFILES = ", fp);
    lpos = 8;
    for (tp = ftab; tp; tp = tp->f_next)
        if (tp->f_type != INVISIBLE) {
            len = strlen(tp->f_fn);
            if (tp->f_fn[len - 1] != 'c')
                continue;
            if ((len = 3 + len) + lpos > 72) {
                lpos = 8;
                fputs("\\\n\t", fp);
            }
            fprintf(fp, "$S/%s ", tp->f_fn);
            lpos += len + 1;
        }
    for (fl = conf_list; fl; fl = fl->f_next)
        if (fl->f_type == SYSTEMSPEC) {
            (void) sprintf(swapname, "swap%s.c", fl->f_fn);
            if ((len = 3 + strlen(swapname)) + lpos > 72) {
                lpos = 8;
                fputs("\\\n\t", fp);
            }
            if (eq(fl->f_fn, "generic"))
                fprintf(fp, "$A/%s/%s ",
                    archname, swapname);
            else
                fprintf(fp, "%s ", swapname);
            lpos += len + 1;
        }
    if (lpos != 8)
        putc('\n', fp);
}

/*
 * Create the makerules for each file
 * which is part of the system.
 */
void do_rules(f)
    FILE *f;
{
    register char *cp, *np, och;
    register struct file_list *ftp;
    char *special;

    for (ftp = ftab; ftp != 0; ftp = ftp->f_next) {
        if (ftp->f_type == INVISIBLE)
            continue;
        cp = (np = ftp->f_fn) + strlen(ftp->f_fn) - 1;
        och = *cp;
        *cp = '\0';
        if (och == 'o') {
            fprintf(f, "%so:\n\t-cp $S/%so .\n\n", tail(np), np);
            continue;
        }
        fprintf(f, "%so: $S/%s%c\n", tail(np), np, och);
        special = ftp->f_special;
        if (special == 0) {
            static char cmd[128];
            sprintf(cmd, "${COMPILE_%c}", toupper(och));
            special = cmd;
        }
        *cp = och;
        fprintf(f, "\t%s\n\n", special);
    }
}

/*
 * Create the load strings
 */
void do_load(f)
    register FILE *f;
{
    register struct file_list *fl;
    register int first;
    struct file_list *do_systemspec();

    for (first = 1, fl = conf_list; fl; first = 0)
        fl = fl->f_type == SYSTEMSPEC ?
            do_systemspec(f, fl, first) : fl->f_next;
    fputs("all:", f);
    for (fl = conf_list; fl; fl = fl->f_next)
        if (fl->f_type == SYSTEMSPEC)
            fprintf(f, " %s", fl->f_needs);
    putc('\n', f);
}

/*
 * Build the makefile from the skeleton
 */
void makefile()
{
    FILE *ifp, *ofp;
    char line[BUFSIZ];
    struct opt *op;
    struct signal *sig;
    struct cputype *cp;
    struct device *dp;

    read_files();
    strcpy(line, "../Makefile.kconf");
    //strcat(line, archname);
    ifp = fopen(line, "r");
    if (ifp == 0) {
        perror(line);
        exit(1);
    }
    ofp = fopen("Makefile", "w");
    if (ofp == 0) {
        perror("Makefile");
        exit(1);
    }
    fprintf(ofp, "PARAM = -D%s\n", raise(board));
    if (cputype == 0) {
        printf("cpu type must be specified\n");
        exit(1);
    }
    for (cp = cputype; cp; cp = cp->cpu_next) {
        fprintf(ofp, "PARAM += -D%s\n", cp->cpu_name);
    }
    for (dp = dtab; dp != 0; dp = dp->d_next) {
        if (dp->d_unit <= 0)
            fprintf(ofp, "PARAM += -D%s_ENABLED\n", raise(dp->d_name));
        else
            fprintf(ofp, "PARAM += -D%s%d_ENABLED\n", raise(dp->d_name), dp->d_unit);

        if (dp->d_type == SERVICE && dp->d_slave > 0)
            fprintf(ofp, "PARAM += -D%s_NUNITS=%d\n", raise(dp->d_name), dp->d_slave);
    }
    for (sig = siglist; sig; sig = sig->sig_next) {
        int bit = sig->sig_pin & 0xff;
        int port = sig->sig_pin >> 8;
        if (bit > 15 || port < 1 || port > 7) {
            printf("%s: invalid pin name R%c%u\n",
                sig->sig_name, 'A'+port-1, bit);
            exit(1);
        }
        fprintf(ofp, "PARAM += -D%s_PORT=TRIS%c -D%s_PIN=%d",
            sig->sig_name, 'A'+port-1, sig->sig_name, bit);
        if (sig->sig_invert)
            fprintf(ofp, " -D%s_INVERT", sig->sig_name);
        fprintf(ofp, "\n");
    }
    for (op = opt; op; op = op->op_next) {
        if (op->op_value)
            fprintf(ofp, "PARAM += -D%s=\"%s\"\n", op->op_name, op->op_value);
        else
            fprintf(ofp, "PARAM += -D%s\n", op->op_name);
    }
    if (hadtz) {
        fprintf(ofp, "PARAM += -DTIMEZONE=%d\n", zone);
        fprintf(ofp, "PARAM += -DDST=%d\n", dst);
    }
    if (maxusers > 0)
        fprintf(ofp, "PARAM += -DMAXUSERS=%d\n", maxusers);

    if (ldscript)
        fprintf(ofp, "LDSCRIPT = \"%s\"\n", ldscript);

    for (op = mkopt; op; op = op->op_next)
        fprintf(ofp, "%s = %s\n", op->op_name, op->op_value);

    if (debugging)
        fprintf(ofp, "DEBUG = -g\n");

    while (fgets(line, BUFSIZ, ifp) != 0) {
        if (*line != '%') {
            fprintf(ofp, "%s", line);
            continue;
        }
        if (eq(line, "%OBJS\n"))
            do_objs(ofp);
        else if (eq(line, "%CFILES\n"))
            do_cfiles(ofp);
        else if (eq(line, "%RULES\n"))
            do_rules(ofp);
        else if (eq(line, "%LOAD\n"))
            do_load(ofp);
        else
            fprintf(stderr,
                "Unknown %% construct in generic makefile: %s",
                line);
    }
    (void) fclose(ifp);
    (void) fclose(ofp);
}

char *
tail(fn)
    char *fn;
{
    register char *cp;

    cp = rindex(fn, '/');
    if (cp == 0)
        return (fn);
    return (cp+1);
}

void do_swapspec(f, name)
    FILE *f;
    register char *name;
{

    if (!eq(name, "generic"))
        fprintf(f, "swap%s.o: swap%s.c\n", name, name);
    else
        fprintf(f, "swapgeneric.o: $A/%s/swapgeneric.c\n", archname);
    fprintf(f, "\t${COMPILE_C}\n\n");
}

struct file_list *
do_systemspec(f, fl, first)
    FILE *f;
    register struct file_list *fl;
    int first;
{
    fprintf(f, "%s: %s.elf\n\n", fl->f_needs, fl->f_needs);

    fprintf(f, "%s.elf: ${SYSTEM_DEP} swap%s.o", fl->f_needs, fl->f_fn);
    // Don't use newvers target.
    // A preferred way is to run newvers.sh from SYSTEM_LD_HEAD macro.
    //if (first)
    //  fprintf(f, " newvers");
    fprintf(f, "\n\t${SYSTEM_LD_HEAD}\n");
    fprintf(f, "\t${SYSTEM_LD} swap%s.o\n", fl->f_fn);
    fprintf(f, "\t${SYSTEM_LD_TAIL}\n\n");
    do_swapspec(f, fl->f_fn);
    for (fl = fl->f_next; fl; fl = fl->f_next)
        if (fl->f_type != SWAPSPEC)
            break;
    return (fl);
}

/*
 * Convert a name to uppercase.
 * Return a pointer to a static buffer.
 */
char *
raise(str)
    register char *str;
{
    static char buf[100];
    register char *cp = buf;

    while (*str) {
        *cp++ = islower(*str) ? toupper(*str++) : *str++;
    }
    *cp = 0;
    return buf;
}
