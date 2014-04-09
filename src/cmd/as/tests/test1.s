#        .set    noreorder
        add     $2, $16, $31
        addi    $2, $3, 123
        addiu   $3, $4, 234
        addu    $2, $31, $17
        and     $31, $3, $18
        andi    $4, $5, 345
        b       .+0x14
        bal     .-0x28
        beq     $3, $2, .+4
        beql	$2, $3, .-8
        bgez	$4, .+12
        bgezal	$5, .-16
        bgezall	$6, .+20
        bgezl	$7, .-24
        bgtz	$8, .+28
        bgtzl	$9, .-32
        blez	$10, .+36
        blezl	$11, .-40
        bltz	$12, .+44
        bltzal	$13, .-48
        bltzall	$14, .+52
        bltzl	$15, .-56
        bne	$16, $17, .-60
        bnel	$17, $18, .+64
        break	123
        clo	$3, $2
        clz	$2, $3
        deret
        di	$4
        div	$5, $6
        divu	$6, $7
        ehb
        ei	$8
        eret
        ext	$9, $10, 12, 5
        ins	$10, $11, 13, 6
        j	.+0x3c
        jal	.-0x40
        jalr	$12, $13
        jalr.hb	$14, $15
        jr	$16
        jr.hb	$17
        lb	$18, 123($19)
        lbu	$20, 321($21)
        lh	$22, 234($23)
        lhu	$24, 432($25)
        ll	$26, 468($27)
        lui	$5, 987
        lw	$28, 864($29)
        lwl	$30, 124($31)
        lwr	$3, 420($2)
        madd	$3, $4
        maddu	$5, $6
        mfc0	$7, $8, 7
        mfhi	$9
        mflo	$10
        move	$11, $12
        movn	$19, $31, $4
        movz	$31, $5, $20
        msub	$13, $14
        msubu	$15, $16
        mtc0	$17, $18, 5
        mthi	$19
        mtlo	$20
        mul	$6, $21, $31
        mult	$21, $22
        multu	$23, $24
        nop
        nor	$22, $31, $7
        or	$31, $8, $23
        ori	$5, $6, 456
        rdhwr	$25, $26
        rdpgpr	$27, $28
        ror	$29, $30, 27
        rorv	$31, $3, $2
        sb	$3, 123($4)
        sc	$5, 324($6)
        sdbbp	234
        seb	$7, $8
        seh	$9, $10
        sh	$11, 432($12)
        sll	$13, $14, 15
        sllv	$15, $16, $17
        slt	$9, $24, $31
        slti	$6, $7, 567
        sltiu	$7, $8, 678
        sltu	$25, $31, $10
        sra	$18, $19, 9
        srav    $20, $21, $22
        srl	$23, $24, 25
        srlv	$25, $26, $27
        ssnop
        sub	$31, $11, $26
        subu	$12, $27, $31
        sw	$28, 426($29)
        swl	$30, 123($31)
        swr	$31, 321($3)
        sync    1
        syscall	654
        teq	$2, $3
        teqi	$4, .+4
        tge	$5, $6
        tgei	$7, .-8
        tgeiu	$8, .+12
        tgeu	$9, $10
        tlt	$11, $12
        tlti	$13, .-16
        tltiu	$14, .+20
        tltu	$15, $16
        tne	$17, $18
        tnei	$19, .-24
        wait	53
        wrpgpr  $20, $21
        wsbh	$22, $23
        xor	$28, $31, $13
        xori	$8, $9, 789
