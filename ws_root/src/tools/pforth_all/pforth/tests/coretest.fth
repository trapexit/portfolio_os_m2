\ @(#) coretest.fth 95/04/11 1.2
\ (c) 1993 Johns Hopkins University / Applied Physics Laboratory
\ May be distributed freely as long as this copyright notice remains.
\ Version 1.1
\ This program tests the CORE words of an ANS Forth system.
\ The program assumes a two's complement implementation where
\ the range of signed numbers is -2^(n-1) ... 2^(n-1)-1 and
\ the range of unsigned numbers is 0 ... 2^(n)-1.
\ I haven't figured out how to test key, quit, abort, or abort"...
\ I also haven't thought of a way to test environment?...
\                                  -- John Hayes, Johns Hopkins APL

include? testing tester.fth

testing core words
HEX

\ ------------------------------------------------------------------------
testing basic assumptions

{ -> }                                  \ start with clean slate
( test if any bits are set; answer in base 1 )
{ : bitsset? IF 0 0 ELSE 0 THEN ; -> }
{  0 bitsset? -> 0 }            ( zero is all bits clear )
{  1 bitsset? -> 0 0 }          ( other number have at least one bit )
{ -1 bitsset? -> 0 0 }

\ ------------------------------------------------------------------------
testing Booleans: INVERT AND OR XOR

{ 0 0 AND -> 0 }
{ 0 1 AND -> 0 }
{ 1 0 AND -> 0 }
{ 1 1 AND -> 1 }

{ 0 INVERT 1 AND -> 1 }
{ 1 INVERT 1 AND -> 0 }

0        CONSTANT 0s
0 INVERT CONSTANT 1s

{ 0s INVERT -> 1s }
{ 1s INVERT -> 0s }

{ 0s 0s AND -> 0s }
{ 0s 1s AND -> 0s }
{ 1s 0s AND -> 0s }
{ 1s 1s AND -> 1s }

{ 0s 0s OR -> 0s }
{ 0s 1s OR -> 1s }
{ 1s 0s OR -> 1s }
{ 1s 1s OR -> 1s }

{ 0s 0s XOR -> 0s }
{ 0s 1s XOR -> 1s }
{ 1s 0s XOR -> 1s }
{ 1s 1s XOR -> 0s }

\ ------------------------------------------------------------------------
testing 2* 2/ LSHIFT RSHIFT

( we trust 1s, invert, and bitsset?; we will confirm rshift later )
1s 1 RSHIFT INVERT CONSTANT msb
{ msb bitsset? -> 0 0 }

{ 0s 2* -> 0s }
{ 1 2* -> 2 }
{ 4000 2* -> 8000 }
{ 1s 2* 1 XOR -> 1s }
{ msb 2* -> 0s }

{ 0s 2/ -> 0s }
{ 1 2/ -> 0 }
{ 4000 2/ -> 2000 }
{ 1s 2/ -> 1s }                         \ msb propogated
{ 1s 1 XOR 2/ -> 1s }
{ msb 2/ msb AND -> msb }

{ 1 0 LSHIFT -> 1 }
{ 1 1 LSHIFT -> 2 }
{ 1 2 LSHIFT -> 4 }
{ 1 f LSHIFT -> 8000 }                  \ biggest guaranteed shift
{ 1s 1 LSHIFT 1 XOR -> 1s }
{ msb 1 LSHIFT -> 0 }

{ 1 0 RSHIFT -> 1 }
{ 1 1 RSHIFT -> 0 }
{ 2 1 RSHIFT -> 1 }
{ 4 2 RSHIFT -> 1 }
{ 8000 f RSHIFT -> 1 }                  \ biggest
{ msb 1 RSHIFT msb AND -> 0 }           \ rshift zero fills msbs
{ msb 1 RSHIFT 2* -> msb }

\ ------------------------------------------------------------------------
testing comparisons: 0= = 0< < > U< MIN MAX
0 INVERT                        CONSTANT max-uint
0 INVERT 1 RSHIFT               CONSTANT max-int
0 INVERT 1 RSHIFT INVERT        CONSTANT min-int
0 INVERT 1 RSHIFT               CONSTANT mid-uint
0 INVERT 1 RSHIFT INVERT        CONSTANT mid-uint+1

0s CONSTANT <false>
1s CONSTANT <true>

{ 0 0= -> <true> }
{ 1 0= -> <false> }
{ 2 0= -> <false> }
{ -1 0= -> <false> }
{ max-uint 0= -> <false> }
{ min-int 0= -> <false> }
{ max-int 0= -> <false> }

{ 0 0 = -> <true> }
{ 1 1 = -> <true> }
{ -1 -1 = -> <true> }
{ 1 0 = -> <false> }
{ -1 0 = -> <false> }
{ 0 1 = -> <false> }
{ 0 -1 = -> <false> }

{ 0 0< -> <false> }
{ -1 0< -> <true> }
{ min-int 0< -> <true> }
{ 1 0< -> <false> }
{ max-int 0< -> <false> }

{ 0 1 < -> <true> }
{ 1 2 < -> <true> }
{ -1 0 < -> <true> }
{ -1 1 < -> <true> }
{ min-int 0 < -> <true> }
{ min-int max-int < -> <true> }
{ 0 max-int < -> <true> }
{ 0 0 < -> <false> }
{ 1 1 < -> <false> }
{ 1 0 < -> <false> }
{ 2 1 < -> <false> }
{ 0 -1 < -> <false> }
{ 1 -1 < -> <false> }
{ 0 min-int < -> <false> }
{ max-int min-int < -> <false> }
{ max-int 0 < -> <false> }

{ 0 1 > -> <false> }
{ 1 2 > -> <false> }
{ -1 0 > -> <false> }
{ -1 1 > -> <false> }
{ min-int 0 > -> <false> }
{ min-int max-int > -> <false> }
{ 0 max-int > -> <false> }
{ 0 0 > -> <false> }
{ 1 1 > -> <false> }
{ 1 0 > -> <true> }
{ 2 1 > -> <true> }
{ 0 -1 > -> <true> }
{ 1 -1 > -> <true> }
{ 0 min-int > -> <true> }
{ max-int min-int > -> <true> }
{ max-int 0 > -> <true> }

{ 0 1 U< -> <true> }
{ 1 2 U< -> <true> }
{ 0 mid-uint U< -> <true> }
{ 0 max-uint U< -> <true> }
{ mid-uint max-uint U< -> <true> }
{ 0 0 U< -> <false> }
{ 1 1 U< -> <false> }
{ 1 0 U< -> <false> }
{ 2 1 U< -> <false> }
{ mid-uint 0 U< -> <false> }
{ max-uint 0 U< -> <false> }
{ max-uint mid-uint U< -> <false> }

{ 0 1 MIN -> 0 }
{ 1 2 MIN -> 1 }
{ -1 0 MIN -> -1 }
{ -1 1 MIN -> -1 }
{ min-int 0 MIN -> min-int }
{ min-int max-int MIN -> min-int }
{ 0 max-int MIN -> 0 }
{ 0 0 MIN -> 0 }
{ 1 1 MIN -> 1 }
{ 1 0 MIN -> 0 }
{ 2 1 MIN -> 1 }
{ 0 -1 MIN -> -1 }
{ 1 -1 MIN -> -1 }
{ 0 min-int MIN -> min-int }
{ max-int min-int MIN -> min-int }
{ max-int 0 MIN -> 0 }

{ 0 1 MAX -> 1 }
{ 1 2 MAX -> 2 }
{ -1 0 MAX -> 0 }
{ -1 1 MAX -> 1 }
{ min-int 0 MAX -> 0 }
{ min-int max-int MAX -> max-int }
{ 0 max-int MAX -> max-int }
{ 0 0 MAX -> 0 }
{ 1 1 MAX -> 1 }
{ 1 0 MAX -> 1 }
{ 2 1 MAX -> 2 }
{ 0 -1 MAX -> 0 }
{ 1 -1 MAX -> 1 }
{ 0 min-int MAX -> 0 }
{ max-int min-int MAX -> max-int }
{ max-int 0 MAX -> max-int }

\ ------------------------------------------------------------------------
testing stack ops: 2DROP 2DUP 2OVER 2SWAP ?DUP DEPTH DROP DUP OVER ROT SWAP

{ 1 2 2DROP -> }
{ 1 2 2DUP -> 1 2 1 2 }
{ 1 2 3 4 2OVER -> 1 2 3 4 1 2 }
{ 1 2 3 4 2SWAP -> 3 4 1 2 }
{ 0 ?DUP -> 0 }
{ 1 ?DUP -> 1 1 }
{ -1 ?DUP -> -1 -1 }
{ DEPTH -> 0 }
{ 0 DEPTH -> 0 1 }
{ 0 1 DEPTH -> 0 1 2 }
{ 0 DROP -> }
{ 1 2 DROP -> 1 }
{ 1 DUP -> 1 1 }
{ 1 2 OVER -> 1 2 1 }
{ 1 2 3 ROT -> 2 3 1 }
{ 1 2 SWAP -> 2 1 }

\ ------------------------------------------------------------------------
testing >R R> R@

{ : gr1 >R R> ; -> }
{ : gr2 >R R@ R> DROP ; -> }
{ 123 gr1 -> 123 }
{ 123 gr2 -> 123 }
{ 1s gr1 -> 1s }   ( return stack holds cells )

\ ------------------------------------------------------------------------
testing add/subtract: + - 1+ 1- ABS NEGATE

{ 0 5 + -> 5 }
{ 5 0 + -> 5 }
{ 0 -5 + -> -5 }
{ -5 0 + -> -5 }
{ 1 2 + -> 3 }
{ 1 -2 + -> -1 }
{ -1 2 + -> 1 }
{ -1 -2 + -> -3 }
{ -1 1 + -> 0 }
{ mid-uint 1 + -> mid-uint+1 }

{ 0 5 - -> -5 }
{ 5 0 - -> 5 }
{ 0 -5 - -> 5 }
{ -5 0 - -> -5 }
{ 1 2 - -> -1 }
{ 1 -2 - -> 3 }
{ -1 2 - -> -3 }
{ -1 -2 - -> 1 }
{ 0 1 - -> -1 }
{ mid-uint+1 1 - -> mid-uint }

{ 0 1+ -> 1 }
{ -1 1+ -> 0 }
{ 1 1+ -> 2 }
{ mid-uint 1+ -> mid-uint+1 }

{ 2 1- -> 1 }
{ 1 1- -> 0 }
{ 0 1- -> -1 }
{ mid-uint+1 1- -> mid-uint }

{ 0 NEGATE -> 0 }
{ 1 NEGATE -> -1 }
{ -1 NEGATE -> 1 }
{ 2 NEGATE -> -2 }
{ -2 NEGATE -> 2 }

{ 0 ABS -> 0 }
{ 1 ABS -> 1 }
{ -1 ABS -> 1 }
{ min-int ABS -> mid-uint+1 }

\ ------------------------------------------------------------------------
testing multiply: S>D * M* UM*

{ 0 S>D -> 0 0 }
{ 1 S>D -> 1 0 }
{ 2 S>D -> 2 0 }
{ -1 S>D -> -1 -1 }
{ -2 S>D -> -2 -1 }
{ min-int S>D -> min-int -1 }
{ max-int S>D -> max-int 0 }

{ 0 0 M* -> 0 S>D }
{ 0 1 M* -> 0 S>D }
{ 1 0 M* -> 0 S>D }
{ 1 2 M* -> 2 S>D }
{ 2 1 M* -> 2 S>D }
{ 3 3 M* -> 9 S>D }
{ -3 3 M* -> -9 S>D }
{ 3 -3 M* -> -9 S>D }
{ -3 -3 M* -> 9 S>D }
{ 0 min-int M* -> 0 S>D }
{ 1 min-int M* -> min-int S>D }
{ 2 min-int M* -> 0 1s }
{ 0 max-int M* -> 0 S>D }
{ 1 max-int M* -> max-int S>D }
{ 2 max-int M* -> max-int 1 LSHIFT 0 }
{ min-int min-int M* -> 0 msb 1 RSHIFT }
{ max-int min-int M* -> msb msb 2/ }
{ max-int max-int M* -> 1 msb 2/ INVERT }

{ 0 0 * -> 0 }                          \ test identities
{ 0 1 * -> 0 }
{ 1 0 * -> 0 }
{ 1 2 * -> 2 }
{ 2 1 * -> 2 }
{ 3 3 * -> 9 }
{ -3 3 * -> -9 }
{ 3 -3 * -> -9 }
{ -3 -3 * -> 9 }

{ mid-uint+1 1 RSHIFT 2 * -> mid-uint+1 }
{ mid-uint+1 2 RSHIFT 4 * -> mid-uint+1 }
{ mid-uint+1 1 RSHIFT mid-uint+1 OR 2 * -> mid-uint+1 }

{ 0 0 UM* -> 0 0 }
{ 0 1 UM* -> 0 0 }
{ 1 0 UM* -> 0 0 }
{ 1 2 UM* -> 2 0 }
{ 2 1 UM* -> 2 0 }
{ 3 3 UM* -> 9 0 }

{ mid-uint+1 1 RSHIFT 2 UM* -> mid-uint+1 0 }
{ mid-uint+1 2 UM* -> 0 1 }
{ mid-uint+1 4 UM* -> 0 2 }
{ 1s 2 UM* -> 1s 1 LSHIFT 1 }
{ max-uint max-uint UM* -> 1 1 INVERT }

\ ------------------------------------------------------------------------
testing divide: FM/MOD SM/REM UM/MOD */ */MOD / /MOD MOD

{ 0 S>D 1 FM/MOD -> 0 0 }
{ 1 S>D 1 FM/MOD -> 0 1 }
{ 2 S>D 1 FM/MOD -> 0 2 }
{ -1 S>D 1 FM/MOD -> 0 -1 }
{ -2 S>D 1 FM/MOD -> 0 -2 }
{ 0 S>D -1 FM/MOD -> 0 0 }
{ 1 S>D -1 FM/MOD -> 0 -1 }
{ 2 S>D -1 FM/MOD -> 0 -2 }
{ -1 S>D -1 FM/MOD -> 0 1 }
{ -2 S>D -1 FM/MOD -> 0 2 }
{ 2 S>D 2 FM/MOD -> 0 1 }
{ -1 S>D -1 FM/MOD -> 0 1 }
{ -2 S>D -2 FM/MOD -> 0 1 }
{  7 S>D  3 FM/MOD -> 1 2 }
{  7 S>D -3 FM/MOD -> -2 -3 }
{ -7 S>D  3 FM/MOD -> 2 -3 }
{ -7 S>D -3 FM/MOD -> -1 2 }
{ max-int S>D 1 FM/MOD -> 0 max-int }
{ min-int S>D 1 FM/MOD -> 0 min-int }
{ max-int S>D max-int FM/MOD -> 0 1 }
{ min-int S>D min-int FM/MOD -> 0 1 }
{ 1s 1 4 FM/MOD -> 3 max-int }
{ 1 min-int M* 1 FM/MOD -> 0 min-int }
{ 1 min-int M* min-int FM/MOD -> 0 1 }
{ 2 min-int M* 2 FM/MOD -> 0 min-int }
{ 2 min-int M* min-int FM/MOD -> 0 2 }
{ 1 max-int M* 1 FM/MOD -> 0 max-int }
{ 1 max-int M* max-int FM/MOD -> 0 1 }
{ 2 max-int M* 2 FM/MOD -> 0 max-int }
{ 2 max-int M* max-int FM/MOD -> 0 2 }
{ min-int min-int M* min-int FM/MOD -> 0 min-int }
{ min-int max-int M* min-int FM/MOD -> 0 max-int }
{ min-int max-int M* max-int FM/MOD -> 0 min-int }
{ max-int max-int M* max-int FM/MOD -> 0 max-int }

{ 0 S>D 1 SM/REM -> 0 0 }
{ 1 S>D 1 SM/REM -> 0 1 }
{ 2 S>D 1 SM/REM -> 0 2 }
{ -1 S>D 1 SM/REM -> 0 -1 }
{ -2 S>D 1 SM/REM -> 0 -2 }
{ 0 S>D -1 SM/REM -> 0 0 }
{ 1 S>D -1 SM/REM -> 0 -1 }
{ 2 S>D -1 SM/REM -> 0 -2 }
{ -1 S>D -1 SM/REM -> 0 1 }
{ -2 S>D -1 SM/REM -> 0 2 }
{ 2 S>D 2 SM/REM -> 0 1 }
{ -1 S>D -1 SM/REM -> 0 1 }
{ -2 S>D -2 SM/REM -> 0 1 }
{  7 S>D  3 SM/REM -> 1 2 }
{  7 S>D -3 SM/REM -> 1 -2 }
{ -7 S>D  3 SM/REM -> -1 -2 }
{ -7 S>D -3 SM/REM -> -1 2 }
{ max-int S>D 1 SM/REM -> 0 max-int }
{ min-int S>D 1 SM/REM -> 0 min-int }
{ max-int S>D max-int SM/REM -> 0 1 }
{ min-int S>D min-int SM/REM -> 0 1 }
{ 1s 1 4 SM/REM -> 3 max-int }
{ 2 min-int M* 2 SM/REM -> 0 min-int }
{ 2 min-int M* min-int SM/REM -> 0 2 }
{ 2 max-int M* 2 SM/REM -> 0 max-int }
{ 2 max-int M* max-int SM/REM -> 0 2 }
{ min-int min-int M* min-int SM/REM -> 0 min-int }
{ min-int max-int M* min-int SM/REM -> 0 max-int }
{ min-int max-int M* max-int SM/REM -> 0 min-int }
{ max-int max-int M* max-int SM/REM -> 0 max-int }

{ 0 0 1 UM/MOD -> 0 0 }
{ 1 0 1 UM/MOD -> 0 1 }
{ 1 0 2 UM/MOD -> 1 0 }
{ 3 0 2 UM/MOD -> 1 1 }
{ max-uint 2 UM* 2 UM/MOD -> 0 max-uint }
{ max-uint 2 UM* max-uint UM/MOD -> 0 2 }
{ max-uint max-uint UM* max-uint UM/MOD -> 0 max-uint }

: iffloored
	[ -3 2 / -2 = INVERT ] LITERAL IF POSTPONE \ THEN ;
: ifsym
	[ -3 2 / -1 = INVERT ] LITERAL IF POSTPONE \ THEN ;

\ The system might do either floored or symmetric division.
\ Since we have already tested m*, fm/mod, and sm/rem we can use them in test.
iffloored : t/mod  >R S>D R> FM/MOD ;
iffloored : t/     t/mod SWAP DROP ;
iffloored : tmod   t/mod DROP ;
iffloored : t*/mod >R M* R> FM/MOD ;
iffloored : t*/    t*/mod SWAP DROP ;
ifsym     : t/mod  >R S>D R> SM/REM ;
ifsym     : t/     t/mod SWAP DROP ;
ifsym     : tmod   t/mod DROP ;
ifsym     : t*/mod >R M* R> SM/REM ;
ifsym     : t*/    t*/mod SWAP DROP ;

{ 0 1 /MOD -> 0 1 t/mod }
{ 1 1 /MOD -> 1 1 t/mod }
{ 2 1 /MOD -> 2 1 t/mod }
{ -1 1 /MOD -> -1 1 t/mod }
{ -2 1 /MOD -> -2 1 t/mod }
{ 0 -1 /MOD -> 0 -1 t/mod }
{ 1 -1 /MOD -> 1 -1 t/mod }
{ 2 -1 /MOD -> 2 -1 t/mod }
{ -1 -1 /MOD -> -1 -1 t/mod }
{ -2 -1 /MOD -> -2 -1 t/mod }
{ 2 2 /MOD -> 2 2 t/mod }
{ -1 -1 /MOD -> -1 -1 t/mod }
{ -2 -2 /MOD -> -2 -2 t/mod }
{ 7 3 /MOD -> 7 3 t/mod }
{ 7 -3 /MOD -> 7 -3 t/mod }
{ -7 3 /MOD -> -7 3 t/mod }
{ -7 -3 /MOD -> -7 -3 t/mod }
{ max-int 1 /MOD -> max-int 1 t/mod }
{ min-int 1 /MOD -> min-int 1 t/mod }
{ max-int max-int /MOD -> max-int max-int t/mod }
{ min-int min-int /MOD -> min-int min-int t/mod }

{ 0 1 / -> 0 1 t/ }
{ 1 1 / -> 1 1 t/ }
{ 2 1 / -> 2 1 t/ }
{ -1 1 / -> -1 1 t/ }
{ -2 1 / -> -2 1 t/ }
{ 0 -1 / -> 0 -1 t/ }
{ 1 -1 / -> 1 -1 t/ }
{ 2 -1 / -> 2 -1 t/ }
{ -1 -1 / -> -1 -1 t/ }
{ -2 -1 / -> -2 -1 t/ }
{ 2 2 / -> 2 2 t/ }
{ -1 -1 / -> -1 -1 t/ }
{ -2 -2 / -> -2 -2 t/ }
{ 7 3 / -> 7 3 t/ }
{ 7 -3 / -> 7 -3 t/ }
{ -7 3 / -> -7 3 t/ }
{ -7 -3 / -> -7 -3 t/ }
{ max-int 1 / -> max-int 1 t/ }
{ min-int 1 / -> min-int 1 t/ }
{ max-int max-int / -> max-int max-int t/ }
{ min-int min-int / -> min-int min-int t/ }

{ 0 1 MOD -> 0 1 tmod }
{ 1 1 MOD -> 1 1 tmod }
{ 2 1 MOD -> 2 1 tmod }
{ -1 1 MOD -> -1 1 tmod }
{ -2 1 MOD -> -2 1 tmod }
{ 0 -1 MOD -> 0 -1 tmod }
{ 1 -1 MOD -> 1 -1 tmod }
{ 2 -1 MOD -> 2 -1 tmod }
{ -1 -1 MOD -> -1 -1 tmod }
{ -2 -1 MOD -> -2 -1 tmod }
{ 2 2 MOD -> 2 2 tmod }
{ -1 -1 MOD -> -1 -1 tmod }
{ -2 -2 MOD -> -2 -2 tmod }
{ 7 3 MOD -> 7 3 tmod }
{ 7 -3 MOD -> 7 -3 tmod }
{ -7 3 MOD -> -7 3 tmod }
{ -7 -3 MOD -> -7 -3 tmod }
{ max-int 1 MOD -> max-int 1 tmod }
{ min-int 1 MOD -> min-int 1 tmod }
{ max-int max-int MOD -> max-int max-int tmod }
{ min-int min-int MOD -> min-int min-int tmod }

{ 0 2 1 */ -> 0 2 1 t*/ }
{ 1 2 1 */ -> 1 2 1 t*/ }
{ 2 2 1 */ -> 2 2 1 t*/ }
{ -1 2 1 */ -> -1 2 1 t*/ }
{ -2 2 1 */ -> -2 2 1 t*/ }
{ 0 2 -1 */ -> 0 2 -1 t*/ }
{ 1 2 -1 */ -> 1 2 -1 t*/ }
{ 2 2 -1 */ -> 2 2 -1 t*/ }
{ -1 2 -1 */ -> -1 2 -1 t*/ }
{ -2 2 -1 */ -> -2 2 -1 t*/ }
{ 2 2 2 */ -> 2 2 2 t*/ }
{ -1 2 -1 */ -> -1 2 -1 t*/ }
{ -2 2 -2 */ -> -2 2 -2 t*/ }
{ 7 2 3 */ -> 7 2 3 t*/ }
{ 7 2 -3 */ -> 7 2 -3 t*/ }
{ -7 2 3 */ -> -7 2 3 t*/ }
{ -7 2 -3 */ -> -7 2 -3 t*/ }
{ max-int 2 max-int */ -> max-int 2 max-int t*/ }
{ min-int 2 min-int */ -> min-int 2 min-int t*/ }

{ 0 2 1 */MOD -> 0 2 1 t*/mod }
{ 1 2 1 */MOD -> 1 2 1 t*/mod }
{ 2 2 1 */MOD -> 2 2 1 t*/mod }
{ -1 2 1 */MOD -> -1 2 1 t*/mod }
{ -2 2 1 */MOD -> -2 2 1 t*/mod }
{ 0 2 -1 */MOD -> 0 2 -1 t*/mod }
{ 1 2 -1 */MOD -> 1 2 -1 t*/mod }
{ 2 2 -1 */MOD -> 2 2 -1 t*/mod }
{ -1 2 -1 */MOD -> -1 2 -1 t*/mod }
{ -2 2 -1 */MOD -> -2 2 -1 t*/mod }
{ 2 2 2 */MOD -> 2 2 2 t*/mod }
{ -1 2 -1 */MOD -> -1 2 -1 t*/mod }
{ -2 2 -2 */MOD -> -2 2 -2 t*/mod }
{ 7 2 3 */MOD -> 7 2 3 t*/mod }
{ 7 2 -3 */MOD -> 7 2 -3 t*/mod }
{ -7 2 3 */MOD -> -7 2 3 t*/mod }
{ -7 2 -3 */MOD -> -7 2 -3 t*/mod }
{ max-int 2 max-int */MOD -> max-int 2 max-int t*/mod }
{ min-int 2 min-int */MOD -> min-int 2 min-int t*/mod }

\ ------------------------------------------------------------------------
testing HERE , @ ! CELL+ CELLS C, C@ C! CHARS 2@ 2! ALIGN ALIGNED +! ALLOT

HERE 1 ALLOT
HERE
CONSTANT 2nda
CONSTANT 1sta
{ 1sta 2nda U< -> <true> }              \ here must grow with allot
{ 1sta 1+ -> 2nda }                     \ ... by one address unit
( Missing test: negative allot )

HERE 1 ,
HERE 2 ,
CONSTANT 2nd
CONSTANT 1st
{ 1st 2nd U< -> <true> }                \ here must grow with allot
{ 1st CELL+ -> 2nd }                    \ ... by one cell
{ 1st 1 CELLS + -> 2nd }
{ 1st @ 2nd @ -> 1 2 }
{ 5 1st ! -> }
{ 1st @ 2nd @ -> 5 2 }
{ 6 2nd ! -> }
{ 1st @ 2nd @ -> 5 6 }
{ 1st 2@ -> 6 5 }
{ 2 1 1st 2! -> }
{ 1st 2@ -> 2 1 }
{ 1s 1st !  1st @ -> 1s }               \ can store cell-wide value

HERE 1 C,
HERE 2 C,
CONSTANT 2ndc
CONSTANT 1stc
{ 1stc 2ndc U< -> <true> }              \ here must grow with allot
{ 1stc CHAR+ -> 2ndc }                  \ ... by one char
{ 1stc 1 CHARS + -> 2ndc }
{ 1stc C@ 2ndc C@ -> 1 2 }
{ 3 1stc C! -> }
{ 1stc C@ 2ndc C@ -> 3 2 }
{ 4 2ndc C! -> }
{ 1stc C@ 2ndc C@ -> 3 4 }

ALIGN 1 ALLOT HERE ALIGN HERE 3 CELLS ALLOT
CONSTANT a-addr  CONSTANT ua-addr
{ ua-addr ALIGNED -> a-addr }
{    1 a-addr C!  a-addr C@ ->    1 }
{ 1234 a-addr  !  a-addr  @ -> 1234 }
{ 123 456 a-addr 2!  a-addr 2@ -> 123 456 }
{ 2 a-addr CHAR+ C!  a-addr CHAR+ C@ -> 2 }
{ 3 a-addr CELL+ C!  a-addr CELL+ C@ -> 3 }
{ 1234 a-addr CELL+ !  a-addr CELL+ @ -> 1234 }
{ 123 456 a-addr CELL+ 2!  a-addr CELL+ 2@ -> 123 456 }

: bits ( x -- u )
	0 SWAP BEGIN DUP WHILE DUP msb AND IF >R 1+ R> THEN 2* REPEAT DROP ;
( characters >= 1 au, <= size of cell, >= 8 bits )
{ 1 CHARS 1 < -> <false> }
{ 1 CHARS 1 CELLS > -> <false> }
( TBD: how to find number of bits? )

( cells >= 1 au, integral multiple of char size, >= 16 bits )
{ 1 CELLS 1 < -> <false> }
{ 1 CELLS 1 CHARS MOD -> 0 }
{ 1s bits 10 < -> <false> }

{ 0 1st ! -> }
{ 1 1st +! -> }
{ 1st @ -> 1 }
{ -1 1st +! 1st @ -> 0 }

\ ------------------------------------------------------------------------
testing CHAR [CHAR] [ ] BL S"

{ BL -> 20 }
{ CHAR X -> 58 }
{ CHAR Hello -> 48 }
{ : gc1 [CHAR] X ; -> }
{ : gc2 [CHAR] Hello ; -> }
{ gc1 -> 58 }
{ gc2 -> 48 }
{ : gc3 [ gc1 ] LITERAL ; -> }
{ gc3 -> 58 }
{ : gc4 S" XY" ; -> }
{ gc4 SWAP DROP -> 2 }
{ gc4 DROP DUP C@ SWAP CHAR+ C@ -> 58 59 }

\ ------------------------------------------------------------------------
testing ' ['] FIND EXECUTE IMMEDIATE COUNT LITERAL POSTPONE STATE

{ : gt1 123 ; -> }
{ ' gt1 EXECUTE -> 123 }
{ : gt2 ['] gt1 ; IMMEDIATE -> }
{ gt2 EXECUTE -> 123 }
HERE 3 C, CHAR g C, CHAR t C, CHAR 1 C, CONSTANT gt1string
HERE 3 C, CHAR g C, CHAR t C, CHAR 2 C, CONSTANT gt2string
{ gt1string FIND -> ' gt1 -1 }
{ gt2string FIND -> ' gt2 1 }
( How to search for non-existent word? )
{ : gt3 gt2 LITERAL ; -> }
{ gt3 -> ' gt1 }
{ gt1string COUNT -> gt1string CHAR+ 3 }

{ : gt4 POSTPONE gt1 ; IMMEDIATE -> }
{ : gt5 gt4 ; -> }
{ gt5 -> 123 }
{ : gt6 345 ; IMMEDIATE -> }
{ : gt7 POSTPONE gt6 ; -> }
{ gt7 -> 345 }

{ : gt8 STATE @ ; IMMEDIATE -> }
{ gt8 -> 0 }
{ : gt9 gt8 LITERAL ; -> }
{ gt9 0= -> <false> }

\ ------------------------------------------------------------------------
testing IF ELSE THEN BEGIN WHILE REPEAT UNTIL RECURSE

{ : gi1 IF 123 THEN ; -> }
{ : gi2 IF 123 ELSE 234 THEN ; -> }
{ 0 gi1 -> }
{ 1 gi1 -> 123 }
{ -1 gi1 -> 123 }
{ 0 gi2 -> 234 }
{ 1 gi2 -> 123 }
{ -1 gi1 -> 123 }

{ : gi3 BEGIN DUP 5 < WHILE DUP 1+ REPEAT ; -> }
{ 0 gi3 -> 0 1 2 3 4 5 }
{ 4 gi3 -> 4 5 }
{ 5 gi3 -> 5 }
{ 6 gi3 -> 6 }

{ : gi4 BEGIN DUP 1+ DUP 5 > UNTIL ; -> }
{ 3 gi4 -> 3 4 5 6 }
{ 5 gi4 -> 5 6 }
{ 6 gi4 -> 6 7 }

\ { : gi5 BEGIN DUP 2 > WHILE DUP 5 < WHILE DUP 1+ REPEAT 123 ELSE 345 THEN ; -> }
\ { 1 gi5 -> 1 345 }
\ { 2 gi5 -> 2 345 }
\ { 3 gi5 -> 3 4 5 123 }
\ { 4 gi5 -> 4 5 123 }
\ { 5 gi5 -> 5 123 }

{ : gi6 ( n -- 0,1,..n ) DUP IF DUP >R 1- RECURSE R> THEN ; -> }
{ 0 gi6 -> 0 }
{ 1 gi6 -> 0 1 }
{ 2 gi6 -> 0 1 2 }
{ 3 gi6 -> 0 1 2 3 }
{ 4 gi6 -> 0 1 2 3 4 }

\ ------------------------------------------------------------------------
testing DO LOOP +LOOP I J UNLOOP LEAVE EXIT

{ : gd1 DO I LOOP ; -> }
{ 4 1 gd1 -> 1 2 3 }
{ 2 -1 gd1 -> -1 0 1 }
{ mid-uint+1 mid-uint gd1 -> mid-uint }

{ : gd2 DO I -1 +LOOP ; -> }
{ 1 4 gd2 -> 4 3 2 1 }
{ -1 2 gd2 -> 2 1 0 -1 }
{ mid-uint mid-uint+1 gd2 -> mid-uint+1 mid-uint }

{ : gd3 DO 1 0 DO J LOOP LOOP ; -> }
{ 4 1 gd3 -> 1 2 3 }
{ 2 -1 gd3 -> -1 0 1 }
{ mid-uint+1 mid-uint gd3 -> mid-uint }

{ : gd4 DO 1 0 DO J LOOP -1 +LOOP ; -> }
{ 1 4 gd4 -> 4 3 2 1 }
{ -1 2 gd4 -> 2 1 0 -1 }
{ mid-uint mid-uint+1 gd4 -> mid-uint+1 mid-uint }

{ : gd5 123 SWAP 0 DO I 4 > IF DROP 234 LEAVE THEN LOOP ; -> }
{ 1 gd5 -> 123 }
{ 5 gd5 -> 123 }
{ 6 gd5 -> 234 }

{ : gd6  ( pat: {0 0},{0 0}{1 0}{1 1},{0 0}{1 0}{1 1}{2 0}{2 1}{2 2} )
	0 SWAP 0 DO
	I 1+ 0 DO I J + 3 = IF I UNLOOP I UNLOOP EXIT THEN 1+ LOOP
	LOOP ; -> }
{ 1 gd6 -> 1 }
{ 2 gd6 -> 3 }
{ 3 gd6 -> 4 1 2 }

\ ------------------------------------------------------------------------
testing defining words: : ; CONSTANT VARIABLE CREATE DOES> >BODY

{ 123 CONSTANT x123 -> }
{ x123 -> 123 }
{ : equ CONSTANT ; -> }
{ x123 equ y123 -> }
{ y123 -> 123 }

{ VARIABLE v1 -> }
{ 123 v1 ! -> }
{ v1 @ -> 123 }

{ : nop : POSTPONE ; ; -> }
{ nop nop1 nop nop2 -> }
{ nop1 -> }
{ nop2 -> }

{ : does1 DOES> @ 1 + ; -> }
{ : does2 DOES> @ 2 + ; -> }
{ CREATE cr1 -> }
{ cr1 -> HERE }
{ ' cr1 >BODY -> HERE }
{ 1 , -> }
{ cr1 @ -> 1 }
{ does1 -> }
{ cr1 -> 2 }
{ does2 -> }
{ cr1 -> 3 }

{ : weird: CREATE DOES> 1 + DOES> 2 + ; -> }
{ weird: w1 -> }
{ ' w1 >BODY -> HERE }
{ w1 -> HERE 1 + }
{ w1 -> HERE 2 + }

\ ------------------------------------------------------------------------
testing EVALUATE

: ge1 S" 123" ; IMMEDIATE
: ge2 S" 123 1+" ; IMMEDIATE
: ge3 S" : ge4 345 ;" ;
: ge5 EVALUATE ; IMMEDIATE

{ ge1 EVALUATE -> 123 }                 ( test evaluate in interp. state )
{ ge2 EVALUATE -> 124 }
{ ge3 EVALUATE -> }
{ ge4 -> 345 }

{ : ge6 ge1 ge5 ; -> }                  ( test evaluate in compile state )
{ ge6 -> 123 }
{ : ge7 ge2 ge5 ; -> }
{ ge7 -> 124 }

\ ------------------------------------------------------------------------
testing source >in WORD

: gs1 s" source" 2dup evaluate >r swap >r = r> r> = ;
{ gs1 -> <true> <true> } 

variable scans
: rescan?  -1 scans +! scans @ if 0 >in ! then ;

{ 2 scans !
345 rescan?
-> 345 345 }

: gs2  5 scans ! s" 123 rescan?" evaluate ;
{ gs2 -> 123 123 123 123 123 }

: gs3 WORD COUNT SWAP C@ ;
{ BL gs3 Hello -> 5 CHAR H }
{ CHAR " gs3 Goodbye" -> 7 CHAR G }
{ BL WORD
C@ -> 0 }                               \ blank line returns zero-length string

: gs4 source >in ! drop ;
{ gs4 123 456
-> }

\ ------------------------------------------------------------------------
testing <# # #S #> HOLD SIGN BASE >NUMBER HEX DECIMAL

: s=  \ ( addr1 c1 addr2 c2 -- t/f ) compare two strings.
	>R SWAP R@ = IF                   \ make sure strings have same length
	R> ?DUP IF                        \ if non-empty strings
		0 DO
		OVER C@ OVER C@ - IF 2DROP <false> UNLOOP EXIT THEN
		SWAP CHAR+ SWAP CHAR+
		LOOP
	THEN
	2DROP <true>                      \ if we get here, strings match
	ELSE
	R> DROP 2DROP <false>             \ lengths mismatch
	THEN ;

: gp1  <# 41 HOLD 42 HOLD 0 0 #> S" BA" s= ;
{ gp1 -> <true> }

: gp2  <# -1 SIGN 0 SIGN -1 SIGN 0 0 #> S" --" s= ;
{ gp2 -> <true> }

: gp3  <# 1 0 # # #> S" 01" s= ;
{ gp3 -> <true> }

: gp4  <# 1 0 #S #> S" 1" s= ;
{ gp4 -> <true> }

24 CONSTANT max-base                    \ base 2 .. 36
: count-bits
	0 0 INVERT BEGIN DUP WHILE >R 1+ R> 2* REPEAT DROP ;
count-bits 2* CONSTANT #bits-ud         \ number of bits in ud

: gp5
	BASE @ <true>
	max-base 1+ 2 DO                  \ for each possible base
	I BASE !                          \ TBD: assumes base works
	I 0 <# #S #> S" 10" s= AND
	LOOP
	SWAP BASE ! ;
{ gp5 -> <true> }

: gp6
	BASE @ >R  2 BASE !
	max-uint max-uint <# #S #>        \ maximum ud to binary
	R> BASE !                         \ s: c-addr u
	DUP #bits-ud = SWAP
	0 DO                              \ s: c-addr flag
	OVER C@ [CHAR] 1 = AND            \ all ones
	>R CHAR+ R>
	LOOP SWAP DROP ;
{ gp6 -> <true> }

: gp7
	BASE @ >R  max-base BASE !
	<true>
	a 0 DO
	I 0 <# #S #>
	1 = SWAP C@ 30 I + = AND AND
	LOOP
	max-base a DO
	I 0 <# #S #>
	1 = SWAP C@ 41 I a - + = AND AND
	LOOP
	R> BASE ! ;
{ gp7 -> <true> }

\ >number tests
CREATE gn-buf 0 C,
: gn-string     gn-buf 1 ;
: gn-consumed   gn-buf CHAR+ 0 ;
: gn'           [CHAR] ' WORD CHAR+ C@ gn-buf C!  gn-string ;

{ 0 0 gn' 0' >NUMBER -> 0 0 gn-consumed }
{ 0 0 gn' 1' >NUMBER -> 1 0 gn-consumed }
{ 1 0 gn' 1' >NUMBER -> BASE @ 1+ 0 gn-consumed }
{ 0 0 gn' -' >NUMBER -> 0 0 gn-string } \ should fail to convert these
{ 0 0 gn' +' >NUMBER -> 0 0 gn-string }
{ 0 0 gn' .' >NUMBER -> 0 0 gn-string }

: >number-based
	BASE @ >R BASE ! >NUMBER R> BASE ! ;

{ 0 0 gn' 2' 10 >number-based -> 2 0 gn-consumed }
{ 0 0 gn' 2'  2 >number-based -> 0 0 gn-string }
{ 0 0 gn' f' 10 >number-based -> f 0 gn-consumed }
{ 0 0 gn' g' 10 >number-based -> 0 0 gn-string }
{ 0 0 gn' g' max-base >number-based -> 10 0 gn-consumed }
{ 0 0 gn' z' max-base >number-based -> 23 0 gn-consumed }

: gn1   \ ( ud base -- ud' len ) ud should equal ud' and len should be zero.
	BASE @ >R BASE !
	<# #S #>
	0 0 2SWAP >NUMBER SWAP DROP          \ return length only
	R> BASE ! ;
{ 0 0 2 gn1 -> 0 0 0 }
{ max-uint 0 2 gn1 -> max-uint 0 0 }
{ max-uint DUP 2 gn1 -> max-uint DUP 0 }
{ 0 0 max-base gn1 -> 0 0 0 }
{ max-uint 0 max-base gn1 -> max-uint 0 0 }
{ max-uint DUP max-base gn1 -> max-uint DUP 0 }

: gn2   \ ( -- 16 10 )
	BASE @ >R  HEX BASE @  DECIMAL BASE @  R> BASE ! ;
{ gn2 -> 10 a }

\ ------------------------------------------------------------------------
testing FILL MOVE

CREATE fbuf 00 C, 00 C, 00 C,
CREATE sbuf 12 C, 34 C, 56 C,
: seebuf fbuf C@  fbuf CHAR+ C@  fbuf CHAR+ CHAR+ C@ ;

{ fbuf 0 20 FILL -> }
{ seebuf -> 00 00 00 }

{ fbuf 1 20 FILL -> }
{ seebuf -> 20 00 00 }

{ fbuf 3 20 FILL -> }
{ seebuf -> 20 20 20 }

{ fbuf fbuf 3 CHARS MOVE -> }           \ bizarre special case
{ seebuf -> 20 20 20 }

{ sbuf fbuf 0 CHARS MOVE -> }
{ seebuf -> 20 20 20 }

{ sbuf fbuf 1 CHARS MOVE -> }
{ seebuf -> 12 20 20 }

{ sbuf fbuf 3 CHARS MOVE -> }
{ seebuf -> 12 34 56 }

{ fbuf fbuf CHAR+ 2 CHARS MOVE -> }
{ seebuf -> 12 12 34 }

{ fbuf CHAR+ fbuf 2 CHARS MOVE -> }
{ seebuf -> 12 34 34 }

\ ------------------------------------------------------------------------
testing output: . ." CR EMIT SPACE SPACES TYPE U.

: output-test
	." You should see 0-9 separated by a space:" CR
	9 1+ 0 DO I . LOOP CR
	." You should see 0-9 (with no spaces):" CR
	[CHAR] 9 1+ [CHAR] 0 DO I 0 SPACES EMIT LOOP CR
	." You should see A-G separated by a space:" CR
	[CHAR] G 1+ [CHAR] A DO I EMIT SPACE LOOP CR
	." You should see 0-5 separated by two spaces:" CR
	5 1+ 0 DO I [CHAR] 0 + EMIT 2 SPACES LOOP CR
	." You should see two separate lines:" CR
	S" line 1" TYPE CR S" line 2" TYPE CR
	." You should see the number ranges of signed and unsigned numbers:" CR
	."   signed: " min-int . max-int . CR
	." unsigned: " 0 U. max-uint U. CR
;

{ output-test -> }

\ ------------------------------------------------------------------------
testing input: ACCEPT

CREATE abuf 80 CHARS ALLOT

: accept-test
	CR ." Please type up to 80 characters:" CR
	abuf 80 ACCEPT
	CR ." received: " [CHAR] " EMIT
	abuf SWAP TYPE [CHAR] " EMIT CR
;

{ accept-test -> }

\ ------------------------------------------------------------------------
testing dictionary search rules

{ : gdx   123 ; : gdx   gdx 234 ; -> }

{ gdx -> 123 234 }
