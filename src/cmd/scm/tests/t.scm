123
-123
#o123
#o-123
#b123
#b-123
#x123
#x-123
#d123
#d-123
123.456e12
-123.456e-12
(if 1 2 3)
(if #f 2 3 4)
(cond (#f 2) (#f 1) (else 3))
(eval '(if #f 'x1 'x2 'x3))
(define memq (lambda (obj lst) (cond
    ((null? lst) #f)
    ((eq? obj (car lst)) lst)
    (else (memq obj (cdr lst))) )))
(memq 'b '(a b c d))
