/*
 * Global memory area system.
 *
 * Works with two system calls:
 *
 * byte = rdglob(addr);
 * success = wrglob(addr,byte);
 */
#include "param.h"
#include "systm.h"
#include "user.h"

#ifndef GLOBSZ
#define GLOBSZ 256
#endif

unsigned char global_segment[GLOBSZ];

void rdglob()
{
        struct a {
                int    addr;
        } *uap = (struct a *)u.u_arg;

	// Only root should have access to the shared memory block
	if(u.u_uid!=0)
	{
		u.u_rval = -1;
		return;
	}

	if(uap->addr>=GLOBSZ)
	{
		u.u_rval = -1;
		return;
	}
	u.u_rval = global_segment[uap->addr];
}

void wrglob()
{
        struct a {
                int    		addr;
		unsigned char 	value;
        } *uap = (struct a *)u.u_arg;

	// Only root should have access to the shared memory block
	if(u.u_uid!=0)
	{
		u.u_rval = -1;
		return;
	}

	if(uap->addr>=GLOBSZ)
	{
		u.u_rval = -1;
		return;
	}
	u.u_rval = 0;
	global_segment[uap->addr] = uap->value;
}
