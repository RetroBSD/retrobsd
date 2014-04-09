#include "hash.h"
#include "defs.h"

#define STRCMP(A, B)    (cf(A, B) != 0)
#define FACTOR                  035761254233    /* Magic multiplication factor */
#define TABLENGTH               64                              /* must be multiple of 2 */
#define LOG2LEN                 6                               /* log2 of TABLENGTH */

/*
    NOTE: The following algorithm only works on machines where
    the results of multiplying two integers is the least
    significant part of the double word integer required to hold
    the result.  It is adapted from Knuth, Volume 3, section 6.4.
*/

#define hash(str)               (int) (((unsigned) (crunch(str) * FACTOR)) >> shift)

struct node
{
	ENTRY item;
	struct node *Next;
};

static struct node      **last;
static struct node      *next;
static struct node      **table;

static unsigned int     bitsper;                /* Bits per byte */
static unsigned int     shift;

static unsigned int crunch();

hcreate()
{
#ifdef NOTDEF
	unsigned char c = ~0;                   /* A byte full of 1's */
#endif
	int j;

	table = (struct node **) alloc(TABLENGTH * sizeof(struct node *));

	for (j=0; j < TABLENGTH; ++j)
	{
		table[j] = (struct node *)NIL;
	}

	bitsper = 0;    /* число битов в байте */
#ifdef NOTDEF
	while (c)
	{
		c >>= 1;
		bitsper++;
	}

#else
	bitsper = 8;
#endif
	shift = (bitsper * sizeof(int)) - LOG2LEN;
}


void hscan(uscan)
	void    (*uscan)();
{
	struct node             *p, *nxt;
	int                             j;

	for (j=0; j < TABLENGTH; ++j)
	{
		p = table[j];
		while (p)
		{
			nxt = p->Next;
			(*uscan)(&p->item);
			p = nxt;
		}
	}
}



ENTRY *
hfind(str)
	char    *str;
{
	struct node     *p;
	struct node     **q;
	unsigned int    i;
	int                     res;

	i = hash(str);

	if(table[i] == NIL)
	{
		last = &table[i];
		next = NIL;
		return(NIL);
	}
	else
	{
		q = &table[i];
		p = table[i];
		while (p != NIL && (res = STRCMP(str, p->item.key)))
		{
			q = &(p->Next);
			p = p->Next;
		}

		if (p != NIL && res == 0)
			return(&(p->item));
		else
		{
			last = q;
			next = p;
			return(NIL);
		}
	}
}

ENTRY *
henter(item)
	ENTRY item;
{
	struct node     *p = (struct node *)alloc(sizeof(struct node));

	p->item = item;
	*last = p;
	p->Next = next;
	return(&(p->item));
}


static unsigned int
crunch(key)
	char    *key;
{
	unsigned int    sum = 0;
	int s;

	for (s = 0; *key; s++)                          /* Simply add up the bytes */
		sum += *key++;

	return(sum + s);
}
