/*
 * Pulse Width Modulation driver for PIC32.
 * Using Output Compare peripherals.
 *
 * Devices:
 *      /dev/pwmX
 *
 * Copyright (C) 2012 Majenko Technologies <matt@majenko.co.uk>
 *
 * Permission to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice appear in all
 * copies and that both that the copyright notice and this
 * permission notice and warranty disclaimer appear in supporting
 * documentation, and that the name of the author not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 *
 * The author disclaim all warranties with regard to this
 * software, including all implied warranties of merchantability
 * and fitness.  In no event shall the author be liable for any
 * special, indirect or consequential damages or any damages
 * whatsoever resulting from loss of use, data or profits, whether
 * in an action of contract, negligence or other tortious action,
 * arising out of or in connection with the use or performance of
 * this software.
 */
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/ioctl.h>
#include <sys/systm.h>
#include <sys/uio.h>
#include <sys/pwm.h>
#include <machine/debug.h>
#include <sys/kconfig.h>

#define _BC(R,B) (R &= ~(1<<B))
#define _BS(R,B) (R |= (1<<B))

struct pwm_state state[PWM_MAX_DEV];

int pwm_set_mode(int unit, int mode)
{
    switch (mode) {
    case PWM_MODE_PWM:
        _BS(T2CON,15);      // ON
        _BC(T2CON,6);
        _BC(T2CON,5);       // >- 1:1 prescale
        _BC(T2CON,4);
        _BC(T2CON,3);       // 16 bit timer
        _BC(T2CON,1);       // Internal clock source
        PR2 = 0xFFFF;
        switch (unit) {
        case 0:
            _BS(OC1CON,15); // ON
            _BC(OC1CON,5);  // 16 bit
            _BC(OC1CON,3);  // TMR2
            _BS(OC1CON,2);  //
            _BS(OC1CON,1);  //  >- PWM Mode, no fault pin
            _BC(OC1CON,0);  //
            state[0].mode = PWM_MODE_PWM;
            break;
        case 1:
            _BS(OC2CON,15); // ON
            _BC(OC2CON,5);  // 16 bit
            _BC(OC2CON,3);  // TMR2
            _BS(OC2CON,2);  //
            _BS(OC2CON,1);  //  >- PWM Mode, no fault pin
            _BC(OC2CON,0);  //
            state[1].mode = PWM_MODE_PWM;
            break;
        case 2:
            _BS(OC3CON,15); // ON
            _BC(OC3CON,5);  // 16 bit
            _BC(OC3CON,3);  // TMR2
            _BS(OC3CON,2);  //
            _BS(OC3CON,1);  //  >- PWM Mode, no fault pin
            _BC(OC3CON,0);  //
            state[2].mode = PWM_MODE_PWM;
            break;
        case 3:
            _BS(OC4CON,15); // ON
            _BC(OC4CON,5);  // 16 bit
            _BC(OC4CON,3);  // TMR2
            _BS(OC4CON,2);  //
            _BS(OC4CON,1);  //  >- PWM Mode, no fault pin
            _BC(OC4CON,0);  //
            state[3].mode = PWM_MODE_PWM;
            break;
        case 4:
            _BS(OC5CON,15); // ON
            _BC(OC5CON,5);  // 16 bit
            _BC(OC5CON,3);  // TMR2
            _BS(OC5CON,2);  //
            _BS(OC5CON,1);  //  >- PWM Mode, no fault pin
            _BC(OC5CON,0);  //
            state[4].mode = PWM_MODE_PWM;
            break;
        default:
            return EINVAL;
        }
        DEBUG("pwm%d: Mode set to PWM\n",unit);
        break;
    default:
        return EINVAL;
    }
    return 0;
}

int pwm_duty(int unit, unsigned int duty)
{
    if (state[unit].mode != PWM_MODE_PWM)
        return EINVAL;

    switch (unit) {
    case 0:
        OC1RS = duty;
        break;
    case 1:
        OC2RS = duty;
        break;
    case 2:
        OC3RS = duty;
        break;
    case 3:
        OC4RS = duty;
        break;
    case 4:
        OC5RS = duty;
        break;
    default:
        return EINVAL;
    }
    DEBUG("pwm%d: Duty set to %d\n",unit,duty);
    return 0;
}

int
pwm_open (dev, flag, mode)
    dev_t dev;
{
    int unit = minor(dev);

    if (unit >= PWM_MAX_DEV)
        return ENXIO;
    if (u.u_uid != 0)
        return EPERM;
    DEBUG("pwm%d: Opened\n",unit);
    return 0;
}

int
pwm_close (dev, flag, mode)
    dev_t dev;
{
    return 0;
}

int
pwm_read (dev, uio, flag)
    dev_t dev;
    struct uio *uio;
    int flag;
{
    // TODO
    return ENODEV;
}

int pwm_write (dev_t dev, struct uio *uio, int flag)
{
    return ENODEV;
}

int
pwm_ioctl (dev, cmd, addr, flag)
    dev_t dev;
    register u_int cmd;
    caddr_t addr;
{
    int unit;
    int *val;
    val = (int *)addr;

    unit = minor(dev);

    if (unit >= PWM_MAX_DEV)
        return ENODEV;

    if (cmd == PWM_SET_MODE) {
        return pwm_set_mode(unit, *val);
    }

    if (cmd == PWM_DUTY) {
        return pwm_duty(unit, (unsigned int) *val);
    }
    return 0;
}

/*
 * Test to see if device is present.
 * Return true if found and initialized ok.
 */
static int
pwmprobe(config)
    struct conf_device *config;
{
    printf("pwm: %u channels\n", PWM_MAX_DEV);
    return 1;
}

struct driver pwmdriver = {
    "pwm", pwmprobe,
};
