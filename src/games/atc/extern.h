/*
 * Copyright (c) 1987 by Ed James, UC Berkeley.  All rights reserved.
 *
 * Copy permission is hereby granted provided that this notice is
 * retained on all partial or complete copies.
 *
 * For more info on this and all of my stuff, mail edjames@berkeley.edu.
 */

extern char		GAMES[], LOG[], *file;

extern int		clocktick, safe_planes, start_time, test_mode;

extern FILE		*filein, *fileout;

extern C_SCREEN		screen, *sp;

extern LIST		air, ground;

extern struct sgttyb	tty_start, tty_new;

extern DISPLACEMENT	displacement[MAXDIR];

extern PLANE		*findplane(), *newplane();

int dir_no(int ch);
void ioclrtoeol(int pos);
void iomove(int pos);
void ioaddstr(int pos, char *str);
void ioclrtobot(void);
int getAChar(void);
void done_screen(void);
void redraw(void);
void ioerror(int pos, int len, char *str);
int number(int l);
void loser(PLANE *p, char *s);
void init_gr(void);
int yyparse(void);
void setup_screen(C_SCREEN *scp);
int addplane(void);
void log_score(int list_em);
void quit(int sig);
void update(int sig);
int getcommand(void);
void planewin(void);
int name(PLANE *p);
void erase_all(void);
void append(LIST *l, PLANE *p);
void delete(LIST *l, PLANE *p);
void draw_all(void);
