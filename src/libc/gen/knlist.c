#include <stdio.h>
#include <string.h>
#include <nlist.h>
#include <sys/sysctl.h>
#include <machine/cpu.h>

int
knlist(list)
	struct nlist *list;
{
	register struct nlist *p;
	int  mib[2], entries = 0;
	size_t size;

	mib[0] = CTL_MACHDEP;
	mib[1] = CPU_NLIST;

	/*
	 * Clean out any left-over information for all valid entries.
	 * Type and value defined to be 0 if not found; historical
	 * versions cleared other and desc as well.
	 */
	for (p=list; p->n_name && p->n_name[0]; ++p) {
                size = sizeof(p->n_value);
                if (sysctl(mib, 2, &p->n_value, &size,
                    p->n_name, 1 + strlen(p->n_name)) < 0) {
                        p->n_value = 0;
                        continue;
                }
                ++entries;
	}
	return entries;
}
