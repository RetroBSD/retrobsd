#ifndef _MRAMS_H
#define _MRAMS_H

extern int mrams_size(int unit);
extern int mrams_read(int unit, unsigned int offset, char *data, unsigned int bcount);
extern int mrams_write (int unit, unsigned offset, char *data, unsigned bcount);
extern void mrams_preinit (int unit);

#endif
