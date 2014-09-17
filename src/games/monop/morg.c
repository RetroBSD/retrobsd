#include "extern.h"

/*
 *	These routines deal with mortgaging.
 */

static char	*names[MAX_PRP+2],
		*morg_coms[]	= {
			"quit",		/*  0 */
			"print",	/*  1 */
			"where",	/*  2 */
			"own holdings",	/*  3 */
			"holdings",	/*  4 */
			"shell",	/*  5 */
			"mortgage",	/*  6 */
			"unmortgage",	/*  7 */
			"buy",		/*  8 */
			"sell",		/*  9 */
			"card",		/* 10 */
			"pay",		/* 11 */
			"trade",	/* 12 */
			"resign",	/* 13 */
			"save game",	/* 14 */
			"restore game",	/* 15 */
			0
		};

static shrt	square[MAX_PRP+2];

static int	num_good,got_houses;

/*
 *	This routine sets up the list of mortgageable property
 */
static int
set_mlist() {

	reg OWN	*op;

	num_good = 0;
	for (op = cur_p->own_list; op; op = op->next)
		if (! ((PROP*)op->sqr->desc)->morg) {
			if (op->sqr->type == PRPTY && ((PROP*)op->sqr->desc)->houses)
				got_houses++;
			else {
				names[num_good] = op->sqr->name;
				square[num_good++] = sqnum(op->sqr);
			}
                }
	names[num_good++] = "done";
	names[num_good--] = 0;
	return num_good;
}

/*
 *	This routine actually mortgages the property.
 */
static void
m(prop)
reg int	prop; {

	reg int	price;

	price = board[prop].cost/2;
	((PROP*)board[prop].desc)->morg = TRUE;
	printf("That got you $%d\n",price);
	cur_p->money += price;
}

/*
 *	This routine is the command level response the mortgage command.
 * it gets the list of mortgageable property and asks which are to
 * be mortgaged.
 */
void
mortgage() {

	reg int	prop;

	for (;;) {
		if (set_mlist() == 0) {
			if (got_houses)
				printf("You can't mortgage property with houses on it.\n");
			else
				printf("You don't have any un-mortgaged property.\n");
			return;
		}
		if (num_good == 1) {
			printf("Your only mortageable property is %s\n",names[0]);
			if (getyn("Do you want to mortgage it? ") == 0)
				m(square[0]);
			return;
		}
		prop = getinp("Which property do you want to mortgage? ",names);
		if (prop == num_good)
			return;
		m(square[prop]);
		notify();
	}
}

/*
 *	This routine sets up the list of mortgaged property
 */
static int
set_umlist() {

	reg OWN	*op;

	num_good = 0;
	for (op = cur_p->own_list; op; op = op->next)
		if (((PROP*)op->sqr->desc)->morg) {
			names[num_good] = op->sqr->name;
			square[num_good++] = sqnum(op->sqr);
		}
	names[num_good++] = "done";
	names[num_good--] = 0;
	return num_good;
}

/*
 *	This routine actually unmortgages the property
 */
static void
unm(prop)
reg int	prop; {

	reg int	price;

	price = board[prop].cost/2;
	((PROP*)board[prop].desc)->morg = FALSE;
	price += price/10;
	printf("That cost you $%d\n",price);
	cur_p->money -= price;
	set_umlist();
}

/*
 *	This routine is the command level repsponse to the unmortgage
 * command.  It gets the list of mortgaged property and asks which are
 * to be unmortgaged.
 */
void
unmortgage() {

	reg int	prop;

	for (;;) {
		if (set_umlist() == 0) {
			printf("You don't have any mortgaged property.\n");
			return;
		}
		if (num_good == 1) {
			printf("Your only mortaged property is %s\n",names[0]);
			if (getyn("Do you want to unmortgage it? ") == 0)
				unm(square[0]);
			return;
		}
		prop = getinp("Which property do you want to unmortgage? ",names);
		if (prop == num_good)
			return;
		unm(square[prop]);
	}
}

/*
 *	This routine is a special execute for the force_morg routine
 */
static void
fix_ex(com_num)
reg int	com_num; {

	told_em = FALSE;
	(*func[com_num])();
	notify();
}

/*
 *	This routine forces the indebted player to fix his
 * financial woes.
 */
void
force_morg() {

	told_em = fixing = TRUE;
	while (cur_p->money <= 0)
		fix_ex(getinp("How are you going to fix it up? ",morg_coms));
	fixing = FALSE;
}
