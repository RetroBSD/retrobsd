(define (square n) (* n n))

(define (divides? a b)
  (= (remainder b a) 0))

(define (find-divisor n test-divisor)
  (cond ((> (square test-divisor) n) n)
        ((divides? test-divisor n) test-divisor)
        (else (find-divisor n (+ test-divisor 1)))))

(define (smallest-divisor n)
  (find-divisor n 2))

(define (prime? n)
  (= n (smallest-divisor n)))

(define (t n)
  (display n)
  (if (prime? n)
    (display '-prime)
    (display '-compound))
  (newline))

(t 2)
(t 20)
(t 23)
(t 65)
(t 67)
(t 2011)
