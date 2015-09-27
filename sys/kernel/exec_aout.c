#include <sys/param.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/inode.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/namei.h>
#include <sys/fs.h>
#include <sys/mount.h>
#include <sys/file.h>
#include <sys/resource.h>
#include <sys/exec.h>
#include <sys/exec_aout.h>
#include <sys/dir.h>
#include <sys/uio.h>
#include <machine/debug.h>

int exec_aout_check(struct exec_params *epp)
{
    int error;

    if (epp->hdr_len < sizeof(struct exec))
        return ENOEXEC;
    if (!(N_GETMID(epp->hdr.aout) == MID_ZERO &&
          N_GETFLAG(epp->hdr.aout) == 0))
        return ENOEXEC;

    switch (N_GETMAGIC(epp->hdr.aout)) {
    case OMAGIC:
        epp->hdr.aout.a_data += epp->hdr.aout.a_text;
        epp->hdr.aout.a_text = 0;
        break;
    default:
        printf("Bad a.out magic = %0o\n", N_GETMAGIC(epp->hdr.aout));
        return ENOEXEC;
    }

    /*
     * Save arglist
     */
    exec_save_args(epp);

    DEBUG("Exec file header:\n");
    DEBUG("a_midmag =  %#x\n", epp->hdr.aout.a_midmag);     /* magic number */
    DEBUG("a_text =    %d\n",  epp->hdr.aout.a_text);       /* size of text segment */
    DEBUG("a_data =    %d\n",  epp->hdr.aout.a_data);       /* size of initialized data */
    DEBUG("a_bss =     %d\n",  epp->hdr.aout.a_bss);        /* size of uninitialized data */
    DEBUG("a_reltext = %d\n",  epp->hdr.aout.a_reltext);    /* size of text relocation info */
    DEBUG("a_reldata = %d\n",  epp->hdr.aout.a_reldata);    /* size of data relocation info */
    DEBUG("a_syms =    %d\n",  epp->hdr.aout.a_syms);       /* size of symbol table */
    DEBUG("a_entry =   %#x\n", epp->hdr.aout.a_entry);      /* entry point */

    /*
     * Set up memory allocation
     */
    epp->text.vaddr = epp->heap.vaddr = NO_ADDR;
    epp->text.len = epp->heap.len = 0;

    epp->data.vaddr = (caddr_t)USER_DATA_START;
    epp->data.len = epp->hdr.aout.a_data;
    epp->bss.vaddr = epp->data.vaddr + epp->data.len;
    epp->bss.len = epp->hdr.aout.a_bss;
    epp->heap.vaddr = epp->bss.vaddr + epp->bss.len;
    epp->heap.len = 0;
    epp->stack.len = SSIZE + roundup(epp->argbc + epp->envbc, NBPW) + (epp->argc + epp->envc+4)*NBPW;
    epp->stack.vaddr = (caddr_t)USER_DATA_END - epp->stack.len;

    /*
     * Allocate core at this point, committed to the new image.
     * TODO: What to do for errors?
     */
    exec_estab(epp);

    /* read in text and data */
    DEBUG("reading a.out image\n");
    error = rdwri (UIO_READ, epp->ip,
               (caddr_t)epp->data.vaddr, epp->hdr.aout.a_data,
               sizeof(struct exec) + epp->hdr.aout.a_text, IO_UNIT, 0);
    if (error)
        DEBUG("read image returned error=%d\n", error);
    if (error) {
        /*
         * Error - all is lost, when the old image is possible corrupt
         * and we could not load a new.
         */
        psignal (u.u_procp, SIGSEGV);
        return error;
    }

    exec_clear(epp);
    exec_setupstack(epp->hdr.aout.a_entry, epp);

    return 0;
}
