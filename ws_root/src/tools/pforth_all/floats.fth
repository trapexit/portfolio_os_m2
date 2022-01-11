\ @(#) floats.fth 95/11/09 1.5
\ High Level Forth support for Floating Point
\
\ Author: Phil Burk and Darren Gibbs
\ Copyright 1994 3DO, Phil Burk, Larry Polansky, Devid Rosenboom
\
\ The pForth software code is dedicated to the public domain,
\ and any third party may reproduce, distribute and modify
\ the pForth software code or any derivative works thereof
\ without any compensation or license.  The pForth software
\ code is provided on an "as is" basis without any warranty
\ of any kind, including, without limitation, the implied
\ warranties of merchantability and fitness for a particular
\ purpose and their equivalents under the laws of any jurisdiction.

ANEW TASK-FLOATS.FTH

: FALIGNED	( addr -- a-addr )
	1 floats 1- +
	1 floats /
	1 floats *
;

: FALIGN	( -- , align DP )
	dp @ faligned dp !
;

\ account for size of create when aligning floats
here
create fp-create-size
fp-create-size swap - constant CREATE_SIZE

: FALIGN.CREATE  ( -- , align DP for float after CREATE )
	dp @
	CREATE_SIZE +
	faligned
	CREATE_SIZE -
	dp !
;

: FCREATE  ( <name> -- , create with float aligned data )
	falign.create
	CREATE
;

: FVARIABLE ( <name> -- ) ( F: -- )
	FCREATE 1 floats allot
;

: FCONSTANT
	FCREATE here   1 floats allot   f! 
	DOES> f@ 
;

: F0SP ( -- ) ( F: ? -- )
	fdepth 0 ?do fdrop loop 
;

\ Convert between single precision and floating point
: S>F ( s -- ) ( F: -- r )
	s>d d>f
;
: F>S ( -- s ) ( F: r -- )
	f>d drop
;


\ FP Output --------------------------------------------------------
fvariable FVAR-REP  \ scratch var for represent
: REPRESENT { c-addr u | n flag1 flag2 --  n flag1 flag2 , FLOATING } ( F: r -- )
	TRUE -> flag2   \ !!! need to check range
	fvar-rep f!
\
	fvar-rep f@ f0<
	IF
		-1 -> flag1
		fvar-rep f@ fabs fvar-rep f!   \ absolute value
	ELSE
		0 -> flag1
	THEN
\
	fvar-rep f@
	fdup 0 s>f f- f0=
	IF
		c-addr u ascii 0 fill
		0 -> n
	ELSE
		flog 1 s>f f+ f>s -> n   \ round up exponent
\ normalize r to u digits
		fvar-rep f@
		10 s>f u n - s>f f** f*
		1 s>f 2 s>f f/ f+   \ round result
\
\ convert float to double_int then convert to text
		f>d
		<#  u 0 do # loop #>  \ ( -- addr cnt )
		drop c-addr u move
	THEN
\
	n flag1 flag2
;

variable FP-PRECISION

: PRECISION ( -- u )
	fp-precision @
;
: SET-PRECISION ( u -- )
	fp-precision !
;
7 set-precision

create FP-OUTPUT-PAD 32 allot

: FS.  ( F: r -- , scientific notation )
	fp-output-pad precision represent
	( -- n flag1 flag2 )
	IF
		IF ." -"
		THEN
		." 0."
		fp-output-pad precision type
		." E"
		. \ n
		space
	ELSE
		." <FP-OUT-OF-RANGE>"
	THEN
;

: FP.PRINT.DECIMAL   ( n -- , print with decimal point shifted )
	>r
	fp-output-pad r@ type
	ascii . emit
	fp-output-pad r@ +
	precision r> - 1 max type
;

: FE.  ( F: r -- , engineering notation ) { | n n3 ndiff -- }
	fp-output-pad precision represent
	( -- n flag1 flag2 )
	IF
		IF ." -"
		THEN
\ convert exponent to multiple of three
		-> n
		n 1- s>d 3 fm/mod \ use floored divide
		3 * -> n3
		1+ -> ndiff \ amount to move decimal point
		ndiff fp.print.decimal
		." E"
		n3 . \ n
		space
	ELSE
		." <FP-OUT-OF-RANGE>"
	THEN
;


: F.  ( F: r -- ) { | n n3 ndiff -- }
	fp-output-pad precision represent
	( -- n flag1 flag2 )
	IF
		IF ." -"
		THEN
\ compare n with precision to see whether we do scientific display
		dup abs precision >
		IF   \ use scientific notation
			." 0."
			fp-output-pad precision type
			." E"
			. \ n
		ELSE
\ output leading zeros
			dup 0>
			IF
\ place decimal point in middle
				fp.print.decimal
			ELSE
				." 0."
				dup negate 0 ?do ascii 0 emit loop
				fp-output-pad precision rot + type
			THEN
		THEN
		space
	ELSE
		." <FP-OUT-OF-RANGE>"
	THEN
;

\ FP Input ----------------------------------------------------------
variable FP-REQUIRE-E   \ must we put an E in FP numbers?
false fp-require-e !   \ violate ANSI !!

: >FLOAT { c-addr u | dlo dhi u' fsign flag nshift -- flag }
	u 0= IF 0 s>f true exit THEN
	false -> flag
	0 -> nshift
\
\ check for minus sign
	c-addr c@ ascii - =
	dup -> fsign
	IF
		c-addr 1+ -> c-addr  \ skip past
		u 1- -> u
	THEN
\
\ convert first set of digits
	0 0 c-addr u >number -> u' -> c-addr -> dhi -> dlo
	u' 0>
	IF
\ convert optional second set of digits
		c-addr c@ ascii . =
		IF
			dlo dhi c-addr 1+ u' 1- dup -> nshift >number
			dup nshift - -> nshift
			-> u' -> c-addr -> dhi -> dlo
		THEN
\ convert exponent
		u' 0>
		IF
			c-addr c@ ascii E =
			IF
				c-addr 1+ u' 1- ((number?))
				num_type_single =
				IF
					nshift + -> nshift
					true -> flag
				THEN
			THEN
		ELSE
\ only require E field if this variable is true
			fp-require-e @ not -> flag
		THEN
	THEN
\ convert double precision int to float
	flag
	IF
		dlo dhi d>f
		10 s>f nshift s>f f** f*   \ apply exponent
		fsign
		IF
			fnegate
		THEN
	THEN
	flag
;

3 constant NUM_TYPE_FLOAT   \ possible return type for NUMBER?

: (FP.NUMBER?)   ( $addr -- 0 | n 1 | d 2 | r 3 , convert string to number )
\ check to see if it is a valid float, if not use old (NUMBER?)
	dup count >float
	IF
		drop NUM_TYPE_FLOAT
	ELSE
		(number?)
	THEN
;

defer fp.old.number?
variable FP-IF-INIT

: FP.TERM    ( -- , deinstall fp conversion )
	fp-if-init @
	IF
		what's 	fp.old.number? is number?
		fp-if-init off
	THEN
;

: FP.INIT  ( -- , install FP converion )
	fp.term
	what's number? is fp.old.number?
	['] (fp.number?) is number?
	fp-if-init on
	." Floating point numeric conversion installed." cr
;

FP.INIT
if.forgotten fp.term
