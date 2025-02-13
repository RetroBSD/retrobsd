#pragma once

#define ST_RP           0x08000000      /* Enable reduced power mode */
#define ST_UM           0x00000010      /* User mode */

// Peripheral registers.
#ifdef __ASSEMBLER__
#define PIC32_R(a)              (0xBF800000 + (a))
#else
#define PIC32_R(a)              *(volatile unsigned*)(0xBF800000 + (a))
#endif


// Port A-G registers.
#define TRISA           PIC32_R (0x86000) /* Port A: mask of inputs */

struct devspec {
	unsigned _id;
	const char *_name;
};
