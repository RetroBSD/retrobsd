/*
 * Service routines for sub process control
 */
#include "defs.h"
#include <fcntl.h>
#include <sys/wait.h>

static int userpc = 1;

int
getsig(sig)
{
    return expr(0) ? shorten(expv) : sig;
}

static void
bpwait()
{
    register int w;
    int stat;

    signal(SIGINT, SIG_IGN);
    while ((w = wait(&stat)) != pid && w != -1)
        ;
    signal(SIGINT, sigint);

#ifdef TIOCGETP
    ioctl(0, TIOCGETP, &usrtty);
    ioctl(0, TIOCSETP, &adbtty);
#else
    tcgetattr(0, &usrtty);
    tcsetattr(0, TCSANOW, &adbtty);
#endif
    if (w == -1) {
        pid = 0;
        errflg = BADWAIT;

    } else if ((stat & 0177) != 0177) {
        signo = stat & 0177;
        if (signo) {
            sigprint();
        }
        if (stat & 0200) {
            print(" - core dumped");
            close(fcor);
            setcor();
        }
        pid = 0;
        errflg = ENDPCS;

    } else {
        signo = stat>>8;
        if (signo != SIGTRAP) {
            sigprint();
        } else {
            signo = 0;
        }
        flushbuf();
    }
}

/*
 * get REG values from pcs
 */
static void
readregs()
{
    register u_int i;

    for (i=0; i<NREG; i++) {
        uframe[reglist[i].roffs] = ptrace(PT_READ_U, pid,
            (char*)((long)&uframe[reglist[i].roffs] - (long)&corhdr),
            0);
    }
}

static void
execbkpt(bkptr)
    BKPTR bkptr;
{
    int bkptloc;

#ifdef DEBUG
    print("exbkpt: %d\n", bkptr->count);
#endif
    bkptloc = bkptr->loc;
    ptrace(PT_WRITE_I, pid, (void*) bkptloc, bkptr->ins);

#ifdef TIOCGETP
    ioctl(0, TIOCSETP, &usrtty);
#else
    tcsetattr(0, TCSANOW, &usrtty);
#endif
    ptrace(PT_STEP, pid, (void*) bkptloc, 0);
    bpwait();
    chkerr();
    ptrace(PT_WRITE_I, pid, (void*) bkptloc, BPT);
    bkptr->flag = BKPTSET;
}

int
runpcs(runmode, execsig)
{
    int rc = 0;
    register BKPTR bkpt;

    if (adrflg)
        userpc = shorten(dot);
    print("%s: running\n", symfil);

    while (loopcnt-- > 0) {
#ifdef DEBUG
        print("\ncontinue %d %d\n", userpc, execsig);
#endif
#ifdef TIOCGETP
        ioctl(0, TIOCSETP, &usrtty);
#else
        tcsetattr(0, TCSANOW, &usrtty);
#endif
        ptrace (runmode, pid, (void*) userpc, execsig);
        bpwait();
        chkerr();
        readregs();

        /* look for bkpt */
        if (signo == 0 && (bkpt = scanbkpt(uframe[FRAME_PC] - 2))) {
            /* stopped at bkpt */
            userpc = uframe[FRAME_PC] = bkpt->loc;
            if (bkpt->flag == BKPTEXEC ||
                ((bkpt->flag = BKPTEXEC, command(bkpt->comm, ':')) &&
                --bkpt->count))
            {
                execbkpt(bkpt);
                execsig = 0;
                loopcnt++;
                userpc = 1;
            } else {
                bkpt->count = bkpt->initcnt;
                rc = 1;
             }
        } else {
            rc = 0;
            execsig = signo;
            userpc = 1;
        }
    }
    return(rc);
}

void
endpcs()
{
    register BKPTR  bkptr;

    if (pid) {
        ptrace(PT_KILL, pid, 0, 0);
        pid = 0;
        userpc = 1;
        for (bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt) {
            if (bkptr->flag) {
                bkptr->flag = BKPTSET;
            }
        }
    }
}

static void
doexec()
{
    char *argl[MAXARG];
    char args[LINSIZ];
    char *p, **ap, *filnam;

    ap = argl;
    p = args;
    *ap++ = symfil;
    do {
        if (rdc() == EOR)
            break;
        /*
         * If we find an argument beginning with a `<' or a `>', open
         * the following file name for input and output, respectively
         * and back the argument collocation pointer, p, back up.
         */
        *ap = p;
        while (lastc != EOR && lastc != SP && lastc != TB) {
            *p++ = lastc;
            readchar();
        }
        *p++ = 0;
        filnam = *ap + 1;
        if (**ap == '<') {
            close(0);
            if (open(filnam, 0) < 0) {
                print("%s: cannot open\n", filnam);
                exit(0);
            }
            p = *ap;
        } else if (**ap == '>') {
            close(1);
            if (open(filnam, O_CREAT | O_WRONLY, 0666) < 0) {
                print("%s: cannot create\n", filnam);
                exit(0);
            }
            p = *ap;
        } else {
            ap++;
        }
    } while (lastc != EOR);
    *ap++ = 0;
    execv(symfil, argl);
}

void
setup()
{
    close(fsym);
    fsym = -1;
    pid = fork();
    if (pid == 0) {
        ptrace(PT_TRACE_ME, 0, 0, 0);
        signal(SIGINT, sigint);
        signal(SIGQUIT, sigqit);
        doexec();
        exit(0);
    } else if (pid == -1) {
        error(NOFORK);
    } else {
        bpwait();
        readregs();
        lp[0] = EOR;
        lp[1] = 0;
        fsym = open(symfil, wtflag);
        if (errflg) {
            print("%s: cannot execute\n", symfil);
            endpcs();
            error((char *)0);
        }
    }
}

BKPTR
scanbkpt(adr)
{
    register BKPTR bkptr;

    for (bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt) {
        if (bkptr->flag && bkptr->loc == adr)
            break;
    }
    return bkptr;
}

void
delbp()
{
    register BKPTR bkptr;

    for (bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt) {
        if (bkptr->flag)
            del1bp(bkptr);
    }
}

void
del1bp(bkptr)
    BKPTR bkptr;
{
    ptrace(PT_WRITE_I, pid, (void*) bkptr->loc, bkptr->ins);
}

void
setbp()
{
    register BKPTR bkptr;

    for (bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt) {
        if (bkptr->flag)
            set1bp(bkptr);
    }
}

void
set1bp(bkptr)
    BKPTR bkptr;
{
    register int a;

    a = bkptr->loc;
    bkptr->ins = ptrace(PT_READ_I, pid, (void*) a, 0);
    ptrace(PT_WRITE_I, pid, (void*) a, BPT);
    if (errno) {
        print("cannot set breakpoint: ");
        psymoff(leng(bkptr->loc), ISYM, "\n");
    }
}
