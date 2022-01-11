\ @(#) patch_maker.fth 95/10/23 1.1
\
\ Binary Patch File builder
\
\ Author: Phil Burk
\ Copyright 1995 3DO
\ All Rights Reserved

anew task-patch_maker.fth

\ General purpose packed name tool ------------------------------
\ Appends byte data to data area
:struct PackedNames
    aptr  pnam_BaseAddress   \ pointer to memory to hold data
    long  pnam_CharIndex     \ index of next byte location to be filled
    long  pnam_MaxChars      \ maximum chars that will fit
    long  pnam_NameIndex     \ count names added
;struct

: pnam.init  { addr size pnam -- }
	addr   pnam s! pnam_BaseAddress
	0      pnam s! pnam_CharIndex
	size   pnam s! pnam_MaxChars
	0      pnam s! pnam_NameIndex
;

: pnam.get.data { pnam -- addr cnt }
	pnam s@ pnam_BaseAddress
	pnam s@ pnam_CharIndex
;

: pnam.add.data { addr cnt pnam |  name_addr --  }
\ is there room for this
	pnam s@ pnam_CharIndex  cnt +
	pnam s@ pnam_MaxChars > abort" pnam.add.name -- too many names!"
	
\ copy chars
	pnam pnam.get.data  + -> name_addr
	addr  name_addr cnt move
	
\ advance character index
	pnam s@ pnam_CharIndex  cnt +
	pnam s! pnam_CharIndex
	
\ advance name index
	pnam s@ pnam_NameIndex 1+
	pnam s! pnam_NameIndex
;

create scratch-name 128 allot

: pnam.add.name { $name pnam | addr cnt name_addr --  }
	$name count -> cnt -> addr
	
\ move data to scratch area so we can append NUL
	addr scratch-name cnt move
\ NUL terminate
	0 scratch-name cnt + c!

\ add to array
	scratch-name  cnt 1+  pnam  pnam.add.data
;

: pnam.dump { pnam -- }
	pnam pnam.get.data  dump
;


\ Build two instances of PackedNames.
\ One for templates and one for port names.

500 constant PM_NAMES_SIZE
create pm-names-data PM_NAMES_SIZE allot

500 constant PM_TEMPLATES_SIZE
create pm-templates-data PM_TEMPLATES_SIZE allot

PackedNames  pm-names
PackedNames  pm-templates

\ templates and names can be created that return their index
: pnam.create.name ( $name pnam <name> -- )
	pnam.add.name constant
;
: pm.name: ( $name <name> -- )
	pm-names s@  pnam_CharIndex >r  \ save CHAR index before adding name
	pm-names pnam.add.name
	r> 1+ \ add 1 so that zero can be used to represent SELF
	constant
;
: pm.template: ( $name <name> -- )
	pm-templates s@  pnam_NameIndex >r  \ save NAME index before adding name
	pm-templates pnam.add.name
	r> constant
;

\ make an area to accumulate patch commands
7 constant PCMD_NUM_LONGS
PCMD_NUM_LONGS 4 * constant PCMD_SIZE
50 constant PCMD_MAX_COMMANDS
PCMD_MAX_COMMANDS PCMD_SIZE * constant PM_COMMANDS_SIZE

create pm-commands-data PM_COMMANDS_SIZE allot
PackedNames pm-commands

256
enum: PATCH_CMD_ADD_TEMPLATE
enum: PATCH_CMD_DEFINE_PORT
enum: PATCH_CMD_DEFINE_KNOB
enum: PATCH_CMD_EXPOSE
enum: PATCH_CMD_CONNECT
enum: PATCH_CMD_SET_CONSTANT
drop

0 constant PATCH_CMD_END   \ TAG_END
$ FFFFFFFE constant PATCH_CMD_JUMP  \ TAG_JUMP
$ FFFFFFFF constant PATCH_CMD_NOP   \ TAG_NOP

0 constant PATCH_BLOCK_SELF

variable pm-cell-pad

: pm.cell>commands  ( cell -- )
	pm-cell-pad !
	pm-cell-pad 4 pm-commands pnam.add.data
;

: pm.zeros>commands ( num_zeros -- )
	0
	DO
		0 pm.cell>commands
	LOOP
;

\ IEEE32 format is S-EEEEEEEE-FFFFFFFFFFFFFFFFFFFFFFF
fvariable FP-IEEE-TEMP

: FLOG2 ( r -- flog2[r] )
	fln 2.0 fln f/
;

: float>ieee32 { | exp sign fptemp -- , fpnum -f- }
	fdup f0<
	IF
		1 -> sign
		fnegate FP-IEEE-TEMP f!
	ELSE
		FP-IEEE-TEMP f!
	THEN
\ calculate exponent
	FP-IEEE-TEMP f@ flog2 floor f>s
	dup 127 + -> exp
\ exp . cr
\
\ calculate fraction between 1.0 and 2.0
	s>f   \ truncated log base 2
	2.0 fswap f**    \ fdup f. cr
	FP-IEEE-TEMP f@ fswap f/   \ ." frac = " fdup f. cr   \ fraction 
	$ 00800000 s>f f*    \ fdup f. cr
	f>s $ 007FFFFF and    \ dup .hex cr
\
\ merge with exp and sign
	exp 23 lshift or
	sign 31 lshift or
;


\ patch commands
: pc.add.template { block_index tmpl_index -- }
	PATCH_CMD_ADD_TEMPLATE  pm.cell>commands
	block_index             pm.cell>commands
	tmpl_index              pm.cell>commands
	4   pm.zeros>commands
;

: pc.define.port { name_index num_parts port_type signal_type -- }
	PATCH_CMD_DEFINE_PORT   pm.cell>commands
	name_index              pm.cell>commands
	num_parts               pm.cell>commands
	port_type               pm.cell>commands
	signal_type             pm.cell>commands
	2   pm.zeros>commands
;


: pc.define.knob { name_index num_parts knob_type  -- , default_val -f- }
	PATCH_CMD_DEFINE_KNOB   pm.cell>commands
	name_index              pm.cell>commands
	num_parts               pm.cell>commands
	knob_type               pm.cell>commands
	float>ieee32            pm.cell>commands
	2   pm.zeros>commands
;

: pc.set.constant { name_index port_index part_num  -- , default_val -f- }
	PATCH_CMD_SET_CONSTANT  pm.cell>commands
	name_index              pm.cell>commands
	port_index              pm.cell>commands
	part_num                pm.cell>commands
	float>ieee32            pm.cell>commands
	2   pm.zeros>commands
;
: pc.expose.port { port_name_index src_block_name_index src_port_name_index -- }
	PATCH_CMD_EXPOSE        pm.cell>commands
	port_name_index         pm.cell>commands
	src_block_name_index    pm.cell>commands
	src_port_name_index     pm.cell>commands
	3   pm.zeros>commands
;

: pc.connect { from_blk from_port from_part_num to_blk to_port to_part_num -- }
	PATCH_CMD_CONNECT       pm.cell>commands
	from_blk                pm.cell>commands
	from_port               pm.cell>commands
	from_part_num           pm.cell>commands
	to_blk                  pm.cell>commands
	to_port                 pm.cell>commands
	to_part_num             pm.cell>commands
;

: pc.end.list ( -- , terminate command list )
	PATCH_CMD_END       pm.cell>commands
	6   pm.zeros>commands
;

: pm.names.init
	pm-names-data  PM_NAMES_SIZE  pm-names pnam.init
	pm-templates-data  PM_TEMPLATES_SIZE  pm-templates pnam.init
	pm-commands-data  PM_COMMANDS_SIZE  pm-commands pnam.init
;

: pm.dump
	pm-names pnam.dump
	pm-templates pnam.dump
	pm-commands pnam.dump
;


: pm.patch>file { | pos_3pch pos_pcmd -- }
	iff-filename @ 0=
	if
		c" output.patch"
	else
		iff-filename @
	then
	$iff.open
	c" 3PCH" iff.begin.form -> pos_3pch
		c" PCMD" iff.begin.form -> pos_pcmd
			c" PTMP" pm-templates pnam.get.data chunk>ofx
			c" PNAM" pm-names pnam.get.data chunk>ofx
			c" PCMD" pm-commands pnam.get.data chunk>ofx
		pos_pcmd iff.end.form
	pos_3pch iff.end.form
	iff.close
	iff-filename off
;


: patch{
	pm.names.init
;
: }patch
	pc.end.list
	pm.patch>file
;
