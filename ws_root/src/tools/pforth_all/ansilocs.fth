\ @(#) ansilocs.fth 96/04/23 1.5
\ local variable support words
\ These support the ANSI standard (LOCAL) and TO words.
\
\ They are built from the following low level primitives written in 'C':
\    (local@) ( i+1 -- n , fetch from ith local variable )
\    (local!) ( n i+1 -- , store to ith local variable )
\    (local.entry) ( num -- , allocate stack frame for num local variables )
\    (local.exit)  ( -- , free local variable stack frame )
\    local-compiler ( -- addr , variable containing CFA of locals compiler )
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

anew task-ansilocs.fth

decimal
16 constant LV_MAX_VARS    \ maximum number of local variables
31 constant LV_MAX_CHARS   \ maximum number of letters in name

lv_max_vars lv_max_chars $array LV-NAMES
variable LV-#NAMES   \ number of names currently defined

\ Search name table for match
: LV.MATCH ( $string -- index true | $string false )
    0 swap
    lv-#names @ 0
    ?DO  i lv-names
        over $=
        IF  2drop true i LEAVE
        THEN
    LOOP swap
;

variable to-flag
to-flag off
: TO  ( -- )  to-flag on  ; immediate
: ->  ( -- )  to-flag on  ; immediate

: VALUE
	CREATE ( n <name> )
." VALUE NEEDS TO BE STATE SMART !!! %Q" ABORT
		,
	DOES>
		to-flag @
		IF ! to-flag off
		ELSE @
		THEN
;
		
: LV.COMPILER  ( $name -- handled? , check for matching locals name )
\ ." LV.COMPILER: name = " dup count type cr
	lv.match
	IF ( index )
		1+ \ adjust for optimised (local@), LocalsPtr points above vars
		[compile] literal
		to-flag @
		IF
\ ." LV.COMPILER: compile local!" cr
			compile (local!)
		ELSE
\ ." LV.COMPILER: compile local@" cr
			compile (local@)
		THEN
		true
	ELSE
		drop false
	THEN
   	to-flag off
;

: LV.CLEANUP ( -- , restore stack frame on exit from colon def )
	lv-#names @
	IF
		compile (local.exit)
	THEN
;
: LV.FINISH ( -- , restore stack frame on exit from colon def )
	lv.cleanup
	lv-#names off
	local-compiler off
;

: (LOCAL)  ( adr len -- , ANSI local primitive )
	dup
	IF
		lv-#names @ lv_max_vars >= abort" Too many local variables!"
		lv-#names @  lv-names place 
		1 lv-#names +!
	ELSE
		2drop
		lv-#names @ [compile] literal   compile (local.entry)
		['] lv.compiler local-compiler !
	THEN
;

: ;      lv.finish  [compile] ;      ; immediate
: exit   lv.cleanup  compile exit   ; immediate
: does>  lv.finish  [compile] does>  ; immediate

: LV.TERM
	." Locals turned off" cr
	local-compiler off
;

if.forgotten lv.term
