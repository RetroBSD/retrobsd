#include "defs.h"
#include <fcntl.h>
#include <strings.h>

u_int   *uframe = UFRAME;
char    *symfil  = "a.out";
char    *corfil  = "core";

static long bss;

static int
getfile(filnam, cnt)
    char    *filnam;
    int     cnt;
{
    register int f;

    if (strcmp("-", filnam) == 0)
        return -1;

    f = open(filnam, wtflag);
    if (f < 0 && argcount > cnt) {
        if (wtflag)
            f = open(filnam, O_CREAT | O_TRUNC | wtflag, 644);
        if (f < 0)
            print("cannot open `%s'\n", filnam);
    }
    return f;
}

void
setsym()
{
    struct exec hdr;

    bzero(&txtmap, sizeof (txtmap));
    fsym = getfile(symfil, 1);
    txtmap.ufd = fsym;

    if (read(fsym, &hdr, sizeof (hdr)) >= (int)sizeof (hdr)) {
        magic = hdr.a_magic;
        txtsiz = hdr.a_text;
        datsiz = hdr.a_data;
        bss = hdr.a_bss;
        entrypt = hdr.a_entry;

        txtmap.f1 = N_TXTOFF(hdr);
        symoff = N_SYMOFF(hdr);

        switch (magic) {
        case OMAGIC:                    /* 0407 */
            txtmap.b1 = USER_DATA_START;
            txtmap.e1 = USER_DATA_START + txtsiz + datsiz;
            txtmap.b2 = txtmap.b1;
            txtmap.e2 = txtmap.e1;
            txtmap.f2 = txtmap.f1;
            break;
        default:
            magic = 0;
            txtsiz = 0;
            datsiz = 0;
            bss = 0;
            entrypt = 0;
        }
        datbas = txtmap.b2;
        symINI(&hdr);
    }
    if (magic == 0) {
        txtmap.e1 = maxfile;
    }
}

void
setcor()
{
    fcor = getfile(corfil, 2);
    datmap.ufd = fcor;
    if (read(fcor, corhdr, sizeof corhdr) == sizeof corhdr) {
        if (! kernel) {
            txtsiz = ((struct user*)corhdr)->u_tsize;
            datsiz = ((struct user*)corhdr)->u_dsize;
            stksiz = ((struct user*)corhdr)->u_ssize;
            datmap.f1 = USIZE;
            datmap.b2 = USER_DATA_END - stksiz;
            datmap.e2 = USER_DATA_END;
        } else {
            datsiz = datsiz + bss;
            stksiz = (long) USIZE;
            datmap.f1 = 0;
            datmap.b2 = 0140000L;
            datmap.e2 = 0140000L + USIZE;
        }
        switch (magic) {
        case OMAGIC:                    /* 0407 */
            datmap.b1 = USER_DATA_START;
            datmap.e1 = USER_DATA_START + txtsiz + datsiz;
            if (kernel) {
                datmap.f2 = (long)corhdr[KA6] * 0100L;
            } else {
                datmap.f2 = USIZE + txtsiz + datsiz;
            }
            break;

        default:
            magic = 0;
            datmap.b1 = 0;
            datmap.e1 = maxfile;
            datmap.f1 = 0;
            datmap.b2 = 0;
            datmap.e2 = 0;
            datmap.f2 = 0;
        }
        datbas = datmap.b1;
        if (! kernel && magic) {
            /* User's frame pointer in user's address space. */
            register u_int frame;
            frame = (long) ((struct user*)corhdr)->u_frame;
            frame -= KERNEL_DATA_END - USIZE;
            if (frame > 0 && frame < USIZE && ! (frame & 3)) {
                /* User's frame pointer in adb address space. */
                uframe = (u_int*) &corhdr[frame >> 2];
            }
        }
    } else {
        datmap.e1 = maxfile;
    }
}
