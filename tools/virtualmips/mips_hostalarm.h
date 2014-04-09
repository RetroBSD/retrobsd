#ifndef __MIPS64_HOSTALARM_H__
#define __MIPS64_HOSTALARM_H__

struct qemu_alarm_timer {
    char const *name;
    unsigned int flags;

    int (*start) (struct qemu_alarm_timer * t);
    void (*stop) (struct qemu_alarm_timer * t);
    void (*rearm) (struct qemu_alarm_timer * t);
    void *priv;
};

void mips_init_host_alarm (void);
void host_alarm (cpu_mips_t *cpu, int nclocks);

#endif
