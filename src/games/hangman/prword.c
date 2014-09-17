#include "hangman.h"

/*
 * prword:
 *	Print out the current state of the word
 */
void
prword()
{
	move(KNOWNY, KNOWNX + sizeof "Word: ");
	addstr(Known);
	clrtoeol();
}
