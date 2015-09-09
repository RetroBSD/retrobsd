/*
 *
 * RetroBSD - PS2 keyboard driver for the Maximite PIC32 board
 *
 * Copyright (C) 2011 Rob Judd <judd@ob-wan.com>
 * All rights reserved.  The three clause ("New" or "Modified")
 * Berkeley software License Agreement specifies the terms and
 * conditions for redistribution.
 *
 */

#ifndef __KBD_H__
#define __KBD_H__


extern char init_kbd(void);
extern void read_kbd(void);
extern char write_kbd(u_char data);

#endif // __KBD_H__
