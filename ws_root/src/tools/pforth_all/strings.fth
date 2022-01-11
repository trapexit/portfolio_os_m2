\ @(#) strings.fth 95/11/09 1.3
\ String support for PForth
\
\ Copyright Phil Burk 1994

ANEW TASK-STRINGS.FTH

\ Structure of string table
: $ARRAY  (  )
    CREATE  ( #strings #chars_max --  ) 
        dup ,
        2+ * even-up allot
    DOES>    ( index -- $addr )
        dup @  ( get #chars )
        rot * + 4 +
;

\ Useful for alphabetical sorting or exact compare. Case sensitive.
: COMPARE ( addr1 addr2 count -- flag , 0:equal, 1:s1>s2, -1:s1<s2 )
	>r 0 -rot  ( default result ) r>
    0
    DO  dup c@
		2 pick c@ - ?dup
        IF	0>
			IF rot drop 1 -rot
			ELSE rot drop -1 -rot
			THEN leave
        THEN
		1+ swap 1+ swap
    LOOP 2drop
;

\ Compare two strings
: $= ( $1 $2 -- flag , true if equal )
    -1 -rot
    dup c@ 1+ 0
    DO  dup c@ tolower
        2 pick c@ tolower -
        IF rot drop 0 -rot LEAVE
        THEN
		1+ swap 1+ swap
    LOOP 2drop
;

: TEXT=  ( addr1 addr2 count -- flag )
    >r -1 -rot
	r> 0
    DO  dup c@ tolower
        2 pick c@ tolower -
        IF rot drop 0 -rot LEAVE
        THEN
		1+ swap 1+ swap
    LOOP 2drop
;

: TEXT=?  ( addr1 count addr2 -- flag , for JForth compatibility )
	swap text=
;

: $MATCH?  ( $string1 $string2 -- flag , case INsensitive )
	dup c@ 1+ text=
;


: INDEX ( $string char -- false | address_char true , search for char in string )
    >r >r 0 r> r>
    over c@ 1+ 1
    DO  over i + c@ over =
        IF  rot drop
            over i + rot rot LEAVE
        THEN
    LOOP 2drop
    ?dup 0= 0=
;


: $APPEND.CHAR  ( $string char -- ) \ ugly stack diagram
    over count + c!
    dup c@ 1+ swap c!
;

\ ----------------------------------------------
: ($ROM)  ( index address -- $string )
    ( -- index address )
    swap 0
    DO dup c@ 1+ + even-up
    LOOP
;

: $ROM ( packed array of strings, unalterable )
    CREATE ( <name> -- )
    DOES> ( index -- $string )  ($rom)
;

: TEXTROM ( packed array of strings, unalterable )
    CREATE ( <name> -- )
    DOES> ( index -- address count )  ($rom) count
;

\ -----------------------------------------------
