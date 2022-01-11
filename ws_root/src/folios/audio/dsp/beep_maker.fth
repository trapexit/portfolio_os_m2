\ @(#) beep_maker.fth 96/10/16 1.7
\ Beep Machine Builder
\
\ 961016 PLB Use LWORD so that case of BM.NAME: is preserved.
\
\ Author: Phil Burk
\ Copyright 1996 3DO

decimal
include? dspp{ dspp_asm.fth
include? DSPI_CLOCK dspp_addresses.j

anew task-beep_maker.fth

128 constant BM_MAX_VOICES

variable BM-VOICE-INDEX
variable BM-NEXT-ADDR
bm_max_voices 2* warray BM-VOICE-OFFSETS

: QUAD.UP  ( n -- n+? , quad aligned )
	3 + 3 invert and
;

: BM.ALLOC.VOICE  { size -- , allocate beep voice }
	bm-next-addr @ quad.up  \ next address for voice
	bm-voice-index @
	dup bm_max_voices >= abort" Too many beep voices!"
	bm-voice-offsets w!      \ save address of voice in array
	1 bm-voice-index +!
	size bm-next-addr +!
;

: BM.VOICE>ADDR ( v# -- addr )
	bm-voice-offsets w@
;

$ BEEB1234  constant BM_FILE_PAIR

: BEEP.FILE{    ( -- formpos BM_FILE_PAIR , begin BEEP file)
	iff-filename @ 0=
	if
		c" output.bm"
	else
		iff-filename @
	then
	$iff.open
	c" BEEP" iff.begin.form ( -- formpos )
	BM_FILE_PAIR
	0 iff-filename !
;

: }BEEP.FILE ( formpos BM_FILE_PAIR -- )
	BM_FILE_PAIR - abort" }ins.file - Stack error in assembly!"

	( formpos -- ) iff.end.form
	iff.close
;

\  FIFO register offsets
0 constant FIFO_CURRENT
1 constant FIFO_NEXT
2 constant FIFO_FREQ
3 constant FIFO_PHASE
4 constant FIFO_BUMP
5 constant FIFO_STATUS

:  DSPI.CHAN>FIFO ( DMAChan -- Address , DSPI FIFO registers)
	DSPI_FIFO_OSC_SIZE * DSPI_FIFO_OSC +
;

\ Allocate Global Variables used by all Beep Machines
0
dup 1+ swap constant gAmplitude
dup 1+ swap constant gTicksUsed   \ reports ticks used in batch
dup 1+ swap constant gFrameCount  \ advance once per frame
dup 1+ swap constant gMixLeft
dup 1+ swap constant gMixRight
dup 1+ swap constant gBenchStart  \ record clock at start
dup 1+ swap constant gTickAccum   \ accumulate ticks spent waiting
dup 1+ swap constant gSaveClock   \ save clock for ticks waiting calc
dup 1+ swap constant gScratch     \ temporary usage
dup 1+ swap constant gMisc1       \ for miscellaneous machine use
dup 1+ swap constant gMisc2
dup 1+ swap constant gMisc3
dup 1+ swap constant gFuture4
dup 1+ swap constant gFuture3
dup 1+ swap constant gFuture2
dup 1+ swap constant gFuture1
quad.up constant BM_FIRST_ADDR

\ 4 - Registered Beep Machine Identifier (unique for each machine)
\ 4 - Silicon Version Expected
\ 4 - NumChannelsAssigned - to voices
\ 4 - NumVoices
:STRUCT BeepMachineInfo
	long bminfo_ID
	long bminfo_SiliconVersion
	long bminfo_NumChannelsAssigned
	long bminfo_NumVoices
;STRUCT

:STRUCT BeepMachineInit
	long      bmin_ParamID
	byte      bmin_FirstVoice  \ First voice to be set.
	byte      bmin_NumVoices   \ Number of voices to be set.
	short     bmin_Value       \ Set DSPP to this raw integer value.
;STRUCT

BeepMachineInfo bm-info  \ static structure for beep machines
variable bm-init-ptr
variable bm-init-num

: CODE>BEEP ( -- , write code chunk )
\ pack code on pad before writing
	eni-org @ 0
	do
		i eni@          \ get code
		i 2* pad + w!   \ write to pad
	loop
\
	c" CODE" pad eni-org @ 2* chunk>ofx
	eni-org @ . ." code words used in beep machine." cr
;

: INIT>BEEP ( -- , write init chunk )
	bm-init-ptr @ 0>
	IF
		c" INIT" bm-init-ptr @
		bm-init-num @ sizeof() BeepMachineInit *
		chunk>ofx
	THEN
;

: BM.INIT, { paramID first num val -- }
	paramID here s! bmin_ParamID
	first here s! bmin_FirstVoice
	num here s! bmin_NumVoices
	val here s! bmin_Value
	sizeof() BeepMachineInit allot
	1 bm-init-num +!
;

: ALLOC.VAR ( <name> -- )
	bm-next-addr @
	dup constant
	1+ bm-next-addr !
;

\ -----------------------------
: ilog2 ( n -- ilog2 , 8 => 3,  16 => 4, etc.)
	0 swap
	BEGIN
		2/ dup 0>
	WHILE
		swap 1+ swap
	REPEAT
	drop
;
	
\ Support for writing include files for Beep Machine
: BM.MAKE.PARAM.ID  { offset sig_type rate_divide -- packed_id }
	$ B   28 lshift
	sig_type    24 lshift OR
	rate_divide  ilog2  22 lshift OR
	bm-info s@ bminfo_ID    16  lshift OR
	offset      OR
;

: (.hex8)  ( n -- addr len )
	base @ >r hex
	0 <# # # # # # # # # #>
	r> base !
;

: BM.NAME:  ( <name> -- )
	c" #define BEEP_MACHINE_NAME " count log.type
	[char] " log.emit
	bl lword      count log.type  \ 961016  LWORD preserves case
	[char] " log.emit
	log.cr
;

: BM.DEFINE: { num | outcnt -- , write to log file }
	c" #define " count
		dup -> outcnt
		log.type
	num constant
	latest count $ 1F and
		dup outcnt + -> outcnt
		log.type
	40 outcnt - 1 max 0 do BL log.emit loop
	c" (0x"      count log.type
	num (.hex8)            log.type
	[char] )           log.emit
	log.cr
;

: BM.PARAM: ( offset sig_type rate_divide <name> -- , write to log file )
	bm.make.param.id
	bm.define:
;

\ -----------------------------
: WRITE.BEEP.FILE
	beep.file{
		c" INFO" bm-info sizeof() BeepMachineInfo chunk>ofx
		c" VCDO" 0 bm-voice-offsets
			bm-info s@ bminfo_NumVoices 1+ 2* chunk>ofx
		code>beep
		init>beep
	}beep.file
;

\ Register all Beep Machines
0
dup 1+ swap constant BMID_RESERVED
dup 1+ swap constant BMID_BASIC
dup 1+ swap constant BMID_MIXED
drop


: BEEP.INIT
	0 bm-voice-index !
	bm_first_addr bm-next-addr !
	0 bm-init-ptr !
	0 bm-init-num !
;

: BEEP{
	dspp{
	beep.init
;
: }BEEP
	}dspp
	write.beep.file
;

