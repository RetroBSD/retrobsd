/*
 * ADC driver for PIC32.
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
#include "uio.h"
#include "adc.h"
#include "debug.h"

const struct devspec adcdevs[] = {
    { 0, "adc0" }, { 1, "adc1" }, { 2, "adc2" }, { 3, "adc3" },
    { 4, "adc4" }, { 5, "adc5" }, { 6, "adc6" }, { 7, "adc7" },
    { 8, "adc8" }, { 9, "adc9" }, { 10, "adc10" }, { 11, "adc11" },
    { 12, "adc12" }, { 13, "adc13" }, { 14, "adc14" }, { 15, "adc15" },
    { 0, 0 }
};

extern int uwritec(struct uio *);

unsigned short adcactive = 0;

int adc_open(dev_t dev, int flag, int mode)
{
    int channel;

    channel = minor(dev);
    if(channel>ADCMAX)
        return ENODEV;

    DEBUG1("adc%2: opened\n",channel);

    AD1PCFG &= ~(1<<channel);
    if(adcactive==0)
    {
        // Enable and configure the ADC here
        AD1CSSL = 0xFFFF;
        AD1CON2 = 0b0000010000111100;
        AD1CON3 = 0b0000111100001111;
        AD1CON1 = 0b1000000011100110;
        // IPC(6) = 0x04040404;
    }

    adcactive |= (1<<channel);
    return 0;
}

int adc_close(dev_t dev, int flag, int mode)
{
    int channel;

    channel = minor(dev);
    if(channel>ADCMAX)
        return ENODEV;

    AD1PCFG |= (1<<channel);
    adcactive &= ~(1<<channel);
    if(adcactive==0)
    {
        // Switch off the ADC here.
        AD1CSSL = 0x0000;
        AD1CON1 = 0x0000;
        asm volatile("NOP");
        IECCLR(1) = 1<<(PIC32_IRQ_AD1-32);
    }
    return 0;
}

// Return the most recent ADC value
int adc_read(dev_t dev, struct uio *uio, int flag)
{
    int channel;
    char temp[6];
    int c;
    unsigned int lr;
    int tv;

    channel = minor(dev);
    if(channel>ADCMAX)
        return ENODEV;

    lr = *(&ADC1BUF0+(channel<<2));

    c=0;
    if(lr >= 1000)
    {
        tv = lr/1000;
        temp[c++] = '0' + tv;
        lr = lr - (tv*1000);
    }
    if((lr >= 100) || (c>0))
    {
        tv = lr/100;
        temp[c++] = '0' + tv;
        lr = lr - (tv*100);
    }
    if((lr >= 10) || (c>0))
    {
        tv = lr/10;
        temp[c++] = '0' + tv;
        lr = lr - (tv*10);
    }
    temp[c++] = '0' + lr;
    temp[c++] = '\n';
    temp[c] = 0;

    uiomove(temp,strlen(temp), uio);

    return 0;
}

int adc_write(dev_t dev, struct uio *uio, int flag)
{
    return EINVAL;
}

int adc_ioctl(dev_t dev, register u_int cmd, caddr_t addr, int flag)
{
    switch(cmd)
    {
        default:
            return EINVAL;
    }

    return 0;
}
