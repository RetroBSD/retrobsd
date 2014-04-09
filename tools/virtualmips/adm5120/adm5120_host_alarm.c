 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *
  * This file is part of the virtualmips distribution.
  * See LICENSE file for terms of the license.
  *
  */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<string.h>

#include "device.h"
#include "adm5120.h"
#include "mips.h"

#define ADM_FREQ  175000000     /*175MHZ */

int instructions = 0;
extern m_uint32_t sw_table[SW_INDEX_MAX];
#define MAX_INSTRUCTIONS  1
int timeout = 0;
m_uint32_t time_reload;
extern m_uint32_t uart_table[2][UART_INDEX_MAX];
extern cpu_mips_t *current_cpu;

void uart_set_interrupt (cpu_mips_t * cpu, int channel);
/*ADM5120 use host_alarm_handler to process all the things,
This method is deprecated.
JZ4740 uses a timer method, which is more flexible.
See jz4740_host_alam.c */

void host_alarm_handler (int host_signum)
{

    m_uint32_t tim;
    if (unlikely (current_cpu->state != CPU_STATE_RUNNING))
        return;
    if ((uart_table[0][UART_CR_REG / 4] & UART_RX_INT_EN)
        && (uart_table[0][UART_CR_REG / 4] & UART_PORT_EN)) {
        if (vtty_is_char_avail (current_cpu->vm->vtty_con1)) {
            uart_set_interrupt (current_cpu, 0);
            uart_table[0][UART_ICR_REG / 4] |= UART_RX_INT;
            return;
        }

    }

    if (uart_table[0][UART_CR_REG / 4] & UART_PORT_EN) {

        if (uart_table[0][UART_CR_REG / 4] & UART_TX_INT_EN) {
            uart_table[0][UART_ICR_REG / 4] |= UART_TX_INT;
            uart_set_interrupt (current_cpu, 0);
            return;
        }

    }

    /*check count and compare */
    /*Why 2*1000? CPU is 175MHZ, we assume CPI(cycle per instruction)=2
     * see arch/mips/adm5120/setup.c for more information
     * 49 void __init mips_time_init(void)
     */
    current_cpu->cp0.reg[MIPS_CP0_COUNT] += ADM_FREQ / (2 * 1000);
    if (current_cpu->cp0.reg[MIPS_CP0_COMPARE] != 0) {
        if (current_cpu->cp0.reg[MIPS_CP0_COUNT] >=
            current_cpu->cp0.reg[MIPS_CP0_COMPARE]) {
            mips_set_irq (current_cpu, MIPS_TIMER_INTERRUPT);
            mips_update_irq_flag (current_cpu);
        }
    }

    /*Linux kernel does not use this timer. It use mips count */
    if (sw_table[Timer_REG / 4] & SW_TIMER_EN) {
        tim = sw_table[Timer_REG / 4] & SW_TIMER_MASK;
        if (tim == 0) {
            tim = time_reload;
            timeout = 1;
        } else
            tim -= 0x2000;      /*1ms=2000*640ns.but 2000 is too slow. I set it to 0x2000 */
        if ((m_int32_t) tim < 0x2000)
            tim = 0;
        sw_table[Timer_REG / 4] &= ~SW_TIMER_MASK;
        sw_table[Timer_REG / 4] += tim;

    }

}
