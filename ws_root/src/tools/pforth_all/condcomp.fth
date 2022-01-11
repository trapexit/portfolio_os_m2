\ @(#) condcomp.fth 96/03/20 1.4
\ Conditional Compilation support
\
\ Words: STRINGS= [IF] [ELSE] [THEN] EXISTS?
\
\ Lifted from X3J14 dpANS-6 document.

anew task-condcomp.fth

\ COMPARE is different in JForth and ANS Forth so
\ we use this other word.
: STRINGS= { addr1 len1 addr2 len2 -- flag , true if match }
	len1 len2 =
	IF
		addr1 addr2 len1 compare 0=
	ELSE
		0
	THEN
;

: [ELSE]  ( -- )
    1
    BEGIN                                 \ level
      BEGIN
        BL WORD                           \ level $word
        COUNT  DUP                        \ level adr len len
      WHILE                               \ level adr len
        2DUP  S" [IF]"  STRINGS=
        IF                                \ level adr len
          2DROP 1+                        \ level'
        ELSE                              \ level adr len
          2DUP  S" [ELSE]"
          STRINGS=                        \ level adr len flag
          IF                              \ level adr len
             2DROP 1- DUP IF 1+ THEN      \ level'
          ELSE                            \ level adr len
            S" [THEN]"  STRINGS=
            IF
              1-                          \ level'
            THEN
          THEN
        THEN ?DUP 0=  IF EXIT THEN        \ level'
      REPEAT  2DROP                       \ level
    REFILL 0= UNTIL                       \ level
    DROP
;  IMMEDIATE

: [IF]  ( flag -- )
	0=
	IF POSTPONE [ELSE]
	THEN
;  IMMEDIATE

: [THEN]  ( -- )
;  IMMEDIATE

: EXISTS? ( <name> -- flag , true if defined )
    bl word find
    swap drop
; immediate
