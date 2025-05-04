/*
 * Assembler for BESM-6.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include "mesm6/a.out.h"

#define W       8               /* длина слова в байтах */

/* типы лексем */

#define LEOF    1
#define LEOL    2
#define LNAME   3
#define LCMD    4
#define LACMD   5
#define LNUM    6
#define LLCMD   7
#define LSCMD   8
#define LLSHIFT 9
#define LRSHIFT 10
#define LINCR   11
#define LDECR   12

/* номера сегментов */

#define SCONST  0
#define STEXT   1
#define SDATA   2
#define SSTRNG  3
#define SBSS    4
#define SEXT    6
#define SABS    7               /* вырожденный случай для getexpr */

/* директивы ассемблера */

#define ACOMM   0
#define ASCII   1
#define BSS     2
#define COMM    3
#define DATA    4
#define GLOBL   5
#define HALF    6
#define STRNG   7
#define TEXT    8
#define EQU     9
#define WORD    10

/* типы команд */

#define TLONG   01              /* длинноадресная команда */
#define TALIGN  02              /* после команды делать выравнивание */
#define TLIT    04              /* команда имеет литеральный аналог */
#define TINT    010             /* команда имеет целочисленный режим */
#define TCOMP   020             /* из команды можно сделать компонентную */

/* длины таблиц */
/* хэш-длины должны быть степенями двойки! */

#define HASHSZ  2048                    /* длина хэша таблицы имен */
#define HCONSZ  256                     /* длина хэша сегмента констант */
#define HCMDSZ  1024                    /* длина хэша команд ассемблера */

#define STSIZE  (HASHSZ*9/10)           /* длина таблицы имен */
#define CSIZE   (HCONSZ*9/10)           /* длина сегмента констант */
#define SPACESZ (STSIZE*8)              /* длина массива под имена */

#define SEGMTYPE(s)     segmtype[s]     /* номер сегмента в тип символа */
#define TYPESEGM(s)     typesegm[s]     /* тип символа в номер сегмента */
#define SEGMREL(s)      segmrel[s]      /* номер сегмента в тип настройки */

#define EMPCOM          0x3a00000L      /* пустая команда - заполнитель */
#define UTCCOM          0x3a00000L      /* команда <> */
#define WTCCOM          0x3b00000L      /* команда [] */

/* превращение команды в компонентную */

#define MAKECOMP(c)     ((c) & 0x2000000L ? (c)|0x4800000L : (c)|0x6000000L)

/* оптимальное значение хэш-множителя для 32-битного слова == 011706736335L */
/* то же самое для 16-битного слова = 067433 */

#define SUPERHASH(key,mask) (((short)(key) * (short)067433) & (short)(mask))

#define ISHEX(c)        (ctype[(c)&0377] & 1)
#define ISOCTAL(c)      (ctype[(c)&0377] & 2)
#define ISDIGIT(c)      (ctype[(c)&0377] & 4)
#define ISLETTER(c)     (ctype[(c)&0377] & 8)

FILE *sfile [SABS], *rfile [SABS];
long count [SABS];
short segm;
char *infile, *outfile = "a.out";
char tfilename[] = "/tmp/asXXXXXX";
short line;                             /* номер текущей строки */
short debug;                            /* флаг отладки */
short xflags, Xflag, uflag;
short stlength;                         /* длина таблицы символов в байтах */
short stalign;                          /* выравнивание таблицы символов */
long cbase, tbase, dbase, adbase, bbase;
struct nlist stab [STSIZE];
short stabfree;
char space [SPACESZ];                   /* место под имена символов */
short lastfree;                         /* счетчик занятого места */
short regleft;                          /* номер регистра слева от команды */
struct { long h, h2, hr2; } constab [CSIZE];
short nconst;
char name [256];
struct word { long left, right; } intval;
short extref;
short blexflag, backlex, blextype;
short hashtab [HASHSZ], hashctab [HCMDSZ];
short hashconst [HCONSZ];
short aflag;                            /* не выравнивать на границу слова */

int getlex(int *val);
long getexpr(int *s);

/* на втором проходе hashtab не нужна, используем ее под именем
/* newindex для переиндексации настройки в случае флагов x или X */

#define newindex hashtab

short ctype [256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,8,0,7,7,7,7,7,7,7,7,5,5,0,0,0,0,0,0,
    0,9,9,9,9,9,9,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,0,0,0,0,8,
    0,9,9,9,9,9,9,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,
    8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,0,
};

short segmtype [] = {   /* преобразование номера сегмента в тип символа */
    N_CONST,            /* SCONST */
    N_TEXT,             /* STEXT */
    N_DATA,             /* SDATA */
    N_STRNG,            /* SSTRNG */
    N_BSS,              /* SBSS */
    N_UNDF,             /* SEXT */
    N_ABS,              /* SABS */
};

short segmrel [] = {    /* преобразование номера сегмента в тип настройки */
    RCONST,             /* SCONST */
    RTEXT,              /* STEXT */
    RDATA,              /* SDATA */
    RSTRNG,             /* SSTRNG */
    RBSS,               /* SBSS */
    REXT,               /* SEXT */
    RABS,               /* SABS */
};

short typesegm [] = {   /* преобразование типа символа в номер сегмента */
    SEXT,               /* N_UNDF */
    SABS,               /* N_ABS */
    SCONST,             /* N_CONST */
    STEXT,              /* N_TEXT */
    SDATA,              /* N_DATA */
    SBSS,               /* N_BSS */
    SSTRNG,             /* N_STRNG */
};

/*
 * Следующие команды имеют разный смысл
 * в ассемблерах Рачинского и Балакирева:
 *
 *      вд, вр, врм, вч, пф, счк, ур
 *
 * Они вынесены в отдельную таблицу
 */
struct table {
    long val;
    char *name;
    short type;
} table [] = {
    0x0000000L,     "зп",           TLONG|TCOMP,
    0x0000000L,     "зч",           TLONG|TCOMP,
    0x0000000L,     "atx",          TLONG|TCOMP,
    0x0100000L,     "зм",           TLONG|TCOMP,
    0x0100000L,     "зпм",          TLONG|TCOMP,
    0x0100000L,     "stx",          TLONG|TCOMP,
    0x0200000L,     "сн",           TLONG|TLIT|TCOMP,
    0x0200000L,     "счн",          TLONG|TLIT|TCOMP,
    0x0200000L,     "xtna",         TLONG|TLIT|TCOMP,
    0x0200000L,     "xtra",         TLONG|TLIT|TCOMP,
    0x0300000L,     "см",           TLONG|TLIT|TCOMP,
    0x0300000L,     "счм",          TLONG|TLIT|TCOMP,
    0x0300000L,     "xts",          TLONG|TLIT|TCOMP,
    0x0400000L,     "ас",           TLONG|TLIT|TINT|TCOMP,
    0x0400000L,     "сл",           TLONG|TLIT|TINT|TCOMP,
    0x0400000L,     "aadx",         TLONG|TLIT|TINT|TCOMP,
    0x0400000L,     "apx",          TLONG|TLIT|TINT|TCOMP,
    0x0500000L,     "ав",           TLONG|TLIT|TINT|TCOMP,
    0x0500000L,     "вч",           TLONG|TLIT|TINT|TCOMP,
    0x0500000L,     "asux",         TLONG|TLIT|TINT|TCOMP,
    0x0500000L,     "asx",          TLONG|TLIT|TINT|TCOMP,
    0x0600000L,     "вчоб",         TLONG|TLIT|TINT|TCOMP,
    0x0600000L,     "ов",           TLONG|TLIT|TINT|TCOMP,
    0x0600000L,     "xsa",          TLONG|TLIT|TINT|TCOMP,
    0x0600000L,     "xsua",         TLONG|TLIT|TINT|TCOMP,
    0x0700000L,     "вчаб",         TLONG|TLIT|TCOMP,
    0x0700000L,     "мв",           TLONG|TLIT|TCOMP,
    0x0700000L,     "amsx",         TLONG|TLIT|TCOMP,
    0x0800000L,     "сч",           TLONG|TLIT|TCOMP,
    0x0800000L,     "xta",          TLONG|TLIT|TCOMP,
    0x0900000L,     "и",            TLONG|TLIT|TCOMP,
    0x0900000L,     "лу",           TLONG|TLIT|TCOMP,
    0x0900000L,     "aax",          TLONG|TLIT|TCOMP,
    0x0a00000L,     "нтж",          TLONG|TLIT|TCOMP,
    0x0a00000L,     "ср",           TLONG|TLIT|TCOMP,
    0x0a00000L,     "aex",          TLONG|TLIT|TCOMP,
    0x0b00000L,     "слк",          TLONG|TLIT|TCOMP,
    0x0b00000L,     "цс",           TLONG|TLIT|TCOMP,
    0x0b00000L,     "arx",          TLONG|TLIT|TCOMP,
    0x0c00000L,     "из",           TLONG|TLIT|TINT|TCOMP,
    0x0c00000L,     "avx",          TLONG|TLIT|TINT|TCOMP,
    0x0d00000L,     "или",          TLONG|TLIT|TCOMP,
    0x0d00000L,     "лс",           TLONG|TLIT|TCOMP,
    0x0d00000L,     "aox",          TLONG|TLIT|TCOMP,
    0x0e00000L,     "ад",           TLONG|TCOMP,
    0x0e00000L,     "дел",          TLONG|TCOMP,
    0x0e00000L,     "adx",          TLONG|TCOMP,
    0x0f00000L,     "ау",           TLONG|TLIT|TINT|TCOMP,
    0x0f00000L,     "умн",          TLONG|TLIT|TINT|TCOMP,
    0x0f00000L,     "amux",         TLONG|TLIT|TINT|TCOMP,
    0x0f00000L,     "amx",          TLONG|TLIT|TINT|TCOMP,
    0x1000000L,     "сбр",          TLONG|TLIT|TCOMP,
    0x1000000L,     "сб",           TLONG|TLIT|TCOMP,
    0x1000000L,     "apkx",         TLONG|TLIT|TCOMP,
    0x1100000L,     "рб",           TLONG|TLIT|TCOMP,
    0x1100000L,     "рзб",          TLONG|TLIT|TCOMP,
    0x1100000L,     "aux",          TLONG|TLIT|TCOMP,
    0x1200000L,     "чед",          TLONG|TLIT|TCOMP,
    0x1200000L,     "acx",          TLONG|TLIT|TCOMP,
    0x1300000L,     "вн",           TLONG|TLIT|TCOMP,
    0x1300000L,     "нед",          TLONG|TLIT|TCOMP,
    0x1300000L,     "anx",          TLONG|TLIT|TCOMP,
    0x1400000L,     "слпп",         TLONG|TCOMP,
    0x1400000L,     "сп",           TLONG|TCOMP,
    0x1400000L,     "eax",          TLONG|TCOMP,
    0x1400000L,     "epx",          TLONG|TCOMP,
    0x1500000L,     "вп",           TLONG|TCOMP,
    0x1500000L,     "вчпп",         TLONG|TCOMP,
    0x1500000L,     "esx",          TLONG|TCOMP,
    0x1600000L,     "сдпп",         TLONG|TCOMP,
    0x1600000L,     "ск",           TLONG|TCOMP,
    0x1600000L,     "asrx",         TLONG|TCOMP,
    0x1700000L,     "рк",           TLONG,
    0x1700000L,     "уржп",         TLONG,
    0x1700000L,     "xtr",          TLONG,
    0x1800000L,     "пб",           TLONG,
    0x1800000L,     "uj",           TLONG,
    0x1800000L,     "xj",           TLONG,
    0x1900000L,     "пв",           TLONG|TALIGN,
    0x1900000L,     "vjm",          TLONG|TALIGN,
    0x1a00000L,     "уц",           TLONG,
    0x1a00000L,     "циклу",        TLONG,
    0x1a00000L,     "vgm",          TLONG,
    0x1b00000L,     "кц",           TLONG,
    0x1b00000L,     "цикл",         TLONG,
    0x1b00000L,     "vlm",          TLONG,
    0x1e00000L,     "ок",           TLONG|TCOMP,
    0x1e00000L,     "сдлп",         TLONG|TCOMP,
    0x1e00000L,     "alx",          TLONG|TCOMP,
    0x1e00000L,     "aslx",         TLONG|TCOMP,
    0x2000000L,     "ир",           TLONG,
    0x2000000L,     "пирв",         TLONG,
    0x2000000L,     "vzm",          TLONG,
    0x2100000L,     "ин",           TLONG,
    0x2100000L,     "пинр",         TLONG,
    0x2100000L,     "vim",          TLONG,
    0x2200000L,     "ибр",          TLONG,
    0x2200000L,     "пибр",         TLONG,
    0x2200000L,     "vpzm",         TLONG,
    0x2300000L,     "име",          TLONG,
    0x2300000L,     "пимн",         TLONG,
    0x2300000L,     "vnm",          TLONG,
    0x2400000L,     "имр",          TLONG,
    0x2400000L,     "пимр",         TLONG,
    0x2400000L,     "vnzm",         TLONG,
    0x2500000L,     "иб",           TLONG,
    0x2500000L,     "пибл",         TLONG,
    0x2500000L,     "vpm",          TLONG,
    0x2800000L,     "прв",          TLONG,
    0x2800000L,     "uz",           TLONG,
    0x2800000L,     "xz",           TLONG,
    0x2900000L,     "пнр",          TLONG,
    0x2900000L,     "ун",           TLONG,
    0x2900000L,     "ui",           TLONG,
    0x2900000L,     "xi",           TLONG,
    0x2a00000L,     "пбр",          TLONG,
    0x2a00000L,     "убр",          TLONG,
    0x2a00000L,     "upz",          TLONG,
    0x2a00000L,     "xpz",          TLONG,
    0x2b00000L,     "пмн",          TLONG,
    0x2b00000L,     "уме",          TLONG,
    0x2b00000L,     "un",           TLONG,
    0x2b00000L,     "xn1",          TLONG,
    0x2c00000L,     "пмр",          TLONG,
    0x2c00000L,     "умр",          TLONG,
    0x2c00000L,     "unz",          TLONG,
    0x2c00000L,     "xnz",          TLONG,
    0x2d00000L,     "пбл",          TLONG,
    0x2d00000L,     "убл",          TLONG,
    0x2d00000L,     "up",           TLONG,
    0x2d00000L,     "xp1",          TLONG,
    0x2e00000L,     "пос",          TLONG,
    0x2e00000L,     "ус",           TLONG,
    0x2e00000L,     "uiv",          TLONG,
    0x2e00000L,     "xv1",          TLONG,
    0x2f00000L,     "пно",          TLONG,
    0x2f00000L,     "унс",          TLONG,
    0x2f00000L,     "uzv",          TLONG,
    0x2f00000L,     "xvz",          TLONG,
    0x3000000L,     "ст",           TLONG|TCOMP,
    0x3000000L,     "счт",          TLONG|TCOMP,
    0x3000000L,     "xtga",         TLONG|TCOMP,
    0x3000000L,     "xtwa",         TLONG|TCOMP,
    0x3100000L,     "сем",          TLONG|TCOMP,
    0x3100000L,     "сс",           TLONG|TCOMP,
    0x3100000L,     "xtqa",         TLONG|TCOMP,
    0x3100000L,     "xtsa",         TLONG|TCOMP,
    0x3200000L,     "сх",           TLONG,
    0x3200000L,     "счх",          TLONG,
    0x3200000L,     "xtha",         TLONG,
    0x3300000L,     "счк",          TLONG,
    0x3300000L,     "тс",           TLONG,
    0x3300000L,     "xtta",         TLONG,
    0x3400000L,     "зн",           TLONG|TCOMP,
    0x3400000L,     "зпн",          TLONG|TCOMP,
    0x3400000L,     "ztx",          TLONG|TCOMP,
    0x3500000L,     "зк",           TLONG,
    0x3500000L,     "зпк",          TLONG,
    0x3500000L,     "atcx",         TLONG,
    0x3600000L,     "зпх",          TLONG,
    0x3600000L,     "зх",           TLONG,
    0x3600000L,     "ath",          TLONG,
    0x3700000L,     "зпт",          TLONG|TCOMP,
    0x3700000L,     "зт",           TLONG|TCOMP,
    0x3700000L,     "atgx",         TLONG|TCOMP,
    0x3700000L,     "atwx",         TLONG|TCOMP,
    0x3800000L,     "где",          TLONG,
    0x3800000L,     "уа",           TLONG,
    0x3800000L,     "pctm",         TLONG,
    0x3800000L,     "vtdm",         TLONG,
    0x3900000L,     "ка",           TLONG,
    0x3900000L,     "пфс",          TLONG,
    0x3900000L,     "atc",          TLONG,
    0x3a00000L,     "пфа",          TLONG,
    0x3a00000L,     "utcs",         TLONG,
    0x3a00000L,     "xtpc",         TLONG,
    0x3b00000L,     "ик",           TLONG,
    0x3b00000L,     "пф",           TLONG,
    0x3b00000L,     "wtc",          TLONG,
    0x3b00000L,     "xtc",          TLONG,
    0x3c00000L,     "па",           TLONG,
    0x3c00000L,     "уиа",          TLONG,
    0x3c00000L,     "vtm",          TLONG,
    0x3d00000L,     "са",           TLONG,
    0x3d00000L,     "слиа",         TLONG,
    0x3d00000L,     "utm",          TLONG,
    0x3d00000L,     "xtm",          TLONG,
    0x3e00000L,     "груп",         TLONG,
    0x3e00000L,     "уг",           TLONG,
    0x3e00000L,     "do",           TLONG,
    0x3f00000L,     "эк",           TALIGN,
    0x3f00000L,     "ex",           TALIGN,
    0x3f01000L,     "вт",           TALIGN,
    0x3f01000L,     "выт",          TALIGN,
    0x3f01000L,     "pop",          TALIGN,
    0x3f02000L,     "врг",          0,
    0x3f02000L,     "чр",           0,
    0x3f02000L,     "rmod",         0,
    0x3f03000L,     "вых",          TALIGN,
    0x3f03000L,     "ij",           TALIGN,
    0x3f06000L,     "зр",           0,
    0x3f06000L,     "ург",          0,
    0x3f06000L,     "wmod",         0,
    0x3f07000L,     "ост",          TALIGN,
    0x3f07000L,     "стоп",         TALIGN,
    0x3f07000L,     "halt",         TALIGN,
    0x3f11000L,     "вдп",          0,
    0x3f11000L,     "мм",           0,
    0x3f11000L,     "yma",          0,
    0x3f14000L,     "кор",          0,
    0x3f14000L,     "кп",           0,
    0x3f14000L,     "ecn",          0,
    0x3f16000L,     "сдп",          0,
    0x3f16000L,     "сд",           0,
    0x3f16000L,     "asn",          0,
    0x3f18000L,     "врж",          0,
    0x3f18000L,     "rta",          0,
    0x3f19000L,     "вд",           0,
    0x3f19000L,     "мр",           0,
    0x3f19000L,     "yta",          0,
    0x3f1a000L,     "нс",           0,
    0x3f1a000L,     "нтжп",         0,
    0x3f1a000L,     "een",          0,
    0x3f1b000L,     "упр",          0,
    0x3f1b000L,     "уп",           0,
    0x3f1b000L,     "set",          0,
    0x3f1c000L,     "кс",           0,
    0x3f1c000L,     "слп",          0,
    0x3f1c000L,     "ean",          0,
    0x3f1d000L,     "вчп",          0,
    0x3f1d000L,     "кв",           0,
    0x3f1d000L,     "esn",          0,
    0x3f1e000L,     "од",           0,
    0x3f1e000L,     "сдл",          0,
    0x3f1e000L,     "aln",          0,
    0x3f1f000L,     "ра",           0,
    0x3f1f000L,     "урж",          0,
    0x3f1f000L,     "ntr",          0,
    0x3f20000L,     "уи",           0,
    0x3f20000L,     "ati",          0,
    0x3f21000L,     "уим",          0,
    0x3f21000L,     "ум",           0,
    0x3f21000L,     "sti",          0,
    0x3f22000L,     "ви",           0,
    0x3f22000L,     "ita",          0,
    0x3f23000L,     "виц",          0,
    0x3f23000L,     "вц",           0,
    0x3f23000L,     "iita",         0,
    0x3f24000L,     "пи",           0,
    0x3f24000L,     "уии",          0,
    0x3f24000L,     "mtj",          0,
    0x3f25000L,     "си",           0,
    0x3f25000L,     "сли",          0,
    0x3f25000L,     "jam",          0,
    0x3f26000L,     "вчиоб",        0,
    0x3f26000L,     "ми",           0,
    0x3f26000L,     "msj",          0,
    0x3f27000L,     "вчи",          0,
    0x3f27000L,     "им",           0,
    0x3f27000L,     "jsm",          0,
    0x3f28000L,     "иу",           0,
    0x3f28000L,     "ур",           0,
    0x3f28000L,     "ato",          0,
    0x3f29000L,     "му",           0,
    0x3f29000L,     "урм",          0,
    0x3f29000L,     "sto",          0,
    0x3f2a000L,     "вр",           0,
    0x3f2a000L,     "ив",           0,
    0x3f2a000L,     "ota",          0,
    0x3f2c000L,     "ип",           0,
    0x3f2c000L,     "ури",          0,
    0x3f2c000L,     "mto",          0,
    0x3f34000L,     "ца",           0,
    0x3f34000L,     "цела",         0,
    0x3f34000L,     "ent",          0,
    0x3f35000L,     "целф",         0,
    0x3f35000L,     "цф",           0,
    0x3f35000L,     "int",          0,
    0x3f36000L,     "мд",           0,
    0x3f36000L,     "сдпд",         0,
    0x3f36000L,     "asy",          0,
    0x3f38000L,     "кч",           0,
    0x3f38000L,     "уисч",         0,
    0x3f38000L,     "atia",         0,
    0x3f3c000L,     "инв",          0,
    0x3f3c000L,     "пд",           0,
    0x3f3c000L,     "aca",          0,
    0x3f3e000L,     "рд",           0,
    0x3f3e000L,     "сдлд",         0,
    0x3f3e000L,     "aly",          0,
    0x3f3f000L,     "лог",          0,
    0x3f3f000L,     "уу",           0,
    0x3f3f000L,     "tst",          0,
    0x3f51000L,     "вдпм",         0,
    0x3f51000L,     "ммм",          0,
    0x3f51000L,     "yms",          0,
    0x3f54000L,     "корм",         0,
    0x3f54000L,     "кпм",          0,
    0x3f54000L,     "ecns",         0,
    0x3f56000L,     "сдм",          0,
    0x3f56000L,     "сдпм",         0,
    0x3f56000L,     "asns",         0,
    0x3f58000L,     "вржм",         0,
    0x3f58000L,     "rts",          0,
    0x3f59000L,     "вдм",          0,
    0x3f59000L,     "мрм",          0,
    0x3f59000L,     "yts",          0,
    0x3f5a000L,     "нсм",          0,
    0x3f5a000L,     "нтжпм",        0,
    0x3f5a000L,     "eens",         0,
    0x3f5c000L,     "ксм",          0,
    0x3f5c000L,     "слпм",         0,
    0x3f5c000L,     "eans",         0,
    0x3f5d000L,     "вчпм",         0,
    0x3f5d000L,     "квм",          0,
    0x3f5d000L,     "esns",         0,
    0x3f5e000L,     "одм",          0,
    0x3f5e000L,     "сдлм",         0,
    0x3f5e000L,     "alns",         0,
    0x3f5f000L,     "рам",          0,
    0x3f5f000L,     "уржм",         0,
    0x3f5f000L,     "ntrs",         0,
    0x3f62000L,     "вим",          0,
    0x3f62000L,     "its",          0,
    0x3f63000L,     "вицм",         0,
    0x3f63000L,     "вцм",          0,
    0x3f63000L,     "iits",         0,
    0x3f6a000L,     "врм",          0,
    0x3f6a000L,     "ивм",          0,
    0x3f6a000L,     "ots",          0,
    0x3f74000L,     "цам",          0,
    0x3f74000L,     "целам",        0,
    0x3f74000L,     "ents",         0,
    0x3f75000L,     "целфм",        0,
    0x3f75000L,     "цфм",          0,
    0x3f75000L,     "ints",         0,
    0x3f76000L,     "мдм",          0,
    0x3f76000L,     "сдпдм",        0,
    0x3f76000L,     "asys",         0,
    0x3f78000L,     "кчм",          0,
    0x3f78000L,     "уисчм",        0,
    0x3f78000L,     "atis",         0,
    0x3f7c000L,     "инвм",         0,
    0x3f7c000L,     "пдм",          0,
    0x3f7c000L,     "acs",          0,
    0x3f7e000L,     "рдм",          0,
    0x3f7e000L,     "сдлдм",        0,
    0x3f7e000L,     "alys",         0,
    0x3f7f000L,     "логм",         0,
    0x3f7f000L,     "уум",          0,
    0x3f7f000L,     "tsts",         0,
    0x4000000L,     "счц",          TLONG,
    0x4000000L,     "xtal",         TLONG,
    0x4100000L,     "смц",          TLONG,
    0x4100000L,     "счцм",         TLONG,
    0x4100000L,     "xtsl",         TLONG,
    0x4200000L,     "снц",          TLONG,
    0x4200000L,     "счнц",         TLONG,
    0x4200000L,     "utra",         TLONG,
    0x4200000L,     "xtnal",        TLONG,
    0x4300000L,     "смл",          TLONG,
    0x4300000L,     "счлм",         TLONG,
    0x4300000L,     "uts",          TLONG,
    0x4300000L,     "xtsu",         TLONG,
    0x4400000L,     "асц",          TLONG,
    0x4400000L,     "слц",          TLONG,
    0x4400000L,     "aadu",         TLONG,
    0x4500000L,     "авц",          TLONG,
    0x4500000L,     "вчц",          TLONG,
    0x4500000L,     "asuu",         TLONG,
    0x4600000L,     "вчобц",        TLONG,
    0x4600000L,     "овц",          TLONG,
    0x4600000L,     "usua",         TLONG,
    0x4700000L,     "вчабц",        TLONG,
    0x4700000L,     "мвц",          TLONG,
    0x4700000L,     "amu",          TLONG,
    0x4800000L,     "счл",          TLONG,
    0x4800000L,     "uta",          TLONG,
    0x4800000L,     "xtau",         TLONG,
    0x4900000L,     "ил",           TLONG,
    0x4900000L,     "лул",          TLONG,
    0x4900000L,     "aau",          TLONG,
    0x4a00000L,     "нтжл",         TLONG,
    0x4a00000L,     "срл",          TLONG,
    0x4a00000L,     "aeu",          TLONG,
    0x4b00000L,     "слкл",         TLONG,
    0x4b00000L,     "цсл",          TLONG,
    0x4b00000L,     "aru",          TLONG,
    0x4c00000L,     "изц",          TLONG,
    0x4c00000L,     "avu",          TLONG,
    0x4d00000L,     "илил",         TLONG,
    0x4d00000L,     "лсл",          TLONG,
    0x4d00000L,     "aou",          TLONG,
    0x4f00000L,     "аул",          TLONG,
    0x4f00000L,     "умнц",         TLONG,
    0x4f00000L,     "amuu",         TLONG,
    0x5000000L,     "сбл",          TLONG,
    0x5000000L,     "сбрл",         TLONG,
    0x5000000L,     "apu",          TLONG,
    0x5100000L,     "рбл",          TLONG,
    0x5100000L,     "рзбл",         TLONG,
    0x5100000L,     "auu",          TLONG,
    0x5200000L,     "вчл",          TLONG,
    0x5200000L,     "чедл",         TLONG,
    0x5200000L,     "acu",          TLONG,
    0x5300000L,     "внл",          TLONG,
    0x5300000L,     "недл",         TLONG,
    0x5300000L,     "anu",          TLONG,
    0x5400000L,     "асф",          TLONG,
    0x5400000L,     "слф",          TLONG,
    0x5500000L,     "авф",          TLONG,
    0x5500000L,     "вчф",          TLONG,
    0x5600000L,     "вчобф",        TLONG,
    0x5600000L,     "овф",          TLONG,
    0x5700000L,     "вчабф",        TLONG,
    0x5700000L,     "мвф",          TLONG,
    0x5c00000L,     "изф",          TLONG,
    0x5f00000L,     "ауф",          TLONG,
    0x5f00000L,     "умнф",         TLONG,
    0x6000000L,     "зп.",          TLONG,
    0x6000000L,     "зчк",          TLONG,
    0x6000000L,     "atk",          TLONG,
    0x6100000L,     "змк",          TLONG,
    0x6100000L,     "зпм.",         TLONG,
    0x6100000L,     "stk",          TLONG,
    0x6200000L,     "снк",          TLONG,
    0x6200000L,     "счн.",         TLONG,
    0x6200000L,     "ktra",         TLONG,
    0x6300000L,     "смк",          TLONG,
    0x6300000L,     "счм.",         TLONG,
    0x6300000L,     "kts",          TLONG,
    0x6400000L,     "аск",          TLONG,
    0x6400000L,     "сл.",          TLONG,
    0x6400000L,     "aadk",         TLONG,
    0x6500000L,     "авк",          TLONG,
    0x6500000L,     "вч.",          TLONG,
    0x6500000L,     "asuk",         TLONG,
    0x6600000L,     "вчоб.",        TLONG,
    0x6600000L,     "овк",          TLONG,
    0x6600000L,     "ksua",         TLONG,
    0x6700000L,     "вчаб.",        TLONG,
    0x6700000L,     "мвк",          TLONG,
    0x6700000L,     "amk",          TLONG,
    0x6800000L,     "сч.",          TLONG,
    0x6800000L,     "kta",          TLONG,
    0x6900000L,     "и.",           TLONG,
    0x6900000L,     "лук",          TLONG,
    0x6900000L,     "aak",          TLONG,
    0x6a00000L,     "нтж.",         TLONG,
    0x6a00000L,     "срк",          TLONG,
    0x6a00000L,     "aek",          TLONG,
    0x6b00000L,     "слк.",         TLONG,
    0x6b00000L,     "цск",          TLONG,
    0x6b00000L,     "ark",          TLONG,
    0x6c00000L,     "изк",          TLONG,
    0x6c00000L,     "из.",          TLONG,
    0x6c00000L,     "avk",          TLONG,
    0x6d00000L,     "или.",         TLONG,
    0x6d00000L,     "лск",          TLONG,
    0x6d00000L,     "aok",          TLONG,
    0x6e00000L,     "адк",          TLONG,
    0x6e00000L,     "дел.",         TLONG,
    0x6e00000L,     "adk",          TLONG,
    0x6f00000L,     "аук",          TLONG,
    0x6f00000L,     "умн.",         TLONG,
    0x6f00000L,     "amuk",         TLONG,
    0x7000000L,     "сбк",          TLONG,
    0x7000000L,     "сбр.",         TLONG,
    0x7000000L,     "apk",          TLONG,
    0x7100000L,     "рбк.",         TLONG,
    0x7100000L,     "рзб.",         TLONG,
    0x7100000L,     "auk",          TLONG,
    0x7200000L,     "вчк",          TLONG,
    0x7200000L,     "чед.",         TLONG,
    0x7200000L,     "ack",          TLONG,
    0x7300000L,     "внк",          TLONG,
    0x7300000L,     "нед.",         TLONG,
    0x7300000L,     "ank",          TLONG,
    0x7400000L,     "слпп.",        TLONG,
    0x7400000L,     "спк",          TLONG,
    0x7400000L,     "eak",          TLONG,
    0x7500000L,     "впк",          TLONG,
    0x7500000L,     "вчпп.",        TLONG,
    0x7500000L,     "esk",          TLONG,
    0x7600000L,     "сдпп.",        TLONG,
    0x7600000L,     "скк",          TLONG,
    0x7600000L,     "ask",          TLONG,
    0x7800000L,     "стк",          TLONG,
    0x7800000L,     "счт.",         TLONG,
    0x7800000L,     "ktga",         TLONG,
    0x7900000L,     "сем.",         TLONG,
    0x7900000L,     "сск",          TLONG,
    0x7900000L,     "ktsa",         TLONG,
    0x7a00000L,     "схк",          TLONG,
    0x7a00000L,     "счх.",         TLONG,
    0x7a00000L,     "ktha",         TLONG,
    0x7b00000L,     "счк.",         TLONG,
    0x7b00000L,     "тск",          TLONG,
    0x7b00000L,     "ktta",         TLONG,
    0x7c00000L,     "знк",          TLONG,
    0x7c00000L,     "зпн.",         TLONG,
    0x7c00000L,     "ztk",          TLONG,
    0x7d00000L,     "зкк",          TLONG,
    0x7d00000L,     "зпк.",         TLONG,
    0x7d00000L,     "atck",         TLONG,
    0x7e00000L,     "окк",          TLONG,
    0x7e00000L,     "сдлп.",        TLONG,
    0x7e00000L,     "alk",          TLONG,
    0x7f00000L,     "зпт.",         TLONG,
    0x7f00000L,     "зтк",          TLONG,
    0x7f00000L,     "atgk",         TLONG,

    0,              0L,             0,
};

/*
 * Fatal error message.
 */
void uerror(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    fprintf(stderr, "as: ");
    if (infile)
        fprintf(stderr, "%s, ", infile);
    if (line)
        fprintf(stderr, "%d: ", line);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    exit(1);
}

void startup()
{
    register short i;

    int fd = mkstemp(tfilename);
    if (fd == -1) {
        uerror("cannot create temporary file %s", tfilename);
    } else {
        close(fd);
    }
    for (i=STEXT; i<SBSS; i++) {
        if (! (sfile [i] = fopen(tfilename, "w+")))
            uerror("cannot open %s", tfilename);
        unlink(tfilename);
        if (! (rfile [i] = fopen(tfilename, "w+")))
            uerror("cannot open %s", tfilename);
        unlink(tfilename);
    }
    line = 1;
}

int chash(char *s)
{
    register short h, c;

    h = 12345;
    while (c = *s++) h += h + c;
    return SUPERHASH(h, HCMDSZ-1);
}

void hashinit()
{
    register short i, h;
    register struct table *p;

    for (i=0; i<HCONSZ; i++)
        hashconst[i] = -1;
    for (i=0; i<HCMDSZ; i++)
        hashctab[i] = -1;
    for (p=table; p->name; p++) {
        h = chash(p->name);
        while (hashctab[h] != -1)
            if (--h < 0)
                h += HCMDSZ;
        hashctab[h] = p - table;
    }
    for (i=0; i<HASHSZ; i++)
        hashtab[i] = -1;
}

int hexdig(int c)
{
    if (c <= '9')
        return c - '0';
    else if (c <= 'F')
        return c - 'A' + 10;
    else
        return c - 'a' + 10;
}

void getlhex()
{
    register int c;
    register char *cp, *p;

    /* считать шестнадцатеричное число 'ZZZ */

    c = getchar();
    for (cp=name; ISHEX(c); c=getchar())
        *cp++ = hexdig(c);
    ungetc(c, stdin);
    intval.left = 0;
    intval.right = 0;
    p = name;
    for (c=28; c>=0; c-=4, ++p) {
        if (p >= cp)
            return;
        intval.left |= (long) *p << c;
    }
    for (c=28; c>=0; c-=4, ++p) {
        if (p >= cp)
            return;
        intval.right |= (long) *p << c;
    }
}

/*
 * считать шестнадцатеричное число 0xZZZ
 */
void gethnum()
{
    register int c;
    register char *cp;

    c = getchar();
    for (cp=name; ISHEX(c); c=getchar())
        *cp++ = hexdig(c);
    ungetc(c, stdin);
    intval.left = 0;
    intval.right = 0;
    for (c=0; c<32; c+=4) {
        if (--cp < name)
            return;
        intval.right |= (long) *cp << c;
    }
    for (c=0; c<32; c+=4) {
        if (--cp < name)
            return;
        intval.left |= (long) *cp << c;
    }
}

/*
 * Считать число:
 *      1234 1234d 1234D - десятичное
 *      01234 1234. 1234o 1234O - восьмеричное
 *      1234' 1234h 1234H - шестнадцатеричное
 */
void getnum(int c)
{
    register char *cp;
    int leadingzero;

    leadingzero = (c=='0');
    for (cp=name; ISHEX(c); c=getchar()) *cp++ = hexdig(c);
    intval.left = 0;
    intval.right = 0;
    if (c=='.' || c=='o' || c=='O') {
octal:
        for (c=0; c<=27; c+=3) {
            if (--cp < name) return;
            intval.right |= (long) *cp << c;
        }
        if (--cp < name) return;
        intval.right |= (long) *cp << 30;
        intval.left = (long) *cp >> 2;
        for (c=1; c<=31; c+=3) {
            if (--cp < name) return;
            intval.left |= (long) *cp << c;
        }
        return;
    } else if (c=='h' || c=='H' || c=='\'') {
        for (c=0; c<32; c+=4) {
            if (--cp < name) return;
            intval.right |= (long) *cp << c;
        }
        for (c=0; c<32; c+=4) {
            if (--cp < name) return;
            intval.left |= (long) *cp << c;
        }
        return;
    } else if (c!='d' && c!='D') {
        ungetc(c, stdin);
        if (leadingzero)
            goto octal;
    }
    for (c=1; ; c*=10) {
        if (--cp < name) return;
        intval.right += (long) *cp * c;
    }
}

/*
 * Считать число .[a:b], где a, b - номер бита
 * или .[a=b]
 */
void getbitmask()
{
    register int c, a, b;
    int v, compl;

    a = getexpr(&v) - 1;
    if (v != SABS)
        uerror("illegal expression in bit mask");
    c = getlex(&v);
    if (c != ':' && c != '=')
        uerror("illegal bit mask delimiter");
    compl = c == '=';
    b = getexpr(&v) - 1;
    if (v != SABS)
        uerror("illegal expression in bit mask");
    c = getlex(&v);
    if (c != ']')
        uerror("illegal bit mask delimiter");
    if (a<0 || a>=64 || b<0 || b>=64)
        uerror("bit number out of range 1..64");
    if (a < b)
        c = a, a = b, b = c;
    if (compl && --a < ++b) {
        intval.left = 0xffffffff;
        intval.right = 0xffffffff;
        return;
    }
    /* a greater than or equal to b */
    if (a >= 32) {
        if (b >= 32) {
            intval.left = (unsigned long) ~0L >> (63-a+b-32) << (b-32);
            intval.right = 0;
        } else {
            intval.left = (unsigned long) ~0L >> (63-a);
            intval.right = (unsigned long) ~0L << b;
        }
    } else {
        intval.left = 0;
        intval.right = (unsigned long) ~0L >> (31-a+b) << b;
    }
    intval.left &= 0xffffffff;
    intval.right &= 0xffffffff;
    if (compl) {
        intval.left ^= 0xffffffff;
        intval.right ^= 0xffffffff;
    }
}

/*
 * Считать число .N, где N - номер бита
 */
void getbitnum(int c)
{
    getnum(c);
    c = intval.right - 1;
    if (c < 0 || c >= 64)
        uerror("bit number out of range 1..64");
    if (c >= 32) {
        intval.left = 1 << (c-32);
        intval.right = 0;
    } else {
        intval.right = 1 << c;
        intval.left = 0;
    }
}

void getname(int c)
{
    register char *cp;

    for (cp=name; ISLETTER(c) || ISDIGIT(c); c=getchar())
        *cp++ = c;
    *cp = 0;
    ungetc(c, stdin);
}

int lookacmd()
{
    switch (name [1]) {
    case 'a':
        if (! strcmp(".ascii", name)) return ASCII;
        if (! strcmp(".acomm", name)) return ACOMM;
        break;
    case 'b':
        if (! strcmp(".bss", name)) return BSS;
        break;
    case 'c':
        if (! strcmp(".comm", name)) return COMM;
        break;
    case 'd':
        if (! strcmp(".data", name)) return DATA;
        break;
    case 'e':
        if (! strcmp(".equ", name)) return EQU;
        break;
    case 'g':
        if (! strcmp(".globl", name)) return GLOBL;
        break;
    case 'h':
        if (! strcmp(".half", name)) return HALF;
        break;
    case 's':
        if (! strcmp(".strng", name)) return STRNG;
        break;
    case 't':
        if (! strcmp(".text", name)) return TEXT;
        break;
    case 'w':
        if (! strcmp(".word", name)) return WORD;
        break;
    }
    return -1;
}

int lookcmd()
{
    register short i, h;

    h = chash(name);
    while ((i = hashctab[h]) != -1) {
        if (!strcmp(table[i].name, name))
            return i;
        if (--h < 0)
            h += HCMDSZ;
    }
    return -1;
}

int hash(char *s)
{
    register short h, c;

    h = 12345;
    while (c = *s++) h += h + c;
    return SUPERHASH(h, HASHSZ-1);
}

char *alloc(int len)
{
    register short r;

    r = lastfree;
    if ((lastfree += len) > SPACESZ)
        uerror("out of memory");
    return space + r;
}

int lookname()
{
    register short i, h;

    h = hash(name);
    while ((i = hashtab[h]) != -1) {
        if (!strcmp(stab[i].n_name, name))
            return i;
        if (--h < 0)
            h += HASHSZ;
    }

    /* занесение в таблицу нового символа */

    if ((i = stabfree++) >= STSIZE)
        uerror("symbol table overflow");
    stab[i].n_len = strlen(name);
    stab[i].n_name = alloc(1 + stab[i].n_len);
    strcpy(stab[i].n_name, name);
    stab[i].n_value = 0;
    stab[i].n_type = 0;
    hashtab[h] = i;
    return i;
}

/*
 * int getlex(int *val) - считать лексему, вернуть тип лексемы,
 * записать в *val ее значение.
 * Возвращаемые типы лексем:
 *      LEOL    - конец строки. Значение - номер начавшейся строки.
 *      LEOF    - конец файла.
 *      LNUM    - целое число. Значение - в intval, *val не определено.
 *      LCMD    - машинная команда. Значение - ее индекс в table.
 *      LNAME   - идентификатор. Значение - индекс в stab.
 *      LACMD   - инструкция ассемблера. Значение - тип.
 *      LLCMD   - длинноадресная команда. Значение - код.
 *      LSCMD   - короткоадресная команда. Значение - код.
 */
int getlex(int *pval)
{
    register short c;

    if (blexflag) {
        blexflag = 0;
        *pval = blextype;
        return backlex;
    }
    for (;;) switch (c = getchar()) {
    case ';':
skiptoeol:
        while ((c = getchar()) != '\n')
            if (c == EOF)
                return LEOF;
    case '\n':
        c = getchar();
        if (c == '#')
            goto skiptoeol;
        ungetc(c, stdin);
        *pval = ++line;
        return LEOL;
    case ' ':
    case '\t':
        continue;
    case EOF:
        return LEOF;
    case '\\':
        c = getchar();
        if (c=='<')
            return LLSHIFT;
        if (c=='>')
            return LRSHIFT;
        ungetc(c, stdin);
        return '\\';
    case '+':
        if ((c = getchar()) == '+')
            return LINCR;
        ungetc(c, stdin);
        return '+';
    case '-':
        if ((c = getchar()) == '-')
            return LINCR;
        ungetc(c, stdin);
        return '-';
    case '^':       case '&':       case '|':       case '~':
    case '#':       case '*':       case '/':       case '%':
    case '"':       case ',':       case '[':       case ']':
    case '(':       case ')':       case '{':       case '}':
    case '<':       case '>':       case '=':       case ':':
        return c;
    case '\'':
        getlhex(c);
        return LNUM;
    case '0':
        if ((c = getchar()) == 'x' || c=='X') {
            gethnum();
            return LNUM;
        }
        ungetc(c, stdin);
        c = '0';
    case '1':       case '2':       case '3':
    case '4':       case '5':       case '6':       case '7':
    case '8':       case '9':
        getnum(c);
        return LNUM;
    case '@':
    case '$':
        *pval = hexdig(getchar());
        *pval = *pval<<4 | hexdig(getchar());
        return (c == '$') ? LSCMD : LLCMD;
    default:
        if (!ISLETTER(c))
            uerror("bad character: \\%o", c & 0377);
        if (c=='.') {
            c = getchar();
            if (c == '[') {
                getbitmask();
                return LNUM;
            } else if (ISOCTAL(c)) {
                getbitnum(c);
                return LNUM;
            }
            ungetc(c, stdin);
            c = '.';
        }
        getname(c);
        if (name[0]=='.') {
            if (name[1]==0)
                return '.';
            if ((*pval = lookacmd()) != -1)
                return LACMD;
        }
        if ((*pval = lookcmd()) != -1)
            return LCMD;
        *pval = lookname();
        return LNAME;
    }
}

void ungetlex(int val, int type)
{
    blexflag = 1;
    backlex = val;
    blextype = type;
}

int getterm()
{
    register int ty;
    int cval, s;

    switch (getlex(&cval)) {
    default:
        uerror("operand missed");
    case LNUM:
        return SABS;
    case LNAME:
        intval.left = intval.right = 0;
        ty = stab[cval].n_type & N_TYPE;
        if (ty==N_UNDF || ty==N_COMM || ty==N_ACOMM) {
            extref = cval;
            return SEXT;
        }
        intval.right = stab[cval].n_value;
        return TYPESEGM(ty);
    case '.':
        intval.left = 0;
        intval.right = count[segm] / 2;
        return segm;
    case '(':
        getexpr(&s);
        if (getlex(&cval) != ')')
            uerror("bad () syntax");
        return s;
    case '{':
        /* обрезание порядка */
        getexpr(&s);
        if (getlex(&cval) != '}')
            uerror("bad () syntax");
        intval.left &= 07777777L;
        return s;
    }
}

/*
 * long getexpr(int *s) - считать выражение.
 * Вернуть значение, номер базового сегмента записать в *s.
 * Возвращаются 4 младших байта значения,
 * полная копия остается в intval.
 *
 * выражение    = [операнд] {операция операнд}...
 * операнд      = LNAME | LNUM | "." | "(" выражение ")" | "{" выражение "}"
 * операция     = "+" | "-" | "&" | "|" | "^" | "~" | "\" | "/" | "*" | "%"
 */
long getexpr(int *s)
{
    register short clex;
    int cval, s2;
    struct word rez;

    /* смотрим первую лексему */
    switch (clex = getlex(&cval)) {
    default:
        ungetlex(clex, cval);
        rez.left = rez.right = 0;
        *s = SABS;
        break;
    case LNUM:
    case LNAME:
    case '.':
    case '(':
    case '{':
        ungetlex(clex, cval);
        *s = getterm();
        rez = intval;
        break;
    }
    for (;;) {
        switch (clex = getlex(&cval)) {
            register long t;
        case '+':
            s2 = getterm();
            if (*s == SABS) *s = s2;
            else if (s2 != SABS)
                uerror("too complex expression");
            t = rez.right>>16 & 0xffff;
            rez.right &= 0xffff;
            rez.right += intval.right & 0xffff;
            if (rez.right & ~0xffff) t++;
            rez.right &= 0xffff;
            t += intval.right>>16 & 0xffff;
            rez.right |= t << 16;
            rez.left += intval.left;
            if (t & ~0xffff) rez.left++;
            break;
        case '-':
            s2 = getterm();
            if (s2 != SABS)
                uerror("too complex expression");
            t = rez.right>>16 & 0xffff;
            rez.right &= 0xffff;
            rez.right -= intval.right & 0xffff;
            if (rez.right & ~0xffff) t--;
            rez.right &= 0xffff;
            t -= intval.right>>16 & 0xffff;
            rez.right |= t << 16;
            rez.left -= intval.left;
            if (t & ~0xffff) rez.left--;
            break;
        case '&':
            s2 = getterm();
            if (*s != SABS || s2 != SABS)
                uerror("too complex expression");
            rez.left &= intval.left;
            rez.right &= intval.right;
            break;
        case '|':
            s2 = getterm();
            if (*s != SABS || s2 != SABS)
                uerror("too complex expression");
            rez.left |= intval.left;
            rez.right |= intval.right;
            break;
        case '^':
            s2 = getterm();
            if (*s != SABS || s2 != SABS)
                uerror("too complex expression");
            rez.left ^= intval.left;
            rez.right ^= intval.right;
            break;
        case '~':
            s2 = getterm();
            if (*s != SABS || s2 != SABS)
                uerror("too complex expression");
            rez.left ^= ~intval.left;
            rez.right ^= ~intval.right;
            break;
        case LLSHIFT:           /* сдвиг влево */
            s2 = getterm();
            if (*s != SABS || s2 != SABS)
                uerror("too complex expression");
            clex = intval.right & 077;
            if (clex<32) {
                rez.left <<= clex;
                rez.left |= rez.right >> (32-clex);
                rez.right <<= clex;
            } else {
                rez.left = rez.right << (clex-32);
                rez.right = 0;
            }
            break;
        case LRSHIFT:           /* сдвиг вправо */
            s2 = getterm();
            if (*s != SABS || s2 != SABS)
                uerror("too complex expression");
            clex = intval.right & 077;
            if (clex<32) {
                rez.right >>= clex;
                rez.right |= rez.left << (32-clex);
                rez.left >>= clex;
            } else {
                rez.right = rez.left >> (clex-32);
                rez.left = 0;
            }
            break;
        case '*':       /* 31-битная операция */
            s2 = getterm();
            if (*s != SABS || s2 != SABS)
                uerror("too complex expression");
            rez.left = 0;
            rez.right *= intval.right;
            break;
        case '/':       /* 31-битная операция */
            s2 = getterm();
            if (*s != SABS || s2 != SABS)
                uerror("too complex expression");
            rez.left = 0;
            if (intval.right)
                rez.right /= intval.right;
            else
                uerror("division by zero");
            break;
        case '%':       /* 31-битная операция */
            s2 = getterm();
            if (*s != SABS || s2 != SABS)
                uerror("too complex expression");
            rez.left = 0;
            if (intval.right)
                rez.right %= intval.right;
            else
                uerror("division (%%) by zero");
            break;
        default:
            ungetlex(clex, cval);
            intval = rez;
            return rez.right;
        }
    }
    /* NOTREACHED */
}

void puthr(long h, long r)
{
    static long sh, sr;

    if (count[segm] & 01) {
        fputh(h, sfile[segm]);
        fputh(r, rfile[segm]);
        fputh(sh, sfile[segm]);
        fputh(sr, rfile[segm]);
    } else {
        sh = h;
        sr = r;
    }
    count[segm]++;
}

void align(int s)
{
    register short save;

    if (s != segm) {
        save = segm;
        segm = s;
    } else save = -1;

    if (count[s] & 01)
        puthr(s==STEXT ? EMPCOM : 0L, (long) RABS);

    if (save >= 0)
        segm = save;
}

long enterconst(int bs)
{
    register short hash, i;
    register long h, h2, hr2;

    h = intval.left;
    h2 = intval.right;
    hr2 = SEGMREL(bs);
    if (bs == SEXT) hr2 |= RPUTIX(extref);
    hash = SUPERHASH(h+h2+hr2, HCONSZ-1);
    while ((i = hashconst[hash]) != -1) {
        if (h==constab[i].h && h2==constab[i].h2 && hr2==constab[i].hr2)
            return i;
        if (--hash < 0) hash += HCONSZ;
    }
    hashconst[hash] = nconst;
    constab[nconst].h = h;
    constab[nconst].h2 = h2;
    constab[nconst].hr2 = hr2;
    return nconst++;
}

void makecmd(long val, int type)
{
    register short clex, index, incr;
    register long addr, reltype;
    int cval, segment;

    index = regleft;
    reltype = RABS;
    for (;;) {
        switch (clex = getlex(&cval)) {
        case LEOF:
        case LEOL:
            ungetlex(clex, cval);
            addr = 0;
            goto putcom;
        case '#':
            getexpr(&segment);
            if (type & TLIT) {
                addr = intval.right >> 19 & 017777;
                if (type & TINT) {
                    if (!addr && !intval.left && !(intval.left>>16) ||
                        addr==017777 && intval.left==0xfffff)
                    {
                        addr = intval.right & 0xfffff;
                        val |= 0x4000000;
                        reltype = SEGMREL(segment);
                        if (reltype == REXT)
                            reltype |= RPUTIX(extref);
                        break;
                    }
                } else {
                    if (!addr && !intval.left && !(intval.left>>16) ||
                        addr==017777 && intval.left==0xffffffff)
                    {
                        addr = intval.right & 0xfffff;
                        val |= 0x4000000;
                        reltype = SEGMREL(segment);
                        if (reltype == REXT)
                            reltype |= RPUTIX(extref);
                        break;
                    }
                }
            }
            addr = enterconst(segment);
            reltype = RCONST;
            break;
        case '[':
            makecmd(WTCCOM, TLONG);
            if (getlex(&cval) != ']')
                uerror("bad [] syntax");
            continue;
        case '<':
            makecmd(UTCCOM, TLONG);
            if (getlex(&cval) != '>')
                uerror("bad <> syntax");
            continue;
        default:
            ungetlex(clex, cval);
            addr = getexpr(&segment);
            reltype = SEGMREL(segment);
            if (reltype == REXT)
                reltype |= RPUTIX(extref);
            break;
        }
        break;
    }
    if ((clex = getlex(&cval)) == ',') {
        index = getexpr(&segment);
        if (segment != SABS)
            uerror("bad register number");
        if ((type & TCOMP) && addr==0 && reltype==RABS) {
            if ((clex = getlex(&cval)) == LINCR || clex==LDECR) {
                incr = getexpr(&segment);
                if (segment != SABS)
                    uerror("bad register increment");
                if (incr == 0)
                    incr = 1;
                /* делаем компонентную команду */
                addr = clex==LINCR ? incr : -incr;
                val = MAKECOMP(val);
            } else
                ungetlex(clex, cval);
        }
    } else
        ungetlex(clex, cval);
putcom:
    if (type & TLONG) {
        if (reltype & REXT == REXT &&
            stab[RGETIX(reltype)].n_type == N_EXT+N_ACOMM)
        {
            /* если команда ссылается на ACOMM,
            /* вставляем перед нею utc */
            puthr((long) index<<28 | UTCCOM | addr>>12 & 0xfffff,
                reltype | RSHIFT);
            puthr(val | addr&07777, (long) RABS | RTRUNC);
        } else {
            addr &= 0xfffff;
            puthr((long) index<<28 | val | addr & 0xfffff,
                reltype | RLONG);
        }
    } else {
        puthr((long) index<<28 | val | addr & 07777,
            reltype | RSHORT);
    }
    if (! aflag && (type & TALIGN))
        align(segm);
}

void makeascii()
{
    register short c, n;
    int cval;

    c = getlex(&cval);
    if (c != '"')
        uerror("no .ascii parameter");
    n = 0;
    for (;;) {
        switch (c = getchar()) {
        case EOF:
            uerror("EOF in text string");
        case '"':
            break;
        case '\\':
            switch (c = getchar()) {
            case EOF:
                uerror("EOF in text string");
            case '\n':
                continue;
            case '0': case '1': case '2': case '3':
            case '4': case '5': case '6': case '7':
                cval = c & 07;
                c = getchar();
                if (c>='0' && c<='7') {
                    cval = cval<<3 | c&7;
                    c = getchar();
                    if (c>='0' && c<='7') {
                        cval = cval<<3 | c&7;
                    } else ungetc(c, stdin);
                } else ungetc(c, stdin);
                c = cval;
                break;
            case 't':
                c = '\t';
                break;
            case 'b':
                c = '\b';
                break;
            case 'r':
                c = '\r';
                break;
            case 'n':
                c = '\n';
                break;
            case 'f':
                c = '\f';
                break;
            }
        default:
            fputc(c, sfile[segm]);
            n++;
            continue;
        }
        break;
    }
    c = W - n % W;
    n = (n + c) / (W/2);
    count[segm] += n;
    while (c--)
        fputc(0, sfile[segm]);
    while (n--)
        fputh(0L, rfile[segm]);
}

void pass1()
{
    register short clex;
    int cval, tval, csegm;
    register long addr;

    segm = STEXT;
    while ((clex = getlex(&cval)) != LEOF) {
        switch (clex) {
        case LEOF:
            return;
        case LEOL:
            regleft = 0;
            continue;
        case ':':
            align(segm);
            continue;
        case LNUM:
            ungetlex(clex, cval);
            getexpr(&cval);
            if (cval != SABS)
                uerror("bad register number");
            regleft = intval.right & 017;
            continue;
        case LCMD:
            makecmd(table[cval].val, table[cval].type);
            break;
        case LSCMD:
            makecmd((long) cval<<12 | 0x3f00000L, 0);
            break;
        case LLCMD:
            makecmd((long) cval<<20, TLONG);
            break;
        case '.':
            if (getlex(&cval) != '=')
                uerror("bad command");
            align(segm);
            addr = 2 * getexpr(&csegm);
            if (csegm != segm)
                uerror("bad count assignment");
            if (addr < count[segm])
                uerror("negative count increment");
            if (segm == SBSS)
                count [segm] = addr;
            else while (count[segm] < addr) {
                fputh(segm==STEXT? EMPCOM: 0L, sfile[segm]);
                fputh(0L, rfile[segm]);
                count[segm]++;
            }
            break;
        case LNAME:
            if ((clex = getlex(&tval)) == ':') {
                align(segm);
                stab[cval].n_value = count[segm] / 2;
                stab[cval].n_type &= ~N_TYPE;
                stab[cval].n_type |= SEGMTYPE(segm);
                continue;
            } else if (clex=='=' || clex==LACMD && tval==EQU) {
                stab[cval].n_value = getexpr(&csegm);
                if (csegm == SEXT)
                    uerror("indirect equivalence");
                stab[cval].n_type &= N_EXT;
                stab[cval].n_type |= SEGMTYPE(csegm);
                break;
            } else if (clex==LACMD && (tval==COMM || tval==ACOMM)) {
                /* name .comm len */
                if (stab[cval].n_type != N_UNDF &&
                    stab[cval].n_type != (N_EXT|N_COMM) &&
                    stab[cval].n_type != (N_EXT|N_ACOMM))
                    uerror("name already defined");
                stab[cval].n_type = N_EXT | (tval==COMM ?
                    N_COMM : N_ACOMM);
                getexpr(&tval);
                if (tval != SABS)
                    uerror("bad length .comm");
                stab[cval].n_value = intval.right;
                break;
            }
            uerror("bad command");
        case LACMD:
            switch (cval) {
            case TEXT:
                segm = STEXT;
                break;
            case DATA:
                segm = SDATA;
                break;
            case STRNG:
                segm = SSTRNG;
                break;
            case BSS:
                segm = SBSS;
                break;
            case HALF:
                for (;;) {
                    getexpr(&cval);
                    addr = SEGMREL(cval);
                    if (cval == SEXT)
                        addr |= RPUTIX(extref);
                    puthr(intval.right, addr);
                    if ((clex = getlex(&cval)) != ',') {
                        ungetlex(clex, cval);
                        break;
                    }
                }
                break;
            case WORD:
                align(segm);
                for (;;) {
                    getexpr(&cval);
                    addr = SEGMREL(cval);
                    if (cval == SEXT)
                        addr |= RPUTIX(extref);
                    fputh(intval.right, sfile[segm]);
                    fputh(addr, rfile[segm]);
                    fputh(intval.left, sfile[segm]);
                    fputh(0L, rfile[segm]);
                    count[segm] += 2;
                    if ((clex = getlex(&cval)) != ',') {
                        ungetlex(clex, cval);
                        break;
                    }
                }
                break;
            case ASCII:
                align(segm);
                makeascii();
                break;
            case GLOBL:
                for (;;) {
                    if ((clex = getlex(&cval)) != LNAME)
                        uerror("bad parameter .globl");
                    stab[cval].n_type |= N_EXT;
                    if ((clex = getlex(&cval)) != ',') {
                        ungetlex(clex, cval);
                        break;
                    }
                }
                break;
            case COMM:
            case ACOMM:
                /* .comm name,len */
                tval = cval;
                if (getlex(&cval) != LNAME)
                    uerror("bad parameter .comm");
                if (stab[cval].n_type != N_UNDF &&
                    stab[cval].n_type != (N_EXT|N_COMM) &&
                    stab[cval].n_type != (N_EXT|N_ACOMM))
                    uerror("name already defined");
                stab[cval].n_type = N_EXT | (tval==COMM ?
                    N_COMM : N_ACOMM);
                if ((clex = getlex(&tval)) == ',') {
                    getexpr(&tval);
                    if (tval != SABS)
                        uerror("bad length .comm");
                } else {
                    ungetlex(clex, cval);
                    intval.right = 1;
                }
                stab[cval].n_value = intval.right;
                break;
            }
            break;
        default:
            uerror("bad syntax");
        }
        if ((clex = getlex(&cval)) != LEOL)
            if (clex == LEOF) return;
            else uerror("bad command end");
        regleft = 0;
    }
}

void middle()
{
    register short i, snum;

    align(STEXT);
    align(SDATA);
    align(SSTRNG);
    stlength = 0;
    for (snum=0, i=0; i<stabfree; i++) {
        /* если не установлен флаг uflag,
        /* неопределенное имя считается внешним */
        if (stab[i].n_type == N_UNDF)
            if (uflag)
                uerror("name undefined", stab[i].n_name);
            else stab[i].n_type |= N_EXT;
        if (xflags) newindex[i] = snum;
        if (!xflags || (stab[i].n_type & N_EXT) || Xflag &&
            stab[i].n_name[0] != 'L')
        {
            stlength += 2 + W/2 + stab[i].n_len;
            snum++;
        }
    }
    stalign = W - stlength % W;
    stlength += stalign;
}

void makeheader()
{
    struct exec hdr;

    hdr.a_magic = FMAGIC;
    hdr.a_const = nconst * W;
    hdr.a_text = count [STEXT] * (W/2);
    hdr.a_data = (count [SDATA] + count [SSTRNG]) * (W/2);
    hdr.a_bss = count [SBSS] * (W/2);
    hdr.a_syms = stlength;
    hdr.a_entry = HDRSZ/W + count [SCONST] / (W/2);
    hdr.a_flag = 0;
    fputhdr(&hdr, stdout);
}

long adjust(long h, long a, int hr)
{
    switch (hr & RSHORT) {
    case 0:
        a += h & 0777777777;
        h &= ~0777777777;
        h |= a & 0777777777;
        break;
    case RSHORT:
        a += h & 07777;
        h &= ~07777;
        h |= a & 07777;
        break;
    case RSHIFT:
        a >>= 12;
        goto rlong;
    case RTRUNC:
        a &= 07777;
    case RLONG:
rlong:
        a += h & 0xfffff;
        h &= ~0xfffff;
        h |= a & 0xfffff;
        break;
    }
    return h;
}

long makehalf(long h, long hr)
{
    register short i;

    switch ((int) hr & REXT) {
    case RABS:
        break;
    case RCONST:
        h = adjust(h, cbase, (int) hr);
        break;
    case RTEXT:
        h = adjust(h, tbase, (int) hr);
        break;
    case RDATA:
        h = adjust(h, dbase, (int) hr);
        break;
    case RSTRNG:
        h = adjust(h, adbase, (int) hr);
        break;
    case RBSS:
        h = adjust(h, bbase, (int) hr);
        break;
    case REXT:
        i = RGETIX(hr);
        if (stab[i].n_type != N_EXT+N_UNDF &&
            stab[i].n_type != N_EXT+N_COMM &&
            stab[i].n_type != N_EXT+N_ACOMM)
            h = adjust(h, stab[i].n_value, (int) hr);
        break;
    }
    return h;
}

void pass2()
{
    register short i;
    register long h;

    cbase = HDRSZ/W;
    tbase = cbase + nconst;
    dbase = tbase + count[STEXT]/2;
    adbase = dbase + count[SDATA]/2;
    bbase = adbase + count[SSTRNG]/2;

    /* обработка таблицы символов */
    for (i=0; i<stabfree; i++) {
        h = stab[i].n_value;
        switch (stab[i].n_type & N_TYPE) {
        case N_UNDF:
        case N_ABS:
            break;
        case N_CONST:
            h = adjust(h, cbase, 0);
            break;
        case N_TEXT:
            h = adjust(h, tbase, 0);
            break;
        case N_DATA:
            h = adjust(h, dbase, 0);
            break;
        case N_STRNG:
            h = adjust(h, adbase, 0);
            stab[i].n_type += N_DATA - N_STRNG;
            break;
        case N_BSS:
            h = adjust(h, bbase, 0);
            break;
        }
        stab[i].n_value = h;
    }
    /* обработка сегмента констант */
    for (i=0; i<nconst; i++) {
        fputh(makehalf(constab[i].h2, constab[i].hr2), stdout);
        fputh(constab[i].h, stdout);
    }
    for (segm=STEXT; segm<SBSS; segm++) {
        rewind(sfile [segm]);
        rewind(rfile [segm]);
        h = count [segm];
        while (h--) fputh(makehalf(fgeth(sfile[segm]),
            fgeth(rfile[segm])), stdout);
    }
}

/*
 * преобразование типа символа в тип настройки
 */
int typerel(int t)
{
    switch (t & N_TYPE) {
    case N_ABS:     return RABS;
    case N_CONST:   return RCONST;
    case N_TEXT:    return RTEXT;
    case N_DATA:    return RDATA;
    case N_BSS:     return RBSS;
    case N_STRNG:   return RDATA;
    case N_UNDF:
    case N_COMM:
    case N_ACOMM:
    case N_FN:
    default:        return 0;
    }
}

long relhalf(hr)
register long hr;
{
    register short i;

    switch ((int) hr & REXT) {
    case RSTRNG:
        hr = RDATA | hr & RSHORT;
        break;
    case REXT:
        i = RGETIX(hr);
        if (stab[i].n_type == N_EXT+N_UNDF ||
            stab[i].n_type == N_EXT+N_COMM ||
            stab[i].n_type == N_EXT+N_ACOMM)
        {
            /* переиндексация */
            if (xflags)
                hr = hr & (RSHORT|REXT) | RPUTIX(newindex [i]);
        } else
            hr = hr & RSHORT | typerel((int) stab[i].n_type);
        break;
    }
    return hr;
}

void makereloc()
{
    register short i;
    register long len;

    for (i=0; i<nconst; i++) {
        fputh(relhalf(constab[i].hr2), stdout);
        fputh(0L, stdout);
    }
    for (segm=STEXT; segm<SBSS; segm++) {
        rewind(rfile [segm]);
        len = count [segm];
        while (len--) fputh(relhalf(fgeth(rfile[segm])), stdout);
    }
}

void makesymtab()
{
    register short i;

    for (i=0; i<stabfree; i++)
        if (!xflags || stab[i].n_type & N_EXT || Xflag &&
            stab[i].n_name[0] != 'L')
            fputsym(&stab[i], stdout);
    while (stalign--)
        putchar(0);
}

void usage()
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "    mkb-as [-uaxXd] [-o outfile] [infile]\n");
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "    -o filename     Set output file name, default stdout\n");
    fprintf(stderr, "    -u              Treat undefined names as error\n");
    fprintf(stderr, "    -a              Don't align on word boundary\n");
    fprintf(stderr, "    -x              Discard local symbols\n");
    fprintf(stderr, "    -X              Discard locals starting with 'L' or '.'\n");
    fprintf(stderr, "    -d              Debug mode\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    register short i;
    register char *cp;
    int ofile = 0;

    /* разбор флагов */

    for (i=1; i<argc; i++) {
        switch (argv[i][0]) {
        case '-':
            for (cp=argv[i]; *cp; cp++) {
                switch (*cp) {
                case 'd':       /* флаг отладки */
                    debug++;
                    break;
                case 'X':
                    Xflag++;
                case 'x':
                    xflags++;
                    break;
                case 'a':       /* не выравнивать на границу слова */
                    aflag++;
                    break;
                case 'u':
                    uflag++;
                    break;
                case 'o':       /* выходной файл */
                    if (ofile)
                        uerror("too many -o flags");
                    ofile = 1;
                    if (cp [1]) {
                        /* -ofile */
                        outfile = cp+1;
                        while (*++cp);
                        --cp;
                    } else if (i+1 < argc)
                        /* -o file */
                        outfile = argv[++i];
                    break;
                default:
                    fprintf(stderr, "Unknown option: %s\n", cp);
                    usage();
                }
            }
            break;
        default:
            if (infile)
                uerror("too many input files");
            infile = argv[i];
            break;
        }
    }
    if (! infile && isatty(0))
        usage();

    /* настройка ввода-вывода */

    if (infile && ! freopen(infile, "r", stdin))
        uerror("cannot open %s", infile);
    if (! freopen(outfile, "w", stdout))
        uerror("cannot open %s", outfile);

    i = getchar();
    ungetc(i=='#' ? ';' : i, stdin);

    startup();          /* открытие временных файлов */
    hashinit();         /* инициализация хэш-таблиц */
    pass1();            /* первый проход */
    middle();           /* промежуточные действия */
    makeheader();       /* запись заголовка */
    pass2();            /* второй проход */
    makereloc();        /* запись файлов настройки */
    makesymtab();       /* запись таблицы символов */
    return 0;
}
