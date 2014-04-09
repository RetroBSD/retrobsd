/*
 * SmallC: interface to curses library.
 */
#define WINDOW int

extern WINDOW *stdscr, *curscr;

extern int LINES, COLS;

/*
 * pseudo functions for standard screen
 */
#define	addch(ch)	waddch(stdscr, ch)
#define	getch()		wgetch(stdscr)
#define	addstr(str)	waddstr(stdscr, str)
#define	getstr(str)	wgetstr(stdscr, str)
#define	move(y, x)	wmove(stdscr, y, x)
#define	clear()		wclear(stdscr)
#define	erase()		werase(stdscr)
#define	clrtobot()	wclrtobot(stdscr)
#define	clrtoeol()	wclrtoeol(stdscr)
#define	insertln()	winsertln(stdscr)
#define	deleteln()	wdeleteln(stdscr)
#define	refresh()	wrefresh(stdscr)
#define	inch()		winch(stdscr)
#define	insch(c)	winsch(stdscr,c)
#define	delch()		wdelch(stdscr)
#define	standout()	wstandout(stdscr)
#define	standend()	wstandend(stdscr)

/*
 * mv functions
 */
#define	mvwaddch(win,y,x,ch)	wmove(win,y,x) == 0 ? 0 : waddch(win,ch)
#define	mvwgetch(win,y,x)	wmove(win,y,x) == 0 ? 0 : wgetch(win)
#define	mvwaddstr(win,y,x,str)	wmove(win,y,x) == 0 ? 0 : waddstr(win,str)
#define mvwgetstr(win,y,x,str)  wmove(win,y,x) == 0 ? 0 : wgetstr(win,str)
#define	mvwinch(win,y,x)	wmove(win,y,x) == 0 ? 0 : winch(win)
#define	mvwdelch(win,y,x)	wmove(win,y,x) == 0 ? 0 : wdelch(win)
#define	mvwinsch(win,y,x,c)	wmove(win,y,x) == 0 ? 0 : winsch(win,c)
#define	mvaddch(y,x,ch)		mvwaddch(stdscr,y,x,ch)
#define	mvgetch(y,x)		mvwgetch(stdscr,y,x)
#define	mvaddstr(y,x,str)	mvwaddstr(stdscr,y,x,str)
#define mvgetstr(y,x,str)       mvwgetstr(stdscr,y,x,str)
#define	mvinch(y,x)		mvwinch(stdscr,y,x)
#define	mvdelch(y,x)		mvwdelch(stdscr,y,x)
#define	mvinsch(y,x,c)		mvwinsch(stdscr,y,x,c)

#ifdef TODO

#define	TRUE	(1)
#define	FALSE	(0)
#define	ERR	(0)
#define	OK	(1)

/*
 * Capabilities from termcap
 */
extern int     AM, BS, CA, DA, DB, EO, HC, HZ, IN, MI, MS, NC, NS, OS, UL,
		XB, XN, XT, XS, XX;
extern char	*AL, *BC, *BT, *CD, *CE, *CL, *CM, *CR, *CS, *DC, *DL,
		*DM, *DO, *ED, *EI, *K0, *K1, *K2, *K3, *K4, *K5, *K6,
		*K7, *K8, *K9, *HO, *IC, *IM, *IP, *KD, *KE, *KH, *KL,
		*KR, *KS, *KU, *LL, *MA, *ND, *NL, *RC, *SC, *SE, *SF,
		*SO, *SR, *TA, *TE, *TI, *UC, *UE, *UP, *US, *VB, *VS,
		*VE, *AL_PARM, *DL_PARM, *UP_PARM, *DOWN_PARM,
		*LEFT_PARM, *RIGHT_PARM;
extern char	PC;

/*
 * From the tty modes...
 */
extern int	GT, NONL, UPPERCASE, normtty, _pfast;

extern int	My_term, _echoit, _rawmode, _endwin;

extern char	*Def_term, ttytype[];

extern int	_tty_ch, _res_flg;

extern SGTTY	_tty;

/*
 * pseudo functions
 */
#define	clearok(win,bf)	 (win->_clear = bf)
#define	leaveok(win,bf)	 (win->_leave = bf)
#define	scrollok(win,bf) (win->_scroll = bf)
#define flushok(win,bf)	 (bf ? (win->_flags |= _FLUSH):(win->_flags &= ~_FLUSH))
#define	getyx(win,y,x)	 y = win->_cury, x = win->_curx
#define	winch(win)	 (win->_y[win->_cury][win->_curx] & 0177)

#define raw()	 (_tty.sg_flags|=RAW, _pfast=_rawmode=TRUE, ioctl(_tty_ch,TIOCSETP,&_tty))
#define noraw()	 (_tty.sg_flags&=~RAW,_rawmode=FALSE,_pfast=!(_tty.sg_flags&CRMOD),ioctl(_tty_ch,TIOCSETP,&_tty))
#define cbreak() (_tty.sg_flags |= CBREAK, _rawmode = TRUE, ioctl(_tty_ch,TIOCSETP,&_tty))
#define nocbreak() (_tty.sg_flags &= ~CBREAK,_rawmode=FALSE,ioctl(_tty_ch,TIOCSETP,&_tty))
#define echo()	 (_tty.sg_flags |= ECHO,  _echoit = TRUE,    ioctl(_tty_ch, TIOCSETP, &_tty))
#define noecho() (_tty.sg_flags &= ~ECHO, _echoit = FALSE,   ioctl(_tty_ch, TIOCSETP, &_tty))
#define nl()	 (_tty.sg_flags |= CRMOD, _pfast  = _rawmode,ioctl(_tty_ch, TIOCSETP, &_tty))
#define nonl()	 (_tty.sg_flags &= ~CRMOD,_pfast  = TRUE,    ioctl(_tty_ch, TIOCSETP,&_tty))
#define	savetty() ((void) ioctl(_tty_ch, TIOCGETP, &_tty), _res_flg = _tty.sg_flags)
#define	resetty() (_tty.sg_flags = _res_flg, (void) ioctl(_tty_ch, TIOCSETP, &_tty))

#define	erasechar()	(_tty.sg_erase)
#define	killchar()	(_tty.sg_kill)
#define baudrate()	(_tty.sg_ospeed)

/*
 * Used to be in unctrl.h.
 */
#define	unctrl(c)	_unctrl[(c) & 0177]
extern char *_unctrl[];

#endif
