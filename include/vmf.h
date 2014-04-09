/*      Program Name:   vmf.h
 *      Author:  S.M. Schultz
 *
 *      -----------   Modification History   ------------
 *      Version Date            Reason For Modification
 *      1.0     01Jan80         1. Initial release.
 *      2.0     31Mar83         2. Cleanup.
 *	3.0	08Sep93		3. Change v_foffset to off_t instead of int.
 *	3.1	21Oct93		4. Create union member of structure to 
 *				   make 'int' or 'char' access to data easy.
 *				   Define segment+offset and modified macros.
 *				   Place into the public domain.
 *      --------------------------------------------------              
*/

#include <sys/types.h>

#define MAXSEGNO	16384	/* max number of segments in a space */
#define BYTESPERSEG     1024	/* must be power of two! */
#define	LOG2BPS		10	/* log2(BYTESPERSEG) */
#define WORDSPERSEG     (BYTESPERSEG/sizeof (int))

struct vspace {
	int     v_fd;           /* file for swapping */
	off_t   v_foffset;      /* offset for computing file addresses */
	int     v_maxsegno;     /* number of segments in this space */
	};

struct dlink {                  /* general double link structure */
	struct dlink *fwd;      /* forward link */
	struct dlink *back;     /* back link */
	};

struct	vseg {                    /* structure of a segment in memory */
	struct	dlink	s_link;		/* for linking into lru list */
	int	s_segno;        	/* segment number */
	struct	vspace	*s_vspace;      /* which virtual space */
	int	s_lock_count;
	int     s_flags;
	union
		{
		int	_winfo[WORDSPERSEG];	/* the actual segment */
		char	_cinfo[BYTESPERSEG];
		} v_un;
	};

#define	s_winfo	v_un._winfo
#define	s_cinfo	v_un._cinfo

/* masks for s_flags */
#define S_DIRTY 01              	/* segment has been modified */

	long	nswaps;         	/* number of swaps */
	long	nmapsegs;       	/* number of mapseg calls */

	int	vminit(), vmopen();
	struct	vseg	*vmmapseg();
	void	vmlock(), vmunlock(), vmclrseg(), vmmodify();
	void	vmflush(), vmclose();

typedef	long	VADDR;
#define	VMMODIFY(seg) (seg->s_flags |= S_DIRTY)
#define	VSEG(va) ((short)(va >> LOG2BPS))
#define	VOFF(va) ((u_short)va % BYTESPERSEG)
