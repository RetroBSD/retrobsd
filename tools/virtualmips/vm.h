/*
 * Cisco router simulation platform.
 * Copyright (c) 2005,2006 Christophe Fillot (cf@utc.fr)
 *
 * Virtual machine abstraction.
 */
 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *
  * This file is part of the virtualmips distribution.
  * See LICENSE file for terms of the license.
  *
  */

#ifndef __VM_H__
#define __VM_H__

#include <pthread.h>

#include "cpu.h"
#include "dev_vtty.h"
#include "system.h"
#include "gdb_interface.h"

#define VM_PAGE_SHIFT  12
#define VM_PAGE_SIZE   (1 << VM_PAGE_SHIFT)
#define VM_PAGE_IMASK  (VM_PAGE_SIZE - 1)
#define VM_PAGE_MASK   (~(VM_PAGE_IMASK))

/* Maximum number of devices per VM */
#define VM_DEVICE_MAX  (1 << 6)
/* VM types */
enum {
    VM_TYPE_SWARM = 0,
    VM_TYPE_ADM5120,
    VM_TYPE_PIC32,
    VM_TYPE_PAVO,
};

/* VM instance status */
enum {
    VM_STATUS_HALTED = 0,       /* VM is halted and no HW resources are used */
    VM_STATUS_SHUTDOWN,         /* Shutdown procedure engaged */
    VM_STATUS_RUNNING,          /* VM is running */
    VM_STATUS_SUSPENDED,        /* VM is suspended */
};

/* VM instance */
struct vm_instance {
    char *name;
    int type;
    int status;                 /* Instance status */

    char *log_file;             /* Log filename */
    int log_file_enabled;       /* Logging enabled */
    u_int ram_size;             /* RAM size in Mb */
    //u_int rom_size;           /* ROM size in Mb */
    //char *rom_filename;       /* ROM filename */
    //m_pa_t rom_address;       /* ROM phy address */

    u_int flash_size;           /* FLASH size in Mb */
    char *flash_filename;       /* FLASH filename */
    m_pa_t flash_address;       /* FLASH phy address */
    u_int flash_type;           /* NAND Flash OR NOR FLASH */

    u_int boot_method;          /* binary or elf */
    char *kernel_filename;
    u_int boot_from;

    char *configure_filename;

    FILE *lock_fd, *log_fd;     /* Lock/Log file descriptors */
    int debug_level;            /* Debugging Level */
    u_int trace_address;        /* Trace this address */
    int jit_use;                /* CPUs use JIT */

    /* Basic hardware: system CPU */
    cpu_group_t *cpu_group;
    cpu_mips_t *boot_cpu;

    /* Memory mapped devices */
    struct vdevice *dev_list;
    struct vdevice *dev_array[VM_DEVICE_MAX];

    /* IRQ routing */
    void (*set_irq) (vm_instance_t * vm, u_int irq);
    void (*clear_irq) (vm_instance_t * vm, u_int irq);

    /* Console VTTY type and parameters */
#define NVTTY 6
    int vtty_type [NVTTY];
    int vtty_tcp_port[NVTTY];
    vtty_serial_option_t vtty_serial_option[NVTTY];
    /* Virtual TTY for Console and AUX ports */
    vtty_t *vtty_con [NVTTY];

    /* Specific hardware data */
    void *hw_data;

    /* gdb interface */
    m_uint32_t gdb_debug, gdb_port;
    int gdb_interact_sock;      //connect socket
    int gdb_listen_sock;        //listen socket
    int gdb_debug_from_poll;
    virtual_breakpoint_t *breakpoint_head, *breakpoint_tail;
    int mipsy_debug_mode;
    int mipsy_break_nexti;
};

char *vm_get_type (vm_instance_t * vm);
char *vm_get_platform_type (vm_instance_t * vm);
void vm_flog (vm_instance_t * vm, char *module, char *format, va_list ap);
void vm_log (vm_instance_t * vm, char *module, char *format, ...);
int vm_close_log (vm_instance_t * vm);
int vm_create_log (vm_instance_t * vm);
void vm_error (vm_instance_t * vm, char *format, ...);
vm_instance_t *vm_create (const char *name, int machine_type);
void vm_free (vm_instance_t * vm);
int vm_ram_init (vm_instance_t * vm, m_pa_t paddr);
int vm_init_vtty (vm_instance_t * vm);
void vm_delete_vtty (vm_instance_t * vm);
int vm_bind_device (vm_instance_t * vm, struct vdevice *dev);
int vm_unbind_device (vm_instance_t * vm, struct vdevice *dev);
int vm_map_device (vm_instance_t * vm, struct vdevice *dev, m_pa_t base_addr);
int vm_suspend (vm_instance_t * vm);
int vm_resume (vm_instance_t * vm);
int vm_stop (vm_instance_t * vm);
void vm_monitor (vm_instance_t * vm);
#endif
