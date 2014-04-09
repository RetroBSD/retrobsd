/*
 * func2.c	Phantasia support routines
 */

#include "phant.h"

static void
voidupdate(vp,loc)			/* update an energy void */
        register struct nrgvoid	*vp;
        register int	loc;
{
        FILE	*fp;

	fp = fopen(voidfile,ACCESS);
	fseek(fp,(long) loc*sizeof(*vp),0);
	fwrite((char *) vp,sizeof(*vp),1,fp);
	fclose(fp);
}

static void
statread(stat,loc)			/* read a charac. structure */
        register struct	stats	*stat;
        register int	loc;
{
        FILE	*fp;

	fp = fopen(peoplefile,"r");
	fseek(fp,(long) loc * sizeof(*stat),0);
	if (fread((char *) stat,sizeof(*stat),1,fp) != 1)
	        /*ignore*/;
	fclose(fp);
}

static void
tampered(stat,what,bufp)			/* decree'd, intervened, etc. */
        register struct	stats	*stat, *bufp;
        int	what;
{
        struct	nrgvoid	vbuf;
        register int	loc;
        struct	stats	sbuf;
        FILE	*fp;

	changed = TRUE;
	move(6,0);
	stat->tampered = OFF;
	switch ((int) what)
		{
		case NRGVOID:
			addstr("You've hit an energy void !\n");
			stat->man /= 3;
			stat->nrg /= 2;
			stat->gld = floor(stat->gld/1.25) + 0.1;
			stat->x += 10;
			break;
		case TRANSPORT:
			addstr("The king transported you !  ");
			if (stat->chm)
				{
				addstr("But your charm save you. . .\n");
				--stat->chm;
				}
			else
				{
				stat->x += roll(-50,100) * circ(stat->x,stat->y);
				stat->y += roll(-50,100) * circ(stat->x,stat->y);
				addch('\n');
				}
			break;
		case GOLD:
			printw("The king has bestowed %.0f gold pieces on you !\n",bufp->scratch1);
			stat->gld += bufp->scratch1;
			break;
		case CURSED:
			addstr("You've been cursed !  ");
			if (stat->bls)
				{
				addstr("But your blessing saved you. . .\n");
				stat->bls = FALSE;
				}
			else
				{
				addch('\n');
				stat->psn += 2;
				stat->nrg = 10;
				stat->mxn  *= 0.95;
				stat->status = PLAYING;
				}
			break;
		case VAPORIZED:
			addstr("Woops!  You've been vaporized!\n");
			death(stat);
			break;
		case MONSTER:
			addstr("The Valar zapped you with a monster!\n");
			paws(7);
			fight(stat,(int) bufp->scratch1);
			return;
		case BLESS:
			addstr("The Valar has blessed you!\n");
			stat->nrg = (stat->mxn *= 1.05) + stat->shd;
			stat->man += 500;
			stat->str += 0.5;
			stat->brn += 0.5;
			stat->mag += 0.5;
			stat->psn = min(0.5,stat->psn);
			break;
		case MOVED:
			addstr("You've been relocated. . .\n");
			stat->x = bufp->scratch1;
			stat->y = bufp->scratch2;
			break;
		case HEAL:
			addstr("You've been healed!\n");
			stat->psn -=  0.25;
			stat->nrg = stat->mxn + stat->shd;
			break;
		case STOLEN:
			addstr("You'Ve been bumped off as Valar!\n");
			stat->typ = 20 + roll(1,6);
			break;
		case GRAIL:
			addstr("You have found The Holy Grail!!\n");
			if (stat->typ < 20)
				{
				addstr("However, you are not experienced enough to behold it.\n");
				stat->sin *= stat->sin;
				stat->man +=  1000;
				}
			else if (stat->typ == 99 || stat->typ == 90)
				{
				addstr("You have made it to the position of Valar once already.\n");
				addstr("The Grail is of no more use to you now.\n");
				}
			else
				{
				addstr("It is now time to see if you are worthy to behold it. . .\n");
				refresh();
				sleep(4);
				if (rnd() / 2.0 < stat->sin)
					{
					addstr("You blew this one!\n");
					stat->str = stat->man = stat->quk = stat->nrg = stat->mxn = stat->x = stat->y =
						stat->mag = stat->brn = stat->exp =1;
					stat->lvl = 0;
					}
				else
					{
					addstr("You made to position of Valar!\n");
					stat->typ = 99;
					fp = fopen(peoplefile,"r");
					loc = 0;
					while (fread((char *) &sbuf,sizeof(sbuf),1,fp))
						if (sbuf.typ == 99)
							{
							sbuf.tampered = STOLEN;
							update(&sbuf,loc);
							break;
							}
						else
							++loc;
					fclose(fp);
					}
				}
			vbuf.active = TRUE;
			vbuf.x = roll(-1e6,2e6);
			vbuf.y = roll(-1e6,2e6);
			voidupdate(&vbuf,0);
			break;
		}
}

void
checktampered(stat)			/* see if decree'd etc. */
        register struct	stats	*stat;
{
        struct	nrgvoid	vbuf;
        struct	stats sbuf;
        FILE	*fp;
        register int	loc = 0;

	/* first check for energy voids */
	fp = fopen(voidfile,"r");
	while (fread((char *) &vbuf,sizeof(vbuf),1,fp))
		if (vbuf.active && vbuf.x == stat->x && vbuf.y == stat->y)
			{
			fclose(fp);
			if (loc)
				{
				vbuf.active = FALSE;
				voidupdate(&vbuf,loc);
				tampered(stat,NRGVOID,&sbuf);
				}
			else if (stat->status != CLOAKED)
				tampered(stat,GRAIL,&sbuf);
			break;
			}
		else
			++loc;
	fclose(fp);
	/* now check for other things */
	statread(&sbuf,fileloc);
	if (sbuf.tampered)
		tampered(stat,sbuf.tampered,&sbuf);
}

void
decree(stat)				/* king and valar stuff */
        register struct	stats	*stat;
{
        FILE	*fp;
        short	arg;
        char	aline[80], *cp;
        struct	stats	sbuf;
        struct	nrgvoid	vbuf;
        double	temp1 = 0.0, temp2 = 0.0;
        int	ch;
        register int	loc = 0;

	move(3,0);
	clrtoeol();
	if (stat->typ < 20 && !su)	/* king */
		{
		addstr("1:Transport  2:Curse  3:Energy Void  4:Bestow  5:Collect Taxes  ");
		ch = rgetch();
		move(3,0);
		clrtoeol();
		switch (ch)
			{
			case '1':
				arg = TRANSPORT;
				cp = "transport";
				break;
			case '2':
				arg = CURSED;
				cp = "curse";
				break;
			case '3':
				addstr("Enter the X Y coordinates of void ? ");
				getstring(aline,30);
				sscanf(aline,"%lf %lf",&temp1,&temp2);
				vbuf.x = floor(temp1);
				vbuf.y = floor(temp2);
				vbuf.active = TRUE;
				voidupdate(&vbuf,allocvoid());
				goto EXIT;
			case '4':
				arg = GOLD;
				addstr("How much gold to bestow ? ");
				temp1 = inflt();
				if (temp1 > stat->gld || temp1 < 0)
					{
					mvaddstr(6,0,"You don't have that !\n");
					return;
					}
				stat->gld -= floor(temp1);
				cp = "give gold to";
				break;
			case '5':
				fp = fopen(goldfile,"r");
				if (fread((char *) &temp1,sizeof(double),1,fp) != 1)
				        /*ignore*/;
				fclose(fp);
				mvprintw(6,0,"You have collected %.0f in gold.\n",temp1);
				stat->gld += floor(temp1);
				fp = fopen(goldfile,"w");
				temp1 = 0.0;
				fwrite((char *) &temp1,sizeof(double),1,fp);
				fclose(fp);
				return;
			default:
				return;
			}
		}
	else	/* council of wise, valar, etc. */
		{
		addstr("1:Heal  ");
		if (stat->pal || su)
			addstr("2:Seek Grail  ");
		if (stat->typ == 99 || su)
			addstr("3:Throw Monster  4:Relocate  5:Bless  ");
		if (su)
			addstr("6:Vaporize  ");
		ch = rgetch();
		if (!su && ch > '2' && stat->typ != 99)
			{
			illcmd();
			return;
			}
		switch (ch)
			{
			case '1':
				arg = HEAL;
				cp = "heal";
				break;
			case '2':
				if (stat->pal)
					{
					fp = fopen(voidfile,"r");
					if (fread((char *) &vbuf,sizeof(vbuf),1,fp) != 1)
					        /*ignore*/;
					fclose(fp);
					temp1 = hypot(stat->x - vbuf.x,stat->y - vbuf.y);
					temp1 = floor(temp1 + roll(-temp1/10.0,temp1/5.0));
					mvprintw(6,0,"The palantir says the Grail is about %.0f away.\n",temp1);
					return;
					}
				else
					{
					mvaddstr(6,0,"You need a palantir to seek the Grail.\n");
					return;
					}
			case '3':
				mvaddstr(3,0,"Which monster [0-99] ? ");
				temp1 = inflt();
				temp1 = max(0,min(99,temp1));
				cp = "throw a monster at";
				arg = MONSTER;
				break;
			case '4':
				mvaddstr(3,0,"New X Y coordinates ? ");
				getstring(aline,30);
				sscanf(aline,"%lf %lf",&temp1,&temp2);
				cp = "relocate";
				arg = MOVED;
				break;
			case '5':
				arg = BLESS;
				cp = "bless";
				break;
			case '6':
				if (su)
					{
					cp = "vaporize";
					arg = VAPORIZED;
					break;
					}
			default:
				return;
			}
		}
	mvprintw(3,0,"Who do you want to %s ? ",cp);
	getstring(aline,21);
	strunc(aline);
	if (strcmp(stat->name,aline))
		{
		fp = fopen(peoplefile,"r");
		while (fread((char *) &sbuf,sizeof(sbuf),1,fp))
			if (strcmp(aline,sbuf.name))
				++loc;
			else
				{
				fclose(fp);
				if (sbuf.tampered)
					{
					mvaddstr(6,0,"That person has something pending already.\n");
					return;
					}
				else
					{
					sbuf.tampered = arg;
					sbuf.scratch1 = floor(temp1);
					sbuf.scratch2 = floor(temp2);
					update(&sbuf,loc);
EXIT:					mvaddstr(6,0,"It is done.\n");
					return;
					}
				}
		fclose(fp);
		mvaddstr(6,0,"There is no one by that name.\n");
		}
	else
		mvaddstr(6,0,"You may not do it to yourself!\n");
}

int
allocvoid()				/* find a space to put an energy void */
{
        FILE	*fp;
        register int	loc = 0;
        struct	nrgvoid	vbuf;

	fp = fopen(voidfile,"r");
	while (fread((char *) &vbuf,sizeof(vbuf),1,fp))
		if (vbuf.active)
			++loc;
		else
			{
			fclose(fp);
			return (loc);
			}
	fclose(fp);
	return (loc);
}

void
adjuststats(stat)				/* make sure things are within limits, etc. */
        register struct	stats	*stat;
{
        long	ltemp;
        register int	temp;

	stat->x = floor(stat->x);
	stat->y = floor(stat->y);
	valhala = (stat->typ == 99);
	throne = (stat->x == 0.0 && stat->y == 0.0);
	temp = abs(stat->x)/400;
	if (temp > 16)
		temp = 0;
	if (stat->y == 0.0 && !throne && !valhala && temp == abs(stat->x)/400 && sgn(stat->x) == (int) pow(-1.0, (double) temp))
		{
		if (!wmhl)
			stat->wormhole = temp;
		wmhl = TRUE;
		}
	else
		wmhl = FALSE;
	speed = stat->quk + stat->quks - spdcalc(stat->lvl,stat->gld,stat->gem);
	strength = stat->str + stat->swd - strcalc(stat->str,stat->psn);
	time(&ltemp);
	stat->age += ltemp - secs;
	secs = ltemp;
	stat->quks = min(99,stat->quks);
	stat->man = min(stat->man,stat->lvl*15 + 5000);
	stat->chm = min(stat->chm,stat->lvl + 10);
	stat->typ = (stat->crn && stat->typ < 10) ? -abs(stat->typ) : abs(stat->typ);
	if (level(stat->exp) > stat->lvl)
		movelvl(stat);
	stat->gld = floor(stat->gld) + 0.1;
	stat->gem = floor(stat->gem) + 0.1;
	if (stat->rng.type)
		stat->nrg = stat->mxn + stat->shd;
	if (stat->rng.type && stat->rng.duration <= 0)	/* clean up rings */
		switch (stat->rng.type)
			{
			case DLBAD:
			case NAZBAD:
				stat->rng.type = SPOILED;
				stat->rng.duration = roll(10,25);
				break;
			case NAZREG:
				stat->rng.type = NONE;
				break;
			case SPOILED:
				death(stat);
				break;
			}	/* DLREG is ok, so do nothing with it */
	stat->nrg += (stat->mxn+stat->shd)/15+stat->lvl/3+2;
	stat->nrg = min(stat->nrg,stat->mxn + stat->shd);
	if (stat->age > stat->degen * 2500)
		{
		++stat->degen;
		if (stat->quk > 23)
			--stat->quk;
		stat->str *= 0.97;
		stat->brn *= 0.95;
		stat->mag *= 0.97;
		stat->mxn *= 0.95;
		if (stat->quks)
			--stat->quks;
		stat->swd *= 0.93;
		stat->shd *= 0.95;
		}
}

static void
interm(stat,who)				/* interterminal battle routine */
        register struct	stats	*stat;
        int	who;
{
#define	MAXWAIT	20
#define	BLOCK	sizeof(struct stats)
#define	RAN	1
#define	STUCK	2
#define	BLEWIT	3
#define	KILLED	4
#define readfoe()	fseek(fin,foeplace,0);  \
			if (fread((char *) foe,BLOCK,1,fin) != 1) /*ignore*/
#define updateme()	fseek(fout,myplace,0);  \
			fwrite((char *) stat,BLOCK,1,fout);  \
			fflush(fout)

        FILE	*fin, *fout;	/* pointers for input, output */
        double	temp, foespeed, oldhits = 0.0, myhits;
        struct	stats	sbuf;
        register struct	stats *foe;
        register int	loop, lines = 5;
        int	ch;
        long	myplace, foeplace;
        int	oldtags;
        bool	luckout = FALSE;
        char	foename[21];

	fghting = TRUE;
	mvaddstr(4,0,"Preparing for battle!\n");
	refresh();
		/* set up variables, file, etc. */
	myplace = fileloc * BLOCK;
	foeplace = who * BLOCK;
	fin = fopen(peoplefile,"r");
	setbuf(fin, (char *) NULL);
	fout = fopen(peoplefile,ACCESS);
	stat->status = INBATTLE;
	myhits = stat->nrg;
	stat->tampered = oldtags = 1;	/* this must be non-zero to prevent a king or valar from trashing it */
	stat->scratch1 = stat->scratch2 = 0.0;
	updateme();
	foe = &sbuf;
	readfoe();
	foespeed = foe->quk + foe->quks - spdcalc(foe->lvl,foe->gld,foe->gem);
	if (abs(stat->lvl - foe->lvl) > 20)		/* see if greatly mismatched */
		{
		temp = ((double) (stat->lvl - foe->lvl))/((double) max(stat->lvl,foe->lvl));
		if (temp > 0.5)		/* this one outweighs his/her foe */
			foespeed *= 2.0;
		else if (temp < -0.5)	/* foe outweighs this one */
			speed *= 2.0;
		}
	if (stat->blind)
		strcpy(foename,"someone");
	else
		strcpy(foename,foe->name);
	mvprintw(3,0,"You have encountered %s   Level: %d\n",foename,foe->lvl);
	refresh();
		/* now wait for foe to respond */
	for (loop = 1.5*MAXWAIT; foe->status != INBATTLE && loop; --loop)
		{
		readfoe();
		sleep(1);
		}
	if (foe->status != INBATTLE)
		{
		mvprintw(4,0,"%s is not responding.\n",foename);
		goto LEAVE;
		}

	/* otherwise, everything is set to go */
	move(4,0);
	clrtoeol();
		/* check to see who goes first */
	if (speed > foespeed)
		goto HITFOE;
	else if (foespeed > speed)
		goto WAIT;
	else if (stat->lvl > foe->lvl)
		goto HITFOE;
	else if (foe->lvl > stat->lvl)
		goto WAIT;
	else	/* no one is faster */
		{
		printw("You can't fight %s yet.",foename);
		goto LEAVE;
		}

/* routine to hit, etc */
HITFOE:	printstats(stat);
	mvprintw(1,26,"%20.0f",myhits);
	mvaddstr(4,0,"1:Fight  2:Run Away!  3:Power Blast  ");
	if (luckout)
		clrtoeol();
	else
		addstr("4:Luckout  ");
	ch = gch(stat->rng.type);
	move(lines = 5,0);
	clrtobot();
	switch (ch)
		{
		default:	/* fight */
			temp = roll(2,strength);
HIT:			mvprintw(lines++,0,"You hit %s %.0f times!",foename,temp);
			stat->sin += 0.5;
			stat->scratch1 += temp;
			stat->scratch2 = FALSE;
			break;
		case '2':	/* run away */
			--stat->scratch1;	/* this value changes to indicate action */
			if (rnd() > 0.25)
				{
				mvaddstr(lines++,0,"You got away!");
				stat->scratch2 = RAN;
				goto LEAVE;
				}
			mvprintw(lines++,0,"%s is still after you!",foename);
			stat->scratch2 = STUCK;
			break;
		case '3':	/* power blast */
			temp = min(stat->man,stat->lvl*5);
			stat->man -= temp;
			temp = (rnd() + 0.5) * temp * stat->mag * 0.2 + 2;
			mvprintw(lines++,0,"You blasted %s !",foename);
			goto HIT;
		case '4':	/* luckout */
			if (luckout || rnd() > 0.1)
				{
				luckout = TRUE;
				mvaddstr(lines++,0,"Not this time...");
				--stat->scratch1;
				stat->scratch2 = BLEWIT;
				}
			else
				{
				mvaddstr(lines++,0,"You just lucked out!");
				stat->scratch1 = foe->nrg + 5;
				}
			break;
		}
	refresh();
	stat->scratch1 = floor(stat->scratch1);		/* clean up any mess */
	if (stat->scratch1 > foe->nrg)
		stat->scratch2 = KILLED;
	else if (rnd() * speed < rnd() * foespeed)
		{		/* foe's turn */
		++stat->tampered;
		updateme();
		goto WAIT;
		}
	updateme();

	if (((int) stat->scratch2) == KILLED)
		{
		mvprintw(lines++,0,"You killed %s!",foename);
		stat->exp += foe->exp;
		stat->crn += (stat->lvl < 1000) ? foe->crn : 0;
		stat->amu += foe->amu;
		stat->chm += foe->chm;
		stat->gld += foe->gld;
		stat->gem += foe->gem;
		stat->swd = max(stat->swd,foe->swd);
		stat->shd = max(stat->shd,foe->shd);
		stat->quks = max(stat->quks,foe->quks);
		sleep(3);	  /* give other person time to die */
		goto LEAVE;
		}
	goto HITFOE;	/* otherwise, my turn again */

/* routine to wait for foe to do something */
WAIT:	printstats(stat);
	mvprintw(1,26,"%20.0f",myhits);
	mvaddstr(4,0,"Waiting...\n");
	refresh();
	for (loop = MAXWAIT; loop; --loop)
		{
		readfoe();
		if (foe->scratch1 != oldhits)
			switch ((int) foe->scratch2)
				{
				case RAN:
					mvprintw(lines++,0,"%s ran away!",foename);
					goto LEAVE;
				case STUCK:
					mvprintw(lines++,0,"%s tried to run away.",foename);
					goto BOT;
				case BLEWIT:
					mvprintw(lines++,0,"%s tried to luckout!",foename);
					goto BOT;
				default:
					temp = foe->scratch1 - oldhits;
					mvprintw(lines++,0,"%s hit you %.0f times!",foename,temp);
					myhits -= temp;
					goto BOT;
				}
		sleep(1);
		}
	/* timeout */
	mvaddstr(23,0,"Timeout: waiting for response.  Do you want to wait ? ");
	refresh();
	ch = getch();
	move(23,0);
	clrtoeol();
	if (toupper(ch) == 'Y')
		goto WAIT;
	goto LEAVE;

/* routine to decide what happens next */
BOT:	refresh();
	if (lines > 21)
		{
		paws(lines);
		move(lines = 5,0);
		clrtobot();
		}
	if (((int) foe->scratch2) == KILLED || myhits < 0.0)
		{
		myhits = -2;
		goto LEAVE;		/* main will pick up death */
		}
	oldhits = foe->scratch1;
	if (foe->tampered != oldtags)
		{
		oldtags = foe->tampered;
		goto HITFOE;
		}
	goto WAIT;

/* routine to clean up things and leave */
LEAVE:	updateme();
	fclose(fin);
	fclose(fout);
	stat->x += roll(5,-10);
	stat->y += roll(5,-10);
	stat->nrg = myhits;
	stat->tampered = OFF;
	stat->status = PLAYING;
	changed = TRUE;
	paws(lines);
	move(3,0);
	clrtobot();
}

void
checkinterm(stat)				/* see if other person on same x,y */
        register struct	stats	*stat;
{
        FILE	*fp;
        struct	stats	sbuf;
        register int	foeloc = 0;

	users = 0;
	fp = fopen(peoplefile,"r");
	while (fread((char *) &sbuf,sizeof(sbuf),1,fp))
		{
		if (sbuf.status && (sbuf.status != CLOAKED || sbuf.typ != 99))
			{
			++users;
			if (stat->x == sbuf.x && stat->y == sbuf.y
			   && foeloc != fileloc && sbuf.typ != 99
			   && stat->typ !=99 && !stat->wormhole && !sbuf.wormhole)
				{
				fclose(fp);
				interm(stat,foeloc);
				return;
				}
			}
		++foeloc;
		}
	fclose(fp);
}

int
gch(rngtyp)			/* get a character from terminal, but check ring if crazy */
        int	rngtyp;
{
	refresh();
	if (abs(rngtyp) != SPOILED)
		return (getch());
	else
		{
		getch();
		return (roll(0,5) + '0');
		}
}

int
rngcalc(chartyp)				/* pick a duration of a ring */
        int	chartyp;
{
        static	int	rngtab[] = { 0, 10, 20, 13, 25, 40, 20};

	if (chartyp > 10)
		chartyp -= 10;
	return (rngtab[chartyp - 1]);
}

void
interrupt(int sig)				/* call when break key is hit */
{
        char	line[81];
        register int	loop;
        int	x, y, ch;

#ifdef USG3
	signal(SIGINT,SIG_IGN);
#endif
#ifdef USG5
	signal(SIGINT,SIG_IGN);
#endif
	getyx(stdscr,y,x);
	for (loop = 79; loop >= 0; --loop)	/* snarf line */
		{
		move(4,loop);
		line[loop] = inch();
		}
	line[80] = '\0';
	clrtoeol();
	if (fghting)
		{
		move(4,0);
		clrtoeol();
		addstr("Quitting now will automatically kill your character.  Still want to ? ");
		ch = rgetch();
		if (toupper(ch) == 'Y')
			longjmp(mainenv,DIE);
		}
	else
		{
		move(4,0);
		clrtoeol();
		addstr("Do you really want to quit ? ");
		ch = rgetch();
		if (toupper(ch) == 'Y')
			longjmp(mainenv,QUIT);
		}
	mvaddstr(4,0,line);	/* return screen to previous state */
	move(y,x);
	refresh();
#ifdef USG3
	signal(SIGINT,interrupt);
#endif
#ifdef USG5
	signal(SIGINT,interrupt);
#endif
}

int
rgetch()		/* refresh, then get a char. */
{
	refresh();
	return (getch());
}

void
purge() 	/* remove old players */
{
        FILE	*fin, *fout;
        struct	stats	sbuf;
        register int	loc, today, temp;
        long	ltime;

	loc = 0;
	time(&ltime);
	today = localtime(&ltime)->tm_yday;
	fin = fopen(peoplefile,"r");
	fout = fopen(peoplefile,ACCESS);
	while(fread((char *) &sbuf,sizeof(sbuf),1,fin))
		{
		temp = today - sbuf.lastused;
		if (temp < 0)
			temp += 365;
		if (temp > 9)		/* ten days old --> delete */
			{
			initchar(&sbuf);
			strcpy(sbuf.name,"<null>");
			fseek(fout,(long) loc * sizeof(sbuf),0);
			fwrite((char *) &sbuf,sizeof(sbuf),1,fout);
			}
		++loc;
		}
	fclose(fin);
	fclose(fout);
}
