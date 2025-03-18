#include "extern.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

#define SEGSIZE	8192

typedef	struct stat	STAT;
typedef	struct tm	TIME;

extern char	__data_start[],	/* beginning of data space			*/
		rub();

static char	buf[257];

static bool	new_play;	/* set if move on to new player		*/

/*
 *	This routine executes the given command by index number
 */
void
execute(
reg int	com_num) {

	new_play = FALSE;	/* new_play is true if fixing	*/
	(*func[com_num])();
	notify();
	force_morg();
	if (new_play)
		next_play();
	else if (num_doub)
		printf("%s rolled doubles.  Goes again\n", cur_p->name);
}

/*
 *	This routine moves a piece around.
 */
void
do_move() {

	reg int		r1, r2;
	reg bool	was_jail;

	new_play = was_jail = FALSE;
	printf("roll is %d, %d\n", r1=roll(1, 6), r2=roll(1, 6));
	if (cur_p->loc == JAIL) {
		was_jail++;
		if (!move_jail(r1, r2)) {
			new_play++;
			goto ret;
		}
	}
	else {
		if (r1 == r2 && ++num_doub == 3) {
			printf("That's 3 doubles.  You go to jail\n");
			goto_jail();
			new_play++;
			goto ret;
		}
		move(r1+r2);
	}
	if (r1 != r2 || was_jail)
		new_play++;
ret:
	return;
}

/*
 *	This routine shows the results of a move
 */
static void
show_move() {

	reg SQUARE	*sqp;

	sqp = &board[(int)cur_p->loc];
	printf("That puts you on %s\n", sqp->name);
	switch (sqp->type) {
	  case SAFE:
		printf("That is a safe place\n");
		break;
	  case CC:
	  case CHANCE:
	  case SPEC:
		(*((int (*)())(sqp->desc)))();
		break;
	  case PRPTY:
	  case RR:
	  case UTIL:
		if (sqp->owner < 0) {
			printf("That would cost $%d\n", sqp->cost);
			if (getyn("Do you want to buy? ") == 0) {
				buy(player, sqp);
				cur_p->money -= sqp->cost;
			}
			else if (num_play > 2)
				bid();
		}
		else if (sqp->owner == player)
			printf("You own it.\n");
		else
			rent(sqp);
	}
}

/*
 *	This routine moves a normal move
 */
void
move(
reg int	rl) {

	reg int	old_loc;

	old_loc = cur_p->loc;
	cur_p->loc = (cur_p->loc + rl) % N_SQRS;
	if (cur_p->loc < old_loc && rl > 0) {
		cur_p->money += 200;
		printf("You pass %s and get $200\n", board[0].name);
	}
	show_move();
}

/*
 *	This routine saves the current game for use at a later date
 */
void
save() {

	reg char	*sp;
	reg int		outf, num;
	time_t		tme;
        struct stat     junk;
	unsgn		start, end;

	printf("Which file do you wish to save it in? ");
	sp = buf;
	while ((*sp++=getchar()) != '\n' && !feof(stdin))
		continue;
	*--sp = '\0';
	if (feof(stdin))
		clearerr(stdin);

	/*
	 * check for existing files, and confirm overwrite if needed
	 */
	if (stat(buf, &junk) >= 0
	    && getyn("File exists.  Do you wish to overwrite? ") > 0)
		return;

	if ((outf=creat(buf, 0644)) < 0) {
		perror(buf);
		return;
	}
	printf("\"%s\" ", buf);
	time(&tme);			/* get current time		*/
	strcpy(buf, ctime(&tme));
	for (sp = buf; *sp != '\n'; sp++)
		continue;
	*sp = '\0';
	start = (unsigned) __data_start;
	end = (unsigned) sbrk(0);
	while (start < end) {		/* write out entire data space */
		num = start + 16 * 1024 > end ? end - start : 16 * 1024;
		if (write(outf, (void*)start, num) != num) {
                        perror(buf);
                        exit(-1);
                }
		start += num;
	}
	close(outf);
	printf("[%s]\n", buf);
}

/*
 *	This routine restores an old game from a file
 */
void
restore() {

	reg char	*sp;

	printf("Which file do you wish to restore from? ");
	for (sp = buf; (*sp=getchar()) != '\n' && !feof(stdin); sp++)
		continue;
	*sp = '\0';
	if (feof(stdin))
		clearerr(stdin);
	rest_f(buf);
}

/*
 *	This does the actual restoring.  It returns TRUE if the
 * backup was successful, else false.
 */
int
rest_f(
reg char	*file) {

	reg char	*sp;
	reg int		inf, num;
	char		buf[80];
	unsgn		start, end;
	STAT		sbuf;

	if ((inf=open(file, 0)) < 0) {
		perror(file);
		return FALSE;
	}
	printf("\"%s\" ", file);
	if (fstat(inf, &sbuf) < 0) {		/* get file stats	*/
		perror(file);
		exit(1);
	}
	start = (unsigned) __data_start;
	end = start + sbuf.st_size;
	if (brk((void*) end) < 0) {
                perror("brk");
                exit(-1);
	}
	while (start < end) {		/* write out entire data space */
		num = start + 16 * 1024 > end ? end - start : 16 * 1024;
                if (read(inf, (void*) start, num) != num) {
                        perror(file);
                        exit(-1);
                }
		start += num;
	}
	close(inf);
	strcpy(buf, ctime(&sbuf.st_mtime));
	for (sp = buf; *sp != '\n'; sp++)
		continue;
	*sp = '\0';
	printf("[%s]\n", buf);
	return TRUE;
}
