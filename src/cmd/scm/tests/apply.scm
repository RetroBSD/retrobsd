(apply + '(1 2 3))
(define hypot (lambda (a b) (+ (* a a) (* b b))))
(apply hypot '(3 4))

(for-each (lambda (x y) (display x y) (newline))
    '(q w e r t y) '(1 2 3 4 5 6))
(map (lambda (n) (expt n n)) '(1 2 3 4 5 6))
(map + '(1 2 3) '(4 5 6))
