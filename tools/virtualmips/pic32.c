/*
 * PIC32 emulation.
 *
 * Copyright (C) 2011 Serge Vakulenko <serge@vak.ru>
 *
 * This file is part of the virtualmips distribution.
 * See LICENSE file for terms of the license.
 */
#define _GNU_SOURCE
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include "vp_lock.h"
#include "utils.h"
#include "mips.h"
#include "vm.h"
#include "cpu.h"
#include "mips_exec.h"
#include "debug.h"

#include "pic32.h"
#include "device.h"
#include "mips_jit.h"

/*
 * Translate IRQ number to interrupt vector.
 */
static const int irq_to_vector[] = {
    PIC32_VECT_CT,      /* 0  - Core Timer Interrupt */
    PIC32_VECT_CS0,     /* 1  - Core Software Interrupt 0 */
    PIC32_VECT_CS1,     /* 2  - Core Software Interrupt 1 */
    PIC32_VECT_INT0,    /* 3  - External Interrupt 0 */
    PIC32_VECT_T1,      /* 4  - Timer1 */
    PIC32_VECT_IC1,     /* 5  - Input Capture 1 */
    PIC32_VECT_OC1,     /* 6  - Output Compare 1 */
    PIC32_VECT_INT1,    /* 7  - External Interrupt 1 */
    PIC32_VECT_T2,      /* 8  - Timer2 */
    PIC32_VECT_IC2,     /* 9  - Input Capture 2 */
    PIC32_VECT_OC2,     /* 10 - Output Compare 2 */
    PIC32_VECT_INT2,    /* 11 - External Interrupt 2 */
    PIC32_VECT_T3,      /* 12 - Timer3 */
    PIC32_VECT_IC3,     /* 13 - Input Capture 3 */
    PIC32_VECT_OC3,     /* 14 - Output Compare 3 */
    PIC32_VECT_INT3,    /* 15 - External Interrupt 3 */
    PIC32_VECT_T4,      /* 16 - Timer4 */
    PIC32_VECT_IC4,     /* 17 - Input Capture 4 */
    PIC32_VECT_OC4,     /* 18 - Output Compare 4 */
    PIC32_VECT_INT4,    /* 19 - External Interrupt 4 */
    PIC32_VECT_T5,      /* 20 - Timer5 */
    PIC32_VECT_IC5,     /* 21 - Input Capture 5 */
    PIC32_VECT_OC5,     /* 22 - Output Compare 5 */
    PIC32_VECT_SPI1,    /* 23 - SPI1 Fault */
    PIC32_VECT_SPI1,    /* 24 - SPI1 Transfer Done */
    PIC32_VECT_SPI1,    /* 25 - SPI1 Receive Done */

    PIC32_VECT_U1     | /* 26 - UART1 Error */
    PIC32_VECT_SPI3   | /* 26 - SPI3 Fault */
    PIC32_VECT_I2C3,    /* 26 - I2C3 Bus Collision Event */

    PIC32_VECT_U1     | /* 27 - UART1 Receiver */
    PIC32_VECT_SPI3   | /* 27 - SPI3 Transfer Done */
    PIC32_VECT_I2C3,    /* 27 - I2C3 Slave Event */

    PIC32_VECT_U1     | /* 28 - UART1 Transmitter */
    PIC32_VECT_SPI3   | /* 28 - SPI3 Receive Done */
    PIC32_VECT_I2C3,    /* 28 - I2C3 Master Event */

    PIC32_VECT_I2C1,    /* 29 - I2C1 Bus Collision Event */
    PIC32_VECT_I2C1,    /* 30 - I2C1 Slave Event */
    PIC32_VECT_I2C1,    /* 31 - I2C1 Master Event */
    PIC32_VECT_CN,      /* 32 - Input Change Interrupt */
    PIC32_VECT_AD1,     /* 33 - ADC1 Convert Done */
    PIC32_VECT_PMP,     /* 34 - Parallel Master Port */
    PIC32_VECT_CMP1,    /* 35 - Comparator Interrupt */
    PIC32_VECT_CMP2,    /* 36 - Comparator Interrupt */

    PIC32_VECT_U3     | /* 37 - UART3 Error */
    PIC32_VECT_SPI2   | /* 37 - SPI2 Fault */
    PIC32_VECT_I2C4,    /* 37 - I2C4 Bus Collision Event */

    PIC32_VECT_U3     | /* 38 - UART3 Receiver */
    PIC32_VECT_SPI2   | /* 38 - SPI2 Transfer Done */
    PIC32_VECT_I2C4,    /* 38 - I2C4 Slave Event */

    PIC32_VECT_U3     | /* 39 - UART3 Transmitter */
    PIC32_VECT_SPI2   | /* 39 - SPI2 Receive Done */
    PIC32_VECT_I2C4,    /* 39 - I2C4 Master Event */

    PIC32_VECT_U2     | /* 40 - UART2 Error */
    PIC32_VECT_SPI4   | /* 40 - SPI4 Fault */
    PIC32_VECT_I2C5,    /* 40 - I2C5 Bus Collision Event */

    PIC32_VECT_U2     | /* 41 - UART2 Receiver */
    PIC32_VECT_SPI4   | /* 41 - SPI4 Transfer Done */
    PIC32_VECT_I2C5,    /* 41 - I2C5 Slave Event */

    PIC32_VECT_U2     | /* 42 - UART2 Transmitter */
    PIC32_VECT_SPI4   | /* 42 - SPI4 Receive Done */
    PIC32_VECT_I2C5,    /* 42 - I2C5 Master Event */

    PIC32_VECT_I2C2,    /* 43 - I2C2 Bus Collision Event */
    PIC32_VECT_I2C2,    /* 44 - I2C2 Slave Event */
    PIC32_VECT_I2C2,    /* 45 - I2C2 Master Event */
    PIC32_VECT_FSCM,    /* 46 - Fail-Safe Clock Monitor */
    PIC32_VECT_RTCC,    /* 47 - Real-Time Clock and Calendar */
    PIC32_VECT_DMA0,    /* 48 - DMA Channel 0 */
    PIC32_VECT_DMA1,    /* 49 - DMA Channel 1 */
    PIC32_VECT_DMA2,    /* 50 - DMA Channel 2 */
    PIC32_VECT_DMA3,    /* 51 - DMA Channel 3 */
    PIC32_VECT_DMA4,    /* 52 - DMA Channel 4 */
    PIC32_VECT_DMA5,    /* 53 - DMA Channel 5 */
    PIC32_VECT_DMA6,    /* 54 - DMA Channel 6 */
    PIC32_VECT_DMA7,    /* 55 - DMA Channel 7 */
    PIC32_VECT_FCE,     /* 56 - Flash Control Event */
    PIC32_VECT_USB,     /* 57 - USB */
    PIC32_VECT_CAN1,    /* 58 - Control Area Network 1 */
    PIC32_VECT_CAN2,    /* 59 - Control Area Network 2 */
    PIC32_VECT_ETH,     /* 60 - Ethernet Interrupt */
    PIC32_VECT_IC1,     /* 61 - Input Capture 1 Error */
    PIC32_VECT_IC2,     /* 62 - Input Capture 2 Error */
    PIC32_VECT_IC3,     /* 63 - Input Capture 3 Error */
    PIC32_VECT_IC4,     /* 64 - Input Capture 4 Error */
    PIC32_VECT_IC5,     /* 65 - Input Capture 5 Error */
    PIC32_VECT_PMP,     /* 66 - Parallel Master Port Error */
    PIC32_VECT_U4,      /* 67 - UART4 Error */
    PIC32_VECT_U4,      /* 68 - UART4 Receiver */
    PIC32_VECT_U4,      /* 69 - UART4 Transmitter */
    PIC32_VECT_U6,      /* 70 - UART6 Error */
    PIC32_VECT_U6,      /* 71 - UART6 Receiver */
    PIC32_VECT_U6,      /* 72 - UART6 Transmitter */
    PIC32_VECT_U5,      /* 73 - UART5 Error */
    PIC32_VECT_U5,      /* 74 - UART5 Receiver */
    PIC32_VECT_U5,      /* 75 - UART5 Transmitter */
};

/* Initialize the PIC32 Platform (MIPS) */
static int pic32_init_platform (pic32_t *pic32)
{
    struct vm_instance *vm = pic32->vm;
    cpu_mips_t *cpu0;
    void *(*cpu_run_fn) (void *);

    vm_init_vtty (vm);

    /* Create a CPU group */
    vm->cpu_group = cpu_group_create ("System CPU");

    /* Initialize the virtual MIPS processor */
    cpu0 = cpu_create (vm, CPU_TYPE_MIPS32, 0);
    if (! cpu0) {
        vm_error (vm, "unable to create CPU0!\n");
        return (-1);
    }
    /* Add this CPU to the system CPU group */
    cpu_group_add (vm->cpu_group, cpu0);
    vm->boot_cpu = cpu0;

    /* create the CPU thread execution */
    cpu_run_fn = (void *) mips_exec_run_cpu;
    if (pthread_create (&cpu0->cpu_thread, NULL, cpu_run_fn, cpu0) != 0) {
        fprintf (stderr, "cpu_create: unable to create thread for CPU%u\n",
            0);
        free (cpu0);
        return (-1);
    }
    /* 32-bit address */
    cpu0->addr_bus_mask = 0xffffffff;

    /* Initialize RAM */
    vm_ram_init (vm, 0x00000000ULL);

    /* Initialize two flash areas */
    if (vm->flash_size != 0)
        if (dev_pic32_flash_init (vm, "Program flash", vm->flash_size,
                vm->flash_address, vm->flash_filename) == -1)
            return (-1);
    if (pic32->boot_flash_size != 0)
        if (dev_pic32_flash_init (vm, "Boot flash", pic32->boot_flash_size,
                pic32->boot_flash_address, pic32->boot_file_name) == -1)
            return (-1);

    /* Initialize peripherals */
    if (dev_pic32_uart_init (vm, "UART1", PIC32_U1MODE,
            PIC32_IRQ_U1E, vm->vtty_con[0]) == -1)
        return (-1);
    if (dev_pic32_uart_init (vm, "UART2", PIC32_U2MODE,
            PIC32_IRQ_U2E, vm->vtty_con[1]) == -1)
        return (-1);
#if NVTTY >= 3
    if (dev_pic32_uart_init (vm, "UART3", PIC32_U3MODE,
            PIC32_IRQ_U3E, vm->vtty_con[2]) == -1)
        return (-1);
#endif
#if NVTTY >= 4
    if (dev_pic32_uart_init (vm, "UART4", PIC32_U4MODE,
            PIC32_IRQ_U4E, vm->vtty_con[3]) == -1)
        return (-1);
#endif
#if NVTTY >= 5
    if (dev_pic32_uart_init (vm, "UART5", PIC32_U5MODE,
            PIC32_IRQ_U5E, vm->vtty_con[4]) == -1)
        return (-1);
#endif
#if NVTTY >= 6
    if (dev_pic32_uart_init (vm, "UART6", PIC32_U6MODE,
            PIC32_IRQ_U6E, vm->vtty_con[5]) == -1)
        return (-1);
#endif
    if (dev_pic32_intcon_init (vm, "INTCON", PIC32_INTCON) == -1)
        return (-1);
    if (dev_pic32_dmacon_init (vm, "DMACON", PIC32_DMACON) == -1)
        return (-1);
    if (dev_pic32_syscon_init (vm, "SYSCON", PIC32_OSCCON) == -1)
        return (-1);
    if (dev_pic32_adc_init (vm, "ADC", PIC32_AD1CON1) == -1)
        return (-1);
    if (dev_pic32_prefetch_init (vm, "Prefetch", PIC32_CHECON) == -1)
        return (-1);
    if (dev_pic32_bmxcon_init (vm, "BMX", PIC32_BMXCON) == -1)
        return (-1);
    if (dev_pic32_rtcc_init (vm, "RTCC", PIC32_RTCCON) == -1)
        return (-1);
    if (dev_pic32_gpio_init (vm, "GPIO", PIC32_TRISA) == -1)
        return (-1);
    if (dev_pic32_spi_init (vm, "SPI1", PIC32_SPI1CON,
            PIC32_IRQ_SPI1E) == -1)
        return (-1);
    if (dev_pic32_spi_init (vm, "SPI2", PIC32_SPI2CON,
            PIC32_IRQ_SPI2E) == -1)
        return (-1);
    if (dev_pic32_spi_init (vm, "SPI3", PIC32_SPI3CON,
            PIC32_IRQ_SPI3E) == -1)
        return (-1);
    if (dev_pic32_spi_init (vm, "SPI4", PIC32_SPI4CON,
            PIC32_IRQ_SPI4E) == -1)
        return (-1);
    pic32->timer1 = dev_pic32_timer_init (vm, "Timer1", PIC32_T1CON, PIC32_IRQ_T1);
    pic32->timer2 = dev_pic32_timer_init (vm, "Timer2", PIC32_T2CON, PIC32_IRQ_T2);
    pic32->timer3 = dev_pic32_timer_init (vm, "Timer3", PIC32_T3CON, PIC32_IRQ_T3);
    pic32->timer4 = dev_pic32_timer_init (vm, "Timer4", PIC32_T4CON, PIC32_IRQ_T4);
    pic32->timer5 = dev_pic32_timer_init (vm, "Timer5", PIC32_T5CON, PIC32_IRQ_T5);
    if (! pic32->timer1 || ! pic32->timer2 || ! pic32->timer3 ||
        ! pic32->timer4 || ! pic32->timer5)
        return (-1);
    if (dev_sdcard_init (&pic32->sdcard[0], "SD0", pic32->sdcard0_size,
            pic32->sdcard0_file_name) < 0)
        return (-1);
    if (dev_sdcard_init (&pic32->sdcard[1], "SD1", pic32->sdcard1_size,
            pic32->sdcard1_file_name) < 0)
        return (-1);
    if (dev_swap_init (&pic32->swap, "Swap") < 0)
        return (-1);

    /* Initialize DEVCFG area */
    if (dev_pic32_devcfg_init (vm, "DEVCFG", 0x1fc02f00) < 0)
        return (-1);

    pic32->sdcard[1].unit = 1;
    return (0);
}

/*
 * Find pending interrupt with the biggest priority.
 * Setup INTSTAT and cause registers.
 * Update irq_pending flag for CPU.
 */
void pic32_update_irq_flag (pic32_t *pic32)
{
    cpu_mips_t *cpu = pic32->vm->boot_cpu;
    int vector, level, irq, n, v;

    /* Assume no interrupts pending. */
    cpu->irq_cause = 0;
    cpu->irq_pending = 0;
    pic32->intstat = 0;

    if ((pic32->ifs[0] & pic32->iec[0]) ||
        (pic32->ifs[1] & pic32->iec[1]) ||
        (pic32->ifs[2] & pic32->iec[2]))
    {
        /* Find the most prioritive pending interrupt,
         * it's vector and level. */
        vector = 0;
        level = 0;
        for (irq=0; irq<sizeof(irq_to_vector)/sizeof(int); irq++) {
            n = irq >> 5;
            if ((pic32->ifs[n] & pic32->iec[n]) >> (irq & 31) & 1) {
                /* Interrupt is pending. */
                v = irq_to_vector [irq];
                if (v < 0)
                    continue;
                if (pic32->ivprio[v] > level) {
                    vector = v;
                    level = pic32->ivprio[v];
                }
            }
        }
        pic32->intstat = vector | (level << 8);

        cpu->irq_cause = level << 10;
/*printf ("-- vector = %d, level = %d\n", vector, level);*/
    }
/*else printf ("-- no irq pending\n");*/

    mips_update_irq_flag (cpu);
}

void pic32_clear_irq (vm_instance_t *vm, u_int irq)
{
    pic32_t *pic32 = (pic32_t*) vm->hw_data;

    /* Clear interrupt flag status */
    pic32->ifs [irq >> 5] &= ~(1 << (irq & 31));

    pic32_update_irq_flag (pic32);
}

void pic32_set_irq (vm_instance_t *vm, u_int irq)
{
    pic32_t *pic32 = (pic32_t*) vm->hw_data;

    /* Set interrupt flag status */
    pic32->ifs [irq >> 5] |= 1 << (irq & 31);

    pic32_update_irq_flag (pic32);
}

/*
 * Activate core timer interrupt
 */
void set_timer_irq (cpu_mips_t *cpu)
{
    pic32_set_irq (cpu->vm, PIC32_VECT_CT);
}

/*
 * Clear core timer interrupt
 */
void clear_timer_irq (cpu_mips_t *cpu)
{
    pic32_clear_irq (cpu->vm, PIC32_VECT_CT);
}

/*
 * Increment timers.
 */
void host_alarm (cpu_mips_t *cpu, int nclocks)
{
    pic32_t *pic32 = (pic32_t*) cpu->vm->hw_data;

    dev_pic32_timer_tick (cpu, pic32->timer1, nclocks);
    dev_pic32_timer_tick (cpu, pic32->timer2, nclocks);
    dev_pic32_timer_tick (cpu, pic32->timer3, nclocks);
    dev_pic32_timer_tick (cpu, pic32->timer4, nclocks);
    dev_pic32_timer_tick (cpu, pic32->timer5, nclocks);
}

/*
 * Configuration information.
 */
static char *boot_method_string[2] = {"Binary", "ELF"};
static char *boot_from_string[2] = {"NOR FLASH", "NAND FLASH"};
static char *flash_type_string[2] = {"NOR FLASH", "NAND FLASH"};

static char *start_address = 0;
static char *trace_address = 0;
static char *uart_type[NVTTY] = {0};
static char *uart_port[NVTTY] = {0};

static void configure_parameter (void *arg, char *section, char *param, char *value)
{
    pic32_t *pic32 = arg;
    vm_instance_t *vm = pic32->vm;

    if (strcmp (param, "ram_size") == 0)
        vm->ram_size = strtoul (value, 0, 0);
    else if (strcmp (param, "gdb_debug") == 0)
        vm->gdb_debug = strtoul (value, 0, 0);
    else if (strcmp (param, "gdb_port") == 0)
        vm->gdb_port = strtoul (value, 0, 0);
    else if (strcmp (param, "flash_size") == 0)
        vm->flash_size = strtoul (value, 0, 0);
    else if (strcmp (param, "flash_type") == 0)
        vm->flash_type = strtoul (value, 0, 0);
    else if (strcmp (param, "flash_file_name") == 0)
        vm->flash_filename = strdup (value);
    else if (strcmp (param, "flash_phy_address") == 0)
        vm->flash_address = strtoul (value, 0, 0);
    else if (strcmp (param, "boot_method") == 0)
        vm->boot_method = strtoul (value, 0, 0);
    else if (strcmp (param, "boot_from") == 0)
        vm->boot_from = strtoul (value, 0, 0);
    else if (strcmp (param, "kernel_file_name") == 0)
        vm->kernel_filename = strdup (value);
    else if (strcmp (param, "jit_use") == 0)
        vm->jit_use = strtoul (value, 0, 0);
    else if (strcmp (param, "debug_level") == 0)
        vm->debug_level = strtoul (value, 0, 0);
    else if (strcmp (param, "boot_flash_size") == 0)
        pic32->boot_flash_size = strtoul (value, 0, 0);
    else if (strcmp (param, "boot_flash_address")== 0)
        pic32->boot_flash_address = strtoul (value, 0, 0);
    else if (strcmp (param, "boot_file_name") == 0)
        pic32->boot_file_name = strdup (value);
    else if (strcmp (param, "start_address") == 0)
        start_address = strdup (value);
    else if (strcmp (param, "trace_address") == 0)
        trace_address = strdup (value);
    else if (strcmp (param, "sdcard_port") == 0)
        pic32->sdcard_port = strtoul (value, 0, 0);
    else if (strcmp (param, "sdcard0_size") == 0)
        pic32->sdcard0_size = strtoul (value, 0, 0);
    else if (strcmp (param, "sdcard1_size") == 0)
        pic32->sdcard1_size = strtoul (value, 0, 0);
    else if (strcmp (param, "sdcard0_file_name") == 0)
        pic32->sdcard0_file_name = strdup (value);
    else if (strcmp (param, "sdcard1_file_name") == 0)
        pic32->sdcard1_file_name = strdup (value);
    else if (strcmp (param, "uart1_type") == 0)
        uart_type[0] = strdup (value);
    else if (strcmp (param, "uart2_type") == 0)
        uart_type[1] = strdup (value);
    else if (strcmp (param, "uart3_type") == 0)
        uart_type[2] = strdup (value);
    else if (strcmp (param, "uart4_type") == 0)
        uart_type[3] = strdup (value);
    else if (strcmp (param, "uart5_type") == 0)
        uart_type[4] = strdup (value);
    else if (strcmp (param, "uart6_type") == 0)
        uart_type[5] = strdup (value);
    else if (strcmp (param, "uart1_port") == 0)
        uart_port[0] = strdup (value);
    else if (strcmp (param, "uart2_port") == 0)
        uart_port[1] = strdup (value);
    else if (strcmp (param, "uart3_port") == 0)
        uart_port[2] = strdup (value);
    else if (strcmp (param, "uart4_port") == 0)
        uart_port[3] = strdup (value);
    else if (strcmp (param, "uart5_port") == 0)
        uart_port[4] = strdup (value);
    else if (strcmp (param, "uart6_port") == 0)
        uart_port[5] = strdup (value);
    else {
        fprintf (stderr, "%s: unknown parameter `%s'\n", vm->configure_filename, param);
        exit (-1);
    }
    //printf ("Configure: %s = '%s'\n", param, value);
}

static void pic32_parse_configure (pic32_t *pic32)
{
    vm_instance_t *vm = pic32->vm;
    int i;

    conf_parse (vm->configure_filename, configure_parameter, pic32);
    if (start_address)
        pic32->start_address = strtoul (start_address, 0, 0);
    if (trace_address)
        vm->trace_address = strtoul (trace_address, 0, 0);
    for (i=0; i<NVTTY; i++)
        if (uart_type[i]) {
            if (strcmp (uart_type[i], "console") == 0)
                vm->vtty_type[i] = VTTY_TYPE_TERM;
            else if (strcmp (uart_type[i], "tcp") == 0)
                vm->vtty_type[i] = VTTY_TYPE_TCP;
            else if (strcmp (uart_type[i], "none") == 0)
                vm->vtty_type[i] = VTTY_TYPE_NONE;
            else {
                printf ("Unknown option: uart%d_type = %s\n",
                    i+1, uart_type[i]);
                continue;
            }
            if (uart_port[i]) {
                vm->vtty_tcp_port[i] = strtoul (uart_port[i], 0, 0);
            } else if (vm->vtty_type[i] == VTTY_TYPE_TCP) {
                vm->vtty_tcp_port[i] = 2300 + i;
            }
        }

    ASSERT(vm->ram_size != 0, "ram_size can not be 0\n");
    if (vm->flash_type == FLASH_TYPE_NOR_FLASH) {
        if (vm->flash_size != 0) {
            /*ASSERT(vm->flash_filename!=NULL, "flash_file_name can not be NULL\n");*/
            /* flash_filename can be null. virtualmips will create it. */
            ASSERT(vm->flash_address != 0, "flash_address can not be 0\n");
        }
    } else if (vm->flash_type == FLASH_TYPE_NAND_FLASH) {
        ASSERT(vm->flash_size == 0x400, "flash_size should be 0x400.\n We only support 1G byte NAND flash emulation\n");
        assert(1);
    } else
        ASSERT(0, "error flash_type. valid value: 1:NOR FLASH 2:NAND FLASH\n");

    ASSERT(vm->boot_method != 0, "boot_method can not be 0\n 1:binary  2:elf \n");

    if (vm->boot_method == BOOT_BINARY) {
        /* boot from binary image */
        ASSERT(vm->boot_from != 0, "boot_from can not be 0\n 1:NOR FLASH 2:NAND FLASH\n");
        if (vm->boot_from==BOOT_FROM_NOR_FLASH) {
            ASSERT(vm->flash_type == FLASH_TYPE_NOR_FLASH, "flash_type must be 1(NOR FLASH)\n");
            ASSERT(vm->flash_size != 0, "flash_size can not be 0\n");
            ASSERT(vm->flash_filename != NULL, "flash_filename can not be NULL\n");
            ASSERT(vm->flash_address != 0, "flash_address can not be 0\n");
        } else if (vm->boot_from == BOOT_FROM_NAND_FLASH) {
            ASSERT(vm->flash_type == FLASH_TYPE_NAND_FLASH, "flash_type must be 2(NAND FLASH)\n");
            ASSERT(vm->flash_size != 0, "flash_size can not be 0\n");
            /*ASSERT(vm->flash_filename!=NULL,"flash_filename can not be NULL\n");*/
        } else
            ASSERT(0, "error boot_from. valid value: 1:NOR FLASH 2:NAND FLASH\n");
    } else if (vm->boot_method == BOOT_ELF) {
        ASSERT(vm->kernel_filename!=0,"kernel_file_name can not be NULL\n ");
    } else
        ASSERT(0, "error boot_method. valid value: 1:binary  2:elf \n");

    /* Add other configure information validation here */
    if (vm->jit_use) {
        ASSERT (JIT_SUPPORT == 1,
            "You must compile with JIT Support to use jit.\n");
    }

    /* Print the configure information */
    printf("Using configure file: %s\n", vm->configure_filename);
    printf("ram_size: %dk bytes \n", vm->ram_size);
    printf("boot_method: %s \n", boot_method_string[vm->boot_method-1]);
    if (vm->flash_size != 0) {
        printf("flash_type: %s \n",flash_type_string[vm->flash_type-1]);
    	printf("flash_size: %dk bytes \n",vm->flash_size);
    	if (vm->flash_type == FLASH_TYPE_NOR_FLASH) {
    	  printf("flash_file_name: %s \n",vm->flash_filename);
    	  printf("flash_phy_address: 0x%x \n",vm->flash_address);
    	}
    }
    if (vm->boot_method == BOOT_BINARY) {
    	printf("boot_from: %s \n",boot_from_string[vm->boot_from-1]);
    }
    if (vm->boot_method == BOOT_ELF) {
    	printf("kernel_file_name: %s \n",vm->kernel_filename);
    }

    if (vm->gdb_debug != 0) {
    	printf("GDB debug enable\n");
    	printf("GDB port: %d \n",vm->gdb_port);
    }

    /* print other configure information here */
    if (pic32->boot_flash_size > 0) {
        printf ("boot_flash_size: %dk bytes\n", pic32->boot_flash_size);
        printf ("boot_flash_address: 0x%x\n", pic32->boot_flash_address);
        printf ("boot_file_name: %s\n", pic32->boot_file_name);
    }
    if (pic32->sdcard_port) {
        printf ("sdcard_port: SPI%d\n", pic32->sdcard_port);
    }
    if (pic32->sdcard0_size > 0) {
        printf ("sdcard0_size: %dM bytes\n", pic32->sdcard0_size);
        printf ("sdcard0_file_name: %s\n", pic32->sdcard0_file_name);
    }
    if (pic32->sdcard1_size > 0) {
        printf ("sdcard1_size: %dM bytes\n", pic32->sdcard1_size);
        printf ("sdcard1_file_name: %s\n", pic32->sdcard1_file_name);
    }

    printf ("start_address: 0x%x\n", pic32->start_address);
    if (vm->trace_address != 0)
        printf ("trace_address: 0x%x\n", vm->trace_address);
    for (i=0; i<NVTTY; i++) {
        if (vm->vtty_type[i] != VTTY_TYPE_NONE)
            printf ("uart%d_type = %s\n", i+1,
                vm->vtty_type[i] == VTTY_TYPE_TERM ? "console" :
                vm->vtty_type[i] == VTTY_TYPE_TCP  ? "tcp" : "???");
        if (vm->vtty_tcp_port[i])
            printf ("uart%d_type = %u\n", i+1,
                vm->vtty_tcp_port[i]);
    }
}

/*
 * Create an instance of virtual machine.
 */
vm_instance_t *create_instance (char *configure_filename)
{
    vm_instance_t *vm;
    pic32_t *pic32;
    const char *name = "pic32";
    int i;

    pic32 = malloc (sizeof (*pic32));
    if (! pic32) {
        fprintf (stderr, "PIC32: unable to create new instance!\n");
        return NULL;
    }
    memset (pic32, 0, sizeof (*pic32));

    vm = vm_create (name, VM_TYPE_PIC32);
    if (! vm) {
        fprintf (stderr, "PIC32: unable to create VM instance!\n");
        free (pic32);
        return NULL;
    }
    vm->hw_data = pic32;
    pic32->vm = vm;
    vm->vtty_type[0] = VTTY_TYPE_TERM;
    for (i=1; i<NVTTY; i++)
        vm->vtty_type[i] = VTTY_TYPE_NONE;

    /* Initialize default parameters for  pic32 */
    if (configure_filename == NULL)
#ifdef UBW32
        configure_filename = "pic32_ubw32.conf";
#elif defined EXPLORER16
        configure_filename = "pic32_explorer16.conf";
#elif defined MAXIMITE
        configure_filename = "pic32_maximite.conf";
#elif defined MAX32
        configure_filename = "pic32_max32.conf";
#endif
    vm->configure_filename = strdup (configure_filename);
    vm->ram_size = 128;         /* kilobytes */
    vm->boot_method = BOOT_BINARY;
    vm->boot_from = BOOT_FROM_NOR_FLASH;
    vm->flash_type = FLASH_TYPE_NOR_FLASH;

    pic32_parse_configure (pic32);

    /* init gdb debug */
    vm_debug_init (pic32->vm);

    return pic32->vm;
}

int init_instance (vm_instance_t * vm)
{
    pic32_t *pic32 = (pic32_t *) vm->hw_data;
    cpu_mips_t *cpu;

    if (pic32_init_platform (pic32) == -1) {
        vm_error (vm, "unable to initialize the platform hardware.\n");
        return (-1);
    }
    if (! vm->boot_cpu) {
        vm_error (vm, "unable to boot cpu.\n");
        return (-1);
    }

    /* IRQ routing */
    vm->set_irq = pic32_set_irq;
    vm->clear_irq = pic32_clear_irq;

    vm_suspend (vm);

    /* Check that CPU activity is really suspended */
    if (cpu_group_sync_state (vm->cpu_group) == -1) {
        vm_error (vm, "unable to sync with system CPUs.\n");
        return (-1);
    }

    /* Reset the boot CPU */
    cpu = vm->boot_cpu;
    mips_reset (cpu);

    /* Set config0-config3 registers. */
    cpu->cp0.config_usable = 0x0f;
    cpu->cp0.config_reg[0] = 0xa4010582;
    cpu->cp0.config_reg[1] = 0x80000004;
    cpu->cp0.config_reg[2] = 0x80000000;
    cpu->cp0.config_reg[3] = 0x00000060;

    /* set PC and PRID */
    cpu->cp0.reg[MIPS_CP0_PRID] = 0x00ff8700;   /* TODO */
    cpu->cp0.tlb_entries = 0;
    cpu->pc = pic32->start_address;

    /* reset all devices */
    dev_reset_all (vm);
    dev_sdcard_reset (cpu);

#ifdef _USE_JIT_
    /* if jit is used. flush all jit buffer */
    if (vm->jit_use)
        mips_jit_flush (cpu, 0);
#endif

    /* Launch the simulation */
    printf ("--- Start simulation: PC=0x%" LL "x, JIT %sabled\n",
            cpu->pc, vm->jit_use ? "en" : "dis");
    vm->status = VM_STATUS_RUNNING;
    cpu_start (vm->boot_cpu);
    return (0);
}
