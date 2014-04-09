 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *
  * This file is part of the virtualmips distribution.
  * See LICENSE file for terms of the license.
  *
  */

/*
	A simple interface for GDB.
	Based on SimOS.
 */

#define _GNU_SOURCE
#include <sys/signal.h>
#include <string.h>

#include "vm.h"
#include "mips.h"
#include "utils.h"
#include "debug.h"

int forced_inline mips_debug (vm_instance_t * vm, int is_break)
{
    Simdebug_result res;

    res = Simdebug_run (vm, is_break ? SIGTRAP : SIGUSR2);

    switch (res) {
    case SD_CONTINUE:
        vm->mipsy_break_nexti = MIPS_NOBREAK;
        vm->mipsy_debug_mode = 1;
        break;
    case SD_NEXTI_ANYCPU:
        vm->mipsy_break_nexti = MIPS_BREAKANYCPU;
        vm->mipsy_debug_mode = 1;
        break;
    default:
        vm->mipsy_break_nexti = res;
        vm->mipsy_debug_mode = 1;
        break;
    }

    return 0;

}

void vm_debug_init (vm_instance_t * vm)
{
    struct sockaddr_in sockaddr;
    int tmp;

    if (vm->gdb_debug != 1)
        return;
    vm->mipsy_debug_mode = 1;
    vm->mipsy_break_nexti = -1;
    vm->gdb_interact_sock = -1;
    vm->gdb_debug_from_poll = 0;

    vm->gdb_listen_sock = socket (PF_INET, SOCK_STREAM, 0);
    if (vm->gdb_listen_sock < 0) {
        fprintf (stderr, "Can't open debug socket. Run without gdb debug\n");
        vm->gdb_debug = 0;
        return;
    }

    /* Allow rapid reuse of this port. */
    tmp = 1;
    if (setsockopt (vm->gdb_listen_sock, SOL_SOCKET, SO_REUSEADDR,
            (char *) &tmp, sizeof (tmp)) < 0) {
        printf ("simdebug setsockopt SO_REUSEADDR");
        /* Not fatal */
    }
    bzero ((char *) &sockaddr, sizeof (struct sockaddr_in));

    sockaddr.sin_family = PF_INET;
    sockaddr.sin_port = htons (vm->gdb_port);
    sockaddr.sin_addr.s_addr = INADDR_ANY;

    while (bind (vm->gdb_listen_sock, (struct sockaddr *) &sockaddr,
            sizeof (sockaddr)) || listen (vm->gdb_listen_sock, 1)) {
        vm->gdb_port++;
        sockaddr.sin_port = htons (vm->gdb_port);
    }

    vm->breakpoint_head = 0;
    vm->breakpoint_tail = 0;

}
