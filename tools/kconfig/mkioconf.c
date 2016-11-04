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
#include "y.tab.h"
#include "config.h"

/*
 * build the ioconf.c file
 */
static void service_ioconf(fp)
    register FILE *fp;
{
    register struct device *dp;

    for (dp = dtab; dp != NULL; dp = dp->d_next) {
        if (dp->d_type == SERVICE)
            fprintf(fp, "void %sattach(int);\n",
                dp->d_name);
    }

    fprintf(fp, "\nstruct conf_service conf_service_init[] = {\n");
    for (dp = dtab; dp != NULL; dp = dp->d_next) {
        if (dp->d_type == SERVICE)
            fprintf(fp, "    { %sattach },\n", dp->d_name);
    }
    fprintf(fp, "    { 0 }\n};\n");
}

static char *wnum(int num)
{
    if (num == QUES || num == UNKNOWN)
        return ("?");
    sprintf(errbuf, "%d", num);
    return (errbuf);
}

#if ARCH_PIC32
void pic32_ioconf()
{
    register struct device *dp, *mp;
    FILE *fp;
    int i;

    fp = fopen("ioconf.c", "w");
    if (fp == 0) {
        perror("ioconf.c");
        exit(1);
    }
    fprintf(fp, "#include \"sys/types.h\"\n");
    fprintf(fp, "#include \"sys/kconfig.h\"\n\n");
    fprintf(fp, "#define C (char *)\n\n");

    /* print controller initialization structures */
    for (dp = dtab; dp != 0; dp = dp->d_next) {
        if (dp->d_type == SERVICE)
            continue;
        fprintf(fp, "extern struct driver %sdriver;\n", dp->d_name);
    }
    fprintf(fp, "\nstruct conf_ctlr conf_ctlr_init[] = {\n");
    fprintf(fp, "   /* driver,\t\tunit,\taddr,\t\tpri,\tflags */\n");
    for (dp = dtab; dp != 0; dp = dp->d_next) {
        if (dp->d_type != CONTROLLER)
            continue;
        if (dp->d_drive != UNKNOWN) {
            printf("can't specify drive for %s%s\n",
                dp->d_name, wnum(dp->d_unit));
            continue;
        }
        if (dp->d_unit == UNKNOWN || dp->d_unit == QUES)
            dp->d_unit = 0;
        fprintf(fp,
               "    { &%sdriver,\t%d,\tC 0x%08x,\t%d,\t0x%x },\n",
            dp->d_name, dp->d_unit, dp->d_addr, dp->d_pri,
            dp->d_flags);
    }
    fprintf(fp, "    { 0 }\n};\n");

    /* print devices connected to other controllers */
    fprintf(fp, "\nstruct conf_device conf_device_init[] = {\n");
    fprintf(fp,
       "   /* driver,\t\tctlr driver,\tunit,\tctlr,\tdrive,\tflags,\tpins */\n");
    for (dp = dtab; dp != 0; dp = dp->d_next) {
        if (dp->d_type == CONTROLLER || dp->d_type == SERVICE)
            continue;

        mp = dp->d_conn;
        fprintf(fp, "    { &%sdriver,\t", dp->d_name);
        if (mp) {
            fprintf(fp, "&%sdriver,\t%d,\t%d,\t",
                mp->d_name, dp->d_unit, mp->d_unit);
        } else {
            fprintf(fp, "0,\t\t%d,\t0,\t", dp->d_unit);
        }
        fprintf(fp, "%d,\t0x%x,\t", dp->d_drive, dp->d_flags);
        if (dp->d_npins > 0) {
            fprintf(fp, "{");
            for (i=dp->d_npins-1; i>=0; i--) {
                int bit = dp->d_pins[i] & 0xff;
                int port = dp->d_pins[i] >> 8;
                if (bit > 15 || port < 1 || port > 7) {
                    printf("R%c%u: invalid pin name\n", 'A'+port-1, bit);
                    exit(1);
                }
                fprintf(fp, "0x%x%x", port, bit);
                if (i > 0)
                    fprintf(fp, ",");
            }
            fprintf(fp, "}");
        } else
            fprintf(fp, "{0}");
        fprintf(fp, " },\n");
    }
    fprintf(fp, "    { 0 }\n};\n");
    service_ioconf(fp);
    fclose(fp);
}
#endif
