\ @(#) sub_decode_adp4.j 96/08/01 1.7
\ register assignments for sub_decode_adp4.j
\
\ Phil Burk
\ Copyright 3DO 1995
\ All Rights Reserved
\ Proprietary and Confidential

anew task-sub_decode_adp4.j

\ register assignments
8 constant ADP4_DECODE_BASE_REG
ADP4_DECODE_BASE_REG
dup 1+ swap constant  ADPCM_REG_NewVal      \ latest result from decoder, expanded to 16 bits
dup 1+ swap constant  ADPCM_REG_StepSize    \ initialize to 7
dup 1+ swap constant  ADPCM_REG_StepIndex   \ static
ADP4_DECODE_BASE_REG - constant ADP4_DECODE_NUM_REGS
