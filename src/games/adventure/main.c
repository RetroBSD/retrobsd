/*
 * Re-coding of advent in C: main program
 */
#include "hdr.h"
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

int datfd;
int delhit;
char *wd1, *wd2;
struct game game;
struct travlist *tkk;

int
main(
int argc,
char **argv)
{       register int i;
	int rval;
	struct text *kk;
	char *savfile = 0;

	if (argc > 1)
                savfile = argv[1];

        if (savfile != 0 && strcmp(savfile, DATFILE) == 0) {
                /* read data from orig. file and write to datfile */
                rdata(DATFILE, "adventure.dat");
                linkdata();
                poof();
                save("adventure.dat", DATSIZE);
                exit(0);
	}
        /* try local file first */
        datfd = open("adventure.dat", O_RDONLY);
	if (datfd < 0)
                datfd = open("/games/lib/adventure.dat", O_RDONLY);
	if (datfd < 0) {
                printf("No adventure just now\n");
                exit(0);
        }
	setuid(getuid());
        signal(SIGINT, trapdel);

	if (savfile != 0) {
                if (restore(savfile)) {
                        start(0);       /* restarting game : 8305       */
                        k = null;
                        goto l8;
                }
                printf("Your forged file dissappears in a puff of greasy black smoke! (poof)\n");
                unlink(savfile);
                exit(0);
        }
	if (! restdat(datfd, DATSIZE)) {
                printf("Corrupted dat file\n");
                exit(0);
	}
        start(0);
        startup();			/* prepare for a user           */
        blklin = TRUE;

	for (;;)                        /* main command loop (label 2)  */
	{       if (newloc<9 && newloc!=0 && closng)
		{       rspeak(130);    /* if closing leave only by     */
			newloc=loc;     /*      main office             */
			if (!panic) clock2=15;
			panic=TRUE;
		}

		rval=fdwarf();          /* dwarf stuff                  */
		if (rval==99) die(99);

	l2000:  if (loc==0) die(99);    /* label 2000                   */
		kk = &stext[loc];
		if ((abb[loc]%abbnum)==0 || kk->seekadr==0)
			kk = &ltext[loc];
		if (!forced(loc) && dark(0))
		{       if (wzdark && pct(35))
			{       die(90);
				goto l2000;
			}
			kk = &rtext[16];
		}
	        if (toting(bear)) rspeak(141);  /* 2001                 */
		speak(kk);
		k=1;
		if (forced(loc))
			goto l8;
		if (loc==33 && pct(25)&&!closng) rspeak(8);
		if (!dark(0))
		{       abb[loc]++;
			for (i=atloc[loc]; i!=0; i=plink[i])    /*2004  */
			{       obj=i;
				if (obj>100) obj -= 100;
				if (obj==steps && toting(nugget)) continue;
				if (prop[obj]<0)
				{       if (closed) continue;
					prop[obj]=0;
					if (obj==rug||obj==chain)
						prop[obj]=1;
					tally--;
					if (tally==tally2 && tally != 0)
						if (limit>35) limit=35;
				}
				int pk = prop[obj];             /* 2006	*/
				if (obj==steps && loc==fixed[steps])
                                        pk = 1;
				pspeak(obj, pk);
			}                                       /* 2008 */
			goto l2012;
	l2009:          k=54;                   /* 2009                 */
	l2010:          spk=k;
	l2011:          rspeak(spk);
		}
	l2012:  verb=0;                         /* 2012                 */
		obj=0;
	l2600:	checkhints();                   /* to 2600-2602         */
		if (closed)
		{       if (prop[oyster]<0 && toting(oyster))
				pspeak(oyster,1);
			for (i=1; i<100; i++)
				if (toting(i)&&prop[i]<0)       /*2604  */
					prop[i] = -1-prop[i];
		}
		wzdark=dark(0);                 /* 2605                 */
		if (knfloc>0 && knfloc!=loc) knfloc=1;
		getin(&wd1,&wd2);
		if (delhit)                     /* user typed a DEL     */
		{       delhit=0;               /* reset counter        */
			copystr("quit",wd1);    /* pretend he's quitting*/
			*wd2=0;
		}
	l2608:  if ((foobar = -foobar)>0) foobar=0;     /* 2608         */
		/* should check here for "magic mode"                   */
		turns++;

		if (verb==say && *wd2!=0) verb=0;
		if (verb==say)
			goto l4090;
		if (tally==0 && loc>=15 && loc!=33) clock1--;
		if (clock1==0)
		{       closing();                      /* to 10000     */
			goto l19999;
		}
		if (clock1<0) clock2--;
		if (clock2==0)
		{       caveclose();            /* to 11000             */
			continue;               /* back to 2            */
		}
		if (prop[lamp]==1) limit--;
		if (limit<=30 && here(batter) && prop[batter]==0
			&& here(lamp))
		{       rspeak(188);            /* 12000                */
			prop[batter]=1;
			if (toting(batter)) drop(batter,loc);
			limit=limit+2500;
			lmwarn=FALSE;
			goto l19999;
		}
		if (limit==0)
		{       limit = -1;             /* 12400                */
			prop[lamp]=0;
			rspeak(184);
			goto l19999;
		}
		if (limit<0&&loc<=8)
		{       rspeak(185);            /* 12600                */
			gaveup=TRUE;
			done(2);                /* to 20000             */
		}
		if (limit<=30)
		{       if (lmwarn|| !here(lamp)) goto l19999;  /*12200*/
			lmwarn=TRUE;
			spk=187;
			if (place[batter]==0) spk=183;
			if (prop[batter]==1) spk=189;
			rspeak(spk);
		}
	l19999: k=43;
		if (liqloc(loc)==water) k=70;
		if (weq(wd1,"enter") &&
		    (weq(wd2,"strea")||weq(wd2,"water")))
			goto l2010;
		if (weq(wd1,"enter") && *wd2!=0) goto l2800;
		if ((!weq(wd1,"water")&&!weq(wd1,"oil"))
		    || (!weq(wd2,"plant")&&!weq(wd2,"door")))
			goto l2610;
		if (at(vocab(wd2,1,0))) copystr("pour",wd2);
	l2610:  if (weq(wd1,"west"))
			if (++iwest==10) rspeak(17);
	l2630:  i=vocab(wd1,-1,0);
		if (i== -1)
		{       spk=60;                 /* 3000         */
			if (pct(20)) spk=61;
			if (pct(20)) spk=13;
			rspeak(spk);
			goto l2600;
		}
		k=i%1000;
		kq=i/1000+1;
		switch(kq)
		{   case 1: goto l8;
		    case 2: goto l5000;
		    case 3: goto l4000;
		    case 4: goto l2010;
		    default:
			printf("Error 22\n");
			exit(0);
		}

	l8:
		switch(march())
		{   case 2: continue;           /* i.e. goto l2         */
		    case 99:
			switch(die(99))
			{   case 2000: goto l2000;
			    default: bug(111);
			}
		    default: bug(110);
		}

	l2800:  copystr(wd2,wd1);
		*wd2=0;
		goto l2610;

	l4000:  verb=k;
		spk=actspk[verb];
		if (*wd2!=0 && verb!=say) goto l2800;
		if (verb==say) obj= *wd2;
		if (obj!=0) goto l4090;

		switch(verb)
		{   case 1:                     /* take = 8010          */
			if (atloc[loc]==0||plink[atloc[loc]]!=0) goto l8000;
			for (i=1; i<=5; i++)
				if (dloc[i]==loc&&dflag>=2) goto l8000;
			obj=atloc[loc];
			goto l9010;
		    case 2: case 3: case 9:     /* 8000 : drop,say,wave */
		    case 10: case 16: case 17:  /* calm,rub,toss        */
		    case 19: case 21: case 28:  /* find,feed,break      */
		    case 29:                    /* wake                 */
		l8000:  printf("%s what?\n",wd1);
			obj=0;
			goto l2600;
		    case 4: case 6:             /* 8040 open,lock       */
			spk=28;
			if (here(clam)) obj=clam;
			if (here(oyster)) obj=oyster;
			if (at(door)) obj=door;
			if (at(grate)) obj=grate;
			if (obj!=0 && here(chain)) goto l8000;
			if (here(chain)) obj=chain;
			if (obj==0) goto l2011;
			goto l9040;
		    case 5: goto l2009;         /* nothing              */
		    case 7: goto l9070;         /* on                   */
		    case 8: goto l9080;         /* off                  */
		    case 11: goto l8000;        /* walk                 */
		    case 12: goto l9120;        /* kill                 */
		    case 13: goto l9130;        /* pour                 */
		    case 14:                    /* eat: 8140            */
			if (!here(food)) goto l8000;
		l8142:  dstroy(food);
			spk=72;
			goto l2011;
		    case 15: goto l9150;        /* drink                */
		    case 18:                    /* quit: 8180           */
			gaveup=yes(22,54,54);
			if (gaveup) done(2);    /* 8185                 */
			goto l2012;
		    case 20:                    /* invent=8200          */
			spk=98;
			for (i=1; i<=100; i++)
			{       if (i!=bear && toting(i))
				{       if (spk==98) rspeak(99);
					blklin=FALSE;
					pspeak(i,-1);
					blklin=TRUE;
					spk=0;
				}
			}
			if (toting(bear)) spk=141;
			goto l2011;
		    case 22: goto l9220;        /* fill                 */
		    case 23: goto l9230;        /* blast                */
		    case 24:                    /* score: 8240          */
			scorng=TRUE;
			printf("If you were to quit now, you would score");
			printf(" %d out of a possible ",score());
			printf("%d.",mxscor);
			scorng=FALSE;
			gaveup=yes(143,54,54);
			if (gaveup) done(2);
			goto l2012;
		    case 25:                    /* foo: 8250            */
			k=vocab(wd1,3,0);
			spk=42;
			if (foobar==1-k) goto l8252;
			if (foobar!=0) spk=151;
			goto l2011;
		l8252:  foobar=k;
			if (k!=4) goto l2009;
			foobar=0;
			if (place[eggs]==plac[eggs]
				||(toting(eggs)&&loc==plac[eggs])) goto l2011;
			if (place[eggs]==0&&place[troll]==0&&prop[troll]==0)
				prop[troll]=1;
			k=2;
			if (here(eggs)) k=1;
			if (loc==plac[eggs]) k=0;
			move(eggs,plac[eggs]);
			pspeak(eggs,k);
			goto l2012;
		    case 26:                    /* brief=8260           */
			spk=156;
			abbnum=10000;
			detail=3;
			goto l2011;
		    case 27:                    /* read=8270            */
			if (here(magzin)) obj=magzin;
			if (here(tablet)) obj=obj*100+tablet;
			if (here(messag)) obj=obj*100+messag;
			if (closed&&toting(oyster)) obj=oyster;
			if (obj>100||obj==0||dark(0)) goto l8000;
			goto l9270;
		    case 30:                    /* suspend=8300         */
			spk=201;
			printf("I can suspend your adventure for you so");
			printf(" you can resume later, but\n");
			printf("you will have to wait at least");
			printf(" %d minutes before continuing.",latncy);
			if (!yes(200,54,54)) goto l2012;
			datime(&saved, &savet);
                        ciao();
			continue;
		    case 31:                    /* hours=8310           */
			printf("Colossal cave is closed 9am-5pm Mon ");
			printf("through Fri except holidays.\n");
			goto l2012;
		    default: bug(23);
		}

	l4090:
		switch(verb)
		{   case 1:                     /* take = 9010          */
	l9010:          switch(trtake())
			{   case 2011: goto l2011;
			    case 9220: goto l9220;
			    case 2009: goto l2009;
			    case 2012: goto l2012;
			    default: bug(102);
			}
	l9020:      case 2:                     /* drop = 9020          */
			switch(trdrop())
			{   case 2011: goto l2011;
			    case 19000: done(3);
			    case 2012: goto l2012;
			    default: bug(105);
			}
	            case 3:
			switch(trsay())
			{   case 2012: goto l2012;
			    case 2630: goto l2630;
			    default: bug(107);
			}
	l9040:      case 4:  case 6:            /* open, close          */
			switch(tropen())
			{   case 2011: goto l2011;
			    case 2010: goto l2010;
			    default: bug(106);
			}
		    case 5: goto l2009;         /* nothing              */
		    case 7:                     /* on   9070            */
	l9070:          if (!here(lamp))  goto l2011;
			spk=184;
			if (limit<0) goto l2011;
			prop[lamp]=1;
			rspeak(39);
			if (wzdark) goto l2000;
			goto l2012;

		    case 8:                     /* off                  */
	l9080:          if (!here(lamp)) goto l2011;
			prop[lamp]=0;
			rspeak(40);
			if (dark(0)) rspeak(16);
			goto l2012;

		    case 9:                     /* wave                 */
			if ((!toting(obj))&&(obj!=rod||!toting(rod2)))
				spk=29;
			if (obj!=rod||!at(fissur)||!toting(obj)||closng)
				goto l2011;
			prop[fissur]=1-prop[fissur];
			pspeak(fissur,2-prop[fissur]);
			goto l2012;
		    case 10: case 11: case 18:  /* calm, walk, quit     */
		    case 24: case 25: case 26:  /* score, foo, brief    */
		    case 30: case 31:           /* suspend, hours       */
			     goto l2011;
	l9120:      case 12:                    /* kill                 */
			switch(trkill())
			{   case 8000: goto l8000;
			    case 8: goto l8;
			    case 2011: goto l2011;
			    case 2608: goto l2608;
			    case 19000: done(3);
			    default: bug(112);
			}
	l9130:      case 13:                    /* pour                 */
			if (obj==bottle||obj==0) obj=liq(0);
			if (obj==0) goto l8000;
			if (!toting(obj)) goto l2011;
			spk=78;
			if (obj!=oil&&obj!=water) goto l2011;
			prop[bottle]=1;
			place[obj]=0;
			spk=77;
			if (!(at(plant)||at(door))) goto l2011;
			if (at(door))
			{       prop[door]=0;   /* 9132                 */
				if (obj==oil) prop[door]=1;
				spk=113+prop[door];
				goto l2011;
			}
			spk=112;
			if (obj!=water) goto l2011;
			pspeak(plant,prop[plant]+1);
			prop[plant]=(prop[plant]+2)% 6;
			prop[plant2]=prop[plant]/2;
			k=null;
			goto l8;
		    case 14:                    /* 9140 - eat           */
			if (obj==food) goto l8142;
			if (obj==bird||obj==snake||obj==clam||obj==oyster
			    ||obj==dwarf||obj==dragon||obj==troll
			    ||obj==bear) spk=71;
			goto l2011;
	l9150:      case 15:                    /* 9150 - drink         */
			if (obj==0&&liqloc(loc)!=water&&(liq(0)!=water
				||!here(bottle))) goto l8000;
			if (obj!=0&&obj!=water) spk=110;
			if (spk==110||liq(0)!=water||!here(bottle))
				goto l2011;
			prop[bottle]=1;
			place[water]=0;
			spk=74;
			goto l2011;
		    case 16:                    /* 9160: rub            */
			if (obj!=lamp) spk=76;
			goto l2011;
		    case 17:                    /* 9170: throw          */
			switch(trtoss())
			{   case 2011: goto l2011;
			    case 9020: goto l9020;
			    case 9120: goto l9120;
			    case 8: goto l8;
			    case 9210: goto l9210;
			    default: bug(113);
			}
		    case 19: case 20:           /* 9190: find, invent   */
			if (at(obj)||(liq(0)==obj&&at(bottle))
				||k==liqloc(loc)) spk=94;
			for (i=1; i<=5; i++)
				if (dloc[i]==loc&&dflag>=2&&obj==dwarf)
					spk=94;
			if (closed) spk=138;
			if (toting(obj)) spk=24;
			goto l2011;
	l9210:      case 21:                    /* feed                 */
			switch(trfeed())
			{   case 2011: goto l2011;
			    default: bug(114);
			}
	l9220:      case 22:                    /* fill                 */
			switch(trfill())
			{   case 2011: goto l2011;
			    case 8000: goto l8000;
			    case 9020: goto l9020;
			    default: bug(115);
			}
	l9230:      case 23:                    /* blast                */
			if (prop[rod2]<0||!closed) goto l2011;
			bonus=133;
			if (loc==115) bonus=134;
			if (here(rod2)) bonus=135;
			rspeak(bonus);
			done(2);
	l9270:      case 27:                    /* read                 */
			if (dark(0)) goto l5190;
			if (obj==magzin) spk=190;
			if (obj==tablet) spk=196;
			if (obj==messag) spk=191;
			if (obj==oyster&&hinted[2]&&toting(oyster)) spk=194;
			if (obj!=oyster||hinted[2]||!toting(oyster)
				||!closed) goto l2011;
			hinted[2]=yes(192,193,54);
			goto l2012;
	            case 28:                    /* break                */
			if (obj==mirror) spk=148;
			if (obj==vase&&prop[vase]==0)
			{       spk=198;
				if (toting(vase)) drop(vase,loc);
				prop[vase]=2;
				fixed[vase]= -1;
				goto l2011;
			}
			if (obj!=mirror||!closed) goto l2011;
			rspeak(197);
			done(3);

	            case 29:                    /* wake                 */
			if (obj!=dwarf||!closed) goto l2011;
			rspeak(199);
			done(3);

		    default: bug(24);
		}

	l5000:
		obj=k;
		if (fixed[k]!=loc && !here(k)) goto l5100;
	l5010:  if (*wd2!=0) goto l2800;
		if (verb!=0) goto l4090;
		printf("What do you want to do with the %s?\n",wd1);
		goto l2600;
	l5100:  if (k!=grate) goto l5110;
		if (loc==1||loc==4||loc==7) k=dprssn;
		if (loc>9&&loc<15) k=entrnc;
		if (k!=grate) goto l8;
	l5110:  if (k!=dwarf) goto l5120;
		for (i=1; i<=5; i++)
			if (dloc[i]==loc&&dflag>=2) goto l5010;
	l5120:  if ((liq(0)==k&&here(bottle))||k==liqloc(loc)) goto l5010;
		if (obj!=plant||!at(plant2)||prop[plant2]==0) goto l5130;
		obj=plant2;
		goto l5010;
	l5130:  if (obj!=knife||knfloc!=loc) goto l5140;
		knfloc = -1;
		spk=116;
		goto l2011;
	l5140:  if (obj!=rod||!here(rod2)) goto l5190;
		obj=rod2;
		goto l5010;
	l5190:  if ((verb==find||verb==invent)&&*wd2==0) goto l5010;
		printf("I see no %s here\n",wd1);
		goto l2012;
	}
}
