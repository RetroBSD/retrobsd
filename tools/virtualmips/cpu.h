 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *
  * This file is part of the virtualmips distribution.
  * See LICENSE file for terms of the license.
  *
  */

#ifndef __CPU_H__
#define __CPU_H__

#include <pthread.h>

#include "utils.h"
#include "mips.h"
#include "system.h"

/* Possible CPU types */
enum {
    CPU_TYPE_MIPS64 = 1,
    CPU_TYPE_MIPS32,
};

/* Virtual CPU states */
enum {
    CPU_STATE_RUNNING = 0,      /*cpu is running */
    CPU_STATE_HALTED,
    CPU_STATE_SUSPENDED,        /*CPU is SUSPENDED */
    CPU_STATE_RESTARTING,       /*cpu is restarting */
    CPU_STATE_PAUSING,          /*cpu is pausing for timer */
};

/* CPU group definition */
typedef struct cpu_group cpu_group_t;
struct cpu_group {
    char *name;
    cpu_mips_t *cpu_list;
    void *priv_data;
};

void cpu_log (cpu_mips_t * cpu, char *module, char *format, ...);
void cpu_start (cpu_mips_t * cpu);
void cpu_stop (cpu_mips_t * cpu);
void cpu_restart (cpu_mips_t * cpu);
cpu_mips_t *cpu_create (vm_instance_t * vm, u_int type, u_int id);
void cpu_delete (cpu_mips_t * cpu);
cpu_mips_t *cpu_group_find_id (cpu_group_t * group, u_int id);
int cpu_group_find_highest_id (cpu_group_t * group, u_int * highest_id);
int cpu_group_add (cpu_group_t * group, cpu_mips_t * cpu);
cpu_group_t *cpu_group_create (char *name);
void cpu_group_delete (cpu_group_t * group);
int cpu_group_rebuild_mts (cpu_group_t * group);
void cpu_group_start_all_cpu (cpu_group_t * group);
void cpu_group_stop_all_cpu (cpu_group_t * group);
void cpu_group_set_state (cpu_group_t * group, u_int state);
int cpu_group_sync_state (cpu_group_t * group);
int cpu_group_save_state (cpu_group_t * group);
int cpu_group_restore_state (cpu_group_t * group);

#endif
