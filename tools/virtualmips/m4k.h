/*
 * Copyright (C) 2015 Serge Vakulenko <serge@vak.ru>
 *
 * This file is part of the virtualmips distribution.
 * See LICENSE file for terms of the license.
 */
#ifndef __M4K_H__
#define __M4K_H__

#include "types.h"

#define DATA_WIDTH          32          /* MIPS32 architecture */
#define LL                              /* empty - printf format for machine word */

/*
 * Data types
 */
typedef m_uint32_t m_va_t;
typedef m_uint32_t m_pa_t;
typedef m_uint32_t m_reg_t;
typedef m_int32_t m_ireg_t;
typedef m_uint32_t m_cp0_reg_t;

/*Guest endian*/
#define GUEST_BYTE_ORDER  ARCH_LITTLE_ENDIAN

/* Host to VM conversion functions */
#if HOST_BYTE_ORDER == GUEST_BYTE_ORDER
#define htovm16(x) (x)
#define htovm32(x) (x)
#define htovm64(x) (x)

#define vmtoh16(x) (x)
#define vmtoh32(x) (x)
#define vmtoh64(x) (x)
#else //host:big guest:little

#define htovm16(x) (ntohs(x))
#define htovm32(x) (ntohl(x))
#define htovm64(x) (swap64(x))

#define vmtoh16(x) (htons(x))
#define vmtoh32(x) (htonl(x))
#define vmtoh64(x) (swap64(x))
#endif

struct m4k_system {
    /* Associated VM instance */
    vm_instance_t *vm;

    unsigned start_address;         /* jump here on reset */
    char *boot_file_name;           /* image of boot flash */
};

typedef struct m4k_system m4k_t;
struct virtual_tty;

vm_instance_t *create_instance (char *conf);
int init_instance (vm_instance_t *vm);
int m4k_reset (vm_instance_t *vm);
void m4k_update_irq_flag (m4k_t *m4k);
void m4k_set_irq (vm_instance_t *vm, unsigned irq);
void m4k_clear_irq (vm_instance_t *vm, unsigned irq);
void dumpregs (cpu_mips_t *cpu);

#endif
