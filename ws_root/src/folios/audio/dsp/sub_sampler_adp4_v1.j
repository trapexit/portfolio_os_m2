\ @(#) sub_sampler_adp4_v1.j 96/08/07 1.1
\ register assignments for sub_sampler_adp4_v1.ins
\
\ Phil Burk
\ Copyright 3DO 1995
\ All Rights Reserved
\ Proprietary and Confidential

anew task-sub_sampler_adp4_v1.j

\ register assignments
0 constant ADPCM_BASE_DATA_REG

ADPCM_BASE_DATA_REG
dup 1+ swap constant  ADPCM_REG_SampleHolder
dup 1+ swap constant  ADPCM_REG_CurState
dup 1+ swap constant  ADPCM_REG_OldVal    \ previous for interpolation, expanded to 16 bits
dup 1+ swap constant  ADPCM_REG_Phase

dup 1+ swap constant  ADPCM_REG_Output
dup 1+ swap constant  ADPCM_REG_SampleRate
dup 1+ swap constant  ADPCM_REG_Amplitude
dup 1+ swap constant  ADPCM_REG_FIFO_Data_Addr  \ point to hardware

dup ADP4_DECODE_BASE_REG > abort" ADP4 regs overlap decoder regs."
ADP4_DECODE_NUM_REGS + constant ADPCM_NUM_DATA_REGS
