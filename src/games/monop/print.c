#include "extern.h"

static char	*header	= "Name      Own      Price Mg # Rent";

/*
 *	This routine prints out the current board
 */
void
printboard() {

	reg int	i;

	printf("%s\t%s\n", header, header);
	for (i = 0; i < N_SQRS/2; i++) {
		printsq(i, FALSE);
		putchar('\t');
		printsq(i+N_SQRS/2, TRUE);
	}
}

/*
 *	This routine lists where each player is.
 */
void
where() {

	reg int	i;

	printf("%s Player\n", header);
	for (i = 0; i < num_play; i++) {
		printsq(play[i].loc, FALSE);
		printf(" %s (%d)", play[i].name, i+1);
		if (cur_p == &play[i])
			printf(" *");
		putchar('\n');
	}
}

/*
 *	This routine prints out the mortgage flag.
 */
static void
printmorg(SQUARE *sqp)
{

	if (((PROP*)sqp->desc)->morg)
		printf(" * ");
	else
		printf("   ");
}

/*
 *	This routine prints out an individual square
 */
void
printsq(int sqn, bool eoln)
{
	reg int		rnt;
	reg PROP	*pp;
	reg SQUARE	*sqp;

	sqp = &board[sqn];
	printf("%-10.10s", sqp->name);
	if (sqn == JAIL)
		goto spec;
	switch (sqp->type) {
	  case SAFE:
	  case CC:
	  case CHANCE:
	  case SPEC:
spec:
		if (!eoln)
			printf("                        ");
		break;
	  case PRPTY:
		pp = (PROP*) sqp->desc;
		if (sqp->owner < 0) {
			printf(" - %-8.8s %3d", pp->mon_desc->name, sqp->cost);
			if (!eoln)
				printf("         ");
			break;
		}
		printf(" %d %-8.8s %3d", sqp->owner+1, pp->mon_desc->name,
			sqp->cost);
		printmorg(sqp);
		if (pp->monop) {
			if (pp->houses < 5)
				if (pp->houses > 0)
					printf("%d %4d", pp->houses,
						pp->rent[(int)pp->houses]);
				else
					printf("0 %4d", pp->rent[0] * 2);
			else
				printf("H %4d", pp->rent[5]);
		}
		else
			printf("  %4d", pp->rent[0]);
		break;
	  case UTIL:
		if (sqp->owner < 0) {
			printf(" -          150");
			if (!eoln)
				printf("         ");
			break;
		}
		printf(" %d          150", sqp->owner+1);
		printmorg(sqp);
		printf("%d", play[(int)sqp->owner].num_util);
		if (!eoln)
			printf("    ");
		break;
	  case RR:
		if (sqp->owner < 0) {
			printf(" - Railroad 200");
			if (!eoln)
				printf("         ");
			break;
		}
		printf(" %d Railroad 200", sqp->owner+1);
		printmorg(sqp);
		rnt = 25;
		rnt <<= play[(int)sqp->owner].num_rr - 1;
		printf("%d %4d", play[(int)sqp->owner].num_rr, rnt);
		break;
	}
	if (eoln)
		putchar('\n');
}

/*
 *	This routine lists the holdings of the player given
 */
void
printhold(int pl)
{

	reg OWN		*op;
	reg PLAY	*pp;

	pp = &play[pl];
	printf("%s's (%d) holdings (Total worth: $%d):\n", name_list[pl], pl+1,
		pp->money + prop_worth(pp));
	printf("\t$%d", pp->money);
	if (pp->num_gojf) {
		printf(", %d get-out-of-jail-free card", pp->num_gojf);
		if (pp->num_gojf > 1)
			putchar('s');
	}
	putchar('\n');
	if (pp->own_list) {
		printf("\t%s\n", header);
		for (op = pp->own_list; op; op = op->next) {
			putchar('\t');
			printsq(sqnum(op->sqr), TRUE);
		}
	}
}
