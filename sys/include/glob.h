#ifndef _GLOB_H
#define _GLOB_H

#ifdef KERNEL
extern void rdglob();
extern void wrglob();
#else
extern int rdglob(int);
extern int wrglob(int,int);
#endif

#endif
