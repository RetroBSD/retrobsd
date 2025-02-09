#include "extern.h"
#include <stdlib.h>

/*
 *	This routine deals with buying property, setting all the
 * appropriate flags.
 */
void
buy(int player, SQUARE *sqrp)
{

	trading = FALSE;
	sqrp->owner = player;
	add_list(player, &(play[player].own_list), cur_p->loc);
}

/*
 *	This routine calculates the value for sorting of the
 * given square.
 */
static int
value(SQUARE *sqp)
{

	reg int	sqr;

	sqr = sqnum(sqp);
	switch (sqp->type) {
	  case SAFE:
		return 0;
	  case SPEC:
		return 1;
	  case UTIL:
		if (sqr == 12)
			return 2;
		else
			return 3;
	  case RR:
		return 4 + sqr/10;
	  case PRPTY:
		return 8 + (PROP *)(sqp->desc) - prop;
	}
        return 0;
}

/*
 *	This routine adds an item to the list.
 */
void
add_list(int plr, OWN **head, int op_sqr)
{

	reg int	val;
	reg OWN	*tp, *last_tp;
	OWN	*op;

	op = calloc(1, sizeof (OWN));
	op->sqr = &board[op_sqr];
	val = value(op->sqr);
	last_tp = NULL;
	for (tp = *head; tp && value(tp->sqr) < val; tp = tp->next)
		if (val == value(tp->sqr)) {
			free(op);
			return;
		}
		else
			last_tp = tp;
	op->next = tp;
	if (last_tp != NULL)
		last_tp->next = op;
	else
		*head = op;
	if (!trading)
		set_ownlist(plr);
}

/*
 *	This routine deletes property from the list.
 */
void
del_list(int plr, OWN **head, shrt op_sqr)
{

	reg OWN	*op, *last_op;

	switch (board[(int)op_sqr].type) {
	  case PRPTY:
		((PROP*)board[(int)op_sqr].desc)->mon_desc->num_own--;
		break;
	  case RR:
		play[plr].num_rr--;
		break;
	  case UTIL:
		play[plr].num_util--;
		break;
	}
	last_op = NULL;
	for (op = *head; op; op = op->next)
		if (op->sqr == &board[(int)op_sqr])
			break;
		else
			last_op = op;
	if (last_op == NULL)
		*head = op->next;
	else {
		last_op->next = op->next;
		free(op);
	}
}

/*
 *	This routine accepts bids for the current peice
 * of property.
 */
void
bid() {
	static bool	in[MAX_PL];
	reg int		i, num_in, cur_max;
	char		buf[80];
	int		cur_bid;

	printf("\nSo it goes up for auction.  Type your bid after your name\n");
	for (i = 0; i < num_play; i++)
		in[i] = TRUE;
	i = -1;
	cur_max = 0;
	num_in = num_play;
	while (num_in > 1 || (cur_max == 0 && num_in > 0)) {
	        i++;
		i %= num_play;
		if (in[i]) {
			do {
				sprintf(buf, "%s: ", name_list[i]);
				cur_bid = get_int(buf);
				if (cur_bid == 0) {
					in[i] = FALSE;
					if (--num_in == 0)
						break;
				}
				else if (cur_bid <= cur_max) {
					printf("You must bid higher than %d to stay in\n", cur_max);
					printf("(bid of 0 drops you out)\n");
				}
			} while (cur_bid != 0 && cur_bid <= cur_max);
			cur_max = (cur_bid ? cur_bid : cur_max);
		}
	}
	if (cur_max != 0) {
		while (!in[i]) {
		        i++;
			i %= num_play;
                }
		printf("It goes to %s (%d) for $%d\n",play[i].name,i+1,cur_max);
		buy(i, &board[(int)cur_p->loc]);
		play[i].money -= cur_max;
	}
	else
		printf("Nobody seems to want it, so we'll leave it for later\n");
}

/*
 *	This routine calculates the value of the property
 * of given player.
 */
int
prop_worth(PLAY *plp)
{

	reg OWN	*op;
	reg int	worth;

	worth = 0;
	for (op = plp->own_list; op; op = op->next) {
		if (op->sqr->type == PRPTY && ((PROP*)op->sqr->desc)->monop)
			worth += ((PROP*)op->sqr->desc)->mon_desc->h_cost * 50 *
			    ((PROP*)op->sqr->desc)->houses;
		worth += op->sqr->cost;
	}
	return worth;
}
