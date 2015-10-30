/*
 * Simulation of generic M4K core.
 *
 * Copyright (C) 2015 Serge Vakulenko <serge@vak.ru>
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

#include "m4k.h"
#include "device.h"
#include "mips_jit.h"

#define BOOT_RAM_KBYTES 64      /* Size of boot RAM area */

/*
 * Store a word to the physical memory.
 */
int store_word (vm_instance_t *vm, m_pa_t paddr, unsigned data)
{
    struct vdevice *dev = dev_lookup (vm, paddr);
    char *ptr;

    if (! dev)
        return -1;

    /* Only for RAM */
    ptr = (char *) dev->host_addr + (paddr - dev->phys_addr);
    *(m_uint32_t*) ptr = data;
    return 0;
}

/*
 * Load MIPS hex file into memory.
 */
static void load_hex_file (struct vm_instance *vm, const char *filename)
{
    FILE *fp;
    char buf [64], *p, *ep;
    unsigned address, end_address, data, big_endian = 0;

    fp = fopen (filename, "r");
    if (! fp) {
        fprintf (stderr, "%s: Cannot open\n", filename);
        exit (-1);
    }
    while (fgets (buf, sizeof(buf), fp) != 0) {
        /* Check header for endian */
        if (buf[0] == '#') {
            if (strncmp (buf, "# Endian Big", 12) == 0) {
                big_endian = 1;
            }
            continue;
        }
        address = strtoul (buf, &ep, 16);
        if (ep == buf || *ep == 0) {
            continue;
        }
        if (*ep == '-') {
            p = ep+1;
            end_address = strtoul (p, &ep, 16);
            if (ep == p || *ep != ' ') {
                continue;
            }
        } else
            end_address = address;
        p = ep;
        data = strtoul (p, &ep, 16);
        if (ep == p) {
            continue;
        }

        /* Byte swap. */
        if (big_endian) {
            data = (data & 0x000000ff) << 24 |
                   (data & 0x0000ff00) <<  8 |
                   (data & 0x00ff0000) >>  8 |
                   (data & 0xff000000) >> 24 ;
        }

        do {
            if (store_word (vm, address << 2, data) < 0) {
                fprintf (stderr, "%s: No memory at physical address %08x\n",
                    filename, address << 2);
                exit (-1);
            }
            //printf ("  %08x <= %08x\n", address << 2, data);
        } while (address++ < end_address);
    }
    fclose (fp);
}

/*
 * Initialize the M4K platform.
 */
static int m4k_init_platform (m4k_t *m4k)
{
    struct vm_instance *vm = m4k->vm;
    cpu_mips_t *cpu0;
    void *(*cpu_run_fn) (void *);

    //vm_init_vtty (vm);

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

    /* Initialize main RAM */
    vm_ram_init (vm, 0x00000000ULL);

    /* Initialize boot RAM area */
    dev_ram_init (vm, "boot", 0x1fc00000, BOOT_RAM_KBYTES*1024);

    load_hex_file (vm, m4k->boot_file_name);
    return (0);
}

/*
 * Find pending interrupt with the biggest priority.
 * Setup INTSTAT and cause registers.
 * Update irq_pending flag for CPU.
 */
void m4k_update_irq_flag (m4k_t *m4k)
{
    cpu_mips_t *cpu = m4k->vm->boot_cpu;

    /* Assume no interrupts pending. */
    cpu->irq_cause = 0;
    cpu->irq_pending = 0;

    //TODO
    cpu->irq_cause = 0;
/*printf ("-- vector = %d, level = %d\n", vector, level);*/

    mips_update_irq_flag (cpu);
}

void m4k_clear_irq (vm_instance_t *vm, u_int irq)
{
    m4k_t *m4k = (m4k_t*) vm->hw_data;

    /* Clear interrupt flag status */
    //TODO

    m4k_update_irq_flag (m4k);
}

void m4k_set_irq (vm_instance_t *vm, u_int irq)
{
    m4k_t *m4k = (m4k_t*) vm->hw_data;

    /* Set interrupt flag status */
    //TODO

    m4k_update_irq_flag (m4k);
}

/*
 * Activate core timer interrupt
 */
void set_timer_irq (cpu_mips_t *cpu)
{
    //TODO
}

/*
 * Clear core timer interrupt
 */
void clear_timer_irq (cpu_mips_t *cpu)
{
    //TODO
}

/*
 * Increment timers.
 */
void host_alarm (cpu_mips_t *cpu, int nclocks)
{
    //m4k_t *m4k = (m4k_t*) cpu->vm->hw_data;
    //TODO
}

/*
 * Create an instance of virtual machine.
 */
vm_instance_t *create_instance (char *filename)
{
    vm_instance_t *vm;
    m4k_t *m4k;
    const char *name = "m4k";

    m4k = malloc (sizeof (*m4k));
    if (! m4k) {
        fprintf (stderr, "M4K: unable to create new instance!\n");
        return NULL;
    }
    memset (m4k, 0, sizeof (*m4k));

    vm = vm_create (name, VM_TYPE_PIC32);
    if (! vm) {
        fprintf (stderr, "M4K: unable to create VM instance!\n");
        free (m4k);
        return NULL;
    }
    vm->hw_data = m4k;
    m4k->vm = vm;

    /* Initialize default parameters for  m4k */
    vm->ram_size = 4*1024;          /* kilobytes */
    vm->debug_level = 3;            /* trace all instructions */

    m4k->boot_file_name = filename ? filename : "test.hex";
    m4k->start_address = 0xbfc00000;

    const char *output_file_name = "m4k.trace";
    printf ("Redirect output to %s\n", output_file_name);
    if (freopen(output_file_name, "w", stdout) != stdout) {
        fprintf (stderr, "M4K: Unable to redirect output!\n");
        exit(-1);
    }

    /* Print the configure information */
    printf("ram_size: %dk bytes \n", vm->ram_size);
    printf("start_address: 0x%x\n", m4k->start_address);

    /* init gdb debug */
    vm_debug_init (m4k->vm);

    return m4k->vm;
}

int init_instance (vm_instance_t * vm)
{
    m4k_t *m4k = (m4k_t *) vm->hw_data;
    cpu_mips_t *cpu;

    if (m4k_init_platform (m4k) == -1) {
        vm_error (vm, "unable to initialize the platform hardware.\n");
        return (-1);
    }
    if (! vm->boot_cpu) {
        vm_error (vm, "unable to boot cpu.\n");
        return (-1);
    }

    /* IRQ routing */
    vm->set_irq = m4k_set_irq;
    vm->clear_irq = m4k_clear_irq;

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
    cpu->cp0.config_reg[0] = 0xa4000582;
    cpu->cp0.config_reg[1] = 0x80000006;
    cpu->cp0.config_reg[2] = 0x80000000;
    cpu->cp0.config_reg[3] = 0x00000020;

    /* set PC and PRID */
    cpu->pc = m4k->start_address;
    cpu->cp0.tlb_entries = 0;
    cpu->cp0.reg[MIPS_CP0_PRID]  = 0x00018700;
    cpu->cp0.reg[MIPS_CP0_DEBUG] = 0x00010000;

    /* Enable magic opcodes. */
    cpu->magic_opcodes = 1;

    /* reset all devices */
    dev_reset_all (vm);

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
