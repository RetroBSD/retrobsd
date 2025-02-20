/* This file is in the public domain. */

/* EFUNC.H: function declarations and names
 *
 * This file list all the C code functions used. To add functions, declare it
 * here in both the extern function list and the name binding table
 */
int ctrlg(int, int);       /* Abort out of things */
int quit(int, int);        /* Quit */
int ctlxlp(int, int);      /* Begin macro */
int ctlxrp(int, int);      /* End macro */
int ctlxe(int, int);       /* Execute macro */
int fileread(int, int);    /* Get a file, read only */
int filewrite(int, int);   /* Write a file */
int filesave(int, int);    /* Save current file */
int filename(int, int);    /* Adjust file name */
int getccol(int);          /* Get current column */
int gotobol(int, int);     /* Move to start of line */
int forwchar(int, int);    /* Move forward by characters */
int gotoeol(int, int);     /* Move to end of line */
int backchar(int, int);    /* Move backward by characters */
int forwline(int, int);    /* Move forward by lines */
int backline(int, int);    /* Move backward by lines */
int pagedown(int, int);    /* PgDn */
int pageup(int, int);      /* PgUp */
int gotobob(int, int);     /* Move to start of buffer */
int gotoeob(int, int);     /* Move to end of buffer */
int setfillcol(int, int);  /* Set fill column */
int setmark(int, int);     /* Set mark */
int forwsearch(int, int);  /* Search forward */
int backsearch(int, int);  /* Search backwards */
int refresh(int, int);     /* Refresh the screen */
int twiddle(int, int);     /* Twiddle characters */
int tab(int, int);         /* Insert tab */
int newline(int, int);     /* Insert CR-LF */
int openline(int, int);    /* Open up a blank line */
int quote(int, int);       /* Insert literal */
int backword(int, int);    /* Backup by words */
int forwword(int, int);    /* Advance by words */
int forwdel(int, int);     /* Forward delete */
int backdel(int, int);     /* Backward delete */
int killtext(int, int);    /* Kill forward */
int yank(int, int);        /* Yank back from killbuffer */
int killregion(int, int);  /* Kill region */
int copyregion(int, int);  /* Copy region to kill buffer */
int setline(int, int);     /* go to a numbered line */
int deskey(int, int);      /* describe a key's binding */
int insfile(int, int);     /* insert a file */
int forwhunt(int, int);    /* hunt forward for next match */
int backhunt(int, int);    /* hunt backwards for next match */
int showversion(int, int); /* show emacs version */

int mlreply(char *, char *, int);
int mlreplyt(char *, char *, int, char);
void mlwrite(char *, ...);
LINE *lalloc(int);
int mlyesno(char *prompt);
void lfree(LINE *lp);
int typeahead(void);
void modeline(WINDOW *wp);
void updext(void);
void updateline(int row, char vline[], char pline[], short *flags);
void mlputi(int, int);
void mlputli(long, int);
void mlputs(char *);
int readin(char[]);
int ifile(char[]);
int bclear(BUFFER *);
int ffropen(char *);
int ffgetline(char[], int);
int ffclose(void);
int writeout(char *);
int ffwopen(char *);
int ffputline(char[], int);
int ldelnewline(void);
int kinsert(int);
void getwinsize(void);
void edinit(char[]);
void vtinit(void);
void update(void);
int getkey(void);
void mlerase(void);
int getctl(void);
int execute(int, int, int);
BUFFER *bfind(char *);
int linsert(int, int);
int anycb(void);
void vttidy(void);
void lchange(int);
int lnewline(void);
void kdelete(void);
int ldelete(int, int);
int kremove(int);
int getregion(REGION *);
int readpattern(char *);
int forscan(char *patrn, int leavep);
int bsearch(int f, int n);
int eq(int, int);
void expandp(char *, char *, int);
void tcapopen(void);
void tcaprev(int state);
void tcapmove(int row, int col);
void tcapeeol(void);
void tcapeeop(void);
void tcapbeep(void);
void ttopen(void);
void ttclose(void);
int ttgetc(void);
int ttputc(int);
void ttflush(void);
void panic(char *);
int inword(void);
