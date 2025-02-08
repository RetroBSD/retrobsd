/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
#include <sys/param.h>
#include <sys/clist.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/systm.h>

char cwaiting;
struct cblock *cfreelist;
int cfreecount;

/*
 * Character list get/put
 */
int
getc(struct clist *p)
{
    register struct cblock *bp;
    register int c, s;

    s = spltty();
    if (p->c_cc <= 0) {
        c = -1;
        p->c_cc = 0;
        p->c_cf = p->c_cl = NULL;
    } else {
        c = *p->c_cf++ & 0377;
        if (--p->c_cc<=0) {
            bp = (struct cblock *)(p->c_cf-1);
            bp = (struct cblock *)((int)bp & ~CROUND);
            p->c_cf = NULL;
            p->c_cl = NULL;
            bp->c_next = cfreelist;
            cfreelist = bp;
            cfreecount += CBSIZE;
            if (cwaiting) {
                wakeup (&cwaiting);
                cwaiting = 0;
            }
        } else if (((int)p->c_cf & CROUND) == 0){
            bp = (struct cblock *)(p->c_cf);
            bp--;
            p->c_cf = bp->c_next->c_info;
            bp->c_next = cfreelist;
            cfreelist = bp;
            cfreecount += CBSIZE;
            if (cwaiting) {
                wakeup (&cwaiting);
                cwaiting = 0;
            }
        }
    }
    splx(s);
    return (c);
}

/*
 * Copy clist to buffer.
 * Return number of bytes moved.
 */
int
q_to_b (struct clist *q, char *cp, int cc)
{
    register struct cblock *bp;
    register int nc;
    int s;
    char *acp;

    if (cc <= 0)
        return (0);
    s = spltty();
    if (q->c_cc <= 0) {
        q->c_cc = 0;
        q->c_cf = q->c_cl = NULL;
        splx(s);
        return (0);
    }
    acp = cp;

    while (cc) {
        nc = sizeof (struct cblock) - ((int)q->c_cf & CROUND);
        nc = MIN(nc, cc);
        nc = MIN(nc, q->c_cc);
        (void) bcopy(q->c_cf, cp, (unsigned)nc);
        q->c_cf += nc;
        q->c_cc -= nc;
        cc -= nc;
        cp += nc;
        if (q->c_cc <= 0) {
            bp = (struct cblock *)(q->c_cf - 1);
            bp = (struct cblock *)((int)bp & ~CROUND);
            q->c_cf = q->c_cl = NULL;
            bp->c_next = cfreelist;
            cfreelist = bp;
            cfreecount += CBSIZE;
            if (cwaiting) {
                wakeup (&cwaiting);
                cwaiting = 0;
            }
            break;
        }
        if (((int)q->c_cf & CROUND) == 0) {
            bp = (struct cblock *)(q->c_cf);
            bp--;
            q->c_cf = bp->c_next->c_info;
            bp->c_next = cfreelist;
            cfreelist = bp;
            cfreecount += CBSIZE;
            if (cwaiting) {
                wakeup (&cwaiting);
                cwaiting = 0;
            }
        }
    }
    splx(s);
    return (cp - acp);
}

/*
 * Return count of contiguous characters
 * in clist starting at q->c_cf.
 * Stop counting if flag&character is non-null.
 */
int ndqb (struct clist *q, int flag)
{
    int cc;
    int s;

    s = spltty();
    if (q->c_cc <= 0) {
        cc = -q->c_cc;
        goto out;
    }
    cc = ((int)q->c_cf + CBSIZE) & ~CROUND;
    cc -= (int)q->c_cf;
    if (q->c_cc < cc)
        cc = q->c_cc;
    if (flag) {
        register char *p, *end;

        p = q->c_cf;
        end = p;
        end += cc;
        while (p < end) {
            if (*p & flag) {
                cc = (int)p;
                cc -= (int)q->c_cf;
                break;
            }
            p++;
        }
    }
out:
    splx(s);
    return (cc);
}

/*
 * Flush cc bytes from q.
 */
void
ndflush (struct clist *q, int cc)
{
    register struct cblock *bp;
    char *end;
    int rem, s;

    s = spltty();
    if (q->c_cc <= 0)
        goto out;
    while (cc>0 && q->c_cc) {
        bp = (struct cblock *)((int)q->c_cf & ~CROUND);
        if ((int)bp == (((int)q->c_cl-1) & ~CROUND)) {
            end = q->c_cl;
        } else {
            end = (char *)((int)bp + sizeof (struct cblock));
        }
        rem = end - q->c_cf;
        if (cc >= rem) {
            cc -= rem;
            q->c_cc -= rem;
            q->c_cf = bp->c_next->c_info;
            bp->c_next = cfreelist;
            cfreelist = bp;
            cfreecount += CBSIZE;
            if (cwaiting) {
                wakeup (&cwaiting);
                cwaiting = 0;
            }
        } else {
            q->c_cc -= cc;
            q->c_cf += cc;
            if (q->c_cc <= 0) {
                bp->c_next = cfreelist;
                cfreelist = bp;
                cfreecount += CBSIZE;
                if (cwaiting) {
                    wakeup (&cwaiting);
                    cwaiting = 0;
                }
            }
            break;
        }
    }
    if (q->c_cc <= 0) {
        q->c_cf = q->c_cl = NULL;
        q->c_cc = 0;
    }
out:
    splx(s);
}

/*
 * Put a symbol to a character list.
 */
int
putc (int c, struct clist *p)
{
    register struct cblock *bp;
    register char *cp;
    register int s;

    s = spltty();
    if ((cp = p->c_cl) == NULL || p->c_cc < 0 ) {
        if ((bp = cfreelist) == NULL) {
            splx(s);
            return (-1);
        }
        cfreelist = bp->c_next;
        cfreecount -= CBSIZE;
        bp->c_next = NULL;
        p->c_cf = cp = bp->c_info;
    } else if (((int)cp & CROUND) == 0) {
        bp = (struct cblock *)cp - 1;
        if ((bp->c_next = cfreelist) == NULL) {
            splx(s);
            return (-1);
        }
        bp = bp->c_next;
        cfreelist = bp->c_next;
        cfreecount -= CBSIZE;
        bp->c_next = NULL;
        cp = bp->c_info;
    }
    *cp++ = c;
    p->c_cc++;
    p->c_cl = cp;
    splx(s);
    return (0);
}

/*
 * Copy buffer to clist.
 * Return number of bytes not transfered.
 */
int
b_to_q (char *cp, int cc, struct clist *q)
{
    register char *cq;
    register struct cblock *bp;
    register int s, nc;
    int acc;

    if (cc <= 0)
        return (0);
    acc = cc;
    s = spltty();
    if ((cq = q->c_cl) == NULL || q->c_cc < 0) {
        if ((bp = cfreelist) == NULL)
            goto out;
        cfreelist = bp->c_next;
        cfreecount -= CBSIZE;
        bp->c_next = NULL;
        q->c_cf = cq = bp->c_info;
    }

    while (cc) {
        if (((int)cq & CROUND) == 0) {
            bp = (struct cblock *)cq - 1;
            if ((bp->c_next = cfreelist) == NULL)
                goto out;
            bp = bp->c_next;
            cfreelist = bp->c_next;
            cfreecount -= CBSIZE;
            bp->c_next = NULL;
            cq = bp->c_info;
        }
        nc = MIN(cc, sizeof (struct cblock) - ((int)cq & CROUND));
        (void) bcopy(cp, cq, (unsigned)nc);
        cp += nc;
        cq += nc;
        cc -= nc;
    }
out:
    q->c_cl = cq;
    q->c_cc += acc - cc;
    splx(s);
    return (cc);
}

/*
 * Given a non-NULL pointter into the list (like c_cf which
 * always points to a real character if non-NULL) return the pointer
 * to the next character in the list or return NULL if no more chars.
 *
 * Callers must not allow getc's to happen between nextc's so that the
 * pointer becomes invalid.  Note that interrupts are NOT masked.
 */
char *
nextc (struct clist *p, char *cp)
{
    register char *rcp;

    if (p->c_cc && ++cp != p->c_cl) {
        if (((int)cp & CROUND) == 0)
            rcp = ((struct cblock *)cp)[-1].c_next->c_info;
        else
            rcp = cp;
    } else
        rcp = (char *)NULL;
    return (rcp);
}

/*
 * Remove the last character in the list and return it.
 */
int
unputc (struct clist *p)
{
    register struct cblock *bp;
    register int c, s;
    struct cblock *obp;

    s = spltty();
    if (p->c_cc <= 0)
        c = -1;
    else {
        c = *--p->c_cl;
        if (--p->c_cc <= 0) {
            bp = (struct cblock *)p->c_cl;
            bp = (struct cblock *)((int)bp & ~CROUND);
            p->c_cl = p->c_cf = NULL;
            bp->c_next = cfreelist;
            cfreelist = bp;
            cfreecount += CBSIZE;
        } else if (((int)p->c_cl & CROUND) == sizeof(bp->c_next)) {
            p->c_cl = (char *)((int)p->c_cl & ~CROUND);
            bp = (struct cblock *)p->c_cf;
            bp = (struct cblock *)((int)bp & ~CROUND);
            while (bp->c_next != (struct cblock *)p->c_cl)
                bp = bp->c_next;
            obp = bp;
            p->c_cl = (char *)(bp + 1);
            bp = bp->c_next;
            bp->c_next = cfreelist;
            cfreelist = bp;
            cfreecount += CBSIZE;
            obp->c_next = NULL;
        }
    }
    splx(s);
    return (c);
}

/*
 * Put the chars in the from que
 * on the end of the to que.
 */
void
catq (struct clist *from, struct clist *to)
{
    char bbuf [CBSIZE*4];
    register int c;
    int s;

    s = spltty();
    if (to->c_cc == 0) {
        *to = *from;
        from->c_cc = 0;
        from->c_cf = NULL;
        from->c_cl = NULL;
        splx(s);
        return;
    }
    splx(s);
    while (from->c_cc > 0) {
        c = q_to_b(from, bbuf, sizeof bbuf);
        (void) b_to_q(bbuf, c, to);
    }
}
