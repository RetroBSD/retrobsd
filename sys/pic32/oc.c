/*
 * Output Compare driver for PIC32.
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
#include "param.h"
#include "conf.h"
#include "user.h"
#include "ioctl.h"
#include "systm.h"
#include "sys/uio.h"
#include "oc.h"
#include "debug.h"

const struct devspec ocdevs[] = {
    { 0, "oc1" },
    { 1, "oc2" },
    { 2, "oc3" },
    { 3, "oc4" },
    { 4, "oc5" },
    { 0, 0 }
};

#define _BC(R,B) (R &= ~(1<<B))
#define _BS(R,B) (R |= (1<<B))

struct oc_state state[OC_MAX_DEV];

/*
 * Devices:
 *      /dev/ocX 
 *
 * Write to the device outputs to LCD memory as data.  Use ioctl() to send
 * comands:
 *
 *      ioctl(fd, LCD_RESET, 0)  - reset the LCD
 *      ioctl(fd, LCD_SET_PAGE, page)  - set the page address
 *      ioctl(fd, LCD_SET_Y, y)  - set the page offset
 *
 */

int oc_set_mode(int unit, int mode)
{
    switch(mode)
    {
        case OC_MODE_PWM:
            _BS(T2CON,15);  // ON
            _BC(T2CON,6);  
            _BC(T2CON,5);    // >- 1:1 prescale
            _BC(T2CON,4);  
            _BC(T2CON,3);   // 16 bit timer
            _BC(T2CON,1);   // Internal clock source
            PR2 = 0xFFFF;
            switch(unit)
            {
                case 0:
                    _BS(OC1CON,15); // ON
                    _BC(OC1CON,5);  // 16 bit
                    _BC(OC1CON,3);  // TMR2
                    _BS(OC1CON,2);  // 
                    _BS(OC1CON,1);  //  >- PWM Mode, no fault pin
                    _BC(OC1CON,0);  // 
                    state[0].mode = OC_MODE_PWM;
                    break;
                case 1:
                    _BS(OC2CON,15); // ON
                    _BC(OC2CON,5);  // 16 bit
                    _BC(OC2CON,3);  // TMR2
                    _BS(OC2CON,2);  // 
                    _BS(OC2CON,1);  //  >- PWM Mode, no fault pin
                    _BC(OC2CON,0);  //
                    state[1].mode = OC_MODE_PWM;
                    break;
                case 2:
                    _BS(OC3CON,15); // ON
                    _BC(OC3CON,5);  // 16 bit
                    _BC(OC3CON,3);  // TMR2
                    _BS(OC3CON,2);  // 
                    _BS(OC3CON,1);  //  >- PWM Mode, no fault pin
                    _BC(OC3CON,0);  //
                    state[2].mode = OC_MODE_PWM;
                    break;
                case 3:
                    _BS(OC4CON,15); // ON
                    _BC(OC4CON,5);  // 16 bit
                    _BC(OC4CON,3);  // TMR2
                    _BS(OC4CON,2);  // 
                    _BS(OC4CON,1);  //  >- PWM Mode, no fault pin
                    _BC(OC4CON,0);  //
                    state[3].mode = OC_MODE_PWM;
                    break;
                case 4:
                    _BS(OC5CON,15); // ON
                    _BC(OC5CON,5);  // 16 bit
                    _BC(OC5CON,3);  // TMR2
                    _BS(OC5CON,2);  // 
                    _BS(OC5CON,1);  //  >- PWM Mode, no fault pin
                    _BC(OC5CON,0);  //
                    state[4].mode = OC_MODE_PWM;
                    break;
                default:
                    return EINVAL;
            }
	    DEBUG("oc%d: Mode set to PWM\n",unit);
            break;
        default:
            return EINVAL;
    }
    return 0;
}

int oc_pwm_duty(int unit, unsigned int duty)
{
    if(state[unit].mode!=OC_MODE_PWM)
        return EINVAL;

    switch(unit)
    {
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
    DEBUG("oc%d: Duty set to %d\n",unit,duty);
    return 0;
}

int
oc_open (dev, flag, mode)
	dev_t dev;
{
	int unit = minor(dev);

	if (unit >= OC_MAX_DEV)
		return ENXIO;
	if (u.u_uid != 0)
		return EPERM;
	DEBUG("oc%d: Opened\n",unit);
	return 0;
}

int
oc_close (dev, flag, mode)
	dev_t dev;
{
	return 0;
}

int
oc_read (dev, uio, flag)
	dev_t dev;
	struct uio *uio;
	int flag;
{
        // TODO
	return ENODEV;
}

int oc_write (dev_t dev, struct uio *uio, int flag)
{
    return ENODEV;
}

int
oc_ioctl (dev, cmd, addr, flag)
	dev_t dev;
	register u_int cmd;
	caddr_t addr;
{
        int unit;
        int *val;
        val = (int *)addr;

        unit = minor(dev);

        if(unit >= OC_MAX_DEV)
            return ENODEV;

	if (cmd == OC_SET_MODE) {
            return oc_set_mode(unit, *val);
        }

	if (cmd == OC_PWM_DUTY) {
            return oc_pwm_duty(unit, (unsigned int) *val);
        }

	return 0;
}
