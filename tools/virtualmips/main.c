/*
 * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
 *
 * This file is part of the virtualmips distribution.
 * See LICENSE file for terms of the license.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>

#include "utils.h"
#include "mips.h"
#include "device.h"
#include "vm.h"
#include "mips_exec.h"
#include "vp_timer.h"
#include "crc.h"
#include "net_io.h"

#define VERSION  "0.06-retrobsd"

void signal_gen_handler (int sig)
{
    switch (sig) {
    case SIGHUP:
        /* For future use */
        break;

    case SIGQUIT:
        printf ("\n--- Stop simulation\n");
        /*do not worry, exit will release all resource */
        exit (EXIT_SUCCESS);
        break;

    case SIGINT:
        /* In theory, this shouldn't happen thanks to VTTY settings */
        break;

    default:
        fprintf (stderr, "Unhandled signal %d\n", sig);
    }
}

/* Setups signals */
static void setup_signals (void)
{
    struct sigaction act;

    memset (&act, 0, sizeof (act));
    act.sa_handler = signal_gen_handler;
    act.sa_flags = SA_RESTART;
    sigaction (SIGHUP, &act, NULL);
    sigaction (SIGQUIT, &act, NULL);
    sigaction (SIGINT, &act, NULL);
}

int main (int argc, char *argv[])
{
    vm_instance_t *vm;
    char *configure_filename = NULL;

    printf ("VirtualMIPS (version %s)\n", VERSION);
    printf ("Copyright (c) 2008 yajin, 2011 vak.\n");
    printf ("Build date: %s %s\n\n", __DATE__, __TIME__);

    /* Initialize CRC functions */
    crc_init ();

    /* Initialize VTTY code */
    vtty_init ();

    /* Create the default instance */
    vm = create_instance (configure_filename);
    if (!vm)
        return EXIT_FAILURE;

    /* Set seed for random value. */
    srand ((int) time (0));

    setup_signals ();
    init_timers ();
    if (init_instance (vm) < 0) {
        fprintf (stderr, "Unable to initialize instance.\n");
        return EXIT_FAILURE;
    }

    /* We get here, because the cpu is not running now. */
    vm_monitor (vm);

    printf ("VM shut down\n");
    return (0);
}
