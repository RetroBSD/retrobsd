10 rem **************************************
20 rem *** Play the "blackjack" (21) game ***
30 rem **************************************
40 dim C(52): m = 1000
50 rem
60 rem Create deck of cards and shuffle it
70 rem
80 for i=0 to 51: c(i) = i: next i
90 for i=0 to 51: i1=rnd(52): i2=c(i): c(i)=c(i2): c(i2)=i1: next i
100 rem
110 rem Prompt for amount of bet (on this game)
120 rem
130 print "You have", m, " dollars"
140 input "How much do you wish to bet?", b: if b=0 then stop
150 lif b>m then print "You don't have enough money":goto 140
160 t = 0: d = 0
170 rem
180 rem Prompt PLAYER for another card
190 rem
200 print "Total:", t,:Input " Another card (Y/N)?", a$
210 if a$="n" then 380
220 lif a$<>"y" then print "Please answer y-Yes or n-No":goto 200
230 c = c(d): d = d + 1: gosub 530
240 c = c % 13: if c > 9 then c = 9
250 if c > 0 then 300
260 input "(1)one or (t)ten ?", a$
270 if a$="1" then 300
280 if a$<>"t" then 260
290 c = 9
300 t = t + c + 1
310 if t <= 21 then 200
320 print "You went over 21! - you LOSE!"
330 m = m - b: if m > 0 then 80
340 print "You went BUST!":end
350 rem
360 rem Play DEALER
370 rem
380 t1 = 0
390 c = c(d): d = d + 1: print "Dealer draws ",: gosub 530
400 c = c % 13: if c > 9 then c = 9
410 if c > 0 then 470
420 if t1 < 10 then 450
430 if (t1+10) > 23 then 460
440 if (t1+10) >= t then 450
450 c = 9
460 print "Dealer chooses", c+1
470 t1 = t1 + c + 1: print "Dealer totals", t1: if t1 < t then 390
480 lif t1 <= 21 then print "Dealer wins - You LOSE!": goto 330
490 print "Dealer loses - You WIN!!!": m = m + b: goto 80
500 rem
510 rem Subroutine to display text description of a card
520 rem
530 order 590
540 for a = 0 to c / 13: read a$: next a
550 order 600
560 for a = 0 to c % 13: read a1$:next a
570 print a1$, " of ", a$
580 return
590 data "Hearts", "Diamonds", "Clubs", "Spades"
600 data "Ace", "Two", "Three", "Four", "Five", "Six", "Seven"
610 data "Eight", "Nine", "Ten", "Jack", "Queen", "King"
