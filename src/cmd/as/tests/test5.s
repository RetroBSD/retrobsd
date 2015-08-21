        .set    reorder
x:
        add     $2, $16, $31
        b       x
        sub	$31, $11, $26

        add     $2, $16, $31
        beqz    $3, x
        sub	$31, $11, $26

        add     $2, $16, $31
        j       x
        sub	$31, $11, $26

        and     $31, $3, $18
        b       y
        or	$31, $8, $23

        and     $31, $3, $18
        beqz    $2, y
        or	$31, $8, $23

        and     $31, $3, $18
        j       y
        or	$31, $8, $23
y:
