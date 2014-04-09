/*
 * func1.c	Phantasia support routines
 */

#include "phant.h"

bool	findname(name)				/* return TRUE if name in use */
register char	*name;
{
FILE	*fp;
struct	stats	buf;

	fp = fopen(peoplefile,"r");
	while (fread((char *) &buf,sizeof(buf),1,fp))
		if (!strcmp(buf.name,name))
			{
			fclose(fp);
			mvaddstr(21,0,"Name already in use.\n");
			refresh();
			return (TRUE);
			}
	fclose(fp);
	return (FALSE);
}

int	findspace()				/* allocate space for a character in peoplefile */
{
FILE	*fp;
struct	stats	buf;
register int	loc;

	loc = 0;
	fp = fopen(peoplefile,"r");
	while (fread((char *) &buf,sizeof(buf),1,fp))
		{
		if (!strcmp(buf.name,"<null>"))
			{
			fclose(fp);
			return (loc);
			}
		else
			++loc;
		}
	fclose(fp);
	fp = fopen(peoplefile,ACCESS);
	fseek(fp,(long) loc * sizeof(buf),0);
	initchar(&buf);
	strcpy(buf.name,"inuse");
	fwrite((char *) &buf,sizeof(buf),1,fp);
	fclose(fp);
	return (loc);
}

int	findchar(stat)				/* retrieve a character from file */
register struct	stats	*stat;
{
register int	loc = 0, loop;
char	name[21];
FILE	*fp;

	if (fp = fopen(peoplefile,"r"))
		{
		clear();
		mvprintw(10,0,"What was your character's name ? ");
		getstring(name,21);
		strunc(name);
		while (fread((char *) stat,sizeof(*stat),1,fp))
			{
			if (!strcmp(stat->name,name))
				{
				move(11,0);
				refresh();
				fclose(fp);
				nocrmode();
				for (loop = 0; loop < 2; ++loop)
					if (!strcmp(getpass("Password ? "),stat->pswd))
						{
						crmode();
						return (loc);
						}
					else
						printf("No good.\n");
				exit1();
				/*NOTREACHED*/
				}
			++loc;
			}
		}
	fclose(fp);
	addstr("\n\nNot found.\n");
	exit1();
/*NOTREACHED*/
}

void	leave(stat)				/* save character in file */
register struct	stats	*stat;
{
long	ltemp;

	if (!stat->lvl)
		strcpy(stat->name,"<null>");
	stat->status = OFF;
	time(&ltemp);
	stat->age += ltemp - secs;
	update(stat,fileloc);
	exit1();
	/*NOTREACHED*/
}

void	talk(name)				/* send message to all players */
register char	*name;
{
FILE	*fp;
char	aline[160];

	mvaddstr(5,0,"Message ? ");
	getstring(aline,160);
	fp = fopen(messfile,"w");
	if (*aline)
		fprintf(fp,"%s:  %s",name,aline);
	fclose(fp);
}

void	death(stat)				/* remove a player after dying */
register struct	stats	*stat;
{
FILE	*fp;
char	aline[100];
int	ch;
register int	loop;
long	ltemp;

	clear();
	if (stat->typ == 99)
		if (stat->rng.duration)
			{
			addstr("Valar should be more cautious.  You've been killed.\n");
			printw("You only have %d more chance(s).\n",--stat->rng.duration);
			paws(3);
			stat->nrg = stat->mxn;
			return;
			}
		else
			{
			addstr("You had your chances, but Valar aren't totally\n");
			addstr("immortal.  You are now left to wither and die . . .\n");
			paws(3);
			stat->brn = stat->lvl /25;
			stat->nrg = stat->mxn;
			stat->quks = stat->swd = 0;
			stat->typ = 90;
			return;
			}
	if (stat->lvl > 9999)
		addstr("Characters greater than level 10K must be retired.  Sorry.");
	switch(stat->rng.type)
		{
		case -DLREG:
		case -NAZREG:
			mvaddstr(4,0,"Your ring saved you from death!\n");
			refresh();
			stat->rng.type = NONE;
			stat->nrg = stat->mxn/12+1;
			stat->crn -= (stat->crn > 0);
			return;
		case DLBAD:
		case -DLBAD:
		case NAZBAD:
		case -NAZBAD:
		case -SPOILED:
		case SPOILED:
			mvaddstr(4,0,"Your ring has taken control of you and turned you into a monster!\n");
			fp = fopen(monsterfile,"r");
			for (loop = 0; loop <= 13; ++loop) {
				if (! fgets(aline,100,fp))
				        /*ignore*/;
                        }
			ltemp = ftell(fp);
			fclose(fp);
			fp = fopen(monsterfile,ACCESS);
			fseek(fp,ltemp,0);
			fprintf(fp,"%-20s",stat->name);
			fclose(fp);
		}
	initchar(stat);
	fp = fopen(lastdead,"w");
	fprintf(fp,"%s   Login:  %s",stat->name,stat->login);
	fclose(fp);
	strcpy(stat->name,"<null>");
	update(stat,fileloc);
	clear();
	move(10,0);
	switch ((int) roll(1,5))
		{
		case 1:
			addstr("You've crapped out!  ");
			break;
		case 2:
			addstr("You have been disemboweled.  ");
			break;
		case 3:
			addstr("You've been mashed, mauled, and spit upon.  (You're dead.)\n");
			break;
		case 4:
			addstr("You died!  ");
			break;
		case 5:
			addstr("You're a complete failure -- you've died!!\n");
		}
	addstr("Care to give it another try ? ");
	ch = rgetch();
	if (toupper(ch) == 'Y') {
		endwin();
		execl(gameprog, "phantasia", "-s", (char*)0);
	}
	exit1();
	/*NOTREACHED*/
}

void
update(stat,place)			/* update charac file */
        register struct	stats	*stat;
        register int	place;
{
        FILE	*fp;

	fp = fopen(peoplefile,ACCESS);
	fseek(fp,(long) place*sizeof(*stat),0);
	fwrite((char *) stat,sizeof(*stat),1,fp);
	fclose(fp);
}

void	printplayers(stat)			/* show users */
register struct	stats	*stat;
{
FILE	*fp;
struct	stats	buf;
register int	loop = 0;
double	loc;
long	ltmp;
int	ch;

	if (stat->blind)
		{
		mvaddstr(6,0,"You can't see anyone.\n");
		return;
		}
	loc = circ(stat->x,stat->y);
	mvaddstr(6,0,"Name                         X         Y       Lvl  Type  Login\n");
	fp = fopen(peoplefile,"r");
	while (fread((char *) &buf,sizeof(buf),1,fp))
		{
		if (buf.status)
			{
			ch = (buf.status == CLOAKED) ? '?' : 'W';
			if (stat->typ > 10 || buf.typ > 10 || loc >= circ(buf.x,buf.y) || stat->pal)
				if (buf.status != CLOAKED || (stat->typ == 99 && stat->pal))
					if (buf.typ == 99)
						addstr("The Valar is watching you. . .\n");
					else if (buf.wormhole)
						printw("%-20s         %c         %c    %6u  %3d   %-9s\n",
							buf.name,ch,ch,buf.lvl,buf.typ,buf.login);
					else
						printw("%-20s  %8.0f  %8.0f    %6u  %3d   %-9s\n",
							buf.name,buf.x,buf.y,buf.lvl,buf.typ,buf.login);
				else
					if (buf.typ == 99)
						--loop;
					else
						printw("%-20s         ?         ?    %6u  %3d   %-9s\n",
							buf.name,buf.lvl,buf.typ,buf.login);
			++loop;
			}
		}
	fclose(fp);
	time(&ltmp);
	printw("Total users = %d    %s\n",loop,ctime(&ltmp));
	refresh();
}


void	printhelp()				/* print help file */
{
FILE	*fp;
char	instr[100];

	fp = fopen(helpfile,"r");
	while (fgets(instr,100,fp))
		fputs(instr,stdout);
	fclose(fp);
}

void	titlestuff()				/* print out a header */
{
FILE	*fp;
char	instr[80], hiname[21], nxtname[21], aline[80];
bool	cowfound = FALSE, kingfound = FALSE;
struct	stats	buf;
double	hiexp, nxtexp;
unsigned	hilvl, nxtlvl;
register int	loop;

	mvaddstr(0,15,"W e l c o m e   t o   P h a n t a s i a (vers. 3.2)!");
	fp = fopen(motd,"r");
	if (fgets(instr,80,fp))
		mvaddstr(2,40 - strlen(instr)/2,instr);
	fclose(fp);
	fp = fopen(peoplefile,"r");
	while (fread((char *) &buf,sizeof(buf),1,fp))
		if (buf.typ > 10 && buf.typ < 20)
			{
			sprintf(instr,"The present ruler is %s  Level:%d",buf.name,buf.lvl);
			mvaddstr(4,40 - strlen(instr)/2,instr);
			kingfound = TRUE;
			break;
			}
	if (!kingfound)
		mvaddstr(4,24,"There is no ruler at this time.");
	fseek(fp,0L,0);
	while (fread((char *) &buf,sizeof(buf),1,fp))
		if (buf.typ == 99)
			{
			sprintf(instr,"The Valar is %s   Login:  %s",buf.name,buf.login);
			mvaddstr(6,40 - strlen(instr)/2,instr);
			break;
			}
	fseek(fp,0L,0);
	while (fread((char *) &buf,sizeof(buf),1,fp))
		if (buf.typ > 20 && buf.typ < 90)
			{
			if (!cowfound)
				{
				mvaddstr(8,30,"Council of the Wise:");
				loop = 10;
				cowfound = TRUE;
				}
			/* This assumes a finite (<=7) number of C.O.W.: */
			sprintf(instr,"%s   Login:  %s",buf.name,buf.login);
			mvaddstr(loop++,40 - strlen(instr)/2,instr);
			}
	fseek(fp,0L,0);
	*nxtname = *hiname = '\0';
	hiexp = 0.0;
	nxtlvl = hilvl = 0;
	while (fread((char *) &buf,sizeof(buf),1,fp))
		if (buf.exp > hiexp && buf.typ < 20)
			{
			nxtexp = hiexp;
			hiexp = buf.exp;
			nxtlvl = hilvl;
			hilvl = buf.lvl;
			strcpy(nxtname,hiname);
			strcpy(hiname,buf.name);
			}
		else if (buf.exp > nxtexp && buf.typ < 20)
			{
			nxtexp = buf.exp;
			nxtlvl = buf.lvl;
			strcpy(nxtname,buf.name);
			}
	fclose(fp);
	mvaddstr(17,28,"Highest characters are:");
	sprintf(instr,"%s  Level:%d   and   %s  Level:%d",hiname,hilvl,nxtname,nxtlvl);
	mvaddstr(19,40 - strlen(instr)/2,instr);
	fp = fopen(lastdead,"r");
	if (! fgets(aline,80,fp))
	        /*ignore*/;
	sprintf(instr,"The last character to die is %s",aline);
	mvaddstr(21,40 - strlen(instr)/2,instr);
	fclose(fp);
	refresh();
}



void	printmonster()				/* do a monster list on the terminal */
{
FILE	*fp;
register int	count = 0;
char	instr[100];

	puts(" #  Name                    Str     Brains  Quick   Hits    Exp     Treas   Type    Flock%\n");
	fp = fopen(monsterfile,"r");
	while (fgets(instr,100,fp))
		printf("%2d  %s",count++,instr);
	fclose(fp);
}

void
exit1() 				/* exit, but cleanup */
{
	move(23,0);
	refresh();
	nocrmode();
	endwin();
	exit(0);
	/*NOTREACHED*/
}

void	init1() 				/* set up for screen updating */
{
		/* catch/ingnore signals */
	signal(SIGQUIT,SIG_IGN);
	signal(SIGALRM,SIG_IGN);
	signal(SIGTERM,SIG_IGN);
	signal(SIGTSTP,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
	signal(SIGTTOU,SIG_IGN);
	signal(SIGINT,SIG_IGN);

	srand((unsigned) time((long *) NULL));	/* prime random numbers */
	initscr();
	noecho();
	crmode();
	clear();
	refresh();
}

void
getstring(cp,mx)				/* get a string from the stdscr at current y,x */
        register char	*cp;
        register int	mx;
{
        register int	loop = 0, x, y, xorig;
        int	ch;

	getyx(stdscr,y,xorig);
	clrtoeol();
	refresh();
	while((ch = getch()) != '\n' && loop < mx - 1)
		switch (ch)
			{
			case '\033':	/* escape */
			case '\010':	/* backspace */
				if (loop)
					{
					--loop;
					getyx(stdscr,y,x);
					mvaddch(y,x-1,' ');
					move(y,x-1);
					refresh();
					}
				break;
			case '\030':	/* ctrl-x */
				loop = 0;
				move(y,xorig);
				clrtoeol();
				refresh();
				break;
			default:
				if (ch >= ' ') /* printing char */
					{
					addch(ch);
					cp[loop++] = ch;
					refresh();
					}
			}
	cp[loop] = '\0';
}


void	showusers()				/* print a list of all characters */
{
struct	stats	buf;
FILE	*fp;

	if (fp = fopen(peoplefile,"r"))
		{
		puts("Current characters on file are:\n");
		while (fread((char *) &buf,sizeof(buf),1,fp))
			if (strcmp("<null>",buf.name))
				printf("%-20s   Login: %-9s  Level: %6u\n",buf.name,buf.login,buf.lvl);
		fclose(fp);
		}
}

void	kingstuff(stat) 			/* stuff upon entering throne */
register struct 	stats	*stat;
{
FILE	*fp;
struct	stats	buf;
struct	nrgvoid	vbuf;
register int	loc = 0;

	if (stat->typ < 10)	/* check to see if king -- assumes crown */
		{
		fp = fopen(peoplefile,"r");
		while (fread((char *) &buf,sizeof(buf),1,fp))
			if (buf.typ > 10 && buf.typ < 20)	/* found old king */
				if (buf.status != OFF)
					{
					mvaddstr(6,0,"The king is playing, so you cannot steal his throne\n");
					stat->x = stat->y = 9;
					move(3,0);
					fclose(fp);
					return;
					}
				else
					{
					buf.typ -= 10;
					if (buf.crn)
						--buf.crn;
					fclose(fp);
					update(&buf,loc);
KING:				stat->typ = abs(stat->typ) + 10;
					mvaddstr(6,0,"You have become king!\n");
					fp = fopen(messfile,"w");
					fprintf(fp,"All hail the new king!");
					fclose(fp);
					/* clear all energy voids */
					fp = fopen(voidfile,"r");
					if (fread((char *) &vbuf,sizeof(vbuf),1,fp) != 1)
					        /*ignore*/;
					fclose(fp);
					fp = fopen(voidfile,"w");
					fwrite((char *) &vbuf,sizeof(vbuf),1,fp);
					fclose(fp);
					goto EXIT;
					}
			else
				++loc;
		fclose(fp);	  /* old king not found -- install new one */
		goto KING;
		}
EXIT:	mvaddstr(3,0,"0:Decree  ");
}

void
paws(where)				/* wait for input to continue */
        int	where;
{
	mvaddstr(where,0,"-- more --");
	rgetch();
}

void	cstat() 				/* examine/change stats of a character */
{
struct	stats charac;
char	s[60], flag[2];
FILE	*fp;
register int	loc = 0;
int	c, temp, today;
long	ltemp;
double	dtemp;

	flag[0] = 'F';	flag[1] = 'T';
	mvaddstr(10,0,"Which character do you want to look at ? ");
	getstring(s,60);
	if (fp = fopen(peoplefile,"r"))
		while (fread((char *) &charac,sizeof(charac),1,fp))
			if (!strcmp(s,charac.name))
				goto FOUND;
			else
				++loc;
	mvaddstr(11,0,"Not found.");
	exit1();
	/*NOTREACHED*/

FOUND:	fclose(fp);
	time(&ltemp);
	today = localtime(&ltemp)->tm_yday;
	if (!su)
		strcpy(charac.pswd,"XXXXXXXX");
	clear();
TOP:	mvprintw(0,0,"a:Name         %s\n",charac.name);
	printw("b:Password     %s\n",charac.pswd);
	printw(" :Login        %s\n",charac.login);
	temp = today - charac.lastused;
	if (temp < 0)
		temp += 365;
	printw("c:Used         %d\n",temp);
	mvprintw(5,0,"d:Experience   %.0f\n",charac.exp);
	printw("e:Level        %d\n",charac.lvl);
	printw("f:Strength     %.0f\n",charac.str);
	printw("g:Sword        %.0f\n",charac.swd);
	printw("h:Quickness    %d\n",charac.quk);
	printw("i:Quikslvr     %d\n",charac.quks);
	printw("j:Energy       %.0f\n",charac.nrg);
	printw("k:Max-Nrg      %.0f\n",charac.mxn);
	printw("l:Shield       %.0f\n",charac.shd);
	printw("m:Magic        %.0f\n",charac.mag);
	printw("n:Manna        %.0f\n",charac.man);
	printw("o:Brains       %.0f\n",charac.brn);
	mvprintw(0,40,"p:X-coord      %.0f\n",charac.x);
	mvprintw(1,40,"q:Y-coord      %.0f\n",charac.y);
	if (su)
		mvprintw(2,40,"r:Wormhole     %d\n",charac.wormhole);
	else
		mvprintw(2,40,"r:Wormhole     %c\n",flag[charac.wormhole != 0]);
	mvprintw(3,40,"s:Type         %d\n",charac.typ);
	mvprintw(5,40,"t:Sin          %0.3f\n",charac.sin);
	mvprintw(6,40,"u:Poison       %0.3f\n",charac.psn);
	mvprintw(7,40,"v:Gold         %.0f\n",charac.gld);
	mvprintw(8,40,"w:Gem          %.0f\n",charac.gem);
	mvprintw(9,40,"x:Holy Water   %d\n",charac.hw);
	mvprintw(10,40,"y:Charms       %d\n",charac.chm);
	mvprintw(11,40,"z:Crowns       %d\n",charac.crn);
	mvprintw(12,40,"1:Amulets      %d\n",charac.amu);
	mvprintw(13,40,"2:Age          %d\n",charac.age);
	mvprintw(18,5,"3:Virgin %c  4:Blessed %c  5:Ring %c  6:Blind %c  7:Palantir %c",
		flag[charac.vrg],flag[charac.bls],flag[charac.rng.type != 0],flag[charac.blind],flag[charac.pal]);
	if (!su)
		exit1();
	mvaddstr(15,40,"!:Quit");
	mvaddstr(16,40,"?:Delete");
	mvaddstr(19,30,"8:Duration");
	mvaddstr(21,0,"What would you like to change? ");
	c = rgetch();
	switch(c)
		{
		case 'p':	/* change x coord */
			mvprintw(23,0,"x = %f; x = ",charac.x);
			dtemp = inflt();
			if (dtemp != 0.0)
				charac.x = dtemp;
			break;
		case 'q':	/* change y coord */
			mvprintw(23,0,"y = %f; y = ",charac.y);
			dtemp = inflt();
			if (dtemp != 0.0)
				charac.y = dtemp;
			break;
		case 'd':	/* change Experience */
			mvprintw(23,0,"exp = %f; exp = ",charac.exp);
			dtemp = inflt();
			if (dtemp != 0.0)
				charac.exp = dtemp;
			break;
		case 'e':	/* change level */
			mvprintw(23,0,"lvl = %d; lvl;= ",charac.lvl);
			dtemp = inflt();
			if (dtemp != 0.0)
				charac.lvl = dtemp;
			break;
		case 'h':	/* change quickness */
			mvprintw(23,0,"quk = %d; quk;= ",charac.quk);
			dtemp = inflt();
			if (dtemp != 0.0)
				charac.quk = dtemp;
			break;
		case 'f':	/* change strength */
			mvprintw(23,0,"str = %f; str;= ",charac.str);
			dtemp = inflt();
			if (dtemp != 0.0)
				charac.str = dtemp;
			break;
		case 't':	/* change Sin */
			mvprintw(23,0,"sin = %f; sin;= ",charac.sin);
			dtemp = inflt();
			if (dtemp != 0.0)
				charac.sin = dtemp;
			break;
		case 'n':	/* change manna */
			mvprintw(23,0,"man = %f; man;= ",charac.man);
			dtemp = inflt();
			if (dtemp != 0.0)
				charac.man = dtemp;
			break;
		case 'v':	/* change gold */
			mvprintw(23,0,"gld = %f; gld;= ",charac.gld);
			dtemp = inflt();
			if (dtemp != 0.0)
				charac.gld = dtemp;
			break;
		case 'j':	/* change energy */
			mvprintw(23,0,"nrg = %f; nrg;= ",charac.nrg);
			dtemp = inflt();
			if (dtemp != 0.0)
				charac.nrg = dtemp;
			break;
		case 'k':	/* change Maximum energy */
			mvprintw(23,0,"mxn = %f; mxn;= ",charac.mxn);
			dtemp = inflt();
			if (dtemp != 0.0)
				charac.mxn = dtemp;
			break;
		case 'm':	/* change magic */
			mvprintw(23,0,"mag = %f; mag;= ",charac.mag);
			dtemp = inflt();
			if (dtemp != 0.0)
				charac.mag = dtemp;
			break;
		case 'o':	/* change brains */
			mvprintw(23,0,"brn = %f; brn;= ",charac.brn);
			dtemp = inflt();
			if (dtemp != 0.0)
				charac.brn = dtemp;
			break;
		case 'z':	/* change crowns */
			mvprintw(23,0,"crn = %d; crn;= ",charac.crn);
			dtemp = inflt();
			if (dtemp != 0.0)
				charac.crn = dtemp;
			break;
		case '5':	/* change ring type */
			mvprintw(23,0,"rng-type = %d; rng-type;= ",charac.rng.type);
			dtemp = inflt();
			if (dtemp != 0.0)
				charac.rng.type = dtemp;
			break;
		case '8':	/* change ring duration */
			mvprintw(23,0,"rng-duration = %d; rng-duration;= ",charac.rng.duration);
			dtemp = inflt();
			if (dtemp != 0.0)
				charac.rng.duration = dtemp;
			break;
		case '7':	/* change palantir */
			mvprintw(23,0,"pal = %d; pal;= ",charac.pal);
			dtemp = inflt();
			if (dtemp != 0.0)
				{
				charac.pal = dtemp;
				charac.pal = (charac.pal != 0);
				}
			break;
		case 'u':	/* change poison */
			mvprintw(23,0,"psn = %f; psn;= ",charac.psn);
			dtemp = inflt();
			if (dtemp != 0.0)
				charac.psn = dtemp;
			break;
		case 'x':	/* change holy water */
			mvprintw(23,0,"hw = %d; hw;= ",charac.hw);
			dtemp = inflt();
			if (dtemp != 0.0)
				charac.hw = dtemp;
			break;
		case '1':	/* change amulet */
			mvprintw(23,0,"amu = %d; amu;= ",charac.amu);
			dtemp = inflt();
			if (dtemp != 0.0)
				charac.amu = dtemp;
			break;
		case '4':	/* change Blessing */
			mvprintw(23,0,"bls = %d; bls;= ",charac.bls);
			dtemp = inflt();
			if (dtemp != 0.0)
				{
				charac.bls = dtemp;
				charac.bls = (charac.bls != 0);
				}
			break;
		case 'y':	/* change Charm */
			mvprintw(23,0,"chm = %d; chm;= ",charac.chm);
			dtemp = inflt();
			if (dtemp != 0.0)
				charac.chm = dtemp;
			break;
		case 'w':	/* change Gems */
			mvprintw(23,0,"gem = %f; gem;= ",charac.gem);
			dtemp = inflt();
			if (dtemp != 0.0)
				charac.gem = dtemp;
			break;
		case 'i':	/* change Quicksilver */
			mvprintw(23,0,"quks = %d; quks;= ",charac.quks);
			dtemp = inflt();
			if (dtemp != 0.0)
				charac.quks = dtemp;
			break;
		case 'g':	/* change swords */
			mvprintw(23,0,"swd = %f; swd;= ",charac.swd);
			dtemp = inflt();
			if (dtemp != 0.0)
				charac.swd = dtemp;
			break;
		case 'l':	/* change shields */
			mvprintw(23,0,"shd = %f; shd;= ",charac.shd);
			dtemp = inflt();
			if (dtemp != 0.0)
				charac.shd = dtemp;
			break;
		case 's':	/* change type */
			mvprintw(23,0,"typ = %d; typ;= ",charac.typ);
			dtemp = inflt();
			if (dtemp != 0.0)
				charac.typ = dtemp;
			break;
		case '3':	/* change virgin */
			mvprintw(23,0,"vrg = %d; vrg;= ",charac.vrg);
			dtemp = inflt();
			if (dtemp != 0.0)
				{
				charac.vrg = dtemp;
				charac.vrg = (charac.vrg != 0);
				}
			break;
		case 'c':	/* change last-used */
			mvprintw(23,0,"last-used = %d; last-used;= ",charac.lastused);
			dtemp = inflt();
			if (dtemp != 0.0)
				charac.lastused = dtemp;
			break;
		case 'b':		/* change password */
			mvaddstr(23,0,"New password: ");
			getstring(s,60);
			if (*s)
				strcpy(charac.pswd,s);
			break;
		case 'a':		/* change name */
			mvaddstr(23,0,"New name: ");
			getstring(s,60);
			if (*s)
				strcpy(charac.name,s);
			break;
		case 'r':	/* change wormhole */
			mvprintw(23,0,"wormhole = %d; wormhole;= ",charac.wormhole);
			dtemp = inflt();
			if (dtemp != 0.0)
				charac.wormhole = dtemp;
			break;
		case '2':	/* change age */
			mvprintw(23,0,"age = %d; age;= ",charac.age);
			dtemp = inflt();
			if (dtemp != 0.0)
				charac.age = dtemp;
			break;
		case '6':	/* change blindness */
			mvprintw(23,0,"blind = %d; blind;= ",charac.blind);
			dtemp = inflt();
			if (dtemp != 0.0)
				{
				charac.blind = dtemp;
				charac.blind = (charac.blind != 0);
				}
			break;
		case '!':	/* quit, update */
			goto LEAVE;
		case '?':	/* delete char */
			strcpy(charac.name,"<null>");
			initchar(&charac);
			goto LEAVE;
		}
	goto TOP;
LEAVE:	charac.status = OFF;
	update(&charac,loc);
}

unsigned level(expr)			/* calculate level */
double	expr;
{
	if (expr < 1.1e+7)
		return (pow((expr/1000.0), 0.4875));
	else
		return (pow((expr/1250.0), 0.4865));
}

void	strunc(str)				/* remove blank spaces at the end of str[] */
register char	*str;
{
register int	loop;
	loop = strlen(str) - 1;
	while (str[--loop] == ' ')
		str[loop] = '\0';
}

double
inflt()				/* get a floating point # from the terminal */
{
        char	aline[80];
        double	res;

	getstring(aline,80);
	if (sscanf(aline, "%lf", &res) < 1)
		res = 0;
	return (res);
}

void	checkmov(stat)				/* see if beyond PONR */
register struct	stats	*stat;
{
	if (beyond)
		{
		stat->x = sgn(stat->x) * max(abs(stat->x),1.1e+6);
		stat->y = sgn(stat->y) * max(abs(stat->y),1.1e+6);
		}
}
void	scramble(stat)			/* mix up some stats */
register struct	stats	*stat;
{
double	buf[5],	temp;
register int	first, second;
register double	*bp;

	bp = buf;
	*bp++ = stat->str;
	*bp++ = stat->man;
	*bp++ = stat->brn;
	*bp++ = stat->mag;
	*bp++ = stat->nrg;

	bp = buf;
	first = roll(0,5);
	second = roll(0,5);
	temp = bp[first];
	bp[first] = bp[second];
	bp[second] = temp;

	stat->str = *bp++;
	stat->man = *bp++;
	stat->brn = *bp++;
	stat->mag = *bp++;
	stat->nrg = *bp++;
}
