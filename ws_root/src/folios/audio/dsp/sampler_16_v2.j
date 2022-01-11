\ @(#) sampler_16_v2.j 95/09/22 1.2
\
\ 16 bit, 8 bit, and SQS2 instruments can share the same code
\ because the decompression is done in hardware.
\
\ Author: Phil Burk
\ Copyright 1993,4,5 3DO
\ All Rights Reserved


2 ALLOC.OUTPUT.ARRAY  Output
$ 7FFF KNOB_TYPE_RAW_SIGNED ALLOC.KNOB.M2 Amplitude
$ 8000 KNOB_TYPE_SAMPLE_RATE ALLOC.KNOB.M2 SampleRate
2 ALLOC.VARIABLE.ARRAY  OldVal
2 ALLOC.VARIABLE.ARRAY  NewVal
0 DRSC_AT_START ALLOC.VARIABLE.DEFAULT  Phase 
47 ALLOC.TICKS

\ Phase goes from 0->$7FFF
\ If it goes minus, normalize the phase to 0=>$7FFF, take a sample.
\ If it goes minus twice, take two samples.
\ If it stays positive, just interpolate.

	InFIFO FIFOSTATUS+ _A 4 _# _-
	_BLT:  @NoData

\ Calculate phase increment
	Phase _%A SampleRate _A _+
\ Determine whether to get 0,1,or 2 samples
	_BMI:	@Once
	_BCC:	@Interpolate

\ Get two frames
	OldVal 0 PART+ _A InFIFO _A	_MOVE 	\ OldVal<-FIFO
	OldVal 1 PART+ _A InFIFO _A	_MOVE 	\ OldVal<-FIFO
	_JUMP: @GetNewVal

_LABEL: @Once
\ Get one frame
	Phase _%A #$ 8000	_-		     \ wrap back to positive phase
	OldVal 0 PART+ _A NewVal 0 PART+ _A  _MOVE   \ OldVal<-newval
	OldVal 1 PART+ _A NewVal 1 PART+ _A  _MOVE   \ OldVal<-newval

_LABEL: @GetNewVal
	NewVal 0 PART+  _A InFIFO _A _MOVE           \ NewVal<-FIFO
	NewVal 1 PART+  _A InFIFO _A _MOVE           \ NewVal<-FIFO

_LABEL: @Interpolate
\ Phase is still in accumulator!!!
	ACCUME OldVal 0 PART+ _A OldVal 0 PART+ _A _*-   \ old*(frac-1)	= old*frac-old
	NewVal 0 PART+ _A Phase _A ACCUME _*-            \ interpolate new value
	Output 0 PART+ _A _= ACCUME Amplitude _A _*      \ scale output

	Phase _A   OldVal 1 PART+ _A OldVal 1 PART+ _A _*-   \ old*(frac-1)	= old*frac-old
	NewVal 1 PART+ _A Phase _A ACCUME _*-            \ interpolate new value
	Output 1 PART+ _A _= ACCUME Amplitude _A _*      \ scale output

_LABEL: @NoData
	_ENDEXT
