\ $Id: adpcmduck.j,v 1.3 1994/09/21 18:39:41 phil Exp $
\ This file is included by adpcmduck22s.ins or adpcmduck44s.ins !!!
\
\ 3DO DSPP code
\
\ DSPP stuff proprietary property of 3DO -- DO NOT DISTRIBUTE!!!!!
\
\ (c) Copyright 1993, 1994 by The Duck Corporation, written by Dan Miller
\ sections (c) Copyright 1993 by 3DO
\
\	adpcmduck.ins35	--	Fixes for release (sugg. by Phil Burk)
\						0xF77F indicates next word = indexes; hi byte=L, lo byte=R
\						No count, no tag -- still inits old way, though
\
\
\ This can be re-written with proper allocation and optimized DSPP 
\ code.  If Duck's audio init routine fails on GrabKnob("PlayControl"), 
\ it will skip initialization. In this case, no calls will be made to 
\ TweakKnob(); this should allow functional replacement of this code.
\
\ 940325 PLB Modified by Phil Burk to conform with 3DO standard practice
\		Problem areas are marked with a %B
\		Removed init code, use DINI chunk tables.
\		Add to global mixer instead of overwriting.
\		Use sign toggle for Phase state.
\		Use subroutine to save code memory.
\		General optimization, Ticks: 193 =>185, Code: 230 => 127
\		Added interpolation between frames to improve audio quality.for 22 KHz
\		Split sample grabbing between phases to prevent FIFO underflow
\ 940921 PLB Changed Scratch to I_Scratch

decimal

ALLOC.RBASE ourbase
ALLOC.INFIFO Input			\ This should be called InFIFO %B
0 $ 7FFF $ 7FFF ALLOC.KNOB Amplitude \ %T

ALLOC.IMEM SaveMulL
ALLOC.IMEM SaveMulR
ALLOC.IMEM SaveIndexL
ALLOC.IMEM SaveIndexR
ALLOC.IMEM SaveOutL
ALLOC.IMEM SaveOutR
ADPCM_DUCK_RATE 22050 = ?\ ALLOC.IMEM PreviousOutL		\ for interpolation
ADPCM_DUCK_RATE 22050 = ?\ ALLOC.IMEM PreviousOutR
ADPCM_DUCK_RATE 44100 = ?\ variable PreviousOutL		\ dummys
ADPCM_DUCK_RATE 44100 = ?\ variable PreviousOutR
ALLOC.IMEM SaveSamp
ALLOC.IMEM HitMagic										\ got magic so no sample yet
ALLOC.IMEM HeldSamp										\ fetched off phase
ALLOC.IMEM Phase       \ %Q should start at 1

\ constants:

89 constant LOGTBLSIZE
8 constant ADDTBLSIZE

\ do not change the order of these or other allocated resources
\ because that will mess up the initialisation
LOGTBLSIZE ALLOC.IMEM.ARRAY LogTbl
ADDTBLSIZE ALLOC.IMEM.ARRAY AddTbl

\ register assignments:
4 constant  Multiplier      \ static
6 constant  Index	       	\ static
7 constant  OutputSample    \ static
8 constant  dviTemp         \ scratch
9 constant  Delta           \ scratch
10 constant  EncodedSample  \ scratch, data in high nibble

$ 106 constant AudioOutL
$ 107 constant AudioOutR

$ 309 constant Acknowledge

\ Main entry point -------------------------------------------------------
 _LABEL: Starter

	ourbase _RBASE
	
ADPCM_DUCK_RATE 22050 = if
	I_FrameCount _A #$ 01 _AND								\ for 1/2 sample rate
	_BNE: OddFrame
then
 
\ here's the actual ADPCM stuff:
	
 _LABEL: PlaySamples

\ increment Phase; if=0, get new sample word every other time.
	Phase _%A #$ 8000 _+ \ alternate, doesn't need initialization
	_BMI: MinusPhase

\ Split FIFO accesses between two phases so that we don't pull more than
\ two values from FIFO. Pulling 3 could cause an underflow.

\ Positive Phase -----------------------------------------------------

\ Use sample that we may have got on minus phase
	SaveSamp _A HeldSamp _A _MOVE					\ use sample word that we got before
	HitMagic _A _TRA								\ did we just get a magic word?
	_BEQ: GoDecode
	
\ get indexii
	I_Scratch _A _= Input _A _TRL						\ read data from FIFO  (#1)
	SaveIndexL _A _= ACCUME 8 _>>' _TRL			
	SaveIndexR _A _= I_Scratch _A #$ FF _AND			\ L = hibyte, R = lo byte
	SaveSamp _A Input _A _MOVE						\ get new sample word    (#2)
	HitMagic _A #$ 0000 _MOVE						\ clear flag
	_JUMP: GoDecode
	
 _LABEL: MinusPhase \ -------------------------------------------------
 
\ get encoded data from FIFO
	HeldSamp _A _= Input _A _TRL					\ get sample word from FIFO (#1)
	ACCUME #$ F77F _-								\ Magic now so get index!
	_BNE: @NoMagic
	
\ request that indices and sample be read on next phase
	HitMagic _A #$ 0001 _MOVE				
	
_LABEL: @NoMagic

\ Now process low byte of saved sample 
	SaveSamp _%A 8 _<<' _TRL						\ if neg, use lo byte

\ -----------------------------------------------------------------------

_LABEL: GoDecode
 
\ right channel: -------------------------- RIGHT -----------------------

\ load up registers
	Multiplier _reg SaveMulR _A _MOVE					\ rFr..
	Index _reg SaveIndexR _A _MOVE
	EncodedSample _reg _= SaveSamp _A 4 _<<' _TRL				\ shift lo nyb up
	OutputSample _reg SaveOutR _A _MOVE
					
	_JSR: DecodeDuck

	SaveOutR _A OutputSample _reg _MOVE
	SaveMulR _A Multiplier _reg _MOVE					\ sFr..
	SaveIndexR _A Index _reg _MOVE

	
 \ left channel: ------------------------- LEFT -------------------------

 \ load up registers
	Multiplier _reg SaveMulL _A _MOVE					\ rFr..
	Index _reg SaveIndexL _A _MOVE
	EncodedSample _reg SaveSamp _A _MOVE				\ get lo nyb
	OutputSample _reg SaveOutL _A _MOVE

	_JSR: DecodeDuck

	SaveOutL _A OutputSample _reg _MOVE
	SaveMulL _A Multiplier _reg _MOVE					\ sFr..
	SaveIndexL _A Index _reg _MOVE

\ ----------------------------------------- OUTPUT ----------------------
\ These illegally add directly to the AudioOut variables.
\ If we fix this, we can route the output through effects, etc.
\ but we will break the early Horde games.  %B
ADPCM_DUCK_RATE 22050 = if
\ interpolate between samples on the same frame that we calculate them
	SaveOutR _A 1 _>>>' _TRA             				\ arithmetic shift right, new/2
	PreviousOutR _A #$ 4000 ACCUME _*+      			\ prev/2 + new/2
	ACCUME Amplitude _A AudioOutR _%A _CLIP _*+
	PreviousOutR _A SaveOutR _A _MOVE    				\ save for next 

	SaveOutL _A 1 _>>>' _TRA             				\ arithmetic shift right, new/2
	PreviousOutL _A #$ 4000 ACCUME _*+      			\ prev/2 + new/2
	ACCUME Amplitude _A AudioOutL _%A _CLIP _*+
	PreviousOutL _A SaveOutL _A _MOVE    				\ save for next 

	_JUMP: AllDone

_LABEL: OddFrame
then

	SaveOutL _A Amplitude _A AudioOutL _%A _CLIP _*+
	SaveOutR _A Amplitude _A AudioOutR _%A _CLIP _*+
	_JUMP: AllDone
	

\ subroutine used by both channels ---------------------------------------
_LABEL: DecodeDuck  ( -- , decode current channel based on registers )
 	EncodedSample _%reg #$ F000 _AND					\ hi nyb = encoded sample

	dviTemp _reg _= LogTbl _# Index _reg _+
	_NOP  												\ required for pipeline
	_NOP
	Multiplier _reg dviTemp _[reg] _MOVE				\ Multiplier = LogTbl[Index]

\ delta = (EncodedSample + 1/2) * Multiplier/4
	ACCUME _= EncodedSample _reg #$ 7000 _AND
	ACCUME #$ 0800 _OR   								\ for the +1/2
	Delta _reg _= Multiplier _reg  ACCUME _*			\ Delta=MulTbl[x]*LogTbl[Index]
	
\ apply sign bit, encoded sample is not two's complement
	EncodedSample _reg _TRA								\ TRA = TRansfer to Accume? (sets flags)
	_BPL: @DontNegate
	Delta _%reg _NEG
	_NOP												\ Pipeline, I suppose...
_LABEL:  @DontNegate

	OutputSample _%reg Delta _reg _CLIP _+				\ sample = prev +] delta
	
\ get next Multiplier:
	EncodedSample _reg 8 _>>' _TRL  					\ shift right by 12, AND with 0007, 
	
	ACCUME #$ 0070 4 _>>' _AND
	dviTemp _reg _= AddTbl _# ACCUME _+     			\ index into AddTbl
	_NOP  												\ 2 NOPs required for pipeline
	_NOP
 
	Index _%reg dviTemp _[reg] _CLIP _+					\ Index +]= AddTbl[x]
	_NOP												\ required for CC to be set
	_BGT: @PositiveIndex            					\ clip between 0 and 88
	Index _reg #$ 0000 _MOVE
	_JUMP: @LookupStepSize
	
_LABEL: @PositiveIndex
	Index _reg LOGTBLSIZE _# _-
	_BLT: @LookupStepSize
	Index _reg  LOGTBLSIZE 1- _# _MOVE

_LABEL: @LookupStepSize
	_RTS
	
 _LABEL: AllDone									
 
\ hack in an initialization table
create dini-chunk
here
	LOGTBLSIZE , \ many
	DINI_AT_ALLOC ,
	1 , \ index of LogTbl
	0 , \ reserved
	$ 7 , $ 8 , $ 9 , $ A , $ B , $ C , $ D , $ F , 
	$ 10 , $ 12 , $ 13 , $ 15 , $ 17 , $ 1A , $ 1C , $ 1F , 
	$ 22 , $ 26 , $ 29 , $ 2E , $ 32 , $ 37 , $ 3D , $ 43 , 
	$ 4A , $ 51 , $ 59 , $ 62 , $ 6C , $ 76 , $ 82 , $ 8F , 
	$ 9E , $ AD , $ BF , $ D2 , $ E7 , $ FE , $ 117 , $ 133 , 
	$ 152 , $ 174 , $ 199 , $ 1C2 , $ 1EF , $ 220 , $ 256 , $ 292 , 
	$ 2D4 , $ 31D , $ 36C , $ 3C4 , $ 424 , $ 48E , $ 503 , $ 583 , 
	$ 610 , $ 6AC , $ 756 , $ 812 , $ 8E1 , $ 9C4 , $ ABE , $ BD1 , 
	$ CFF , $ E4C , $ FBA , $ 114D , $ 1308 , $ 14EF , $ 1707 , $ 1954 , 
	$ 1BDD , $ 1EA6 , $ 21B7 , $ 2516 ,
	$ 28CB , $ 2CDF , $ 315C , $ 364C , 
	$ 3BBA , $ 41B2 , $ 4844 , $ 4F7E ,
	$ 5771 , $ 6030 , $ 69CE , $ 7463 , 
	$ 7FFF ,


	ADDTBLSIZE , \ many
	DINI_AT_ALLOC ,
	0 , \ index of AddTbl
	0 , \ reserved
    -1 , -1 , -1 , -1 , 2 , 4 , 6 , 8 ,

here swap - ofx-custom-size !

	dini-chunk ofx-custom-addr !
	$" DINI" ofx-custom-name !
	
}ins
}dspp
