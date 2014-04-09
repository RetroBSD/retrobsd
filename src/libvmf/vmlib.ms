.NH 1
VMF -- Virtual Memory Facility
.NH 2
Introduction
.PP
Since 2.11BSD is not a virtual memory operating system, a user level
"virtual memory" system can be of great utility to programs with
large address space requirements. This library provides a standard set
of routines for managing large virtual memory spaces.  It supports
creation of multiple concurrent virtual spaces, mapping of virtual
pages into real memory, a lock/unlock mechanism, and a capability to
clear specified virtual pages.
.NH 3
Concepts
.PP
The virtual memory scheme supports any number (subject to open file
limitations) of virtual
.I spaces,
each of which is made up of up to 512
.I segments
(pages), each of which is 512 words (16 bit shorts) long.
The 512 ints (1024 chars) of a segment can be used for any purpose.
.PP
Each space may be temporary or permanent, in the sense that it is or
is not forgotten when the program ends.  Between program invocations,
each permanent space resides in a named file.  The most common (by
far) type is temporary using a file which is unlinked immediately
after opening the virtual address space.
.PP
At any time during the lifetime of a process that uses the virtual 
memory facility, some number of segments will be in real memory, but
typically most will be "swapped out" to a file.
When the calling program knows
the real memory address of a segment, the segment may be viewed
and accessed as an array of 512 integers or as an array of 1024
characters.
.NH 3
Data Structures
.PP
Two structures are associated with the virtual memory scheme: the
space structure and the segment buffer structure.
.IP Space\ Structure 4
.sp
One instance of the space structure must be allocated by the user for
each space to be used.  Spaces are identified by the address of their
space structures.  Included in the structure is the UNIX file descriptor
of the "swap" file to be used for the space, the offset (in bytes)
into the file at which the room allocated for the virtual space begins
(currently this is always zero and is reserved for future use),
and the number of segments in the space (currently always 512).  The
user must never be concerned with any element of the space structure.
.IP Segment\ Buffer\ Structure 4
.sp
The virtual memory system includes a pool of buffers for holding
segments while they reside in real memory.
.sp
The segment buffer structure (see 'struct seg' definition below)
includes: a double link, the segment number (in range [0, 511]),
the address of the virtual space structure instance for the virtual
space of which the segment is a member, a lock count, a "modified"
flag, and an array of 512 shorts (1024 bytes) for the segment itself.  This
array represents the virtual page which is accessed directly by
the user.
.NH 2
Defined Constants and Global Variables
.PP
The following defined symbolic constants are specified as either
calling parameters or returned values of the routines in the "dtlib"
facility.
.IP MAXSEGNO 18
Specifies the maximum number of segments (pages) in a virtual space.
.IP WORDSPERSEG 18
Specifies the number of words in a segment.
.IP BYTESPERSEG 18
Specifies the number of bytes in a segment.  This is always a power of 2.
.IP LOG2BPS 18
The base 2 logarithm of BYTESPERSEG.
.PP
There are several global variables defined by the "vmlib" facility that an
applications programmer may examine. 
.IP nswaps 18
This is a 32 bit value which gives the number of pages swapped to or from
the paging file.
.IP nmapsegs 18
This 32 bit value holds the number of calls made to map a segment into
memory via \fIvmmapseg\fP.
.PP
The following
defined "C" Language structures are specified as calling parameters
and/or returned values of the routines in the "vmlib" facility.
.PP
.nf
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
.fi
.NH 2
Routine Synopsis
.PP
Below are listed the names of each function
in the "vmlib" facility along with a brief synopsis
of what each routine does.
.IP vminit 14
This routine initializes the virtual memory system by setting up the
pool of in-memory segment buffers.  The argument to this function is
the number (typically 4 to 8 but can be higher as long as memory
can be malloc'd).
It must be called before any
other "vmlib" facility routine is called.
.IP vmopen 14
For each virtual space that a program uses, the program must 
allocate an instance of the space structure (see definition 
of 'struct vspace' above).  This routine is used to initialize
a virtual space structure using the specified address of a
space structure and the name of the file that will serve as 
swap file for the space.  If the second argument is \fBNULL\fP
an invisible temporary file is used rather than a named (permanent)
file.
.IP vmclose 14
This routine is used to close the UNIX file descriptor associated
with the swap file for a virtual space.  Any modified in-memory segments
belonging to the specified address space are flushed to the paging file.
.IP vmmapseg 14
This routine is the primary interface to the virtual memory mechanism.
It is executed with a specified virtual space address and a segment
number ( in range [0, 511]), and returns a pointer to an in-memory
page containing the specified segment.
.IP vmmodify 14
Whenever a program modifies the data of a segment, it is the program's
responsibility to inform the virtual memory system of the modification.
This routine flags the specified segment as modified and should be
executed before any future execution of 'vmmapseg' that could overlay
the modified buffer.  This function is also available as a macro
(\fBVMMODIFY\fP) for use in-line.  The macro form is actually preferred
since only a single bit is being set in a word.
.IP vmlock 14
This routine increments the lock count of the specified segment buffer.
A buffer with a nonzero lock count is
.I locked
and cannot be overlayed.  The utility of this feature to the user is
that a locked segment buffer will stay at the same real memory address
until unlocked, hence real memory pointers into or to it can remain
valid.  Overuse of this feature will result in not being able to page
any new segments into memory from the paging file.
.IP vmunlock 14
This routine decrements the lock count of the specified buffer.  It is
a serious error to decrement the count below zero (lock underflow).
.IP vmclrseg 14
This routine clears the user data area (page) of the specified segment buffer.
.IP vmflush 14
This routine simply swaps out all segments that are marked as
"modified".  It may be called at any time and is always called when
\fIvmclose\fP is called.
.bp
.NH 2
Detailed Description of Virtual Memory Functions
.NH 3
vminit -- Initialize Virtual Memory Interface
.NH 4
.PP
The "vminit" routine initializes the virtual memory system by setting up the
pool of in-memory segment buffers.  It must be called before any
other "vmlib" facility routine is called.
.NH 4
CALLING SEQUENCE
.PP
.in +8
.nf
status = vminit(nseg)

int nseg;
int status;
.fi
.in -8
.IP status
The return value will be \-1 and \fIerrno\fP will be set to ENOMEM
if memory for
the specified number of segments can not be allocated.
.IP nseg
This is the number of in-memory segments to allocate.  These are shared
between all virtual spaces opened.  Each segment is 1kb plus overhead,
so the number of segments should be chosen with care (if all data could
be held in memory we wouldn't need a virtual memory system after all).
Typical number of segments is between 4 and 8 although some applications
such as \fBld\fP(1) use 12 or more.
.NH 3
vmopen -- Open a Virtual Space
.NH 4
.PP
For each virtual space that a program uses, the program must 
allocate an instance of the space structure (see definition 
of 'struct vspace' above).  The "vmopen" routine is used to initialize
a virtual space structure using the specified address of a
space structure and the name of the file that will serve as 
swap file for the space.  If the filename is NULL then an invisible
temporary file created in /tmp will be used and the address space
will be vanish when the program exits or calls \fIvmclose\fP.
.NH 4
CALLING SEQUENCE
.PP
.in +8
.nf
status = vmopen(vsptr,filename)

struct vspace   *vsptr;
char *filename;
int status;
.fi
.in -8
.IP status
Indicates success if 0, an error if \-1.  An error is caused by failure
to create (or open for update) the specified file.  The global \fBerrno\fP
will be set if \-1 is returned by \fIvmopen\fP.
.NH 3
vmclose -- Close a Virtual Space
.NH 4
.PP
Close the UNIX file descriptor associated with the swap 
file for a virtual space.  Modified segments belonging to the specified
virtual space are flushed.
.NH 4
CALLING SEQUENCE
.PP
.in +8
.nf
void vmclose(vsptr)

struct vspace   *vsptr;
.fi
.in -8
.IP vsptr
The address of the virtual space structure to be closed.  No status is
returned.
.NH 3
vmmapseg -- Map a Virtual Segment into Real Memory
.NH 4
.PP
The "vmmapseg" routine is the primary interface to the virtual memory mechanism.
It is executed with a specified virtual space address and a segment
number ( in range [0, 511]), and returns the real memory address
of a buffer guaranteed to contain the segment requested.  The address 
returned is that of the 'struct vseg' not that of the data portion.  It
is up to the program to refer to the 's_cinfo' or 's_winfo' member 
to refer to the byte or word oriented data respectively.
.NH 4
CALLING SEQUENCE
.PP
.in +8
.nf
segptr = vmmapseg(vsptr,segno)

struct vspace *vsptr;
int segno;
struct vseg *segptr;
.fi
.in -8
.NH 4
.IP vsptr
Specifies the address of a virtual space structure allocated by
the calling program.  A previous vmopen call using this structure
must have been performed before calling \fIvmmapseg\fP.
.IP segno
Specifies the segment i.e., virtual page, number to be mapped into
real physical memory.  The value must be in the range [0 to MAXSEGNO-1].
There are two macros (defined in \fIvmf.h\fP) which are useful in
calculating segment numbers: \fBVSEG\fP and \fBVOFF\fP.
.IP segptr
The address of a virtual seg structure which contains the virtual
space segment associated with segno.  The data is referred to with
the 's_cinfo' or 's_winfo' structure members:  segptr->s_cinfo
or segptr->s_winfo respectively.
.PP
Currently this routine aborts the program on an error condition.
.NH 3
vmmodify -- Mark a Segment as Modified
.NH 4
.PP
Whenever the user program modifies any segment, it is the program's
responsibility to inform the virtual memory system of the modification.
The "vmmodify" routine flags the specified segment as modified and should be
executed before any future execution of 'vmmapseg' that could reuse
the modified buffer.  Note that a locked segment buffer cannot be reused,
until it is unlocked.  A macro form of this routine exists, \fBVMMODIFY\fP
may be used to mark a segment as modified.
.NH 4
CALLING SEQUENCE
.PP
.in +8
.nf
void vmmodify(segptr)

struct vseg *segptr;
.fi
.in -8
.IP segptr
Specifies the address of a seg structure associated with a current
incore virtual memory segment which has been modified.
.NH 3
vmlock -- Lock a Virtual Segment into Real Memory
.NH 4
.PP
Increment the lock count of the specified segment buffer.
A buffer with a nonzero lock count is
.I locked
and cannot be reused/swapped-out.  The utility of this feature to the user is
that a locked segment buffer will stay at the same real memory address
until unlocked, hence real memory pointers into or to it can remain
valid.  Overuse of \fIvmlock\fP will cause errors in \fIvmmapseg\fP
when no segments are available to satisfy swap in/out requests.
.NH 4
CALLING SEQUENCE
.PP
.in +8
.nf
void vmlock(segptr1)

struct vseg      *segptr;
.fi
.in -8
.NH 4
.IP segptr
Specifies the address of a seg structure associated with a current
incore virtual memory segment.  No information is returned.
.NH 3
vmunlock -- Unlock a Previously Locked Virtual Memory Segment
.NH 4
.PP
Decrement the lock count of the specified buffer.  It is a serious error
to decrement the count below zero, this typically means that the segment
was never locked in the first place.
.NH 4
CALLING SEQUENCE
.PP
.in +8
.nf
void vmunlock(segptr)

struct vseg      *segptr;
.fi
.in -8
.NH 4
.IP segptr1
Specifies the address of a seg structure associated with a current
incore virtual memory segment.  No information is returned from this
routine.
.NH 3
vmclrseg -- Clear a Virtual Memory Segment to Zeros
.NH 4
.PP
Clear the data area of the specified segement buffer.  No information is
returned by this function.
.NH 4
CALLING SEQUENCE
.PP
.in +8
.nf
void vmclrseg(segptr)

struct vseg      *segptr
.fi
.in -8
.NH 4
.IP segptr
Specifies the address of a seg structure associated with a current
incore virtual memory segment.  The data portion (segptr->s_cinfo)
is cleared by a call to \fIbzero\fP(3).
.NH 3
vmflush -- Flush Virtual Memory Cache to Swap File
.NH 4
.PP
Swap out all segments that are marked as
"modified". It may be called just prior to program termination
if there are any permanent spaces that have been modified but 
normally a program will simply call \fIvmclose\fP which takes care
of flushing modified pages as part of its normal duties.
.NH 4
CALLING SEQUENCE
.PP
.in +8
.nf
void vmflush()
.fi
.in -8
