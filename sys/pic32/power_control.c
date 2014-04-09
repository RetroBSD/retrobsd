#include "param.h"
#include "conf.h"
#include "user.h"
#include "systm.h"
#include "machparam.h"
#include "reboot.h"

volatile unsigned int psCounter;
int countdown;

#define COUNTDOWN 5

void power_off()
{
    TRIS_CLR(POWER_CONTROL_PORT) = 1<<POWER_CONTROL_PIN;
    LAT_SET(POWER_CONTROL_PORT) = 1<<POWER_CONTROL_PIN;
}

void power_switch_check()
{
    if(PORT_VAL(POWER_SWITCH_PORT) & (1<<POWER_SWITCH_PIN))
    {
        countdown=COUNTDOWN;
        psCounter=0;
        if(psCounter>0)
        {
            LAT_SET(POWER_LED_PORT) = 1<<POWER_LED_PIN;
            printf("power: switch released - power down aborted\n");
        }
    } else {
        psCounter++;
    }

    if(psCounter==20)
    {
        if(countdown==0)
        {
            printf("power: powering off\n");
            boot(0,RB_HALT | RB_POWEROFF);
        }

        psCounter = 0;
        printf("power: powering down in %d\n",countdown);
        countdown--;

#ifdef POWER_LED_PORT
        if((countdown%2)==0)
        {
            LAT_SET(POWER_LED_PORT) = 1<<POWER_LED_PIN;
        } else {
            LAT_CLR(POWER_LED_PORT) = 1<<POWER_LED_PIN;
        }
#endif
    }
}

void power_init()
{
    psCounter = 0;
    countdown = COUNTDOWN;
#ifdef POWER_CONTROL_PORT
    TRIS_CLR(POWER_CONTROL_PORT) = 1<<POWER_CONTROL_PIN;
    LAT_CLR(POWER_CONTROL_PORT) = 1<<POWER_CONTROL_PIN;
#endif

#ifdef POWER_SWITCH_PORT
    TRIS_SET(POWER_SWITCH_PORT) = 1<<POWER_SWITCH_PIN;
#endif

#ifdef POWER_LED_PORT
    TRIS_CLR(POWER_LED_PORT) = 1<<POWER_LED_PIN;
    LAT_SET(POWER_LED_PORT) = 1<<POWER_LED_PIN;
#endif
}
