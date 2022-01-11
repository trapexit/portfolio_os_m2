\ @(#) quit.fth 95/11/15 1.4
\ Outer Interpreter in Forth
\
\ This used so that THROW can be caught by QUIT.
\
\ Author: Phil Burk
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

include? catch catch.fth

anew task-quit.fth

: FIND&COMPILE ( $word --  {n} , find word in dictionary and handle it )
	dup >r   \ save in case needed
	find ( -- xt flag | $word 0 )

	CASE
		-1 OF           \ not immediate
			state @     \ compiling?
			IF compile,
			ELSE execute
			THEN
		ENDOF

		1 OF execute    \ immediate, so execute regardless of STATE
		ENDOF
		
		0 OF
			number?     \ is it a number?
			num_type_single =
			IF   ?literal  \ compile it or leave it on stack
			ELSE
				r@ count type ."   is not recognized!!" cr
				abort
			THEN
		ENDOF
	ENDCASE
	
	rdrop
;

\ interpret whatever is in source
: INTERPRET ( ?? -- ?? )
	BEGIN
		>in @ source nip ( 1- ) <   \ any input left? !!! is -1 needed?
	WHILE
		bl word
		dup c@ 0>
		IF
			0 >r \ flag
			local-compiler @
			IF
				dup local-compiler @ execute  ( ?? -- ?? )
				r> drop TRUE >r
			THEN
			r> 0=
			IF
				find&compile   ( -- {n} , may leave numbers on stack )
			THEN
		ELSE
			drop
		THEN
	REPEAT
;

: EVALUATE ( i*x c-addr num -- j*x , evaluate string of Forth )
\ save current input state and switch to pased in string
	source >r >r
	set-source
	-1 push-source-id
	>in @ >r
	0 >in !
\ interpret the string
	interpret
\ restore input state
	pop-source-id drop
	r> >in !
	r> r> set-source
;

: POSTPONE  ( <name> -- )
	bl word find
	CASE
		0 OF ." Postpone could not find " count type cr abort ENDOF
		1 OF compile, ENDOF \ immediate
		-1 OF (compile) ENDOF \ normal
	ENDCASE
; immediate
		
: OK
	depth 0<
	IF
		." QUIT: Stack underflow!"
		depth negate 0
		?DO 0
		LOOP
	ELSE
		."  OK  "
		trace-stack @
		IF   .s
		THEN
		cr
	THEN
;

variable QUIT-QUIT

: QUIT  ( -- , interpret input until none left )
	quit-quit off
	postpone [
	BEGIN
		refill
		quit-quit @ 0= and
	WHILE
		." TIB = " source type cr
		['] interpret catch ?dup
		IF
			." Exception # " . cr
		ELSE
			state @ 0= IF ok THEN
		THEN
	REPEAT
;
