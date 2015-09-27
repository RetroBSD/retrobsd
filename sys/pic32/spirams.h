#ifndef _SPIRAMS_H
#define _SPIRAMS_H

extern int spirams_size(int unit);
extern int spirams_read(int unit, unsigned int offset, char *data, unsigned int bcount);
extern int spirams_write (int unit, unsigned offset, char *data, unsigned bcount);
extern void spirams_preinit (int unit);

#endif
