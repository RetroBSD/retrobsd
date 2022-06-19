#include "uucp.h"
#include "pk.h"
#include <setjmp.h>

jmp_buf Failbuf;

int Retries = 0;
struct pack *Pk;

static int gwrblk(char *blk, int len, int fn);
static int grdblk(char *blk, int len, int fn);

void pkfail()
{
	longjmp(Failbuf, 1);
}

int gturnon()
{
	struct pack *pkopen();

	if (setjmp(Failbuf))
		return FAIL;
	Pk = pkopen(Ifn, Ofn);
	if (Pk == NULL)
		return FAIL;
	return SUCCESS;
}

int gturnoff()
{
	if(setjmp(Failbuf))
		return(FAIL);
	pkclose(Pk);
	return SUCCESS;
}

int gwrmsg(char type, char *str, int fn)
{
	char bufr[BUFSIZ];
	register char *s;
	int len, i;

	if(setjmp(Failbuf))
		return(FAIL);
	bufr[0] = type;
	s = &bufr[1];
	while (*str)
		*s++ = *str++;
	*s = '\0';
	if (*(--s) == '\n')
		*s = '\0';
	len = strlen(bufr) + 1;
	if ((i = len % PACKSIZE)) {
		len = len + PACKSIZE - i;
		bufr[len - 1] = '\0';
	}
	gwrblk(bufr, len, fn);
	return SUCCESS;
}

/*ARGSUSED*/
int grdmsg(str, fn)
register char *str;
{
	unsigned len;

	if(setjmp(Failbuf))
		return FAIL;
	for (;;) {
		len = pkread(Pk, str, PACKSIZE);
		if (len == 0)
			continue;
		str += len;
		if (*(str - 1) == '\0')
			break;
	}
	return SUCCESS;
}

int gwrdata(fp1, fn)
FILE *fp1;
{
	char bufr[BUFSIZ];
	register int len;
	int ret, mil;
	struct timeval t1, t2;
	long bytes;
	char text[BUFSIZ];

	if(setjmp(Failbuf))
		return FAIL;
	bytes = 0L;
	Retries = 0;
	gettimeofday(&t1, NULL);

	while ((len = read(fileno(fp1), bufr, BUFSIZ)) > 0) {
		bytes += len;
		ret = gwrblk(bufr, len, fn);
		if (ret != len) {
			return FAIL;
		}
		if (len != BUFSIZ)
			break;
	}
	ret = gwrblk(bufr, 0, fn);

        gettimeofday(&t2, NULL);
	Now = t2;
	t2.tv_sec -= t1.tv_sec;
	mil = (t2.tv_usec - t1.tv_usec) / 1000;
	if (mil < 0) {
		--t2.tv_sec;
		mil += 1000;
	}
	sprintf(text, "sent data %ld bytes %ld.%02d secs",
				bytes, (long)t2.tv_sec, mil/10);
	sysacct(bytes, t2.tv_sec);
	if (Retries > 0)
		sprintf((char *)text+strlen(text)," %d retries", Retries);
	DEBUG(1, "%s\n", text);
	syslog(text);
	return SUCCESS;
}

int grddata(fn, fp2)
FILE *fp2;
{
	register int len;
	char bufr[BUFSIZ];
	struct timeval t1, t2;
	int mil;
	long bytes;
	char text[BUFSIZ];

	if(setjmp(Failbuf))
		return FAIL;
	bytes = 0L;
	Retries = 0;
	gettimeofday(&t1, NULL);

	for (;;) {
		len = grdblk(bufr, BUFSIZ, fn);
		if (len < 0) {
			return FAIL;
		}
		bytes += len;
		if (write(fileno(fp2), bufr, len) != len)
			return FAIL;
		if (len < BUFSIZ)
			break;
	}
	gettimeofday(&t2, NULL);
	Now = t2;
	t2.tv_sec -= t1.tv_sec;
	mil = (t2.tv_usec - t1.tv_usec) / 1000;
	if (mil < 0) {
		--t2.tv_sec;
		mil += 1000;
	}
	sprintf(text, "received data %ld bytes %ld.%02d secs",
				bytes, (long)t2.tv_sec, mil/10);
	sysacct(bytes, t2.tv_sec);
	if (Retries > 0)
		sprintf((char *)text+strlen(text)," %d retries", Retries);
	DEBUG(1, "%s\n", text);
	syslog(text);
	return SUCCESS;
}

#if !defined(BSD4_2) && !defined(USG)
/* call ultouch every TC calls to either grdblk or gwrblk */
#define	TC	20
static	int tc = TC;
#endif

/*ARGSUSED*/
int grdblk(blk, len,  fn)
register int len;
char *blk;
{
	register int i, ret;

#if !defined(BSD4_2) && !defined(USG)
	/* call ultouch occasionally */
	if (--tc < 0) {
		tc = TC;
		ultouch();
	}
#endif
	for (i = 0; i < len; i += ret) {
		ret = pkread(Pk, blk, len - i);
		if (ret < 0)
			return FAIL;
		blk += ret;
		if (ret == 0)
			return i;
	}
	return i;
}

/*ARGSUSED*/
int gwrblk(blk, len, fn)
register char *blk;
{
#if !defined(BSD4_2) && !defined(USG)
	/* call ultouch occasionally */
	if (--tc < 0) {
		tc = TC;
		ultouch();
	}
#endif
	return pkwrite(Pk, blk, len);
}
