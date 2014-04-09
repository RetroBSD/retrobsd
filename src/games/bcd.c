#ifdef CROSS
#   include </usr/include/stdio.h>
#else
#   include <stdio.h>
#endif

int chtab[] = {
        00000, /*   */
        03004, /* ! */
        02404, /* " */
        02040, /* sharp */
        02042, /* $ */
        02104, /* % */
        00001, /* & */
        03002, /* ' */
        02201, /* ( */
        02202, /* ) */
        02102, /* * */
        00005, /* + */
        02044, /* , */
        00002, /* - */
        02041, /* . */
        00014, /* / */
        00004, /* 0 */
        00010, /* 1 */
        00020, /* 2 */
        00040, /* 3 */
        00100, /* 4 */
        00200, /* 5 */
        00400, /* 6 */
        01000, /* 7 */
        02000, /* 8 */
        04000, /* 9 */
        02200, /* : */
        02402, /* ; */
        02401, /* < */
        02204, /* = */
        02400, /* > */
        03000, /* ? */
        02100, /* at */
        011,
        021,
        041,
        0101,
        0201,
        0401,
        01001,
        02001,
        04001,
        012,
        022,
        042,
        0102,
        0202,
        0402,
        01002,
        02002,
        02002,
        024,
        044,
        0104,
        0204,
        0404,
        01004,
        02004,
        04004,
        02020, /* [ */
        03001, /* \ */
        02101, /* ] */
        00006, /* ^ */
        02024 /* _ */
};

char s[128];
char *sp = {&s[0]};

void putstr(ss)
        char *ss;
{
	int t;

	while ((t = *ss++)) {
		if (t >= 'a' && t <= 'z')
			t += 'A'-'a';
		putchar(t);
	}
}

int main(argc, argv)
        char *argv[];
{
	char *spp;
	int i;
	int j;
	int c;
	int l;

	if (argc < 2) {
		putstr("% ");
		while ((c = getchar()) != '\0' && c != '\n')
			*sp++ = c;
		*sp = 0;
		sp = &s[0];
	} else
		sp = *++argv;
	putstr("\n\n\n\n");
	putstr(" ________________________________");
	putstr("________________\n");
	spp = sp;
	while(*spp++);
	spp--;
	l = spp - sp;
	putchar('/');
	putstr(sp);
	i = 49 - l;
	while(--i>0) putchar(' ');
	putstr("|\n");
	j = 0;
	spp = sp;
	while (j++ < 12) {
		putchar('|');
		i = 0;
		spp = sp;
		while (i < 48) {
			if (i > l)
                                c = 0;
			else
                                c = *spp++ - 040;
			i++;
			if (c >= 'a' - 040)
                                c = c - 040;

			if (c < 0 || c > 137)
                                c = 0;

			if ((chtab[c] >> (j-1)) & 1)
				putstr(":");
			else if (j > 3)
				putchar('0' + j - 3);
			else
				putchar(' ');
		}
		putstr("|\n");
	}
	putchar('|');
	putstr("____________");
	putstr("____________________________________");
	putstr("|\n");
	putstr("\n\n\n\n");
        return 0;
}
