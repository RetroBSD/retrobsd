#ifndef _pin_magic_
#define _pin_magic_
// This header file serves two purposes:
//
// 1) Isolate non-portable MCU port- and pin-specific identifiers and
// operations so the library code itself remains somewhat agnostic
// (PORTs and pin numbers are always referenced through macros).
//
// 2) GCC doesn't always respect the "inline" keyword, so this is a
// ham-fisted manner of forcing the issue to minimize function calls.
// This sometimes makes the library a bit bigger than before, but fast++.
// However, because they're macros, we need to be SUPER CAREFUL about
// parameters -- for example, write8(x) may expand to multiple PORT
// writes that all refer to x, so it needs to be a constant or fixed
// variable and not something like *ptr++ (which, after macro
// expansion, may increment the pointer repeatedly and run off into
// la-la land). Macros also give us fine-grained control over which
// operations are inlined on which boards (balancing speed against
// available program space).
// When using the TFT shield, control and data pins exist in set physical
// locations, but the ports and bitmasks corresponding to each vary among
// boards. A separate set of pin definitions is given for each supported
// board type.
// When using the TFT breakout board, control pins are configurable but
// the data pins are still fixed -- making every data pin configurable
// would be much too slow. The data pin layouts are not the same between
// the shield and breakout configurations -- for the latter, pins were
// chosen to keep the tutorial wiring manageable more than making optimal
// use of ports and bitmasks. So there's a second set of pin definitions
// given for each supported board.
// Shield pin usage:

// Breakout pin usage:
// LCD Data Bit : 7 6 5 4 3 2 1 0
// Uno dig. pin : 7 6 5 4 3 2 9 8
// Uno port/pin : PD7 PD6 PD5 PD4 PD3 PD2 PB1 PB0
// Mega dig. pin: 27 6 5 4 3 2 9 8
// Mega port/pin: PH4 PH3 PE3 PG5 PE5 PE4 PH6 PH5

// Pixel read operations require a minimum 400 nS delay from RD_ACTIVE
// to polling the input pins. At 16 MHz, one machine cycle is 62.5 nS.
// This code burns 7 cycles (437.5 nS) doing nothing; the RJMPs are
// equivalent to two NOPs each, final NOP burns the 7th cycle, and the
// last line is a radioactive mutant emoticon.
#define DELAY7 \
asm volatile( \
"rjmp .+0" "\n\t" \
"rjmp .+0" "\n\t" \
"rjmp .+0" "\n\t" \
"nop" "\n" \
::);


// As part of the inline control, macros reference other macros...if any
// of these are left undefined, an equivalent function version (non-inline)
// is declared later. The Uno has a moderate amount of program space, so
// only write8() is inlined -- that one provides the most performance
// benefit, but unfortunately also generates the most bloat. This is
// why only certain cases are inlined for each board.


// Arduino Mega, ADK, etc.
// Mega port/pin: PH4 PH3 PE3 PG5 PE5 PE4 PH6 PH5
// Because the MEGA port to pin mapping is very messy it is necessary to shift the data bits around a lot.
  #define write8inline(d) {                          \
    PORTE = (PORTE & B11001111) | ((d << 2) & B00110000); \
	PORTE = (PORTE & B11110111) | ((d >> 2) & B00001000); \
	PORTG = (PORTG & B11011111) | ((d << 1) & B00100000); \
	PORTH = (PORTH & B11100111) | ((d >> 3) & B00011000); \
	PORTH = (PORTH & B10011111) | ((d << 5) & B01100000); \
	WR_STROBE; }
  #define read8inline(result) {                       \
    RD_ACTIVE;                                        \
    DELAY7;                                           \
    result = ((PINH & B00011000) << 3) | ((PINE & B00001000) << 2) | ((PING & B00100000) >> 1) |((PINE & B00110000) >> 2) | ((PINH & B01100000) >> 5); \
    RD_IDLE; }

 // // These set the PORT directions as required before the write and read
 // // operations.
  #define setWriteDirInline() { DDRE |=  B00111000; DDRG |=  B00100000; DDRH |= B01111000;}
  #define setReadDirInline() { DDRE &=  ~B00111000; DDRG &=  ~B00100000; DDRH &= ~B01111000;}


// All of the functions are inlined on the Arduino Mega. When using the
// breakout board, the macro versions aren't appreciably larger than the
// function equivalents, and they're super simple and fast. When using
// the shield, the macros become pretty complicated...but this board has
// so much code space, the macros are used anyway. If you need to free
// up program space, some macros can be removed, at a minor cost in speed.
#define write8 write8inline
#define read8 read8inline
#define setWriteDir setWriteDirInline
#define setReadDir setReadDirInline
#define writeRegister8 writeRegister8inline
#define writeRegister16 writeRegister16inline
#define writeRegisterPair writeRegisterPairInline
// When using the TFT breakout board, control pins are configurable.
#define RD_ACTIVE *rdPort &= rdPinUnset
#define RD_IDLE *rdPort |= rdPinSet
#define WR_ACTIVE *wrPort &= wrPinUnset
#define WR_IDLE *wrPort |= wrPinSet
#define CD_COMMAND *cdPort &= cdPinUnset
#define CD_DATA *cdPort |= cdPinSet
#define CS_ACTIVE *csPort &= csPinUnset
#define CS_IDLE *csPort |= csPinSet
#endif
// Data read and write strobes, ~2 instructions and always inline
#define RD_STROBE { RD_ACTIVE; RD_IDLE; }
#define WR_STROBE { WR_ACTIVE; WR_IDLE; }
// These higher-level operations are usually functionalized,
// except on Mega where's there's gobs and gobs of program space.
// Set value of TFT register: 8-bit address, 8-bit value
#define writeRegister8inline(a, d) { \
CD_COMMAND; write8(a); CD_DATA; write8(d); }
// Set value of TFT register: 16-bit address, 16-bit value
// See notes at top about macro expansion, hence hi & lo temp vars
#define writeRegister16inline(a, d) { \
uint8_t hi, lo; \
hi = (a) >> 8; lo = (a); CD_COMMAND; write8(hi); write8(lo); \
hi = (d) >> 8; lo = (d); CD_DATA ; write8(hi); write8(lo); }
// Set value of 2 TFT registers: Two 8-bit addresses (hi & lo), 16-bit value
#define writeRegisterPairInline(aH, aL, d) { \
uint8_t hi = (d) >> 8, lo = (d); \
CD_COMMAND; write8(aH); CD_DATA; write8(hi); \
CD_COMMAND; write8(aL); CD_DATA; write8(lo); }

