/*
 * ADVENTURE -- Jim Gillogly, Jul 1977
 *
 * This program is a re-write of ADVENT, written in FORTRAN mostly by
 * Don Woods of SAIL.  In most places it is as nearly identical to the
 * original as possible given the language and word-size differences.
 * A few places, such as the message arrays and travel arrays were changed
 * to reflect the smaller core size and word size.  The labels of the
 * original are reflected in this version, so that the comments of the
 * fortran are still applicable here.
 *
 * The data file distributed with the fortran source is assumed to be called
 * "glorkz" in the directory where the program is first run.
 *
 * Data save/restore rewritten in portable way by Serge Vakulenko.
 */

/* hdr.h: included by c advent files */
#ifdef CROSS
#   ifdef __APPLE__
#       define _OFF_T
        typedef long long off_t;
#   endif
#   include </usr/include/stdio.h>
#else
#   include <stdio.h>
#endif
#include <stdlib.h>

int datfd;                              /* message file descriptor      */
int delhit;

#define DATFILE "glorkz"                /* all the original msgs        */
#define DATSIZE (46*1024)               /* size of encrypted data       */

#define TAB     011
#define LF      012
#define FLUSHLINE while (getchar()!='\n')
#define FLUSHLF   while (next()!=LF)

char *wd1, *wd2;                        /* the complete words           */

struct hashtab {                        /* hash table for vocabulary    */
        int val;                        /* word type &index (ktab)      */
        int hash;                       /* 32-bit hash value            */
};

struct text {
        unsigned short seekadr;         /* DATFILE must be < 2**16      */
        unsigned short txtlen;          /* length of msg starting here  */
};

struct travlist {                       /* direcs & conditions of travel*/
        struct travlist *next;          /* ptr to next list entry       */
        unsigned short conditions;      /* m in writeup (newloc / 1000) */
        unsigned short tloc;            /* n in writeup (newloc % 1000) */
        unsigned short tverb;           /* the verb that takes you there*/
};

/*
 * Game state.
 */
struct {
    short loc, newloc, oldloc, oldlc2, wzdark, gaveup, kq, k, k2;
    short verb, obj, spk;
    short blklin;
    int saved, savet, mxscor, latncy;

    #define MAXSTR  20                  /* max length of user's words   */

    #define HTSIZE  512                 /* max number of vocab words    */
    struct hashtab voc[HTSIZE];         /* hash table for vocabulary    */

    #define RTXSIZ  205
    struct text rtext[RTXSIZ];          /* random text messages         */

    #define MAGSIZ  35
    struct text mtext[MAGSIZ];          /* magic messages               */

    short clsses;
    #define CLSMAX  12
    struct text ctext[CLSMAX];          /* classes of adventurer        */
    short cval[CLSMAX];

    struct text ptext[101];             /* object descriptions          */

    #define LOCSIZ  141                 /* number of locations          */
    struct text ltext[LOCSIZ];          /* long loc description         */
    struct text stext[LOCSIZ];          /* short loc descriptions       */

    struct travlist *travel[LOCSIZ];    /* direcs & conditions of travel*/

    short atloc[LOCSIZ];

    short plac[101];                      /* initial object placement     */
    short fixd[101], fixed[101];          /* location fixed?              */

    short actspk[35];                     /* rtext msg for verb <n>       */

    short cond[LOCSIZ];                   /* various condition bits       */

    short hntmax;
    short hints[20][5];                   /* info on hints                */
    short hinted[20], hintlc[20];

    short place[101], prop[101], plink[201];
    short abb[LOCSIZ];

    short maxtrs, tally, tally2;          /* treasure values              */

    #define FALSE   0
    #define TRUE    1

    short keys, lamp, grate, cage, rod,   /* mnemonics                    */
        rod2, steps, bird, door, pillow, snake, fissur, tablet, clam,
        oyster, magzin, dwarf, knife, food, bottle, water, oil, plant,
        plant2, axe, mirror, dragon, chasm, troll, troll2, bear, messag,
        vend, batter, nugget, coins, chest, eggs, tridnt, vase, emrald,
        pyram, pearl, rug, chain, spices, back, look, cave, null, entrnc,
        dprssn, say, lock, throw, find, invent;

    short chloc, chloc2, dseen[7],        /* dwarf stuff                  */
        dloc[7], odloc[7], dflag, daltlc;

    short tk[21], stick, dtotal, attack;
    short turns, lmwarn, iwest, knfloc,   /* various flags & counters     */
        detail, abbnum, maxdie, numdie, holdng, dkill, foobar, bonus,
        clock1, clock2, closng, panic, closed, scorng;

    short limit;
} game;

struct travlist *tkk;                   /* travel is closer to keys(...)*/

extern const short setbit[16];            /* bit defn masks 1,2,4,...     */

void linkdata (void);
void startup (void);
void trapdel (int);
void rdata (char *, char *);
void mspeak (int);
void rspeak (int);
void speak (struct text *);
int toting (int);
int yes (int, int, int);
int yesm (int, int, int);
void drop (int, int);
int vocab (char *, int, int);
void poof (void);
int confirm (char *);
void save (char *, unsigned);
void ciao (void);
int restore (char *);
int restdat (int, unsigned);
void start (int);
int length (char *);
void bug (int);
int getcmd (char *);
int fdwarf (void);
int die (int);
int forced (int);
int dark (int);
int pct (int);
void pspeak (int, int);
void checkhints (void);
void getin (char **, char **);
void copystr (char *, char *);
void done (int);
int closing (void);
int caveclose (void);
int here (int);
int at (int);
int liqloc (int);
int weq (char *, char *);
int march (void);
void dstroy (int);
int score (void);
void move (int, int);
void datime (int *, int *);
int trtake (void);
int trdrop (void);
int trsay (void);
int tropen (void);
int trkill (void);
int trtoss (void);
int trfill (void);
int trfeed (void);
int liq (int);
int ran (int);
void carry (int, int);
void juggle (int);
int put (int, int, int);

#define voc     game.voc
#define rtext   game.rtext
#define mtext   game.mtext
#define ctext   game.ctext
#define cval    game.cval
#define ptext   game.ptext
#define ltext   game.ltext
#define stext   game.stext
#define travel  game.travel
#define atloc   game.atloc
#define plac    game.plac
#define fixd    game.fixd
#define fixed   game.fixed
#define actspk  game.actspk
#define cond    game.cond
#define hntmax  game.hntmax
#define hints   game.hints
#define hinted  game.hinted
#define hintlc  game.hintlc
#define place   game.place
#define prop    game.prop
#define plink   game.plink
#define abb     game.abb
#define dseen   game.dseen
#define dloc    game.dloc
#define odloc   game.odloc
#define tk      game.tk
#define loc     game.loc
#define newloc  game.newloc
#define oldloc  game.oldloc
#define oldlc2  game.oldlc2
#define wzdark  game.wzdark
#define gaveup  game.gaveup
#define kq      game.kq
#define k       game.k
#define k2      game.k2
#define verb    game.verb
#define obj     game.obj
#define spk     game.spk
#define blklin  game.blklin
#define saved   game.saved
#define savet   game.savet
#define mxscor  game.mxscor
#define latncy  game.latncy
#define clsses  game.clsses
#define maxtrs  game.maxtrs
#define tally   game.tally
#define tally2  game.tally2
#define keys    game.keys
#define lamp    game.lamp
#define grate   game.grate
#define cage    game.cage
#define rod     game.rod
#define rod2    game.rod2
#define steps   game.steps
#define bird    game.bird
#define door    game.door
#define pillow  game.pillow
#define snake   game.snake
#define fissur  game.fissur
#define tablet  game.tablet
#define clam    game.clam
#define oyster  game.oyster
#define magzin  game.magzin
#define dwarf   game.dwarf
#define knife   game.knife
#define food    game.food
#define bottle  game.bottle
#define water   game.water
#define oil     game.oil
#define plant   game.plant
#define plant2  game.plant2
#define axe     game.axe
#define mirror  game.mirror
#define dragon  game.dragon
#define chasm   game.chasm
#define troll   game.troll
#define troll2  game.troll2
#define bear    game.bear
#define messag  game.messag
#define vend    game.vend
#define batter  game.batter
#define nugget  game.nugget
#define coins   game.coins
#define chest   game.chest
#define eggs    game.eggs
#define tridnt  game.tridnt
#define vase    game.vase
#define emrald  game.emrald
#define pyram   game.pyram
#define pearl   game.pearl
#define rug     game.rug
#define chain   game.chain
#define spices  game.spices
#define back    game.back
#define look    game.look
#define cave    game.cave
#define null    game.null
#define entrnc  game.entrnc
#define dprssn  game.dprssn
#define say     game.say
#define lock    game.lock
#define throw   game.throw
#define find    game.find
#define invent  game.invent
#define chloc   game.chloc
#define chloc2  game.chloc2
#define dflag   game.dflag
#define daltlc  game.daltlc
#define stick   game.stick
#define dtotal  game.dtotal
#define attack  game.attack
#define turns   game.turns
#define lmwarn  game.lmwarn
#define iwest   game.iwest
#define knfloc  game.knfloc
#define detail  game.detail
#define abbnum  game.abbnum
#define maxdie  game.maxdie
#define numdie  game.numdie
#define holdng  game.holdng
#define dkill   game.dkill
#define foobar  game.foobar
#define bonus   game.bonus
#define clock1  game.clock1
#define clock2  game.clock2
#define closng  game.closng
#define panic   game.panic
#define closed  game.closed
#define scorng  game.scorng
#define newloc  game.newloc
#define limit   game.limit
