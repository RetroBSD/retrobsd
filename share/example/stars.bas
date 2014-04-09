10 INPUT "Your name: ", U$
20 PRINT "Hello ", U$
30 INPUT "How many stars do you want: ", N
40 S$ = ""
50 FOR I = 1 TO N
60 S$ = S$ + "*"
70 NEXT I
80 PRINT S$
90 INPUT "Do you want more stars? ", A$
100 IF A$ = "" THEN GOTO 90
110 IF A$ = "Y" THEN GOTO 30
120 IF A$ = "y" THEN GOTO 30
130 PRINT "Goodbye ", U$
140 END
