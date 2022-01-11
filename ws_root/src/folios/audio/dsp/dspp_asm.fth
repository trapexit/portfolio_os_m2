\ @(#) dspp_asm.fth 96/07/16 1.36
\ $Id: dspp_asm.fth,v 1.25 1995/03/14 07:10:19 phil Exp $
echo off
\ DSPP RPN Assembler
\
\ by Phil Burk
\ Copyright 1992,93,94 3DO
\ All rights reserved.
\ Proprietary and confidential.
\
\ ------------------------------------------------------
\ X- allow double RBASE references
\ X- improve syntax for DINI chunks
\ O- trap for known DSPP bugs
\ O- fix _*NEG bad ALUB field, should be 11 for MULT output
\ O- rethink ins{ initialization
\ ------------------------------------------------------

\ Copyright 1992 NTG
\ 9/17/92 PLB Add barrel shift commands  _>> _>>'
\ 9/21/92 PLB Add _=A
\ 10/6/92 PLB Add ins{ and reference code for creating OFX files
\ 10/7/92 PLB Convert to new DRLC/DRSC/DCOD format.
\ 00001 PLB 10/12/92 Fix knob resource index, add FIFO attributes
\           add _EXPORT
\ 00002 PLB 11/12/92 Add ALLOC.OUTFIFO, effectively merge IMEM and OUTPUT
\ 00003 PLB 11/15/92 Change instrument linking procedure
\ 00004 PLB 12/14/92 Use DRLC_LIST for FIFOs
\ 930103 PLB Allow multiple references to RBASE, !!!! Oh?
\ 930126 PLB Output to "output.dsp" instead of "output.ofx"
\ 930226 PLB Fixed stack errors in dspp.operand, use safecount,
\ 930301 PLB Allow RBASE to go up to 31 for RED
\ 930504 PLB Fix BHI code, was FD00, now F000
\ 930504 PLB Fix Negate, must take ALUB as input.
\ 930611 PLB Added envelope specification
\ 930804 PLB Translate _ to . for knob names, kludge until chordic fixed
\ 930810 PLB Only translate _ to . for knob names if if-translate-unders on
\ 930824 PLB Added embedded samples
\ 930824 PLB Added support for DFID chunk
\ 940106 PLB Fixed stack leftover in _MOVE
\ 940106 PLB Clear dspp-type-ref in set.knob.calc
\ 9403?? PLB Added support for ADC
\ 940513 PLB Write 0 byte to pad end of chunk
\ 940520 PLB Added ANVIL_VERSION, and BULLDOG versions, allow 1024 of EN,
\            clean up linked list handlers,
\            use more structured references to node contents
\ 940601 PLB Added support for shared library templates.
\ 940610 PLB c/OVER/SWAP/ in fifo.alloc.rsrc which caused stack error
\ 940610 PLB Added easier DINI syntax
\ 940713 PLB Check for unresolved local labels.
\ 940818 PLB Added \\\ comment for AutoDocs
\ 941101 PLB Converted to use ANSI style file I/O for compatibility
\            with PForth
\ 941201 PLB Increase DSPP_MAX_BRANCHES from 64 to 128
\ 950223 PLB Added _IMPORT support for ALLOC.IMEM
\ 950407 PLB Cleaned up resource writing words.
\            Added support for resource binding, DRLC_BIND
\            Added M2 RBASE
\ 950414 PLB Add BIND.RESOURCE
\ 950419 PLB Fix resource index for RBases by setting index in rsrc>dins
\ 950420 PLB Moved log{ to log_to_file.fth so test_tools.fth can use it.
\ 950428 PLB Added ALLOC.TRIGGER
\ 950525 PLB track RBASE references using linked list to allow double RBASE refs
\ 950530 PLB Add NUL to end of ins-name so template has valid NAME chunk.
\ 950628 PLB Convert to new M2 style. No longer supports Opera DSP format
\ 950807 PLB Converted to relative likage in resource lists. Branches use DRLC_LIST
\ 950809 PLB Everything is DRLC_LIST so eliminate drlc_Flags
decimal

include? log.type log_to_file.fth

anew task-dspp_asm.fth

: enum:  ( n <name> -- n+1 )
	dup 1+ swap constant
;

: debug:
	>in @
	bl word count type cr
	>in !
	:
;

: \\\ ( <line> -- , comment rest of line 940818, for autodocs )
	[compile] \
;

: $"    ( <string> -- addr , return string address, allot space for string )
	state @
	IF compile (c")   ,"
	ELSE ascii " parse tuck
		here place
		here
		swap 1+ ( for count byte )
		allot align
	THEN
; immediate

: safecount ( $string -- addr count<32 )
	count 31 and
;

: BYTE: ( offset <name> -- )
	dup 1+ swap constant
;
: LONG: ( offset <name> -- )
	dup 4 + swap constant
;

: translate.chars { addr cnt fromchar tochar -- }
	cnt 0
	do
		addr i + c@ fromchar =
		if
			tochar addr i + c!
		then
	loop
;

\ code space declaration

1024 constant EN_SIZE_MAX
variable EN-SIZE \ of DSPP instruction space

EN_SIZE_MAX array en-image

\ version control ------------------------------
variable dspp-version
1 constant BLUE_VERSION
2 constant RED_VERSION
3 constant GREEN_VERSION
4 constant ANVIL_VERSION
5 constant BULLDOG_VERSION

: warn.m2.only cr ." DSPP Assembler only generates M2 compatible DSP files!" cr ;

: _BLUE BLUE_VERSION   warn.m2.only dspp-version !   512 EN-SIZE ! ;
: _RED RED_VERSION     warn.m2.only dspp-version !   512 EN-SIZE ! ;
: _GREEN GREEN_VERSION warn.m2.only dspp-version !   512 EN-SIZE ! ;
: _ANVIL ANVIL_VERSION warn.m2.only dspp-version !   512 EN-SIZE ! ;
: _BULLDOG BULLDOG_VERSION dspp-version !   EN_SIZE_MAX EN-SIZE ! ;

_BULLDOG

: _BLUE?  ( -- flag , true if BLUE )
	dspp-version @ BLUE_VERSION =
;
: _RED?  ( -- flag , true if RED )
	dspp-version @ RED_VERSION =
;

\ new red only
: dspp.m2.only ( -- )
	dspp-version @ BULLDOG_VERSION < abort" Only on or after M2!"
;
: dspp.before.m2.only ( -- )
	dspp-version @ BULLDOG_VERSION < not abort" Only before M2!"
;

\ ----------------------------------------------------------------
4 constant DHDR_FORMAT_VERSION   \ must match value in dspp.h
1 constant DHDR_F_PRIVILEGED
2 constant DHDR_F_SHARED
4 constant DHDR_F_PATCHES_ONLY

\ OFX file support
\ Constants for OFX file
0 constant DCOD_RUN_DSPP    \ code segment
1 constant DCOD_INIT_DSPP
2 constant DCOD_ARGS

\ {{{{{{{{{{{{{{{{{{{{{{ These must match audio.h !!!!!!!!
0
enum: DRSC_TYPE_CODE
enum: DRSC_TYPE_KNOB
enum: DRSC_TYPE_VARIABLE
enum: DRSC_TYPE_INPUT
enum: DRSC_TYPE_OUTPUT
enum: DRSC_TYPE_IN_FIFO
enum: DRSC_TYPE_OUT_FIFO
enum: DRSC_TYPE_TICKS
enum: DRSC_TYPE_RBASE
enum: DRSC_TYPE_TRIGGER
\ hardware addresses that may change between ASIC versions
enum: DRSC_TYPE_HW_DAC_OUTPUT
enum: DRSC_TYPE_HW_OUTPUT_STATUS
enum: DRSC_TYPE_HW_OUTPUT_CONTROL
enum: DRSC_TYPE_HW_ADC_INPUT
enum: DRSC_TYPE_HW_INPUT_STATUS
enum: DRSC_TYPE_HW_INPUT_CONTROL
enum: DRSC_TYPE_HW_NOISE
enum: DRSC_TYPE_HW_INT_CPU
enum: DRSC_TYPE_HW_CLOCK
drop

\ Subtype controls hardware decompression
0 constant DRSC_INFIFO_SUBTYPE_16BIT
1 constant DRSC_INFIFO_SUBTYPE_8BIT
2 constant DRSC_INFIFO_SUBTYPE_SQS2

$ FF constant DRSC_TYPE_MASK
\ Do not change anything in between {{{ }}} here cuz it matches dspp.h !!!!!!

\ Attributes
0
enum: DRSC_FIFO_NORMAL
enum: DRSC_FIFO_STATUS
enum: DRSC_FIFO_READ
enum: DRSC_FIFO_OSC   \ M2 set of regs for OSCillator
enum: DRSC_FIFO_NUM_PARTS
drop

$ 8000 constant DRSC_IMPORT
$ 4000 constant DRSC_EXPORT
$ 2000 constant DRSC_BIND
$ 1000 constant DRSC_BOUND_TO  \ some other resource has bound to this one
$ 0200 constant DRSC_AT_START
$ 0100 constant DRSC_AT_ALLOC

0 [if]
            . knob types:
                sample rate ratio:
                    type: unsigned
                    range: 0.0..88100.0 Hz *
                oscillator frequency:
                    type: signed
                    range: -22050.0..22050.0 Hz *
                lfo frequency:
                    type: signed
                    range: -86.1..86.1 Hz *
                signed (raw) signal (e.g. amplitude):
                    type: signed
                    range: -1.0..1.0
                unsigned (raw) signal
                    type: unsigned
                    range: 0.0..2.0
[then]

0
enum: AF_PORT_TYPE_INPUT
enum: AF_PORT_TYPE_OUTPUT
drop

0
enum: KNOB_TYPE_RAW_SIGNED
enum: KNOB_TYPE_RAW_UNSIGNED
enum: KNOB_TYPE_OSC_FREQ
enum: KNOB_TYPE_LFO_FREQ
enum: KNOB_TYPE_SAMPLE_RATE
enum: KNOB_TYPE_WHOLE_NUMBER
drop

16 constant DRSC_SUBTYPE_SHIFT   \ shift by 16 to put in second byte of DSPPResource Structure

: or.drsc.subtype  ( RsrcType SubType -- RsrcType|SubType
\	." or.drsc.subtype: " over .hex dup .hex cr
	$ FF and
	DRSC_SUBTYPE_SHIFT lshift
	or
;

32 constant AF_MAX_NAME_SIZE

1 constant DINI_AT_ALLOC
2 constant DINI_AT_START

\ }}}}}}}}}}}}}}}}}}}}}}}} These must have matched audio.h !!!!!!!!


variable dspp-function-id
variable dspp-header-flags


\ data structures -----------------------------------------

variable dspp-echo    \ echo lines to screen if true
variable dspp-debug   \ echo stuff if debugging assembler
variable eni-org \ current EN address
variable dspp-dynamic-links  \ hold dynamic link info for shared subroutines

variable dspp-opcode  \ construct image of opcode
variable dspp-if-mult \ on if a multiply operation is involved

variable dspp-if-export
variable dspp-if-import
: _EXPORT dspp-if-export on ;
: _IMPORT dspp-if-import on ;

variable _=-num      \ numeric address for _= command
variable _=-type     \ operand type for _= command
variable dspp-barrel-num
variable dspp-barrel-type
variable dspp-barrel-dir   \ 0 = left, 1 = right-logical, 2 = right-arith

\ error detection and control
variable trap-long-short   \ blue bug  makes the following illegal
\  x+y
\  z=A

\ keep an array of branch lists for relocation by audiofolio
variable dspp-num-labels        \ current number of labels
128 constant DSPP_MAX_LABELS    \ maximum number of labels
DSPP_MAX_LABELS array dspp-label-addrs  \ code address of label
DSPP_MAX_LABELS barray dspp-label-flags  \ code address of label
DSPP_MAX_LABELS array dspp-branch-heads \ array of branch lists for relocation by audiofolio
DSPP_MAX_LABELS 32 $array dspp-symbols

$ 89ABDEAD constant DSPP_UNRESOLVED_LABEL
1 constant LABEL_F_EXPORTED    \ set in dspp-label-flags
char ~ constant DSPP_ILLEGAL_LABEL_CHAR   \ used to mark locals as unuseable

: eni.legal? ( addr -- , validate it )
	dup 0 EN-SIZE @ within not
	if
		." EN address space exceeded = " .hex abort
	else drop
	then
;


: .n { val ndigs -- , print a value using n digits }
	val 0
	<# ndigs 1- 0 max 0 do # loop #s #>
	log.type
;

: eni@ ( &en -- val , get from EN image )
	dup eni.legal?
	en-image @
;

: dspp.dump.en ( addr -- )
	dup p" 0x" count log.type 4 .n bl log.emit
	eni@ p" 0x" count log.type 4 .n log.cr
;

: (eni!) ( val &en -- )
	en-image !
;

: eni! ( val &en -- )
	dup eni.legal?
	dspp-echo @
	if
		tuck (eni!)
		base @ >r hex
		dspp.dump.en
		r> base !
	else
		(eni!)
	then
;

: _ORG ( address -- )
	dup eni.legal?
	eni-org !
;

: eni, ( val -- , save sequentially in EN image )
	eni-org @ eni!
	eni-org @ 1+ _ORG
;
\ ---------------------------------------------------------------------------
\ The Type value has encoded information about the operand type
\ as well resource information for the building of .dsp instruments.
\ Use high word of type to control allocation and reference for
\ instrument file building.

\ Organization of TYPE value:
24 constant TYPE_PART_SHIFT         \ shifter for attribute/part info
$ FF TYPE_PART_SHIFT lshift constant TYPE_PART_MASK
16 constant TYPE_RESOURCE_SHIFT         \ shifter for resource info
$ FF TYPE_RESOURCE_SHIFT lshift constant TYPE_RESOURCE_MASK

TYPE_RESOURCE_MASK TYPE_PART_MASK or constant TYPE_REFERENCE_MASK
  \ info for allocated resource references
$ 0000FFFF constant TYPE_OPERAND_MASK  \ info for allocated resource references

1 TYPE_RESOURCE_SHIFT lshift constant _REFERENCE_EI
2 TYPE_RESOURCE_SHIFT lshift constant _REFERENCE_I
3 TYPE_RESOURCE_SHIFT lshift constant _REFERENCE_EO
4 TYPE_RESOURCE_SHIFT lshift constant _REFERENCE_FIFO
5 TYPE_RESOURCE_SHIFT lshift constant _REFERENCE_OUTPUT
6 TYPE_RESOURCE_SHIFT lshift constant _REFERENCE_HARDWARE
7 TYPE_RESOURCE_SHIFT lshift constant _REFERENCE_TRIGGER

variable dspp-type-ref   \ or this with next type
variable dspp-rbase-ref  \ is next RBASE allocated

: PART+  { addr partnum  -- addr' }
	dspp-type-ref @ TYPE_REFERENCE_MASK and
	IF
		partnum $ FF invert and abort" PART+ partnum out of range!"
		partnum TYPE_PART_SHIFT lshift
		dspp-type-ref @ or
\ ." PART+ - type = " dup .hex cr
		dspp-type-ref !
		addr
	ELSE
		addr partnum +
	THEN
;

: FIFOSTATUS+ ( -- )
	dspp-type-ref @
	DRSC_FIFO_STATUS TYPE_PART_SHIFT lshift +
	dspp-type-ref !
;
: FIFOREAD+ ( -- )
	dspp-type-ref @
	DRSC_FIFO_READ TYPE_PART_SHIFT lshift +
	dspp-type-ref !
;

: dspp.get.type.part ( type -- )
	TYPE_PART_SHIFT rshift $ FF and
;

\ Define Type codes so that we can turn on/off writeback and
\ indirect with bit masks.
$ 0001 constant TYPE_F_INDIRECT
$ 0002 constant TYPE_F_WRITEBACK
$ 0004 constant TYPE_F_ADDRESS
$ 0008 constant TYPE_F_REG
$ 0010 constant TYPE_F_ACCUME
$ 0020 constant TYPE_F_INSTANT_BARREL
$ 0040 constant TYPE_F_IMMEDIATE

TYPE_F_REG constant _REG
_REG TYPE_F_INDIRECT or constant _[REG]
_REG TYPE_F_WRITEBACK or constant _%REG
_REG TYPE_F_INDIRECT or TYPE_F_WRITEBACK or constant _[%REG]

TYPE_F_ADDRESS TYPE_F_INDIRECT or constant TYPE_[A]
TYPE_F_ADDRESS TYPE_F_WRITEBACK or constant TYPE_%A
TYPE_F_ADDRESS TYPE_F_INDIRECT or TYPE_F_WRITEBACK or constant TYPE_[%A]

TYPE_F_INSTANT_BARREL constant _'   \ instant barrel shift

: _ref_type: ( code -- )
	create ,
	does> @ dspp-type-ref @ or
		dspp-type-ref off
;

TYPE_F_ADDRESS _ref_type: _A
TYPE_[A] _ref_type: _[A]
TYPE_%A _ref_type: _%A
TYPE_[%A] _ref_type: _[%A]
TYPE_F_IMMEDIATE _ref_type: _#



\ define resource structure for assembler only, not a file structure
0
LONG: drs_Next          \ linked list pointer
LONG: drs_RsrcType      \ type of resource, first 4 bytes of DSPPResource ORed together
       \ reserved | CalcType | Flags | Type
LONG: drs_NumParts      \ number of parts, a single FIFO may have multiple parts
LONG: drs_PartRefs      \ Array of refs for each part
    \ For each part will contain Code Offset of first reference
    \ which may be a linked list.
    \ set to -1 if unreferenced or >= 0
LONG: drs_Many          \ How many to allocate
LONG: drs_Index         \ resource index in .dsp file, assigned when written
LONG: drs_NamePtr       \ address of saved mixed case name
LONG: drs_BoundTo       \ pfa of resource this is bound to, or zero
LONG: drs_AllocOffset   \ offset from allocated resource
LONG: drs_ReferenceType \ type of reference, eg. _REFERENCE_EI
LONG: drs_ReferenceCount  \ number of time used as an operand, must be >1 at end or error
LONG: drs_DefaultValue   \ default at alloc, can be overridden by DINI
constant drs_size

drs_size
LONG: rbase_RefNodeList  \ real list of rbase references
constant rbase_size

0
LONG: rbnd_Next        \ singly linked list
LONG: rbnd_RefAddress  \ code address of reference to RBASE
constant rbnd_size

drs_size
LONG: dini_ResourcePFA   \ PFA of associated Resource allocation
LONG: dini_WhenFlags     \ eg. DINI_AT_ALLOC
LONG: dini_DataPtr       \ where is the data
constant dini_size

\ =====================================================================
\ single linked list tools for list in dictionary
\ assume cell 0 contains a single pointer to next node
0 constant SLL_TERMINATOR
: sll.get.tail ( head -- tail-node | 0 , scan list for tail )
	begin dup @ SLL_TERMINATOR <>  \ are there more nodes?
	while @
	repeat
;
: sll.add.tail ( node head -- )
	sll.get.tail ( -- node tail )
	over swap ! ( point tail to node )
	SLL_TERMINATOR swap ! ( terminate list at node )
;
: sll.add.head ( node head -- )
	2dup @ swap !  \ store what's in head to node
	!              \ point head to node
;

: sll.remove.head { head | first second -- }
	head @ SLL_TERMINATOR =
	IF
		." Attempt to remove head of empty list!"
	ELSE
		head  @ -> first
		first @ head !    \ unlink
	THEN
;

\ ---------------------------------------------------------------
\ linked list tools for list in EN memory
\ ref is an address in en memory
\ The list consists of opcode fields masked by $FC00
\     with link fields masked by $03FF.
\ The link fields contain relative offsets from the current node.
\ A link field of zero terminates the list.

: dspp.next.node.eni  { ref -- next-en-addr }
	ref eni@ $ 03FF and
	ref +   \ use relative addressing
;

: dspp.link.nodes.eni  { ref ref_next -- , ref1 links to ref2 }
	ref eni@ $ FC00 and ( -- opcode-of-last-node )
	ref_next ref -    \ use relative addressing
	dup 0< abort" dspp.link.nodes.eni - backwards link"
	or ( or with next node address )
	ref eni!
;
: dspp.scan.list.eni { ref data xcfa | rcur rnext -- lastref , scan linked list }
	begin
\ ." dspp.scan.list.eni - ref = $" ref .hex data .hex xcfa .hex cr
		ref dspp.next.node.eni -> rnext
		ref data xcfa execute   ( ref data -- , custom function )
		rnext ref = not
	while
		rnext -> ref
	repeat
	ref
;

: dspp.find.tail.eni ( ref -- lastref , return end of linked list )
	0 ['] 2drop dspp.scan.list.eni
;

: dspp.add.node.eni { ref en-addr -- , point last node to en-addr }
	dspp-debug @
	IF ." dspp.add.node.eni - ref = $" ref .hex
		." , en-addr = $" en-addr .hex cr
	THEN
	ref dspp.find.tail.eni -> ref \ find last node
	dspp-debug @
	IF ." dspp.add.node.eni - last = " ref . cr
	THEN
\
\ make end of list point to current location
	ref en-addr dspp.link.nodes.eni
;

: dspp.add.org.eni ( ref -- , add a reference to eni-org to list )
	eni-org @ dspp.add.node.eni
;

: drs.partref[]  { indx drs  -- addr , of partref }
	drs drs_PartRefs + @
	dup 0= abort" Attempted to reference to unallocated drs_PartRefs!"
	indx cells +
;

: is.label.local? ( label# -- flag , is it locally scoped? )
	dspp-symbols 1+ c@
	[char] @ =
;

\ ================================ CODE ASSEMBLY =====================


: ACCUME 0 TYPE_F_ACCUME ;

: A$ ( <num> -- num type )
	[compile] $
	_A ?literal
; immediate


: %A$ ( <num> -- num type )
	[compile] $
	_%A ?literal
; immediate

: [A]$ ( <num> -- num type )
	[compile] $
	_[A] ?literal
; immediate


: [%A]$ ( <num> -- num type )
	[compile] $
	_[%A] ?literal
; immediate

: #$ ( <num> -- num type )
	[compile] $
	_# ?literal
; immediate

: reg:
	create ( reg# -- ) ,
	does> @ _reg
;

0  dup reg: R0
1+ dup reg: R1
1+ dup reg: R2
1+ dup reg: R3
1+ dup reg: R4
1+ dup reg: R5
1+ dup reg: R6
1+ dup reg: R7
1+ dup reg: R8
1+ dup reg: R9
1+ dup reg: R10
1+ dup reg: R11
1+ dup reg: R12
1+ dup reg: R13
1+ dup reg: R14
1+ dup reg: R15
drop


: %reg:
	create ( reg# -- ) ,
	does> @ _%reg
;

0  dup %reg: %R0
1+ dup %reg: %R1
1+ dup %reg: %R2
1+ dup %reg: %R3
1+ dup %reg: %R4
1+ dup %reg: %R5
1+ dup %reg: %R6
1+ dup %reg: %R7
1+ dup %reg: %R8
1+ dup %reg: %R9
1+ dup %reg: %R10
1+ dup %reg: %R11
1+ dup %reg: %R12
1+ dup %reg: %R13
1+ dup %reg: %R14
1+ dup %reg: %R15
drop

: [reg]:
	create ( reg# -- ) ,
	does> @ _[reg]
;

0  dup [reg]: [R0]
1+ dup [reg]: [R1]
1+ dup [reg]: [R2]
1+ dup [reg]: [R3]
1+ dup [reg]: [R4]
1+ dup [reg]: [R5]
1+ dup [reg]: [R6]
1+ dup [reg]: [R7]
1+ dup [reg]: [R8]
1+ dup [reg]: [R9]
1+ dup [reg]: [R10]
1+ dup [reg]: [R11]
1+ dup [reg]: [R12]
1+ dup [reg]: [R13]
1+ dup [reg]: [R14]
1+ dup [reg]: [R15]
drop

: [%reg]:
	create ( reg# -- ) ,
	does> @ _[%reg]
;

0  dup [%reg]: [%R0]
1+ dup [%reg]: [%R1]
1+ dup [%reg]: [%R2]
1+ dup [%reg]: [%R3]
1+ dup [%reg]: [%R4]
1+ dup [%reg]: [%R5]
1+ dup [%reg]: [%R6]
1+ dup [%reg]: [%R7]
1+ dup [%reg]: [%R8]
1+ dup [%reg]: [%R9]
1+ dup [%reg]: [%R10]
1+ dup [%reg]: [%R11]
1+ dup [%reg]: [%R12]
1+ dup [%reg]: [%R13]
1+ dup [%reg]: [%R14]
1+ dup [%reg]: [%R15]
drop

: _= ( num type -- )
	_=-type !
	_=-num !
;
: or.op  ( val -- )
	dspp-opcode @ or dspp-opcode !
;


: dspp.alu.#ops? ( alu_code -- #ops )
	>r 1
	r@ 2 =
	r@ 4 = or
	r@ 9 > or
	if drop 2
	then
;


variable dspp-reg-keep
variable dspp-rtype-keep
variable dspp-reg-cnt  \ registers in a row
variable dspp-reg-cache \ operand word cache

: dspp.or.reg.cache ( val -- , )
	dspp-reg-cache @ or dspp-reg-cache !
;

: dspp.reg>cache  { reg# indx -- , accumulate registers }
	reg# dspp-reg-keep + c@
	reg# dspp-rtype-keep + c@ $ 1 and \ indirect bit on?
	if $ 10 or then
	indx 5 * shift
	dspp.or.reg.cache
;

: dspp.reg.flush ( -- )
	dspp-reg-cnt @ 0>
	if
		dspp-reg-cnt @
		case
			3 of
				0 2 dspp.reg>cache \ put 1st at R3
				1 1 dspp.reg>cache \ put 2nd at R2
				2 0 dspp.reg>cache \ put 3rd at R1
\ BLUE BUG, check for accidental writeback
				dspp-reg-cache @ $ 1800 and 0<>
				dspp-version @ RED_VERSION < and
				if
					." Accidental writeback cuz of BLUE BUG!" cr
					dspp-reg-cache off
					dspp-reg-cnt off
					abort
				then
\
				dspp-reg-cache @ eni,
				dspp-rtype-keep @ $ 02020202 and
				if
					." DSPP assembler internal error! 3 reg w/ writeback!" cr
					abort
				then
			endof
			2 of
				0 1 dspp.reg>cache \ put 1st at R2
				0 dspp-rtype-keep + c@ 2 and \ writeback mode?
				if $ 1000 dspp.or.reg.cache
				then
				1 0 dspp.reg>cache \ put 2nd at R1
				1 dspp-rtype-keep + c@ 2 and \ writeback mode?
				if $ 0800 dspp.or.reg.cache
				then
				$ A400 dspp-reg-cache @ or eni,
			endof
			1 of
				0 0 dspp.reg>cache \ put 1st at R1
				0 dspp-rtype-keep + c@ 2 and \ writeback mode?
				if $ 0800 dspp.or.reg.cache
				then
				$ A000 dspp-reg-cache @ or eni,
			endof
			." Too many registers!" abort
		endcase
	then
	dspp-reg-cache off
	dspp-reg-cnt off
;

: dspp.range.address ( addr -- )
	dup 0 $ 400 within not
	if
		." Address out of legal range = $" .hex abort
	else
		drop
	then
;
: dspp.range.immediate ( imm -- )
	dup $ -8000 $ FFFF within not
	if
		." Immediate exceeds 16 bits = $" .hex abort
	else
		drop
	then
;
: dspp.range.reg ( reg# -- )
	dup 0 16 within not
	if
		." Illegal register number = " . abort
	else
		drop
	then
;

: dspp.reg.oper  { numA typeA -- , accumulate registers }
	numA dspp.range.reg
	dspp-reg-cnt @ 2 >
	if
		dspp.reg.flush
	else
\ if there are any writebacks, don't let it get to 3
		dspp-reg-cnt @ 2 =
		if
			typeA $ 2 and 0<>  \ writeback in this type
			dspp-rtype-keep @ $ 02020202 and 0<> or
			if
				dspp.reg.flush
			then
		then
	then
\
	numA dspp-reg-cnt @ dspp-reg-keep + c!
\
	typeA $ 3 and
	dspp-reg-cnt @ dspp-rtype-keep + c!
\
	1 dspp-reg-cnt +!
;

: dspp.immediate { numA -- , compile an immediate operand }
	numA dspp.range.immediate
	numA $ F000 and 0=
	numA $ F000 and $ F000 = or
	if
		numA $ 1FFF and $ C000 or eni, \ no shift, gets right justified
	else
		numA $ 7 and 0=
		if
			numA -3 shift  \ justify
			$ E000 or eni,
		else
			." Immediate value out of range = " numA .hex cr
			abort
		then
	then
;

: dspp.link.reference { drs typeA | partref -- }
\ ." link: " drs drs_NamePtr + @ id. space typeA .hex cr
	typeA dspp.get.type.part
\ ." link: partnum = " dup . cr
	drs drs.partref[] -> partref
\ ." link: partref = " partref .hex cr
\
	partref @ 0< \ address of first dspp reference in list of fields
	if
		eni-org @ partref
\ ." link: 1st " 2dup .hex .hex cr
		!
	else
\ ." link: later " partref @ .hex cr
		partref @ dspp.add.org.eni
	then
	1  drs drs_ReferenceCount + +!  \ increment use count
\ ." link: done" cr
;

: dspp.operand,  { numA typeA | opmask -- , lay down operand }
dspp-debug @ if ." dspp.operand : numA = " numA .hex ." , typeA = " typeA .hex cr then
	typeA _REG and
	if
		numA typeA dspp.reg.oper
	else
		dspp.reg.flush
		typeA TYPE_REFERENCE_MASK and 0=
		if
			typeA TYPE_OPERAND_MASK and
			case
				TYPE_F_ACCUME of ( default in mux ) endof
				_# of numA dspp.immediate endof
\ assume the operand is an address
				numA dspp.range.address
				TYPE_F_ADDRESS of numA $ 8000 or eni, endof
				TYPE_[A] of numA $ 8400 or eni, endof \ indirect
				TYPE_%A of numA $ 8800 or eni, endof \ writeback
				TYPE_[%A] of numA $ 8C00 or eni, endof \ writeback+indirect
				dup .hex ."  = Unsupported DSPP operand type!" abort
			endcase
		else
\ reference needs to be recorded
			typeA TYPE_OPERAND_MASK and
			case
\	_# of numA $ C000 -> opmask endof \ 920226 extra numA blew stack
				_# of $ C000 -> opmask endof
				TYPE_F_ADDRESS of $ 8000 -> opmask endof
				TYPE_[A] of $ 8400 -> opmask endof \ indirect
				TYPE_%A of $ 8800 -> opmask endof \ writeback
				TYPE_[%A] of $ 8C00 -> opmask endof \ writeback+indirect
				dup .hex ."  = Unsupported DSPP operand type!" abort
			endcase
			numA typeA dspp.link.reference \ compile linked reference
			opmask eni,
		then
	then
;

: dspp.echo.line ( -- )
	dspp-echo @
	if
		>newline p" # " count log.type
		source log.type \ log.cr
	then
;

: dspp.reset.op
	dspp-if-mult off
	dspp-reg-cache off
	dspp-reg-cnt off
	dspp-barrel-type off
	_=-type off
;

: dspp.opcode, ( opcode -- )
	dspp.echo.line
	eni,
;
\ OP     ALUA ALUB  code
\ -R5      A    1    1
\ R7+C     1    A    4
\ R7+A     1    A    4
\ A+R7     A    1    1
\ R7+R8    1    2    6
\ R7*R8+C  MULT A    C
\ R0*R6-R7 MULT 1    D

\ typeA and typeB are Multiplier inputs if ifmult on
\ typeB and typeC are ALU inputs if ifmult off

\ build ALU_MUX field for ALU
\    00 = ACCUME
\    01 = ALU_OP1
\    10 = ALU_OP2
\    11 = result from Multiply
: dspp.calc.mux { typeA typeB typeC ifmult | op1used mux -- mux mask }
	false -> op1used
	0 -> mux
	ifmult
	if
		$ 0C00 mux or -> mux
	else
		typeB
		TYPE_F_REG TYPE_F_ADDRESS or
		TYPE_F_IMMEDIATE or
		and  \ were any of the above types used?
		if
			true -> op1used
			$ 0400 mux or -> mux
		then
	then
\
	typeC
	TYPE_F_REG TYPE_F_ADDRESS or
	TYPE_F_IMMEDIATE or
	and  \ were any of the above types used?
	if
		op1used
		if $ 0200
		else $ 0100
		then
		mux or -> mux
	then
\
	mux
;

: dspp.another.op? ( numops type -- numops | numops+1 )
	dup 0=
	swap TYPE_F_ACCUME = or not
	if 1+
	then
;

\  -------------------------------------------------------
\ Barrel Shifter

: _<<  (  num type -- )
	dspp-barrel-type !
	dspp-barrel-num !
	0 dspp-barrel-dir !
;

: _>>  (  num type -- )
	dspp-barrel-type !
	dspp-barrel-num !
	1 dspp-barrel-dir !
;

: _>>>  (  num type -- )
	dspp-barrel-type !
	dspp-barrel-num !
	2 dspp-barrel-dir !
;

: _<<' ( num -- ) _' _<< ;
: _>>' ( num -- ) _' _>> ;
: _>>>' ( num -- ) _' _>>> ;

: _CLIP ( -- ) 16 _<<' ;

: dspp.barrel.nd>code { num bsdir -- bscode }
	num 6 <
	if
		num
	else
		num 8 =
		if
			6
		else
			num 16 =
			if
				7
			else
				." Illegal Barrel shift amount = " num . cr
				abort
			then
		then
	then
	bsdir 0> \ are we right shifting
	if
		negate
	then
	$ 0F and
;

: dspp.barrel.flush ( -- )
\ check the barrel shifter
	dspp-barrel-type @
	if
		dspp-barrel-type @
		case
		_' of endof \ instants already compiled
		_# of
			dspp-barrel-num @ dspp-barrel-dir @ dspp.barrel.nd>code
			$ 0010 or
			_# dspp.operand,
		endof
		dspp-barrel-num @ dspp-barrel-type @ dspp.operand,
		endcase
	then
	dspp-barrel-num off
	dspp-barrel-type off
	dspp-barrel-dir off
;

\  -------------------------------------------------------

: dspp.make.opcode { typeA typeB typeC ifmult alu_code -- }
dspp-debug @ if ." dspp.make.opcode : "  cr then
	alu_code 0 16 within not abort" Illegal ALU code."
	alu_code 4 lshift dspp-opcode !
	ifmult
	if
		typeA TYPE_F_ACCUME = not
		typeB TYPE_F_ACCUME = not and  \ are neither inputs the ACCUME?
		if
			$ 1000 or.op  \ set multiplier select flag
		then
	then
\
	0
	typeA dspp.another.op?
	typeB dspp.another.op?
	typeC dspp.another.op?
	_=-type @ dspp.another.op?
	3 and 13 lshift or.op
\
	typeA typeB typeC ifmult dspp.calc.mux or.op
\
\ check the barrel shifter
	dspp-barrel-type @ ?dup
	if
		_' =  \ compile instant
		if
			dspp-barrel-num @ dspp-barrel-dir @ dspp.barrel.nd>code
			or.op
		else
\ use operand
			8 or.op
		then
	then

	dspp-opcode @ dspp.opcode,
;

: dspp.=.flush ( -- , flush op= )
dspp-debug @ if ." dspp.=.flush ( -- ) _=-type = " _=-type @ . cr then
	_=-type @
	if
		_=-num @ _=-type @ dspp.operand,
	then
dspp-debug @ if ." dspp.=.flush finished." then
;

: dspp.end.op ( -- , end compilation of opcode and operands )
dspp-debug @ if ." dspp.end.op" cr then
	dspp.barrel.flush
	dspp.=.flush
	dspp.reg.flush
	dspp.reset.op
;

: dspp.alu.op1A  { numA typeA alu_code -- , make alu opcode }
dspp-debug @ if ." dspp.alu.op1A : " numA .hex typeA .hex alu_code .hex cr then
	depth >r
	0 typeA 0 false alu_code dspp.make.opcode
\
	numA typeA dspp.operand,
	dspp.end.op
	depth r> - abort" Stack error in dspp.alu.op1A"
;
: dspp.alu.op1B  { numB typeB alu_code -- , make alu opcode }
	depth >r
	0 0 typeB false alu_code dspp.make.opcode  \ use ALUB
\
	numB typeB dspp.operand,
	dspp.end.op
	depth r> - abort" Stack error in dspp.alu.op1B"
;


: dspp.alu.op2  { numA typeA numB typeB alu_code -- , make alu opcode }
\ ." dspp.alu.op2: " numA .hex typeA .hex numB .hex typeB .hex alu_code .hex cr
	depth >r
	0 typeA typeB false alu_code dspp.make.opcode
\
	numA typeA dspp.operand,
	numB typeB dspp.operand,
	dspp.end.op
	depth r> - abort" Stack error in dspp.alu.op2"
;

: dspp.alu.op2*  { numA typeA numB typeB alu_code -- , make alu opcode }
	depth >r
	typeA typeB 0 true alu_code dspp.make.opcode
\
	numA typeA dspp.operand,
	numB typeB dspp.operand,
	dspp.end.op
	depth r> - abort" Stack error in dspp.alu.op2"
;
: dspp.alu.op2B*  { numA typeA numB typeB alu_code -- , make alu opcode }
	depth >r
	typeA 0 typeB true alu_code dspp.make.opcode  \ use ALUB
\
	numA typeA dspp.operand,
	numB typeB dspp.operand,
	dspp.end.op
	depth r> - abort" Stack error in dspp.alu.op2B*"
;
: dspp.alu.op3  { numA typeA numB typeB numC typeC alu_code -- , make alu opcode }
	depth >r
	typeA typeB typeC true alu_code dspp.make.opcode
\
	numA typeA dspp.operand,
	numB typeB dspp.operand,
	numC typeC dspp.operand,
	dspp.end.op
	depth r> - abort" Stack error in dspp.alu.op3"
;

: alu1:
	create ( alu_code -- ) ,
	does> @ dspp.alu.op1A
;
: alu1B:
	create ( alu_code -- ) ,
	does> @ dspp.alu.op1B
;
: alu2:
	create ( alu_code -- ) ,
	does> @ dspp.alu.op2
;
: alu2*:
	create ( alu_code -- ) ,
	does> @ dspp.alu.op2*
;
: alu2B*:
	create ( alu_code -- ) ,
	does> @ dspp.alu.op2B*
;
: alu3:
	create ( alu_code -- ) ,
	does> @ dspp.alu.op3
;

0 alu1: _TRA
1 alu1B: _NEG
2 alu2: _+
3 alu1: _+C
4 alu2: _-
5 alu1: _-B
6 alu1: _++
7 alu1: _--
8 alu1: _TRL
9 alu1: _NOT
10 alu2: _AND
11 alu2: _NAND
12 alu2: _OR
13 alu2: _NOR
14 alu2: _XOR
15 alu2: _XNOR

8 alu2*: _*
1 alu2B*: _*NEG
2 alu3: _*+
3 alu2*: _*+C
4 alu3: _*-
5 alu2*: _*-B
9 alu2*: _*NOT
10 alu3: _*AND
11 alu3: _*NAND
12 alu3: _*OR
13 alu3: _*NOR
14 alu3: _*XOR
15 alu3: _*XNOR

: _=A  ( -- , convenience )
	_= ACCUME _TRL
;

: _MOVE  { numA typeA numB typeB | opmask -- , make move opcode }
	depth >r
	typeA TYPE_REFERENCE_MASK and
	if
\ reference needs to be recorded
		typeA TYPE_OPERAND_MASK and
		case
			TYPE_F_ADDRESS of $ 9800 -> opmask endof
			TYPE_[A] of $ 9C00 -> opmask endof \ indirect
			dup .hex ." = Illegal destination type for _MOVE" abort
		endcase
		numA typeA dspp.link.reference \ compile linked reference
		opmask dspp.opcode,
	else
		typeA TYPE_OPERAND_MASK and
		case
			_REG of numA dspp.range.reg
				$ 9000 numA or dspp.opcode, endof
			_[REG] of numA dspp.range.reg
				$ 9010 numA or dspp.opcode, endof
			TYPE_F_ADDRESS of numA dspp.range.address
				$ 9800 numA or dspp.opcode, endof
			TYPE_[A] of numA dspp.range.address
				 $ 9C00 numA or dspp.opcode, endof
			dup .hex ." = Illegal destination type for _MOVE" abort
		endcase
	then
\
	numB typeB dspp.operand,
	dspp.reg.flush
	dspp.reset.op

	depth r> - abort" Stack error in _MOVE"
;

\ special instructions
: _NOP	$ 8000 dspp.opcode, ;
: _BAC	$ 8080 dspp.opcode, ;
: _RTS	$ 8200 dspp.opcode, ;
: _SLEEP $ 8380 dspp.opcode, ;
: _OP_MASK ( xx -- )
	$ 8280 or dspp.opcode,
;

: rbase.add.ref.node { rbase_addr | rbnd -- }
\	rbase_addr drs_FirstRef + @ 0< not abort" ERROR - Obsolete use of FirstRef for _RBASE"
\ allocate node for tracking references and add to list for RBASE
	rbase_size allocate abort" ERROR - insufficient host memory!"
	-> rbnd
\ ." rbase.add.ref.node - rbnd = " rbnd .hex cr
	eni-org @ rbnd rbnd_RefAddress + !
\ ." rbase.add.ref.node - ref at  " eni-org @  .hex cr
	rbnd  rbase_addr rbase_RefNodeList + sll.add.tail
;

: _RBASE ( rbase_addr -- )
	dspp-rbase-ref @
	if
		rbase.add.ref.node
		$ 8100 dspp.opcode,
		dspp-rbase-ref off
	else
		dup  0 32 within not abort" RBASE out of range!"
		$ 8100 or dspp.opcode,
	then
;

: _RMAP ( rb -- )
	dup 0 8 within not abort" RMAP out of range!"
	$ 8180 or dspp.opcode,
;

: _RBASE# { rbase_addr rbase_num -- , Bulldog RBASE }
\	." rbase_addr = " rbase_addr . cr
\	." rbase_num = " rbase_num . cr
	dspp-rbase-ref @
	IF
		rbase_addr rbase.add.ref.node
		dspp-rbase-ref off
		0
	ELSE
		rbase_addr 0 1024 within not abort" ERROR - RBASE address out of range!"
		rbase_num 2/ 2/ 0 4 within not abort" RBASE# out of range!"
		rbase_addr 3 and
		IF
			." RBASE address must be modulo 4!" cr abort
		THEN
		rbase_addr
	THEN
	rbase_num 2/ 2/ or
	$ 9400 or dspp.opcode,
;

: _RBASE0 ( rbase_address -- ) 0 _RBASE# ;
: _RBASE4 ( rbase_address -- ) 4 _RBASE# ;
: _RBASE8 ( rbase_address -- ) 8 _RBASE# ;
: _RBASE12 ( rbase_address -- ) 12 _RBASE# ;



\ Branches and labels

\ resolution of forward references
\ If a label is forward referenced, the negative dspp address-1 is stored
\ in the labels cell.  This address points to a linked list of addresses
\ to be resolved.  When the label is declared, the list is traversed,
\ the forward references resolved, and the labels positive address
\ stored in the array.
\ The only difference between local and external labels is that locals
\ are cleared when an external label is defined.

\ New label scheme implemented 950803
\ All label relocations now stored as DRLC_LIST type.
\ dspp-label-addrs = array of label addresses in code space, set to -1 if undefined
\ dspp-branch-heads = head of linked list of branches in code space, set to -1 if undefined

: dspp.init.labels  ( -- , fill tables )
	DSPP_MAX_LABELS 0
	do
		DSPP_UNRESOLVED_LABEL i dspp-label-addrs !
		0 i dspp-label-flags c!
		-1 i dspp-branch-heads !
		0 i dspp-symbols !
		0 dspp-num-labels !
	loop
;

: dspp.resolve.branch   { ref  labelAddr   -- }
	dspp-echo @ if ." resolve " ref .hex ." to " labelAddr .hex then
	ref eni@ $ FC00 and
	labelAddr or
	ref eni!  \ fix address in branch
;

: dspp.resolve.branches ( labelAddr branchHead  -- , scan linked list )
	swap ['] dspp.resolve.branch dspp.scan.list.eni drop
;


: dspp.branch { label# opcode | ref ena -- }
	label# dspp-branch-heads @ -> ref
	dspp-echo @
	IF ." dspp.branch - label# = $" label# .hex
		." , ref = $" ref  .hex cr
	THEN
	eni-org @  -> ena
	opcode dspp.opcode,
	ref 0<
	if
		ena label# dspp-branch-heads ! \ set head of linked list
	else
		ref ena dspp.add.node.eni
	then
;

: (_LABEL)  { label# | targ -- , define a label }
	label# dspp-label-addrs @ -> targ
	targ DSPP_UNRESOLVED_LABEL = not
	IF
		." Label already resolved = " label# . cr
		abort
	THEN
	eni-org @ label# dspp-label-addrs !
;

: print.label# ( label# -- )
	dspp-symbols dup c@ 0>
	IF
		count type
	ELSE
		drop
	THEN
;

: dspp.resolve.labels ( -- , resolve all branch lists to actual branches )
	dspp-num-labels @ 0
	?DO
		i dspp-label-addrs @ dup DSPP_UNRESOLVED_LABEL =
		IF
			." Unresolved label = " i print.label# cr
			abort
		THEN
		i dspp-branch-heads @ dspp.resolve.branches
	LOOP
;

: dspp.finish.locals { | labelAddr -- , make sure all locals have labels, smudge names }
	dspp-num-labels @ 0
	?DO
		i dspp-label-addrs @ -> labelAddr
		i is.label.local?
		IF
			labelAddr DSPP_UNRESOLVED_LABEL =
			IF
				." Unresolved label = " i print.label# cr
				abort
			THEN
			DSPP_ILLEGAL_LABEL_CHAR i dspp-symbols 1+ c!
		THEN
	LOOP
;

: _ENDEXT  ( -- , end external routine )
	dspp.finish.locals
;

: _JUMP ( label# -- ) $ 8400 dspp.branch ;
: _JSR ( label# -- ) $ 8400 dspp.branch ;
: _BVS ( label# -- ) $ A400 dspp.branch ;
: _BMI ( label# -- ) $ A800 dspp.branch ;
: _BRB ( label# -- ) $ AC00 dspp.branch ;
: _BEQ ( label# -- ) $ B400 dspp.branch ;
: _BCS ( label# -- ) $ B800 dspp.branch ;
: _BHS ( label# -- ) $ B800 dspp.branch ;
: _BOP ( label# -- ) $ BC00 dspp.branch ;
: _BVC ( label# -- ) $ C400 dspp.branch ;
: _BPL ( label# -- ) $ C800 dspp.branch ;
: _BRP ( label# -- ) $ CC00 dspp.branch ;
: _BNE ( label# -- ) $ D400 dspp.branch ;
: _BCC ( label# -- ) $ D800 dspp.branch ;
: _BLO ( label# -- ) $ D800 dspp.branch ;
: _B?? ( label# -- ) $ DC00 dspp.branch ;
: _BLT ( label# -- ) $ E000 dspp.branch ;
: _BLE ( label# -- ) $ E400 dspp.branch ;
: _BGE ( label# -- ) $ E800 dspp.branch ;
: _BGT ( label# -- ) $ EC00 dspp.branch ;
: _BHI ( label# -- ) $ F000 dspp.branch ;
: _BLS ( label# -- ) $ F400 dspp.branch ;
: _BXS ( label# -- ) $ F800 dspp.branch ;
: _BXC ( label# -- ) $ FC00 dspp.branch ;
: _BAZ ( label# -- ) $ A000 dspp.branch ;
: _BNZ ( label# -- ) $ B000 dspp.branch ;

\ Label support -----------------------------------------------
: $dspp.find.label { $label | label# result -- label# true | $label false }
	false -> result
	dspp-num-labels @ 0
	?do
		i dspp-symbols $label $=
		if
			i -> label#
			true -> result
			leave
		then
	loop
	result
	if
		label# true
	else
		$label false
	then
;


: $dspp.add.label  { $label | label#  -- label# true | $label false }
	dspp-num-labels @  DSPP_MAX_LABELS >
	IF
		$label false
	ELSE
		dspp-num-labels @ -> label#
		1 dspp-num-labels +!
		$label label# dspp-symbols $move
		label# true
	THEN
;

: $LABEL>#  { $label | label# ifnew? -- label# ifnew? }
	$label $dspp.find.label
	if
\ ." Label found!" dup .hex cr
		-> label#
		FALSE -> ifnew?
	else
\ ." Label NOT found! " dup $type cr
		$dspp.add.label
		if
			-> label#
		else
			count type ." - Symbol table full!" abort
		then
		TRUE -> ifnew?
	then
	label# ifnew?
;

: $dspp.branch { opcode $label -- , branch to named label }
\	opcode .hex ." branch to " $label count type cr
	$label $label># drop
	opcode dspp.branch
;

: $_LABEL { $label | label# ifnew? -- , define label }
	$label $label># -> ifnew?  -> label#
	ifnew? not
	if
		label# dspp-label-addrs @ 0>
		if
			$label count type ."  - label already defined!" abort
		then
	then
\
\ is it local?
	$label 1+ c@ [char] @ =
	if
		dspp-if-export @ abort" Cannot export local @label!"
		label# (_LABEL)
	else
		_ENDEXT
		label#  (_LABEL)
		dspp-if-export @
		if
\ mark label as exported so we can create resource after code resource created
			LABEL_F_EXPORTED label# dspp-label-flags c@ or
			label# dspp-label-flags c!
		then
	then
	dspp-if-export off
;


: dspp.branch: ( opcode )
	state @
	if  ( label# -- )
		[compile] literal
		[compile] ""
		compile $dspp.branch
	else
		[compile] "" $dspp.branch
	then
; immediate

: _LABEL: ( <name> -- )
	state @
	if
		[compile] ""
		compile $_LABEL
	else
		[compile] "" $_LABEL
	then
; immediate


: _JUMP: ( <label> -- ) $ 8400 [compile] dspp.branch: ; immediate
: _JSR: ( <label> -- ) $ 8800 [compile] dspp.branch: ; immediate
: _BVS: ( <label> -- ) $ A400 [compile] dspp.branch: ; immediate
: _BMI: ( <label> -- ) $ A800 [compile] dspp.branch: ; immediate
: _BRB: ( <label> -- ) $ AC00 [compile] dspp.branch: ; immediate
: _BEQ: ( <label> -- ) $ B400 [compile] dspp.branch: ; immediate
: _BCS: ( <label> -- ) $ B800 [compile] dspp.branch: ; immediate
: _BHS: ( <label> -- ) $ B800 [compile] dspp.branch: ; immediate
: _BOP: ( <label> -- ) $ BC00 [compile] dspp.branch: ; immediate
: _BVC: ( <label> -- ) $ C400 [compile] dspp.branch: ; immediate
: _BPL: ( <label> -- ) $ C800 [compile] dspp.branch: ; immediate
: _BRP: ( <label> -- ) $ CC00 [compile] dspp.branch: ; immediate
: _BNE: ( <label> -- ) $ D400 [compile] dspp.branch: ; immediate
: _BCC: ( <label> -- ) $ D800 [compile] dspp.branch: ; immediate
: _BLO: ( <label> -- ) $ D800 [compile] dspp.branch: ; immediate
: _B??: ( <label> -- ) $ DC00 [compile] dspp.branch: ; immediate
: _BLT: ( <label> -- ) $ E000 [compile] dspp.branch: ; immediate
: _BLE: ( <label> -- ) $ E400 [compile] dspp.branch: ; immediate
: _BGE: ( <label> -- ) $ E800 [compile] dspp.branch: ; immediate
: _BGT: ( <label> -- ) $ EC00 [compile] dspp.branch: ; immediate
: _BHI: ( <label> -- ) $ F000 [compile] dspp.branch: ; immediate
: _BLS: ( <label> -- ) $ F400 [compile] dspp.branch: ; immediate
: _BXS: ( <label> -- ) $ F800 [compile] dspp.branch: ; immediate
: _BXC: ( <label> -- ) $ FC00 [compile] dspp.branch: ; immediate
\ new red only
: _BAZ: ( <label> -- ) $ A000 [compile] dspp.branch: ; immediate
: _BNZ: ( <label> -- ) $ B000 [compile] dspp.branch: ; immediate

: dspp.paint.en  ( -- , fill with negative ones for unused codes )
	EN-SIZE @ 0
	do -1 i (eni!)
	loop
;
: dspp{ ( -- , reset dspp assembler )
	0 _ORG
	dspp.reset.op
	dspp.paint.en
	dspp.init.labels
;

: }dspp
	dspp.resolve.labels
;

dspp{

: dspp.dump.symbols
	>newline
	base @ >r hex
	dspp-num-labels @ 0
	?do
		i .
		i print.label#
		i dspp-label-addrs @ .hex
		i dspp-branch-heads @ count .hex cr
	loop
	r> base !
;

: dspp.dump.emu
	>newline
	base @ >r hex
	EN-SIZE @ 0
	do
		i eni@ -1 = not
		if i dspp.dump.en
		then
	loop
	r> base !
;

: dspp.dump.c  ( -- dump as int16 'C' data )
	>newline
	base @ >r hex
	EN-SIZE @ 0
	do
		i eni@ -1 = not
		if
			i eni@ p"   0x" count log.type 4 .n bl log.emit
			i p" , /* 0x" count log.type 4 .n
			p"  */" count log.type log.cr
		then
	loop
	r> base !
;

: dspp.dump.c.long  ( -- dump as int32 'C' data )
	>newline
	base @ >r hex
	EN-SIZE @ 0
	do
		i 2* eni@ -1 = not
		i 2* 1+ eni@ -1 = not or
		if
			p"   0x" count log.type
			i 2* eni@ 4 .n
			i 2* 1+ eni@ 4 .n
			bl log.emit
			p" , /* 0x" count log.type
			i 2* 4 .n
			p"  */" count log.type log.cr
		then
	loop
	r> base !
;

: dspp.dump.nmem ( -- , dump code to data file )
	>newline
	base @ >r hex
	EN-SIZE @ 0
	do
		i eni@ -1 =
		IF
			$ 8000 \ stick NOP where no code specified, 941201
		ELSE
			i eni@
		THEN
		4 .n log.cr
	loop
	r> base !
;

: dspp.checksum ( -- checksum )
	0
	EN-SIZE @ 0
	do
		i eni@ -1 = not
		if i eni@ +
		then
	loop
;

: dspp.dump
	dspp.dump.emu
	log.cr
	dspp.dump.symbols
	log.cr
	p" # checksum = " count log.type
	dspp.checksum 8 .n log.cr
;

\ --------------------------------------------------
: test.dspp.asm
	dspp{

_LABEL: OscTest
	R4 _= R0 R6 _*
	R0 R6 R7 _*-		\ R0*R6-R7
	R7 ACCUME _+
	%R0 R6 R7 _*-		\ @R0*R6-R7
	[R0] R6 R7 _*-		\ [R0]*R6-R7
	R7 R8 _+			\ R7+R8
	R7 R8 _-			\ R7-R8
	A$ 11 R0 R1 _*+	\ $11*R0+R1
	R0 A$ 11 R1 _*+	\ R0*$11+R1
	R0 R1 A$ 11 _*+	\ R0*R1+$11
_ENDEXT

_LABEL:  BigTest
	R7 R8 _+
	_NOP
	_JSR:  OscTest
	_BLE:  @Fooper
	R7 R8 _-			\ R7-R8
_LABEL:   @Fooper
	_NOP
_ENDEXT
	}dspp

	dspp.dump
;


