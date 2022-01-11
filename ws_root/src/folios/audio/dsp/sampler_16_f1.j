\ @(#) sampler_16_f1.j 96/03/07 1.2
\
\ 16 bit, 8 bit, and SQS2 instruments can share the same code
\ because the decompression is done in hardware.
\
\ Author: Phil Burk
\ Copyright 1993,4,5 3DO
\ All Rights Reserved

\ InFIFO allocated by caller
ALLOC.OUTPUT Output
$ 7FFF KNOB_TYPE_RAW_SIGNED ALLOC.KNOB.M2 Amplitude
6 ALLOC.TICKS

	Output _A _= InFIFO _A Amplitude _A _*	\ direct from FIFO to Output
