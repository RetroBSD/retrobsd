/*
 * Copyright (c) 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * CTL_MACHDEP definitions.
 */
#define	CPU_CONSDEV		1	/* dev_t: console terminal device */
#define	CPU_ERRMSG		2	/* get error message by errno */
#define	CPU_NLIST		3	/* get name address */
#define CPU_TIMO_CMD            4
#define CPU_TIMO_SEND_OP        5
#define CPU_TIMO_SEND_CSD       6
#define CPU_TIMO_READ           7
#define CPU_TIMO_WAIT_CMD       8
#define CPU_TIMO_WAIT_WDATA     9
#define CPU_TIMO_WAIT_WDONE     10
#define CPU_TIMO_WAIT_WSTOP     11
#define CPU_TIMO_WAIT_WIDLE     12
#define	CPU_MAXID		13	/* number of valid machdep ids */

#ifndef	KERNEL
#define CTL_MACHDEP_NAMES { \
	{ 0, 0 }, \
	{ "console_device", CTLTYPE_STRUCT }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ "sd_timeout_cmd", CTLTYPE_INT }, \
	{ "sd_timeout_send_op", CTLTYPE_INT }, \
	{ "sd_timeout_send_csd", CTLTYPE_INT }, \
	{ "sd_timeout_read", CTLTYPE_INT }, \
	{ "sd_timeout_wait_cmd", CTLTYPE_INT }, \
	{ "sd_timeout_wait_wdata", CTLTYPE_INT }, \
	{ "sd_timeout_wait_wdone", CTLTYPE_INT }, \
	{ "sd_timeout_wait_wstop", CTLTYPE_INT }, \
	{ "sd_timeout_wait_widle", CTLTYPE_INT }, \
}
#endif
