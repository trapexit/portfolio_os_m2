\ @(#) sampler_16_v1_regs.j 95/09/11 1.1
\
\ 16 bit, 8 bit, and SQS2 instruments can share the same code
\ because the decompression is done in hardware.
\
\ Author: Phil Burk
\ Copyright 1993,4,5 3DO
\ All Rights Reserved

\ These hardware definitions should be in an include file.
0 constant FIFO_CURRENT
1 constant FIFO_NEXT
2 constant FIFO_FREQ
3 constant FIFO_PHASE

0 constant SM16_BASE_DATA_REG
SM16_BASE_DATA_REG
dup 1+ swap constant  SM16_REG_OUTPUT
dup 1+ swap constant  SM16_REG_SAMPLERATE
dup 1+ swap constant  SM16_REG_AMPLITUDE
dup 1+ swap constant  SM16_REG_PHASE
SM16_BASE_DATA_REG - constant SM16_NUM_DATA_REGS

8 constant SM16_BASE_FIFO_REG
4 constant SM16_NUM_FIFO_REGS
