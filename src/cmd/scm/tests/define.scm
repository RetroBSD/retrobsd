(define p (lambda () (pair 2)))
(define pair (lambda (x) (cons x x)))
(p)
(current-environment)
(pair 'z)
(pair 'x)
(let ((y (+ 2 3))) (pair y))
(define x 5)
x
(set! x 6)
x
