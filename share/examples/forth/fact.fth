\ Iterative factorial function.

." Defining fact function ... "
: fact  ( n -- n! )
	dup 2 < if drop 1 else
	dup begin 1- swap over * swap dup 1 = until
	drop then
; ." done." cr

." 1! = "  1  fact . cr
." 2! = "  2  fact . cr
." 3! = "  3  fact . cr
." 4! = "  4  fact . cr
." 5! = "  5  fact . cr
." 6! = "  6  fact . cr
." 7! = "  7  fact . cr
." 8! = "  8  fact . cr
." 9! = "  9  fact . cr
." 10! = " 10 fact . cr
." 11! = " 11 fact . cr
." 12! = " 12 fact . cr

halt
