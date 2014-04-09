/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/*
 * External definitions for
 * functions in inet(3N)
 */
struct	in_addr;

unsigned long inet_addr (char *);
char	*inet_ntoa (struct in_addr);
struct	in_addr inet_makeaddr (long, long);
unsigned long inet_network (char *);
unsigned long inet_netof (struct in_addr);
unsigned long inet_lnaof (struct in_addr);

/*
 * Macros for number representation conversion.
 */
unsigned htonl (unsigned hostlong);
unsigned htons (unsigned hostshort);
unsigned ntohl (unsigned netlong);
unsigned ntohs (unsigned netshort);
