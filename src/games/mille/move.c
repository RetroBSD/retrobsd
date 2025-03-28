#include "mille.h"
#include <string.h>
#include <unistd.h>
#ifndef	unctrl
#   include "unctrl.h"
#endif

#ifdef	attron
#   include <term.h>
#   define _tty	cur_term->Nttyb
#endif

/*
 * @(#)move.c	1.2 (Berkeley) 3/28/83
 */

#undef	CTRL
#define	CTRL(c)		(c - 'A' + 1)

char	*Movenames[] = {
		"M_DISCARD", "M_DRAW", "M_PLAY", "M_ORDER"
	};

/*
 * return whether or not the player has picked
 */
static int
haspicked(PLAY *pp)
{

	int	card;

	if (Topcard <= Deck)
		return TRUE;
	switch (pp->hand[Card_no]) {
	  case C_GAS_SAFE:	case C_SPARE_SAFE:
	  case C_DRIVE_SAFE:	case C_RIGHT_WAY:
		card = 1;
		break;
	  default:
		card = 0;
		break;
	}
	return (pp->hand[card] != C_INIT);
}

static int
playcard(PLAY *pp)
{
	int	v;
	CARD	card;

	/*
	 * check and see if player has picked
	 */
	switch (pp->hand[Card_no]) {
	  default:
		if (!haspicked(pp))
mustpick:
			return error("must pick first", 0);
	  case C_GAS_SAFE:	case C_SPARE_SAFE:
	  case C_DRIVE_SAFE:	case C_RIGHT_WAY:
		break;
	}

	card = pp->hand[Card_no];
	if (Debug)
		fprintf(outf, "PLAYCARD: Card = %s\n", C_name[card]);
	Next = FALSE;
	switch (card) {
	  case C_200:
		if (pp->nummiles[C_200] == 2)
			return error("only two 200's per hand", 0);
	  case C_100:	case C_75:
		if (pp->speed == C_LIMIT)
			return error("limit of 50", 0);
	  case C_50:
		if (pp->mileage + Value[card] > End)
			return error("puts you over %d", End);
	  case C_25:
		if (!pp->can_go)
			return error("cannot move now", 0);
		pp->nummiles[card]++;
		v = Value[card];
		pp->total += v;
		pp->hand_tot += v;
		if ((pp->mileage += v) == End)
			check_ext(FALSE);
		break;

	  case C_GAS:	case C_SPARE:	case C_REPAIRS:
		if (pp->battle != opposite(card))
			return error("can't play \"%s\"", (int)C_name[card]);
		pp->battle = card;
		if (pp->safety[S_RIGHT_WAY] == S_PLAYED)
			pp->can_go = TRUE;
		break;

	  case C_GO:
		if (pp->battle != C_INIT && pp->battle != C_STOP
		    && !isrepair(pp->battle))
			return error("cannot play \"Go\" on a \"%s\"",
			    (int)C_name[pp->battle]);
		pp->battle = C_GO;
		pp->can_go = TRUE;
		break;

	  case C_END_LIMIT:
		if (pp->speed != C_LIMIT)
			return error("not limited", 0);
		pp->speed = C_END_LIMIT;
		break;

	  case C_EMPTY:	case C_FLAT:	case C_CRASH:
	  case C_STOP:
		pp = &Player[other(Play)];
		if (!pp->can_go)
			return error("opponent cannot go", 0);
		else if (pp->safety[safety(card) - S_CONV] == S_PLAYED)
protected:
			return error("opponent is protected", 0);
		pp->battle = card;
		pp->new_battle = TRUE;
		pp->can_go = FALSE;
		pp = &Player[Play];
		break;

	  case C_LIMIT:
		pp = &Player[other(Play)];
		if (pp->speed == C_LIMIT)
			return error("opponent has limit", 0);
		if (pp->safety[S_RIGHT_WAY] == S_PLAYED)
			goto protected;
		pp->speed = C_LIMIT;
		pp->new_speed = TRUE;
		pp = &Player[Play];
		break;

	  case C_GAS_SAFE:	case C_SPARE_SAFE:
	  case C_DRIVE_SAFE:	case C_RIGHT_WAY:
		if (pp->battle == opposite(card)
		    || (card == C_RIGHT_WAY && pp->speed == C_LIMIT)) {
			if (!(card == C_RIGHT_WAY && !isrepair(pp->battle))) {
				pp->battle = C_GO;
				pp->can_go = TRUE;
			}
			if (card == C_RIGHT_WAY && pp->speed == C_LIMIT)
				pp->speed = C_INIT;
			if (pp->new_battle
			    || (pp->new_speed && card == C_RIGHT_WAY)) {
				pp->coups[card - S_CONV] = TRUE;
				pp->total += SC_COUP;
				pp->hand_tot += SC_COUP;
				pp->coupscore += SC_COUP;
			}
		}
		/*
		 * if not coup, must pick first
		 */
		else if (pp->hand[0] == C_INIT && Topcard > Deck)
			goto mustpick;
		pp->safety[card - S_CONV] = S_PLAYED;
		pp->total += SC_SAFETY;
		pp->hand_tot += SC_SAFETY;
		if ((pp->safescore += SC_SAFETY) == NUM_SAFE * SC_SAFETY) {
			pp->total += SC_ALL_SAFE;
			pp->hand_tot += SC_ALL_SAFE;
		}
		if (card == C_RIGHT_WAY) {
			if (pp->speed == C_LIMIT)
				pp->speed = C_INIT;
			if (pp->battle == C_STOP || pp->battle == C_INIT) {
				pp->can_go = TRUE;
				pp->battle = C_INIT;
			}
			if (!pp->can_go && isrepair(pp->battle))
				pp->can_go = TRUE;
		}
		Next = -1;
		break;

	  case C_INIT:
		error("no card there", 0);
		Next = -1;
		break;
	}
	if (pp == &Player[PLAYER])
		account(card);
	pp->hand[Card_no] = C_INIT;
	Next = (Next == -1 ? FALSE : TRUE);
	return TRUE;
}

/*
 *	Check and see if either side can go.  If they cannot,
 * the game is over
 */
static void
check_go()
{

	CARD	card;
	PLAY	*pp, *op;
	int	i;

	for (pp = Player; pp < &Player[2]; pp++) {
		op = (pp == &Player[COMP] ? &Player[PLAYER] : &Player[COMP]);
		for (i = 0; i < HAND_SZ; i++) {
			card = pp->hand[i];
			if (issafety(card) || canplay(pp, op, card)) {
				if (Debug) {
					fprintf(outf, "CHECK_GO: can play %s (%d), ", C_name[card], card);
					fprintf(outf, "issafety(card) = %d, ", issafety(card));
					fprintf(outf, "canplay(pp, op, card) = %d\n", canplay(pp, op, card));
				}
				return;
			}
			else if (Debug)
				fprintf(outf, "CHECK_GO: cannot play %s\n",
				    C_name[card]);
		}
	}
	Finished = TRUE;
}

void
domove()
{
	PLAY	*pp;
	int	i, j;
	bool	goodplay;

	pp = &Player[Play];
	if (Play == PLAYER)
		getmove();
	else
		calcmove();
	Next = FALSE;
	goodplay = TRUE;
	switch (Movetype) {
	  case M_DISCARD:
		if (haspicked(pp)) {
			if (pp->hand[Card_no] == C_INIT)
				if (Card_no == 6)
					Finished = TRUE;
				else
					error("no card there", 0);
			else {
				if (issafety(pp->hand[Card_no])) {
					error("discard a safety?", 0);
					goodplay = FALSE;
					break;
				}
				Discard = pp->hand[Card_no];
				pp->hand[Card_no] = C_INIT;
				Next = TRUE;
				if (Play == PLAYER)
					account(Discard);
			}
		}
		else
			error("must pick first", 0);
		break;
	  case M_PLAY:
		goodplay = playcard(pp);
		break;
	  case M_DRAW:
		Card_no = 0;
		if (Topcard <= Deck)
			error("no more cards", 0);
		else if (haspicked(pp))
			error("already picked", 0);
		else {
			pp->hand[0] = *--Topcard;
			if (Debug)
				fprintf(outf, "DOMOVE: Draw %s\n", C_name[*Topcard]);
acc:
			if (Play == COMP) {
				account(*Topcard);
				if (issafety(*Topcard))
					pp->safety[*Topcard-S_CONV] = S_IN_HAND;
			}
			if (pp->hand[1] == C_INIT && Topcard > Deck) {
				Card_no = 1;
				pp->hand[1] = *--Topcard;
				if (Debug)
					fprintf(outf, "DOMOVE: Draw %s\n", C_name[*Topcard]);
				goto acc;
			}
			pp->new_battle = FALSE;
			pp->new_speed = FALSE;
		}
		break;

	  case M_ORDER:
		break;
	}
	/*
	 * move blank card to top by one of two methods.  If the
	 * computer's hand was sorted, the randomness for picking
	 * between equally valued cards would be lost
	 */
	if (Order && Movetype != M_DRAW && goodplay && pp == &Player[PLAYER])
		sort(pp->hand);
	else
		for (i = 1; i < HAND_SZ; i++)
			if (pp->hand[i] == C_INIT) {
				for (j = 0; pp->hand[j] == C_INIT; j++)
					if (j >= HAND_SZ) {
						j = 0;
						break;
					}
				pp->hand[i] = pp->hand[j];
				pp->hand[j] = C_INIT;
			}
	if (Topcard <= Deck)
		check_go();
	if (Next)
		nextplay();
}

void
getmove()
{
	char	c, *sp;
#ifdef EXTRAP
	static bool	last_ex = FALSE;	/* set if last command was E */

	if (last_ex) {
		undoex();
		prboard();
		last_ex = FALSE;
	}
#endif
	for (;;) {
		prompt(MOVEPROMPT);
		leaveok(Board, FALSE);
		refresh();
		while ((c = readch()) == killchar() || c == erasechar())
			continue;
		if (islower(c))
			c = toupper(c);
		if (isprint(c) && !isspace(c)) {
			addch(c);
			refresh();
		}
		switch (c) {
		  case 'P':		/* Pick */
			Movetype = M_DRAW;
			goto ret;
		  case 'U':		/* Use Card */
		  case 'D':		/* Discard Card */
			if ((Card_no = getcard()) < 0)
				break;
			Movetype = (c == 'U' ? M_PLAY : M_DISCARD);
			goto ret;
		  case 'O':		/* Order */
			Order = !Order;
			if (Window == W_SMALL) {
				if (!Order)
					mvwaddstr(Score, 12, 21,
						  "o: order hand");
				else
					mvwaddstr(Score, 12, 21,
						  "o: stop ordering");
				wclrtoeol(Score);
			}
			Movetype = M_ORDER;
			goto ret;
		  case 'Q':		/* Quit */
			rub(0);		/* Same as a rubout */
			break;
		  case 'W':		/* Window toggle */
			Window = nextwin(Window);
			newscore();
			prscore(TRUE);
			wrefresh(Score);
			break;
		  case 'R':		/* Redraw screen */
		  case CTRL('L'):
			wrefresh(curscr);
			break;
		  case 'S':		/* Save game */
			On_exit = FALSE;
			save();
			break;
		  case 'E':		/* Extrapolate */
#ifdef EXTRAP
			if (last_ex)
				break;
			Finished = TRUE;
			if (Window != W_FULL)
				newscore();
			prscore(FALSE);
			wrefresh(Score);
			last_ex = TRUE;
			Finished = FALSE;
#else
			error("%c: command not implemented", c);
#endif
			break;
		  case '\r':		/* Ignore RETURNs and	*/
		  case '\n':		/* Line Feeds		*/
		  case ' ':		/* Spaces		*/
		  case '\0':		/* and nulls		*/
			break;
		  case 'Z':		/* Debug code */
			if (geteuid() == ARNOLD) {
				if (!Debug && outf == NULL) {
					char	buf[40];
over:
					prompt(FILEPROMPT);
					leaveok(Board, FALSE);
					refresh();
					sp = buf;
					while ((*sp = readch()) != '\n') {
						if (*sp == killchar())
							goto over;
						else if (*sp == erasechar()) {
							if (--sp < buf)
								sp = buf;
							else {
								addch('\b');
								if (*sp < ' ')
								    addch('\b');
								clrtoeol();
							}
						}
						else
							addstr(unctrl(*sp++));
						refresh();
					}
					*sp = '\0';
					leaveok(Board, TRUE);
					if ((outf = fopen(buf, "w")) == NULL)
						perror(buf);
					setbuf(outf, NULL);
				}
				Debug = !Debug;
				break;
			}
			/* FALLTHROUGH */
		  default:
			error("unknown command: %s", (int)unctrl(c));
			break;
		}
	}
ret:
	leaveok(Board, TRUE);
}

void
account(CARD card)
{

	CARD	oppos;

	if (card == C_INIT)
		return;
	++Numseen[card];
	if (Play == COMP)
		switch (card) {
		  case C_GAS_SAFE:
		  case C_SPARE_SAFE:
		  case C_DRIVE_SAFE:
			oppos = opposite(card);
			Numgos += Numcards[oppos] - Numseen[oppos];
			break;
		  case C_CRASH:
		  case C_FLAT:
		  case C_EMPTY:
		  case C_STOP:
			Numgos++;
			break;
		}
}

void
prompt(int promptno)
{
	static char	*names[] = {
				">>:Move:",
				"Really?",
				"Another hand?",
				"Another game?",
				"Save game?",
				"Same file?",
				"file:",
				"Extension?",
				"Overwrite file?",
			};
	static int	last_prompt = -1;

	if (promptno == last_prompt)
		move(MOVE_Y, MOVE_X + strlen(names[promptno]) + 1);
	else {
		move(MOVE_Y, MOVE_X);
		if (promptno == MOVEPROMPT)
			standout();
		addstr(names[promptno]);
		if (promptno == MOVEPROMPT)
			standend();
		addch(' ');
		last_prompt = promptno;
	}
	clrtoeol();
}

void
sort(CARD *hand)
{
	CARD	*cp, *tp;
	CARD	temp;

	cp = hand;
	hand += HAND_SZ;
	for ( ; cp < &hand[-1]; cp++)
		for (tp = cp + 1; tp < hand; tp++)
			if (*cp > *tp) {
				temp = *cp;
				*cp = *tp;
				*tp = temp;
			}
}
