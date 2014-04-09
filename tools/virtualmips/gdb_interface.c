/*
 * Copyright (C) 1996-1998 by the Board of Trustees
 *    of Leland Stanford Junior University.
 *
 * This file is part of the SimOS distribution.
 * See LICENSE file for terms of the license.
 *
 */

 /*
  * Copyright (C) yajin 2008 <yajinzhou@gmail.com >
  *
  * This file is part of the virtualmips distribution.
  * See LICENSE file for terms of the license.
  *
  */

/*GDB interface for virtualmips based on simos*/

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <assert.h>

#include "vm.h"
#include "mips.h"
#include "gdb_interface.h"
#include "utils.h"
#include "debug.h"

#define GDB_NUM_REGS 72         //general cpu register and cp0 reegister
#define VCONT "vCont"
#define QSYMBOL "qSymbol"
#define QC "qC"
#define QOFFSETS "qOffsets"

int delete_breakpoint (vm_instance_t * vm, m_uint32_t addr, int len)
{
    virtual_breakpoint_t *tmp, *prev = 0;

    tmp = vm->breakpoint_head;

    // addr = SIGN_EXTEND(32,addr);
    while (tmp) {
        if (tmp->addr == addr) {
            /*find it */
            if (prev)
                prev->next = tmp->next;

            if (vm->breakpoint_head == tmp)
                vm->breakpoint_head = tmp->next;

            if (vm->breakpoint_tail == tmp)
                vm->breakpoint_tail = prev;

            if (tmp->save)
                free (tmp->save);
            free (tmp);

            goto exit;
        }

        prev = tmp;
        tmp = tmp->next;
    }

  exit:
    return SUCCESS;
}

virtual_breakpoint_t *alloc_breakpoint (vm_instance_t * vm, m_uint32_t addr,
    char *save, int len)
{
    virtual_breakpoint_t *tmp;

    tmp = malloc (sizeof (virtual_breakpoint_t));
    if (!tmp)
        return NULL;

    memset (tmp, 0, sizeof (*tmp));
    tmp->addr = (addr);

    if (save) {
        tmp->save = malloc (strlen (save) + 1);
        strcpy (tmp->save, save);
    }
    //tmp->cpuno = cpuno;
    tmp->len = len;
    tmp->next = 0;

    return tmp;
}

int insert_breakpoint (vm_instance_t * vm, virtual_breakpoint_t * new)
{
    virtual_breakpoint_t *tmp;

    if (vm->breakpoint_head == 0) {
        vm->breakpoint_tail = vm->breakpoint_head = new;
    } else {
        tmp = vm->breakpoint_head;

        while (tmp) {
            if (tmp->addr == new->addr) {
                /*the same one */
                if (new->save)
                    free (new->save);
                free (new);
                goto exit;
            }

            tmp = tmp->next;
        }

        vm->breakpoint_tail->next = new;
        vm->breakpoint_tail = new;
    }

  exit:
    return SUCCESS;
}

static int fromhex (int a)
{
    if (a >= '0' && a <= '9')
        return a - '0';
    else if (a >= 'a' && a <= 'f')
        return a - 'a' + 10;
    else {
        fprintf (stderr, "Request contains invalid hex digit : %s %d\n",
            __FILE__, __LINE__);
        return 0;
    }
}

/* Convert number NIB to a hex digit.  */

static int tohex (int nib)
{
    if (nib < 10)
        return '0' + nib;
    else
        return 'a' + nib - 10;
}

#if 0
static int bin2hex (const char *bin, char *hex, int count)
{
    int i;
    /* May use a length, or a nul-terminated string as input. */
    if (count == 0)
        count = strlen (bin);

    for (i = 0; i < count; i++) {
        *hex++ = tohex ((*bin >> 4) & 0xf);
        *hex++ = tohex (*bin++ & 0xf);
    }
    *hex = 0;
    return i;
}

#endif

static void convert_int_to_ascii (char *from, char *to, int n)
{
    int nib;
    char ch;
    while (n--) {
        ch = *from++;
        nib = ((ch & 0xf0) >> 4) & 0x0f;
        *to++ = tohex (nib);
        nib = ch & 0x0f;
        *to++ = tohex (nib);
    }
    *to++ = 0;
}

/*****************************************************************
 * readchar
 * Returns next char from remote GDB.  -1 if error.
 *****************************************************************/
static int readchar (int fd)
{
    static char buf[BUFSIZ];
    static int bufcnt = 0;
    static char *bufp;

    if (bufcnt-- > 0)
        return *bufp++ & 0x7f;

    bufcnt = read (fd, buf, BUFSIZ);
    if (bufcnt <= 0) {
        printf ("Simdebug readchar");
        return -1;
    }

    bufp = buf;
    bufcnt--;
    return *bufp++ & 0x7f;
}

/*****************************************************************
 * getpkt
 *
 * Read a packet from the remote machine, with error checking,
 * and store it in BUF.  Returns length of packet, or negative if error.
 *****************************************************************/
static int getpkt (int fd, char *buf)
{
    char *bp;
    unsigned char csum, c1, c2;
    int c;

    bp = buf;
    while (1) {
        csum = 0;

        while (1) {
            c = readchar (fd);
            if (c == '$')
                break;
            if (c < 0)
                goto errout;
        }

        bp = buf;
        while (1) {
            c = readchar (fd);
            if (c < 0)
                goto errout;

            if (c == '#')
                break;

            *bp++ = c;
            csum += c;
        }
        *bp = 0;

        c = readchar (fd);
        if (c < 0)
            goto errout;
        c1 = fromhex (c);

        c = readchar (fd);
        if (c < 0)
            goto errout;
        c2 = fromhex (c);

        if (csum == (c1 << 4) + c2)
            break;

        if (write (fd, "-", 1) < 0)
            return -1;
    }

    if (write (fd, "+", 1) < 0)
        return -1;
    return bp - buf;

  errout:
    return -1;
}

static void write_ok (char *buf)
{
    buf[0] = 'O';
    buf[1] = 'K';
    buf[2] = '\0';
}

static void write_enn (char *buf)
{
    buf[0] = 'E';
#if 0
    buf[1] = 'N';
    buf[2] = 'N';
#endif

    buf[1] = '0';
    buf[2] = '1';
    buf[3] = '\0';
}

/* Send a packet to the remote machine, with error checking.
   The data of the packet is in BUF.  Returns >= 0 on success, -1 otherwise. */

static int putpkt (int fd, char *buf)
{
    //printf("putpkt\n");
    int i;
    unsigned char csum = 0;
    char buf2[2000];
    char buf3[1];
    int cnt = strlen (buf);
    char *p;

    /* Copy the packet into buffer BUF2, encapsulating it
     * and giving it a checksum.  */

    p = buf2;
    *p++ = '$';

    for (i = 0; i < cnt; i++) {
        csum += buf[i];
        *p++ = buf[i];
    }

    *p++ = '#';
    *p++ = tohex ((csum >> 4) & 0xf);
    *p++ = tohex (csum & 0xf);

    /* Send it over and over[A until we get a positive ack.  */

    do {
        int cc;

        if (write (fd, buf2, p - buf2) != p - buf2) {
            printf ("putpkt(write)\n");
            goto errout;
        }

        cc = read (fd, buf3, 1);
        if (cc <= 0) {
            printf ("putpkt(read)\n");
            goto errout;
        }
    }
    while (buf3[0] != '+');

    return 1;                   /* Success! */

  errout:
    return -1;
}

static void attach_gdb (vm_instance_t * vm)
{
    struct sockaddr_in sockaddr1;
    unsigned int tmp;
    int gdb_fd;

    if (!vm->gdb_debug_from_poll)
        printf ("Waiting for gdb on port %d.\n", vm->gdb_port);

    tmp = sizeof (sockaddr1);
    gdb_fd =
        accept (vm->gdb_listen_sock, (struct sockaddr *) &sockaddr1, &tmp);
    vm->gdb_interact_sock = gdb_fd;
    if (gdb_fd == -1) {
        printf ("Debugger accept failed\n");
        return;
    }

    if (!vm->gdb_debug_from_poll)
        printf ("GDB attached. Enjoy yourself!\n");

    /* Tell TCP not to delay small packets.  This greatly speeds up
     * interactive response. */

    tmp = 1;
    setsockopt (gdb_fd, SOL_SOCKET, SO_KEEPALIVE, (char *) &tmp,
        sizeof (tmp));

    tmp = 1;
    setsockopt (gdb_fd, IPPROTO_TCP, TCP_NODELAY, (char *) &tmp,
        sizeof (tmp));

    // int save_fcntl_flags;
#if defined(F_SETFL) && defined (FASYNC)
    save_fcntl_flags = fcntl (gdb_fd, F_GETFL, 0);
    fcntl (gdb_fd, F_SETFL, save_fcntl_flags | FASYNC);
#if defined (F_SETOWN)
    fcntl (gdb_fd, F_SETOWN, getpid ());
#endif
#endif

    signal (SIGIO, SIG_IGN);

    // return gdb_fd;
}

#define MAX_XFER (8*1024)
#define WORD_SZ  4
#define BYTE_SZ   1

int debug_read_memory (cpu_mips_t * cpu, m_uint32_t vaddr, unsigned int len,
    char *buf)
{
    //void *haddr=NULL;
    //m_uint32_t exc;
    //m_uint32_t data;
    //m_uint8_t has_set_value=FALSE;

    char readout_buf[MAX_XFER];
    if (len > MAX_XFER) {
        fprintf (stderr, "read_memory: too many bytes requested");
    }
    assert (len <= MAX_XFER);

    int reallen = len;
    char *cur = readout_buf;
    int i = 0;

    //We read one byte once to avoid align problem
    for (i = 0; i < (reallen / BYTE_SZ); i++) {
        cpu->mips_mts_gdb_lb (cpu, vaddr, cur);

        cur += BYTE_SZ;
        vaddr += BYTE_SZ;
    }

    convert_int_to_ascii (readout_buf, buf, len);
    return SUCCESS;

}

int forced_inline cpu_hit_breakpoint (vm_instance_t * vm, m_uint32_t vaddr)
{
    virtual_breakpoint_t *tmp = vm->breakpoint_head;
    static char cmd_buf[BUFSIZ + 1];

    while (tmp) {
        if (tmp->addr == vaddr) {
            sprintf (cmd_buf, "S%02x", SIGTRAP);
            if (putpkt (vm->gdb_interact_sock, cmd_buf) < 0) {
                /* the connection was terminated prematurely.  Reset */
                close (vm->gdb_interact_sock);
                vm->gdb_interact_sock = -1;
                vm->mipsy_debug_mode = 0;       //we do not debug anymore
                printf ("Remote debugger connection lost, continuing...\n");
                return FAILURE;
            }
            return SUCCESS;
        }
        tmp = tmp->next;
    }
    return FAILURE;
}

/* Convert hex digit A to a number.  */

Simdebug_result Simdebug_run (vm_instance_t * vm, int sig)
{
    static char cmd_buf[BUFSIZ + 1];
    char *b;
    int i = 0;
    Simdebug_result ret = SD_CONTINUE;

    if (vm->gdb_interact_sock <= 0) {
        attach_gdb (vm);
        if (vm->gdb_interact_sock < 0) {
            return SD_CONTINUE; /* could not get debugger */
        }
        vm->gdb_debug_from_poll = 0;    /* whether or not we entered from poll doesn't
                                         * matter this time, but we'd better reset it
                                         * before next time this function runs
                                         */
    } else {
        /* reestablish an old connection.  We might get here either
         * for internal reasons (hitting a breakpoint) or for external
         * reasons (gdb sent us a ^C).  If external, we need to swallow
         * the break packet.  Then in either case, let GDB know why
         * we stopped.
         */

        if (vm->gdb_debug_from_poll) {
            vm->gdb_debug_from_poll = 0;
            int cc;
            char c;

            cc = read (vm->gdb_interact_sock, &c, 1);

            if (cc != 1 || c != '\003') {
                fprintf (stderr, "input_interrupt, cc = %d c = %d\n", cc, c);
                goto errout;
            }

            sig = SIGINT;
            /* Inform the remote debugger we have entered debugging mode. */
            // sprintf (cmd_buf, "S%02x%02x", sig, debug_cpuno);
            sprintf (cmd_buf, "S%02x", sig);
            if (putpkt (vm->gdb_interact_sock, cmd_buf) < 0)
                goto errout;

        }
    }

    while (1) {
        char ch;
        i = 0;
        int pkg_len = 0;

        if ((pkg_len = getpkt (vm->gdb_interact_sock, cmd_buf)) <= 0)
            goto errout;
        ch = cmd_buf[i++];
        SIM_DEBUG (('g', "gdb command %c \n", ch));
        //printf("---\n gdb command %s\n",cmd_buf);

        switch (ch) {
        case 'c':
            ret = SD_CONTINUE;
            goto out;
        case 'g':              //read register
            {
                m_uint32_t r = 0;
                b = cmd_buf;
                for (i = 0; i < GDB_NUM_REGS; i++) {
                    //FIXME: find the mapping from register index to register of cpu and cpu0 of gdb
                    if (vm->boot_cpu->reg_get (vm->boot_cpu, i,
                            &r) == FAILURE)
                        r = 0;
                    /*
                     * flip the bytes around
                     */
                    r = htovm32 (r);    //gdb likes the data of target machine format
                    convert_int_to_ascii ((char *) &r, b, 4);

                    b += 8;
                }
                *b = 0;
                break;
            }

        case 'H':
            write_ok (cmd_buf);
            break;
        case 'm':
            {
                m_uint32_t memaddr;
                unsigned int len;

                if (sscanf (cmd_buf + 1, "%x,%x", &memaddr, &len) != 2) {
                    write_enn (cmd_buf);
                } else {

                    if (debug_read_memory (vm->boot_cpu, memaddr, len,
                            cmd_buf) != SUCCESS)
                        write_enn (cmd_buf);
                }
                break;
            }

        case 'p':
            {
                m_uint32_t r, index = 0;
                b = cmd_buf;
                if (sscanf (cmd_buf + 1, "%x", &index) != 1)
                    write_enn (cmd_buf);
                else {
                    // index=vmtoh32(index);
                    if (vm->boot_cpu->reg_get (vm->boot_cpu, index,
                            &r) == FAILURE)
                        r = 0;
                    /*
                     * flip the bytes around
                     */
                    r = htovm32 (r);    //gdb likes the data of target machine format
                    convert_int_to_ascii ((char *) &r, b, 4);
                    //*(r+4)=0;
                    //*(r+5)=0;
                    //*(r+6)=0;
                    //*(r+7)=0;

                }

            }
            break;

        case 'v':
            {
                char *cmd_tmp;

                cmd_tmp = cmd_buf;

                if (strncmp (cmd_buf, VCONT, strlen (VCONT)) != 0)
                    write_enn (cmd_buf);
                else {
                    cmd_tmp += strlen (VCONT);
                    if (';' == cmd_tmp[0]) {
                        cmd_tmp++;
                        if ('c' == cmd_tmp[0]) {
                            ret = SD_CONTINUE;
                            goto out;
                        } else if ('s' == cmd_tmp[0]) {
                            /*single step */
                            int cpuno;
                            cpuno = 0;

                            ret = cpuno;

                            goto out;
                        } else
                            write_enn (cmd_buf);
                    } else if ('?' == (cmd_tmp)[0]) {
                        /*query */
                        strcpy (cmd_buf, VCONT ";c");
                    } else
                        write_enn (cmd_buf);
                }
            }
            break;

        case 'q':
            {
                if (strncmp (cmd_buf, QSYMBOL, strlen (QSYMBOL)) == 0)
                    write_ok (cmd_buf);
                else if (strncmp (cmd_buf, QC, strlen (QC)) == 0) {
                    strcpy (cmd_buf, "QC0");
                } else if (strncmp (cmd_buf, QOFFSETS,
                        strlen (QOFFSETS)) == 0) {
                    strcpy (cmd_buf, "Text=0;Data=0;Bss=0");
                } else {
                    write_enn (cmd_buf);
                }

            }
            break;

        case 's':
        case 'S':
            ret = SD_NEXTI_ANYCPU;
            goto out;

        case 'z':
            {
                m_uint32_t addr;
                int len;
                if ('0' == (cmd_buf + 1)[0]) {
                    //memory breakpoint
                    if (sscanf (cmd_buf + 3, "%x,%x", &addr, &len) != 2) {

                        write_enn (cmd_buf);
                    }

                    else {
                        if (delete_breakpoint (vm, addr, len) != SUCCESS)
                            write_enn (cmd_buf);
                        else {

                            write_ok (cmd_buf);
                        }

                    }

                }

                else
                    write_enn (cmd_buf);

            }
            break;

        case 'Z':
            {
                m_uint32_t addr;
                int len;
                if ('0' == (cmd_buf + 1)[0]) {
                    //memory breakpoint
                    if (sscanf (cmd_buf + 3, "%x,%x", &addr, &len) != 2) {

                        write_enn (cmd_buf);

                    }

                    else {

                        virtual_breakpoint_t *tmp =
                            alloc_breakpoint (vm, addr, 0, len);
                        insert_breakpoint (vm, tmp);
                        write_ok (cmd_buf);

                    }
                } else
                    write_enn (cmd_buf);

            }
            break;
        case '?':
            sprintf (cmd_buf, "S%02x", sig);
            break;
        default:
            write_enn (cmd_buf);
            break;

        }
        //printf("send %s\n",cmd_buf);
        if (putpkt (vm->gdb_interact_sock, cmd_buf) < 0)
            goto errout;

    }
out:
    return ret;
errout:
    /* the connection was terminated prematurely.  Reset */
    close (vm->gdb_interact_sock);
    vm->gdb_interact_sock = -1;
    printf ("Remote debugger connection lost, continuing...\n");
    return SD_CONTINUE;
}

void bad_memory_access_gdb (vm_instance_t * vm)
{
    static char cmd_buf[BUFSIZ + 1];

    vm->mipsy_break_nexti = MIPS_BREAKANYCPU;
    sprintf (cmd_buf, "S%02x", SIGTRAP);
    if (putpkt (vm->gdb_interact_sock, cmd_buf) < 0) {
        /* the connection was terminated prematurely.  Reset */
        close (vm->gdb_interact_sock);
        vm->gdb_interact_sock = -1;
        vm->mipsy_debug_mode = 0;       //we do not debug anymore
        printf ("Remote debugger connection lost, continuing...\n");
    }
}
