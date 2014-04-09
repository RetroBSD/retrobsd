/*
 * func0.c	Phantasia support routines
 */

#include "phant.h"

void	treasure(stat,treastyp,size)		/* select a treasure */
register struct	stats	*stat;
short	treastyp;
register int	size;
{
register int	which;
int	ch;
double	temp, temp2;
char	aline[35];
FILE	*fp;

	which = roll(1,3);
	move(3,0);
	clrtobot();
	move(5,0);
	if (rnd() > 0.65)	/* gold and gems */
		if (treastyp > 7)	/* gems */
			{
			temp = roll(1,(treastyp - 7)*(treastyp - 7)*(size - 1)/4);
			printw("You have discovered %.0f gems!  Will you pick them up ? ",temp);
			ch = rgetch();
			addch('\n');
			if (toupper(ch) == 'Y')
				if (rnd() < treastyp/40 + 0.05)	/* cursed */
					{
					addstr("They were cursed!\n");
					goto CURSE;
					}
				else
					stat->gem += temp;
			return;
			}
		else	/* gold */
			{
			temp = roll(treastyp*10,treastyp*treastyp*10*(size - 1));
			printw("You have found %.0f gold pieces.  Do you want to pick them up ? ",temp);
			ch = rgetch();
			addch('\n');
			if (toupper(ch) == 'Y')
				if (rnd() < treastyp/35 + 0.04)	/* cursed */
					{
					addstr("They were cursed!\n");
					goto CURSE;
					}
				else
					{
					stat->gld += floor(0.9 * temp);
					fp = fopen(goldfile,"r");
					if (fread((char *) &temp2,sizeof(double),1,fp) != 1)
					        /*ignore*/;
					fclose(fp);
					fp = fopen(goldfile,"w");
					temp2 += floor(temp/10);
					fwrite((char *) &temp2,sizeof(double),1,fp);
					fclose(fp);
					}
			return;
			}
	else	/* other treasures */
		{
		addstr("You have found some treasure.  Do you want to inspect it ? ");
		ch = rgetch();
		addch('\n');
		if (toupper(ch) != 'Y')
			return;
		else
			if (rnd() < 0.08 && treastyp != 4)
				{
				addstr("It was cursed!\n");
				goto CURSE;
				}
			else
				switch(treastyp)
					{
					case 1:
						switch(which)
							{
							case 1:
								addstr("You've discovered a power booster!\n");
								stat->man += roll(size*4,size*30);
								break;
							case 2:
								addstr("You have encountered a druid.\n");
								stat->exp += roll(0,2000 + size*400);
								break;
							case 3:
								addstr("You have found a holy orb.\n");
								stat->sin = max(0,stat->sin - 0.25);
								break;
							}
						break;
					case 2:
						switch (which)
							{
							case 1:
								addstr("You have found an amulet.\n");
								++stat->amu;
								break;
							case 2:
								addstr("You've found some holy water!\n");
								++stat->hw;
								break;
							case 3:
								addstr("You've met a hermit!\n");
								stat->sin *= 0.75;
								stat->man += 12*size;
								break;
							}
						break;
					case 3:
						switch (which)
							{
							case 1:
								temp = roll(7,30 + size/10);
								printw("You've found a +%.0f shield!\n",temp);
								if (temp >= stat->shd)
									stat->shd = temp;
								else
									somebetter();
								break;
							case 2:
								addstr("You have rescued a virgin.  Will you be honorable ? ");
								ch = rgetch();
								if (toupper(ch) == 'Y')
									stat->vrg = TRUE;
								else
									{
									stat->exp += 2000*size;
									++stat->sin;
									}
								break;
							case 3:
								addstr("You've discovered some athelas!\n");
								--stat->psn;
								break;
							}
						break;
					case 4:
						addstr("You've found a scroll.  Will you read it ? ");
						ch = rgetch();
						addch('\n');
						if (toupper(ch) == 'Y')
							switch ((int) roll(1,6))
								{
								case 1:
									addstr("It throws up a shield for you next monster.\n");
									paws(8);
									longjmp(fightenv,2);
									/*NOTREACHED*/
								case 2:
									addstr("It makes you invisible for you next monster.\n");
									paws(8);
									speed = 1e6;
									longjmp(fightenv,0);
									/*NOTREACHED*/
								case 3:
									addstr("It increases your strength ten fold to fight your next monster.\n");
									paws(8);
									strength *= 10;
									longjmp(fightenv,0);
									/*NOTREACHED*/
								case 4:
									addstr("It is a general knowledge scroll.\n");
									stat->brn += roll(2,size);
									stat->mag += roll(1,size/2);
									break;
							case 5:
								addstr("It tells you how to pick your next monster.\n");
								addstr("Which monster do you want [0-99] ? ");
								which = inflt();
								which = min(99,max(0,which));
								fight(stat,which);
								break;
							case 6:
								addstr("It was cursed!\n");
								goto CURSE;
							}
						break;
					case 5:
						switch (which)
							{
							case 1:
								temp = roll(size/4+5,size/2 + 9);
								printw("You've discovered a +%.0f dagger.\n",temp);
								if (temp >= stat->swd)
									stat->swd = temp;
								else
									somebetter();
								break;
							case 2:
								temp = roll(7.5 + size*3,size * 2 + 160);
								printw("You have found some +%.0f armour!\n",temp);
								if (temp >= stat->shd)
									stat->shd = temp;
								else
									somebetter();
								break;
							case 3:
								addstr("You've found a tablet.\n");
								stat->brn += 4.5*size;
								break;
							}
						break;
					case 6:
						switch (which)
							{
							case 1:
								addstr("You've found a priest.\n");
								stat->nrg = stat->mxn + stat->shd;
								stat->sin /= 2;
								stat->man += 24*size;
								stat->brn += size;
								break;
							case 2:
								addstr("You have come upon Robin Hood!\n");
								stat->shd += size*2;
								stat->str += size/2.5 + 1;
								break;
							case 3:
									temp = roll(2 + size/4,size/1.2 + 10);
								printw("You have found a +%.0f axe!\n",temp);
								if (temp >= stat->swd)
									stat->swd = temp;
								else
									somebetter();
								break;
							}
						break;
					case 7:
						switch (which)
							{
							case 1:
								addstr("You've discovered a charm!\n");
								++stat->chm;
								break;
							case 2:
								addstr("You have encountered Merlyn!\n");
								stat->brn += size + 5;
								stat->mag += size/3 + 5;
								stat->man += size*10;
								break;
							case 3:
								temp = roll(5+size/3,size/1.5 + 20);
								printw("You have found a +%.0f war hammer!\n",temp);
								if (temp >= stat->swd)
									stat->swd = temp;
								else
									somebetter();
								break;
							}
						break;
					case 8:
						switch (which)
							{
							case 1:
								addstr("You have found a healing potion.\n");
								stat->psn = min(-2,stat->psn-2);
								break;
							case 2:
								addstr("You have discovered a transporter.  Do you wish to go anywhere ? ");
								ch = rgetch();
								addch('\n');
								if (toupper(ch) == 'Y')
									{
									addstr("X Y Coordinates ? ");
									getstring(aline,80);
									sscanf(aline,"%lf %lf",&stat->x,&stat->y);
									stat->x = floor(stat->x);
									stat->y = floor(stat->y);
									}
								break;
							case 3:
								temp = roll(10 + size/1.2,size*3 + 30);
								printw("You've found a +%.0f sword!\n",temp);
								if (temp >= stat->swd)
									stat->swd = temp;
								else
									somebetter();
								break;
							}
						break;
					case 10:
					case 11:
					case 12:
					case 13:
						if (rnd() < 0.33)
							{
							if (treastyp == 10)
								{
								addstr("You've found a pair of elven boots!\n");
								stat->quk += 2;
								break;
								}
							else if (treastyp == 11 && !stat->pal)
								{
								addstr("You've acquired Saruman's palantir.\n");
								stat->pal = TRUE;
								break;
								}
							else if (!stat->rng.type && stat->typ < 20 && (treastyp == 12 || treastyp == 13))
								{
								if (treastyp == 12)
									if (rnd() < 0.8)
										{
										which = NAZREG;
										temp = 15;
										}
									else
										{
										which = NAZBAD;
										temp = 10 + rngcalc(stat->typ) + roll(0,5);
										}
								else
									if (rnd() > 0.9)
										{
										which = DLREG;
										temp = 0;
										}
									else
										{
										which = DLBAD;
										temp = 15 + rngcalc(stat->typ) + roll(0,5);
										}
									addstr("You've discovered a ring.  Will you pick it up ? ");
									ch = rgetch();
									addch('\n');
									if (toupper(ch) == 'Y')
										{
										stat->rng.type = which;
										stat->rng.duration = temp;
										}
								}
							break;
							}
						case 9:
							switch (which)
								{
								case 1:
									if (!(stat->lvl > 1000 || stat->crn > floor((double) stat->lvl/100)
											|| stat->lvl < 10))
										{
										addstr("You have found a golden crown!\n");
										++stat->crn;
										break;
										}
								case 2:
									addstr("You've been blessed!\n");
									stat->bls = TRUE;
									stat->sin /=3;
									stat->nrg = stat->mxn + stat->shd;
									stat->man += 100*size;
									break;
								case 3:
									temp = roll(1,size/5+5);
									temp = min(temp,99);
									printw("You have discovered some +%.0f quicksilver!\n",temp);
									if (temp >= stat->quks)
										stat->quks = temp;
									else
										somebetter();
									break;
								}
							break;
						}
		refresh();
		return;
		}
CURSE:	if (stat->chm)
		{
		addstr("But your charm saved you!\n");
		--stat->chm;
		}
	else if (stat->amu)
		{
		addstr("But your amulet saved you!\n");
		--stat->amu;
		}
	else
		{
		stat->nrg = (stat->mxn + stat->shd)/10;
		stat->psn += 0.25;
		}
}

void	callmonster(which,size,mons)		/* fill a structure with monster 'which' of size 'size' */
register int	which, size;
register struct	mstats	*mons;
{
FILE	*fp;
char	instr[100];

	which = min(which,99);
	fp = fopen(monsterfile,"r");
	for (++which; which; --which) {
		if (fgets(instr,100,fp) == 0)
		        /*ignore*/;
        }
	strncpy(mons->name,instr,24);
	mons->name[24] = '\0';
	sscanf(instr + 24,"%lf%lf%lf%lf%lf%d%d%d",&mons->str,&mons->brn,&mons->spd,&mons->hit,
		&mons->exp,&mons->trs,&mons->typ,&mons->flk);
	if (mons->typ == 2)	/* Modnar */
		{
		mons->str *= rnd() + 0.5;
		mons->brn *= rnd() + 0.5;
		mons->spd *= rnd() + 0.5;
		mons->hit *= rnd() + 0.5;
		mons->exp *= rnd() + 0.5;
		mons->trs *= rnd();
		}
	else if (mons->typ == 3)	/* mimic */
		{
		fseek(fp,0L,0);
		for (which = roll(0,100); which; --which) {
			if (fgets(instr,100,fp) == 0)
			        /*ignore*/;
                }
		strncpy(mons->name,instr,24);
		}
	strunc(mons->name);
	mons->str += (size-1)*mons->str/2;
	mons->brn *= size;
	mons->spd += size * 1.e-9;
	mons->hit *= size;
	mons->exp *= size;
	fclose(fp);
}

struct	/* lookup table for rolling stats and making increases upon gaining levels */
	{
	struct
		{
		int	base;
		int	interval;
		float	increase;
		} quick,      strength,      manna,	    energy,	   brains,	  magic;
	} table[7] =
		{
/* mag. usr: */ 30, 6, 0.0,   20, 6, 2.0,    50,51,75.0,    30,16,20.0,    60,26, 6.0,	  5, 5,2.75,
/* fighter:  */ 30, 6, 0.0,   40,16, 3.0,    30,21,40.0,    45,26,30.0,    25,21, 3.0,	  3, 4, 1.5,
/* elf:      */ 32, 7, 0.0,   35,11, 2.5,    45,46,65.0,    30,21,25.0,    40,26, 4.0,	  4, 4, 2.0,
/* dwarf:    */ 25, 6, 0.0,   45,21, 5.0,    25,21,30.0,    60,41,35.0,    20,21, 2.5,	  2, 4, 1.0,
/* halfling: */ 34, 0, 0.0,   20, 6, 2.0,    25,21,30.0,    55,36,30.0,    40,36, 4.5,	  1, 4, 1.0,
/* exprmnto: */ 27, 0, 0.0,   25, 0, 0.0,    100,0, 0.0,    35, 0, 0.0,    25, 0, 0.0,	  2, 0, 0.0,
/* super:    */ 38, 0, 0.0,   65, 0, 0.0,    100,0, 0.0,    80, 0, 0.0,    85, 0, 0.0,	  9, 0, 0.0
		};

void	genchar(res,type)				/* init a charac struct */
int	type;
register struct	stats	*res;
{
register int	subscript;

	if (type < '1' || type > '6')
		if (type != '7' || !su)
			type = '2';	/* fighter is default */
	subscript = type - '1';
	res->quk = roll(table[subscript].quick.base,table[subscript].quick.interval);
	res->str = roll(table[subscript].strength.base,table[subscript].strength.interval);
	res->man = roll(table[subscript].manna.base,table[subscript].manna.interval);
	res->mxn = res->nrg = roll(table[subscript].energy.base,table[subscript].energy.interval);
	res->brn = roll(table[subscript].brains.base,table[subscript].brains.interval);
	res->mag = roll(table[subscript].magic.base,table[subscript].magic.interval);
	res->typ = subscript;
	if (subscript < 6)
		++res->typ;
	if (type == '5')
		res->exp = roll(600,200);	/* give halfling some exp. */
}

void	movelvl(stat)				/* update stats for new level */
register struct	stats	*stat;
{
register int	type;
register unsigned new;
double	inc;

	changed = TRUE;
	type = abs(stat->typ);
	if (type < 6)
		;	/* normal */
	else if (type < 10)
		type = roll(1,5);	/* experimento */
	else if (type < 20)
		{
		type -= 10;	/* king */
		if (type > 5)
			type = roll(1,5);	/* experimento */
		}
	else if (type < 26)
		type -= 20;	/* council of wise */
	else
		type = roll(1,5);	/* everything else */
	new = level(stat->exp);
	inc = new - stat->lvl;
	--type; 		/* set up for subscripting into table */
	stat->str += table[type].strength.increase * inc;
	stat->man += table[type].manna.increase * inc;
	stat->brn += table[type].brains.increase * inc;
	stat->mag += table[type].magic.increase * inc;
	stat->mxn += table[type].energy.increase * inc;
	stat->nrg = stat->mxn + stat->shd;
	if ((stat->lvl = min(10000,new)) >= 1000)
		{	/* no longer able to be king */
		stat->gld += stat->crn * 5000;
		stat->crn = 0;
		stat->typ = abs(stat->typ);
		}
	if (stat->lvl >= 3000 && stat->typ < 20)
		{	/* make a member of the council */
		mvaddstr(6,0,"You have made it to the Council of the Wise.\nGood Luck on your search for the Holy Grail.\n");
		stat->rng.type = 0;
		stat->rng.duration = 3;
		stat->typ = abs(stat->typ) + (stat->typ > 10 ? 10 :20);
		}
}

char	*printloc(x,y)			/* return a pointer to a string specifying location */
double	x,y;		/* also, set some global flags */
{
register int	size, loc;
register char	*label;
static	char	res[80],
		*nametable[4][4] =	 /* names of places */
		{
		"Anorien",	"Ithilien",	"Rohan",	"Lorien",
		"Gondor",	"Mordor",	"Dunland",	"Rovanion",
		"South Gondor", "Khand",	"Eriador",	"The Iron Hills",
		"Far Harad",	"Near Harad",	"The Northern Waste", "Rhun"
		};

	throne = beyond = marsh = FALSE;
	if (wmhl)
		return (strcpy(res," is in the Wormholes"));
	else if (valhala)
		return (strcpy(res," is in Valhala"));
	else if ((size = circ(x,y)) >= 1000)
		{
		if (max(abs(x),abs(y)) > 1100000)
			{
			label = "The Point of No Return";
			beyond = TRUE;
			}
		else
			label = "The Ashen Mountains";
		}
	else if (size >= 55)
		label = "Morannon";
	else if (size >= 35)
		label = "Kennaquahair";
	else if (size >= 20)
		{
		label = "The Dead Marshes";
		marsh = TRUE;
		}
	else if (size >= 9)
		label = "The Outer Waste";
	else if (size >= 5)
		label = "The Moors Adventurous";
	else
		{
		if (!x && !y)
			{
			label = "The Lord's Chamber";
			throne = TRUE;
			}
		else
			{
			loc = (x > 0) + 2 * (y >= 0);
			label = nametable[size-1][loc];
			}
		}
	sprintf(res," is in %s  (%.0f,%.0f)",label,x,y);
	return (res);
}

void	initchar(stat)				/* put in some default values */
register struct	stats	*stat;
{
	stat->x = roll(-125,251);
	stat->y = roll(-125,251);
	stat->exp = stat->lvl = stat->sin = 0;
	stat->crn = stat->psn = 0;
	stat->rng.type = NONE;
	stat->rng.duration = 0;
	stat->blind = stat->vrg = stat->pal = FALSE;
	stat->hw = stat->amu = stat->bls = 0;
	stat->chm = 0;
	stat->gem = 0.1;
	stat->gld = roll(25,50) + roll(0,25) + 0.1;
	stat->quks = stat->swd = stat->shd = 0;
	stat->typ = 0;
	stat->status = stat->tampered = OFF;
	stat->scratch1 = stat->scratch2 = 0.0;
	stat->wormhole = 0;
	stat->age = 0;
	stat->degen = 1;
}

void	trade(stat)				/* trading post */
register struct	stats	*stat;
{
static	struct
		{
		char	*item;
		int	cost;
		}	menu[] =
			{
			"Manna",1,
			"Shield",5,
			"Book",200,
			"Sword",500,
			"Charm",1000,
			"Quiksilver",2500,
			"Blessing",7000,
			"Gem",1000	/* this is only to ease changing the value of gems */
			};
double	temp;
int	ch;
register int	size, loop;
bool	cheat = FALSE;

	changed = TRUE;
	clear();
	addstr("You are at a trading post. All purchases must be made with gold.");
	size = sqrt(abs(stat->x/100)) + 1;
	size = min(7,size);
	mvprintw(4,0,"L:Leave  P:Purchase  S:Sell Gems ? ");
	move(6,0);
	for (loop = 0; loop < size; ++loop)
		printw("(%d) %-10s: %6d\n",loop+1,menu[loop].item,menu[loop].cost);
PROMPT:	mvprintw(1,0,"Gold:  %9.0f  Gems:  %9.0f  Level:   %6u  Charms: %6d\n",stat->gld,stat->gem,stat->lvl,stat->chm);
	printw("Shield:%9.0f  Sword: %9.0f  Quicksilver:%3d  Blessed: %s",
		stat->shd,stat->swd,stat->quks,(stat->bls ? " True" : "False"));
	move(4,36);
	ch = rgetch();
	move(15,0);
	clrtobot();
	switch(toupper(ch))
		{
		case 'L':
		case '\n':
			stat->x -= floor(stat->x/10);
			stat->y -= floor(stat->y/10);
			return;
		case 'P':
			mvaddstr(15,0,"What what would you like to buy ? ");
			ch = rgetch();
			move(15,0);
			clrtoeol();
			if (ch - '0' > size)
				addstr("Sorry, this merchant doesn't have that.");
			else
				switch (toupper(ch))
					{
					case '1':
						printw("Manna is one per %d gold piece.  How many do you want (%.0f max) ? ",menu[0].cost,floor(stat->gld/menu[0].cost));
						temp = inflt();
						if (temp * menu[0].cost > stat->gld || temp < 0)
							goto CHEAT;
						else
							{
							stat->gld -= floor(temp) * menu[0].cost;
							if (rnd() < 0.02)
								goto DISHON;
							else
								stat->man += floor(temp);
							}
						break;
					case '2':
						printw("Shields are %d per +1.  How many do you want (%.0f max) ? ",menu[1].cost,floor(stat->gld/menu[1].cost));
						temp = inflt();
						if (!temp)
							break;
						if (temp * menu[1].cost > stat->gld || temp < 0)
							goto CHEAT;
						else
							{
							stat->gld -= floor(temp) * menu[1].cost;
							if (rnd() < 0.02)
								goto DISHON;
							else
								stat->shd = floor(temp);
							}
						break;
					case '3':
						printw("A book costs %d gp.  How many do you want (%.0f max) ? ",menu[2].cost,floor(stat->gld/menu[2].cost));
						temp = inflt();
						if (temp * menu[2].cost > stat->gld || temp < 0)
							goto CHEAT;
						else
							{
							stat->gld -= floor(temp) * menu[2].cost;
							if (rnd() < 0.02)
								goto DISHON;
							else
								if (rnd()*temp > stat->lvl/10 && temp != 1)
									{
									printw("\nYou blew your mind!\n");
									stat->brn /= 5;
									}
								else
									stat->brn += floor(temp)*roll(25,10);
							}
						break;
					case '4':
						printw("Swords are %d gp per +1.  How many + do you want (%.0f max) ? ",menu[3].cost,floor(stat->gld/menu[3].cost));
						temp = inflt();
						if (!temp)
							break;
						if (temp * menu[3].cost > stat->gld || temp < 0)
							goto CHEAT;
						else
							{
							stat->gld -= floor(temp) * menu[3].cost;
							if (rnd() < 0.02)
								goto DISHON;
							else
								stat->swd = floor(temp);
							}
						break;
					case '5':
						printw("A charm costs %d gp.  How many do you want (%.0f max) ? ",menu[4].cost,floor(stat->gld/menu[4].cost));
						temp = inflt();
						if (temp * menu[4].cost > stat->gld || temp < 0)
							goto CHEAT;
						else
							{
							stat->gld -= floor(temp) * menu[4].cost;
							if (rnd() < 0.02)
								goto DISHON;
							else
								stat->chm += floor(temp);
							}
						break;
					case '6':
						printw("Quicksilver is %d gp per +1.  How many + do you want (%.0f max) ? ",menu[5].cost,floor(stat->gld/menu[5].cost));
						temp = inflt();
						if (!temp)
							break;
						if (temp * menu[5].cost > stat->gld || temp < 0)
							goto CHEAT;
						else
							{
							stat->gld -= floor(temp) * menu[5].cost;
							if (rnd() < 0.02)
								goto DISHON;
							else
								stat->quks = min(99,floor(temp));
							}
						break;
					case '7':
						printw("A blessing requires a %d gp donation.  Still want one ? ",menu[6].cost);
						ch = rgetch();
						if (toupper(ch) == 'Y')
							if (stat->gld < menu[6].cost)
								goto CHEAT;
							else
								{
								stat->gld -= menu[6].cost;
								if (rnd() < 0.02)
									goto DISHON;
								else
									stat->bls = TRUE;
								}
						break;
					}
			break;
		case 'S':
			mvprintw(15,0,"A gem is worth %d gp.  How many do you want to sell (%.0f max) ? ",menu[7].cost,stat->gem);
			temp = inflt();
			if (temp > stat->gem || temp < 0)
				goto CHEAT;
			else
				{
				stat->gem -= floor(temp);
				stat->gld += floor(temp) * menu[7].cost;
				}
		}
	goto PROMPT;

CHEAT:	move(17,0);
	if (!cheat)
		{
		addstr("Come on, merchants aren't stupid.  Stop cheating.\n");
		cheat = TRUE;
		goto PROMPT;
		}
	else
		{
		addstr("You had your chance.  This merchant happens to be\n");
		printw("a %.0f level magic user, and you made %s mad!\n",roll(size*10,size*20),(rnd() < 0.5) ? "him" : "her");
		stat->x += roll(-250,500)*size;
		stat->y += roll(-250,500)*size;
		stat->nrg = min(size*20,stat->mxn);
		++stat->sin;
		paws(23);
		}
	return;

DISHON:	mvaddstr(17,0,"The merchant stole your money!");
	refresh();
	stat->x -= floor(stat->x/10);
	stat->y -= floor(stat->y/10);
	sleep(2);
}

void	printstats(stat)				/* show characteristics */
register struct	stats	*stat;
{
	mvprintw(0,0,"%s%s\n",stat->name,printloc(stat->x,stat->y));
	mvprintw(1,0,"Level :%7u   Energy  :%9.0f(%9.0f)  Manna:%9.0f  Users:%3d\n",
		stat->lvl,stat->nrg,stat->mxn + stat->shd,stat->man,users);
	mvprintw(2,0,"Quick :%3.0f(%3d)  Strength:%9.0f(%9.0f)  Gold :%9.0f    ",
		speed,stat->quk + stat->quks,strength,stat->str + stat->swd,stat->gld);
	switch (stat->status)
		{
		case PLAYING:
			if (stat->nrg < 0.2 * (stat->mxn + stat->shd))
				addstr("Low Energy\n");
			else if (stat->blind)
				addstr("Blind\n");
			else
				clrtoeol();
			break;
		case CLOAKED:
			addstr("Cloaked\n");
			break;
		case INBATTLE:
			addstr("In Battle\n");
			break;
		case OFF:
			addstr("Off\n");
		}
}

void	showall(stat)				/* show special items */
register struct	stats	*stat;
{
static	char	*flags[] =
			{
			"False",
			" True"
			};

	mvprintw(6,0,"Type: %3d  --  ",stat->typ);
	switch (abs(stat->typ))
		{
		case 1:
		case 11:
		case 21:
			addstr("Magic User");
			break;
		case 2:
		case 12:
		case 22:
			addstr("Fighter");
			break;
		case 3:
		case 13:
		case 23:
			addstr("Elf");
			break;
		case 4:
		case 14:
		case 24:
			addstr("Dwarf");
			break;
		case 5:
		case 15:
		case 25:
			addstr("Halfling");
			break;
		case 6:
		case 16:
		case 26:
			addstr("Experimento");
			break;
		case 90:
			addstr("Ex-Valar");
			break;
		case 99:
			addstr("Valar");
		}
	if (stat->typ > 10 && stat->typ < 90)
		if (stat->typ > 20)
			addstr(" (Council of Wise)");
		else
			addstr(" (King)");
		addch('\n');
	mvprintw(7,0,"Experience: %9.0f",stat->exp);
	mvprintw(8,0,"Brains    : %9.0f",stat->brn);
	mvprintw(9,0,"Magic Lvl : %9.0f",stat->mag);
	mvprintw(10,0,"Sin       : %9.5f",stat->sin);
	mvprintw(11,0,"Poison    : %9.5f",stat->psn);
	mvprintw(12,0,"Gems      : %9.0f",stat->gem);
	mvprintw(13,0,"Age       : %9d",stat->age);
	mvprintw(7,40,"Holy Water: %9d",stat->hw);
	mvprintw(8,40,"Amulets   : %9d",stat->amu);
	mvprintw(9,40,"Charms    : %9d",stat->chm);
	mvprintw(10,40,"Crowns    : %9d",stat->crn);
	mvprintw(11,40,"Shield    : %9.0f",stat->shd);
	mvprintw(12,40,"Sword     : %9.0f",stat->swd);
	mvprintw(13,40,"Quickslver: %9d",stat->quks);

	mvprintw(14,0,"Blessing: %s   Ring: %s   Virgin: %s   Palantir: %s",
		flags[stat->bls],flags[stat->rng.type != 0],flags[stat->vrg],flags[stat->pal]);
}

void	neatstuff(stat) 			/* random things */
register struct	stats	*stat;
{
double	temp;
int	ch;

	switch ((int) roll(0,100))
		{
		case 1:
		case 2:
			if (stat->psn > 0)
				{
				mvaddstr(5,0,"You've found a medic!  How much will you offer to be cured ? ");
				temp = inflt();
				if (temp < 0 || temp > stat->gld)
					{
					mvaddstr(6,0,"He was not amused, and made you worse.\n");
					++stat->psn;
					}
				else if (rnd()/2.0 > (temp + 1)/max(stat->gld,1))
					mvaddstr(6,0,"Sorry, he wasn't interested.\n");
				else
					{
					mvaddstr(6,0,"He accepted.");
					stat->psn = max(0,stat->psn-1);
					stat->gld -= floor(temp);
					}
				}
			break;
		case 3:
			mvaddstr(6,0,"You've been caught raping and pillaging!\n");
			stat->exp += 4000;
			stat->sin += 0.5;
			break;
		case 4:
			temp = roll(6,50);
			mvprintw(5,0,"You've found %.0f gold pieces, want them ? ",temp);
			clrtoeol();
			ch = rgetch();
			if (toupper(ch) == 'Y')
				stat->gld += temp;
			break;
		case 5:
			if (stat->sin > 1)
				{
				mvaddstr(6,0,"You've found a Holy Orb!\n");
				stat->sin -= 0.25;
				}
			break;
		case 6:
			if (stat->psn < 1)
				{
				mvaddstr(6,0,"You've been hit with a plague!\n");
				++stat->psn;
				}
			break;
		case 7:
			mvaddstr(6,0,"You've found some holy water.\n");
			++stat->hw;
			break;
		case 8:
			mvaddstr(6,0,"You've met a Guru. . .");
			if (rnd()*stat->sin > 1)
				addstr("You disgusted him with your sins!\n");
			else if (stat->psn > 0)
				{
				addstr("He looked kindly upon you, and cured you.\n");
				stat->psn = 0;
				}
			else
				{
				addstr("He rewarded you for your virtue.\n");
				stat->man += 50;
				stat->shd += 2;
				}
			break;
		case 9:
			mvaddstr(6,0,"You've found an amulet.\n");
			++stat->amu;
			break;
		case 10:
			if (stat->blind)
				{
				mvaddstr(6,0,"You've regained your sight!\n");
				stat->blind = FALSE;
				}
			break;
		default:
			if (stat->psn > 0)
				stat->nrg -= min(stat->psn*stat->mxn/45,0.9*stat->mxn);
			stat->nrg = max(stat->nrg,floor(stat->mxn/11));
		}
}
