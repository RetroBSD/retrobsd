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
 
// Map of standard keyboard, US ASCII layout
const char lowerKey[128]={    
                0,    F9,     0,    F5,    F3,    F1,    F2,   F12,     //00
                0,   F10,    F8,    F6,    F4,   TAB,   '`',     0,     //08
                0,     0, L_SHF,     0, L_CTL,   'q',   '1',     0,     //10
                0,     0,   'z',   's',   'a',   'w',   '2',     0,     //18
                0,   'c',   'x',   'd',   'e',   '4',   '3',     0,     //20
                0,   ' ',   'v',   'f',   't',   'r',   '5',     0,     //28
                0,   'n',   'b',   'h',   'g',   'y',   '6',     0,     //30
                0,     0,   'm',   'j',   'u',   '7',   '8',     0,     //38
                0,   ',',   'k',   'i',   'o',   '0',   '9',     0,     //40
                0,   '.',   '/',   'l',   ';',   'p',   '-',     0,     //48
                0,     0,  '\'',     0,   '[',   '=',     0,     0,     //50
             CAPS, R_SHF, ENTER,   ']',     0,  '\\',     0,     0,     //58
                0,     0,     0,     0,     0,     0,   BKSP,    0,     //60
                0,   '1',     0,   '4',   '7',     0,     0,     0,     //68
              '0',   '.',   '2',   '5',   '6',   '8',   ESC,   NUM,     //70
              F11,   '+',   '3',   '-',   '*',   '9',     0,     0      //78
            };

const char upperKey[128] = {
                0,    F9,     0,    F5,    F3,    F1,    F2,   F12,     //00
                0,   F10,    F8,    F6,    F4,   TAB,   '~',     0,     //08
                0,     0, L_SHF,     0, L_CTL,   'Q',   '!',     0,     //10
                0,     0,   'Z',   'S',   'A',   'W',   '@',     0,     //18
                0,   'C',   'X',   'D',   'E',   '$',   '#',     0,     //20
                0,   ' ',   'V',   'F',   'T',   'R',   '%',     0,     //28
                0,   'N',   'B',   'H',   'G',   'Y',   '^',     0,     //30
                0,     0,   'M',   'J',   'U',   '&',   '*',     0,     //38
                0,   '<',   'K',   'I',   'O',   ')',   '(',     0,     //40
                0,   '>',   '?',   'L',   ':',   'P',   '_',     0,     //48
                0,     0,  '\"',     0,   '{',   '+',     0,     0,     //50
             CAPS, R_SHF, ENTER,   '}',     0,   '|',     0,     0,     //58
                0,     0,     0,     0,     0,     0,   BKSP,    0,     //60
                0,   '1',     0,   '4',   '7',     0,     0,     0,     //68
              '0',   '.',   '2',   '5',   '6',   '8',   ESC,   NUM,     //70
              F11,   '+',   '3',   '-',   '*',   '9',     0,     0      //78
            };

