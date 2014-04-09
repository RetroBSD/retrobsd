 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *     
  * This file is part of the virtualmips distribution. 
  * See LICENSE file for terms of the license. 
  *
  */

#ifndef __DEBUG_H__
#define  __DEBUG_H__

#include "vm.h"

#define MIPS_NOBREAK      (-2)
#define MIPS_BREAKANYCPU  (-1)

void vm_debug_init (vm_instance_t * vm);
int mips_debug (vm_instance_t * vm, int is_break);

#endif
