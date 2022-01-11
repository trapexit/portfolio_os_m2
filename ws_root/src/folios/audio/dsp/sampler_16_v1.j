\ @(#) sampler_16_v1.j 96/01/17 1.4
\
\ 16 bit, 8 bit, and SQS2 instruments can share the same code
\ because the decompression is done in hardware.
\
\ Author: Phil Burk
\ Copyright 1993,4,5 3DO
\ All Rights Reserved

include? SM16_BASE_DATA_REG sampler_16_v1_regs.j

SM16_BASE_DATA_REG SM16_NUM_DATA_REGS ALLOC.RBASE#  DataRBase

ALLOC.OUTPUT  Output
	SM16_REG_OUTPUT SM16_BASE_DATA_REG - DataRBase Output BIND.RESOURCE

ALLOC.VARIABLE  Phase
	SM16_REG_PHASE SM16_BASE_DATA_REG - DataRBase Phase BIND.RESOURCE

$ 7FFF KNOB_TYPE_RAW_SIGNED ALLOC.KNOB.M2 Amplitude
	SM16_REG_AMPLITUDE SM16_BASE_DATA_REG - DataRBase Amplitude BIND.RESOURCE

$ 8000 KNOB_TYPE_SAMPLE_RATE ALLOC.KNOB.M2 SampleRate
	SM16_REG_SAMPLERATE SM16_BASE_DATA_REG - DataRBase SampleRate BIND.RESOURCE

\ allocate a FIFO then bind registers to allocated FIFO
SM16_BASE_FIFO_REG SM16_NUM_FIFO_REGS ALLOC.RBASE#  FIFORBase
	DRSC_FIFO_OSC InFIFO FIFORBase BIND.RESOURCE

$" sub_sampler_16_v1.dsp" ALLOC.DYNAMIC.LINKS

	DataRBase SM16_BASE_DATA_REG _RBASE#
	FIFORBase SM16_BASE_FIFO_REG _RBASE#
	_JSR:	sub_sampler_16_v1
