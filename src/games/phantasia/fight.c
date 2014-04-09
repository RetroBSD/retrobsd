/*
 * fight.c   Phantasia monster fighting routine
 *
 *	1.1	(2.11BSD) 1996/10/26
 */

/*
 * The code exists here for fight to the finish.  Simply add code to
 * set 'fgttofin = TRUE' as an option.	Everything else is here.
 */
#include "phant.h"

void	fight(stat,particular)			/* monster fighting routine */
        register struct	stats	*stat;
        int	particular;
{
        bool	fghttofin = FALSE, luckout = FALSE;
        char	aline[80];
        double	monhit, mdamage, sdamage, monspd, maxspd, inflict, monstr, temp, shield;
        int	ch;
        register int	whichm, size, hwmany, lines;
        struct	mstats	monster;

	fghting = changed = TRUE;
	shield = 0.0;
	if (setjmp(fightenv) == 2)
		shield = roll(100 + (stat->mxn + stat->shd)*6.2,3000);
	hwmany = 0;
	size = (valhala) ? stat->lvl/5 : circ(stat->x,stat->y);
	if (particular >= 0)
		whichm = particular;
	else if (marsh)
		whichm = roll(0,15);
	else if (size > 24)
		whichm = roll(14,86);
	else if (size > 15)
		whichm = roll(0,50) + roll(14,37);
	else if (size > 8)
		whichm = roll(0,50) + roll(14,26);
	else if (size > 3)
		whichm = roll(14,50);
	else
		whichm = roll(14,25);

CALL:	move(3,0);
	clrtobot();
	move(5,0);
	lines = 6;
	callmonster(whichm,size,&monster);
	if (stat->blind)
		strcpy(monster.name,"a monster");
	++hwmany;
	if (monster.typ == 1)	/* unicorn */
		if (stat->vrg)
			{
			printw("You just subdued %s, thanx to the virgin.",monster.name);
			stat->vrg = FALSE;
			goto FINISH;
			}
		else
			{
			printw("You just saw %s running away!",monster.name);
			goto LEAVE;
			}
	if (monster.typ == 2 && stat->typ > 20)
		{
		strcpy(monster.name,"Morgoth");
		monster.str = rnd()*(stat->mxn + stat->shd)/1.4 + rnd()*(stat->mxn + stat->shd)/1.5;
		monster.brn = stat->brn;
		monster.hit = stat->str*30;
		monster.typ = 23;
		monster.spd = speed*1.1 + speed*(stat->typ == 90);
		monster.flk = monster.trs = monster.exp = 0;
		mvprintw(4,0,"You've encountered %s, Bane of the Council and Valar.",monster.name);
		}
	fghttofin = luckout = FALSE;
	monstr = monster.str;
	monhit = monster.hit;
	mdamage = sdamage = 0;
	monspd = maxspd = monster.spd;
	*monster.name = toupper(*monster.name);

TOP:	mvprintw(5,0,"You are being attacked by %s,   EXP: %.0f   (Size: %d)",monster.name,monster.exp,size);
	printstats(stat);
	mvprintw(1,26,"%20.0f",stat->nrg + shield);
	if (monster.typ == 4 && stat->bls && stat->chm)
		{
		mvprintw(6,0,"You just overpowered %s!",monster.name);
		lines = 7;
		stat->bls = FALSE;
		--stat->chm;
		goto FINISH;
		}
	monster.spd = min(monster.spd + 1,maxspd);
	if (rnd()*monster.spd > rnd()*speed && monster.typ != 4 && monster.typ != 16)
		{
		if (monster.typ)
			switch (monster.typ)	/* do special things */
				{
				case 5: /* Leanan-Sidhe */
					if (rnd() > 0.25)
						goto NORMALHIT;
					inflict = roll(1,(size - 1)/2);
					inflict = min(stat->str,inflict);
					mvprintw(lines++,0,"%s sapped %0.f of your strength!",monster.name,inflict);
					stat->str -= inflict;
					strength -= inflict;
					break;
				case 6: /* Saruman */
					if (stat->pal)
						{
						mvprintw(lines++,0,"Wormtongue stole your palantir!");
						stat->pal = FALSE;
						}
					else if (rnd() > 0.2)
						goto NORMALHIT;
					else if (rnd() > 0.5)
						{
						mvprintw(lines++,0,"%s transformed your gems into gold!",monster.name);
						stat->gld += stat->gem;
						stat->gem = 0.0;
						}
					else
						{
						mvprintw(lines++,0,"%s scrambled your stats!",monster.name);
						scramble(stat);
						}
					break;
				case 7: /* Thaumaturgist */
					if (rnd() > 0.15)
						goto NORMALHIT;
					mvprintw(lines++,0,"%s transported you!",monster.name);
					stat->x += sgn(stat->x)*roll(50*size,250*size);
					stat->y += sgn(stat->y)*roll(50*size,250*size);
					goto LEAVE;
				case 8: /* Balrog */
					inflict = roll(10,monster.str);
					inflict = min(stat->exp,inflict);
					mvprintw(lines++,0,"%s took away %0.f experience points.",monster.name,inflict);
					stat->exp -= inflict;
					break;
				case 9: /* Vortex */
					if (rnd() > 0.2)
						goto NORMALHIT;
					inflict = roll(0,7.5*size);
					inflict = min(stat->man,floor(inflict));
					mvprintw(lines++,0,"%s sucked up %.0f of your manna!",monster.name,inflict);
					stat->man -= inflict;
					break;
				case 10:	/* Nazgul */
					if (rnd() > 0.3)
						goto NORMALHIT;
					if (stat->rng.type && stat->rng.type < 10)
						{
						mvaddstr(lines++,0,"Will you relinguish your ring ? ");
						ch = rgetch();
						if (toupper(ch) == 'Y')
							{
							stat->rng.type = NONE;
							goto LEAVE;
							}
						}
					mvprintw(lines++,0,"%s neutralized 1/5 of your brain!",monster.name);
					stat->brn *= 0.8;
					break;
				case 11:	/* Tiamat */
					if (rnd() > 0.6)
						goto NORMALHIT;
					mvprintw(lines++,0,"%s took half your gold and gems and flew off.",monster.name);
					stat->gld = floor(stat->gld/2);
					stat->gem = floor(stat->gem/2);
					goto LEAVE;
				case 12:	/* Kobold */
					if (rnd() >.7)
						goto NORMALHIT;
					mvprintw(lines++,0,"%s stole one gold piece and ran away.",monster.name);
					stat->gld = max(0,stat->gld-1);
					goto LEAVE;
				case 13:	/* Shelob */
					if (rnd() > 0.5)
						goto NORMALHIT;
					mvprintw(lines++,0,"%s has bitten and poisoned you!",monster.name);
					++stat->psn;
					break;
				case 14:	/* Faeries */
					if (!stat->hw)
						goto NORMALHIT;
					mvprintw(lines++,0,"Your holy water killed it!");
					--stat->hw;
					goto FINISH;
				case 15:	/* Lamprey */
					if (rnd() > 0.7)
						goto NORMALHIT;
					mvprintw(lines++,0,"%s bit and poisoned you!",monster.name);
					stat->psn += 0.25;
					break;
				case 17:	/* Bonnacon */
					if (rnd() > 0.1)
						goto NORMALHIT;
					mvprintw(lines++,0,"%s farted and scampered off.",monster.name);
					stat->nrg /= 2;
					goto LEAVE;
				case 18:	/* Smeagol */
					if (rnd() > 0.5 || !stat->rng.type)
						goto NORMALHIT;
					mvprintw(lines++,0,"%s tried to steal your ring, ",monster.name);
					if (rnd() > 0.1)
						addstr("but was unsuccessful.");
					else
						{
						addstr("and ran away with it!");
						stat->rng.type = NONE;
						goto LEAVE;
						}
					break;
				case 19:	/* Succubus */
					if (rnd() > 0.3)
						goto NORMALHIT;
					inflict = roll(15,size*10);
					inflict = min(inflict,stat->nrg);
					mvprintw(lines++,0,"%s sapped %0.f of your energy.",monster.name,inflict);
					stat->nrg -= inflict;
					break;
				case 20:	/* Cerberus */
					if (rnd() > 0.25)
						goto NORMALHIT;
					mvprintw(lines++,0,"%s took all your metal treasures!",monster.name);
					stat->swd = stat->shd =stat->gld = stat->crn = 0;
					goto LEAVE;
				case 21:	/* Ungoliant */
					if (rnd() > 0.1)
						goto NORMALHIT;
					mvprintw(lines++,0,"%s poisoned you, and took one quik.",monster.name);
					stat->psn += 5;
					--stat->quk;
					break;
				case 22:	/* Jabberwock */
					if (rnd() > 0.1)
						goto NORMALHIT;
					mvprintw(lines++,0,"%s flew away, and left you to contend with one of its friends.",monster.name);
					whichm = 55 + 22*(rnd() > 0.5);
					goto CALL;
				case 24:	/* Troll */
					if (rnd() > 0.5)
						goto NORMALHIT;
					mvprintw(lines++,0,"%s partially regenerated his energy.!",monster.name);
					monster.hit += floor((monhit*size - monster.hit)/2);
					monster.str = monstr;
					mdamage = sdamage = 0;
					maxspd = monspd;
					break;
				case 25:	/* wraith */
					if (rnd() > 0.3 || stat->blind)
						goto NORMALHIT;
					mvprintw(lines++,0,"%s blindeed you!",monster.name);
					stat->blind = TRUE;
					break;
				default:
					goto NORMALHIT;
				}
		else
NORMALHIT:		{
			inflict = rnd()*monster.str + 0.5;
			mvprintw(lines++,0,"%s hit you %.0f times!",monster.name,inflict);
SPECIALHIT:		if ((shield -= inflict) < 0)
				{
				stat->nrg += shield;
				shield = 0;
				}
			}
		}
	else
		{
		if (fghttofin)
			goto MELEE;
		mvaddstr(3,0,"1:Melee  2:Skirmish  3:Evade  4:Spell  5:Nick  ");
		if (!luckout)
			if (monster.typ == 23)
				addstr("6:Ally  ");
			else
				addstr("6:Luckout  ");
		if (stat->rng.type > 0)
			addstr("7:Use Ring  ");
		else
			clrtoeol();
		ch = gch(stat->rng.type);
		move(lines = 6,0);
		clrtobot();
		switch (ch)
			{
			default:
			case '1':	/* melee */
MELEE:				inflict = roll(strength/2 + 5,1.3*strength) + (stat->rng.type < 0 ? strength : 0);
				mdamage += inflict;
				monster.str = monstr - mdamage/monhit*monstr/4;
				goto HITMONSTER;
			case '2':	/* skirmish */
				inflict = roll(strength/3 + 3,1.1*strength) + (stat->rng.type < 0 ? strength : 0);
				sdamage += inflict;
				maxspd = monspd - sdamage/monhit*monspd/4;
				goto HITMONSTER;
			case '3':	/* evade */
				if ((monster.typ == 4 || monster.typ == 16
				|| rnd()*speed*stat->brn > rnd()*monster.spd*monster.brn)
				&& (monster.typ != 23))
					{
					mvaddstr(lines++,0,"You got away!");
					stat->x += roll(-2,5);
					stat->y += roll(-2,5);
					goto LEAVE;
					}
				else
					mvprintw(lines++,0,"%s is still after you!",monster.name);
				break;
			case '4':	/* spell */
				lines = 7;
				mvaddstr(3,0,"\n\n");
				mvaddstr(3,0,"1:All or Nothing");
				if (stat->mag >= 3)
					mvaddstr(3,18,"2:Magic Bolt");
				if (stat->mag >= 7)
					mvaddstr(3,32,"3:Force Field");
				if (stat->mag >= 10)
					mvaddstr(3,47,"4:Transform");
				if(stat->mag >= 15)
					mvaddstr(3,60,"5:Increase Might\n");
				if (stat->mag >= 20)
					mvaddstr(4,0,"6:Invisibility");
				if (stat->mag >= 25)
					mvaddstr(4,18,"7:Transport");
				if (stat->mag >= 30)
					mvaddstr(4,32,"8:Paralyze");
				if (stat->typ > 20)
					mvaddstr(4,52,"9:Specify");
				mvaddstr(6,0,"Spell ? ");
				ch = rgetch();
				mvaddstr(3,0,"\n\n");
				if (monster.typ == 23 && ch != '4')
					illspell();
				else
					switch (ch)
						{
						case '1':	/* all or nothing */
							{
							inflict = (rnd() < 0.25) ? (monster.hit*1.0001 + 1) : 0;
							if (monster.typ == 4)
								inflict *= .9;
							if (stat->man)
								--stat->man;
							maxspd *= 2;
							monspd *= 2;
							monster.spd = max(1,monster.spd * 2);
							monstr = monster.str *= 2;
							goto HITMONSTER;
							}
						case '2':	/* magic bolt */
							if (stat->mag < 3)
								illspell();
							else
								{
								do
									{
									mvaddstr(6,0,"How much manna for bolt? ");
									getstring(aline,80);
									sscanf(aline,"%lf",&temp);
									}
								while (temp < 0 || temp > stat->man);
								stat->man -= floor(temp);
								inflict = temp*roll(10,sqrt(stat->mag/3.0 + 1.0));
								mvaddstr(6,0,"Magic Bolt fired!\n");
								if (monster.typ == 4)
									inflict = 0.0;
								goto HITMONSTER;
								}
						case '5':	/* increase might */
							{
							if (stat->mag < 15)
								illspell();
							else if (stat->man < 55)
								nomanna();
							else
								{
								stat->man -= 55;
								strength += (1.2*(stat->str+stat->swd)+5-strength)/2;
								mvprintw(6,0,"New strength:  %.0f\n",strength);
								}
							break;
							}
						case '3':	/* force field */
							{
							if (stat->mag < 7)
								illspell();
							else if (stat->man < 20)
								nomanna();
							else
								{
								shield = (stat->mxn + stat->shd)*4.2 + 45;
								stat->man -= 20;
								mvaddstr(6,0,"Force Field up.\n");
								}
							break;
							}
						case '4':	/* transform */
							{
							if (stat->mag < 10)
								illspell();
							else if (stat->man < 35)
								nomanna();
							else
								{
								stat->man -= 35;
								whichm = roll(0,100);
								goto CALL;
								}
							break;
							}
						case '6':	/* invisible */
							{
							if (stat->mag < 20)
								illspell();
							else if (stat->man < 45)
								nomanna();
							else
								{
								stat->man -= 45;
								speed += (1.2*(stat->quk+stat->quks)+5-speed)/2;
								mvprintw(6,0,"New quik :  %.0f\n",speed);
								}
							break;
							}
						case '7':	/* transport */
							{
							if (stat->mag < 25)
								illspell();
							else if (stat->man < 50)
								nomanna();
							else
								{
								stat->man -= 50;
								if (stat->brn + stat->mag < monster.exp/300*rnd())
									{
									mvaddstr(6,0,"Transport backfired!\n");
									stat->x += (250*size*rnd() + 50*size)*sgn(stat->x);
									stat->y += (250*size*rnd() + 50*size)*sgn(stat->y);
									goto LEAVE;
									}
								else
									{
									mvprintw(6,0,"%s is transported.\n",monster.name);
									monster.trs *= (rnd() > 0.3);
									goto FINISH;
									}
								}
							break;
							}
						case '8':	/* paralyze */
							{
							if (stat->mag < 30)
									illspell();
							else if (stat->man < 60)
								nomanna();
							else
								{
								stat->man -= 60;
								if (stat->mag > monster.exp/1000*rnd())
									{
									mvprintw(6,0,"%s is held.\n",monster.name);
									monster.spd = -2;
									}
								else
									mvaddstr(6,0,"Monster unaffected.\n");
								}
							break;
							}
						case '9':	/* specify */
							{
							if (stat->typ < 20)
								illspell();
							else if (stat->man < 1000)
								nomanna();
							else
								{
								mvaddstr(6,0,"Which monster do you want [0-99] ? ");
								whichm = inflt();
								whichm = max(0,min(99,whichm));
								stat->man -= 1000;
								goto CALL;
								}
							break;
							}
						}
				break;
			case '5':
				inflict = 1 + stat->swd;
				stat->exp += floor(monster.exp/10);
				monster.exp *= 0.92;
				maxspd += 2;
				monster.spd = (monster.spd < 0) ? 0 : monster.spd + 2;
				if (monster.typ == 4)
					{
					mvprintw(lines++,0,"You hit %s %.0f times, and made him mad!",monster.name,inflict);
					stat->quk /= 2;
					stat->x += sgn(stat->x)*roll(50*size,250*size);
					stat->y += sgn(stat->y)*roll(50*size,250*size);
					stat->y += (250*size*rnd() + 50*size)*sgn(stat->y);
					goto LEAVE;
					}
				else
					goto HITMONSTER;
			case '6':	/* luckout */
				if (luckout)
					mvaddstr(lines++,0,"You already tried that.");
				else
					if (monster.typ == 23)
						if (rnd() < stat->sin/100)								{
							mvprintw(lines++,0,"%s accepted!",monster.name);
							goto LEAVE;
							}
						else
							{
							luckout = TRUE;
							mvaddstr(lines++,0,"Nope, he's not interested.");
							}
					else
						if ((rnd() + .333)*stat->brn < (rnd() + .333)*monster.brn)
							{
							luckout = TRUE;
							mvprintw(lines++,0,"You blew it, %s.",stat->name);
							}
						else
							{
							mvaddstr(lines++,0,"You made it!");
							goto FINISH;
							}
				break;
			case '\014':	/* clear screen */
				clear();
				break;
			case '7':	/* use ring */
				if (stat->rng.type > 0)
					{
					mvaddstr(lines++,0,"Now using ring.");
						stat->rng.type = -stat->rng.type;
						if (abs(stat->rng.type) != DLREG)
							--stat->rng.duration;
						goto NORMALHIT;
					}
				break;
			}
		goto BOT;
HITMONSTER:		{
			inflict = floor(inflict);
			mvprintw(lines++,0,"You hit %s %.0f times!",monster.name,inflict);
			if ((monster.hit -= inflict) >0)
				switch (monster.typ)
					{
					case 4: /* dark lord */
						inflict = stat->nrg + shield +1;
						goto SPECIALHIT;
					case 16:	/* shrieker */
						mvaddstr(lines++,0,"Shreeeek!!  You scared it, and it called one of its friends.");
						paws(lines);
						whichm = roll(70,30);
						goto CALL;
					}
			else
				{
				if (monster.typ == 23)	/* morgoth */
					mvaddstr(lines++,0,"You have defeated Morgoth, but he may return. . .");
				else
					mvprintw(lines++,0,"You killed it.  Good work, %s.",stat->name);
				goto FINISH;
				}
			}
		}
BOT:	refresh();
	if (lines == 23)
		{
		paws(23);
		move(lines = 6,0);
		clrtobot();
		}
	if (stat->nrg <= 0)
		{
		paws(lines);
		death(stat);
		goto LEAVE;
		}
	goto TOP;
FINISH:	stat->exp += monster.exp;
	if (rnd() < monster.flk/100.0)	/* flock monster */
		{
		paws(lines);
		fghttofin = FALSE;
		goto CALL;
		}
	else if (size > 1 && monster.trs && rnd() > pow(0.6,(double) (hwmany/3 + size/3)))	/* this takes # of flocks and size into account */
		{
		paws(lines);
		treasure(stat,monster.trs,size);
		}
LEAVE:	stat->rng.type = abs(stat->rng.type);
	paws(lines+3);
	move(4,0);
	clrtobot();
	fghting = FALSE;
}
