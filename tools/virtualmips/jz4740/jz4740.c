 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *
  * This file is part of the virtualmips distribution.
  * See LICENSE file for terms of the license.
  *
  */

#include<string.h>
#include <assert.h>
#include<stdlib.h>
#include <confuse.h>

#include "utils.h"
#include "mips.h"
#include "vm.h"
#include "cpu.h"
#include "mips_exec.h"
#include "debug.h"

#include "jz4740.h"
#include "pavo.h"
#include "device.h"

int jz4740_boot_from_nandflash (vm_instance_t * vm)
{
    struct vdevice *dev;
    unsigned char *page_addr;
    int i;

    pavo_t *pavo;
    if (vm->type == VM_TYPE_PAVO) {
        pavo = VM_PAVO (vm);
    } else
        ASSERT (0, "Error vm type\n");

    /*get ram device */
    dev = dev_lookup (vm, 0x0);
    assert (dev != NULL);
    assert (dev->host_addr != 0);
    /*copy 8K nand flash data to 8K RAM */
    for (i = 0; i < (0x2000 / NAND_FLASH_1G_PAGE_DATA_SIZE); i++) {
        page_addr =
            get_nand_flash_page_ptr (i, pavo->nand_flash->flash_map[0]);
        memcpy ((unsigned char *) dev->host_addr +
            NAND_FLASH_1G_PAGE_DATA_SIZE * i, page_addr,
            NAND_FLASH_1G_PAGE_DATA_SIZE);
    }

    return (0);

}

int jz4740_reset (vm_instance_t * vm)
{
    cpu_mips_t *cpu;
    m_va_t kernel_entry_point;
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
    cpu->cp0.config_usable = 0x83;      /* configure sel 0 1 7 is valid */
    cpu->cp0.config_reg[0] = JZ4740_CONFIG0;
    cpu->cp0.config_reg[1] = JZ4740_CONFIG1;
    cpu->cp0.config_reg[7] = JZ4740_CONFIG7;

    /*set PC and PRID */
    cpu->cp0.reg[MIPS_CP0_PRID] = JZ4740_PRID;
    cpu->cp0.tlb_entries = JZ4740_DEFAULT_TLB_ENTRYNO;
    cpu->pc = JZ4740_ROM_PC;
    /*reset all devices */
    dev_reset_all (vm);

#ifdef _USE_JIT_
    /*if jit is used. flush all jit buffer */
    if (vm->jit_use)
        mips_jit_flush (cpu, 0);
#endif
    /*If we boot from elf kernel image, load the image and set pc to elf entry */
    if (vm->boot_method == BOOT_ELF) {
        if (mips_load_elf_image (cpu, vm->kernel_filename,
                &kernel_entry_point) == -1)
            return (-1);
        cpu->pc = kernel_entry_point;
    } else if (vm->boot_method == BOOT_BINARY) {
        if (jz4740_boot_from_nandflash (vm) == -1)
            return (-1);
    }

/* Launch the simulation */
    printf ("VM '%s': starting simulation (CPU0 PC=0x%" LL
        "x), JIT %sabled.\n", vm->name, cpu->pc, vm->jit_use ? "en" : "dis");
    vm->status = VM_STATUS_RUNNING;
    cpu_start (vm->boot_cpu);
    return (0);

}
