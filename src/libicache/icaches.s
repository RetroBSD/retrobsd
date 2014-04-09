 # /*
 # Copyright (c) 2013, Alexey Frunze
 # All rights reserved.
 #
 # Redistribution and use in source and binary forms, with or without
 # modification, are permitted provided that the following conditions are met: 
 #
 # 1. Redistributions of source code must retain the above copyright notice, this
 #    list of conditions and the following disclaimer. 
 # 2. Redistributions in binary form must reproduce the above copyright notice,
 #    this list of conditions and the following disclaimer in the documentation
 #    and/or other materials provided with the distribution. 
 #
 # THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 # ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 # WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 # DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 # ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 # (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 # LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 # ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 # (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 # SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 #
 # The views and conclusions contained in the software and documentation are those
 # of the authors and should not be interpreted as representing official policies, 
 # either expressed or implied, of the FreeBSD Project.
 # */
 #
 # /*****************************************************************************/
 # /*                                                                           */
 # /*                               MIPS icache                                 */
 # /*                                                                           */
 # /*****************************************************************************/

        .text

        .extern _gp
        .extern _icRegs_
        .extern _icHostRegs_
        .extern _icmain_

        .globl  _icstart_
        .type   _icstart_, @function
_icstart_:
        la      $28, _gp
        subu    $29, $29, 1024 # allocate 1K of stack for us
        sw      $29, _icRegs_ + 4*29 # leave the rest to the program
        addu    $29, $29, 1024 - 16
        j       _icmain_

        .globl  _icDoSysCall_
        .type   _icDoSysCall_, @function
_icDoSysCall_:
        sw      $4, $_icsyscall_ # patch the syscall instruction
        # We need to write back the data cache and invalidate the instruction
        # cache for the location of the modified instruction before we can
        # actually execute it
        la      $4, $_icsyscall_
        synci   0($4) # does nothing on MIPS32 M4K since it has no caches

 #      sw      $1, _icHostRegs_ + 4*1 # ar
 #      lw      $1, _icRegs_     + 4*1 #
 #      sw      $2, _icHostRegs_ + 4*2 # v0
        lw      $2, _icRegs_     + 4*2 #
 #      sw      $3, _icHostRegs_ + 4*3 # v1
        lw      $3, _icRegs_     + 4*3 #
 #      sw      $4, _icHostRegs_ + 4*4 # a0
        lw      $4, _icRegs_     + 4*4 #
 #      sw      $5, _icHostRegs_ + 4*5 # a1
        lw      $5, _icRegs_     + 4*5 #
 #      sw      $6, _icHostRegs_ + 4*6 # a2
        lw      $6, _icRegs_     + 4*6 #
 #      sw      $7, _icHostRegs_ + 4*7 # a3
        lw      $7, _icRegs_     + 4*7 #
 #      sw      $8, _icHostRegs_ + 4*8 # t0
        lw      $8, _icRegs_     + 4*8 #
 #      sw      $9, _icHostRegs_ + 4*9 # t1
        lw      $9, _icRegs_     + 4*9 #
 #      sw      $10, _icHostRegs_ + 4*10 # t2
        lw      $10, _icRegs_     + 4*10 #
 #      sw      $11, _icHostRegs_ + 4*11 # t3
        lw      $11, _icRegs_     + 4*11 #
 #      sw      $12, _icHostRegs_ + 4*12 # t4
        lw      $12, _icRegs_     + 4*12 #
 #      sw      $13, _icHostRegs_ + 4*13 # t5
        lw      $13, _icRegs_     + 4*13 #
 #      sw      $14, _icHostRegs_ + 4*14 # t6
        lw      $14, _icRegs_     + 4*14 #
 #      sw      $15, _icHostRegs_ + 4*15 # t7
        lw      $15, _icRegs_     + 4*15 #
        sw      $16, _icHostRegs_ + 4*16
        lw      $16, _icRegs_     + 4*16
        sw      $17, _icHostRegs_ + 4*17
        lw      $17, _icRegs_     + 4*17
        sw      $18, _icHostRegs_ + 4*18
        lw      $18, _icRegs_     + 4*18
        sw      $19, _icHostRegs_ + 4*19
        lw      $19, _icRegs_     + 4*19
        sw      $20, _icHostRegs_ + 4*20
        lw      $20, _icRegs_     + 4*20
        sw      $21, _icHostRegs_ + 4*21
        lw      $21, _icRegs_     + 4*21
        sw      $22, _icHostRegs_ + 4*22
        lw      $22, _icRegs_     + 4*22
        sw      $23, _icHostRegs_ + 4*23
        lw      $23, _icRegs_     + 4*23
 #      sw      $24, _icHostRegs_ + 4*24 # t8
        lw      $24, _icRegs_     + 4*24 #
 #      sw      $25, _icHostRegs_ + 4*25 # t9
        lw      $25, _icRegs_     + 4*25 #
 #      sw      $26, _icHostRegs_ + 4*26 # k0
 #      lw      $26, _icRegs_     + 4*26 #
 #      sw      $27, _icHostRegs_ + 4*27 # k1
 #      lw      $27, _icRegs_     + 4*27 #
        sw      $28, _icHostRegs_ + 4*28 # gp
        lw      $28, _icRegs_     + 4*28 #
        sw      $29, _icHostRegs_ + 4*29 # sp
 #       lw      $29, _icRegs_     + 4*29 #
        # Make sure sp is updated "atomically" and not part by part
        .set    noat
        lw      $1, _icRegs_     + 4*29 #
        move    $29, $1 #
        .set    at
        sw      $30, _icHostRegs_ + 4*30
        lw      $30, _icRegs_     + 4*30
 #      sw      $31, _icHostRegs_ + 4*31 # ra
 #      lw      $31, _icRegs_     + 4*31 # ra

$_icsyscall_:
        # This instruction gets patched, so it can have
        # the requested system call number embedded in it
        syscall

        # RetroBSD may advance PC on returning from a syscall handler,
        # skipping 2 instructions that follow the syscall instruction.
        # Those 2 instructions typically set C's errno variable and
        # are either executed on error or skipped on success.
        # Account for this peculiarity.
        j       $_icsyscall_error_
        nop
$_icsyscall_success_:

 #      sw      $1, _icRegs_     + 4*1 # ar
 #      lw      $1, _icHostRegs_ + 4*1 #
        sw      $2, _icRegs_     + 4*2 # v0
 #      lw      $2, _icHostRegs_ + 4*2 #
        sw      $3, _icRegs_     + 4*3 # v1
 #      lw      $3, _icHostRegs_ + 4*3 #
        sw      $4, _icRegs_     + 4*4 # a0
 #      lw      $4, _icHostRegs_ + 4*4 #
        sw      $5, _icRegs_     + 4*5 # a1
 #      lw      $5, _icHostRegs_ + 4*5 #
        sw      $6, _icRegs_     + 4*6 # a2
 #      lw      $6, _icHostRegs_ + 4*6 #
        sw      $7, _icRegs_     + 4*7 # a3
 #      lw      $7, _icHostRegs_ + 4*7 #
        sw      $8, _icRegs_     + 4*8 # t0
 #      lw      $8, _icHostRegs_ + 4*8 #
        sw      $9, _icRegs_     + 4*9 # t1
 #      lw      $9, _icHostRegs_ + 4*9 #
        sw      $10, _icRegs_     + 4*10 # t2
 #      lw      $10, _icHostRegs_ + 4*10 #
        sw      $11, _icRegs_     + 4*11 # t3
 #      lw      $11, _icHostRegs_ + 4*11 #
        sw      $12, _icRegs_     + 4*12 # t4
 #      lw      $12, _icHostRegs_ + 4*12 #
        sw      $13, _icRegs_     + 4*13 # t5
 #      lw      $13, _icHostRegs_ + 4*13 #
        sw      $14, _icRegs_     + 4*14 # t6
 #      lw      $14, _icHostRegs_ + 4*14 #
        sw      $15, _icRegs_     + 4*15 # t7
 #      lw      $15, _icHostRegs_ + 4*15 #
        sw      $16, _icRegs_     + 4*16
        lw      $16, _icHostRegs_ + 4*16
        sw      $17, _icRegs_     + 4*17
        lw      $17, _icHostRegs_ + 4*17
        sw      $18, _icRegs_     + 4*18
        lw      $18, _icHostRegs_ + 4*18
        sw      $19, _icRegs_     + 4*19
        lw      $19, _icHostRegs_ + 4*19
        sw      $20, _icRegs_     + 4*20
        lw      $20, _icHostRegs_ + 4*20
        sw      $21, _icRegs_     + 4*21
        lw      $21, _icHostRegs_ + 4*21
        sw      $22, _icRegs_     + 4*22
        lw      $22, _icHostRegs_ + 4*22
        sw      $23, _icRegs_     + 4*23
        lw      $23, _icHostRegs_ + 4*23
        sw      $24, _icRegs_     + 4*24 # t8
 #      lw      $24, _icHostRegs_ + 4*24 #
        sw      $25, _icRegs_     + 4*25 # t9
 #      lw      $25, _icHostRegs_ + 4*25 #
 #      sw      $26, _icRegs_     + 4*26 # k0
 #      lw      $26, _icHostRegs_ + 4*26 #
 #      sw      $27, _icRegs_     + 4*27 # k1
 #      lw      $27, _icHostRegs_ + 4*27 #
        sw      $28, _icRegs_     + 4*28 # gp
        lw      $28, _icHostRegs_ + 4*28 #
        sw      $29, _icRegs_     + 4*29 # sp
 #       lw      $29, _icHostRegs_ + 4*29 #
        # Make sure sp is updated "atomically" and not part by part
        .set    noat
        lw      $1, _icHostRegs_ + 4*29 #
        move    $29, $1
        .set    at
        sw      $30, _icRegs_     + 4*30
        lw      $30, _icHostRegs_ + 4*30
 #      sw      $31, _icRegs_     + 4*31 # ra
 #      lw      $31, _icHostRegs_ + 4*31 #

        li      $2, 2 # success, 2 instructions skipped
        j       $31

$_icsyscall_error_:
 #      sw      $1, _icRegs_     + 4*1 # ar
 #      lw      $1, _icHostRegs_ + 4*1 #
        sw      $2, _icRegs_     + 4*2 # v0
 #      lw      $2, _icHostRegs_ + 4*2 #
        sw      $3, _icRegs_     + 4*3 # v1
 #      lw      $3, _icHostRegs_ + 4*3 #
        sw      $4, _icRegs_     + 4*4 # a0
 #      lw      $4, _icHostRegs_ + 4*4 #
        sw      $5, _icRegs_     + 4*5 # a1
 #      lw      $5, _icHostRegs_ + 4*5 #
        sw      $6, _icRegs_     + 4*6 # a2
 #      lw      $6, _icHostRegs_ + 4*6 #
        sw      $7, _icRegs_     + 4*7 # a3
 #      lw      $7, _icHostRegs_ + 4*7 #
        sw      $8, _icRegs_     + 4*8 # t0
 #      lw      $8, _icHostRegs_ + 4*8 #
        sw      $9, _icRegs_     + 4*9 # t1
 #      lw      $9, _icHostRegs_ + 4*9 #
        sw      $10, _icRegs_     + 4*10 # t2
 #      lw      $10, _icHostRegs_ + 4*10 #
        sw      $11, _icRegs_     + 4*11 # t3
 #      lw      $11, _icHostRegs_ + 4*11 #
        sw      $12, _icRegs_     + 4*12 # t4
 #      lw      $12, _icHostRegs_ + 4*12 #
        sw      $13, _icRegs_     + 4*13 # t5
 #      lw      $13, _icHostRegs_ + 4*13 #
        sw      $14, _icRegs_     + 4*14 # t6
 #      lw      $14, _icHostRegs_ + 4*14 #
        sw      $15, _icRegs_     + 4*15 # t7
 #      lw      $15, _icHostRegs_ + 4*15 #
        sw      $16, _icRegs_     + 4*16
        lw      $16, _icHostRegs_ + 4*16
        sw      $17, _icRegs_     + 4*17
        lw      $17, _icHostRegs_ + 4*17
        sw      $18, _icRegs_     + 4*18
        lw      $18, _icHostRegs_ + 4*18
        sw      $19, _icRegs_     + 4*19
        lw      $19, _icHostRegs_ + 4*19
        sw      $20, _icRegs_     + 4*20
        lw      $20, _icHostRegs_ + 4*20
        sw      $21, _icRegs_     + 4*21
        lw      $21, _icHostRegs_ + 4*21
        sw      $22, _icRegs_     + 4*22
        lw      $22, _icHostRegs_ + 4*22
        sw      $23, _icRegs_     + 4*23
        lw      $23, _icHostRegs_ + 4*23
        sw      $24, _icRegs_     + 4*24 # t8
 #      lw      $24, _icHostRegs_ + 4*24 #
        sw      $25, _icRegs_     + 4*25 # t9
 #      lw      $25, _icHostRegs_ + 4*25 #
 #      sw      $26, _icRegs_     + 4*26 # k0
 #      lw      $26, _icHostRegs_ + 4*26 #
 #      sw      $27, _icRegs_     + 4*27 # k1
 #      lw      $27, _icHostRegs_ + 4*27 #
        sw      $28, _icRegs_     + 4*28 # gp
        lw      $28, _icHostRegs_ + 4*28 #
        sw      $29, _icRegs_     + 4*29 # sp
 #       lw      $29, _icHostRegs_ + 4*29 #
        # Make sure sp is updated "atomically" and not part by part
        .set    noat
        lw      $1, _icHostRegs_ + 4*29 #
        move    $29, $1 #
        .set    at
        sw      $30, _icRegs_     + 4*30
        lw      $30, _icHostRegs_ + 4*30
 #      sw      $31, _icRegs_     + 4*31 # ra
 #      lw      $31, _icHostRegs_ + 4*31 #

        li      $2, 0 # failure, 0 instructions skipped
        j       $31
