\ @(#) t_pforth.fth 95/04/11 1.2
\ Test PForth
\
\ Copyright 1994 3DO, Phil Burk

anew tast_pforth.fth
decimal
echo @

\ put some stuff on stack to protect from underflow
111 222 333 444
variable TEST-DEPTH
variable TEST-PASSED
0 test-passed !
variable TEST-FAILED
0 test-failed !

: IS.OK? ( flag -- )
	depth 1- test-depth @ = not
	IF
		." ERROR !!! Stack depth changed." cr
		abort
	THEN
\
	IF
		echo @
		IF
			50 spaces  ." PASS" cr
		THEN
		1 test-passed +!
	ELSE
		50 spaces  ." ERROR !!! Test failed." cr
		." Test: " source type cr
		1 test-failed +!
	THEN
;

depth test-depth !
echo off

\ ==========================================================
\ test is.ok?
-1 is.ok?

\ bit manipulation
hex
F0F0CCAA 0F0F3355 and 0= is.ok?
12345678 0F0F0F0F and 02040608 = is.ok?
F0F0CCAA 0F0F3355 or FFFFFFFF = is.ok?
F0F0CCAA -1 xor 0F0F3355 = is.ok?
decimal

\ stack words
123 456 swap 123 = swap 456 = and is.ok?
55 66 77 rot 55 = rot 66 = rot 77 = and and is.ok?

\ test compiler

91827364 constant CON1
12345678 constant CON2

91827364 con1 = is.ok?

:NONAME con1 con2 + ; execute con1 con2 + = is.ok?

\ test CREATE
765 constant CR_CON
create CR-DATA  cr_con ,
cr-data @ cr_con = is.ok?

\ fetch and store
variable var1
$ 89abcdef var1 !   var1     @ $ 89abcdef = is.ok?
$ 89abcdef var1 !   var1    w@ $ 000089ab = is.ok?
$ 12345678 var1 !   var1    w@ $ 00001234 = is.ok?
$ 89abcdef var1 !   var1 2+ w@ $ 0000cdef = is.ok?
$ 12345678 var1 !   var1 2+ w@ $ 00005678 = is.ok?
$ 6142 var1 w!   var1 @ $ 61425678 = is.ok?

\ dictionary conversion
' cr-data >body cr-data = is.ok?
cr-data body> ' cr-data = is.ok?
create CR-FOO  0 ,
' cr-data >code @ ' cr-foo >code @ = is.ok?

: TP1 ( -- con1 ) con1 ;
: TP2 ( -- con2 ) con2 ;
tp1 con1 = is.ok?
' tp1 execute con1 = is.ok?
' tp1 >code code> ' tp1 = is.ok?
' tp2 >name name> ' tp2 = is.ok?

\ test conditionals and loops
: TP.IF IF con1 ELSE con2 THEN ;
0 tp.if con2 = is.ok?
-1 tp.if con1 = is.ok?

: TP.UNTIL 2 BEGIN dup 100 + swap 1- dup 0< UNTIL drop ;
tp.until  rot 102 =  rot 101 =  rot 100 =  and and is.ok?

: TP.WHILE 4 BEGIN 1- dup 0> WHILE dup 200 + swap REPEAT drop ;
tp.while  rot 203 =  rot 202 =  rot 201 =  and and is.ok?

: TP.DO.LOOP 3 0 DO i 300 + LOOP ;
tp.do.loop rot 300 =  rot 301 =  rot 302 =  and and is.ok?

\ test nested conditionals
: TP.NESTED  IF 8 6 DO i LOOP ELSE 99 THEN ;
1 tp.nested 7 = swap 6 = and is.ok?
0 tp.nested 99 = is.ok?

\ source-id should be FILE pointer
source-id 0= not   source-id -1 = not and is.ok?

\ test accept
\ .( Enter up to 40 characters.)"
\ create spad 50 allot
\ spad 40 accept
\ .( You entered:) spad swap type cr

\ 
\ ==========================================================
echo off

.( Test result:   )
test-passed @ 4 .r .(  passed, )
test-failed @ 4 .r .(  failed.) cr

\ get rid of underflow protection words
drop drop drop drop

echo !
