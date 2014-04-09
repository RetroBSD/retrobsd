#asm
#       csa09 Small C v1 comparison support
#       All are dyadic except for lneg.
.globl  eq
.globl  ne
.globl  lt
.globl  le
.globl  gt
.globl  ge
.globl  ult
.globl  ule
.globl  ugt
.globl  uge
.globl  lneg
.globl  bool
.globl  case
.globl  _Xstktop

eq:     cmpl    r0,4(sp)
        jeql    true
        jbr     false

ne:     cmpl    r0,4(sp)
        jneq    true
        jbr     false

lt:     cmpl    r0,4(sp)
        jgtr    true
        jbr     false

le:     cmpl    r0,4(sp)
        jgeq    true
        jbr     false

gt:     cmpl    r0,4(sp)
        jlss    true
        jbr     false

ge:     cmpl    r0,4(sp)
        jleq    true
        jbr     false

ult:    cmpl    r0,4(sp)
        jgtru   true
        jbr     false

ule:    cmpl    r0,4(sp)
        jgequ   true
        jbr     false

ugt:    cmpl    r0,4(sp)
        jlequ   true
        jbr     false

uge:    cmpl    r0,4(sp)
        jlssu   true
        jbr     false

lneg:   cmpl    r0,$0
        jeql    ltrue
        movl    $0,r0
        rsb
ltrue:  movl    $1,r0
        rsb

bool:   jsb     lneg
        jbr     lneg

true:   movl    $1,r0
        movl    (sp),r3
        addl2   $8,sp
        jmp     (r3)

false:  movl    $0,r0
        movl    (sp),r3
        addl2   $8,sp
        jmp     (r3)
_Xstktop:       movl    sp,r0
        rsb
#       Case jump, value is in r0, case table in (sp)
case:   movl    (sp)+,r1        # pick up case pointer
casl:
        movl    (r1)+,r2        # pick up value.
        movl    (r1)+,r3        # pick up label.
        bneq    notdef          # if not default, check it
        jmp     (r2)            # is default, go do it.
notdef: cmpl    r0,r2           # compare table value with switch value
        bneq    casl            # go for next table ent if not
        jmp     (r3)            # otherwise, jump to it.
#endasm
