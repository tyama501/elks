10 MODE 1
20 CLS
30 COLOR 1,0
40 FOR Y=0 TO 12 STEP 1
50 PLOT 290,207-Y
60 DRAW 348,207-Y
70 NEXT Y
80 COLOR 7,0
90 FOR Y=0 TO 6 STEP 1
100 READ A$
110 FOR X=0 TO 29 STEP 1
120 IF MID$(A$,X+1,1)<>"7" THEN GOTO 140
130 PLOT 2*X+290,207-2*Y
140 NEXT X
150 NEXT Y
160 COLOR 4,0
170 PLOT 288,209
180 DRAW 350,209
190 DRAW 350,193
200 DRAW 288,193
210 DRAW 288,209
220 STOP
230 DATA "777777777777777777777777777777"
240 DATA "777000007077777077707770000777"
250 DATA "777077777077777077077707770777"
260 DATA "777000007077777000777007077777"
270 DATA "777077777077777070777777707777"
280 DATA "777000007000007077007000007777"
290 DATA "777777777777777777777777777777"
