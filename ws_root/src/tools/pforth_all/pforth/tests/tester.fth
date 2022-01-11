\ @(#) tester.fth 95/04/11 1.2
\ (c) 1993 Johns Hopkins University / Applied Physics Laboratory
\ May be distributed freely as long as this copyright notice remains.
\ Version 1.0  (Thank you John Hayes)
\ *** Modified for pForth ***

anew task-tester.fth
HEX

\ set the following flag to true for more verbose output; this may
\ allow you to tell which test caused your system to hang.
VARIABLE verbose
   FALSE verbose !
   TRUE verbose !

VARIABLE actual-depth 		\ stack record
CREATE actual-results 20 CELLS ALLOT

: empty-stack \ ( ... -- ) Empty stack.
   DEPTH dup 0>
   IF 0 DO DROP LOOP
   ELSE drop
   THEN ;

CREATE the-test 84 CHARS ALLOT

: error 	\ ( c-addr u -- ) Display an error message followed by
		\ the line that had the error.
   TYPE the-test COUNT TYPE CR \ display line corresponding to error
   empty-stack 			\ throw away every thing else
;


: {
	source the-test place
	empty-stack
;

: -> 	\ ( ... -- ) Record depth and content of stack.
   DEPTH
   DUP actual-depth ! 	\ record depth
   ?DUP IF 			\ if there is something on stack
      0 DO actual-results I CELLS + ! LOOP \ save them
   THEN
;

: } 	\ ( ... -- ) Compare stack (expected) contents with saved
		\ (actual) contents.
   DEPTH
   actual-depth @ =
   IF 	\ if depths match
      DEPTH dup 0>
      IF 		\ if there is something on the stack
         0
         DO 			\ for each stack item
            actual-results I CELLS + @ \ compare actual with expected
	        <> IF p" INCORRECT RESULT: " count error LEAVE THEN
         LOOP
      ELSE drop
      THEN
   ELSE 				\ depth mismatch
      p" WRONG NUMBER OF RESULTS: " count error
   THEN ;

: testing \ ( -- ) Talking comment.
	." Testing: "
	EOL word count type cr
;

