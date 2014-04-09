/*
 * iostat
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <nlist.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/file.h>
#include <sys/buf.h>
#include <sys/dk.h>

struct nlist nl[] = {
    { "_dk_busy" },
#define X_DK_BUSY   0
    { "_dk_xfer" },
#define X_DK_XFER   1
    { "_dk_bytes" },
#define X_DK_BYTES  2
    { "_tk_nin" },
#define X_TK_NIN    3
    { "_tk_nout" },
#define X_TK_NOUT   4
    { "_cp_time" },
#define X_CP_TIME   5
    { "_hz" },
#define X_HZ        6
    { "_dk_ndrive" },
#define X_DK_NDRIVE 7
    { "_dk_name" },
#define X_DK_NAME   8
    { "_dk_unit" },
#define X_DK_UNIT   9
    { 0 },
};

char    **dr_name;
char    **dk_name;
int *dr_select;
int *dk_unit;
int dk_ndrive;
int ndrives = 0;

struct {
    int dk_busy;
    long    cp_time[CPUSTATES];
    long    *dk_bytes;
    long    *dk_xfer;
    long    tk_nin;
    long    tk_nout;
} s, s1;

int mf;
int hz;
double  etime;
int tohdr = 1;

void printhdr(sig)
    int sig;
{
    register int i;

    printf("---tty---");
    for (i = 0; i < dk_ndrive; i++)
        if (dr_select[i])
            printf(" ---%3.3s--", dr_name[i]);
    printf(" ------cpu------\n");

    printf(" tin tout");
    for (i = 0; i < dk_ndrive; i++)
        if (dr_select[i])
            printf(" kbps tps");
    printf("  us  ni  sy  id\n");
    tohdr = 19;
}

main(argc, argv)
    char *argv[];
{
    extern char *ctime();
    register  i;
    int iter;
    double f1, f2;
    long t;
    char *arg, **cp, name[6], buf[BUFSIZ];

    knlist(nl);
    if(nl[X_DK_BUSY].n_value == 0) {
        printf("dk_busy not found in /vmunix namelist\n");
        exit(1);
    }
    mf = open("/dev/kmem", 0);
    if(mf < 0) {
        printf("cannot open /dev/kmem\n");
        exit(1);
    }
    iter = 0;
    for (argc--, argv++; argc > 0 && argv[0][0] == '-'; argc--, argv++)
        ;
    if (nl[X_DK_NDRIVE].n_value == 0) {
        printf("dk_ndrive undefined in system\n");
        exit(1);
    }
    lseek(mf, (long)nl[X_DK_NDRIVE].n_value, L_SET);
    read(mf, &dk_ndrive, sizeof (dk_ndrive));
    if (dk_ndrive <= 0) {
        printf("dk_ndrive %d\n", dk_ndrive);
        exit(1);
    }
    dr_select = (int *)calloc(dk_ndrive, sizeof (int));
    dr_name = (char **)calloc(dk_ndrive, sizeof (char *));
    dk_name = (char **)calloc(dk_ndrive, sizeof (char *));
    dk_unit = (int *)calloc(dk_ndrive, sizeof (int));
    s.dk_bytes = (long *)calloc(dk_ndrive, sizeof (long));
    s1.dk_bytes = (long *)calloc(dk_ndrive, sizeof (long));
    s.dk_xfer = (long *)calloc(dk_ndrive, sizeof (long));
    s1.dk_xfer = (long *)calloc(dk_ndrive, sizeof (long));
    for (arg = buf, i = 0; i < dk_ndrive; i++) {
        dr_name[i] = arg;
        sprintf(dr_name[i], "dk%d", i);
        arg += strlen(dr_name[i]) + 1;
    }
    read_names();
    lseek(mf, (long)nl[X_HZ].n_value, L_SET);
    read(mf, &hz, sizeof hz);

    /*
     * Choose drives to be displayed.  Priority
     * goes to (in order) drives supplied as arguments,
     * default drives.  If everything isn't filled
     * in and there are drives not taken care of,
     * display the first few that fit.
     */
    ndrives = 0;
    while (argc > 0 && !isdigit(argv[0][0])) {
        for (i = 0; i < dk_ndrive; i++) {
            if (strcmp(dr_name[i], argv[0]))
                continue;
            dr_select[i] = 1;
            ndrives++;
        }
        argc--, argv++;
    }
    for (i = 0; i < dk_ndrive && ndrives < 4; i++) {
        if (dr_select[i])
            continue;
        dr_select[i] = 1;
        ndrives++;
    }
    if (argc > 1)
        iter = atoi(argv[1]);
    signal(SIGCONT, printhdr);
loop:
    if (--tohdr == 0)
        printhdr();
    lseek(mf, (long)nl[X_DK_BUSY].n_value, L_SET);
    read(mf, &s.dk_busy, sizeof s.dk_busy);

    lseek(mf, (long)nl[X_DK_XFER].n_value, L_SET);
    read(mf, s.dk_xfer, dk_ndrive*sizeof (long));

    lseek(mf, (long)nl[X_DK_BYTES].n_value, L_SET);
    read(mf, s.dk_bytes, dk_ndrive*sizeof (long));

    lseek(mf, (long)nl[X_TK_NIN].n_value, L_SET);
    read(mf, &s.tk_nin, sizeof s.tk_nin);

    lseek(mf, (long)nl[X_TK_NOUT].n_value, L_SET);
    read(mf, &s.tk_nout, sizeof s.tk_nout);

    lseek(mf, (long)nl[X_CP_TIME].n_value, L_SET);
    read(mf, s.cp_time, sizeof s.cp_time);

    for (i = 0; i < dk_ndrive; i++) {
        if (! dr_select[i])
            continue;
#define X(fld)  t = s.fld[i]; s.fld[i] -= s1.fld[i]; s1.fld[i] = t
        X(dk_xfer); X(dk_bytes);
    }
    t = s.tk_nin; s.tk_nin -= s1.tk_nin; s1.tk_nin = t;
    t = s.tk_nout; s.tk_nout -= s1.tk_nout; s1.tk_nout = t;
    etime = 0;
    for(i=0; i<CPUSTATES; i++) {
        X(cp_time);
        etime += s.cp_time[i];
    }
    if (etime == 0.0)
        etime = 1.0;
    etime /= (float) hz;
    printf("%4.0f%5.0f", s.tk_nin/etime, s.tk_nout/etime);
    for (i=0; i<dk_ndrive; i++)
        if (dr_select[i])
            stats(i);
    for (i=0; i<CPUSTATES; i++)
        stat1(i);
    printf("\n");
    fflush(stdout);
contin:
    if (--iter && argc > 0) {
        sleep(atoi(argv[0]));
        goto loop;
    }
}

stats(dn)
{
    /* number of bytes transferred */
    printf("%5.0f", (double) s.dk_bytes[dn] / 1024 / etime);

    /* number of transfers */
    printf("%4.0f", (double) s.dk_xfer[dn] / etime);
}

stat1(o)
{
    register i;
    double time;

    time = 0;
    for(i=0; i<CPUSTATES; i++)
        time += s.cp_time[i];
    if (time == 0.0)
        time = 1.0;
    printf(" %3.0f", 100.0 * s.cp_time[o] / time);
}

read_names()
{
    char name[2];
    register int i;

    lseek(mf, (long)nl[X_DK_NAME].n_value, L_SET);
    read(mf, dk_name, dk_ndrive * sizeof (char *));
    lseek(mf, (long)nl[X_DK_UNIT].n_value, L_SET);
    read(mf, dk_unit, dk_ndrive * sizeof (int));

    for(i = 0; dk_name[i]; i++) {
        lseek(mf, (long)dk_name[i], L_SET);
        read(mf, name, sizeof name);
        sprintf(dr_name[i], "%c%c%d", name[0], name[1],
            dk_unit[i]);
    }
}
