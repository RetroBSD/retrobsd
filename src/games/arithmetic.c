#ifdef CROSS
#   include </usr/include/stdio.h>
#else
#   include <stdio.h>
#endif
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

#define	MAX	100

char	types[10];
int	right[MAX];
int	left[MAX];
int	rights;
int	wrongs;
long	stvec;
long	etvec;
long	dtvec;

void score()
{
	time(&etvec);

	printf("\n\nRights %d; Wrongs %d; Score %d%%\n", rights, wrongs,
		(rights * 100)/(rights + wrongs));

	if (rights == 0)
                return;
	printf("Total time %ld seconds; %.1f seconds per problem\n\n\n",
		etvec - stvec,
		(etvec - stvec) / (rights + 0.));

	sleep(3);
	time(&dtvec);
	stvec += dtvec - etvec;
}

void delete(sig)
{
	if(rights + wrongs == 0.) {
		printf("\n");
		exit(0);
	}
	score();
	exit(0);
}

int getnum(s)
        char *s;
{
	int	a;
	char	c;

	a = 0;
	while((c = *s++) >= '0' && c <= '9') {
		a = a*10 + c - '0';
	}
	return(a);
}

int arand;

void srand13(n)
{
	arand = (n & 077774) | 01;
}

int rand13()		/*uniform on 0 to 2**13-1*/
{
	arand *= 3125;
	arand &= 077777;
	return(arand/4);
}

/*
 * 'hmul' returns the upper 16 bits of the product, where the operands
 *  are assumed to be 16-bit integers. It replaces an old PDP-11
 *  assembler language subroutine. -- dks.
 */
int hmul(a, b)
{
        return (long)a*b >> 16;
}

int skrand(range)
{
        int temp;

	temp = rand13() + rand13();
	if (temp >017777)
                temp = 040000 - temp;
	return hmul(temp, 8*range);
}

int rrandom(range)
{
	return(hmul(rand13(), 8*range));
}

void getln(s)
        char *s;
{
	register char	*rs;

	rs = s;

	while((*rs = getchar()) == ' ');
	while(*rs != '\n')
		if(*rs == 0)
			exit(0);
		else if(rs >= &s[99]) {
			while((*rs = getchar()) != '\n')
				if(*rs == '\0')
                                        exit(0);
		}
		else
			*++rs = getchar();
	while(*--rs == ' ')
		*rs = '\n';
}

int main(argc, argv)
        char	*argv[];
{
	int range, k, dif, l;
	char line[100];
	int ans, pans, i, j, t;

	signal(SIGINT, delete);

	range = 11;
	dif = 0;
	while(argc > 1) {
		switch(*argv[1]) {
		case '+':
		case '-':
		case 'x':
		case '/':
			while ((types[dif] = argv[1][dif]))
				dif++;
			break;

		default:
			range = getnum(argv[1]) + 1;
		}
		argv++;
		argc--;
	}
	if(range > MAX) {
		printf("Range is too large.\n");
		exit(0);
	}

	if(dif == 0) {
		types[0] = '+';
		types[1] = '-';
		dif = 2;
	}

	for(i = 0; i < range; i++) {
		left[i] = right[i] = i;
	}
	time(&stvec);
	k = stvec;
	srand13(k);
	k = 0;
	l = 0;
	goto start;

loop:
	if(++k%20 == 0)
		score();

start:
	i = skrand(range);
	j = skrand(range);
	if(dif > 1)
		l = rrandom(dif);

	switch(types[l]) {
		case '+':
		default:
			ans = left[i] + right[j];
			printf("%d + %d =   ", left[i], right[j]);
			break;

		case '-':
			t = left[i] + right[j];
			ans = left[i];
			printf("%d - %d =   ", t, right[j]);
			break;

		case 'x':
			ans = left[i] * right[j];
			printf("%d x %d =   ", left[i], right[j]);
			break;

		case '/':
			while(right[j] == 0)
				j = rrandom(range);
			t = left[i] * right[j] + rrandom(right[j]);
			ans = left[i];
			printf("%d / %d =   ", t, right[j]);
			break;
	}

loop1:
	getln(line);
	dtvec += etvec - stvec;
	if(line[0]=='\n') goto loop1;
	pans = getnum(line);
	if(pans == ans) {
		printf("Right!\n");
		rights++;
		goto loop;
	}
	else {
		printf("What?\n");
		wrongs++;
		if(range >= MAX)	goto loop1;
		left[range] = left[i];
		right[range++] = right[j];
		goto loop1;
	}
}
