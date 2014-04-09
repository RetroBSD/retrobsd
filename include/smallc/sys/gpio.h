/*
 * Ioctl definitions for GPIO driver.
 */
#define GPIO_PORT(n)	(n)         /* port number */
#define GPIO_PORTA	0
#define GPIO_PORTB	1
#define GPIO_PORTC	2
#define GPIO_PORTD	3
#define GPIO_PORTE	4
#define GPIO_PORTF	5
#define GPIO_PORTG	6

#define GPIO_CONFIN	0x20016700  /* configure as input */
#define GPIO_CONFOUT    0x20026700  /* configure as output */
#define GPIO_CONFOD	0x20046700  /* configure as open drain */
#define GPIO_DECONF	0x20086700  /* deconfigure */
#define GPIO_STORE	0x20106700  /* store all outputs */
#define GPIO_SET	0x20206700  /* set to 1 by mask */
#define GPIO_CLEAR	0x20406700  /* set to 0 by mask */
#define GPIO_INVERT	0x20806700  /* invert by mask */
#define GPIO_POLL	0x21006700  /* poll */
#define GPIO_LOL	0x82006700  /* display lol picture */
