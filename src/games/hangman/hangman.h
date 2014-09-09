#include <stdio.h>
#include <ctype.h>
#include <curses.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#define MINLEN	6
#define MAXERRS	7
#ifdef CROSS
#   define DICT	"/usr/local/share/dict/words"
#else
#   define DICT	"/share/dict/words"
#endif

#define MESGY	12
#define MESGX	0
#define PROMPTY	11
#define PROMPTX	0
#define KNOWNY	10
#define KNOWNX	1
#define NUMBERY	4
#define NUMBERX	(COLS - 1 - 26)
#define AVGY	5
#define AVGX	(COLS - 1 - 26)
#define GUESSY	2
#define GUESSX	(COLS - 1 - 26)

typedef struct {
	short	y, x;
	char	ch;
} ERR_POS;

extern bool	Guessed[];

extern char	Word[], Known[], *Noose_pict[];

extern int	Errors, Wordnum;

extern double	Average;

extern ERR_POS	Err_pos[];

extern FILE	*Dict;

extern off_t	Dict_size;

void	die(int);

off_t	offabs(off_t);
void    prman(void);
void    prword(void);
void    prdata(void);
void    setup(void);
void    playgame(void);
void    getword(void);
void    getguess(void);
void    endgame(void);
int     readch(void);
