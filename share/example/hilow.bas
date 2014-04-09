10 rem ****************************
20 rem *** Play the HI/LOW game ***
30 rem ****************************
40 n = rnd(100)
50 c = 0
60 input "Guess a number? ", g
70 c = c+1
80 if g=n then 120
90 if g>n then print "lower"
100 if g<n then print "higher"
110 goto 60
120 print "You guessed it in", c, " tries!"
130 exit
