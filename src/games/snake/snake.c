/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * snake - crt hack game.
 *
 * You move around the screen with arrow keys trying to pick up money
 * without getting eaten by the snake.  hjkl work as in vi in place of
 * arrow keys.  You can leave at the exit any time.
 *
 * compile as follows:
 *	cc -O snake.c move.c -o snake -lm -ltermlib
 */

#include "snake.h"
#include <pwd.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/wait.h>

	/*
	 * If CHECKBUSY is defined, the file BUSY must be executable
	 * and must return a value which is used to determine the priority
	 * a which snake runs.  A zero value means no nice.
	 * If BUSY does not exist, snake won't play.
	 */
#ifndef BUSY
#ifdef CROSS
#define BUSY	"/usr/local/games/busy"
#else
#define BUSY	"/games/lib/busy"
#endif
#endif

	/*
	 * This is the data file for scorekeeping.
	 */
#ifndef SNAKERAWSCORES
#ifdef CROSS
#define SNAKERAWSCORES	"/usr/local/games/snakerawscores"
#else
#define SNAKERAWSCORES	"/games/lib/snakerawscores"
#endif
#endif

	/*
	 * If it exists, a log is kept here.  Otherwise it isn't.
	 */
#ifndef LOGFILE
#ifdef CROSS
#define LOGFILE	"/usr/local/games/snake.log"
#else
#define LOGFILE	"/games/lib/snake.log"
#endif
#endif

#define PENALTY  10	/* % penalty for invoking spacewarp	*/

#define EOT	'\004'
#define LF	'\n'
#define DEL	'\177'

#define ME		'I'
#define SNAKEHEAD	'S'
#define SNAKETAIL	's'
#define TREASURE	'$'
#define GOAL		'#'

#define BSIZE	80

struct point you;
struct point money;
struct point finish;
struct point snake[6];

int loot, penalty;
int long tl, tm=0L;
int argcount;
char **argval;
int moves;
static char str[BSIZE];
char stri[BSIZE];
char *p;
char ch, savec;
char *kl, *kr, *ku, *kd;
int repeat=1;
long tv;
char *tn;

int mx [8] = {
	0, 1, 1, 1, 0,-1,-1,-1
};

int my [8] = {
	-1,-1, 0, 1, 1, 1, 0,-1
};

float absv[8]= {
	1, 1.4, 1, 1.4, 1, 1.4, 1, 1.4
};

int oldw = 0;

void
drawbox()
{
	register int i;
	struct point p;

	p.line = -1;
	for (i= 0; i<ccnt; i++) {
		p.col = i;
		pchar(&p, '-');
	}
	p.col = ccnt;
	for (i= -1; i<=lcnt; i++) {
		p.line = i;
		pchar(&p, '|');
	}
	p.col = -1;
	for (i= -1; i<=lcnt; i++) {
		p.line = i;
		pchar(&p, '|');
	}
	p.line = lcnt;
	for (i= 0; i<ccnt; i++) {
		p.col = i;
		pchar(&p, '-');
	}
}

void
setup(){	/*
		 * setup the board
		 */
	int i;

	clear();
	pchar(&you,ME);
	pchar(&finish,GOAL);
	pchar(&money,TREASURE);
	for(i=1; i<6; i++) {
		pchar(&snake[i],SNAKETAIL);
	}
	pchar(&snake[0], SNAKEHEAD);
	drawbox();
	fflush(stdout);
}

void
randm(sp)
struct point *sp;
{
	register int issame;
	struct point p;
	register int i;

	sp->col = sp->line = -1;	/* impossible */
	do {
		issame = 0;
		p.col = random() % ccnt;
		p.line = random() % lcnt;

		/* make sure it's not on top of something else */
		if (p.line == 0 && p.col <5) issame++;
		if(same(&p, &you)) issame++;
		if(same(&p, &money)) issame++;
		if(same(&p, &finish)) issame++;
		for (i=0; i<5; i++)
			if(same(&p, &snake[i])) issame++;

	} while (issame);
	*sp = p;
}

/*
 * Flush typeahead to keep from buffering a bunch of chars and then
 * overshooting.  This loses horribly at 9600 baud, but works nicely
 * if the terminal gets behind.
 */
void
flushi()
{
#ifdef CROSS
        ioctl(0, TCSETAW, &newtty);
#else
        ioctl(0, TIOCSETP, &newtty);
#endif
//#ifdef TIOCFLUSH
//	ioctl(0, TIOCFLUSH, NULL);
//#endif
}

void
winnings(won)
int won;
{
	struct point p;

	p.line = p.col = 1;
	if(won>0){
		move(&p);
		print("$%d",won);
	}
}

void
spacewarp(w)
int w;
{
	struct point p;
	int j;

	randm(&you);
	point(&p,COLUMNS/2 - 8,LINES/2 - 1);
	if (p.col < 0)
		p.col = 0;
	if (p.line < 0)
		p.line = 0;
	if (w) {
		sprintf(str,"BONUS!!!");
		loot = loot - penalty;
		penalty = 0;
	} else {
		sprintf(str,"SPACE WARP!!!");
		penalty += loot/PENALTY;
	}
	for(j=0;j<3;j++){
		clear();
		delay(5);
		aprint(&p,str);
		delay(10);
	}
	setup();
	winnings(cashvalue);
}

int
removep(sp)
struct point *sp;
{
	int j;

	if (same(sp,&money)) {
		pchar(sp,TREASURE);
		return(2);
	}
	if (same(sp,&finish)) {
		pchar(sp,GOAL);
		return(3);
	}
	if (same(sp,&snake[0])) {
		pchar(sp,SNAKEHEAD);
		return(4);
	}
	for(j=1;j<6;j++){
		if(same(sp,&snake[j])){
			pchar(sp,SNAKETAIL);
			return(4);
		}
	}
	if ((sp->col < 4) && (sp->line == 0)){
		winnings(cashvalue);
		if((you.line == 0) && (you.col < 4)) pchar(&you,ME);
		return(5);
	}
	if (same(sp,&you)) {
		pchar(sp,ME);
		return(1);
	}
	pchar(sp,' ');
	return(0);
}

int
stretch(ps)
struct point *ps;
{
	struct point p;

	point(&p,you.col,you.line);
	if(abs(ps->col-you.col) < 6){
		if(you.line < ps->line){
			for (p.line = you.line+1;p.line <= ps->line;p.line++)
				pchar(&p,'v');
			delay(10);
			for (;p.line > you.line;p.line--)
				removep(&p);
		} else {
			for (p.line = you.line-1;p.line >= ps->line;p.line--)
				pchar(&p,'^');
			delay(10);
			for (;p.line < you.line;p.line++)
				removep(&p);
		}
		return(1);
	} else if(abs(ps->line-you.line) < 3){
		p.line = you.line;
		if(you.col < ps->col){
			for (p.col = you.col+1;p.col <= ps->col;p.col++)
				pchar(&p,'>');
			delay(10);
			for (;p.col > you.col;p.col--)
				removep(&p);
		} else {
			for (p.col = you.col-1;p.col >= ps->col;p.col--)
				pchar(&p,'<');
			delay(10);
			for (;p.col < you.col;p.col++)
				removep(&p);
		}
		return(1);
	}
	return(0);
}

void
snap()
{
	struct point p;

	if(you.line < 3){
		pchar(point(&p,you.col,0),'-');
	}
	if(you.line > lcnt-4){
		pchar(point(&p,you.col,lcnt-1),'_');
	}
	if(you.col < 10){
		pchar(point(&p,0,you.line),'(');
	}
	if(you.col > ccnt-10){
		pchar(point(&p,ccnt-1,you.line),')');
	}
	if (! stretch(&money)) {
                if (! stretch(&finish))
                        delay(10);
        }
	if(you.line < 3){
		point(&p,you.col,0);
		removep(&p);
	}
	if(you.line > lcnt-4){
		point(&p,you.col,lcnt-1);
		removep(&p);
	}
	if(you.col < 10){
		point(&p,0,you.line);
		removep(&p);
	}
	if(you.col > ccnt-10){
		point(&p,ccnt-1,you.line);
		removep(&p);
	}
	fflush(stdout);
}

void
win(ps)
struct point *ps;
{
	struct point x;
	int j,k;
	int boxsize;	/* actually diameter of box, not radius */

	boxsize = 10;
	point(&x,ps->col,ps->line);
	for(j=1;j<boxsize;j++){
		for(k=0;k<j;k++){
			pchar(&x,'#');
			x.line--;
		}
		for(k=0;k<j;k++){
			pchar(&x,'#');
			x.col++;
		}
		j++;
		for(k=0;k<j;k++){
			pchar(&x,'#');
			x.line++;
		}
		for(k=0;k<j;k++){
			pchar(&x,'#');
			x.col--;
		}
	}
	fflush(stdout);
}

void
suspend()
{

	cook();
#ifdef SIGTSTP
	kill(getpid(), SIGTSTP);
#else
        char *sh = getenv("SHELL");
	if (sh == NULL)
		sh = "/bin/sh";
	if (system(sh) < 0)
	        /*ignore*/;
#endif
	raw();
	setup();
	winnings(cashvalue);
}

void
length(num)
int num;
{
	print("You made %d moves.\n",num);
}

void
logit(msg)
char *msg;
{
	FILE *logfile;
	long t;

	if ((logfile=fopen(LOGFILE, "a")) != NULL) {
		time(&t);
		fprintf(logfile, "%s $%d %dx%d %s %s", getlogin(), cashvalue, lcnt, ccnt, msg, ctime(&t));
		fclose(logfile);
	}
}

int
post(iscore, flag)
int	iscore, flag;
{
	short	score = iscore;
	int	rawscores;
	short	uid;
	short	oldbest=0;
	short	allbwho=0, allbscore=0;
	struct	passwd *p, *getpwuid();

	/*
	 * Neg uid, 0, and 1 cannot have scores recorded.
	 */
	if ((uid=getuid()) > 1 && (rawscores=open(SNAKERAWSCORES,2))>=0) {
		/* Figure out what happened in the past */
		if (read(rawscores, &allbscore, sizeof(short)) < 0 ||
                    read(rawscores, &allbwho, sizeof(short)) < 0 ||
                    lseek(rawscores, ((long)uid)*sizeof(short), 0) < 0 ||
                    read(rawscores, &oldbest, sizeof(short)) < 0) {
			print("Error reading scores\n");
                }
		if (flag)
                        return (score > oldbest ? 1 : 0);

		/* Update this jokers best */
		if (score > oldbest) {
			if (lseek(rawscores, ((long)uid)*sizeof(short), 0) < 0 ||
                            write(rawscores, &score, sizeof(short)) < 0) {
                                print("Error writing scores\n");
                        }
			print("You bettered your previous best of $%d\n", oldbest);
		} else
			print("Your best to date is $%d\n", oldbest);

		/* See if we have a new champ */
		p = getpwuid(allbwho);
		if (p == NULL || score > allbscore) {
			if (lseek(rawscores, (long)0, 0) < 0 ||
                            write(rawscores, &score, sizeof(short)) < 0 ||
                            write(rawscores, &uid, sizeof(short)) < 0) {
                                print("Error writing scores.\n");
                        }
			if (p != NULL)
				print("You beat %s's old record of $%d!\n", p->pw_name, allbscore);
			else
				print("You set a new record!\n");
		} else
			print("The highest is %s with $%d\n", p->pw_name, allbscore);
		close(rawscores);
	} else
		if (!flag)
			print("Unable to post score.\n");
	return (1);
}

void
chase (np, sp)
struct point *sp, *np;
{
	/* this algorithm has bugs; otherwise the
	   snake would get too good */
	struct point d;
	int w, i, wt[8];
	double sqrt(), v1, v2, vp, max;
	point(&d,you.col-sp->col,you.line-sp->line);
	v1 = sqrt( (double) (d.col*d.col + d.line*d.line) );
	w=0;
	max=0;
	for(i=0; i<8; i++)
	{
		vp = d.col*mx[i] + d.line*my[i];
		v2 = absv[i];
		if (v1>0)
			vp = ((double)vp)/(v1*v2);
		else vp=1.0;
		if (vp>max)
		{
			max=vp;
			w=i;
		}
	}
	for(i=0; i<8; i++)
	{
		point(&d,sp->col+mx[i],sp->line+my[i]);
		wt[i]=0;
		if (d.col<0 || d.col>=ccnt || d.line<0 || d.line>=lcnt)
			continue;
		if (d.line == 0 && d.col < 5) continue;
		if (same(&d,&money)) continue;
		if (same(&d,&finish)) continue;
		wt[i]= i==w ? loot/10 : 1;
		if (i==oldw) wt [i] += loot/20;
	}
	for(w=i=0; i<8; i++)
		w+= wt[i];
	vp = random() % w;
	for(i=0; i<8; i++)
		if (vp <wt[i])
			break;
		else
			vp -= wt[i];
	if (i==8) {
		print("failure\n");
		i=0;
		while (wt[i]==0) i++;
	}
	oldw=w=i;
	point(np,sp->col+mx[w],sp->line+my[w]);
}

void
surround(ps)
struct point *ps;
{
	struct point x;
	int j;

	if(ps->col == 0)ps->col++;
	if(ps->line == 0)ps->line++;
	if(ps->line == LINES -1)ps->line--;
	if(ps->col == COLUMNS -1)ps->col--;
	aprint(point(&x,ps->col-1,ps->line-1),"/*\\\r* *\r\\*/");
	for (j=0;j<20;j++){
		pchar(ps,'@');
		delay(1);
		pchar(ps,' ');
		delay(1);
	}
	if (post(cashvalue,1)) {
		aprint(point(&x,ps->col-1,ps->line-1),"   \ro.o\r\\_/");
		delay(6);
		aprint(point(&x,ps->col-1,ps->line-1),"   \ro.-\r\\_/");
		delay(6);
	}
	aprint(point(&x,ps->col-1,ps->line-1),"   \ro.o\r\\_/");
}

int
pushsnake()
{
	int i, bonus;
	int issame = 0;

	/*
	 * My manual says times doesn't return a value.  Furthermore, the
	 * snake should get his turn every time no matter if the user is
	 * on a fast terminal with typematic keys or not.
	 * So I have taken the call to times out.
	 */
	for(i=4; i>=0; i--)
		if (same(&snake[i], &snake[5]))
			issame++;
	if (!issame)
		pchar(&snake[5],' ');
	for(i=4; i>=0; i--)
		snake[i+1]= snake[i];
	chase(&snake[0], &snake[1]);
	pchar(&snake[1],SNAKETAIL);
	pchar(&snake[0],SNAKEHEAD);
	for(i=0; i<6; i++)
	{
		if (same(&snake[i],&you))
		{
			surround(&you);
			i = (cashvalue) % 10;
			bonus = random() % 10;
			ll();
			print("%d\n", bonus);
			delay(30);
			if (bonus == i) {
				spacewarp(1);
				logit("bonus");
				flushi();
				return(1);
			}
			if ( loot >= penalty ){
				print("You and your $%d have been eaten\n",cashvalue);
			} else {
				print("The snake ate you.  You owe $%d.\n",-cashvalue);
			}
			logit("eaten");
			length(moves);
			done();
		}
	}
	return(0);
}

/* Main command loop */
void
mainloop()
{
	int j, k;

	for (;;) {
		int c, lastc = 0, match;

		/* Hask to place cursor over the player position. */
		you.col++; you.line++;
		move(&you);
		you.col--; you.line--;

		fflush(stdout);
		if (((c = getchar() & 0177) <= '9') && (c >= '0')) {
			ungetc(c,stdin);
			j = scanf("%d",&repeat);
			c = getchar() & 0177;
		} else {
			if (c != '.') repeat = 1;
		}
		if (c == '.') {
			c = lastc;
		}
		if ((Klength > 0) &&
		    (c == *KL || c == *KR || c == *KU || c == *KD)) {
			savec = c;
			match = 0;
			kl = KL;
			kr = KR;
			ku = KU;
			kd = KD;
			for (j=Klength;j>0;j--){
				if (match != 1) {
				match = 0;
					if (*kl++ == c) {
						ch = 'h';
						match++;
					}
					if (*kr++ == c) {
						ch = 'l';
						match++;
					}
					if (*ku++ == c) {
						ch = 'k';
						match++;
					}
					if (*kd++ == c) {
						ch = 'j';
						match++;
					}
					if (match == 0) {
						ungetc(c,stdin);
						ch = savec;
		/* Oops!
		 * This works if we figure it out on second character.
		 */
						break;
					}
				}
				savec = c;
				if(j != 1) c = getchar() & 0177;
			}
			c = ch;
		}
                flushi();
		lastc = c;
		switch (c){
		case CTRL('z'):
		case CTRL('c'):
			suspend();
			continue;
		case EOT:
		case 'x':
		case 0177:	/* del or end of file */
			ll();
			length(moves);
			logit("quit");
			done();
		case '!':
			cook();
			putchar('\n');
			putchar(c);
			fflush(stdout);
			j = read(0,stri,BSIZE);
			stri[j] = 0;
			if (fork() == 0) {
				if (setuid(getuid()) < 0)
                                        /*ignore*/;
				if (system(stri) < 0)
                                        /*ignore*/;
			} else
				wait(0);
			print("READY?\n");
			fflush(stdout);
			raw();
			c = getchar();
			ungetc(c,stdin);
			putpad(KS);
			putpad(TI);
			point(&cursor,0,lcnt-1);
		case CTRL('l'):
			setup();
			winnings(cashvalue);
			continue;
		case 'p':
		case 'd':
			snap();
			continue;
		case 'w':
			spacewarp(0);
			continue;
		case 'A':
			repeat = you.col;
			c = 'h';
			break;
		case 'H':
		case 'S':
			repeat = you.col - money.col;
			c = 'h';
			break;
		case 'T':
			repeat = you.line;
			c = 'k';
			break;
		case 'K':
		case 'E':
			repeat = you.line - money.line;
			c = 'k';
			break;
		case 'P':
			repeat = ccnt - 1 - you.col;
			c = 'l';
			break;
		case 'L':
		case 'F':
			repeat = money.col - you.col;
			c = 'l';
			break;
		case 'B':
			repeat = lcnt - 1 - you.line;
			c = 'j';
			break;
		case 'J':
		case 'C':
			repeat = money.line - you.line;
			c = 'j';
			break;
		}
		for(k=1;k<=repeat;k++){
			moves++;
			switch(c) {
			case 's':
			case 'h':
			case '\b':
				if (you.col >0) {
					pchar(&you,' ');
					you.col--;
					pchar(&you,ME);
				}
				break;
			case 'f':
			case 'l':
			case ' ':
				if (you.col < ccnt-1) {
					pchar(&you,' ');
					you.col++;
					pchar(&you,ME);
				}
				break;
			case CTRL('p'):
			case 'e':
			case 'k':
			case 'i':
				if (you.line > 0) {
					pchar(&you,' ');
					you.line--;
					pchar(&you,ME);
				}
				break;
			case CTRL('n'):
			case 'c':
			case 'j':
			case LF:
			case 'm':
				if (you.line+1 < lcnt) {
					pchar(&you,' ');
					you.line++;
					pchar(&you,ME);
				}
				break;
			}

			if (same(&you,&money)) {
				loot += 25;
				if(k < repeat)
					pchar(&you,' ');
				do {
					randm(&money);
				} while ((money.col == finish.col && money.line == finish.line) ||
					 (money.col < 5 && money.line == 0) ||
					 (money.col == you.col && money.line == you.line));
				pchar(&money,TREASURE);
				winnings(cashvalue);
				continue;
			}
			if (same(&you,&finish)) {
				win(&finish);
				ll();
				cook();
				print("You have won with $%d.\n",cashvalue);
				fflush(stdout);
				logit("won");
				post(cashvalue,0);
				length(moves);
				done();
			}
			if (pushsnake())
                                break;
		}
		fflush(stdout);
	}
}

void
busy()
{
#ifdef CHECKBUSY
	if (! strcmp (argval[0], "test")) return;
	FILE *pip;
	if ((access(BUSY,1) != 0) || (pip = popen(BUSY,"r")) == NULL){
		print("Sorry, no snake just now.\n");
		done();
	}
	int b;
	if (fscanf(pip, "%d", &b) != 1) {
		print("Sorry, busy file damaged.\n");
		done();
	}
	pclose(pip);
	if (b > 20) {
		print("Sorry, the system is too heavily loaded right now.\n");
		done();
	}
	if (nice(b) < 0)
	        /*ignore*/;
#endif
}

void
stop(int sig)
{
	signal(SIGINT, SIG_IGN);
	ll();
	length(moves);
	done();
}

int
main(argc,argv)
int argc;
char **argv;
{
	int i;
	char stdbuf[BUFSIZ];

	argcount = argc;
	argval = argv;
	penalty = loot = 0;
	getcap();
	ccnt -= 2; lcnt -= 2;	/* compensate for border */
	busy();
	time(&tv);

	for (i=1; i<argc; i++) {
		switch(argv[i][1]) {
		case 'd':
			sscanf(argv[1], "-d%ld", &tv);
			break;
		case 'w':	/* width */
		case 'c':	/* columns */
			ccnt = atoi(argv[i]+2);
			break;
		case 'l':	/* length */
		case 'h':	/* height */
		case 'r':	/* rows */
			lcnt = atoi(argv[i]+2);
			break;
		default:
			print("bad option %s\n", argv[1]);
		}
	}

	srandom((int)tv);
	setbuf(stdout, stdbuf);
	i = ((lcnt < ccnt) ? lcnt : ccnt);	/* min screen edge */
	if (i < 4) {
		print("Screen too small for a fair game\n");
		done();
	}
	/*
	 * chunk is the amount of money the user gets for each $.
	 * The formula below tries to be fair for various screen sizes.
	 * We only pay attention to the smaller of the 2 edges, since
	 * that seems to be the bottleneck.
	 * This formula is a hyperbola which includes the following points:
	 *	(24, $25)	(original scoring algorithm)
	 *	(12, $40)	(experimentally derived by the "feel")
	 *	(48, $15)	(a guess)
	 * This will give a 4x4 screen $99/shot.  We don't allow anything
	 * smaller than 4x4 because there is a 3x3 game where you can win
	 * an infinite amount of money.
	 */
	if (i < 12) i = 12;	/* otherwise it isn't fair */
	/*
	 * Compensate for border.  This really changes the game since
	 * the screen is two squares smaller but we want the default
	 * to be $25, and the high scores on small screens were a bit
	 * much anyway.
	 */
	i += 2;
	chunk = (675.0 / (i+6)) + 2.5;	/* min screen edge */

	signal (SIGINT, stop);
	putpad(TI); /*	String to begin programs that use cm */
	putpad(KS); /*	Put terminal in keypad transmit mode */

	randm(&finish);
	randm(&you);
	randm(&money);
	randm(&snake[0]);

	for(i=1;i<6;i++)
		chase (&snake[i], &snake[i-1]);
	setup();
	mainloop();
        return 0;
}
