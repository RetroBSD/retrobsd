/*
 * Re-coding of advent in C: data initialization
 */
#include "hdr.h"
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <sys/wait.h>

const short setbit[16] = {
        1, 2, 4, 010, 020,040, 0100, 0200, 0400,
        01000, 02000, 04000, 010000, 020000, 040000, 0100000,
};

void
linkdata()                              /*  secondary data manipulation */
{       register int i,j;
	/*      array linkages          */
	for (i=1; i<=LOCSIZ; i++)
		if (ltext[i].seekadr!=0 && travel[i] != 0)
			if ((travel[i]->tverb)==1) cond[i]=2;
	for (j=100; j>0; j--)
		if (fixd[j]>0)
		{       drop(j+100,fixd[j]);
			drop(j,plac[j]);
		}
	for (j=100; j>0; j--)
	{       fixed[j]=fixd[j];
		if (plac[j]!=0 && fixd[j]<=0) drop(j,plac[j]);
	}

	maxtrs=79;
	tally=0;
	tally2=0;

	for (i=50; i<=maxtrs; i++)
	{       if (ptext[i].seekadr!=0) prop[i] = -1;
		tally -= prop[i];
	}

	/* define mnemonics */
	keys=vocab("keys",1,0);
	lamp=vocab("lamp",1,0);
	grate=vocab("grate",1,0);
	cage=vocab("cage",1,0);
	rod=vocab("rod",1,0);
	rod2=rod+1;
	steps=vocab("steps",1,0);
	bird=vocab("bird",1,0);
	door=vocab("door",1,0);
	pillow=vocab("pillow",1,0);
	snake=vocab("snake",1,0);
	fissur=vocab("fissu",1,0);
	tablet=vocab("table",1,0);
	clam=vocab("clam",1,0);
	oyster=vocab("oyster",1,0);
	magzin=vocab("magaz",1,0);
	dwarf=vocab("dwarf",1,0);
	knife=vocab("knife",1,0);
	food=vocab("food",1,0);
	bottle=vocab("bottl",1,0);
	water=vocab("water",1,0);
	oil=vocab("oil",1,0);
	plant=vocab("plant",1,0);
	plant2=plant+1;
	axe=vocab("axe",1,0);
	mirror=vocab("mirro",1,0);
	dragon=vocab("drago",1,0);
	chasm=vocab("chasm",1,0);
	troll=vocab("troll",1,0);
	troll2=troll+1;
	bear=vocab("bear",1,0);
	messag=vocab("messa",1,0);
	vend=vocab("vendi",1,0);
	batter=vocab("batte",1,0);

	nugget=vocab("gold",1,0);
	coins=vocab("coins",1,0);
	chest=vocab("chest",1,0);
	eggs=vocab("eggs",1,0);
	tridnt=vocab("tride",1,0);
	vase=vocab("vase",1,0);
	emrald=vocab("emera",1,0);
	pyram=vocab("pyram",1,0);
	pearl=vocab("pearl",1,0);
	rug=vocab("rug",1,0);
	chain=vocab("chain",1,0);

	back=vocab("back",0,0);
	look=vocab("look",0,0);
	cave=vocab("cave",0,0);
	null=vocab("null",0,0);
	entrnc=vocab("entra",0,0);
	dprssn=vocab("depre",0,0);

	say=vocab("say",2,0);
	lock=vocab("lock",2,0);
	throw=vocab("throw",2,0);
	find=vocab("find",2,0);
	invent=vocab("inven",2,0);
	/* initialize dwarves */
	chloc=114;
	chloc2=140;
	for (i=1; i<=6; i++)
		dseen[i]=FALSE;
	dflag=0;
	dloc[1]=19;
	dloc[2]=27;
	dloc[3]=33;
	dloc[4]=44;
	dloc[5]=64;
	dloc[6]=chloc;
	daltlc=18;

	/* random flags & ctrs */
	turns=0;
	lmwarn=FALSE;
	iwest=0;
	knfloc=0;
	detail=0;
	abbnum=5;
	for (i=0; i<=4; i++)
		if (rtext[2*i+81].seekadr!=0) maxdie=i+1;
	numdie=holdng=dkill=foobar=bonus=0;
	clock1=30;
	clock2=50;
	saved=0;
	closng=panic=closed=scorng=FALSE;
}

void
trapdel(sig)                            /* come here if he hits a del   */
{	delhit++;			/* main checks, treats as QUIT  */
	signal(2,trapdel);		/* catch subsequent DELs        */
}

void
startup()
{
        time_t now;

	time(&now);
	srand(now);                     /* random odd seed              */
/*      srand(371);             */      /* non-random seed              */
	hinted[3] = yes(65, 1, 0);
	newloc = 1;
	limit = 330;
	if (hinted[3]) limit = 1000;    /* better batteries if instrucs */
}
