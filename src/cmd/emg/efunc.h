/* This file is in the public domain. */

/* EFUNC.H: function declarations and names
 *
 * This file list all the C code functions used. To add functions, declare it
 * here in both the extern function list and the name binding table
 */

extern int ctrlg();	       /* Abort out of things */
extern int quit();	       /* Quit */
extern int ctlxlp();	       /* Begin macro */
extern int ctlxrp();	       /* End macro */
extern int ctlxe();	       /* Execute macro */
extern int fileread();	       /* Get a file, read only */
extern int filewrite();        /* Write a file */
extern int filesave();	       /* Save current file */
extern int filename();	       /* Adjust file name */
extern int getccol();	       /* Get current column */
extern int gotobol();	       /* Move to start of line */
extern int forwchar();	       /* Move forward by characters */
extern int gotoeol();	       /* Move to end of line */
extern int backchar();	       /* Move backward by characters */
extern int forwline();	       /* Move forward by lines */
extern int backline();	       /* Move backward by lines */
extern int pagedown();	       /* PgDn */
extern int pageup();	       /* PgUp */
extern int gotobob();	       /* Move to start of buffer */
extern int gotoeob();	       /* Move to end of buffer */
extern int setfillcol();       /* Set fill column */
extern int setmark();	       /* Set mark */
extern int forwsearch();       /* Search forward */
extern int backsearch();       /* Search backwards */
extern int refresh();	       /* Refresh the screen */
extern int twiddle();	       /* Twiddle characters */
extern int tab();	       /* Insert tab */
extern int newline();	       /* Insert CR-LF */
extern int openline();	       /* Open up a blank line */
extern int quote();	       /* Insert literal */
extern int backword();	       /* Backup by words */
extern int forwword();	       /* Advance by words */
extern int forwdel();	       /* Forward delete */
extern int backdel();	       /* Backward delete */
extern int killtext();	       /* Kill forward */
extern int yank();	       /* Yank back from killbuffer */
extern int killregion();       /* Kill region */
extern int copyregion();       /* Copy region to kill buffer */
extern int setline();	       /* go to a numbered line */
extern int deskey();	       /* describe a key's binding */
extern int insfile();	       /* insert a file */
extern int forwhunt();	       /* hunt forward for next match */
extern int backhunt();	       /* hunt backwards for next match */
extern int showversion();      /* show emacs version */
