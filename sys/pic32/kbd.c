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

/*
../kbd.c: In function 'initKBD':
../kbd.c:155:8: error: request for member 'ON' in something not a structure or union
../kbd.c:156:8: error: request for member 'CNPUE15' in something not a structure or union
../kbd.c:157:8: error: request for member 'CNPUE16' in something not a structure or union
../kbd.c: In function 'readKBD':
../kbd.c:173:9: error: request for member 'RD7' in something not a structure or union
../kbd.c:174:9: error: request for member 'RD6' in something not a structure or union
 *
 * This driver uses a 20uS timer, probably too fast
 */

#include "sys/types.h"
#include "machine/io.h"
#include "kbd.h"

//#define TRACE   printf
#ifndef TRACE
#define TRACE(...)
#endif

// I2C registers
struct kbdreg {
    //
};

#define USASCII         1
//#define RUSSIAN         1

#define true  1
#define false 0
#define PS2CLOCK        1//PORTD.RD6
#define PS2DATA         2//PORTD.RD7
#define QUEUE_SIZE      256

extern volatile char in_queue[QUEUE_SIZE];
extern volatile int in_queue_head, in_queue_tail;
volatile int abort;

#define POLL            20*(CPU_KHZ/1000)   // # clock cycles for 20uS between keyboard reads
#define TIMEOUT         500*(CPU_KHZ/1000)  // # clock cycles for 500uS timeout

enum {PS2START, PS2BIT, PS2PARITY, PS2STOP};

// Keyboard state machine and buffer
int state;
unsigned char key_buff;
int key_state, key_count, key_parity, key_timer;

// IBM keyboard scancode set 2 - Special Keys
#define F1      0x0e
#define F2      0x0f
#define F3      0x10
#define F4      0x11
#define F5      0x12
#define F6      0x13
#define F7      0x14                        // maps to F5
#define F8      0x15
#define F9      0x16
#define F10     0x17
#define F11     0x18
#define F12     0x19

#define NUM     0x00
#define BKSP    0x08
#define TAB     0x09
#define L_ALT   0x11
#define L_SHF   0x12
#define L_CTL   0x14
#define CAPS    0x58
#define R_SHF   0x59
#define ENTER   0x0d
#define ESC     0x1b
#define SCRL    0x7e

#ifdef USASCII
#include "usascii.inc"
#elif defined RUSSIAN
#include "russian.inc"
#endif

/*
    Standard PC init sequence:

    Keyboard: AA  Self-test passed                ;Keyboard controller init
    Host:     ED  Set/Reset Status Indicators
    Keyboard: FA  Acknowledge
    Host:     00  Turn off all LEDs
    Keyboard: FA  Acknowledge
    Host:     F2  Read ID
    Keyboard: FA  Acknowledge
    Keyboard: AB  First byte of ID
    Host:     ED  Set/Reset Status Indicators     ;BIOS init
    Keyboard: FA  Acknowledge
    Host:     02  Turn on Num Lock LED
    Keyboard: FA  Acknowledge
    Host:     F3  Set Typematic Rate/Delay        ;Windows init
    Keyboard: FA  Acknowledge
    Host:     20  500 ms / 30.0 reports/sec
    Keyboard: FA  Acknowledge
    Host:     F4  Enable
    Keyboard: FA  Acknowledge
    Host:     F3  Set Typematic Rate/delay
    Keyboard: FA  Acknowledge
    Host:     00  250 ms / 30.0 reports/sec
    Keyboard: FA  Acknowledge
*/

char init_kbd(void)
{
	// enable pullups on the clock and data lines.
	// This stops them from floating and generating random chars when no keyboard is attached
// 	CNCON.ON = 1;                           // turn on Change Notice for interrupt
// 	CNPUE.CNPUE15 = 1;                      // turn on the pullup for pin D6 also called CN15
// 	CNPUE.CNPUE16 = 1;                      // turn on the pullup for pin D7 also called CN16

 	return false;
}

void read_kbd(void)
{
    int data = PS2DATA;
    int clock = PS2CLOCK;
    static char key_up = false;
    static unsigned char code = 0;
    static unsigned then = 0;
    unsigned now = mips_read_c0_register (C0_COUNT, 0);

    // Is it time to poll the keyboard yet?
    if ((int) (now - then) < POLL)
        return;
	else
    	then = now;

    if (key_state) {                                            // if clock was high, key_state = 1
        if (!clock) {                                           // PS2CLOCK == 0, falling edge detected
            key_state = 0;                                      // transition to state 0
            key_timer = TIMEOUT;                                // restart the counter

            switch(state){
            default:
            case PS2START:
                if(!data) {                                     // PS2DATA == 0
                    key_count = 8;                              // init bit counter
                    key_parity = 0;                             // init parity check
                    code = 0;
                    state = PS2BIT;
                }
                break;

            case PS2BIT:
                code >>= 1;                                     // shift in data bit
                if(data)                                        // PS2DATA == 1
                    code |= 0x80;
                key_parity ^= code;
                if (--key_count == 0)
                    state = PS2PARITY;                          // all bits read
                break;

            case PS2PARITY:
                if(data)
                    key_parity ^= 0x80;
                if(key_parity & 0x80)                           // parity odd, continue
                    state = PS2STOP;
                else
                    state = PS2START;
                break;

            case PS2STOP:
                if(data) {
                    if(code == 0xf0)
                    	 key_up = true;
                    else {

                        char chr;
                        static char LShift = 0;
                        static char RShift = 0;
                        static char LCtrl = 0;
                        static char LAlt = 0;
                        static char CapsLock = 0;

                        if(key_up) {                            // check for special key release
                            key_up = false;
                            switch(code) {
                                case L_SHF: LShift = 0;
                                case R_SHF: RShift = 0;
                                case L_CTL: LCtrl = 0;
                                case L_ALT: LAlt = 0;
                            }
                            goto exit;
                        } else {                                // check for special key press
                            switch(code) {
                                case L_SHF: LShift = 1;
                                case R_SHF: RShift = 1;
                                case L_CTL: LCtrl = 1;
                                case L_ALT: LAlt = 1;
                                case CAPS:  CapsLock = !CapsLock;
                                default: break;
                            goto exit;
                            }
                        }

                        if(LShift || RShift)                        // get the ASCII code
                            chr = lowerKey[code%128];
                        else
                            chr = upperKey[code%128];

                        if(!chr)                                    // it was an unmapped key
                            break;

                        if(CapsLock && chr >= 'a' && chr <= 'z')    // check for altered keys
                            chr -= 32;
                        if(LCtrl)
                            chr &= 0x1F;

                        in_queue[in_queue_head] = chr;
                        in_queue_head = (in_queue_head + 1) % QUEUE_SIZE;

                        if(chr == 3) {                              // check for CTL-C
                            in_queue_head = in_queue_tail = 0;
                            abort = true;
                        }

//			PrintSignonToUSB = false;                   // show that the keyboard is in use

                        LAlt = LAlt;                                // not used yet
                    } // if key_up
exit:
                code = 0;
                } // if(data)
                state = PS2START;

            } // switch(state)

        } // if(!clock)

    } else // if(key_state)
        key_state = 1;                                          // PS2CLOCK == 1, rising edge detected

    if ((key_timer -= POLL) <= 0)
        state = PS2START;                                       // timeout, reset state machine

    return;
}

char write_kbd(u_char data)
{

    // do something here

    return false;
}
