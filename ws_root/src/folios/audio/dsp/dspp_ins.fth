\ @(#) dspp_ins.fth 96/07/16 1.4
\ Build *.DSP instrument files
\
\ by Phil Burk
\ Copyright 1992,93,94 3DO
\ All rights reserved.
\ Proprietary and confidential.
\
\ 960520 PLB Fix CR6010.  Don't INIT resource if IMPORTed.
\ 960711 PLB Put Ticks and Entry resources first.

include? dspp{  dspp_asm.fth

anew task-dspp_ins.fth

0 constant SUPPORT_SAMP_ENV \ FIXME - remove sample and envelope code

\ linked list headers for OFX instrument definition structures
variable dins-rsrc-list  \ list of all resources
variable dins-dini-list  \ list of resource initialisers
variable ofx-ticks-needed \ just used for reporting

variable ofx-file-id  \ file id
variable ofx-pad
variable ins-name       \ instrument name for NAME chunk
variable ins-form-pos   \ position in primary 3INS form

\ used to write a custom optional chunk
variable ofx-custom-name
variable ofx-custom-addr
variable ofx-custom-size

SUPPORT_SAMP_ENV [if]
20 constant MAX_ATTACHED_SAMPLES
MAX_ATTACHED_SAMPLES array fifo-names
MAX_ATTACHED_SAMPLES array sample-names
MAX_ATTACHED_SAMPLES array sample-flags
1 constant EMBED_ATTACHED_SAMPLE
variable num-samples    \ count of samples attached

: attach.sample.flags { $sampname $fifoname flags -- }
	$fifoname num-samples @ fifo-names !
	$sampname num-samples @ sample-names !
	flags num-samples @ sample-flags !
	num-samples @ 1+
	dup MAX_ATTACHED_SAMPLES >=
	abort" Exceeded maximum number of attached samples!"
	num-samples !
;

: attach.sample.xref ( $sampname $fifoname -- , default is use XREF )
	0 attach.sample.flags
;

: attach.sample ( $sampname $fifoname -- , default is use XREF, for compatibility )
	attach.sample.xref
;

: attach.sample.embed ( $sampname $fifoname -- , embed the sample in the file )
	EMBED_ATTACHED_SAMPLE attach.sample.flags
;

\ ----------------------------------------------------------------

20 constant MAX_ATTACHED_ENVELOPES
variable envelope-index    \ count of envelopes attached

\ define envelope attachment structure, model after ENVH chunk
0
LONG:  envl_NumPoints
LONG:  envl_SustainBegin
LONG:  envl_SustainEnd
LONG:  envl_SustainTime
LONG:  envl_ReleaseBegin
LONG:  envl_ReleaseEnd
LONG:  envl_ReleaseTime
LONG:  envl_MicrosPerDelta
LONG:  envl_Flags
LONG:  envl_ReleaseJump
dup constant size_envl_chunk
LONG:  envl_HookName
LONG:  envl_DataPtr
constant size_envl

: rec-array
	create ( n size -- )
		dup ,
		* allot
	does> ( n [addr] -- addr' )
		dup @ ( n addr size )
		>r swap r> * +
		4 +  ( skip over stored size )
;

MAX_ATTACHED_ENVELOPES size_envl rec-array env-attachments

: bump.envelope.index ( -- env-index )
	envelope-index @ 1+
	dup MAX_ATTACHED_ENVELOPES >=
	abort" Exceeded maximum number of attached envelopes!"
	dup envelope-index !
;

: set.envelope.sustain { SustainBegin SustainEnd SustainTime | envl -- }
	envelope-index @ env-attachments -> envl
	SustainBegin envl envl_SustainBegin + !
	SustainEnd envl envl_SustainEnd + !
	SustainTime envl envl_SustainTime + !
;

: set.envelope.release { ReleaseBegin ReleaseEnd ReleaseTime | envl -- }
	envelope-index @ env-attachments -> envl
	ReleaseBegin envl envl_ReleaseBegin + !
	ReleaseEnd envl envl_ReleaseEnd + !
	ReleaseTime envl envl_ReleaseTime + !
;
: set.envelope.jump ( ReleaseJump -- )
	envelope-index @ env-attachments
	envl_ReleaseJump + !
;

: new.envelope {  EnvData NumPoints $hookname flags | envl -- }
	bump.envelope.index
	$hookname count type ."  = envelope #" dup . cr
	env-attachments -> envl
\	." envl = " envl . cr
	NumPoints envl envl_NumPoints + !
	-1 -1 0 set.envelope.sustain
	-1 -1 0 set.envelope.release
	1000 envl envl_MicrosPerDelta + !
	flags envl envl_Flags + !
	0 envl envl_ReleaseJump + !
	$hookname envl envl_HookName + !
	EnvData envl envl_DataPtr + !
;

: attach.envelope {  EnvData NumPoints SustainBegin SustainEnd SustainTime $hookname | envl -- }
	EnvData NumPoints $hookname 0 new.envelope
	SustainBegin SustainEnd SustainTime set.envelope.sustain
;
[then]

\ ----------------------------------------------------------------

\ build a data structure that I can append data to for building chunks
\ first long word is byte offset to data
: area  ( size <name> -- )
	create 0 , dup , allot
;
: area.bytes ( area -- nbytes ) @ ;
: area.size ( area -- nbytes ) cell+ @ ;

: area.next  { nbytes areapfa -- new-addr , and advance pointer }
	areapfa @ nbytes +
	areapfa cell+ @ < not abort" Area exceeded!"
	areapfa @ 8 + areapfa +
	nbytes areapfa +!
;

: area.push.long ( datum areapfa -- )
	4 swap area.next !
;
: area.push.short ( datum areapfa -- )
	2 swap area.next w!
;
: area.push.byte ( datum areapfa -- )
	1 swap area.next c!
;
: area.push.block { addr cnt areapfa -- }
	addr cnt
	dup areapfa area.next
	swap cmove
;
: area.push.string { $addr areapfa -- }
	$addr dup c@ 1+ areapfa area.push.block
;

2000 area ofx-rsrc-area
2000 area ofx-rloc-area
2000 area ofx-rnames-area
2000 area ofx-knob-area
2000 area ofx-dini-area
variable ofx-patch-knob

\ -------------------------------------------------------------------
variable ofx-rsrc-indx   \ used to build DRSC chunks

: ins.reset.vars ( -- )
	dins-rsrc-list off
	dins-dini-list off
	ins-name off
[ SUPPORT_SAMP_ENV [if] ]
	num-samples off
	-1 envelope-index !
[ [then] ]
	dspp-num-labels off
	dspp-dynamic-links off
	ofx-custom-addr off
	0 ofx-rsrc-indx !
	0 dspp-function-id !
	0 dspp-header-flags !
	0 ofx-ticks-needed !
\ reset chunk builders
	0 ofx-rsrc-area   !
	0 ofx-dini-area   !
	0 ofx-rloc-area   !
	0 ofx-rnames-area !
	0 ofx-knob-area   !
	0 ofx-patch-knob  !
	0 _ORG
;

: ofx.push.rsrc  ( long -- , post increment )
	ofx-rsrc-area area.push.long
;

: ofx.push.rloc.byte  ( byte -- , post increment )
	ofx-rloc-area area.push.byte
;
: ofx.push.rloc.short  ( short -- , post increment )
	ofx-rloc-area area.push.short
;
: ofx.push.knob  ( long -- , post increment )
	ofx-knob-area area.push.long
;
: ofx.push.dini  ( long -- , post increment )
	ofx-dini-area area.push.long
;

\ build chunk structures
: ofx.add.rloc { part icode rndx offset -- , add to rloc pad }
	dspp-echo @
	IF
		." ofx.add.rloc: Part = $" part .hex
\		." , icode = " icode .
		." , RsrcIndex = $" rndx .hex
		." , offset in code = $" offset .hex cr
	THEN
	offset 0< abort" ofx.add.rloc - Attempt to relocate unreferenced resource! "
\ This code must match DSPPRelocation structure defined in dspp.h in audiofolio.
	rndx   ofx.push.rloc.short
	part   ofx.push.rloc.short
	0      ofx.push.rloc.byte
	icode  ofx.push.rloc.byte
	offset ofx.push.rloc.short
;

create scratch-pad 256 allot

: OFX.ADD.RSRC.NAME { $name -- , write name to name block }
	$name safecount
	dspp-echo @
	IF
		." ofx.add.rsrc.name: " 2dup type cr
	THEN
	ofx-rnames-area area.push.block \ 'C' style
	0 ofx-rnames-area area.push.byte  \ NUL terminator
;

: ofx.add.rsrc.info { rtype many offset boundto defval -- , add a resource to rsrc area }
	dspp-echo @
	if
		." OFX.ADD.RSRC.INFO:"
		."   rtype = $" rtype .hex
		." , many = " many .
		." , offset = " offset .
		." , boundto = " boundto .
		." , defval = $" defval .hex
		." , index = " ofx-rsrc-indx @ .
		cr
	then
	rtype ofx.push.rsrc
	many ofx.push.rsrc
	offset ofx.push.rsrc   \ offset from allocated resource
	boundto ofx.push.rsrc  \ what resource is this bound to
	defval ofx.push.rsrc   \ default value
\
	1 ofx-rsrc-indx +!    \ advance index
;

: ofx.add.resource ( rtype many offset boundto defval $name -- , add a resource to rsrc area )
	ofx.add.rsrc.name
	ofx.add.rsrc.info
;

: ofx.add.rsrc ( rtype many $name -- )
	>r
	0 0 0 \ offset boundto default_value
	r> ofx.add.resource
;

: ofx.add.drs.reloc.part { drs partnum | partref -- , add reloc to file }
\ ." ofx.add.drs.reloc.part: " drs drs_NamePtr + @ id. cr
	partnum drs drs.partref[] @ -> partref
\ does this part have any references?
	partref 0< not
	if
		partnum
		0  \ icode
		ofx-rsrc-indx @
		partref  ofx.add.rloc    \ add any reference relocations
	then
;

: drs.check.references { drs -- , has this resource been used, if not why not!?! }
	drs drs_ReferenceCount + @ 0=
	IF
		drs drs_NamePtr + @ count type
		DRSC_EXPORT DRSC_BIND or DRSC_BOUND_TO or
		drs drs_RsrcType + @ and
		if
			."  is unreferenced but OK because exported or bound." cr
		else
			."  is unreferenced resource and not exported or bound! - ERROR" cr
			abort
		then
	THEN
;

: ofx.add.drs.reloc { drs  -- }
	drs drs.check.references
	drs drs_NumParts + @  0
	DO
		drs i
		ofx.add.drs.reloc.part
	LOOP
\ free relocs because we are done wth them
	drs drs_PartRefs + @ free abort" Failure when freeing drs_PartRefs"
	0 drs drs_PartRefs + !
;

: ofx.add.drs.rsrc { drs -- }
	drs drs_RsrcType + @
	drs drs_Many + @
	drs drs_AllocOffset + @
	drs drs_BoundTo + @ ?dup
	IF
		drs_Index + @    \ look up index of resource it is bound to
	ELSE 0
	THEN
	drs drs_DefaultValue + @
	drs drs_NamePtr + @
	ofx.add.resource
;


: ofx.add.dini { DataPtr Many WhenFlags RsrcIndex  -- , add to rloc pad }
	dspp-echo @
	if
		."  ofx.add.dini: " DataPtr . Many . WhenFlags . RsrcIndex . cr
	then
	RsrcIndex 0< abort" ofx.add.dini : Uncompiled resource!"
	Many ofx.push.dini
	WhenFlags ofx.push.dini
	RsrcIndex ofx.push.dini
	0 ofx.push.dini \ reserved
\ lay array of initialization data into chunk
	DataPtr
	Many 0
	do
		dup @ ofx.push.dini
		cell+
	loop
	drop
;

\ --------------------------------------------------------------------
\ OFX file io
: ofx.write ( addr cnt -- )
	ofx-file-id @ dup 0= abort" OFX file not open!"
	write-file drop
;

: chunk>ofx { $id addr cnt -- }
	cnt 0>  \ only write chunk if size > 0
	IF
\ dump diagnostic
		dspp-echo @
		if
			." Now writing chunk: " $id safecount type space cnt . cr
			addr cnt dump cr
		then
\
		$id safecount ofx.write
		cnt ofx-pad ! ofx-pad 4 ofx.write  \ write chunk length
\
\ 940513 Write 0 byte to pad end of chunk
\	addr cnt even-up ofx.write  \ this wrote random data as a pad
		addr cnt ofx.write
		cnt 1 and
		if
			0 ofx-pad !
			ofx-pad 1 ofx.write
		then
	THEN
;

\ =========================== IFF =====================================
variable iff-filename
0 iff-filename !

: iff.close  ( -- )
	ofx-file-id @ ?dup
	if
		close-file drop
		ofx-file-id off
	then
;

if.forgotten iff.close

: $iff.open  ( $filename -- form-pos )
	iff.close
	dspp-echo @
	if
		." Output written to: " dup count type cr
	then
	count r/w create-file abort" Could not open OFX file for write!"
	ofx-file-id !
;

: iff.begin.form { $id -- form-pos }
	dspp-echo @
	if
		." Begin FORM=" $id count type cr
	then
	ofx-file-id @ file-position drop
	p" FORMxxxx" count ofx.write
	$id count ofx.write
;

: iff.end.form { form-pos | filesize -- }
	dspp-echo @
	if
		." End FORM" cr
	then
\ write FORM size
	ofx-file-id @ file-position drop -> filesize ( get file size )
\ ." iff.end.form : FORM from " form-pos . ." to " filesize . cr
	filesize 8 - ( subtract FORM header )
	form-pos - ( subtract start of FORM )
	ofx-pad !
\ position to FORM size
	form-pos 4 + ofx-file-id @ reposition-file drop
	ofx-pad 4 ofx.write
\ go back to end of file
	filesize ofx-file-id @ reposition-file drop
;

\ ====================================================================
\ structure of DHDR chunk
0
LONG:  dhdr_FunctionID
LONG:  dhdr_SiliconVersion
LONG:  dhdr_FormatVersion
LONG:  dhdr_Flags
constant size_dhdr_chunk

create dhdr-data size_dhdr_chunk allot

: dhdr>ofx ( -- )
	dspp-echo @
	if
		." DHDR: Silicon = " dspp-version @ .
		." , Function = " dspp-function-id @ . cr
		." , Flags = " dspp-header-flags @ .hex cr
	then
	dspp-version @ dhdr-data dhdr_SiliconVersion + !
	dspp-function-id @ dhdr-data dhdr_FunctionID + !
	DHDR_FORMAT_VERSION dhdr-data dhdr_FormatVersion + !
	dspp-header-flags @ dhdr-data dhdr_Flags + !
	p" DHDR" dhdr-data size_dhdr_chunk chunk>ofx
;

: code>ofx ( -- )
\ write subchunk entry
	DCOD_RUN_DSPP pad !   \ dcod_Type
	12 pad cell+ !        \ dcod_Offset
	eni-org @ pad cell+ cell+ !   \ dcod_Size
\
\ pack code on pad before writing
	eni-org @ 0
	do i eni@ i 2* pad + 12 + w!
	loop
\
	p" DCOD" pad eni-org @ 2* 12 + chunk>ofx
;

: dlink>ofx { | dlinks daddr dlen -- , write any dynamic links to file }
	dspp-dynamic-links @ -> dlinks
	dlinks
	IF
		dlinks count -> dlen -> daddr
		daddr pad dlen cmove
		pad dlen ascii , 0 translate.chars \ translate commas to NULs
		dspp-echo @
		IF
			." dlink>ofx " pad dlen type cr
		THEN
		0 pad dlen + c! \ NUL terminator
		c" DLNK" pad dlen 1+ chunk>ofx
	THEN
	dspp-dynamic-links off
;

: dspp.show.ref ( ref data -- )
	drop
	4 spaces dup .hex ."  = " eni@ .hex cr
;

: dspp.show.ref.list ( head -- , show list of code reference if any nodes )
	dup 0<
	if drop
	else 0 ['] dspp.show.ref  dspp.scan.list.eni drop
	then
;


: imported.labels>ofx { | labelAddr branchHead -- , write DRSC_TYPE_CODE and relocate from it }
	dspp-num-labels @ 0
	?DO
		i dspp-label-addrs @ -> labelAddr
		i dspp-branch-heads @ -> branchHead  \ has there been a branch to this label

		labelAddr DSPP_UNRESOLVED_LABEL =
		IF
			dspp-echo @
			if
				." imported.labels>ofx - " i print.label#
				." , branch at $" branchHead .hex cr
			then
			i is.label.local?
			IF
				." Locals must be resolved." cr
				abort
			THEN
\ write relocation request for all references in LIST format
			0   \ external so no code offset
			0
			ofx-rsrc-indx @  \ index of following resource
			branchHead  \ code address of first linked branch
			ofx.add.rloc
\
\ write IMPORTED resource request
			DRSC_TYPE_CODE DRSC_IMPORT or     \ type
			1                              \ many
			i dspp-symbols ofx.add.rsrc
		THEN
	LOOP
;


: labels>ofx { codeRsrcIndex | labelAddr branchHead -- , write DRSC_TYPE_CODE and relocate from it }
	dspp-num-labels @  0
	?DO
		i dspp-label-addrs @ -> labelAddr
		i dspp-branch-heads @ -> branchHead  \ has there been a branch to this label

		labelAddr DSPP_UNRESOLVED_LABEL = not
		IF
			dspp-echo @
			if
				." labels>ofx - " i print.label# ."  at $" labelAddr .hex
				." , branch at $" branchHead .hex cr
			then

			branchHead 0<
			IF
				." Unreferenced Label =  " i print.label# cr
			ELSE
\ write relocation request for all references in LIST format
				labelAddr   \ partnum is label address
				0
				codeRsrcIndex
				branchHead  \ code address of first linked branch
				ofx.add.rloc
			THEN

			i dspp-label-flags c@ LABEL_F_EXPORTED and
			IF
				DRSC_TYPE_CODE DRSC_EXPORT or DRSC_BIND or \ type
				0          \ many=0 -> pre allocated
				labelAddr  \ offset
				codeRsrcIndex     \ boundto
				0          \ defval
				i dspp-symbols \ $name
				ofx.add.resource
			THEN
		THEN
	LOOP
;


: dspp.scan.ref.list { head xcfa -- }
	depth >r
	head
	begin dup @ 0>  \ are there more nodes?
	while @ dup xcfa execute
	repeat
	drop
	depth r> -
	if
		." Stack error in dspp.scan.ref.list for "
		head drs_NamePtr + @ id. cr
		abort
	then
;

: ofx.report.drs { drs -- }
	dspp-echo @
	if
		." ------------" cr
		." Name = " drs drs_NamePtr + @ id. cr
		." Type  = $" drs drs_RsrcType + @ .hex cr
		." Many  = " drs drs_Many + @ . cr
		drs drs_BoundTo + @ ?dup
		IF ." BoundTo "  drs_NamePtr + @  id. cr
		THEN
		." AllocOffset = $" drs drs_AllocOffset + @ .hex cr
	then
;

: ofx.add.drs { drs -- }
	drs ofx.report.drs    \ for debugging
	drs ofx.add.drs.reloc
	drs ofx.add.drs.rsrc
;

: generic>ofx { drs -- }
	drs ofx.add.drs
;

: dini>ofx ( drs -- )
	( -- DataPtr Many WhenFlags RsrcIndex )
	>r
	r@ dini_DataPtr + @
	r@ drs_Many + @
	r@ dini_WhenFlags + @
	r@ dini_ResourcePFA + @ drs_Index + @
	ofx.add.dini
	r> drop
;

: rbase>ofx { drs | rbnd -- }
	drs ofx.report.drs    \ for debugging
\ traverse list of rbase references
	BEGIN
		drs rbase_RefNodeList + @   \ get head of list
		dup -> rbnd                     \ is it a real node
	WHILE
\ make relocation based on info in ref
		0 \ part
		0 \ icode
		ofx-rsrc-indx @
		rbnd rbnd_RefAddress + @
\ ." rbase>ofx - rbnd at " rbnd .hex cr
\ ." rbase>ofx - ref at " dup .hex cr
		ofx.add.rloc
\ remove node from head of list and free memory
		drs rbase_RefNodeList + sll.remove.head
		rbnd free abort" ERROR - rbase>ofx - rbnd FREE failed!"
	REPEAT
	drs ofx.add.drs.rsrc
;

: rsrc>dins   { drs -- , put resource to appropriate area }
\
\ ." rsrc>dins - Setting drs " drs .hex ."  index to " ofx-rsrc-indx @ . cr
	ofx-rsrc-indx @ drs drs_Index + !  \ for DINI chunk and binding
\
	drs drs_RsrcType + @ DRSC_TYPE_MASK and
	CASE
		DRSC_TYPE_RBASE  OF drs rbase>ofx ENDOF \ not exactly right !!!
		drs generic>ofx
	ENDCASE
;

: ofx.flush.areas ( -- , flush chunk images accumulated in RAM )
	p" MRSC" ofx-rsrc-area 8 + ofx-rsrc-area @ chunk>ofx \ new M2 specific
	p" DRLC" ofx-rloc-area 8 + ofx-rloc-area @ chunk>ofx
	p" DNMS" ofx-rnames-area 8 + ofx-rnames-area @ chunk>ofx
	p" DINI" ofx-dini-area 8 + ofx-dini-area @ chunk>ofx
;

: all.labels>ofx ( -- , write code, labels and branches to OFX file }
\ first write imported labels cuz they each have their own import resource
	imported.labels>ofx
\ write DRSC_TYPE_CODE request to resolve all branches
	ofx-rsrc-indx @ >r  \ save index for binding
	DRSC_TYPE_CODE DRSC_BOUND_TO or  eni-org @  p" Entry" ofx.add.rsrc
\ then write regular labels that reference this code resource
	r> labels>ofx
;

: dspp.form>ofx ( $formname -- )
\ ." Begin form = " dup count type
	iff.begin.form  ( -- form-pos )
\ ."  at pos = " dup . cr
\ .s cr

	depth >r
	dhdr>ofx
	code>ofx

	all.labels>ofx  \ write Code resource and labels
\
\ build DRSC and DRLC Chunks simultaneously
	dins-rsrc-list    ['] rsrc>dins    dspp.scan.ref.list

\ run this last after resource indices have been set
	dins-dini-list    ['] dini>ofx    dspp.scan.ref.list

	ofx.flush.areas
	dlink>ofx        \ write dynamic link info

\ report code and ticks used.
	." CodeSize = " eni-org @ .
	." , Ticks = " ofx-ticks-needed @ . cr

	ofx-custom-addr @
	if
		ofx-custom-name @
		ofx-custom-addr @
		ofx-custom-size @
		chunk>ofx
		ofx-custom-addr off
	then
	depth r> - abort" Stack error in }ins"
\ ." End form at pos = " dup . cr
\ .s cr
	iff.end.form
;

SUPPORT_SAMP_ENV [if]
: sample>ofx { $fifoname $samplename flags | srcfid numbytes -- }
	p" ATSM" iff.begin.form ( -- form-pos )
	p" HOOK" $fifoname count chunk>ofx
	flags EMBED_ATTACHED_SAMPLE and
	if
		0 -> numbytes
		$samplename count r/o create-file 0=
		if
			-> srcfid
			begin
				pad 2048 srcfid read-file drop
				dup numbytes + -> numbytes
				pad over ofx.write
				0> not
			until
			srcfid close-file drop
			numbytes . ." bytes written to embedded sample " $samplename count type cr
		else
			." Could not open file!" cr
			abort
		then
	else
		p" XREF" $samplename count chunk>ofx
	then
	iff.end.form
;

: envl>ofx { envl -- }
	p" ATNV" iff.begin.form ( -- form-pos )
		p" HOOK" envl envl_HookName + @ count chunk>ofx
		p" ENVL" iff.begin.form
			p" ENVH" envl size_envl_chunk chunk>ofx
			p" PNTS" envl envl_DataPtr + @
			envl envl_NumPoints + @ 2 cells * chunk>ofx
		iff.end.form
	iff.end.form
;
[then]

$ 1234babe constant INS_FILE_PAIR
create name-scratch 128 allot

: ins.file{ ( -- formpos INS_FILE_PAIR , begin 3INS file)
	iff-filename @ 0=
	if
		p" output.dsp"
	else
		iff-filename @
	then
	$iff.open
	p" 3INS" iff.begin.form ( -- formpos )
	ins-name @
	if
\		." Name == " dup count type cr
\ stick NUL at end of name chunk so template has NUL terminated names 950530
		ins-name @ name-scratch $move
		0  name-scratch c@  1+ name-scratch + c!
		p" NAME" name-scratch count 1+ chunk>ofx
		ins-name off
	then
	INS_FILE_PAIR
;


: }ins.file ( formpos INS_FILE_PAIR -- )
	INS_FILE_PAIR - abort" }ins.file - Stack error in assembly!"
[ SUPPORT_SAMP_ENV [if] ]
\ dump any ATSM chunks specified
	num-samples @ 0
	?do
		i fifo-names @
		i sample-names @
		i sample-flags @
		sample>ofx
	loop
	num-samples off

\ dump any ATNV chunks specified
	envelope-index @ 1+ 0
	?do
		i env-attachments envl>ofx
	loop
	-1 envelope-index !
[ [then] ]
	
	( formpos -- ) iff.end.form
	iff.close
;

\ -----------------------------------------------------------------
\ define CREATE words for allocated named resources

: drs.init { RsrcType RefType numparts drs -- , initialise generic drs structure }
	0 drs drs_Next + !
	numparts drs drs_NumParts + !
	numparts 0>
	IF
		numparts cells allocate abort" ERROR - insufficient host memory!"
\ ." Allocated " numparts . ."  parts at " dup .hex cr
		drs drs_PartRefs + !
		numparts 0
		DO
			-1  i drs drs.partref[]  !  \ initialize to "not yet used".
		LOOP
	ELSE
		0 drs drs_PartRefs + !
	THEN
	1 drs drs_Many + !
	-1 drs drs_Index + !
	0 drs drs_BoundTo + !
	0 drs drs_AllocOffset + !
	RefType drs drs_ReferenceType + !
	0 drs drs_ReferenceCount + !
	0 drs drs_DefaultValue + !
\
	RsrcType
\ OR with IMPORT/EXPORT flags
	dspp-if-export @ if DRSC_EXPORT or   dspp-if-export off then
	dspp-if-import @ if DRSC_IMPORT or   dspp-if-import off then
	drs drs_RsrcType + !
\
\ ." drs.init:" cr
\ drs drs_size dump cr
;

: CAPTURE.NAME  ( <name> -- <name> $name )
	>in @ >r
	bl parse-word
	tuck
	here place
	here
	swap 1+ ( for count byte )
	allot align
	r> >in !
;

: create.dsp.header  { type size reftype numparts | <name> drs -- addr }
	capture.name -> <name>
	create
	here -> drs
	size allot align
	type reftype numparts drs drs.init
\ ." set NamePtr to: " dup count type cr
	<name> drs drs_NamePtr + !
	drs
;

: ALLOC.RBASE# { baseReg numRegs | drs -- }
	dspp.m2.only
	DRSC_TYPE_RBASE DRSC_AT_ALLOC or
	rbase_size 0 1
	create.dsp.header  ( -- , make allocation node for RBASE )
		dup -> drs
		dins-rsrc-list sll.add.tail
		baseReg drs drs_AllocOffset + !
		numRegs drs drs_Many + !
		SLL_TERMINATOR  drs rbase_RefNodeList + !
	does> ( -m addr )
		dspp-rbase-ref on
;

: alloc.resource.parts  ( type size reftype numparts <name> -- addr )
	create.dsp.header  ( -- , make allocation node )
		dup dins-rsrc-list sll.add.tail
	does> ( addr )
		dup drs_ReferenceType + @ dspp-type-ref !
;

: alloc.resource  ( type size reftype <name> -- addr )
	1 alloc.resource.parts
;


: ALLOC.DATA.ARRAY { rsrcType refType many -- }
	rsrcType
	drs_size refType many alloc.resource.parts
	many swap drs_Many + !
;

: ALLOC.KNOB.ARRAY { defaultValue calcType many -- }
	DRSC_TYPE_KNOB  DRSC_AT_ALLOC or   calcType or.drsc.subtype
	drs_size _REFERENCE_EI many alloc.resource.parts
	>r
	many r@ drs_Many + !
	defaultValue r@ drs_DefaultValue + !
	r> drop
;

: ALLOC.KNOB.M2 ( defaultValue calcType -- )
	1 ALLOC.KNOB.ARRAY
;

: ALLOC.KNOB  ( min max default <name> -- )
	." ALLOC.KNOB is obsolete" cr
	nip nip 0 ALLOC.KNOB.M2
;

: ALLOC.TRIGGER ( <name> -- )
	DRSC_TYPE_TRIGGER drs_size _REFERENCE_TRIGGER  alloc.resource
;

: ALLOC.INPUT.ARRAY { many -- }
	DRSC_TYPE_INPUT _REFERENCE_I many alloc.data.array
;

: ALLOC.INPUT ( -- , make allocation node for INPUT )
	1 ALLOC.INPUT.ARRAY
;

: ALLOC.OUTPUT.ARRAY { many -- }
	DRSC_TYPE_OUTPUT  DRSC_AT_ALLOC or    \ set at ALLOC time
	_REFERENCE_I many alloc.data.array
;

: ALLOC.OUTPUT ( -- , make allocation node for OUTPUT )
	1 ALLOC.OUTPUT.ARRAY
;

: ALLOC.UNSIGNED.OUTPUT ( <name>  -- )
	DRSC_TYPE_OUTPUT  DRSC_AT_ALLOC or    \ set at ALLOC time
	KNOB_TYPE_RAW_UNSIGNED or.drsc.subtype \ unsigned signal type
	_REFERENCE_I 1 alloc.data.array
;

: ALLOC.VARIABLE.ARRAY { many -- }
	DRSC_TYPE_VARIABLE many 1 =
	dspp-if-import @ 0=  AND  \ 960520
	IF DRSC_AT_ALLOC or
	THEN    \ set at ALLOC time if a single value and not imported
	_REFERENCE_I many alloc.data.array
;

: ALLOC.VARIABLE ( -- , make allocation node for VARIABLE )
	1 ALLOC.VARIABLE.ARRAY
;

: ALLOC.DATA.DEFAULT { defaultVal AT_Flags RsrcType -- }
	RsrcType  AT_Flags or
	drs_size _REFERENCE_I 1 alloc.resource.parts >r
	1 r@ drs_Many + !
	defaultVal r@ drs_DefaultValue + !
	r> drop
;

: ALLOC.VARIABLE.DEFAULT ( defaultVal AT_Flags -- )
	DRSC_TYPE_VARIABLE alloc.data.default
;
: ALLOC.OUTPUT.DEFAULT ( defaultVal AT_Flags -- )
	DRSC_TYPE_OUTPUT alloc.data.default
;

: ALLOC.TICKS  ( N -- , allocate ticks, does not require a name or dic entry )
	dup ofx-ticks-needed !
	here >r
	drs_size allot align
	DRSC_TYPE_TICKS 0 1 r@ drs.init
	r@ dins-rsrc-list sll.add.head  \ 960711 Put Ticks first!
	1 r@ drs_ReferenceCount + !  \ do this so we don't abort on unreferenced resource
	r@ drs_Many + !
	c" Ticks" r@ drs_NamePtr + !
	r> drop
;

: ALLOC.DYNAMIC.LINKS ( $dlinks -- , name[s] of shared routines separated by commas )
	dspp-dynamic-links @ abort" ALLOC.DYNAMIC.LINKS already called!"
	dspp-dynamic-links !
;

: dini.setup ( ResPFA WhenFlags DataAddr DataMany dini -- )
	>r
	r@ drs_Many + !
	r@ dini_DataPtr + !
	r@ dini_WhenFlags + !
	r@ dini_ResourcePFA + !
	r> drop
;

: init.resource.array ( ResPFA WhenFlags DataAddr DataMany -- , declare initial data for a resource )
	0 dini_size 0 0
	create.dsp.header
		dup >r dini.setup
		r> dins-dini-list sll.add.tail
\ clear this so global doesn't mess up later code
\ 940609 Kludge %Q if you reference a DSPP variable, gets set
	0 dspp-type-ref !

;

: INIT.RESOURCE ( ResPFA WhenFlags DataValue -- , declare initial data for a resource )
	here >r , r> 1   \ lay down value into dictionary and point to it
	init.resource.array
;

: CONNECT.HARDWARE ( RsrcType -- )
	drs_size _REFERENCE_HARDWARE alloc.resource drop
;

: CONNECT.HARDWARE.ARRAY { RsrcType many -- }
	RsrcType  drs_size  _REFERENCE_HARDWARE  many alloc.resource.parts
	many swap drs_Many + !
;

: (alloc.infifo)  ( rsrcType -- )
	drs_size _REFERENCE_FIFO DRSC_FIFO_NUM_PARTS alloc.resource.parts
	drop
;
: ALLOC.INFIFO
	DRSC_TYPE_IN_FIFO (alloc.infifo)
;
: ALLOC.INFIFO.SUBTYPE  ( subType -- )
	DRSC_TYPE_IN_FIFO swap or.drsc.subtype
	(alloc.infifo)
;

: ALLOC.OUTFIFO
	DRSC_TYPE_OUT_FIFO drs_size _REFERENCE_FIFO DRSC_FIFO_NUM_PARTS alloc.resource.parts
	drop
;

: BIND.RESOURCE { offset target-rsrc drs -- }
	offset drs drs_AllocOffset + !
	target-rsrc drs drs_BoundTo + !
\ set bind flag
	drs drs_RsrcType + @
	DRSC_BIND or
	drs drs_RsrcType + !
\ mark target as bound to so we consider it referenced
	target-rsrc drs_RsrcType + @
	DRSC_BOUND_TO or
	target-rsrc drs_RsrcType + !
	dspp-type-ref off   \ clear because we probably referenced a resource
;

: ins.main{
;
: ins.shared{
	0 _ORG
;

: }ins.main  ( -- , put DSPP FORM to file )
	_SLEEP
\	dspp-version @ BULLDOG_VERSION <
\	IF
		_NOP
\	THEN
	p" DSPP" dspp.form>ofx
	ins.reset.vars
	dspp.init.labels
;

: }ins.shared  ( -- , put DSPS shared FORM to file )
	p" DSPS" dspp.form>ofx
	ins.reset.vars
	dspp.init.labels
;

: ins{
	ins.reset.vars
;


: }ins
	ins.file{
	ins.main{
	}ins.main
	}ins.file
	ins.reset.vars
;
