/*
 * File handling and access routines
 */
#include "defs.h"
#include <fcntl.h>

static int
within(adr, lbd, ubd)
    long adr, lbd, ubd;
{
    return (adr>=lbd && adr<ubd);
}

static int
chkmap(adr, space)
    register long *adr;
    register int space;
{
    register MAPPTR amap;

    amap = (space & DSP) ? &datmap : &txtmap;
    switch (space & (ISP | DSP | STAR)) {
    case ISP:
        if (within(*adr, amap->b1, amap->e1)) {
            *adr += (amap->f1) - (amap->b1);
            break;
        }
        /* falls through */

    case ISP+STAR:
        if (within(*adr, amap->b2, amap->e2)) {
            *adr += (amap->f2) - (amap->b2);
            break;
        }
        goto err;

    case DSP:
        if (within(*adr, amap->b1, amap->e1)) {
            *adr += (amap->f1) - (amap->b1);
            break;
        }
        /* falls through */

    case DSP+STAR:
        if (within(*adr, amap->b2, amap->e2)) {
            *adr += (amap->f2) - (amap->b2);
            break;
        }
        /* falls through */

    default:
err:    errflg = (space & DSP) ? BADDAT : BADTXT;
        return 0;
    }
    return 1;
}

int
acces(mode, adr, space, value)
    long    adr;
{
    int     w, w1, pmode, rd, file;
    BKPTR   bkptr;

    if (space == NSP)
        return 0;
    rd = (mode == RD);

    if (pid) {                          /* tracing on? */
        if ((adr & 3) && ! rd)
            error(ODDADR);
        pmode = (space & DSP) ?
                        (rd ? PT_READ_D : PT_WRITE_D) :
                        (rd ? PT_READ_I : PT_WRITE_I);
        bkptr = scanbkpt((u_int) adr);
        if (bkptr) {
            if (rd)
                return bkptr->ins;
            bkptr->ins = value;
            return 0;
        }
        w = ptrace(pmode, pid, (void*) shorten(adr & ~1), value);
        if (adr & 01) {
            w1 = ptrace(pmode, pid, (void*) shorten(adr + 1), value);
            w = ((w >> 8) & LOBYTE) | (w1 << 8);
        }
        if (errno) {
            errflg = (space & DSP) ? BADDAT : BADTXT;
        }
        return w;
    }
    w = 0;
    if (mode==WT && wtflag==O_RDONLY) {
        error("not in write mode");
    }
    if (! chkmap(&adr, space))
        return 0;

    file = (space & DSP) ? datmap.ufd : txtmap.ufd;
    if (lseek(file, adr, 0) == -1L ||
        (rd ? read(file, &w, 4) : write(file, &value, 4)) < 1) {
        errflg = (space & DSP) ? BADDAT : BADTXT;
    }
    return w;
}

void
put(adr, space, value)
    long   adr;
{
    acces(WT, adr, space, value);
}

u_int
get(adr, space)
    long    adr;
{
    return acces(RD, adr, space, 0);
}

u_int
chkget(n, space)
    long n;
{
    register int w;

    w = get(n, space);
    chkerr();
    return w;
}
