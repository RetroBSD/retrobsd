 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *
  * This file is part of the virtualmips distribution.
  * See LICENSE file for terms of the license.
  */

#define _GNU_SOURCE
#include<string.h>
#include <assert.h>
#include<stdlib.h>
#include <confuse.h>

#include "vp_lock.h"
#include "utils.h"
#include "mips.h"
#include "vm.h"
#include "cpu.h"
#include "mips_exec.h"
#include "debug.h"

#include "pavo.h"
#include "device.h"
#include "dev_cs8900.h"
#include "mips_jit.h"

#define MIPS_TIMER_INTERRUPT    7

extern m_uint32_t jz4740_int_table[JZ4740_INT_INDEX_MAX];
int dev_jz4740_gpio_init (vm_instance_t * vm, char *name, m_pa_t paddr,
    m_uint32_t len);
int dev_jz4740_uart_init (vm_instance_t * vm, char *name, m_pa_t paddr,
    m_uint32_t len, u_int irq, vtty_t * vtty);
int dev_jz4740_cpm_init (vm_instance_t * vm, char *name, m_pa_t paddr,
    m_uint32_t len);
int dev_jz4740_emc_init (vm_instance_t * vm, char *name, m_pa_t paddr,
    m_uint32_t len);
int dev_jz4740_rtc_init (vm_instance_t * vm, char *name, m_pa_t paddr,
    m_uint32_t len);
int dev_jz4740_wdt_tcu_init (vm_instance_t * vm, char *name, m_pa_t paddr,
    m_uint32_t len);
int dev_jz4740_int_init (vm_instance_t * vm, char *name, m_pa_t paddr,
    m_uint32_t len);
int dev_jz4740_dma_init (vm_instance_t * vm, char *name, m_pa_t paddr,
    m_uint32_t len);
int dev_jz4740_lcd_init (vm_instance_t * vm, char *name, m_pa_t paddr,
    m_uint32_t len);

void dev_jz4740_gpio_setirq (int irq);
void dev_jz4740_gpio_clearirq (int irq);

/* Initialize default parameters for pavo */
static void pavo_init_defaults (pavo_t * pavo)
{
    vm_instance_t *vm = pavo->vm;

    if (vm->configure_filename == NULL)
        vm->configure_filename = strdup (PAVO_DEFAULT_CONFIG_FILE);
    vm->ram_size = PAVO_DEFAULT_RAM_SIZE;
    vm->boot_method = PAVO_DEFAULT_BOOT_METHOD;
    vm->kernel_filename = strdup (PAVO_DEFAULT_KERNEL_FILENAME);
}

int pavo_init_cs8900 (pavo_t * pavo, char *name, m_pa_t paddr, m_uint32_t len,
    int irq_no)
{

    struct vm_instance *vm = pavo->vm;

    int nio_type = -1;
    netio_desc_t *nio;
    int count;
    char *tokens[10];
    struct cs8900_data *d;

    if ((count = m_strsplit (pavo->cs8900_iotype, ':', tokens, 10)) < 2) {
        vm_error (vm, "unable to parse NIO description '%s'.\n",
            pavo->cs8900_iotype);
        return (-1);
    }
    nio_type = netio_get_type (tokens[0]);

    switch (nio_type) {
    case NETIO_TYPE_TAP:
        nio = netio_desc_create_tap (name, tokens[1]);
        break;
        //case NETIO_TYPE_LINUX_ETH:
        //      nio=netio_desc_create_lnxeth(name,tokens[1]);
        //      break;
    default:
        return (-1);
    }
    if (!nio) {
        vm_error (vm, "unable to create NETIO  descriptor %s\n", tokens[0]);
        return (-1);
    }
    d = dev_cs8900_init (vm, name, paddr, len, irq_no);
    if (!d) {
        vm_error (vm, "unable to int cs8900\n");
        return (-1);
    }
    if (dev_cs8900_set_nio (d, nio) == -1) {
        vm_error (vm, "unable to set cs8900 nio \n");
        return (-1);
    }

    return 0;

}

/* Initialize the PAVO Platform (MIPS) */
static int pavo_init_platform (pavo_t * pavo)
{
    struct vm_instance *vm = pavo->vm;
    cpu_mips_t *cpu0;
    void *(*cpu_run_fn) (void *);

    vm_init_vtty (vm);

    /* Create a CPU group */
    vm->cpu_group = cpu_group_create ("System CPU");

    /* Initialize the virtual MIPS processor */
    if (!(cpu0 = cpu_create (vm, CPU_TYPE_MIPS32, 0))) {
        vm_error (vm, "unable to create CPU0!\n");
        return (-1);
    }
    /* Add this CPU to the system CPU group */
    cpu_group_add (vm->cpu_group, cpu0);
    vm->boot_cpu = cpu0;

    cpu_run_fn = (void *) mips_exec_run_cpu;
    /* create the CPU thread execution */
    if (pthread_create (&cpu0->cpu_thread, NULL, cpu_run_fn, cpu0) != 0) {
        fprintf (stderr, "cpu_create: unable to create thread for CPU%u\n",
            0);
        free (cpu0);
        return (-1);
    }
    cpu0->addr_bus_mask = PAVO_ADDR_BUS_MASK;

    /* Initialize RAM */
    vm_ram_init (vm, 0x00000000ULL);

    /*create 1GB nand flash */
    if ((vm->flash_size == 0x400) && (vm->flash_type = FLASH_TYPE_NAND_FLASH))
        if (dev_nand_flash_1g_init (vm, "NAND FLASH 1G", NAND_DATAPORT,
                0x10004, &(pavo->nand_flash)) == -1)
            return (-1);
    if (dev_jz4740_gpio_init (vm, "JZ4740 GPIO", JZ4740_GPIO_BASE,
            JZ4740_GPIO_SIZE) == -1)
        return (-1);
    if (dev_jz4740_uart_init (vm, "JZ4740 UART0", JZ4740_UART0_BASE,
            JZ4740_UART0_SIZE, 9, vm->vtty_con1) == -1)
        return (-1);
    if (dev_jz4740_uart_init (vm, "JZ4740 UART1", JZ4740_UART1_BASE,
            JZ4740_UART1_SIZE, 8, vm->vtty_con2) == -1)
        return (-1);

    if (dev_jz4740_cpm_init (vm, "JZ4740 CPM", JZ4740_CPM_BASE,
            JZ4740_CPM_SIZE) == -1)
        return (-1);
    if (dev_jz4740_emc_init (vm, "JZ4740 EMC", JZ4740_EMC_BASE,
            JZ4740_EMC_SIZE) == -1)
        return (-1);
    if (dev_jz4740_rtc_init (vm, "JZ4740 RTC", JZ4740_RTC_BASE,
            JZ4740_RTC_SIZE) == -1)
        return (-1);
    if (dev_jz4740_wdt_tcu_init (vm, "JZ4740 WDT/TCU", JZ4740_WDT_TCU_BASE,
            JZ4740_WDT_TCU_SIZE) == -1)
        return (-1);
    if (dev_jz4740_int_init (vm, "JZ4740 INT", JZ4740_INT_BASE,
            JZ4740_INT_SIZE) == -1)
        return (-1);
    if (dev_jz4740_dma_init (vm, "JZ4740 DMA", JZ4740_DMA_BASE,
            JZ4740_DMA_SIZE) == -1)
        return (-1);

    if (pavo->cs8900_enable == 1) {
        if (pavo_init_cs8900 (pavo, "CS8900A", CS8900_IO_BASE, CS8900_SIZE,
                CS8900_DEFAULT_IRQ) == -1)
            return (-1);
    }

    /*LCD*/
#ifdef SIM_LCD
        if (dev_jz4740_lcd_init (vm, "JZ4740 LCD", JZ4740_LCD_BASE,
            JZ4740_LCD_SIZE) == -1)
        return (-1);

#endif

    return (0);
}

static int pavo_boot (pavo_t * pavo)
{
    vm_instance_t *vm = pavo->vm;

    if (!vm->boot_cpu)
        return (-1);

    return jz4740_reset (vm);

}

void pavo_clear_irq (vm_instance_t * vm, u_int irq)
{
    m_uint32_t irq_mask;

    irq_mask = 1 << irq;

    /*clear ISR and IPR */
    jz4740_int_table[INTC_ISR / 4] &= ~irq_mask;
    jz4740_int_table[INTC_IPR / 4] &= ~irq_mask;

}

/*map irq to soc irq*/
int forced_inline plat_soc_irq (u_int irq)
{
    if ((irq >= 48) && (irq <= 175)) {
        dev_jz4740_gpio_setirq (irq);
        /*GPIO IRQ */
        if ((irq >= 48) && (irq <= 79))
            irq = IRQ_GPIO0;
        else if ((irq >= 80) && (irq <= 111))
            irq = IRQ_GPIO1;
        else if ((irq >= 112) && (irq <= 143))
            irq = IRQ_GPIO2;
        else if ((irq >= 144) && (irq <= 175))
            irq = IRQ_GPIO3;
    }
    return irq;
}

void pavo_set_irq (vm_instance_t * vm, u_int irq)
{
    m_uint32_t irq_mask;

    irq = plat_soc_irq (irq);

    irq_mask = 1 << irq;
    jz4740_int_table[INTC_ISR / 4] |= irq_mask;
    /*first check ICMR. masked interrupt is **invisible** to cpu */
    if (unlikely (jz4740_int_table[INTC_IMR / 4] & irq_mask)) {
        /*the irq is masked. clear IPR */
        jz4740_int_table[INTC_IPR / 4] &= ~irq_mask;
    } else {
        /*the irq is not masked */

        /*set IPR */
        /*
         * we set IPR, not *or* . yajin
         *
         * JZ Kernel 'plat_irq_dispatch' determine which is the highest priority interrupt
         * and handle.
         * It uses a function ffs to find first set irq from least bit to highest bit.
         * 260         irq = ffs(intc_ipr) - 1;
         *
         * That means when tcu0 irq and gpio1 irq occurs at the same time ,INTC_IPR=0x8800000
         * and irq handler will handle tcu0 irq(bit 23) not gpio1 irq(bit 27).
         *
         * In pavo gpio1->cs8900 int
         *
         * TCU0 irq occurs every 10 ms and gpio1 occurs about 10ms (cs8900 has received a packet
         * or has txed a packet), jz kernel always handle tcu0 irq. gpio1 irq is hungry. So I just set
         * jz4740_int_table[INTC_IPR/4]= irq_mask not or(|) irq_mask. TCU0 irq may be lost. However,
         * gpio1 irq is not so ofen so it is not a big problem.
         *
         * In emulator, irq is not a good method for hardware to tell kernel something has happened.
         * Emulator likes polling more than interrupt :) .
         *
         */
        jz4740_int_table[INTC_IPR / 4] = irq_mask;

        mips_set_irq (vm->boot_cpu, JZ4740_INT_TO_MIPS);
        mips_update_irq_flag (vm->boot_cpu);
    }
}

COMMON_CONFIG_INFO_ARRAY;
static void printf_configure (pavo_t * pavo)
{

    vm_instance_t *vm = pavo->vm;
    PRINT_COMMON_CONFIG_OPTION;

    /*print other configure information here */
    if (pavo->cs8900_enable == 1) {
        printf ("CS8900 net card enabled\n");
        printf ("CS8900 iotype %s \n", pavo->cs8900_iotype);
    } else
        printf ("CS8900 net card disenabled\n");
}

static void pavo_parse_configure (pavo_t * pavo)
{
    vm_instance_t *vm = pavo->vm;
    cfg_opt_t opts[] = {
        COMMON_CONFIG_OPTION
            /*add other configure information here */
            CFG_SIMPLE_INT ("cs8900_enable", &(pavo->cs8900_enable)),
        CFG_SIMPLE_STR ("cs8900_iotype", &(pavo->cs8900_iotype)),
        CFG_SIMPLE_INT ("jit_use", &(vm->jit_use)),

        CFG_END ()
    };
    cfg_t *cfg;

    cfg = cfg_init (opts, 0);
    cfg_parse (cfg, vm->configure_filename);
    cfg_free (cfg);

    VALID_COMMON_CONFIG_OPTION;

    /*add other configure information validation here */
    if (vm->boot_method == BOOT_BINARY) {
        ASSERT (vm->boot_from == 2,
            "boot_from must be 2(NAND Flash)\n pavo only can boot from NAND Flash.\n");
    }
    if (vm->flash_size != 0) {
        ASSERT (vm->flash_size == 4,
            "flash_size should be 4.\n We only support 4MB NOR flash emulation\n");
    }
    if (pavo->cs8900_enable == 1) {
        ASSERT (pavo->cs8900_iotype != NULL, "You must set cs8900_enable \n");
    }
    if (vm->jit_use == 1) {
        ASSERT (JIT_SUPPORT == 1,
            "You must compile with JIT Support to use jit. \n");
    }

    /*Print the configure information */
    printf_configure (pavo);

}

/* Clear timer interrupt */
void clear_timer_irq (cpu_mips_t *cpu)
{
    mips_clear_irq (cpu, MIPS_TIMER_INTERRUPT);
    mips_update_irq_flag (cpu);
}

/* Create a router instance */
vm_instance_t *create_instance (char *configure_filename)
{
    pavo_t *pavo;
    char *name;
    if (!(pavo = malloc (sizeof (*pavo)))) {
        fprintf (stderr, "ADM5120': Unable to create new instance!\n");
        return NULL;
    }

    memset (pavo, 0, sizeof (*pavo));
    name = strdup ("pavo");

    if (!(pavo->vm = vm_create (name, VM_TYPE_PAVO))) {
        fprintf (stderr, "PAVO : unable to create VM instance!\n");
        goto err_vm;
    }
    free (name);

    if (configure_filename != NULL)
        pavo->vm->configure_filename = strdup (configure_filename);
    pavo_init_defaults (pavo);
    pavo_parse_configure (pavo);
    /*init gdb debug */
    vm_debug_init (pavo->vm);

    pavo->vm->hw_data = pavo;

    return (pavo->vm);

  err_vm:
    free (name);
    free (pavo);
    return NULL;

}

int init_instance (vm_instance_t * vm)
{
    pavo_t *pavo = VM_PAVO (vm);

    if (pavo_init_platform (pavo) == -1) {
        vm_error (vm, "unable to initialize the platform hardware.\n");
        return (-1);
    }
    /* IRQ routing */
    vm->set_irq = pavo_set_irq;
    vm->clear_irq = pavo_clear_irq;

    return (pavo_boot (pavo));

}
