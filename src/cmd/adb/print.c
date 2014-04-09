#include "defs.h"
#include <fcntl.h>

const REGLIST reglist [FRAME_WORDS] = {
    { "at",     FRAME_R1    },      /* 0  */
    { "v0",     FRAME_R2    },      /* 1  */
    { "v1",     FRAME_R3    },      /* 2  */
    { "a0",     FRAME_R4    },      /* 3  */
    { "a1",     FRAME_R5    },      /* 4  */
    { "a2",     FRAME_R6    },      /* 5  */
    { "a3",     FRAME_R7    },      /* 6  */
    { "t0",     FRAME_R8    },      /* 7  */
    { "t1",     FRAME_R9    },      /* 8  */
    { "t2",     FRAME_R10   },      /* 9  */
    { "t3",     FRAME_R11   },      /* 10 */
    { "t4",     FRAME_R12   },      /* 11 */
    { "t5",     FRAME_R13   },      /* 12 */
    { "t6",     FRAME_R14   },      /* 13 */
    { "t7",     FRAME_R15   },      /* 14 */
    { "s0",     FRAME_R16   },      /* 15 */
    { "s1",     FRAME_R17   },      /* 16 */
    { "s2",     FRAME_R18   },      /* 17 */
    { "s3",     FRAME_R19   },      /* 18 */
    { "s4",     FRAME_R20   },      /* 19 */
    { "s5",     FRAME_R21   },      /* 20 */
    { "s6",     FRAME_R22   },      /* 21 */
    { "s7",     FRAME_R23   },      /* 22 */
    { "t8",     FRAME_R24   },      /* 23 */
    { "t9",     FRAME_R25   },      /* 24 */
    { "gp",     FRAME_GP    },      /* 25 */
    { "sp",     FRAME_SP    },      /* 26 */
    { "fp",     FRAME_FP    },      /* 27 */
    { "ra",     FRAME_RA    },      /* 28 */
    { "lo",     FRAME_LO    },      /* 29 */
    { "hi",     FRAME_HI    },      /* 30 */
    { "status", FRAME_STATUS},      /* 31 */
    { "pc",     FRAME_PC    },      /* 32 */
};

static const REGLIST kregs[] = {    // TODO
    { "r1",     FRAME_R1    },      /* 0  */
    { "r2",     FRAME_R2    },      /* 1  */
    { "r3",     FRAME_R3    },      /* 2  */
    { "r4",     FRAME_R4    },      /* 3  */
    { "r5",     FRAME_R5    },      /* 4  */
    { "r6",     FRAME_R6    },      /* 5  */
    { "sp",     FRAME_SP    },      /* 26 */
};

static void
printmap(s, amap)
    char    *s;
    MAP     *amap;
{
    int file;

    file = amap->ufd;
    print("%s%12t`%s'\n", s, file < 0 ? "-" :
                             file == fcor ? corfil : symfil);
    print("b1 = %-12Q", amap->b1);
    print("e1 = %-12Q", amap->e1);
    print("f1 = %-12Q", amap->f1);
    print("\n");
    print("b2 = %-12Q", amap->b2);
    print("e2 = %-12Q", amap->e2);
    print("f2 = %-12Q", amap->f2);
    printc(EOR);
}

static void
printregs()
{
    register const REGLIST *p;
    int v;

    if (kernel) {
        // TODO
        for (p=kregs; p<&kregs[7]; p++) {
            v = corhdr[p->roffs];
            print("%s%8t%x%8t", p->rname, v);
            valpr(v, DSYM);
            printc(EOR);
        }
    } else {
	print ("                t0 = %9x  s0 = %9x  t8 = %9x   lo = %9x\n",
		uframe [FRAME_R8], uframe [FRAME_R16],
		uframe [FRAME_R24], uframe [FRAME_LO]);
	print ("at = %9x  t1 = %9x  s1 = %9x  t9 = %9x   hi = %9x\n",
		uframe [FRAME_R1], uframe [FRAME_R9], uframe [FRAME_R17],
		uframe [FRAME_R25], uframe [FRAME_HI]);
	print ("v0 = %9x  t2 = %9x  s2 = %9x               status = %9x\n",
		uframe [FRAME_R2], uframe [FRAME_R10],
		uframe [FRAME_R18], uframe [FRAME_STATUS]);
	print ("v1 = %9x  t3 = %9x  s3 = %9x                   pc = %9x\n",
		uframe [FRAME_R3], uframe [FRAME_R11],
		uframe [FRAME_R19], uframe [FRAME_PC]);
	print ("a0 = %9x  t4 = %9x  s4 = %9x  gp = %9x\n",
		uframe [FRAME_R4], uframe [FRAME_R12],
		uframe [FRAME_R20], uframe [FRAME_GP]);
	print ("a1 = %9x  t5 = %9x  s5 = %9x  sp = %9x\n",
		uframe [FRAME_R5], uframe [FRAME_R13],
		uframe [FRAME_R21], uframe [FRAME_SP]);
	print ("a2 = %9x  t6 = %9x  s6 = %9x  fp = %9x\n",
		uframe [FRAME_R6], uframe [FRAME_R14],
		uframe [FRAME_R22], uframe [FRAME_FP]);
	print ("a3 = %9x  t7 = %9x  s7 = %9x  ra = %9x\n",
		uframe [FRAME_R7], uframe [FRAME_R15],
		uframe [FRAME_R23], uframe [FRAME_RA]);
        printpc();
    }
}

/*
 * general printing routines ($)
 */
void
printtrace(modif)
{
    int     narg, i, stat, name, limit;
    u_int   dynam;
    register BKPTR bkptr;
    char    hi, lo;
    int     word, stack = 0;
    char    *comptr;
    long    argp, frame, link;
    register struct SYMbol  *symp;

    if (cntflg == 0) {
        cntval = -1;
    }

    switch (modif) {

    case '<':
        if (cntval == 0) {
            while (readchar() != EOR)
                ;
            lp--;
            break;
        }
        if (rdc() == '<') {
            stack = 1;
        } else {
            stack = 0;
            lp--;
        }
        /* fall thru ... */
    case '>': {
        char        file[64];
        char        Ifile[128];
        int         index;

        index = 0;
        if (rdc() != EOR) {
            do {
                file[index++] = lastc;
                if (index >= 63)
                    error(LONGFIL);
            } while (readchar() != EOR);
            file[index] = 0;
            if (modif == '<') {
                if (Ipath) {
                    strcpy(Ifile, Ipath);
                    strcat(Ifile, "/");
                    strcat(Ifile, file);
                }
                if (strcmp(file, "-") != 0) {
                    iclose(stack, 0);
                    infile = open(file, 0);
                    if (infile < 0) {
                        infile = open(Ifile, 0);
                    }
                } else {
                    lseek(infile, 0L, 0);
                }
                if (infile < 0) {
                    infile = 0;
                    error(NOTOPEN);
                } else {
                    if (cntflg) {
                        var[9] = cntval;
                    } else {
                        var[9] = 1;
                    }
                }
            } else {
                oclose();
                outfile = open(file, O_CREAT | O_WRONLY, 0644);
                lseek(outfile, 0L, 2);
            }
        } else {
            if (modif == '<') {
                iclose(-1, 0);
            } else {
                oclose();
            }
        }
        lp--;
        }
        break;

    case 'o':
        octal = TRUE;
        break;

    case 'd':
        octal = FALSE;
        break;

    case 'q': case 'Q': case '%':
        done();

    case 'w': case 'W':
        maxpos = adrflg ? adrval : MAXPOS;
        break;

    case 's': case 'S':
        maxoff = adrflg ? adrval : MAXOFF;
        break;

    case 'v':
        print("variables\n");
        for (i=0; i<=35; i++) {
            if (var[i]) {
                printc(i + (i <= 9 ? '0' : 'a'-10));
                print(" = %Q\n", var[i]);
           }
        }
        break;

    case 'm': case 'M':
        printmap("? map", &txtmap);
        printmap("/ map", &datmap);
        break;

    case 0: case '?':
        if (pid) {
            print("pcs id = %d\n", pid);
        } else {
            print("no process\n");
        }
        sigprint();
        flushbuf();
    case 'r': case 'R':
        printregs();
        return;

    case 'f': case 'F':
        // No FPU registers
        return;

    case 'c': case 'C':
        // TODO
        frame = (adrflg ? adrval :
            (kernel ? corhdr[FRAME_R5] : uframe[FRAME_R5])) & ALIGNED;
        lastframe = 0;
        callpc = (adrflg ? get(frame + 2, DSP) :
             (kernel ? (-2) : uframe[FRAME_PC]));
        while (cntval--) {
            chkerr();
            print("%Q: ", frame); /* Add frame address info */
            narg = findroutine(frame);
            print("%s(", cache_sym(symbol));
            argp = frame+4;
            if (--narg >= 0) {
                print("%o", get(argp, DSP));
            }
            while (--narg >= 0) {
                argp += 2;
                print(",%o", get(argp, DSP));
            }
            /* Add return-PC info.  Force printout of
             * symbol+offset (never just a number! ) by using
             * max possible offset.  Overlay has already been set
             * properly by findfn.
             */
            print(") from ");
            {
                int     savmaxoff = maxoff;

                maxoff = ((unsigned)-1)>>1;
                psymoff((long)callpc, ISYM, "");
                maxoff = savmaxoff;
            }
            printc('\n');

            if (modif == 'C') {
                while (localsym(frame)) {
                    word = get(localval, DSP);
                    print("%8t%s:%10t", cache_sym(symbol));
                    if (errflg) {
                        print("?\n");
                        errflg = 0;
                    } else {
                        print("%o\n", word);
                    }
                }
            }
            lastframe = frame;
            frame = get(frame, DSP) & ALIGNED;
            if (kernel ? ((u_int)frame > ((u_int)0140000 + USIZE)) :
                (frame == 0))
                break;
        }
        break;

    case 'e': case 'E':                 /* print externals */
        symset();
        while ((symp = symget())) {
            chkerr();
            if (symp->type == (N_EXT | N_DATA) ||
                symp->type == (N_EXT | N_BSS)) {
                print("%s:%12t%o\n", no_cache_sym(symp),
                    get(leng(symp->value), DSP));
            }
        }
        break;

    case 'a': case 'A':
        frame = adrflg ? adrval : uframe[FRAME_R4];

        while (cntval--) {
            chkerr();
            stat = get(frame, DSP);
            dynam = get(frame+2, DSP);
            link = get(frame+4, DSP);
            if (modif == 'A') {
                print("%8O:%8t%-8o,%-8o,%-8o", frame, stat, dynam, link);
            }
            if (stat == 1)
                break;
            if (errflg)
                error(A68BAD);

            if (get(link-4, ISP) != 04767) {
                if (get(link-2, ISP) != 04775)
                    error(A68LNK);
                print(" ? ");
            } else {
                print("%8t");
                name = shorten(link) + get(link-2, ISP);
                valpr(name, ISYM);
                name = get(leng(name -2), ISP);
                print("%8t\"");
                limit = 8;
                do {
                    word = get(leng(name), DSP);
                    name += 2;
                    lo = word & LOBYTE;
                    hi = (word >> 8) & LOBYTE;
                    printc(lo);
                    printc(hi);
                } while (lo && hi && limit--);
                printc('"');
            }
            limit = 4;
            i = 6;
            print("%24targs:%8t");
            while (limit--) {
                print("%8t%o", get(frame + i, DSP));
                i += 2;
            }
            printc(EOR);

            frame = dynam;
        }
        errflg = 0;
        flushbuf();
        break;
                                        /* set default c frame */
    case 'b': case 'B':                 /* print breakpoints */
        print("breakpoints\ncount%8tbkpt%24tcommand\n");
        for (bkptr=bkpthead; bkptr; bkptr=bkptr->nxtbkpt) {
            if (bkptr->flag) {
                print("%-8.8d", bkptr->count);
                psymoff(leng(bkptr->loc), ISYM, "%24t");
                comptr = bkptr->comm;
                while (*comptr)
                    printc(*comptr++);
            }
        }
        break;

    default:
        error(BADMOD);
    }
}

int
getreg(regnam)
{
    register const REGLIST *p;
    register char *regptr;
    char regnxt;

    if (kernel)                         /* not supported */
        return NOREG;
    regnxt = readchar();
    for (p=reglist; p<&reglist[NREG]; p++) {
        regptr = p->rname;
        if ((regnam == *regptr++) && (regnxt == *regptr))
            return p->roffs;
    }
    lp--;
    return NOREG;
}

void
printpc()
{
    dot = uframe[FRAME_PC];
    psymoff(dot, ISYM, ":%16t");
    printins(ISP, dot, chkget(dot, ISP));
    printc(EOR);
}

void
sigprint()
{
#ifndef CROSS
    if (signo >= 0 && signo < NSIG)
        print("%s", sys_siglist[signo]);
    else
#endif
        print("unknown signal %d", signo);
}
