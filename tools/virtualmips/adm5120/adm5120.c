 /*
  * Copyright (C) yajin 2008<yajinzhou@gmail.com >
  *
  * This file is part of the virtualmips distribution.
  * See LICENSE file for terms of the license.
  *
  */

#define _GNU_SOURCE
#include<string.h>
#include <assert.h>
#include<stdlib.h>
#include <confuse.h>

#include "utils.h"
#include "adm5120.h"
#include "mips.h"
#include "vm.h"
#include "cpu.h"
#include "mips_exec.h"
#include "debug.h"
#include "vp_lock.h"

int dev_sw_init (vm_instance_t * vm, char *name, m_pa_t paddr,
    m_uint32_t len);
int dev_rom_init (vm_instance_t * vm, char *name);
int dev_nor_flash_4m_init (vm_instance_t * vm, char *name);
int dev_mpmc_init (vm_instance_t * vm, char *name, m_pa_t paddr,
    m_uint32_t len);
int dev_intctrl_init (vm_instance_t * vm, char *name, m_pa_t paddr,
    m_uint32_t len);
int dev_uart_init (vm_instance_t * vm, char *name, m_pa_t paddr,
    m_uint32_t len, vtty_t * vtty, int uart_index);
int dev_pci_init (vm_instance_t * vm, char *name, m_pa_t paddr,
    m_uint32_t len);

/* Initialize default parameters for a adm5120 */
static void adm5120_init_defaults (adm5120_t * adm5120)
{
    vm_instance_t *vm = adm5120->vm;

    if (vm->configure_filename == NULL)
        vm->configure_filename = strdup (ADM5120_DEFAULT_CONFIG_FILE);
    vm->ram_size = ADM5120_DEFAULT_RAM_SIZE;
    vm->boot_method = ADM5120_DEFAULT_BOOT_METHOD;
    vm->kernel_filename = strdup (ADM5120_DEFAULT_KERNEL_FILENAME);
}

/* Initialize the adm5120 Platform (MIPS) */
static int adm5120_init_platform (adm5120_t * adm5120)
{
    struct vm_instance *vm = adm5120->vm;
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
    cpu0->addr_bus_mask = ADM5120_ADDR_BUS_MASK;

    /* Initialize RAM */
    vm_ram_init (vm, 0x00000000ULL);

    /* Initialize ROM */
    /*if (vm->rom_size!=0)
     * {
     * if (dev_rom_init(vm, "ROM")==-1)
     * return (-1);
     * } */

    /* Initialize FLASH */
    if ((vm->flash_size != 0) && (vm->flash_type == FLASH_TYPE_NOR_FLASH))
        if (dev_nor_flash_4m_init (vm, "NORFLASH4M") == -1)
            return (-1);

    if (dev_sw_init (vm, "SW", ADM5120_SWCTRL_BASE, SW_INDEX_MAX * 4) == -1)
        return (-1);
    if (dev_mpmc_init (vm, "MPMC", ADM5120_MPMC_BASE,
            MPMC_INDEX_MAX * 4) == -1)
        return (-1);
    if (dev_intctrl_init (vm, "INT CTRL", ADM5120_INTC_BASE,
            INTCTRL_INDEX_MAX * 4) == -1)
        return (-1);
    if (dev_uart_init (vm, "UART 0", ADM5120_UART0_BASE, 0x24, vm->vtty_con1,
            0) == -1)
        return (-1);
    if (dev_uart_init (vm, "UART 1", ADM5120_UART1_BASE, 0x24, vm->vtty_con2,
            0) == -1)
        return (-1);
    if (dev_pci_init (vm, "PCI", ADM5120_PCI_BASE, PCI_INDEX_MAX * 4) == -1)
        return (-1);

    return (0);
}

extern m_uint32_t sw_table[SW_INDEX_MAX];
/*set adm5120 reg default value. Only needed if boot from linux elf image.
Linux will query sw_table[0x7] to get sdram size.
 */
static void adm5120_reg_default_value (adm5120_t * adm5120)
{
    if (adm5120->vm->ram_size == 128)
        sw_table[0x7] = 0x50405;        //  128M
    else if (adm5120->vm->ram_size == 64)
        sw_table[0x7] = 0x50404;        //  64m
    else if (adm5120->vm->ram_size == 32)
        sw_table[0x7] = 0x50423;        //16M *2
    else if (adm5120->vm->ram_size == 16)
        sw_table[0x7] = 0x50403;        //16M
    else if (adm5120->vm->ram_size == 8)
        sw_table[0x7] = 0x50402;        // 8M
    else if (adm5120->vm->ram_size == 4)
        sw_table[0x7] = 0x50401;        //  4M
    else
        ASSERT (0, "Invalid ram_size.\n Valid value:4,8,16,32,64,128\n");
}

static int adm5120_boot (adm5120_t * adm5120)
{
    vm_instance_t *vm = adm5120->vm;
    cpu_mips_t *cpu;
    m_va_t kernel_entry_point;

    if (!vm->boot_cpu)
        return (-1);

    vm_suspend (vm);

    /* Check that CPU activity is really suspended */
    if (cpu_group_sync_state (vm->cpu_group) == -1) {
        vm_error (vm, "unable to sync with system CPUs.\n");
        return (-1);
    }

    /* Reset the boot CPU */
    cpu = (vm->boot_cpu);
    mips_reset (cpu);

    /*set configure register */
    cpu->cp0.config_usable = 0x3;       /*only configure sel 0 and 1 is valid */
    cpu->cp0.config_reg[0] = ADM5120_CONFIG0;
    cpu->cp0.config_reg[1] = ADM5120_CONFIG1;

    /*set PC and PRID */
    cpu->cp0.reg[MIPS_CP0_PRID] = ADM5120_PRID;
    cpu->cp0.tlb_entries = ADM5120_DEFAULT_TLB_ENTRYNO;
    cpu->pc = ADM5120_ROM_PC;
    /*If we boot from elf kernel image, load the image and set pc to elf entry */
    if (vm->boot_method == BOOT_ELF) {
        if (mips_load_elf_image (cpu, vm->kernel_filename,
                &kernel_entry_point) == -1)
            return (-1);
        adm5120_reg_default_value (adm5120);
        cpu->pc = kernel_entry_point;
    }

    /* Launch the simulation */
    printf ("\nADM5120 '%s': starting simulation (CPU0 PC=0x%" LL "x), "
        "JIT %sabled.\n", vm->name, cpu->pc, vm->jit_use ? "en" : "dis");

    vm->status = VM_STATUS_RUNNING;
    cpu_start (vm->boot_cpu);
    return (0);

}

extern m_uint32_t intctrl_table[INTCTRL_INDEX_MAX];

/*
Mapping adm irq to mips irq.

So why we need a mapping of interrupts?

IN ADM5120,there are 10 interrupts
0	 Timer
1	Uart 0
2	 Uart 1
3	 USB Host
4	 External I/O 0
5	 External I/O 1
6	 PCI 0
7	 PCI 1
8	 PCI 2
9	 Switch

ADM5120 will triger INTERRUPT 2 and 3 to MIPS.
INT_M(0X14) register control the interrupt releation of ADM5120 and iqr/firq.

 */
int adm_irq2mips_irq (vm_instance_t * vm, u_int irq)
{
    m_uint32_t int_bit_mask = 0;

    int_bit_mask = 1 << irq;
    if ((intctrl_table[IRQ_MODE_REG / 4] & int_bit_mask) == int_bit_mask) {
        return ADM5120_MIPSINT_FIQ;
    } else
        return ADM5120_MIPSINT_IRQ;

}

int whether_irq_enable (vm_instance_t * vm, u_int irq)
{
    m_uint32_t int_bit_mask = 0;

    int_bit_mask = 1 << irq;
    if ((intctrl_table[IRQ_ENABLE_REG / 4] & int_bit_mask) == int_bit_mask) {
        return TRUE;
    } else
        return FALSE;

}

void adm5120_clear_irq (vm_instance_t * vm, u_int irq)
{
    assert (irq <= INT_LVL_MAX);
    int mips_irq_no;
    m_uint32_t int_bit_mask = 0;

    int_bit_mask = 1 << irq;

    /*clear raw status */
    intctrl_table[IRQ_RAW_STATUS_REG / 4] &= ~int_bit_mask;

    mips_irq_no = adm_irq2mips_irq (vm, irq);

    if (ADM5120_MIPSINT_FIQ == mips_irq_no) {
        intctrl_table[FIQ_STATUS_REG / 4] &= ~int_bit_mask;
    } else {
        intctrl_table[IRQ_STATUS_REG / 4] &= ~int_bit_mask;
    }
    irq = mips_irq_no;
    mips_clear_irq (vm->boot_cpu, irq);

}

/*We must map adm irq to mips irq before setting irq*/
void adm5120_set_irq (vm_instance_t * vm, u_int irq)
{
    assert (irq <= INT_LVL_MAX);

    int mips_irq_no;
    m_uint32_t int_bit_mask = 0;

    int_bit_mask = 1 << irq;

    /*set raw status */
    intctrl_table[IRQ_RAW_STATUS_REG / 4] |= int_bit_mask;

    /*check whether irq is enabled */
    if (whether_irq_enable (vm, irq) == FALSE)
        return;

    mips_irq_no = adm_irq2mips_irq (vm, irq);

    if (ADM5120_MIPSINT_FIQ == mips_irq_no) {
        intctrl_table[FIQ_STATUS_REG / 4] |= int_bit_mask;
    } else {
        intctrl_table[IRQ_STATUS_REG / 4] |= int_bit_mask;
    }

    irq = mips_irq_no;
    mips_set_irq (vm->boot_cpu, irq);
    mips_update_irq_flag (vm->boot_cpu);

}

COMMON_CONFIG_INFO_ARRAY static void printf_configure (adm5120_t * adm5120)
{
    vm_instance_t *vm = adm5120->vm;
    PRINT_COMMON_CONFIG_OPTION;
}

static void adm5120_parse_configure (adm5120_t * adm5120)
{
    vm_instance_t *vm = adm5120->vm;
    cfg_opt_t opts[] = {
        COMMON_CONFIG_OPTION CFG_END ()
    };
    cfg_t *cfg;

    cfg = cfg_init (opts, 0);
    cfg_parse (cfg, vm->configure_filename);
    cfg_free (cfg);

    VALID_COMMON_CONFIG_OPTION;

    if (vm->boot_method == BOOT_BINARY) {
        ASSERT (vm->boot_from == 1,
            "boot_from must be 1(NOR Flash)\n ADM5120 only can boot from NOR Flash.\n");
    }

    /*Print the configure information */
    printf_configure (adm5120);

}

/* Create a router instance */
vm_instance_t *create_instance (char *configure_filename)
{
    adm5120_t *adm5120;
    char *name;
    if (!(adm5120 = malloc (sizeof (*adm5120)))) {
        fprintf (stderr, "ADM5120': Unable to create new instance!\n");
        return NULL;
    }

    memset (adm5120, 0, sizeof (*adm5120));
    name = strdup ("adm5120");

    if (!(adm5120->vm = vm_create (name, VM_TYPE_ADM5120))) {
        fprintf (stderr, "ADM5120 : unable to create VM instance!\n");
        goto err_vm;
    }
    free (name);
    if (configure_filename != NULL)
        adm5120->vm->configure_filename = strdup (configure_filename);
    adm5120_init_defaults (adm5120);
    adm5120_parse_configure (adm5120);
    /*init gdb debug */
    vm_debug_init (adm5120->vm);

    adm5120->vm->hw_data = adm5120;

    return (adm5120->vm);

  err_vm:
    free (adm5120);
    return NULL;

}

int init_instance (vm_instance_t * vm)
{
    adm5120_t *adm5120 = VM_ADM5120 (vm);

    if (adm5120_init_platform (adm5120) == -1) {
        vm_error (vm, "unable to initialize the platform hardware.\n");
        return (-1);
    }
    /* IRQ routing */
    vm->set_irq = adm5120_set_irq;
    vm->clear_irq = adm5120_clear_irq;

    return (adm5120_boot (adm5120));

}
