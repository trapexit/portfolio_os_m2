\ @(#) catch.fth 95/11/09 1.3
\ Catch and Throw support
\
\ Lifted from X3J14 dpANS-6 document.

anew task-catch.fth

variable CATCH-HANDLER
0 catch-handler !

: CATCH  ( xt -- exception# | 0 )
	sp@ >r              ( xt ) \ save data stack pointer
	catch-handler @ >r  ( xt ) \ save previous handler
	rp@ catch-handler ! ( xt ) \ set current handler
	execute             ( )    \ execute returns if no throw
	r> catch-handler !  ( )    \ restore previous handler
	r> drop             ( )    \ discard saved stack pointer
	0                   ( )    \ normal completion
;

: THROW ( ???? exception# -- ???? exception# )
	?dup                      ( exc# ) \ 0 THROW is a no-op
	IF
		catch-handler @ rp!   ( exc# ) \ restore prev return stack
		r> catch-handler !    ( exc# ) \ restore prev handler
		r> swap >r            ( saved-sp ) \ exc# on return stack
		sp! drop r>           ( exc# ) \ restore stack
	THEN
	\ return to caller of catch
;

hex
: BAD.WORD  -5 throw ;
: NAIVE.WORD ( -- )
	7777 8888 23 . cr
	bad.word
	." After bad word!" cr
;

: CATCH.BAD ( -- )
	['] naive.word catch  .
;

: CATCH.GOOD ( -- )
	777 ['] . catch . cr
;
decimal
