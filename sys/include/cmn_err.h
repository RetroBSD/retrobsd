#ifndef _SYS_CMN_ERR_H
#define _SYS_CMN_ERR_H

#define CE_CONT   0 /* continuation */
#define CE_NOTICE 1 /* notice */
#define CE_WARN   2 /* warning */
#define CE_PANIC  3 /* panic */

extern void cmn_err(int level, char *fmt, ...);
extern void xcmn_err(int level, char **fmtp);

#endif /* _SYS_CMN_ERR_H */
