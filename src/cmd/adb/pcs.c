/*
 * Sub process control
 */
#include "defs.h"

void
subpcs(modif)
{
    register int check;
    int execsig, runmode;
    register BKPTR  bkptr;
    char *comptr;

    execsig = 0;
    loopcnt = cntval;

    switch (modif) {

    case 'd': case 'D':                 /* delete breakpoint */
        bkptr = scanbkpt(shorten(dot));
        if (! bkptr)
            error(NOBKPT);
        bkptr->flag = 0;
        if (pid)
            del1bp(bkptr);
        return;

    case 'b': case 'B':                 /* set breakpoint */
        bkptr = scanbkpt(shorten(dot));
        if (bkptr) {
            bkptr->flag = 0;
            if (pid)
                del1bp(bkptr);
        }
        for (bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt) {
            if (bkptr->flag == 0)
                break;
        }
        if (bkptr == 0) {
            bkptr = (BKPTR) malloc(sizeof *bkptr);
            if (! bkptr) {
                error(SZBKPT);
            } else {
                bkptr->nxtbkpt = bkpthead;
                bkpthead=bkptr;
            }
        }
        bkptr->loc = dot;
        bkptr->initcnt = bkptr->count = cntval;
        bkptr->flag = BKPTSET;
        check = MAXCOM-1;
        comptr = bkptr->comm;
        rdc();
        lp--;
        do {
            *comptr++ = readchar();
        } while (check-- && lastc != EOR);
        *comptr = 0;
        lp--;
        if (! check)
            error(EXBKPT);
        if (pid)
            set1bp(bkptr);
        return;

    case 'k': case 'K':                 /* exit */
        if (! pid)
            error(NOPCS);
        print("%d: killed", pid);
        endpcs();
        return;

    case 'r': case 'R':                 /* run program */
        endpcs();
        setup();
        setbp();
        runmode = PT_CONTINUE;
        break;

    case 's': case 'S':                 /* single step */
        runmode = PT_STEP;
        if (pid) {
            execsig = getsig(signo);
        } else {
            setup();
            loopcnt--;
        }
        break;

    case 'c': case 'C': case 0:         /* continue with optional signal */
        if (pid == 0)
            error(NOPCS);
        runmode = PT_CONTINUE;
        execsig = getsig(signo);
        break;

    default:
        error(BADMOD);
        return;
    }

    if (loopcnt > 0 && runpcs(runmode, execsig)) {
        print("breakpoint%16t");
    } else {
        print("stopped at%16t");
    }
    printpc();
}
