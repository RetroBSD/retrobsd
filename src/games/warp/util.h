/* $Header: util.h,v 7.0 86/10/08 15:14:37 lwall Exp $ */

/* $Log:	util.h,v $
 * Revision 7.0  86/10/08  15:14:37  lwall
 * Split into separate files.  Added amoebas and pirates.
 *
 */

#define RANDRAND 268435456.0 /* that's 2**28 */
#define HALFRAND 0x4000 /* that's 2**14 */
int rand();
#define myrand() (rand()&32767)
#define rand_mod(m) ((int)((double)myrand() / 32768.0 * ((double)(m))))
/* pick number in 0..m-1 */

#define roundsleep(x) sleep(x)

void movc3();
void no_can_do();
int exdis();

EXT bool waiting INIT(FALSE);		/* are we waiting for subprocess (in doshell)? */

void util_init();
char *safemalloc();
char *safecpy();
char *cpytill();
char *instr();
#ifdef SETUIDGID
    int eaccess();
#endif
char *savestr();
char *getval();
