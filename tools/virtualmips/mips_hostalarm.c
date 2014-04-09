#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <time.h>
#include <errno.h>
#ifdef __linux__
#include <linux/rtc.h>
#endif

#include "cpu.h"
#include "vm.h"
#include "mips_exec.h"
#include "mips_memory.h"
#include "mips.h"
#include "mips_cp0.h"
#include "debug.h"
#include "vp_timer.h"
#include "mips_hostalarm.h"

/*code from qemu*/

#define TFR(expr) do { if ((expr) != -1) break; } while (errno == EINTR)
static struct qemu_alarm_timer *alarm_timer;

extern cpu_mips_t *current_cpu;

#define RTC_FREQ 1024

#define ALARM_FLAG_DYNTICKS  0x1
#define ALARM_FLAG_EXPIRED   0x2

/* TODO: MIN_TIMER_REARM_US should be optimized */
#define MIN_TIMER_REARM_US 250

struct hpet_info {
    unsigned long hi_ireqfreq;  /* Hz */
    unsigned long hi_flags;     /* information */
    unsigned short hi_hpet;
    unsigned short hi_timer;
};

#define	HPET_INFO_PERIODIC	0x0001  /* timer is periodic */

#define	HPET_IE_ON	_IO('h', 0x01)  /* interrupt on */
#define	HPET_IE_OFF	_IO('h', 0x02)  /* interrupt off */
#define	HPET_INFO	_IOR('h', 0x03, struct hpet_info)
#define	HPET_EPI	_IO('h', 0x04)  /* enable periodic */
#define	HPET_DPI	_IO('h', 0x05)  /* disable periodic */
#define	HPET_IRQFREQ	_IOW('h', 0x6, unsigned long)   /* IRQFREQ usec */

/*
 * Host alarm, fired once 1 ms.
 * It will find whether a timer has been expired.
 * If so, run timers.
 */
void host_alarm_handler (int host_signum)
{
    if (unlikely (current_cpu->state != CPU_STATE_RUNNING))
        return;

    if (vp_timer_expired (active_timers[VP_TIMER_REALTIME],
            vp_get_clock (rt_clock))) {
        /* Tell cpu we need to pause because timer out */
        current_cpu->pause_request |= CPU_INTERRUPT_EXIT;
    }

    /* Check count and compare */
#define KHZ 80000
    current_cpu->cp0.reg[MIPS_CP0_COUNT] += KHZ / 2;
    if (current_cpu->cp0.reg[MIPS_CP0_COMPARE] != 0) {
        if (current_cpu->cp0.reg[MIPS_CP0_COUNT] >=
            current_cpu->cp0.reg[MIPS_CP0_COMPARE]) {
                set_timer_irq (current_cpu);
        }
    }
/*printf ("-- count = %u, compare = %u\n", current_cpu->cp0.reg[MIPS_CP0_COUNT], current_cpu->cp0.reg[MIPS_CP0_COMPARE]);*/

    host_alarm (current_cpu, KHZ);
}

#ifdef __linux__

static void enable_sigio_timer (int fd)
{
    struct sigaction act;

    /* timer signal */
    sigfillset (&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = host_alarm_handler;

    sigaction (SIGIO, &act, NULL);
    fcntl (fd, F_SETFL, O_ASYNC);
    fcntl (fd, F_SETOWN, getpid ());
}

static int hpet_start_timer (struct qemu_alarm_timer *t)
{
    struct hpet_info info;
    int r, fd;

    fd = open ("/dev/hpet", O_RDONLY);
    if (fd < 0)
        return -1;

    /* Set frequency */
    r = ioctl (fd, HPET_IRQFREQ, RTC_FREQ);
    if (r < 0) {
        fprintf (stderr,
            "Could not configure '/dev/hpet' to have a 1024Hz timer. This is not a fatal\n"
            "error, but for better emulation accuracy type:\n"
            "'echo 1024 > /proc/sys/dev/hpet/max-user-freq' as root.\n");
        goto fail;
    }

    /* Check capabilities */
    r = ioctl (fd, HPET_INFO, &info);
    if (r < 0)
        goto fail;

    /* Enable periodic mode */
    r = ioctl (fd, HPET_EPI, 0);
    if (info.hi_flags && (r < 0))
        goto fail;

    /* Enable interrupt */
    r = ioctl (fd, HPET_IE_ON, 0);
    if (r < 0)
        goto fail;

    enable_sigio_timer (fd);
    t->priv = (void *) (long) fd;

    return 0;
  fail:
    close (fd);
    return -1;
}

static void hpet_stop_timer (struct qemu_alarm_timer *t)
{
    int fd = (long) t->priv;

    close (fd);
}

static int rtc_start_timer (struct qemu_alarm_timer *t)
{
    int rtc_fd;

    TFR (rtc_fd = open ("/dev/rtc", O_RDONLY));
    if (rtc_fd < 0)
        return -1;
    if (ioctl (rtc_fd, RTC_IRQP_SET, RTC_FREQ) < 0) {
        fprintf (stderr,
            "Could not configure '/dev/rtc' to have a 1024 Hz timer. This is not a fatal\n"
            "error, but for better emulation accuracy either use a 2.6 host Linux kernel or\n"
            "type 'echo 1024 > /proc/sys/dev/rtc/max-user-freq' as root.\n");
        goto fail;
    }
    if (ioctl (rtc_fd, RTC_PIE_ON, 0) < 0) {
      fail:
        close (rtc_fd);
        return -1;
    }

    enable_sigio_timer (rtc_fd);

    t->priv = (void *) (long) rtc_fd;

    return 0;
}

static void rtc_stop_timer (struct qemu_alarm_timer *t)
{
    int rtc_fd = (long) t->priv;

    close (rtc_fd);
}

#endif /* __linux__ */

static int unix_start_timer (struct qemu_alarm_timer *t)
{
    struct sigaction act;
    struct itimerval itv;
    int err;

    /* timer signal */
    sigfillset (&act.sa_mask);
    act.sa_flags = 0;
    act.sa_handler = host_alarm_handler;

    sigaction (SIGALRM, &act, NULL);

    itv.it_interval.tv_sec = 0;
    /* for i386 kernel 2.6 to get 1 ms */
    itv.it_interval.tv_usec = 999;
    itv.it_value.tv_sec = 0;
    itv.it_value.tv_usec = 10 * 1000;

    err = setitimer (ITIMER_REAL, &itv, NULL);
    if (err)
        return -1;

    return 0;
}

static void unix_stop_timer (struct qemu_alarm_timer *t)
{
    struct itimerval itv;

    memset (&itv, 0, sizeof (itv));
    setitimer (ITIMER_REAL, &itv, NULL);
}

static struct qemu_alarm_timer alarm_timers[] = {
#ifdef __linux__
    /* HPET - if available - is preferred */
    {"hpet", 0, hpet_start_timer, hpet_stop_timer, NULL, NULL},
    /* ...otherwise try RTC */
    {"rtc", 0, rtc_start_timer, rtc_stop_timer, NULL, NULL},
#endif
    {"unix", 0, unix_start_timer, unix_stop_timer, NULL, NULL},
    {NULL,}
};

/*host alarm*/
void mips_init_host_alarm (void)
{
    struct qemu_alarm_timer *t;
    int i, err = -1;

    for (i = 0; alarm_timers[i].name; i++) {
        t = &alarm_timers[i];
        err = t->start (t);
        if (! err)
            break;
    }
/*#define DEBUG_HOST_ALARM*/
#ifdef DEBUG_HOST_ALARM
    printf ("--- Using %s timer\n", alarm_timers[i].name);
#endif
    if (err) {
        fprintf (stderr, "Unable to find any suitable alarm timer.\n");
        fprintf (stderr, "Terminating\n");
        exit (1);
    }
    alarm_timer = t;
}
